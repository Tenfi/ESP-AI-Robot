/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <string.h>
#include <sys/param.h>
#include <stdlib.h>
#include <ctype.h>

#include "esp_log.h"
#include "esp_event.h"
#include "esp_tls.h"

#include "cJSON.h"
#include "esp_http_client.h"

#include "baidu.h"

static const char *TAG = "baidu_speech";

esp_http_client_handle_t baidu_erniebot_http_handle = NULL;

/*
 * Parses the data response from the Baidu API at the following endpoint:
 * http://vop.baidu.com/server_api
 *
 * This function handles the response format and extracts relevant information
 * for further processing in the application.
 */
static char *baidu_stt_response_parse(const char *data, size_t len)
{
    cJSON *root = cJSON_Parse(data);
    if (root == NULL) {
        ESP_LOGI(TAG, "Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        return NULL; 
    }

    cJSON *result_node = cJSON_GetObjectItem(root, "result");
    if (result_node == NULL || result_node->type != cJSON_Array) {
        ESP_LOGI(TAG, "Error: 'result' key not found in JSON.\n");
        ESP_LOGI(TAG, "Data received: %s", data);
        cJSON_Delete(root);
        return NULL; 
    }

    cJSON *first_result = cJSON_GetArrayItem(result_node, 0);
    if (first_result == NULL || first_result->type != cJSON_String) {
        ESP_LOGI(TAG, "Error: First item in 'result' is not a string.\n");
        ESP_LOGI(TAG, "Data received: %s", data);
        cJSON_Delete(root);
        return NULL;
    }

    char *result_value = NULL;
    asprintf(&result_value, "%s", first_result->valuestring);
    cJSON_Delete(root);

    ESP_LOGI(TAG, "result_value: %s", result_value);

    return result_value;
}

/*
 * Parses the data response from the Baidu API at the following endpoint:
 * https://aip.baidubce.com/rpc/2.0/ai_custom/v1/wenxinworkshop/chat/ernie-lite-8k
 *
 * This function handles the response format and extracts relevant information
 * for further processing in the application.
 */
static char *baidu_content_response_parse(const char *data, size_t len)
{
    cJSON *root = cJSON_Parse(data);
    if (root == NULL) {
        printf("Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        goto err;
    }

    cJSON *result_node = cJSON_GetObjectItem(root, "result");
    if (result_node == NULL || !cJSON_IsString(result_node)) {
        printf("Error: 'result' key not found or not a string.\n");
        goto err;
    }

    char *result_value = strdup(result_node->valuestring);

    cJSON_Delete(root);
    return result_value;

err:
    cJSON_Delete(root);
    return NULL;
}

static char *baidu_content_stream_response_parse(const char *data, size_t len)
{
    cJSON *root = cJSON_Parse(data);
    if (root == NULL) {
        ESP_LOGE(TAG, "Error parsing JSON: %s", cJSON_GetErrorPtr());
        goto err;
    }

    cJSON *is_end = cJSON_GetObjectItem(root, "is_end");
    if (is_end == NULL || !cJSON_IsBool(is_end)) {
        ESP_LOGE(TAG, "Error: response is not stream");
        goto err;
    }

    if (cJSON_IsTrue(is_end)) {
        ESP_LOGI(TAG, "response is end");
        goto err;
    }

    cJSON *result_node = cJSON_GetObjectItem(root, "result");
    if (result_node == NULL || !cJSON_IsString(result_node)) {
        ESP_LOGE(TAG, "Error: 'result' key not found or not a string.");
        goto err;
    }

    char *result_value = strdup(result_node->valuestring);

    cJSON_Delete(root);
    return result_value;

err:
    cJSON_Delete(root);
    return NULL;
}

#define BAIDUBCE_MESSAGE_FORMAT "{\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}],\"system\":\"你的回答要尽量简洁，不超过 200 字\",\"stream\":false}"
#define BAIDU_LLM_URL            "https://aip.baidubce.com/rpc/2.0/ai_custom/v1/wenxinworkshop/chat/ernie-lite-8k?access_token=%s"

esp_err_t baidu_erniebot_send_request(const char *content)
{
    if (content == NULL) {
        ESP_LOGI(TAG, "NULL content input");
        return ESP_OK;
    }

    char* url_str = NULL;
    asprintf(&url_str, BAIDU_LLM_URL, CONFIG_BAIDU_LLM_ACCESS_TOKEN);

    esp_http_client_config_t config = {
        .timeout_ms = 30000,
        // .user_data              = local_response_buffer,
        .buffer_size = 1460,
        .url = url_str,
        .method = HTTP_METHOD_POST,
        .skip_cert_common_name_check = true,
    };

    /* Set the headers */
    baidu_erniebot_http_handle = esp_http_client_init(&config);
    esp_http_client_set_header(baidu_erniebot_http_handle, "Content-Type", "application/json");

    /* Create JSON payload with format and content */
    char *json_payload = NULL;
    asprintf(&json_payload, BAIDUBCE_MESSAGE_FORMAT, content);
    ESP_LOGI(TAG, "json_payload: %s", json_payload);

    esp_http_client_open(baidu_erniebot_http_handle, strlen(json_payload));
    esp_http_client_write(baidu_erniebot_http_handle, json_payload, strlen(json_payload));

    free(json_payload);
    json_payload = NULL;

    return ESP_OK;  
}

esp_err_t baidu_erniebot_recv_response(char **response_data)
{
    int content_length  = esp_http_client_fetch_headers(baidu_erniebot_http_handle);
    char *output_buffer = heap_caps_calloc(1, content_length + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    // int read_length = esp_http_client_read_response(baidu_erniebot_http_handle, output_buffer, content_length);
    int read_length = esp_http_client_read_response(baidu_erniebot_http_handle, output_buffer, content_length);

    if (read_length > 0) {
        ESP_LOGI(TAG, "read_length: %d output_buffer: %s", read_length, output_buffer);
        *response_data = baidu_content_response_parse(output_buffer, read_length);
        // *response_data = baidu_content_response_parse(output_buffer, content_length);
    } else {
        ESP_LOGE(TAG, "Failed to read any data from response");
        esp_http_client_close(baidu_erniebot_http_handle);
        esp_http_client_cleanup(baidu_erniebot_http_handle);
        baidu_erniebot_http_handle = NULL;
        return ESP_OK;
    }

    esp_http_client_close(baidu_erniebot_http_handle);
    esp_http_client_cleanup(baidu_erniebot_http_handle);
    baidu_erniebot_http_handle = NULL;

    free(output_buffer);
    output_buffer = NULL;

    return ESP_OK;
}


#define BAIDUBCE_STT_URL            "http://vop.baidu.com/server_api?dev_pid=1537&cuid=123456PHP&token=%s"
/*
* The following code sends an audio input to Baidu's speech-to-text (STT) service,
* receives a text response, and processes it.
*/
static esp_http_client_handle_t g_baidu_asr_http_handle = NULL;

esp_err_t baidu_asr_send_audio(const int16_t *audio, size_t len, size_t total_len)
{
    char *url_str = NULL;
    asprintf(&url_str, BAIDUBCE_STT_URL, CONFIG_BAIDU_AUDIO_ACCESS_TOKEN);

    if (!g_baidu_asr_http_handle) {
        esp_http_client_config_t config = {
            .url = url_str,
            .method = HTTP_METHOD_POST,
            .skip_cert_common_name_check = true,
            .buffer_size = 1460,
            .timeout_ms = 30000,
        };

        /* Set the headers */
        g_baidu_asr_http_handle = esp_http_client_init(&config);
        esp_http_client_set_header(g_baidu_asr_http_handle, "Content-Type", "audio/pcm;rate=16000");

        esp_http_client_open(g_baidu_asr_http_handle, total_len);
    }

    for (int written_len = 0; written_len < len; ) {
        written_len += esp_http_client_write(g_baidu_asr_http_handle, (char *)(audio + written_len), len - written_len);
    }

    return ESP_OK;
}

esp_err_t baidu_asr_recv_text(char **text)
{
    int content_length = esp_http_client_fetch_headers(g_baidu_asr_http_handle);

    char *output_buffer = heap_caps_calloc(1, content_length + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    int read_length = esp_http_client_read_response(g_baidu_asr_http_handle, output_buffer, content_length);
    if (read_length > 0) {
        *text = baidu_stt_response_parse(output_buffer, content_length);
    } else {
        ESP_LOGE(TAG, "Failed to read any data from response");
    }

    free(output_buffer);
    output_buffer = NULL;
    esp_http_client_close(g_baidu_asr_http_handle);
    esp_http_client_cleanup(g_baidu_asr_http_handle);
    g_baidu_asr_http_handle = NULL;

    return ESP_OK;
}

/*
* The following code sends a text input to Baidu's text-to-speech (TTS) service,
* receives an audio response, and processes it.
* 
*/
static esp_http_client_handle_t g_baidu_tts_http_handle = NULL;

esp_err_t baidu_tts_send_text(const char *text)
{
    esp_http_client_config_t config = {
        .url = "https://tsn.baidu.com/text2audio",
        .method = HTTP_METHOD_POST,
        .timeout_ms = 30000,
        .buffer_size = 1460,
        .skip_cert_common_name_check = true,
    };

    // Set the headers
    g_baidu_tts_http_handle = esp_http_client_init(&config);
    esp_http_client_set_header(g_baidu_tts_http_handle, "Content-Type", "application/json");


    #define BAIDU_TTS_FORMAT "tex=%s&tok=%s&cuid=12345postman&ctp=1&lan=zh&spd=6&pit=5&vol=8&per=4100&aue=6&="

    char *payload = NULL;
    // Create JSON payload with model, max tokens, and content
    asprintf(&payload, BAIDU_TTS_FORMAT, text, CONFIG_BAIDU_AUDIO_ACCESS_TOKEN);
    // ESP_LOGI(TAG, "payload: %s", payload);
    esp_http_client_open(g_baidu_tts_http_handle, strlen(payload));
    esp_http_client_write(g_baidu_tts_http_handle, payload, strlen(payload));
    free(payload);
    payload = NULL;
    return ESP_OK;
}

bool baidu_tts_recv_audio(uint8_t **data, size_t *len, size_t *total_len, bool recv)
{
    static int s_baidu_tts_audio_total_len = -1;
    if (recv) {
        if (s_baidu_tts_audio_total_len == -1) {
            s_baidu_tts_audio_total_len = esp_http_client_fetch_headers(g_baidu_tts_http_handle);
            ESP_LOGI(TAG, "s_baidu_tts_audio_total_len: %d", s_baidu_tts_audio_total_len);
            *total_len = s_baidu_tts_audio_total_len;
        }

        /* 500 ms audio */
        int audio_length = 16000 * 2 * 500 / 1000;
        // int audio_length = s_baidu_tts_audio_total_len;
        uint8_t *output_buffer = heap_caps_calloc(1, audio_length, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

        /* Read 500 ms for audio playing */
        int read_length = esp_http_client_read(g_baidu_tts_http_handle, (char *)output_buffer, audio_length);
        // ESP_LOGI(TAG, "s_baidu_tts_audio_total_len: %d, read_length: %d", s_baidu_tts_audio_total_len, read_length);

        if (read_length > 0) {
            *len = read_length;
            *data = output_buffer;
            return true;
        } else {
            free(output_buffer);
            output_buffer = NULL;
            goto end;
        }
    } else {
        goto end;
    }

    return false;

end:
    ESP_LOGW(TAG, "End of audio response");
    *data = NULL;
    esp_http_client_close(g_baidu_tts_http_handle);
    esp_http_client_cleanup(g_baidu_tts_http_handle);
    g_baidu_tts_http_handle = NULL;
    s_baidu_tts_audio_total_len = -1;
    return false;
}
