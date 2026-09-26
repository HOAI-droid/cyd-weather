// Kleine Hardware-Abstraktion für RGB-LED, Lautsprecher und Speicher
#pragma once
#include <stdint.h>

void hwBegin();
void hwLed(bool r, bool g, bool b);     // RGB-LED auf der Rückseite
void hwTone(uint16_t freq, uint16_t ms); // 0 = aus
int  hwLoadInt(const char* key, int def);
void hwSaveInt(const char* key, int value);
int  hwLightLevel();                     // Rohwert des Lichtsensors
void hwBacklight(uint8_t level);
uint32_t hwMillis();
