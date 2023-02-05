#include <stdio.h>
#include "bmp280.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "ssd1306.h"

const char *TAG_I2C = "I2C";

// esp_err_t i2c_master_init(void);
QueueHandle_t sendQueue;

int8_t i2c_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt);
int8_t i2c_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt);

static void delay_ms(uint32_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

void read_data(void *parameters)
{
    SSD1306_t ssd1306;
    i2c_master_init(&ssd1306, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
    ESP_LOGI(TAG_I2C, "I2C initialized successfully");

    ssd1306_init(&ssd1306, 128, 64);
    ssd1306_clear_screen(&ssd1306, false);
    ssd1306_contrast(&ssd1306, 0xff);
    ssd1306_display_text(&ssd1306, 1, " Temperature", 12, false);
    ssd1306_display_text(&ssd1306, 5, " Pressure", 10, false);

    int8_t rslt = BMP280_OK;
    double temp;
    double pres;
    char msgbuf[128];

    struct bmp280_dev bmp280;
    struct bmp280_uncomp_data bmp280_data;

    bmp280.dev_id = BMP280_I2C_ADDR_PRIM;
    bmp280.intf = BMP280_I2C_INTF;
    bmp280.read = (void *)i2c_read;
    bmp280.write = (void *)i2c_write;
    bmp280.delay_ms = (void *)delay_ms;

    ESP_LOGI(TAG_I2C, "CALLING BMP280");
    rslt = bmp280_init(&bmp280);
    ESP_LOGI(TAG_I2C, "BMP280 INIT %d", rslt);

    bmp280.conf.os_temp = BMP280_OS_2X;
    bmp280.conf.os_pres = BMP280_OS_16X;
    bmp280.conf.odr = BMP280_ODR_1000_MS;
    bmp280.conf.filter = BMP280_FILTER_COEFF_16;

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

    while (1)
    {
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

        snprintf(msgbuf, sizeof(msgbuf), "{\"time\":\"2099-12-31\",\"temperature\":\"%.2f\",\"pressure\":\"%.2f\"}",
                 temp,
                 pres / 100);

        ESP_LOGI(TAG_I2C, "%s", msgbuf);
        xQueueSend(sendQueue, &msgbuf, (TickType_t)0);

        char show_temp[12];
        sprintf(show_temp, " %.2f degC", temp);
        ssd1306_display_text(&ssd1306, 2, show_temp, 12, false);

        char show_pres[12];
        sprintf(show_pres, " %.2f hPa", pres / 100);
        ssd1306_display_text(&ssd1306, 6, show_pres, 12, false);

        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }

    // ESP_ERROR_CHECK(i2c_driver_delete(I2C_NUM_0));
    // ESP_LOGI(TAG_I2C, "I2C de-initialized successfully");
}
