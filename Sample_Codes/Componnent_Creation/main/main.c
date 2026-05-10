#include <stdio.h>
#include "led_toggle.h"

void app_main(void)
{
    led_gpio_t led_gpio = {
        .gpio_nr = 2,
        .status = 1
    };

    if(led_config(&led_gpio) == ESP_OK){
        printf("LED GPIO configured successfully\n");
    } else {
        printf("Failed to configure LED GPIO\n");
    }

    if(led_drive(&led_gpio) == ESP_OK){
        printf("LED driven successfully\n");
    } else {
        printf("Failed to drive LED\n");
    }

}