#ifndef sensor_h
#define sensor_h

#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"

// Pines según tu diseño del PDF
#define PIN_TEMP        GPIO_NUM_25 
#define PIN_CALIDAD    GPIO_NUM_26
#define CANAL_CALIDAD      ADC_CHANNEL_6 // GPIO 34
#define CANAL_TEMP          ADC_CHANNEL_7 // GPIO 35

extern bool tem_state;
extern bool calidad_state;
extern bool automatic_mode;

void iniciar_hardware(void);
void set_actuator_state(gpio_num_t pin, bool enable);
int obtener_lectura_adc(adc_channel_t canal);
void toggle_auto_mode(void);

#endif
