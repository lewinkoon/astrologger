#include "driver/i2c.h"

#define I2C_MASTER_SCL_IO 22      /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO 21      /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_FREQ_HZ 100000 /*!< I2C master clock frequency */
#define I2C_MASTER_TIMEOUT_MS 1000
#define I2C_MASTER_ACK 0
#define I2C_MASTER_NACK 1

esp_err_t i2c_master_init(void)
{

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(I2C_NUM_0, &conf);

    return i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
}

int8_t i2c_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt)
{
    int32_t iError = 0;

    esp_err_t espRc;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);

    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write(cmd, reg_data, cnt, true);
    i2c_master_stop(cmd);

    espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 200 / portTICK_PERIOD_MS);
    //  printf("in user_i2c_read, i2c_master_cmd_begin returns %d\r\n", espRc);
    if (espRc == ESP_OK)
    {
        iError = 0;
    }
    else
    {
        iError = -1;
    }
    i2c_cmd_link_delete(cmd);

    return (int8_t)iError;
}

int8_t i2c_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt)
{
    int32_t iError = 0;
    esp_err_t espRc;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_READ, true);

    if (cnt > 1)
    {
        i2c_master_read(cmd, reg_data, cnt - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, reg_data + cnt - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 200 / portTICK_PERIOD_MS);
    if (espRc == ESP_OK)
    {
        iError = 0;
    }
    else
    {
        iError = -1;
    }

    i2c_cmd_link_delete(cmd);

    return (int8_t)iError;
}