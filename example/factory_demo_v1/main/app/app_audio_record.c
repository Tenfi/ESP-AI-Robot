/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_afe_sr_models.h"

#include "model_path.h"
#include "esp_vad.h"
#include "esp_process_sdkconfig.h"
// #include "bsp/esp-bsp.h"
// #include "file_iterator.h"
#include "esp_sparkbot_bsp.h"
#include "bsp_board_extra.h"

#include "baidu.h"
#include "app_audio_record.h"
#include "mmap_generate_audio.h"
#include "ui.h"

#define I2S_CHANNEL_NUM     (1)
#define MAX_AUDIO_INPUT_LENGTH  16000 * 2 * 8   // 8 seconds audio
#define MIN_AUDIO_INPUT_LENGTH  16000 * 2 * 600 / 1000
#define SPIFFS_BASE             "/audio"

#define AUDIO_STOP_BIT          BIT0
#define AUDIO_CHAT_BIT          BIT1

static const char *TAG = "app_sr";

typedef enum {
    MODE_IDLE,
    MODE_CHAT,
    MODE_IMAGE,
} running_mode_t;

typedef struct {
    size_t len;
    uint8_t *wav;
} audio_data_t;

static EventGroupHandle_t   g_stt_event_group;
static QueueHandle_t        g_audio_chat_queue  = NULL;
static QueueHandle_t        g_audio_tts_queue   = NULL;
static QueueHandle_t        g_queue_audio_play  = NULL;
static running_mode_t       g_running_mode      = MODE_CHAT;

static const esp_afe_sr_iface_t *afe_handle     = NULL;
static QueueHandle_t            g_result_que    = NULL;
static srmodel_list_t           *models         = NULL;

static esp_codec_dev_handle_t   spk_codec_dev   = NULL;
// static file_iterator_instance_t *file_iterator  = NULL;

static TaskHandle_t xFeedHandle;
static TaskHandle_t xDetectHandle;
static mmap_assets_handle_t asset_audio;

static bool                     g_voice_recording       = false;
static bool                     g_audio_playing         = false;
static bool                     g_face_updated          = false;
static size_t                   g_stt_recorded_length   = 0;
static int16_t                  *g_audio_record_buf     = NULL;

static void audio_play_task(void *arg)
{
    spk_codec_dev = bsp_extra_audio_codec_speaker_init();
    esp_codec_dev_set_out_vol(spk_codec_dev, 100);

    esp_codec_dev_sample_info_t fs = {
        .sample_rate        = 16000,
        .channel            = 1,
        .bits_per_sample    = 16,
    };

    esp_codec_dev_open(spk_codec_dev, &fs);
    audio_data_t audio_data = {0};

    while (xQueueReceive(g_queue_audio_play, &audio_data, portMAX_DELAY) == pdTRUE) {
        // ESP_LOGW(TAG, "audio_play_task: %d, wav: %p", audio_data.len, audio_data.wav);
        int res = esp_codec_dev_write(spk_codec_dev, audio_data.wav, audio_data.len);
        // ESP_LOGI(TAG, "esp_codec_dev_write %d", res);
        free(audio_data.wav);
    }

    esp_codec_dev_close(spk_codec_dev);
    vTaskDelete(NULL);
}

esp_err_t audio_play(uint8_t *wav, size_t len)
{
    audio_data_t audio_data = {
        .len = len,
        .wav = heap_caps_malloc(len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT),
    };

    if (audio_data.wav == NULL) {
        ESP_LOGE(TAG, "heap_caps_malloc failed");
        return ESP_OK;
    }

    memcpy(audio_data.wav, wav, len);

    xQueueSend(g_queue_audio_play, &audio_data, portMAX_DELAY);
    return ESP_OK;
}

static void audio_feed_task(void *pvParam)
{
    esp_afe_sr_data_t *afe_data = (esp_afe_sr_data_t *) pvParam;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    ESP_LOGI(TAG, "audio_chunksize=%d, feed_channel=%d", audio_chunksize, 3);
    /* Allocate audio buffer and check for result */
    int16_t *audio_buffer = heap_caps_malloc(audio_chunksize * sizeof(int16_t) * 3, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (NULL == audio_buffer) {
        esp_system_abort("No mem for audio buffer");
    }

    /* Set the input gain of the microphone, increase the gain if the input volume is too low */
    esp_codec_dev_handle_t mic_codec_dev = bsp_extra_audio_codec_microphone_init();

    esp_codec_dev_sample_info_t fs = {
        .sample_rate        = 16000,
        .channel            = 1,
        .bits_per_sample    = 16,
    };
    esp_codec_dev_open(mic_codec_dev, &fs);
    /* Set the input gain of the microphone, increase the gain if the input volume is too low */
    esp_codec_dev_set_in_gain(mic_codec_dev, 35.0);

    while (true) {
        /* Read audio data from I2S bus */
        esp_codec_dev_read(mic_codec_dev, audio_buffer, audio_chunksize * I2S_CHANNEL_NUM * sizeof(int16_t));

        /* Send audio data to recording */
        if (g_voice_recording) {

            /* Stop when 8 seconds is full */
            if (g_stt_recorded_length >= MAX_AUDIO_INPUT_LENGTH) {
                audio_record_state_t result = AUDIO_VAD_END;
                xQueueSend(g_result_que, &result, 10);
            }

            /* Data length to write */
            size_t bytes_to_write = audio_chunksize * sizeof(int16_t);

            /* Check if exceding max recording length (should not happen by theory) */
            if (g_stt_recorded_length + bytes_to_write > MAX_AUDIO_INPUT_LENGTH) {
                bytes_to_write = MAX_AUDIO_INPUT_LENGTH - g_stt_recorded_length;
            }

            if (g_stt_recorded_length + bytes_to_write <= MAX_AUDIO_INPUT_LENGTH) {
                memcpy(g_audio_record_buf + g_stt_recorded_length / 2, audio_buffer, bytes_to_write);
                /* Update recorded length */
                g_stt_recorded_length += bytes_to_write;
                // ESP_LOGI(TAG, "Recording: %d bytes written, total: %d", bytes_to_write, g_stt_recorded_length);
            } else {
                /* write to the end of the buffer, then wrap around and write the rest of the buffer. */
                ESP_LOGW(TAG, "Buffer full, unable to write more audio data");
                audio_record_state_t result = AUDIO_VAD_END;
                xQueueSend(g_result_que, &result, 10);
            }
        }

        /* Channel Adjust */
        // for (int  i = audio_chunksize - 1; i >= 0; i--) {
        //     audio_buffer[i * 3 + 2] = 0;
        //     audio_buffer[i * 3 + 1] = audio_buffer[i];
        //     audio_buffer[i * 3 + 0] = audio_buffer[i];
        // }
        for (int  i = audio_chunksize - 1; i >= 0; i--) {
            audio_buffer[i * 2 + 1] = 0;
            audio_buffer[i * 2 + 0] = audio_buffer[i];
        }

        /* Feed samples of an audio stream to the AFE_SR */
        afe_handle->feed(afe_data, audio_buffer);
    }

    /* Clean up if audio feed ends */
    afe_handle->destroy(afe_data);

    /* Task never returns */
    vTaskDelete(NULL);
}

static void audio_detect_task(void *pvParam)
{
    bool wait_speech_flag = false;
    bool detect_flag = false;
    vad_state_t vad_state = VAD_SILENCE;
    esp_afe_sr_data_t *afe_data = (esp_afe_sr_data_t *) pvParam;

    /* Check audio data chunksize */
    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);
    ESP_LOGI(TAG, "------------detect start------------\n");
    ESP_LOGI(TAG, "afe_chunksize: %d", afe_chunksize);

    while (true) {
        afe_fetch_result_t *res = afe_handle->fetch(afe_data);
        vad_state = res->vad_state;
        if (!res || res->ret_value == ESP_FAIL) {
            ESP_LOGE(TAG, "fetch error!");
            continue;
        }

        if (res->wakeup_state == WAKENET_DETECTED) {
            ESP_LOGI(TAG, LOG_BOLD(LOG_COLOR_GREEN) "Wakeword detected");
            audio_record_state_t result = AUDIO_WAKENET_START;
            xQueueSend(g_result_que, &result, 10);
            /* Update face UI */
            ui_send_sys_event(ui_face, LV_EVENT_FACE_ASK, NULL);
        } else if (res->wakeup_state == WAKENET_CHANNEL_VERIFIED) {
            ESP_LOGI(TAG, LOG_BOLD(LOG_COLOR_GREEN) "Channel verified");
            afe_handle->disable_wakenet(afe_data);
            wait_speech_flag = true;
        }
        // else if (vad_state == VAD_SPEECH) {
        //     // vTaskDelay(pdMS_TO_TICKS(500));
        //     detect_flag = true;

        // }
        if (wait_speech_flag) {
            if (vad_state == VAD_SPEECH) {
                ESP_LOGI(TAG, "speech detected, vad start");
                detect_flag = true;
                wait_speech_flag = false;
                audio_record_state_t result = AUDIO_VAD_START;
                xQueueSend(g_result_que, &result, 10);
            }
        }

        /* VAD detect */
        if (detect_flag) {
            // if (vad_state == VAD_SPEECH) {
            //     continue;
            // }

            if (vad_state == VAD_SILENCE) {
                ESP_LOGI(TAG, "wait for more audio");

                // 每隔 100 毫秒检查一次 vad_state
                for (int i = 0; i < 20; i++) {
                    vTaskDelay(pdMS_TO_TICKS(100));
                    res = afe_handle->fetch(afe_data);
                    vad_state = res->vad_state;

                    if (vad_state != VAD_SILENCE) {
                        break; // 如果不再是静音状态，退出循环
                    }
                }

                res = afe_handle->fetch(afe_data);
                vad_state = res->vad_state;
                if (vad_state == VAD_SILENCE) {
                    ESP_LOGI(TAG, "vad state: VAD_SILENCE");
                    audio_record_state_t result = AUDIO_VAD_END;
                    xQueueSend(g_result_que, &result, 10);
                    /* Update face UI */
                    ui_send_sys_event(ui_face, LV_EVENT_FACE_THINK, NULL);

                    afe_handle->enable_wakenet(afe_data);
                    detect_flag = false;
                }
                continue;
            }
        }
    }

    /* Clean up if audio feed ends */
    afe_handle->destroy(afe_data);

    /* Task never returns */
    vTaskDelete(NULL);
}

void audio_record_task(void *pvParam)
{
    while (true) {
        audio_record_state_t result;
        if (xQueueReceive(g_result_que, &result, portMAX_DELAY) == pdTRUE) {
            switch (result) {
            case AUDIO_PLAY_MUYU: {
                g_audio_playing = false;
                void *audio = (void *)mmap_assets_get_mem(asset_audio, MMAP_AUDIO_MUYU_WAV);
                uint32_t len = mmap_assets_get_size(asset_audio, MMAP_AUDIO_MUYU_WAV);
                esp_codec_dev_write(spk_codec_dev, audio, len);
                break;
            }
            case AUDIO_WAKENET_START: {
                ESP_LOGI(TAG, "wakenet start");
                g_audio_playing = false;
                void *audio = (void *)mmap_assets_get_mem(asset_audio, MMAP_AUDIO_ECHO_WAV);
                uint32_t len = mmap_assets_get_size(asset_audio, MMAP_AUDIO_ECHO_WAV);
                esp_codec_dev_write(spk_codec_dev, audio, len);
                break;
            }
            case AUDIO_WAKENET_END: {
                ESP_LOGI(TAG, "timeout");
                break;
            }
            case AUDIO_VAD_START: {
                ESP_LOGI(TAG, "VAD detect start");
                g_voice_recording = true;
                break;
            }
            case AUDIO_VAD_END: {
                ESP_LOGI(TAG, "VAD detect done");
                /* Ensure audio length is > 600ms */

                g_voice_recording = false;
                if (g_stt_recorded_length > MIN_AUDIO_INPUT_LENGTH) {
                    /* Run STT Task */
                    xEventGroupSetBits(g_stt_event_group, AUDIO_CHAT_BIT);
                }
                break;
            }
            default:
                break;
            }
        }
    }
    vTaskDelete(NULL);
}

void app_stt_task(void *arg)
{
    ESP_LOGI(TAG, "app_stt_task start");

    while (1) {

        xEventGroupWaitBits(g_stt_event_group, AUDIO_CHAT_BIT, pdTRUE, pdTRUE, portMAX_DELAY);

        baidu_asr_send_audio(g_audio_record_buf, g_stt_recorded_length, g_stt_recorded_length);
        g_stt_recorded_length = 0;

        /* Receive text from stt */
        char *message_content = NULL;
        baidu_asr_recv_text(&message_content);

        if (message_content == NULL) {
            ESP_LOGE(TAG, "message_content is NULL");
            ui_send_sys_event(ui_face, LV_EVENT_FACE_LOOK, NULL);
            continue;
        }

        /* Send text to tts task */
        QueueHandle_t audio_chat_queue = NULL;

        if (g_running_mode == MODE_CHAT) {
            audio_chat_queue = g_audio_chat_queue;
        }

        if (xQueueSend(audio_chat_queue, &message_content, 0) == pdFALSE) {
            free(message_content);
            message_content = NULL;
        }

        xEventGroupSetBits(g_stt_event_group, AUDIO_STOP_BIT);
    }

    ESP_LOGI(TAG, "app_stt_task end");
    vTaskDelete(NULL);
}

void audio_chat_task(void *arg)
{
    ESP_LOGI(TAG, "audio_chat_task start");
    char *chat_data;

    while (xQueueReceive(g_audio_chat_queue, &chat_data, portMAX_DELAY) == pdTRUE) {

        baidu_erniebot_send_request(chat_data);
        free(chat_data);
        chat_data = NULL;

        char *response_data = NULL;
        esp_err_t ret = baidu_erniebot_recv_response(&response_data);

        if (ret != ESP_OK || response_data == NULL) {
            ESP_LOGI(TAG, "No data from baidu_erniebot_recv_response");
            ui_send_sys_event(ui_face, LV_EVENT_FACE_LOOK, NULL);
            continue;
        }

        ESP_LOGI(TAG, "response_data: %s", response_data);

        /* Send text to tts task */
        QueueHandle_t audio_chat_queue = NULL;

        if (g_running_mode == MODE_CHAT) {
            audio_chat_queue = g_audio_tts_queue;
        }

        if (xQueueSend(audio_chat_queue, &response_data, 0) == pdFALSE) {
            free(response_data);
            response_data = NULL;
        }
    }

    ESP_LOGI(TAG, "audio_chat_task end");
    vTaskDelete(NULL);
}

void audio_tts_task(void *arg)
{
    ESP_LOGI(TAG, "audio_tts_task start");

    char *text = NULL;
    while (xQueueReceive(g_audio_tts_queue, &text, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI(TAG, "audio_tts_task, size: %d, text: %s", strlen(text), text);
        g_audio_playing = true;

        baidu_tts_send_text(text);
        free(text);
        text = NULL;

        uint8_t *data = NULL;

        g_face_updated = true;
        while (1) {
            size_t len = 0;
            size_t total_len = 0;

            if (baidu_tts_recv_audio(&data, &len, &total_len, g_audio_playing)) {
                if (data == NULL) {
                    ESP_LOGI(TAG, "tts audio data received is NULL");
                    break;
                } else {
                    // ESP_LOGI(TAG, "len: %d, total_len: %d", len, total_len);
                    if (g_face_updated) {
                        /* Update face UI */
                        ui_send_sys_event(ui_face, LV_EVENT_FACE_SPEAK, NULL);
                        g_face_updated = false;
                    }
                    audio_play(data, len);
                    free(data);
                    data = NULL;
                }
            } else {
                ESP_LOGI(TAG, "baidu_tts_recv_audio done");
                break;
            }
        }

        g_audio_playing = false;

        vTaskDelay(300);
        if (!g_audio_playing) {
            ui_send_sys_event(ui_face, LV_EVENT_FACE_LOOK, NULL);
        }

        ESP_LOGI(TAG, "heap after audio play, internal current: %d, minimum: %d, total current: %d, minimum: %d",
                 heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
                 heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
                 esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
    }

    ESP_LOGI(TAG, "audio_tts_task end");
    vTaskDelete(NULL);
}

void app_sr_paly_muyu()
{
    audio_record_state_t result = AUDIO_PLAY_MUYU;
    if(g_result_que){
        xQueueSend(g_result_que, &result, 10);
    }
}

static void app_sr_mmap_audio()
{
    const mmap_assets_config_t config = {
        .partition_label = "storage",
        .max_files = MMAP_AUDIO_FILES,
        .checksum = MMAP_AUDIO_CHECKSUM,
        .flags = {
            .mmap_enable = true,
            .app_bin_check = true,
        },
    };

    mmap_assets_new(&config, &asset_audio);
    ESP_LOGI(TAG, "stored_files:%d", mmap_assets_get_stored_files(asset_audio));
}

esp_err_t app_sr_start(void)
{
    /* SPIFF init */
    // bsp_spiffs_mount();
    app_sr_mmap_audio();

    /* Audio buffer init */
    g_audio_record_buf = heap_caps_malloc(MAX_AUDIO_INPUT_LENGTH + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    models = esp_srmodel_init("model");

    afe_handle = &ESP_AFE_SR_HANDLE;
    afe_config_t afe_config = AFE_CONFIG_DEFAULT();
    afe_config.pcm_config.mic_num = 1;
    afe_config.pcm_config.total_ch_num = 2;

    afe_config.wakenet_model_name = esp_srmodel_filter(models, ESP_WN_PREFIX, NULL);

    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(&afe_config);
    ESP_LOGI(TAG, "load wakenet:%s", afe_config.wakenet_model_name);

    g_result_que = xQueueCreate(1, sizeof(audio_record_state_t));
    ESP_RETURN_ON_FALSE(NULL != g_result_que, ESP_ERR_NO_MEM, TAG, "Failed create result queue");

    BaseType_t ret_val = xTaskCreatePinnedToCore(audio_feed_task, "Feed Task", 4 * 1024, afe_data, 5, &xFeedHandle, 0);
    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_FAIL, TAG,  "Failed create audio feed task");

    ret_val = xTaskCreatePinnedToCore(audio_detect_task, "Detect Task", 6 * 1024, afe_data, 5, &xDetectHandle, 1);
    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_FAIL, TAG,  "Failed create audio detect task");

    ret_val = xTaskCreatePinnedToCore(audio_record_task, "Audio Record Task", 4 * 1024, g_result_que, 1, NULL, 0);
    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_FAIL, TAG,  "Failed create audio handler task");

    /*
    Step 1: Send captured audio data to Speech-to-Text (STT) server for processing.
    Step 2: Receive and handle the text response from Baidu Cloud STT service.
    Step 3: Forward the received text response to the Text-to-Speech (TTS) server.
    Step 4: Forward the audio response to audio_play_task.
    */
    g_audio_chat_queue = xQueueCreate(16, sizeof(char *));
    g_stt_event_group = xEventGroupCreate();
    g_audio_tts_queue = xQueueCreate(16, sizeof(char *));
    g_queue_audio_play = xQueueCreate(1, sizeof(audio_data_t));

    xTaskCreatePinnedToCore(app_stt_task, "audio_stt", 1024 * 4, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(audio_chat_task, "audio_chat", 1024 * 6, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(audio_tts_task, "audio_tts", 1024 * 6, NULL, 6, NULL, 1);
    xTaskCreate(audio_play_task, "audio_play_task", 1024 * 5, NULL, 15, NULL);

    return ESP_OK;
}