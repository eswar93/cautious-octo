idf_component_register(
    SRCS "main.c" "sensor.c"
    INCLUDE_DIRS "." "../../third_party/mavlink/c_library_v2/common" "../../third_party/mavlink/c_library_v2"
    REQUIRES esp_wifi esp_event esp_netif nvs_flash driver lwip)
