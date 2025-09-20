// Copyright 2024 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

/*
 * Sends audio input to Baidu's Speech-to-Text (STT) service.
 * 
 * This function initializes an HTTP client if not already done,
 * sends the audio data in PCM format, and handles the connection
 * to the Baidu STT service.
 *
 * Parameters:
 *     audio: Pointer to the audio data to be sent.
 *     len: Length of the audio data.
 *     total_len: Total length of the data to be sent.
 *
 * Returns:
 *     ESP_OK on success.
 */
esp_err_t baidu_asr_send_audio(const int16_t *audio, size_t len, size_t total_len);

/*
 * Receives the text response from Baidu's STT service.
 *
 * This function fetches the headers of the response, reads the response data,
 * and parses the received text. It also cleans up the HTTP client after use.
 *
 * Parameters:
 *     text: Pointer to a string pointer that will hold the parsed text response.
 *
 * Returns:
 *     ESP_OK on success.
 */
esp_err_t baidu_asr_recv_text(char **text);

/*
 * Sends a request to Baidu's Ernie Bot AI service with the specified content.
 *
 * This function initializes an HTTP client, creates a JSON payload containing
 * the input content, and sends it to the Baidu Ernie Bot API for processing.
 *
 * Parameters:
 *     content: Pointer to the input text that will be sent to the AI service.
 *
 * Returns:
 *     ESP_OK if the request was initiated successfully, or an error code on failure.
 */
esp_err_t baidu_erniebot_send_request(const char *content);

/*
 * Receives the response from Baidu's Ernie Bot AI service.
 *
 * This function fetches the headers of the response, reads the response data,
 * and stores it in the provided pointer. It also cleans up the HTTP client after use.
 *
 * Parameters:
 *     response_data: Pointer to a pointer that will hold the received response data.
 *
 * Returns:
 *     ESP_OK if the response was read successfully, or an error code on failure.
 */
esp_err_t baidu_erniebot_recv_response(char **response_data);

/*
 * Sends text input to Baidu's Text-to-Speech (TTS) service.
 *
 * This function initializes an HTTP client, creates a JSON payload with the text,
 * and sends the request to the Baidu TTS service to convert the text to audio.
 *
 * Parameters:
 *     text: Pointer to the input text that needs to be converted to audio.
 *
 * Returns:
 *     ESP_OK on success.
 */
esp_err_t baidu_tts_send_text(const char *text);

/*
 * Receives the audio response from Baidu's TTS service.
 *
 * This function fetches the headers of the response, reads the audio data,
 * and stores it in the provided pointer. It also cleans up the HTTP client after use.
 *
 * Parameters:
 *     data: Pointer to a pointer that will hold the received audio data.
 *     len: Pointer to a variable that will hold the length of the received audio data.
 *     total_len: Pointer to a variable that will hold the total length of the audio data.
 *
 * Returns:
 *     ESP_OK on success.
 */
bool baidu_tts_recv_audio(uint8_t **audio, size_t *len, size_t *total_len, bool recv);


#ifdef __cplusplus
}
#endif /**< _cplusplus */
