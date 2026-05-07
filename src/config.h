#pragma once

// ── Firmware ──────────────────────────────────────────────────
#define FW_VERSION      "v1.4.0"

// ── T-Display-S3 Tasten ───────────────────────────────────────
#define PIN_BTN_MINUS    0    // BOOT-Taste  (active LOW)
#define PIN_BTN_PLUS    14    // KEY-Taste   (active LOW)

// ── Display Power ─────────────────────────────────────────────
#define PIN_POWER_ON    15    // LCD Versorgung (active HIGH)

// ── LCD 8080-Parallelbus ──────────────────────────────────────
#define PIN_LCD_D0      39
#define PIN_LCD_D1      40
#define PIN_LCD_D2      41
#define PIN_LCD_D3      42
#define PIN_LCD_D4      45
#define PIN_LCD_D5      46
#define PIN_LCD_D6      47
#define PIN_LCD_D7      48
#define PIN_LCD_WR       8
#define PIN_LCD_RD       9
#define PIN_LCD_DC       7
#define PIN_LCD_CS       6
#define PIN_LCD_RST      5
#define PIN_LCD_BL      38

// ── I²C QWIIC ────────────────────────────────────────────────
#define PIN_I2C_SDA     18
#define PIN_I2C_SCL     17

// ── Batterie ADC ─────────────────────────────────────────────
#define PIN_BAT_ADC      4    // Spannungsteiler ÷2 on-board

// ── MCP4018T-503 ─────────────────────────────────────────────
#define MCP4018_ADDR    0x2F
#define MCP_R_AB        50000.0f   // 50 kΩ Ende-Ende
#define MCP_R_W            75.0f   // Wiper-Widerstand (typ.)
#define MCP_STEPS           128    // Schritte (0 … 127)

// ── NTC Wolf CHA-07, Art. 2748916 ────────────────────────────
#define NTC_R25         5000.0f    // 5 kΩ bei 25 °C
#define NTC_B           3977.0f    // B-Wert (Schätzwert — verifizieren!)
#define NTC_T25_K        298.15f   // 25 °C in Kelvin

// ── Temperaturbereich ─────────────────────────────────────────
#define TEMP_MIN        -15.0f
#define TEMP_MAX         30.0f
#define TEMP_STEP         0.5f
#define TEMP_DEFAULT     10.0f

// ── Tasten-Timing ─────────────────────────────────────────────
// Option B: Auslösung beim Loslassen, kein Auto-Repeat
#define BTN_ACTION_MS    3500UL   // >= = Sonderfunktion beim Loslassen
#define SHUTDOWN_COUNTDOWN_S  10  // Sekunden Countdown vor Deep Sleep

// ── Batterie ──────────────────────────────────────────────────
#define BAT_WARN_PCT       15     // Icon blinkt rot
#define BAT_CRIT_PCT        5     // automatisch Deep Sleep
#define BAT_UPDATE_MS   10000UL   // Messintervall

// ── WiFi ──────────────────────────────────────────────────────
#define WIFI_AP_SSID    "CHA-Emulator"
#define WIFI_AP_PASS    "wolf1234"
#define WIFI_TIMEOUT_S     10     // Verbindungs-Timeout

// ── Webserver ─────────────────────────────────────────────────
#define WEB_PORT           80
