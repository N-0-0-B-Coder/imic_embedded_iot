set(app_src mqtt_services.c)

set(pri_req esp_wifi nvs_flash json tcp_transport app_update http_services mqtt)

string(REPLACE "\\" "/" IDF_PATH_FIXED $ENV{IDF_PATH})

idf_component_register(SRCS ${app_src}
                       INCLUDE_DIRS "."
                       INCLUDE_DIRS "${IDF_PATH_FIXED}/components/mqtt/esp-mqtt/include"
                       REQUIRES ${pri_req})