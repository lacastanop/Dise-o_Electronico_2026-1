# 🌡️ IoT Ambiental — Dispositivo de Bajo Consumo para Interiores

> **Nota de Aplicación — Diseño Electrónico**  
> Laura Sofía Castaño Pineda · [lacastanop@unal.edu.co](mailto:lacastanop@unal.edu.co)  
> Universidad Nacional de Colombia

---

## ¿De qué trata este proyecto?

Dispositivo IoT portable de un único SoC para monitoreo continuo de **temperatura** y **calidad del aire** en espacios cerrados. Transmite datos por Wi-Fi y opera con batería Li-Po, priorizando bajo consumo energético y PCB de dimensiones ultra-compactas.

La apuesta central del diseño es migrar de la arquitectura tradicional de doble chip (STM32 + ESP32) a un **único ESP32-C3**, reduciendo costo, área de PCB y consumo estático de manera simultánea.

---

## Características principales

| Atributo | Valor |
|---|---|
| SoC | ESP32-C3 (RISC-V 32-bit + Wi-Fi 2.4 GHz) |
| Sensor de temperatura | TMP102 (I²C, 10 µA activo) |
| Sensor de calidad de aire | SGP30 — TVOC + eCO₂ (I²C) |
| Alimentación | USB-C 5 V / Batería Li-Po 3.7 V |
| Dimensiones de PCB | 61.2 mm × 35.6 mm (2410 × 1400 mils) |
| Fabricante de PCB objetivo | JLCPCB (FR4, doble cara, 1 oz Cu) |

---

## Arquitectura del sistema

El sistema se divide en tres módulos:

```
┌─────────────────────────────────────────────┐
│           1. MÓDULO DE POTENCIA             │
│                                             │
│  [USB-C 5V] ──► [Fusible PPTC + TVS]        │
│                        │                   │
│                        ▼                   │
│  [TP4056 + PowerPath] ◄──── [Batería Li-Po] │
│                        │                   │
│                        ▼  (VCC_SYS 3.7–5V) │
│              [LDO AMS1117-3.3]              │
└──────────────────┬──────────────────────────┘
                   │ 3.3 V regulado
┌──────────────────▼──────────────────────────┐
│        2. PROCESAMIENTO Y CONTROL           │
│                                             │
│  [3V3_MCU] ──► [ESP32-C3]                   │
│       (filtro ferrita)    │                 │
│                           ▼                 │
│              [ Bus I²C compartido ]         │
└──────────────────┬──────────────────────────┘
                   │ SCL / SDA
┌──────────────────▼──────────────────────────┐
│         3. ADQUISICIÓN DE SENSORES          │
│                                             │
│   [TMP102 · 0x48]     [SGP30 · 0x58]        │
└─────────────────────────────────────────────┘
```

---

## Presupuesto de corriente (Worst-Case)

Escenario de pico: micro-calefactor del SGP30 activo + transmisión Wi-Fi del ESP32-C3.

| Componente | Corriente |
|---|---|
| ESP32-C3 (pico Wi-Fi) | 350 mA |
| SGP30 (micro-hotplate activo) | 48.2 mA |
| TMP102 (sensado activo) | 0.01 mA |
| **Total pico** | **≈ 400 mA** |

El LDO AMS1117-3.3 (SOT-223) soporta hasta **1.0 A continuo**, dejando un margen de seguridad del **60%**:

$$\text{Margen} = \left(1 - \frac{400\,\text{mA}}{1000\,\text{mA}}\right) \times 100\% = 60\%$$

---

## Reglas de diseño de PCB (Altium / JLCPCB)

Diseño conforme al estándar **IPC-2152** (elevación térmica < 10 °C en pistas de 1 oz).

| Tipo de pista | Ancho | Función |
|---|---|---|
| Batería / Carga | 30 mils | Hasta 1 A desde TP4056 |
| Alimentación MCU | 15 mils | Picos de 500 mA (radio Wi-Fi) |
| Señal I²C / GPIO | 10 mils | Corrientes de orden µA |

**Capacidades de fabricación (JLCPCB):**
- Cobre: 1 oz/ft² (35 µm)
- Ancho mínimo pista / espacio: 5 mils (diseño a 10 mils)
- Vías: diámetro interior mínimo 0.3 mm

---

## Flujo de firmware (ESP-IDF / C++)

El firmware implementa un ciclo de **Deep Sleep** agresivo para maximizar la autonomía de la batería:

```
┌─────────────────────────┐
│   Despierta de Sleep    │
└────────────┬────────────┘
             │
             ▼
┌─────────────────────────┐
│  Inicializa Bus I²C     │
│  SDA → GPIO4            │
│  SCL → GPIO5            │
└────────────┬────────────┘
             │
             ▼
┌─────────────────────────┐
│   Lectura de sensores   │
│   TMP102 (0x48)         │
│   SGP30  (0x58)         │
└────────────┬────────────┘
             │
             ▼
┌─────────────────────────┐
│  Enciende módem Wi-Fi   │
│  y conecta al AP        │
└────────────┬────────────┘
             │
             ▼
┌─────────────────────────┐
│ Transmite datos (MQTT / │
│ HTTP) a la nube         │
└────────────┬────────────┘
             │
             ▼
┌─────────────────────────┐
│  Apaga Wi-Fi e I²C      │
└────────────┬────────────┘
             │
             ▼
┌─────────────────────────┐
│  Deep Sleep por 10 s    │
└─────────────────────────┘
```

---

## Referencias

- **Espressif Systems (2026)** — *ESP32-C3-WROOM-02 Hardware Datasheet v1.7*: Strapping pins y keep-out de antena RF 2.4 GHz.
- **Sensirion AG (2020)** — *SGP30 Gas Sensor Datasheet*: Calibración, consumo del micro-hotplate MOX e interfaz I²C.
- **Texas Instruments (2015)** — *TMP102 Digital Temperature Sensor Datasheet*: Interfaz SMBus/I²C.
- **JLCPCB (2026)** — *PCB Capabilities and Manufacturing Constraints*: Reglas físicas para prototipado rápido multicapa.

---

<sub>Proyecto académico — Asignatura Diseño Electrónico · Universidad Nacional de Colombia</sub>
