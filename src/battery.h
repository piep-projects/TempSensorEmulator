#pragma once
#include <Arduino.h>

void batteryBegin();
void batteryUpdate();    // alle BAT_UPDATE_MS aufrufen
float batteryVoltage();  // Spannung in Volt
int   batteryPercent();  // 0–100
bool  batteryCharging(); // true wenn USB lädt
