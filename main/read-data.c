#include <stdio.h>
#include "bmp280.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "ssd1306.h"

const char *TAG_I2C = "I2C";

// esp_err_t i2c_master_init(void);
extern QueueHandle_t sendQueue;

int8_t i2c_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt);
int8_t i2c_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt);

// display
SSD1306_t ssd1306;

// sensor variables
time_t now;
struct tm timeinfo;
int8_t rslt = BMP280_OK;
double temp;
double pres;
char timestamp[32];
char data_buff[128];
struct bmp280_dev bmp280;
struct bmp280_uncomp_data bmp280_data;

void init_display(void)
{
    i2c_master_init(&ssd1306, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
    ESP_LOGI(TAG_I2C, "I2C initialized successfully");

    ssd1306_init(&ssd1306, 128, 64);
    ssd1306_clear_screen(&ssd1306, false);
    ssd1306_contrast(&ssd1306, 0xff);
    ssd1306_display_text(&ssd1306, 0, "Time", 4, false);
    ssd1306_display_text(&ssd1306, 3, "Temperature", 11, false);
    ssd1306_display_text(&ssd1306, 6, "Pressure", 8, false);
}

static void delay_ms(uint32_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

void init_sensor(void)
{
    init_display();

    bmp280.dev_id = BMP280_I2C_ADDR_PRIM;
    bmp280.intf = BMP280_I2C_INTF;
    bmp280.read = (void *)i2c_read;
    bmp280.write = (void *)i2c_write;
    bmp280.delay_ms = (void *)delay_ms;

    rslt = bmp280_init(&bmp280);
    ESP_LOGI(TAG_I2C, "BMP280 INIT %d", rslt);

    bmp280.conf.os_temp = BMP280_OS_2X;
    bmp280.conf.os_pres = BMP280_OS_16X;
    bmp280.conf.odr = BMP280_ODR_1000_MS;
    bmp280.conf.filter = BMP280_FILTER_COEFF_16;

    // calibration default values
    // bmp280.calib_param.dig_t1 = 27504;
    // bmp280.calib_param.dig_t2 = 26435;
    // bmp280.calib_param.dig_t3 = -1000;
    // bmp280.calib_param.dig_p1 = 36477;
    // bmp280.calib_param.dig_p2 = -10685;
    // bmp280.calib_param.dig_p3 = 3024;
    // bmp280.calib_param.dig_p4 = 2855;
    // bmp280.calib_param.dig_p5 = 140;
    // bmp280.calib_param.dig_p6 = -7;
    // bmp280.calib_param.dig_p7 = 15500;
    // bmp280.calib_param.dig_p8 = -14600;
    // bmp280.calib_param.dig_p9 = 6000;

    rslt = bmp280_set_config(&bmp280.conf, &bmp280);
    ESP_LOGI(TAG_I2C, "BMP280 SET CONFIG %d", rslt);
    ESP_LOGI(TAG_I2C, "BMP280 SET CONFIG OS_TEMP %u", bmp280.conf.os_temp);
    ESP_LOGI(TAG_I2C, "BMP280 SET CONFIG OS_PRESS %u", bmp280.conf.os_pres);
    ESP_LOGI(TAG_I2C, "BMP280 SET CONFIG ODR %u", bmp280.conf.odr);
    ESP_LOGI(TAG_I2C, "BMP280 SET CONFIG FILTER %u", bmp280.conf.filter);

    // ESP_ERROR_CHECK(i2c_driver_delete(I2C_NUM_0));
    // ESP_LOGI(TAG_I2C, "I2C de-initialized successfully");
}

void timer_callback(void *args)
{
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%X", &timeinfo);

    rslt = bmp280_set_power_mode(BMP280_FORCED_MODE, &bmp280);

    vTaskDelay(100 / portTICK_PERIOD_MS);

    rslt = bmp280_get_uncomp_data(&bmp280_data, &bmp280);
    // ESP_LOGI(TAG_I2C, "BMP280 RAW %d", rslt);
    // ESP_LOGI(TAG_I2C, "BMP280 RAW TEMP VALUE %ld", bmp280_data.uncomp_temp);
    // ESP_LOGI(TAG_I2C, "BMP280 RAW PRESS VALUE %ld", bmp280_data.uncomp_press);

    rslt = bmp280_get_comp_temp_double(&temp, bmp280_data.uncomp_temp, &bmp280);
    // ESP_LOGI(TAG_I2C, "BMP280 COMPENSATE TEMP %d", rslt);
    // ESP_LOGI(TAG_I2C, "BMP280 COMPENSATE TEMP VALUE %f", temp);

    rslt = bmp280_get_comp_pres_double(&pres, bmp280_data.uncomp_press, &bmp280);
    // ESP_LOGI(TAG_I2C, "BMP280 COMPENSATE PRES %d", rslt);
    // ESP_LOGI(TAG_I2C, "BMP280 COMPENSATE PRES VALUE %f", pres);

    snprintf(data_buff, sizeof(data_buff), "{\"time\":\"%s\",\"temperature\":\"%.2f\",\"pressure\":\"%.2f\"}",
             timestamp,
             temp - 2,
             pres / 100);

    ESP_LOGI(TAG_I2C, "%s", data_buff);
    xQueueSend(sendQueue, &data_buff, (TickType_t)0);

    char show_time[9];
    strftime(show_time, sizeof(show_time), "%X", &timeinfo);
    ssd1306_display_text(&ssd1306, 1, show_time, 9, false);

    char show_temp[10];
    sprintf(show_temp, "%.2f degC", temp - 2);
    ssd1306_display_text(&ssd1306, 4, show_temp, 10, false);

    char show_pres[10];
    sprintf(show_pres, "%.2f hPa", pres / 100);
    ssd1306_display_text(&ssd1306, 7, show_pres, 10, false);
}
