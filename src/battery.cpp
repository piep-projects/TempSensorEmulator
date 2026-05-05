#include "battery.h"
#include "config.h"

// Spannungs-zu-SoC-Stützpunkte (LiPo-Kennlinie)
static const float V_TABLE[] = { 3.20f, 3.40f, 3.60f, 3.80f, 4.00f, 4.20f };
static const float P_TABLE[] = {  0.0f, 10.0f, 30.0f, 60.0f, 85.0f, 100.0f };
static constexpr int TABLE_LEN = 6;

// Gleitender Mittelwert über 4 Messungen
static float g_samples[4] = { 3.7f, 3.7f, 3.7f, 3.7f };
static int   g_idx = 0;
static float g_voltage = 3.7f;
static float g_prevVoltage = 3.7f;

void batteryBegin() {
    analogSetAttenuation(ADC_11db);   // bis ~3,6 V am ADC-Pin (×2 = 7,2 V Bereich)
    // Erste Messung sofort
    batteryUpdate();
    for (int i = 0; i < 4; i++) g_samples[i] = g_voltage;
}

void batteryUpdate() {
    // analogReadMilliVolts korrigiert ADC-Nichtlinearität
    uint32_t mv = analogReadMilliVolts(PIN_BAT_ADC);
    float v = (mv * 2.0f) / 1000.0f;   // Spannungsteiler ÷2

    g_prevVoltage = g_voltage;
    g_samples[g_idx] = v;
    g_idx = (g_idx + 1) % 4;

    float sum = 0;
    for (int i = 0; i < 4; i++) sum += g_samples[i];
    g_voltage = sum / 4.0f;
}

float batteryVoltage() {
    return g_voltage;
}

int batteryPercent() {
    float v = g_voltage;
    if (v <= V_TABLE[0]) return 0;
    if (v >= V_TABLE[TABLE_LEN - 1]) return 100;
    for (int i = 0; i < TABLE_LEN - 1; i++) {
        if (v <= V_TABLE[i + 1]) {
            float t = (v - V_TABLE[i]) / (V_TABLE[i + 1] - V_TABLE[i]);
            return (int)(P_TABLE[i] + t * (P_TABLE[i + 1] - P_TABLE[i]));
        }
    }
    return 100;
}

bool batteryCharging() {
    // Laden erkannt wenn Spannung steigt und über Ruheschwelle liegt
    return (g_voltage > g_prevVoltage + 0.01f) || (g_voltage > 4.05f);
}
