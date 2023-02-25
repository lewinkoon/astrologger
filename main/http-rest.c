#include <sys/param.h>

#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_tls.h"

#include "credentials.h"

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048

const char *TAG_REST = "REST";

void blink_led(bool led_state);

extern const char root_cert_pem_start[] asm("_binary_root_cert_pem_start");
extern const char root_cert_pem_end[] asm("_binary_root_cert_pem_end");

extern QueueHandle_t sendQueue;

static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer; // Buffer to store response of http request from event handler
    static int output_len;      // Stores number of bytes read
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG_REST, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG_REST, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG_REST, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG_REST, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG_REST, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        /*
         *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
         *  However, event handler can also be used in case chunked encoding is used.
         */
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            // If user_data buffer is configured, copy the response into the buffer
            int copy_len = 0;
            if (evt->user_data)
            {
                copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                if (copy_len)
                {
                    memcpy(evt->user_data + output_len, evt->data, copy_len);
                }
            }
            else
            {
                const int buffer_len = esp_http_client_get_content_length(evt->client);
                if (output_buffer == NULL)
                {
                    output_buffer = (char *)malloc(buffer_len);
                    output_len = 0;
                    if (output_buffer == NULL)
                    {
                        ESP_LOGE(TAG_REST, "Failed to allocate memory for output buffer");
                        return ESP_FAIL;
                    }
                }
                copy_len = MIN(evt->data_len, (buffer_len - output_len));
                if (copy_len)
                {
                    memcpy(output_buffer + output_len, evt->data, copy_len);
                }
            }
            output_len += copy_len;
        }

        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG_REST, "HTTP_EVENT_ON_FINISH");
        if (output_buffer != NULL)
        {
            // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
            // ESP_LOG_BUFFER_HEX(TAG_REST, output_buffer, output_len);
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG_REST, "HTTP_EVENT_DISCONNECTED");
        int mbedtls_err = 0;
        esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
        if (err != 0)
        {
            ESP_LOGI(TAG_REST, "Last esp error code: 0x%x", err);
            ESP_LOGI(TAG_REST, "Last mbedtls failure: 0x%x", mbedtls_err);
        }
        if (output_buffer != NULL)
        {
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG_REST, "HTTP_EVENT_REDIRECT");
        esp_http_client_set_header(evt->client, "From", "user@example.com");
        esp_http_client_set_header(evt->client, "Accept", "text/html");
        esp_http_client_set_redirection(evt->client);
        break;
    }
    return ESP_OK;
}

void http_request(void *parameters)
{
    char msgbuf[256];
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
    const char *post_data = "{\"time\":\"2099-12-31T00:00:00\",\"temperature\":\"99.99\",\"pressure\":\"999.99\"}";

    esp_http_client_config_t config = {
        .host = DB_HOST,
        .path = "/rest/v1/bmp280",
        .event_handler = _http_event_handler,
        // .crt_bundle_attach = esp_crt_bundle_attach,
        .cert_pem = root_cert_pem_start,
        .user_data = local_response_buffer,
        .buffer_size = 256,
        .buffer_size_tx = 1024,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "apikey", API_KEY);
    esp_http_client_set_header(client, "Authorization", AUTH_BEARER);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    while (1)
    {
        if (xQueueReceive(sendQueue, &msgbuf, (TickType_t)2))
        {
            blink_led(1);

            // printf("Received data from queue == %s\n", msgbuf);
            post_data = msgbuf;

            esp_http_client_set_post_field(client, post_data, strlen(post_data));
            esp_err_t err = esp_http_client_perform(client);
            if (err == ESP_OK)
            {
                ESP_LOGI(TAG_REST, "HTTP Status = %d, content_length = %llu\n",
                         esp_http_client_get_status_code(client),
                         esp_http_client_get_content_length(client));
            }
            else
            {
                ESP_LOGE(TAG_REST, "HTTP request failed: %s", esp_err_to_name(err));
            }

            blink_led(0);
        }
        // ESP_LOG_BUFFER_HEX(TAG_REST, local_response_buffer, strlen(local_response_buffer));
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    esp_http_client_cleanup(client);
    ESP_LOGI(TAG_REST, "Finish HTTP request");

    vTaskDelete(NULL);
}