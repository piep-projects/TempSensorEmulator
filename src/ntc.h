#pragma once
#include <Arduino.h>
#include <math.h>
#include "config.h"

// Widerstand des NTC bei gegebener Temperatur
inline float ntcOhm(float tempC) {
    float T = tempC + 273.15f;
    return NTC_R25 * expf(NTC_B * (1.0f / T - 1.0f / NTC_T25_K));
}

// NTC-Widerstand → MCP4018-Schritt (0–127)
// Board nutzt W↔A (Pin B nicht an GND geführt):
// R_WA(step) = (MCP_STEPS - step) × (R_AB / MCP_STEPS) + R_W
inline uint8_t ntcToStep(float tempC) {
    float R = ntcOhm(tempC);
    float s = (float)MCP_STEPS - (R - MCP_R_W) * (float)MCP_STEPS / MCP_R_AB;
    return (uint8_t)constrain((int)roundf(s), 0, MCP_STEPS - 1);
}
