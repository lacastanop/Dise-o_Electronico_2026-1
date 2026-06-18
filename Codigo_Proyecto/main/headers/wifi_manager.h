#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"

// --- CONFIGURACIÓN DE RED (DEBE SER 2.4GHz) ---
#define SSID_WIFI "POCO X8 Pro Max"
#define PASS_WIFI "123456789"

// Prototipo de la función principal de WiFi
void iniciar_wifi(void);

#endif
