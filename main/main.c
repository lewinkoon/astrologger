#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_netif_sntp.h"
#include "lwip/ip_addr.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "sdkconfig.h"

void configure_led(void);
void wifi_init_sta(void);
void read_data(void *parameters);
void http_request(void *parameters);
extern QueueHandle_t sendQueue;

void app_main(void)
{
    configure_led();

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    sendQueue = xQueueCreate(10, 128);
    wifi_init_sta();

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    ESP_LOGI("TIME", "Time: %u", timeinfo.tm_year);
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&config);

    xTaskCreate(&read_data, "read_data", 8192, NULL, 5, NULL);

    vTaskDelay(5000 / portTICK_PERIOD_MS);

    xTaskCreate(&http_request, "http_request", 8192, NULL, 5, NULL);
}