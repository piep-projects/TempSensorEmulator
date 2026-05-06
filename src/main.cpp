// Wolf CHA-07 Aussenfuhler-Emulator  —  main.cpp
// piep design · v1.2.0

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

// ── Button-Handling ───────────────────────────────────────────
struct Button {
    uint8_t  pin;
    uint32_t pressedAt;
    uint32_t lastRepeat;
    bool     edgeFired;
    bool     longFired;    // Sonderfunktion bereits ausgelöst
};

static Button btnMinus{ PIN_BTN_MINUS, 0, 0, false, false };
static Button btnPlus { PIN_BTN_PLUS,  0, 0, false, false };

// Gibt delta zurück (+/- TEMP_STEP oder 0).
// Auto-Repeat stoppt 300 ms vor Ablauf der Sonderfunktion.
static float pollButton(Button& b, float delta) {
    bool pressed = (digitalRead(b.pin) == LOW);
    uint32_t now = millis();

    if (!pressed) {
        b.pressedAt = 0; b.edgeFired = false; b.longFired = false;
        return 0;
    }
    if (!b.pressedAt) { b.pressedAt = now; b.lastRepeat = now; }

    uint32_t held = now - b.pressedAt;

    if (!b.edgeFired) { b.edgeFired = true; return delta; }

    if (!b.longFired &&
        held > BTN_HOLD_MS &&
        held < BTN_ACTION_MS - 300 &&
        now - b.lastRepeat > BTN_REPEAT_MS) {
        b.lastRepeat = now;
        return delta;
    }
    return 0;
}

// ── Sonderfunktionen ──────────────────────────────────────────

static void checkSleepTrigger() {
    if (digitalRead(PIN_BTN_PLUS) != LOW || btnPlus.longFired || !btnPlus.pressedAt) return;
    if (millis() - btnPlus.pressedAt < SLEEP_HOLD_MS) return;

    btnPlus.longFired = true;
    prefsFlush();
    drawShutdown(g_tempC);
    delay(2000);
    displayBrightness(0);
    digitalWrite(PIN_POWER_ON, LOW);
    esp_sleep_enable_ext1_wakeup(
        (1ULL << PIN_BTN_MINUS) | (1ULL << PIN_BTN_PLUS),
        ESP_EXT1_WAKEUP_ANY_LOW);
    esp_deep_sleep_start();
}

static void checkWifiTrigger() {
    if (digitalRead(PIN_BTN_MINUS) != LOW || btnMinus.longFired || !btnMinus.pressedAt) return;
    if (millis() - btnMinus.pressedAt < WIFI_TRIGGER_MS) return;

    btnMinus.longFired = true;
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

    Serial.println("BOOT 3.5s = WLAN  |  KEY 3.5s = Sleep");
}

// ── Loop ──────────────────────────────────────────────────────
void loop() {
    float d = pollButton(btnMinus, -TEMP_STEP)
            + pollButton(btnPlus,  +TEMP_STEP);
    if (d != 0) {
        float t = constrain(g_tempC + d, TEMP_MIN, TEMP_MAX);
        if (t != g_tempC) applyTemp(t);
    }

    checkSleepTrigger();
    checkWifiTrigger();

    static uint32_t lastBat = 0;
    if (millis() - lastBat > BAT_UPDATE_MS) {
        batteryUpdate();
        lastBat = millis();
        g_displayDirty = true;

        if (batteryPercent() <= BAT_CRIT_PCT) {
            Serial.println("Akku kritisch — Deep Sleep");
            prefsFlush();
            drawShutdown(g_tempC);
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
        drawMain(g_tempC, ntcOhm(g_tempC), ntcToStep(g_tempC),
                 batteryPercent(), batteryCharging(),
                 ip.c_str(), mcp4018IsOk());
        g_displayDirty = false;
    }

    wifiLoop();
    delay(20);
}
