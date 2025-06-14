set(app_src ota_services.c)

set(pri_req lwip esp_http_client nvs_flash app_update http_services)
idf_component_register(SRCS ${app_src}
                       INCLUDE_DIRS "." 
                       REQUIRES ${pri_req})