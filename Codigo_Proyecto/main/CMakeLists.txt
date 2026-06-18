idf_component_register(SRCS "sources/main.c" 
                                "sources/wifi_manager.c" 
                                "sources/agronica.c"
                                "sources/dashboard.c"
                       INCLUDE_DIRS "headers"
                       REQUIRES esp_adc esp_http_server esp_wifi esp_netif nvs_flash esp_event esp_driver_gpio esp_timer)
