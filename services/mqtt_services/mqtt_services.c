#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <stddef.h>
#include <string.h>
#include "cJSON.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_sntp.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "mqtt_client.h"

#include "http_services.h"
#include "mqtt_services.h"

bool mqtt_connected = false;
esp_mqtt_client_handle_t client;
char *mqtt_root_ca = NULL,
     *mqtt_device_cert = NULL,
     *mqtt_private_key = NULL,
     *device_id = "ESP32-001",
     *firmware_version = "1.0.0";


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

void publish_json_data() {
    char *json_string = NULL;

    for (;;) {
        if (mqtt_connected) {
            // Create the root JSON object
            cJSON *root = cJSON_CreateObject();
            if (root == NULL) {
                ESP_LOGE(TAG, "Failed to create root JSON object");
                vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1 seconds
                continue;
            }

            // Add "created_at" field
            time_t now = time(NULL);
            cJSON_AddNumberToObject(root, "created_at", now);

            // Add "device" object
            cJSON *device = cJSON_CreateObject();
            if (device == NULL) {
                ESP_LOGE(TAG, "Failed to create device JSON object");
                cJSON_Delete(root);
                vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1 seconds
                continue;
            }
            cJSON_AddStringToObject(device, "serial_number", device_id);
            cJSON_AddStringToObject(device, "firmware_version", firmware_version);
            cJSON_AddItemToObject(root, "device", device);

            // Add "data" array
            cJSON *data_array = cJSON_CreateArray();
            if (data_array == NULL) {
                ESP_LOGE(TAG, "Failed to create data JSON array");
                cJSON_Delete(root);
                vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 100 seconds
                continue;
            }

            // Add first sensor data (velocity)
            cJSON *velocity = cJSON_CreateObject();
            if (velocity != NULL) {
                float random_value = ((float)rand() / RAND_MAX) * 100.0f; // Random value between 0.0 and 99.9
                char random_value_str[10];
                snprintf(random_value_str, sizeof(random_value_str), "%.1f", random_value);
                cJSON_AddStringToObject(velocity, "name", "velocity");
                cJSON_AddStringToObject(velocity, "value", random_value_str);
                cJSON_AddStringToObject(velocity, "unit", "km/h");
                cJSON_AddStringToObject(velocity, "series", "v");
                cJSON_AddNumberToObject(velocity, "timestamp", now);
                cJSON_AddItemToArray(data_array, velocity);
            }

            // Add second sensor data (frequency)
            cJSON *frequency = cJSON_CreateObject();
            if (frequency != NULL) {
                int16_t random_value = rand() % 100; // Random value between 0 and 99
                char random_value_str[10];
                snprintf(random_value_str, sizeof(random_value_str), "%d", random_value);
                cJSON_AddStringToObject(frequency, "name", "frequency");
                cJSON_AddStringToObject(frequency, "value", random_value_str);
                cJSON_AddStringToObject(frequency, "unit", "Hz");
                cJSON_AddStringToObject(frequency, "series", "f");
                cJSON_AddNumberToObject(frequency, "timestamp", now);
                cJSON_AddItemToArray(data_array, frequency);
            }

            cJSON_AddItemToObject(root, "data", data_array);

            // Convert JSON object to string
            json_string = cJSON_PrintUnformatted(root);
            if (json_string == NULL) {
                ESP_LOGE(TAG, "Failed to print JSON object");
                cJSON_Delete(root);
                vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 100 seconds
                continue;
            }

            // Publish the JSON string via MQTT
            esp_mqtt_client_publish(client, "/topic/data", json_string, 0, 1, 0);
            ESP_LOGI(TAG, "Published JSON: %s", json_string);

            // Free allocated memory
            cJSON_Delete(root);
            free(json_string);
        } else {
            ESP_LOGW(TAG, "MQTT client is not connected. Skipping publish.");
        }

        // Delay 100 seconds
        vTaskDelay(pdMS_TO_TICKS(100000));
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

void initialize_sntp(void) {
    ESP_LOGI("SNTP", "Initializing SNTP");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org"); // Use the default NTP server
    esp_sntp_init();

    // Wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (timeinfo.tm_year < (1970 - 1900) && ++retry < retry_count) {
        ESP_LOGI("SNTP", "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(pdMS_TO_TICKS(2000)); // Wait 2 seconds
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (timeinfo.tm_year < (1970 - 1900)) {
        ESP_LOGE("SNTP", "Failed to synchronize time");
    } else {
        ESP_LOGI("SNTP", "System time synchronized: %s", asctime(&timeinfo));
    }
}

static void mqtt_app_start(void) {
    srand(time(NULL));

    // Initialize SNTP to synchronize time
    initialize_sntp();

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
    //xTaskCreate(publish_json_data, "mqtt_publish_task", 2048, NULL, 5, NULL);
    //xTaskCreate(mqtt_ping_task, "mqtt_ping_task", 1024, NULL, 5, NULL);
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
