#include "mcp4018.h"
#include "config.h"
#include <Wire.h>

static bool g_ok = false;

bool mcp4018Begin(int sda, int scl) {
    Wire.begin(sda, scl);
    Wire.setClock(100000);
    Wire.beginTransmission(MCP4018_ADDR);
    g_ok = (Wire.endTransmission() == 0);
    return g_ok;
}

bool mcp4018Set(uint8_t step) {
    for (int attempt = 0; attempt < 3; attempt++) {
        Wire.beginTransmission(MCP4018_ADDR);
        Wire.write(step & 0x7F);
        if (Wire.endTransmission() == 0) {
            g_ok = true;
            return true;
        }
    }
    g_ok = false;
    return false;
}

bool mcp4018IsOk() {
    return g_ok;
}
