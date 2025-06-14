#include "sleep_services.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "soc/soc_caps.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_wifi.h"


static const char *TAG = "ESP32_SLEEP";
sleep_mode_t mode;
wakeup_source_t wk_mode;
uint32_t duration_sec;

void wait_gpio_inactive(void)
{
    ESP_LOGI(TAG, "Checking if Wakeup GPIO%d to go high ", GPIO_WAKEUP_NUM);
    while (gpio_get_level(GPIO_WAKEUP_NUM) == GPIO_WAKEUP_LEVEL) {
        printf(".");
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ESP_LOGI(TAG, "\nChecking done. Wakeup GPIO %d ready!\n", GPIO_WAKEUP_NUM);
}


esp_err_t register_timer_wakeup() {
    esp_err_t ret = ESP_FAIL;

    // Enable timer as a wake-up source
    ret = esp_sleep_enable_timer_wakeup(duration_sec * 1000000);
    if (ret != ESP_OK) {
        return ret;
    }

    ESP_LOGI(TAG, "Timer wakeup mode enabled for %ld seconds", duration_sec);
    return ret;
}

esp_err_t register_gpio_wakeup(sleep_mode_t mode)
{
    esp_err_t ret = ESP_FAIL;

    uint64_t wk_pin_bit_mask = (1 << GPIO_WAKEUP_NUM);
    
    if (mode == SLEEP_LIGHT) {

        // Initialize GPIO for boot button wakeup
        gpio_config_t config = {
                .pin_bit_mask = wk_pin_bit_mask,
                .mode = GPIO_MODE_INPUT,
                .pull_down_en = false,
                .pull_up_en = false,
                .intr_type = GPIO_INTR_DISABLE
        };

        ret = gpio_config(&config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure GPIO: %s", esp_err_to_name(ret));
            return ret;
        }

        // Enable wake up from GPIO
        ret = gpio_wakeup_enable(GPIO_WAKEUP_NUM, GPIO_INTR_LOW_LEVEL);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to enable GPIO wake-up: %s", esp_err_to_name(ret));
        return ret;
        }
        
        esp_sleep_enable_gpio_wakeup();
        ESP_LOGI(TAG, "Light sleep gpio wakeup mode enabled");

        /* Make sure the GPIO is inactive and it won't trigger wakeup immediately */
        wait_gpio_inactive();
        ESP_LOGI(TAG, "gpio wakeup source is ready");

    } else if (mode == SLEEP_DEEP) {

        #ifdef SOC_GPIO_SUPPORT_DEEPSLEEP_WAKEUP

        esp_deep_sleep_enable_gpio_wakeup(wk_pin_bit_mask, GPIO_WAKEUP_LEVEL);

        #else

        ESP_LOGE(TAG, "Deep sleep gpio wakeup mode is not supported, please use light sleep instead");
        ret = ESP_ERR_NOT_SUPPORTED;

        #endif
        
    }

    return ret;
}


esp_err_t register_ext_wakeup(sleep_mode_t mode) {
    esp_err_t ret = ESP_FAIL;

    if (mode == SLEEP_LIGHT) {
        ESP_LOGE(TAG, "Light sleep ext wakeup mode is not supported, please use deep sleep instead");
        ret = ESP_ERR_NOT_SUPPORTED;
    } else if (mode == SLEEP_DEEP) {
        // Enable wake up from EXT0
        ret = esp_sleep_enable_ext0_wakeup(GPIO_WAKEUP_NUM, GPIO_WAKEUP_LEVEL);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to enable EXT0 wake-up: %s", esp_err_to_name(ret));
        }

        ret = rtc_gpio_pullup_dis(GPIO_WAKEUP_NUM);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to disable pull-up: %s", esp_err_to_name(ret));
            return ret;
        }
        ret = rtc_gpio_pulldown_en(GPIO_WAKEUP_NUM);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to enable pull-down: %s", esp_err_to_name(ret));
            return ret;
        }
        ESP_LOGI(TAG, "Deep sleep ext wakeup mode enabled on GPIO%d", GPIO_WAKEUP_NUM);
    }

    return ret;
}

void sleep_task(void *arg) {
    
    esp_err_t ret;

    if (wk_mode == WAKEUP_TIMER) {
        ESP_LOGI(TAG, "Wakeup source: timer");

        ESP_LOGI(TAG, "Entering %s sleep for %ld seconds...",
            (mode == SLEEP_LIGHT) ? "light" : "deep", duration_sec);

        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second to allow time for logging
        // Enable timer as a wake-up source
        ret = register_timer_wakeup();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to register timer wake-up: %s", esp_err_to_name(ret));
            goto err;
        }

    } else if (wk_mode == WAKEUP_GPIO) {
        ESP_LOGI(TAG, "Wakeup source: GPIO");

        ret = register_gpio_wakeup(mode);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to register GPIO wake-up: %s", esp_err_to_name(ret));
            goto err;
        }

    } else if (wk_mode == WAKEUP_EXT0) {
        ESP_LOGI(TAG, "Wakeup source: EXT0");

        ret = register_ext_wakeup(mode);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to register EXT0 wake-up: %s", esp_err_to_name(ret));
            goto err;
        }
    } else if (wk_mode == WAKEUP_EXT1) {
        ESP_LOGI(TAG, "Wakeup source: EXT1");
    } else if (wk_mode == WAKEUP_TOUCHPAD) {
        ESP_LOGI(TAG, "Wakeup source: touchpad");
    }

    // Enter the chosen sleep mode
    if (mode == SLEEP_LIGHT) {
        int64_t time_before_sleep_us = esp_timer_get_time();
        esp_wifi_disconnect();

        ret = esp_light_sleep_start();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to enter light sleep: %s", esp_err_to_name(ret));
            goto err;
        }

        int64_t time_after_sleep_us = esp_timer_get_time();
        ESP_LOGI(TAG, "Returned from light sleep, slept for %ld ms",
                (uint32_t)(time_after_sleep_us - time_before_sleep_us) / 1000);
    } else {

        #ifdef SOC_DEEP_SLEEP_SUPPORTED

        esp_deep_sleep_start();

        #endif
    }
    
    vTaskDelete(NULL);
err:
    ESP_LOGE(TAG, "Failed to enter sleep mode: %s", esp_err_to_name(ret));
    vTaskDelete(NULL);
}

esp_err_t sleep_service(sleep_mode_t sleep_mode, wakeup_source_t wakeup_source, int32_t sleep_duration_sec) {
    mode = sleep_mode;
    wk_mode = wakeup_source;
    duration_sec = sleep_duration_sec;
    BaseType_t ret = xTaskCreate(sleep_task, "sleep_task", 2048, NULL, 8, NULL);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sleep task");
        return ESP_FAIL;
    }
    return ESP_OK;
}