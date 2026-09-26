#pragma once
#include "gfx.h"

void uiDrawScreen(Gfx& g);                       // aktuelle Ansicht (ein Streifen)
void uiDrawBoot(Gfx& g, const char* status, int step);
void uiDrawPortal(Gfx& g, const char* apName, const char* note);
bool uiIsLeetTime();

// Touch-Zonen auf "Jetzt"
bool uiHitIcon(int x, int y);
bool uiHitClock(int x, int y);
bool uiHitCity(int x, int y);

// Ortswahl
bool uiHitAction(int x, int y);        // "Fertig" / "Abbrechen" / "Zurück" oben rechts
int uiHitPlaceRow(int x, int y);       // gespeicherter Ort oder -1
bool uiHitPlacesSearch(int x, int y);
bool uiHitPlacesWifi(int x, int y);
int uiHitResult(int x, int y);         // Suchtreffer oder -1
// Taste unter dem Finger: -1 = keine, sonst KeyCode (0 = Zeichen in *ch)
enum { KEY_CHAR, KEY_BACK, KEY_ALT, KEY_SPACE, KEY_SEARCH };
int uiKeyAt(int x, int y, const char** ch);
