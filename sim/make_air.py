"""Beispielantwort der Open-Meteo-Luftqualitäts-API (für den Simulator)."""
import json
print(json.dumps({"current": {"time": 1790416800, "interval": 3600, "european_aqi": 28, "pm2_5": 8.2, "pm10": 14.1,
                              "nitrogen_dioxide": 11.4, "ozone": 62.0, "alder_pollen": 0.0, "birch_pollen": 0.0,
                              "grass_pollen": 3.1, "mugwort_pollen": 12.5, "olive_pollen": 0.0, "ragweed_pollen": 1.4}}))
