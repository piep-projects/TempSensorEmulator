#include "prefs.h"
#include "config.h"
#include <Preferences.h>

static Preferences g_prefs;
static constexpr const char* NS  = "emulator";
static constexpr const char* KEY = "temp";

// Entprelltes Schreiben: nicht öfter als alle 2 s
static uint32_t g_lastSave = 0;
static float    g_pendingTemp = TEMP_DEFAULT;
static bool     g_dirty = false;

void prefsBegin() {
    g_prefs.begin(NS, false);
}

float prefsLoadTemp() {
    return g_prefs.getFloat(KEY, TEMP_DEFAULT);
}

void prefsSaveTemp(float tempC) {
    g_pendingTemp = tempC;
    g_dirty = true;
    uint32_t now = millis();
    if (now - g_lastSave >= 2000UL) {
        g_prefs.putFloat(KEY, g_pendingTemp);
        g_lastSave = now;
        g_dirty = false;
    }
}

// Sofortiges Schreiben — vor Deep Sleep aufrufen
void prefsFlush() {
    if (g_dirty) {
        g_prefs.putFloat(KEY, g_pendingTemp);
        g_dirty = false;
    }
}
