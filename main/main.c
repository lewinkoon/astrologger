#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "sdkconfig.h"

void wifi_init_sta(void);
void read_data(void *parameters);
void http_request(void *parameters);
extern QueueHandle_t sendQueue;

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    sendQueue = xQueueCreate(10, 128);

    ESP_LOGI("WIFI", "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    xTaskCreate(&read_data, "read_data", 8192, NULL, 5, NULL);

    vTaskDelay(10000 / portTICK_PERIOD_MS);

    ESP_LOGI("REST", "Initialize HTTP request");
    xTaskCreate(&http_request, "http_request", 8192, NULL, 5, NULL);
}
