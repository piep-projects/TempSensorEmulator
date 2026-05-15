#pragma once
#include <Arduino.h>

bool mcp4018Begin(int sda, int scl);
bool mcp4018Set(uint8_t step);
bool mcp4018IsOk();
void mcp4018ScanBus();
