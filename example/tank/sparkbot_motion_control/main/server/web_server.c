/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "math.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_vfs.h"
// #include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_camera.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "time.h"

#include "web_server.h"
#include "tracked_chassis_control.h"

#include "ui.h"

static const char *TAG = "HTTPServer";

#define EXAMPLE_MAX_STA_CONN       (3)

static esp_netif_t *wifi_ap_netif = NULL;
static esp_netif_t *wifi_sta_netif = NULL;
static time_t last_ping_time = 0;


void wifi_init_ap(void) {
    wifi_mode_t wifi_mode = WIFI_MODE_AP;

    ESP_LOGI(TAG, "Initializing ESP-Netif");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_sta_netif = esp_netif_create_default_wifi_sta();
    wifi_ap_netif = esp_netif_create_default_wifi_ap();

    ESP_LOGI(TAG, "Initializing Wi-Fi");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    uint8_t mac[6];
    char ssid_str[20];
    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    sprintf(ssid_str, "esp-sparkbot-%02X%02X", mac[4], mac[5]);

    ESP_ERROR_CHECK(esp_wifi_set_mode(wifi_mode));
    if (wifi_mode == WIFI_MODE_AP || wifi_mode == WIFI_MODE_APSTA) {
        wifi_config_t wifi_ap_config = {
            .ap = {
                .ssid = { 0 },
                .password = "",
                .channel = 8,
                .max_connection = EXAMPLE_MAX_STA_CONN,
                .authmode = WIFI_AUTH_OPEN,
            },
        };
        strncpy((char *)wifi_ap_config.ap.ssid, ssid_str, sizeof(wifi_ap_config.ap.ssid) - 1);
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
    }

    ESP_LOGI(TAG, "Starting Wi-Fi");
    ESP_ERROR_CHECK(esp_wifi_start());
}



httpd_handle_t server = NULL;
static TaskHandle_t ws_stream_task_handle = NULL;
static bool ws_stream_task_running = false;

static void ws_stream_task(void *param) {
    camera_fb_t *fb = NULL;
    uint8_t *jpeg_buf = NULL;
    size_t jpeg_buf_len = 0;

    static int64_t last_frame = 0;
    if (!last_frame) {
        last_frame = esp_timer_get_time();
    }

    ESP_LOGI(TAG, "WebSocket stream task started");

    while (ws_stream_task_running) {
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        /* Convert to JPEG */
        if (fb->format != PIXFORMAT_JPEG) {
            bool jpeg_converted = frame2jpg(fb, 80, &jpeg_buf, &jpeg_buf_len);
            static uint8_t fail_count = 0;
            if (!jpeg_converted) {
                fail_count++;
                if (fail_count > 5) {
                    ESP_LOGE(TAG, "JPEG compression failed too many times, restarting camera");
                    esp_restart();
                }
                ESP_LOGE(TAG, "JPEG compression failed");
                esp_camera_fb_return(fb);
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            } else {
                fail_count = 0;
            }
        } else {
            jpeg_buf = fb->buf;
            jpeg_buf_len = fb->len;
        }

        /* Send JPEG data via WebSocket */
        esp_err_t res = ws_async_send_image(jpeg_buf, jpeg_buf_len);
        if (res != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send image over WebSocket: %s", esp_err_to_name(res));
        }

        esp_camera_fb_return(fb);

        /* Calculate frame time and frame rate */
        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        // ESP_LOGI(TAG, "JPEG: %luKB %lums (%.1ffps)",
        //          (uint32_t)(jpeg_buf_len / 1024),
        //          (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time);

        /* 
         * Control the sending frequency
         * Please note that in the current version, do not set the delay too low, as it may cause network congestion.
         */
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    ESP_LOGI(TAG, "WebSocket stream task stopped");
    ws_stream_task_handle = NULL;
    vTaskDelete(NULL);
}

esp_err_t start_ws_stream() {
    if (ws_stream_task_running) {
        ESP_LOGW(TAG, "WebSocket stream task is already running");
        return ESP_ERR_INVALID_STATE;
    }

    if (ws_stream_task_handle) {
        vTaskDelete(ws_stream_task_handle);
        ws_stream_task_handle = NULL;
    }

    ws_stream_task_running = true;
    if (xTaskCreate(ws_stream_task, "ws_stream_task", 4096, NULL, 5, &ws_stream_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create WebSocket stream task");
        ws_stream_task_running = false;
        return ESP_FAIL;
    }

    last_ping_time = time(NULL);

    return ESP_OK;
}

esp_err_t stop_ws_stream() {
    if (!ws_stream_task_running) {
        ESP_LOGW(TAG, "WebSocket stream task is not running");
        return ESP_ERR_INVALID_STATE;
    }

    ws_stream_task_running = false;
    // if (ws_stream_task_handle) {
    //     vTaskDelete(ws_stream_task_handle);
    //     ws_stream_task_handle = NULL;
    // }

    last_ping_time = 0;

    return ESP_OK;
}

static esp_err_t init_webserver_fs() {
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/webserver",
      .partition_label = "webserver",
      .max_files = 5,
      .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
    }

    return ret;
}

/* Storage WebSocket infomation */
static struct async_resp_arg * g_resp_handle = NULL;

#define IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

/* Set HTTP response content type according to file extension(MIME) */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename)
{
    if (IS_FILE_EXT(filename, ".pdf")) {
        return httpd_resp_set_type(req, "application/pdf");
    } else if (IS_FILE_EXT(filename, ".html")) {
        return httpd_resp_set_type(req, "text/html");
    } else if (IS_FILE_EXT(filename, ".jpeg")) {
        return httpd_resp_set_type(req, "image/jpeg");
    } else if (IS_FILE_EXT(filename, ".ico")) {
        return httpd_resp_set_type(req, "image/x-icon");
    } else if (IS_FILE_EXT(filename, ".eot")) {
        return httpd_resp_set_type(req, "application/vnd.ms-fontobject");
    } else if (IS_FILE_EXT(filename, ".woff2")) {
        return httpd_resp_set_type(req, "font/woff2");
    } else if (IS_FILE_EXT(filename, ".css")) {
        return httpd_resp_set_type(req, "text/css");
    } else if (IS_FILE_EXT(filename, ".ttf")) {
        return httpd_resp_set_type(req, "font/ttf");
    } else if (IS_FILE_EXT(filename, ".js")) {
        return httpd_resp_set_type(req, "text/javascript");
    }
    /* This is a limited set only */
    /* For any other type always set as plain text */
    return httpd_resp_set_type(req, "text/plain");
}

/*
 * Web Server Endpoints
 */

/* API: GET files (All) */
static esp_err_t file_get_handler(httpd_req_t *req) {
    char filepath[ESP_VFS_PATH_MAX + 1024];
    if (strcmp(req->uri, "/") == 0 && !ws_stream_task_running) {
        snprintf(filepath, sizeof(filepath), "/webserver%s", "/index.html");
    } else if (strcmp(req->uri, "/") == 0 && ws_stream_task_running) {
        snprintf(filepath, sizeof(filepath), "/webserver%s", "/dup.html");
    } else {
        snprintf(filepath, sizeof(filepath), "/webserver%s", req->uri);
    }
    FILE *file = fopen(filepath, "r");
    if (!file) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    
    char buffer[128];
    int len;
    set_content_type_from_file(req, filepath);
    while ((len = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        httpd_resp_send_chunk(req, buffer, len);
    }
    fclose(file);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

/* API: GET stop ws send image (force)*/
static esp_err_t stop_ws_send_image_handler(httpd_req_t *req) {
    if (ws_stream_task_running) {
        esp_err_t ret = stop_ws_stream();
        if (ret != ESP_OK) {
            httpd_resp_send_500(req);
            return ret;
        }
    }

    httpd_resp_set_status(req, "307 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

/* WebSocket Handler */
/* reference: https://github.com/espressif/esp-idf/blob/master/examples/protocols/http_server/ws_echo_server/main/ws_echo_server.c */
static esp_err_t ws_handler(httpd_req_t *req) {

    if (!g_resp_handle) {
        g_resp_handle = (struct async_resp_arg*) malloc(sizeof(struct async_resp_arg));
    }
    g_resp_handle->hd = req->handle;
    g_resp_handle->fd = httpd_req_to_sockfd(req);

    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "WS Handshake done, the new connection was opened.");
        start_ws_stream();
        return ESP_OK;
    }
    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len, ERR %d", ret);
        return ret;
    }
    ESP_LOGI(TAG, "WS Received a frame, length: %d", ws_pkt.len);
    if (ws_pkt.len) {
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
    }

    switch (ws_pkt.type)
    {
    case HTTPD_WS_TYPE_TEXT:
        ESP_LOGI(TAG, "WS Received a text message: %s", (char*)ws_pkt.payload);
        const char * input = (char *)ws_pkt.payload;

        if (input != NULL && strlen(input) > 1) {
            if (*input == 'P') {
                ESP_LOGI(TAG, "WS Received a PING.");
                last_ping_time = time(NULL);
                break;
            } else {
                static float x_value = 0.0f;
                static float y_value = 0.0f;
                static bool has_triggered_running = false;
                if (sscanf((const char *)input, "x%f y%f", &x_value, &y_value) == 2) {
                    if (fabsf(x_value) >= 0.1 || fabsf(y_value) >= 0.1) {
                        if (!has_triggered_running) {
                            printf("running!\n");
                            ui_send_sys_event(ui_face, LV_EVENT_FACE_RUNNING, NULL);
                            has_triggered_running = true;
                        }
                    } else {
                        if (has_triggered_running) {
                            printf("look!\n");
                            ui_send_sys_event(ui_face, LV_EVENT_FACE_LOOK, NULL);
                            has_triggered_running = false;
                        }
                    }
                }
                tracked_chassis_motion_control(input);
            }
        }
        free(buf);
        break;
    case HTTPD_WS_TYPE_BINARY:
        ESP_LOGI(TAG, "WS Received a binary.");
        free(buf);
        break;
    case HTTPD_WS_TYPE_PING:
        ESP_LOGI(TAG, "WS Received a PING.");
        last_ping_time = time(NULL);
        break;
    default:
        ESP_LOGI(TAG, "WS Receive a package, type ID: %d", ws_pkt.type);
        free(buf);
        break;
    }

    return ESP_OK;
}

/* WebSocket Sender */
static esp_err_t ws_send_frame_wrapper(void * arg) {
    if (g_resp_handle == NULL) {
        ESP_LOGE(TAG, "g_resp_handle is NULL");
        stop_ws_stream();
        free(((httpd_ws_frame_t *)arg)->payload);
        free(arg);
        return ESP_ERR_HTTPD_BASE;
    }
    httpd_ws_send_frame_async(g_resp_handle->hd, g_resp_handle->fd, (httpd_ws_frame_t *)arg);
    free(((httpd_ws_frame_t *)arg)->payload);
    free(arg);
    return ESP_OK;
}

static esp_err_t ws_async_send(httpd_ws_frame_t* ws_pkt) {
    if (g_resp_handle == NULL) {
        ESP_LOGE(TAG, "g_resp_handle is NULL");
        return ESP_ERR_HTTPD_BASE;
    }
    return httpd_queue_work(g_resp_handle->hd, ws_send_frame_wrapper, ws_pkt);
}

esp_err_t ws_async_send_image(uint8_t *image_payload, size_t len) {
    httpd_ws_frame_t* ws_pkt = malloc(sizeof(httpd_ws_frame_t));
    ws_pkt->type = HTTPD_WS_TYPE_BINARY;
    ws_pkt->payload = image_payload;
    ws_pkt->len = len;

    esp_err_t ret = ws_async_send(ws_pkt);
    return ret;
}

/* WebSocket Event handler */
static void ws_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_base == ESP_HTTP_SERVER_EVENT) {
        switch (event_id)
        {
        case HTTP_SERVER_EVENT_ERROR:
        case HTTP_SERVER_EVENT_DISCONNECTED:
            if (g_resp_handle && ((esp_http_server_event_data *)event_data)->fd == g_resp_handle->fd) {
                ESP_LOGW(TAG, "WS Disconnected !");
                stop_ws_stream();
            }
            ESP_LOGW(TAG, "Client Disconnected!");
            break;
        
        default:
            break;
        }
    } 
}

/* WS Monitor */
void monitor_ws_connection(void *arg) {
    while (1) {
        time_t now = time(NULL);
        if (ws_stream_task_running && last_ping_time > 0 && (now - last_ping_time) > 5) { 
            ESP_LOGW(TAG, "No heartbeat detected for 1 second, stopping stream...");
            stop_ws_stream();
            last_ping_time = 0; 
        }
        vTaskDelay(pdMS_TO_TICKS(220));
    }
}

/* GET handler */
static esp_err_t http_server_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Recv GET: %s", req->uri);

    if (strcmp(req->uri, "/ws") == 0) {
        return ws_handler(req);
    }

    if (strcmp(req->uri, "/stop") == 0) {
        return stop_ws_send_image_handler(req);
    }

    return file_get_handler(req);
}

httpd_handle_t start_webserver() {
    ESP_ERROR_CHECK(init_webserver_fs());
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.stack_size = 8192;
    httpd_handle_t server = NULL;
    httpd_start(&server, &config);

    httpd_uri_t file_uri0 = { .uri = "/", .method = HTTP_GET, .handler = http_server_get_handler, .is_websocket = false };
    httpd_uri_t file_uri1 = { .uri = "/stop", .method = HTTP_GET, .handler = http_server_get_handler, .is_websocket = false };
    httpd_uri_t file_uri2 = { .uri = "/index.html", .method = HTTP_GET, .handler = http_server_get_handler, .is_websocket = false };
    httpd_uri_t file_uri3 = { .uri = "/assets/*", .method = HTTP_GET, .handler = http_server_get_handler, .is_websocket = false };
    httpd_uri_t file_uri4 = { .uri = "/img/*", .method = HTTP_GET, .handler = http_server_get_handler, .is_websocket = false };
    httpd_uri_t ws_uri = { .uri = "/ws", .method = HTTP_GET, .handler = ws_handler, .is_websocket = true };

    httpd_register_uri_handler(server, &file_uri0);
    httpd_register_uri_handler(server, &file_uri1);
    httpd_register_uri_handler(server, &file_uri2);
    httpd_register_uri_handler(server, &file_uri3);
    httpd_register_uri_handler(server, &file_uri4);
    httpd_register_uri_handler(server, &ws_uri);

    esp_event_handler_register(ESP_HTTP_SERVER_EVENT, ESP_EVENT_ANY_ID, ws_event_handler, NULL);

    xTaskCreate(monitor_ws_connection, "monitor_ws_connection", 2048, NULL, 5, NULL);
    
    return server;
}