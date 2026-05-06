#pragma once
#include <Arduino.h>

void displayBegin();
void displayBrightness(uint8_t val);  // 0–255

void drawSplash();
void drawMain(float tempC, float ohm,
              int batPct, bool charging,
              const char* wifiIP, bool i2cOk);
void drawWifiSetup();
void drawShutdown(float tempC, int countdownSec = 10);
