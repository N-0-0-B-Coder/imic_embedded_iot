set(app_src sleep_services.c)

set(pri_req nvs_flash driver soc esp_timer esp_wifi)
idf_component_register(SRCS ${app_src}
                       INCLUDE_DIRS "." 
                       REQUIRES ${pri_req})