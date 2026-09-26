"""Erzeugt eine Beispielantwort im Open-Meteo-Format (für den Simulator)."""
import json, datetime as dt, sys
cfg = sys.argv[1] if len(sys.argv) > 1 else "partly"
now = int(dt.datetime(2026, 9, 26, 12, 0, tzinfo=dt.timezone.utc).timestamp())  # 14:00 in Berlin
temps = [17,17,16,15,14,13,12,12,11,11,10,10,10,10,11,12,13,15,16,17,18,18,17,17]
pops = [20,20,10,10,5,5,0,0,0,0,5,5,10,10,10,20,30,40,40,30,20,10,10,10]
codes = {"partly": 2, "sunny": 0, "rain": 63, "storm": 95, "snow": 71, "fog": 45, "cloud": 3, "night": 0}
c = codes[cfg]
t0 = {"sunny": 29, "snow": -2, "rain": 12, "storm": 21}.get(cfg, 17)
d = {
  "utc_offset_seconds": 7200,
  "current": {"time": now + 2220, "temperature_2m": t0 + 0.3, "relative_humidity_2m": 64, "apparent_temperature": t0 - 1,
              "is_day": 0 if cfg == "night" else 1, "precipitation": 0.0, "weather_code": c, "cloud_cover": 45,
              "pressure_msl": 1016.2, "wind_speed_10m": 14.2, "wind_direction_10m": 268, "wind_gusts_10m": 28.4},
  "hourly": {"time": [now + i * 3600 for i in range(24)], "temperature_2m": [t + (t0 - 17) for t in temps],
             "precipitation_probability": pops,
             "weather_code": [2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 1, 2, 2, 3, 3, 61, 61, 80, 80, 3, 2, 2, 2, 2],
             "is_day": [1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1]},
  "daily": {"time": [now - 12 * 3600 - 7200 + i * 86400 for i in range(6)],
            "weather_code": [c, 0, 63, 53, 2, 3],
            "temperature_2m_max": [t0 + 2, 21, 16, 14, 15, 13], "temperature_2m_min": [t0 - 7, 11, 9, 8, 7, 6],
            "sunrise": [now - 12 * 3600 - 7200 + 7 * 3600 + 120 + i * 86400 for i in range(6)],
            "sunset": [now - 12 * 3600 - 7200 + 18 * 3600 + 59 * 60 + i * 86400 for i in range(6)],
            "uv_index_max": [3.2, 4, 1.5, 2, 3, 2], "precipitation_probability_max": [20, 0, 80, 60, 10, 30]}
}
print(json.dumps(d))
