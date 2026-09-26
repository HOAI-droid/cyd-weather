// Kleiner Test für Ortssuche-URL und -Parser
#include <stdio.h>
#include <string.h>
#include "../src/weather.h"
int main() {
  char url[256];
  int fails = 0;
  buildGeocodeUrl(url, sizeof(url), "  München , Bayern");
  printf("%s\n", url);
  if (strcmp(url, "/v1/search?name=M%C3%BCnchen&count=1&language=de&format=json")) fails++;
  buildGeocodeUrl(url, sizeof(url), "Bremen-Vegesack");
  printf("%s\n", url);
  if (!strstr(url, "name=Bremen-Vegesack&")) fails++;
  buildGeocodeUrl(url, sizeof(url), "Bad Honnef");
  printf("%s\n", url);
  if (!strstr(url, "name=Bad%20Honnef&")) fails++;
  const char* js = "{\"results\":[{\"id\":2867714,\"name\":\"München\",\"latitude\":48.13743,\"longitude\":11.57549,\"country\":\"Deutschland\",\"admin1\":\"Bayern\"}],\"generationtime_ms\":0.5}";
  char name[48]; float lat, lon;
  bool ok = parseGeocode(js, name, sizeof(name), lat, lon);
  printf("%d %s %.4f %.4f\n", ok, name, lat, lon);
  if (!ok || strcmp(name, "München") || lat < 48.1f || lon < 11.5f) fails++;
  if (parseGeocode("{\"generationtime_ms\":0.5}", name, sizeof(name), lat, lon)) fails++;  // nichts gefunden
  printf(fails ? "FEHLER: %d\n" : "alle Tests ok\n", fails);
  return fails;
}
