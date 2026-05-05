#pragma once
#include <Arduino.h>

void  prefsBegin();
float prefsLoadTemp();
void  prefsSaveTemp(float tempC);
void  prefsFlush();    // sofortiges Schreiben vor Deep Sleep
