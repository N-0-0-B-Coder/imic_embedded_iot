#ifndef SLEEP_SERVICE_H
#define SLEEP_SERVICE_H

#include "esp_sleep.h"
#include "esp_err.h"

typedef enum {
    SLEEP_LIGHT,
    SLEEP_DEEP
} sleep_mode_t;

typedef enum {
    WAKEUP_TIMER,
    WAKEUP_GPIO,
    WAKEUP_EXT0,
    WAKEUP_EXT1,
    WAKEUP_TOUCHPAD
} wakeup_source_t;

// Use boot button as gpio input
#define BOOT_BUTTON_NUM         0
#define GPIO_WAKEUP_NUM         BOOT_BUTTON_NUM

// Boot button is active low
#define GPIO_WAKEUP_LEVEL       0

// API to enter sleep
esp_err_t sleep_service(sleep_mode_t sleep_mode, wakeup_source_t wakeup_source, int32_t sleep_duration_sec);

#endif // SLEEP_SERVICE_H