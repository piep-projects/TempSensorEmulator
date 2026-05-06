// Wolf CHA-07 Aussenfuhler-Emulator  —  main.cpp
// piep design · v1.3.0

#include <Arduino.h>
#include "config.h"
#include "ntc.h"
#include "mcp4018.h"
#include "battery.h"
#include "prefs.h"
#include "display.h"
#include "wifi_mgr.h"

// ── Zustand ───────────────────────────────────────────────────
static float g_tempC        = TEMP_DEFAULT;
static bool  g_displayDirty = true;

// ── Button-Handling (Option B: Auslösung beim Loslassen) ──────
enum BtnEvent { BTN_NONE, BTN_SHORT, BTN_LONG };

struct Button {
    uint8_t  pin;
    uint32_t pressedAt;   // millis() beim Drücken (0 = nicht gedrückt)
    bool     prevPressed;
};

static Button btnMinus{ PIN_BTN_MINUS, 0, false };
static Button btnPlus { PIN_BTN_PLUS,  0, false };

// Erkennt Kurzdruck (<BTN_ACTION_MS) und Langdruck (>=BTN_ACTION_MS) beim Loslassen.
// Kein Auto-Repeat — eine Aktion pro Tastenbetätigung.
static BtnEvent pollButton(Button& b) {
    bool pressed = (digitalRead(b.pin) == LOW);

    if (pressed && !b.prevPressed) {
        b.pressedAt = millis();
    } else if (!pressed && b.prevPressed && b.pressedAt) {
        uint32_t held = millis() - b.pressedAt;
        b.pressedAt = 0;
        b.prevPressed = false;
        return (held >= BTN_ACTION_MS) ? BTN_LONG : BTN_SHORT;
    }

    b.prevPressed = pressed;
    return BTN_NONE;
}

// ── Sonderfunktionen ──────────────────────────────────────────

static void doSleep() {
    prefsFlush();
    for (int s = SHUTDOWN_COUNTDOWN_S; s >= 0; s--) {
        drawShutdown(g_tempC, s);
        if (s > 0) delay(1000);
    }
    displayBrightness(0);
    digitalWrite(PIN_POWER_ON, LOW);
    esp_sleep_enable_ext1_wakeup(
        (1ULL << PIN_BTN_MINUS) | (1ULL << PIN_BTN_PLUS),
        ESP_EXT1_WAKEUP_ANY_LOW);
    esp_deep_sleep_start();
}

static void doWifi() {
    wifiStartPortal();
    g_displayDirty = true;
}

// ── Temperatur setzen ─────────────────────────────────────────
static void applyTemp(float t) {
    g_tempC = t;
    mcp4018Set(ntcToStep(t));
    prefsSaveTemp(t);
    g_displayDirty = true;
    Serial.printf("T=%.1f°C  R=%.0fΩ  Step=%d  I2C=%s\n",
                  t, ntcOhm(t), ntcToStep(t),
                  mcp4018IsOk() ? "OK" : "ERR");
}

// ── Setup ─────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Serial.println("\n=== Wolf CHA-07 Aussenfuhler-Emulator " FW_VERSION " ===");

    pinMode(PIN_BTN_MINUS, INPUT_PULLUP);
    pinMode(PIN_BTN_PLUS,  INPUT_PULLUP);

    displayBegin();
    drawSplash();
    delay(1500);

    bool i2cOk = mcp4018Begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Serial.printf("MCP4018: %s\n", i2cOk ? "OK" : "NICHT GEFUNDEN");

    prefsBegin();
    g_tempC = prefsLoadTemp();
    applyTemp(g_tempC);

    batteryBegin();

    wifiSetTempPtr(&g_tempC);
    wifiBegin();

    Serial.println("Kennlinie:");
    for (int t = -15; t <= 30; t += 5)
        Serial.printf("  %+3d°C → %.0fΩ → Step %d\n",
                      t, ntcOhm(t), ntcToStep(t));

    Serial.println("BOOT loslassen <3.5s = -0.5°C  |  KEY loslassen <3.5s = +0.5°C");
    Serial.println("BOOT >=3.5s loslassen = WLAN   |  KEY >=3.5s loslassen = Sleep");
}

// ── Loop ──────────────────────────────────────────────────────
void loop() {
    BtnEvent evMinus = pollButton(btnMinus);
    BtnEvent evPlus  = pollButton(btnPlus);

    if (evMinus == BTN_SHORT || evPlus == BTN_SHORT) {
        float d = (evMinus == BTN_SHORT ? -TEMP_STEP : 0.0f)
                + (evPlus  == BTN_SHORT ? +TEMP_STEP : 0.0f);
        float t = constrain(g_tempC + d, TEMP_MIN, TEMP_MAX);
        if (t != g_tempC) applyTemp(t);
    }

    if (evMinus == BTN_LONG) doWifi();
    if (evPlus  == BTN_LONG) doSleep();

    static uint32_t lastBat = 0;
    if (millis() - lastBat > BAT_UPDATE_MS) {
        batteryUpdate();
        lastBat = millis();
        g_displayDirty = true;

        if (batteryPercent() <= BAT_CRIT_PCT) {
            Serial.println("Akku kritisch — Deep Sleep");
            prefsFlush();
            drawShutdown(g_tempC, 3);
            delay(3000);
            displayBrightness(0);
            digitalWrite(PIN_POWER_ON, LOW);
            esp_sleep_enable_ext1_wakeup(
                (1ULL << PIN_BTN_MINUS) | (1ULL << PIN_BTN_PLUS),
                ESP_EXT1_WAKEUP_ANY_LOW);
            esp_deep_sleep_start();
        }
    }

    if (g_displayDirty) {
        String ip = wifiIP();
        drawMain(g_tempC, ntcOhm(g_tempC),
                 batteryPercent(), batteryCharging(),
                 ip.c_str(), mcp4018IsOk());
        g_displayDirty = false;
    }

    wifiLoop();
    delay(20);
}
