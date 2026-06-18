#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_netif.h"
#include "driver/gpio.h"
#include "sensor.h"
static const char *TAG = "SENSOR_LOGIC";
adc_oneshot_unit_handle_t adc_handle; // Manejador global para el ADC
bool temp_state = false;
bool calidad_state = false;
bool automatic_mode = true;

void toggle_auto_mode(void) {
    automatic_mode = !automatic_mode;
    ESP_LOGI(TAG, "Modo automático: %s", automatic_mode ? "ON" : "OFF");
}

void set_actuator_state(gpio_num_t pin, bool enable) {
    gpio_set_level(pin, enable ? 1 : 0);
    if (pin == PIN_TEMP) {
        temp_state = enable;
    } else if (pin == PIN_CALIDAD) {
         cali
        calidad_state = enable;
    }
}

void iniciar_hardware(void) {
    // Configurar Salidas Digitales
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_TEMP) | (1ULL << PIN_CALIDAD),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // Inicializamos los actuadores apagados
    set_actuator_state(PIN_TEMP, false);
    set_actuator_state(PIN_CALIDAD, false);

    // --- CONFIGURACIÓN ADC ONESHOT (v6.0+) ---
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, CANAL_HUMEDAD, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, CANAL_LUZ, &config));

    ESP_LOGI(TAG, "Hardware y ADC Oneshot iniciados.");
}

int obtener_lectura_adc(adc_channel_t canal) {
    int lectura = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, canal, &lectura));
    return lectura;
}
