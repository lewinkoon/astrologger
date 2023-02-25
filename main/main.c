#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "sdkconfig.h"

void configure_led(void);
void wifi_init_sta(void);
void sync_time(void);
void init_sensor(void);
void timer_callback(void *args);
void http_request(void *parameters);
QueueHandle_t sendQueue;

void app_main(void)
{
    // init  flash memory
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // configure led gpio
    configure_led();

    // initialize sensor data queue
    sendQueue = xQueueCreate(10, 128);

    // initialize sensor over i2c
    init_sensor();

    // connect to wifi
    wifi_init_sta();

    // sync time over ntp
    sync_time();

    // timer
    const esp_timer_create_args_t timer_args = {
        .callback = &timer_callback,
        .name = "measurement"};
    esp_timer_handle_t timer;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer));

    ESP_ERROR_CHECK(esp_timer_start_periodic(timer, 30000000));

    // send data to database
    xTaskCreate(&http_request, "http_request", 8196, NULL, 2, NULL);
}