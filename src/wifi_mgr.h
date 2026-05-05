#pragma once
#include <Arduino.h>

// Zeiger auf g_tempC setzen (vor wifiBegin aufrufen)
void wifiSetTempPtr(float* ptr);

// Blockiert bis verbunden oder Captive Portal abgeschlossen.
// Ruft drawWifiSetup() auf wenn AP-Modus startet.
void wifiBegin();

// HTTP-Server in loop() bedienen
void wifiLoop();

// Aktuell zugewiesene IP (oder "AP 192.168.4.1" oder "---")
String wifiIP();

bool wifiConnected();
