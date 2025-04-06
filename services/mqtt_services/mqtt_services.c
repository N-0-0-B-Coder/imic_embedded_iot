/* MQTT Mutual Authentication Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "http_services.h"
#include "mqtt_services.h"

bool mqtt_connected = false;
esp_mqtt_client_handle_t client;
char *mqtt_root_ca = NULL,
     *mqtt_device_cert = NULL,
     *mqtt_private_key = NULL;


static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);

        mqtt_connected = true;
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void publish_random_data() {
    char payload[128];

    for (;;) {
        if (mqtt_connected) {
            int temp = rand() % 50;  // Generate random temperature (0-50°C)
            snprintf(payload, sizeof(payload), "{\"temperature\": %d}", temp);
            esp_mqtt_client_publish(client, "/topic/data", payload, 0, 1, 0);
            ESP_LOGI(TAG, "Published: %s", payload);
        } else {
            ESP_LOGW(TAG, "MQTT client is not connected. Skipping publish.");
        }

        vTaskDelay(100000 / portTICK_PERIOD_MS); // Publish every 100 seconds
    }
}

void mqtt_ping_task(void *arg) {
    for (;;) {
        if (mqtt_connected) {
            esp_mqtt_client_publish(client, "/topic/ping", "1", 0, 1, 0);
            ESP_LOGI(TAG, "MQTT ping sent to keep connection alive");
        } else {
            ESP_LOGW(TAG, "MQTT client is not connected. Skipping ping.");
        }

        vTaskDelay(pdMS_TO_TICKS(50000));  // Ping every 50 seconds
    }
}

static void mqtt_app_start(void) {
    srand(time(NULL));

    static SemaphoreHandle_t cert_mutex = NULL;

    if (cert_mutex == NULL) {
        cert_mutex = xSemaphoreCreateMutex();
        if (cert_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create mutex for certificates");
            return;
        }
    }

    if (check_certs_and_keys_exist() != ESP_OK) {
        ESP_LOGI(TAG, "Certs and keys not found in NVS.");
        return;
    }

    if (xSemaphoreTake(cert_mutex, portMAX_DELAY) == pdTRUE) {
        retrieve_certs_and_keys(&mqtt_root_ca, &mqtt_device_cert, &mqtt_private_key);
        xSemaphoreGive(cert_mutex);
    } else {
        ESP_LOGE(TAG, "Failed to take mutex for certificates");
        return;
    }
    if (mqtt_root_ca == NULL || mqtt_device_cert == NULL || mqtt_private_key == NULL) {
        ESP_LOGE(TAG, "Failed to get certificates and keys");
        return;
    }

    //ESP_LOGI(TAG, "MQTT Root CA: \n%s", mqtt_root_ca);
    //ESP_LOGI(TAG, "MQTT Device Certificate: \n%s", mqtt_device_cert);
    //ESP_LOGI(TAG, "MQTT Private Key: \n%s", mqtt_private_key);

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = AWS_BROKER_URL,
        .broker.verification.certificate = mqtt_root_ca,
        .credentials = {
            .authentication = {
                .certificate = mqtt_device_cert,
                .key = mqtt_private_key,
            },
        }
    };

    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    xTaskCreate(publish_random_data, "mqtt_publish_task", 2048, NULL, 5, NULL);
    //xTaskCreate(mqtt_ping_task, "mqtt_ping_task", 4096, NULL, 5, NULL);
}

void mqtt_task(void *arg) {
    //ESP_LOGI(TAG, "[APP] Startup..");
    //ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    //ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    // ESP_ERROR_CHECK(nvs_flash_init());
    // ESP_ERROR_CHECK(esp_netif_init());
    // ESP_ERROR_CHECK(esp_event_loop_create_default());

    mqtt_app_start();
    for (;;) {
        vTaskDelay(1);
    }
}

esp_err_t mqtt_service(void)
{
    BaseType_t xReturned;
    uint8_t count = 0;
    xReturned = xTaskCreate(mqtt_task, "mqtt_task", 8192, NULL, 5, NULL);
    if (xReturned != pdPASS) {
        ESP_LOGE(TAG, "Failed to create MQTT task");
        return ESP_FAIL;
    }
    while (mqtt_connected != true && count < 10) {
        count++;
        ESP_LOGI(TAG, "Waiting for MQTT connection...");
        vTaskDelay(pdMS_TO_TICKS(2000));  // Wait for 2 second before retrying
    }
    if (count == 10) {
        ESP_LOGE(TAG, "Failed to connect to MQTT broker after 10 attempts. Exiting MQTT task.");
        return ESP_FAIL;
    }
    return ESP_OK;
}
