#ifndef __WIFI_CONNECT_H__
#define __WIFI_CONNECT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdio.h>

#define ESP_WIFI_SSID      "NghiaWifi"
#define ESP_WIFI_PASS      "Nghia1994"
#define ESP_MAXIMUM_RETRY  5

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define TAG "imic_esp32"

esp_err_t wifi_service(void);
void set_wifi_service_enabled(bool enabled);

#ifdef __cplusplus
}
#endif

#endif // __WIFI_CONNECT_H__