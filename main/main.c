#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_err.h"
#include "mqtt_services.h"
#include "wifi_services.h"
#include "output.h"
#include "input.h"

void ip_callback(uint32_t gpio_num) {
    if (gpio_get_level(gpio_num)) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        if (gpio_get_level(gpio_num)) {
            int16_t count = 10000;
            while ((gpio_get_level(gpio_num)) && count >= 0) {
                count--;
            }
            if (count >= 0) {
                //xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
                input_set_queue(gpio_num);
            }
        }
    }
}

void app_main(void)
{
    /* Start wifi task*/
    while (wifi_service() != ESP_OK) {
        ESP_LOGI(TAG, "Retrying Wi-Fi connection...");
        vTaskDelay(pdMS_TO_TICKS(2000));  // Wait for 2 second before retrying
    }
    /* Start the output task */
    //output_app();

    /* Set the callback function */
    input_set_callback(&ip_callback);

    /* Start the input task */
    input_app();
}
