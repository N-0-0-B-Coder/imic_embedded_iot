#include "mqtt_services.h"
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
#include "ota_services.h"
#include "sleep_services.h"

#include "esp_partition.h"
#include "esp_ota_ops.h"

static const char *TAG = "ESP32_MQTT";

static int s_retry_num = 0;

bool mqtt_connected = false,
     mqtt_ota = false;
esp_mqtt_client_handle_t client;
char *mqtt_root_ca = NULL,
     *mqtt_device_cert = NULL,
     *mqtt_private_key = NULL,
     *device_id = "ESP32-001",
     *firmware_version = "1.0.0";


static char mqtt_topic[MAX_TOPIC_LENGTH];
static char mqtt_payload[MAX_PAYLOAD_LENGTH];
static int mqtt_payload_len = 0;


static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}




void dump_otadata(void) {
    const esp_partition_t *otadata = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, NULL);
    if (!otadata) {
        ESP_LOGE(TAG, "Failed to find otadata partition");
        return;
    }

    uint8_t buf[32];
    esp_partition_read(otadata, 0, buf, sizeof(buf));
    ESP_LOG_BUFFER_HEX("OTADATA", buf, sizeof(buf));
}

void test_partition(void) {
    ESP_LOGI(TAG, "Starting OTA test...");

    const esp_partition_t *running = esp_ota_get_running_partition();
    ESP_LOGI(TAG, "Running partition: %s @ 0x%" PRIx32, running->label, running->address);

    const esp_partition_t *next = esp_ota_get_next_update_partition(running);
    ESP_LOGI(TAG, "Next partition to set: %s @ 0x%" PRIx32, next->label, next->address);

    esp_err_t ret = esp_ota_set_boot_partition(next);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "esp_ota_set_boot_partition() succeeded.");
    } else {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition() failed: %s", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "Reading OTA data after partition change...");
    dump_otadata();

    ESP_LOGI(TAG, "Waiting 10s before reboot...");
    vTaskDelay(pdMS_TO_TICKS(10000));
    esp_restart();
}


uint8_t check_mqtt_topic(char *topic, char *data) {
    if (topic == NULL || data == NULL) {
        ESP_LOGE(TAG, "Topic or data is NULL");
        return -1;
    }
    
    //ESP_LOGI(TAG, "Topic: %s", topic);

    if (strcmp(topic, "/topic/command/ESP32-001") == 0) {
        cJSON *json = cJSON_Parse(data);
        if (json == NULL) {
            ESP_LOGE(TAG, "Failed to parse OTA JSON data");
            return -1;
        }

        char *command = cJSON_GetObjectItem(json, "command")->valuestring;
        if (strcmp(command, "ota") == 0) {

            ESP_LOGI(TAG, "OTA command received via MQTT! Starting OTA update...");

            char *fw_url = cJSON_GetObjectItem(json, "fw_url")->valuestring;
            if (fw_url == NULL) {
                ESP_LOGE(TAG, "Failed to get fw_url from JSON data");
                cJSON_Delete(json);
                return -1;
            }
    
            uint32_t fw_crc = (uint32_t) cJSON_GetObjectItem(json, "fw_crc")->valuedouble;
            if (fw_crc == 0) {
                ESP_LOGE(TAG, "Failed to get fw_crc from JSON data");
                cJSON_Delete(json);
                return -1;
            }
    
            ota_service(fw_url, fw_crc);
            //test_partition();
            cJSON_Delete(json);

        } else if (strcmp(command, "restart") == 0) {
            ESP_LOGI(TAG, "Restart command found via MQTT! Restarting device...");
            esp_restart();
        } else if (strcmp(command, "factory_reset") == 0) {
            ESP_LOGI(TAG, "Factory reset command found via MQTT! Performing factory reset...");
            esp_restart();
        } else {
            ESP_LOGW(TAG, "Unknown command: %s", command);
            cJSON_Delete(json);
            return -1;
        }
    } 
    else {
        ESP_LOGW(TAG, "Unknown topic: %s", topic);
        return -1;
    }

    return 0;
}



static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected to broker.");
        int topic_command_len = snprintf(NULL, 0, "/topic/command/%s", device_id) + 1; // +1 for null terminator
        char *topic_command = malloc(topic_command_len);
        if (topic_command == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for topic_command");
            break;
        }
        snprintf(topic_command, topic_command_len, "/topic/command/%s", device_id);
        msg_id = esp_mqtt_client_subscribe(client, topic_command, 1);
        ESP_LOGI(TAG, "Subscribe sent with topic %s successful, msg_id=%d", topic_command, msg_id);

        //msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        //ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        //msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        //ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);

        mqtt_connected = true;

        //sleep_service(SLEEP_LIGHT, WAKEUP_GPIO, 0); // Enter light sleep mode after connecting to MQTT broker
        sleep_service(SLEEP_DEEP, WAKEUP_EXT0, 0); // Enter deep sleep mode after connecting to MQTT broker
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT disconnected.");
        mqtt_connected = false;
        mqtt_ota = false;

        wifi_ap_record_t ap_info;

        if (s_retry_num < ESP_MQTT_MAXIMUM_RETRY) {
            int delay_time = (1 << s_retry_num) * 1000; // 1s, 2s, 4s, etc.
            vTaskDelay(pdMS_TO_TICKS(delay_time));
            if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
                ESP_LOGI(TAG, "Retrying MQTT connection...");
                esp_err_t ret = esp_mqtt_client_reconnect(client);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to reconnect to MQTT broker: %s", esp_err_to_name(ret));
                } else {
                    ESP_LOGI(TAG, "Reconnected to MQTT broker.");
                    mqtt_connected = true;
                    mqtt_ota = true;
                }
            }
            
            s_retry_num++;
        }

        break;

    case MQTT_EVENT_SUBSCRIBED:
        //ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        //ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        //ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:

        //ESP_LOGI(TAG, "MQTT_EVENT_DATA");

        // First chunk of message
        if (event->current_data_offset == 0) {
            memset(mqtt_payload, 0, sizeof(mqtt_payload));
            memset(mqtt_topic, 0, sizeof(mqtt_topic));
            mqtt_payload_len = 0;

            int topic_len = event->topic_len < MAX_TOPIC_LENGTH - 1 ? event->topic_len : MAX_TOPIC_LENGTH - 1;
            memcpy(mqtt_topic, event->topic, topic_len);
            mqtt_topic[topic_len] = '\0';
        }

        // Append this chunk to the payload buffer
        if (mqtt_payload_len + event->data_len < MAX_PAYLOAD_LENGTH) {
            memcpy(mqtt_payload + mqtt_payload_len, event->data, event->data_len);
            mqtt_payload_len += event->data_len;
        } else {
            ESP_LOGW(TAG, "MQTT payload too large. Data truncated.");
        }

        // Final chunk
        if (event->current_data_offset + event->data_len == event->total_data_len) {
            mqtt_payload[mqtt_payload_len] = '\0';  // Null-terminate
            //ESP_LOGI(TAG, "Complete TOPIC: %s", mqtt_topic);
            ESP_LOGI(TAG, "Complete DATA: %s", mqtt_payload);

            // Check the MQTT
            if (check_mqtt_topic(mqtt_topic, mqtt_payload) != 0) {
                ESP_LOGE(TAG, "Failed to check MQTT topic or data");
            }
        }

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

    case MQTT_EVENT_BEFORE_CONNECT:
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
            int ret = esp_mqtt_client_publish(client, "/topic/data", json_string, 0, 1, 0);

            if (ret < 0) {
                ESP_LOGE(TAG, "Failed to publish MQTT message: %d", ret);
            } else {
                ESP_LOGI(TAG, "Published JSON: %s", json_string);
            }

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
    if (!esp_sntp_enabled()) {
    ESP_LOGI("SNTP", "Initializing SNTP");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org"); // Use the default NTP server
    esp_sntp_init();
    }

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
    xTaskCreate(publish_json_data, "mqtt_publish_task", 2048, NULL, 5, NULL);
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

    ESP_LOGI(TAG, "MQTT task remaining stack: %d bytes", uxTaskGetStackHighWaterMark(NULL));
    for (;;) {
        vTaskDelay(1);
    }
}

esp_err_t mqtt_service(void)
{
    BaseType_t xReturned;
    uint8_t count = 0;
    xReturned = xTaskCreate(mqtt_task, "mqtt_task", 3 * 1024, NULL, 5, NULL);
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