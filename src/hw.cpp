#include "hw.h"
#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

// Pins des ESP32-2432S028R ("Cheap Yellow Display")
static const int PIN_LED_R = 4, PIN_LED_G = 16, PIN_LED_B = 17;  // aktiv LOW
static const int PIN_SPEAKER = 26;
static const int PIN_LDR = 34;
static const int PIN_BACKLIGHT = 21;
static const int BL_CHANNEL = 7;  // LEDC-Kanal für die Hintergrundbeleuchtung (tone() nutzt Kanal 0)

static Preferences prefs;

void hwBegin() {
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  hwLed(false, false, false);
  pinMode(PIN_LDR, INPUT);
  analogSetPinAttenuation(PIN_LDR, ADC_0db);  // empfindlicher für den kleinen LDR
  ledcSetup(BL_CHANNEL, 5000, 8);
  ledcAttachPin(PIN_BACKLIGHT, BL_CHANNEL);
  hwBacklight(BRIGHTNESS_DAY);
  prefs.begin("wetter", false);
}

void hwLed(bool r, bool g, bool b) {
  digitalWrite(PIN_LED_R, r ? LOW : HIGH);
  digitalWrite(PIN_LED_G, g ? LOW : HIGH);
  digitalWrite(PIN_LED_B, b ? LOW : HIGH);
}

void hwTone(uint16_t freq, uint16_t ms) {
  if (!SOUND_ENABLED || freq == 0) { noTone(PIN_SPEAKER); return; }
  tone(PIN_SPEAKER, freq, ms);
}

int hwLoadInt(const char* key, int def) { return prefs.getInt(key, def); }
void hwSaveInt(const char* key, int value) { prefs.putInt(key, value); }
int hwLightLevel() { return analogRead(PIN_LDR); }
void hwBacklight(uint8_t level) { ledcWrite(BL_CHANNEL, level); }
uint32_t hwMillis() { return millis(); }
