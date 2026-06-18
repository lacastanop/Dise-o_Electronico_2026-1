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
#include "agronica.h"
#include "dashboard.h"

static const char *TAG = "DASHBOARD";

extern bool valve_state;
extern bool dosificador_state;
extern bool automatic_mode;

static void obtener_ip_local(char *buffer, size_t len) {
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif == NULL) {
        strlcpy(buffer, "Sin IP", len);
        return;
    }

    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(netif, &ip_info) != ESP_OK) {
        strlcpy(buffer, "Sin IP", len);
        return;
    }

    if (ip_info.ip.addr == 0) {
        strlcpy(buffer, "Sin IP", len);
        return;
    }

    esp_ip4addr_ntoa(&ip_info.ip, buffer, len);
}

static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        char ipstr[16];
        esp_ip4addr_ntoa(&event->ip_info.ip, ipstr, sizeof(ipstr));
        ESP_LOGI(TAG, "IP local asignada: %s", ipstr);
    }
}

const char* html_page =
    "<html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>Agronica Dashboard</title><style>"
    "body{margin:0;background:linear-gradient(135deg,#0f2027,#203a43,#2c5364);color:#f0f4f8;font-family:'Helvetica Neue',Arial,sans-serif;}"
    "header{padding:30px 20px;text-align:center;color:#fff;}"
    "h1{margin:0;font-size:2.5rem;letter-spacing:1px;}"
    "p.subtitle{color:#cbd6e2;font-size:1rem;max-width:820px;margin:10px auto 0;}"
    ".container{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:20px;padding:20px;max-width:1200px;margin:0 auto;}"
    ".card{background:rgba(255,255,255,0.16);backdrop-filter:blur(20px);border:1px solid rgba(255,255,255,0.16);border-radius:28px;box-shadow:0 28px 90px rgba(0,0,0,0.22);padding:28px;min-height:220px;transition:transform .25s ease,box-shadow .25s ease;}"
    ".card:hover{transform:translateY(-6px);box-shadow:0 34px 110px rgba(0,0,0,0.28);}"
    ".card-header{display:flex;align-items:center;gap:14px;margin-bottom:18px;}"
    ".card-icon{display:inline-flex;align-items:center;justify-content:center;width:52px;height:52px;border-radius:18px;background:rgba(255,255,255,0.18);font-size:1.8rem;box-shadow:inset 0 0 0 1px rgba(255,255,255,0.18);}"
    ".card h2{margin-top:0;color:#ffffff;font-size:1.45rem;}"
    ".metric{font-size:2.75rem;font-weight:800;margin:10px 0;color:#f8fafc;line-height:1;}"
    ".metric-title{font-size:0.85rem;color:#a8c0cf;text-transform:uppercase;letter-spacing:0.16em;margin-bottom:8px;}"
    ".metric-box{background:rgba(255,255,255,0.08);border-radius:20px;padding:18px;box-shadow:inset 0 1px 0 rgba(255,255,255,0.08);margin-top:16px;}"
    ".status{display:inline-block;padding:10px 16px;border-radius:999px;font-weight:700;font-size:0.95rem;margin-top:12px;}"
    ".status.on{background:#21d07a;color:#042b19;} .status.off{background:#ef476f;color:#fff;}"
    ".status-block{display:flex;flex-wrap:wrap;gap:14px;margin-top:16px;}"
    ".btn{display:inline-flex;align-items:center;gap:10px;padding:12px 18px;border-radius:999px;font-weight:700;border:none;text-decoration:none;color:#fff;margin:6px 4px;cursor:pointer;transition:transform .2s ease,opacity .2s ease,box-shadow .2s ease;box-shadow:0 12px 26px rgba(0,0,0,0.18);}"
    ".btn:hover{transform:translateY(-2px);opacity:.98;box-shadow:0 16px 28px rgba(0,0,0,0.2);}"
    ".btn-green{background:#10b981;} .btn-red{background:#ef4444;} .btn-blue{background:#3b82f6;}"
    ".small-text{color:#cbd6e2;font-size:0.95rem;margin-top:10px;}"
    ".info-line{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:12px;margin-top:16px;color:#cbd6e2;}"
    ".info-line span{display:block;}"
    "#clock{font-size:1.6rem;font-weight:700;margin:8px 0;color:#fdfffc;}"
    "footer{text-align:center;color:#a8c0cf;padding:20px 10px 40px;font-size:0.9rem;}"
    "</style></head><body>"
    "<header><h1>Agronica Smart Garden</h1><p class='subtitle'>Monitoreo y control completo en tiempo real para tu sistema de riego inteligente. Sin recargar la página.</p></header>"
    "<div class='container'>"
    "<div class='card'>"
    "<h2>Estado actual</h2>"
    "<div class='metric' id='clock'>--:--:--</div>"
    "<small class='small-text' id='date'>Cargando fecha...</small>"
    "<div class='small-text'>Modo de operación: <span id='auto_mode' class='status on'>Automático</span></div>"
    "<div style='margin-top:12px;'><button id='toggle_auto' class='btn btn-green' onclick='toggleMode()'>Cambiar modo</button></div>"
    "</div>"
    "<div class='card'>"
    "<div class='card-header'><span class='card-icon'>🌡️</span><h2>Sensores</h2></div>"
    "<div class='metric-box'>"
    "<div class='metric-title'>Humedad de suelo</div>"
    "<div class='metric' id='humidity'>--</div>"
    "<div class='small-text'>Lectura ADC del sensor de humedad.</div>"
    "</div>"
    "<div class='metric-box'>"
    "<div class='metric-title'>Nivel de luz</div>"
    "<div class='metric' id='light'>--</div>"
    "<div class='small-text'>Lectura ADC del sensor de luz.</div>"
    "</div>"
    "</div>"
    "<div class='card'>"
    "<h2>Gráfica de sensores</h2>"
    "<canvas id='sensor_chart' width='560' height='260' style='width:100%;max-width:100%;height:auto;background:rgba(255,255,255,0.08);border-radius:16px;display:block;margin-top:12px;'></canvas>"
    "<small class='small-text'>Humedad y luz en los últimos minutos.</small>"
    "</div>"
    "<div class='card'>"
    "<div class='card-header'><span class='card-icon'>💧</span><h2>Actuadores</h2></div>"
    "<div class='status-block'>"
    "<div class='small-text'>Válvula: <span id='valve_status' class='status off'>OFF</span></div>"
    "<div class='small-text'>Dosificador electrónico: <span id='dosificador_status' class='status off'>OFF</span></div>"
    "</div>"
    "<div class='status-block'>"
    "<a href='/actuador?d=valvula&s=1' class='btn btn-green'>💦 Abrir válvula</a>"
    "<a href='/actuador?d=valvula&s=0' class='btn btn-red'>❌ Cerrar válvula</a>"
    "<a href='/actuador?d=dosificador&s=1' class='btn btn-blue'>⚙️ Encender dosificador</a>"
    "<a href='/actuador?d=dosificador&s=0' class='btn btn-red'>🛑 Apagar dosificador</a>"
    "</div>"
    "</div>"
    "</div>"
    "<div class='card'>"
    "<h2>Información del sistema</h2>"
    "<p class='small-text'>Actualiza automáticamente cada 3 segundos.</p>"
    "<p class='small-text'>Última sincronización: <span id='last_update'>Cargando...</span></p>"
    "<p class='small-text'>Conexión WiFi: <span id='wifi_status'>Conectando...</span></p>"
    "<p class='small-text'>Acceso directo: <a id='dashboard_link' href='#' style='color:#2dd4bf;text-decoration:underline;'>Cargando...</a></p>"
    "</div>"
    "</div>"
    "<footer>Usa los botones para control manual y observa los datos reales sin recargar.</footer>"
        "<script>"
    "const chartData = { humidity: [], light: [], labels: [], maxPoints: 12 };"
    "function drawChart() {const canvas = document.getElementById('sensor_chart'); if (!canvas) return; const ctx = canvas.getContext('2d'); const w = canvas.width; const h = canvas.height; ctx.clearRect(0, 0, w, h); ctx.fillStyle = 'rgba(255,255,255,0.08)'; ctx.fillRect(0, 0, w, h); ctx.strokeStyle = 'rgba(255,255,255,0.18)'; ctx.lineWidth = 1; for (let i = 1; i < 5; i++) { const y = (h / 5) * i; ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke(); } ctx.fillStyle = '#888'; ctx.font = '11px Arial'; for (let i = 0; i <= 4; i++) { const y = h - 20 - (i * 25 / 100) * (h - 40); ctx.fillText((100 - i * 25), 2, y + 4); } const drawLine = (values, color) => { if (values.length === 0) return; ctx.beginPath(); ctx.strokeStyle = color; ctx.lineWidth = 2.5; values.forEach((v, index) => { const x = index * (w / Math.max(1, chartData.maxPoints - 1)); const y = h - 20 - (v / 100) * (h - 40); if (index === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y); }); ctx.stroke(); }; drawLine(chartData.humidity, 'rgba(34,197,94,0.95)'); drawLine(chartData.light, 'rgba(250,204,21,0.95)'); ctx.fillStyle = '#22c55e'; ctx.fillText('Humedad', 10, 18); ctx.fillStyle = '#facc15'; ctx.fillText('Luz', 120, 18); }"
    "function addPoint(label, hum, light) { if (chartData.labels.length >= chartData.maxPoints) { chartData.labels.shift(); chartData.humidity.shift(); chartData.light.shift(); } chartData.labels.push(label); chartData.humidity.push(hum); chartData.light.push(light); drawChart(); }"
    "function updateClock() {const now = new Date();document.getElementById('clock').innerText = now.toLocaleTimeString();document.getElementById('date').innerText = now.toLocaleDateString();}"
    "function updateData() {fetch('/data').then(res => res.json()).then(data => {document.getElementById('humidity').innerText = data.humidity;document.getElementById('light').innerText = data.light;document.getElementById('valve_status').innerText = data.valveStatus;document.getElementById('dosificador_status').innerText = data.dosificadorStatus;document.getElementById('last_update').innerText = new Date(data.timestamp).toLocaleTimeString();document.getElementById('wifi_status').innerText = data.wifiStatus;document.getElementById('valve_status').className = data.valveStatus === 'ON' ? 'status on' : 'status off';document.getElementById('dosificador_status').className = data.dosificadorStatus === 'ON' ? 'status on' : 'status off';document.getElementById('auto_mode').innerText = data.autoMode;var toggle = document.getElementById('toggle_auto');toggle.innerText = data.autoMode === 'Automático' ? 'Cambiar a manual' : 'Cambiar a automático';var link = document.getElementById('dashboard_link');link.href = 'http://' + data.ip;link.innerText = data.ip === 'Sin IP' ? 'Sin IP' : 'http://' + data.ip; addPoint(new Date(data.timestamp).toLocaleTimeString(), Number(data.humidity), Number(data.light));});}"
    "function toggleMode() {fetch('/auto').then(() => updateData());}"
    "window.onload = function() {updateClock();updateData();setInterval(updateClock,1000);setInterval(updateData,3000);};"
    "</script></body></html>";

esp_err_t dashboard_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t data_get_handler(httpd_req_t *req) {
    int hum_raw = obtener_lectura_adc(CANAL_HUMEDAD);
    int luz_raw = obtener_lectura_adc(CANAL_LUZ);
    int hum_pct = (hum_raw * 100) / 4095;
    int luz_pct = (luz_raw * 100) / 4095;
    int64_t ts = esp_timer_get_time() / 1000;
    char response[256];

    char ip_address[32];
    obtener_ip_local(ip_address, sizeof(ip_address));
    snprintf(response, sizeof(response),
             "{\"humidity\":%d,\"light\":%d,\"valveStatus\":\"%s\",\"dosificadorStatus\":\"%s\",\"autoMode\":\"%s\",\"wifiStatus\":\"%s\",\"ip\":\"%s\",\"timestamp\":%lld}",
             hum_pct,
             luz_pct,
             valve_state ? "ON" : "OFF",
             dosificador_state ? "ON" : "OFF",
             automatic_mode ? "Automático" : "Manual",
             "Conectado",
             ip_address,
             ts);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t actuador_set_handler(httpd_req_t *req) {
    char device[16];
    char state_str[8];
    char query[64];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK &&
        httpd_query_key_value(query, "d", device, sizeof(device)) == ESP_OK &&
        httpd_query_key_value(query, "s", state_str, sizeof(state_str)) == ESP_OK) {
        int estado = atoi(state_str);
        if (strcmp(device, "valvula") == 0) {
            set_actuator_state(PIN_VALVULA, estado != 0);
            ESP_LOGI(TAG, "Válvula manual: %s", estado ? "ENCENDIDA" : "APAGADA");
        } else if (strcmp(device, "dosificador") == 0 || strcmp(device, "bomba") == 0) {
            set_actuator_state(PIN_DOSIFICADOR, estado != 0);
            ESP_LOGI(TAG, "Dosificador manual: %s", estado ? "ENCENDIDO" : "APAGADO");
        }
    }

    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

esp_err_t auto_mode_handler(httpd_req_t *req) {
    toggle_auto_mode();
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

httpd_handle_t iniciar_dashboard(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uri_get = { .uri = "/", .method = HTTP_GET, .handler = dashboard_get_handler };
        httpd_register_uri_handler(server, &uri_get);

        httpd_uri_t uri_data = { .uri = "/data", .method = HTTP_GET, .handler = data_get_handler };
        httpd_register_uri_handler(server, &uri_data);

        httpd_uri_t uri_act = { .uri = "/actuador", .method = HTTP_GET, .handler = actuador_set_handler };
        httpd_register_uri_handler(server, &uri_act);

        httpd_uri_t uri_auto = { .uri = "/auto", .method = HTTP_GET, .handler = auto_mode_handler };
        httpd_register_uri_handler(server, &uri_auto);

        ESP_LOGI(TAG, "Servidor Web iniciado con éxito.");
    }
    return server;
}