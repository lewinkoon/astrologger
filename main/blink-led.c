#include "driver/gpio.h"

#define BLINK_GPIO 2

void blink_led(int led_state)
{
    gpio_set_level(BLINK_GPIO, led_state);
}

void configure_led(void)
{
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BLINK_GPIO, 0);
}