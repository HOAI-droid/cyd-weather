#include "../src/hw.h"
uint32_t simMillis = 0;
void hwBegin() {}
void hwLed(bool, bool, bool) {}
void hwTone(uint16_t, uint16_t) {}
int hwLoadInt(const char*, int d) { return 48; }
void hwSaveInt(const char*, int) {}
int hwLightLevel() { return 0; }
void hwBacklight(uint8_t) {}
uint32_t hwMillis() { return simMillis; }
