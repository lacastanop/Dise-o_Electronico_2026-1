#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "agronica.h"
#include "dashboard.h"
#include "wifi_manager.h"   // <--- NUEVA LIBRERIA AÑADIDA

void app_main(void) {
    // 1. Memoria NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_flash_init();
    }

    // Inicializar Event Loop (Necesario para WiFi)
    esp_event_loop_create_default();

    // 2. Encender sistema
    iniciar_hardware();
    iniciar_wifi();        // Llama al código del nuevo archivo
    iniciar_dashboard();

    while (1) {
        int hum_raw = obtener_lectura_adc(CANAL_TEMP);
        int luz_raw = obtener_lectura_adc(CANAL_CALIDAD);
        int hum_pct = (hum_raw * 100) / 4095;
        int luz_pct = (luz_raw * 100) / 4095;
        
        ESP_LOGI("MONITOR", "Humedad: %d%% | Luz: %d%%", hum_pct, luz_pct);
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}