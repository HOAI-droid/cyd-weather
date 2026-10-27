"""Beispieldaten im exakten Format der echten Quellen (Open-Meteo, dbf.finalrewind.org v3, EAD Darmstadt).
Aufruf: python3 sample.py <lokale Zeit YYYY-MM-DDTHH:MM> <ausgabe.json> [szenario]"""
import json, sys, datetime as dt, math, random

lokal = dt.datetime.fromisoformat(sys.argv[1])
out = sys.argv[2]
szen = sys.argv[3] if len(sys.argv) > 3 else "normal"
OFF = 3600  # MEZ
random.seed(7)

# ---------------- Open-Meteo (timezone=auto) ----------------
start_h = lokal.replace(hour=0, minute=0)
codes = [1, 1, 2, 2, 3, 3, 3, 3, 3, 2, 2, 3, 3, 61, 61, 63, 80, 80, 3, 3, 2, 1, 0, 0] * 2
hourly = {"time": [], "temperature_2m": [], "precipitation_probability": [], "weather_code": [], "is_day": []}
for i in range(48):
    t = start_h + dt.timedelta(hours=i)
    hourly["time"].append(t.strftime("%Y-%m-%dT%H:%M"))
    hh = i % 24
    hourly["temperature_2m"].append(round(9.5 + 3.9 * math.sin((hh - 9) / 24 * 2 * math.pi) + random.uniform(-.3, .3), 1))
    hourly["precipitation_probability"].append([5,5,5,5,5,10,10,10,15,15,20,20,30,55,70,80,65,40,25,15,10,5,5,5][hh])
    hourly["weather_code"].append(codes[i])
    hourly["is_day"].append(1 if 7 <= t.hour < 17 else 0)
m15 = {"time": [], "precipitation": []}
q = lokal.replace(minute=lokal.minute // 15 * 15)
for i in range(9):
    m15["time"].append((q + dt.timedelta(minutes=15 * i)).strftime("%Y-%m-%dT%H:%M"))
    m15["precipitation"].append([0, 0, 0, 0, 0.2, 0.6, 0.8, 0.4, 0.1][i])
day0 = lokal.date()
wetter = {
    "latitude": 49.87, "longitude": 8.65, "utc_offset_seconds": OFF, "timezone": "Europe/Berlin",
    "timezone_abbreviation": "GMT+1", "elevation": 144.0,
    "current": {"time": q.strftime("%Y-%m-%dT%H:%M"), "interval": 900, "temperature_2m": 11.8,
                "apparent_temperature": 9.4, "relative_humidity_2m": 78, "is_day": 1 if 7 <= lokal.hour < 17 else 0,
                "weather_code": 3, "wind_speed_10m": 14.2, "precipitation": 0.0},
    "hourly": hourly, "minutely_15": m15,
    "daily": {"time": [str(day0), str(day0 + dt.timedelta(days=1))], "weather_code": [61, 2],
              "temperature_2m_max": [13.4, 12.1], "temperature_2m_min": [6.2, 4.8],
              "precipitation_probability_max": [80, 15],
              "sunrise": [f"{day0}T07:21", f"{day0 + dt.timedelta(days=1)}T07:23"],
              "sunset": [f"{day0}T17:05", f"{day0 + dt.timedelta(days=1)}T17:03"], "uv_index_max": [2.4, 1.9]},
}

# ---------------- dbf.finalrewind.org JSON (version=3, HAFAS) ----------------
deps = []
base = lokal.hour * 60 + lokal.minute
plan = [(-1, "K", "Darmstadt Hauptbahnhof", 0), (2, "K", "Darmstadt Hauptbahnhof", 1), (9, "F", "Darmstadt Schloss", 0),
        (14, "K", "Darmstadt Hauptbahnhof", 3), (24, "F", "Darmstadt Schloss", 0), (29, "K", "Darmstadt Hauptbahnhof", 0),
        (39, "F", "Darmstadt Schloss", 0), (44, "K", "Darmstadt Hauptbahnhof", 0), (54, "F", "Darmstadt Schloss", 0)]
for i, (d, line, dest, delay) in enumerate(plan):
    sched = (base + d - delay) % 1440
    rt = (base + d) % 1440
    f = lambda m: f"{m // 60:02d}:{m % 60:02d}"
    deps.append({"time": f(rt), "sched_departure": f(sched), "departure": f(rt), "train": f"Bus {line}",
                 "train_type": "", "train_line": line, "train_no": str(4100 + i), "via": ["Darmstadt Willy-Brandt-Platz"],
                 "destination": dest, "origin": dest, "platform": None, "scheduled_platform": None,
                 "is_cancelled": 1 if (szen == "ausfall" and i == 3) else 0, "delay": delay, "is_delayed": 1 if delay >= 5 else 0,
                 "has_realtime": 1, "station": "Darmstadt-Arheilgen Im Fiedlersee", "linetype": "bus"})
abfahrten = {"departures": deps}
if szen == "rmv":
    rd = []
    for i, (d, line, dest, delay) in enumerate(plan):
        sched = (base + d - delay) % 1440; rt = (base + d) % 1440
        f = lambda m: f"{m // 60:02d}:{m % 60:02d}:00"
        stops = [{"name": "Darmstadt Im Fiedlersee", "extId": "3004999", "routeIdx": 3},
                 {"name": "Darmstadt Willy-Brandt-Platz" if line == "K" or i % 3 else "Darmstadt Ostbahnhof", "extId": "3004111", "routeIdx": 4},
                 {"name": dest, "extId": "3004222", "routeIdx": 9}]
        x = {"name": f"Bus {line}", "type": "ST", "stop": "Darmstadt Im Fiedlersee", "stopExtId": "3004999",
             "time": f(sched), "date": str(day0), "direction": dest, "Stops": {"Stop": stops},
             "ProductAtStop": {"name": f"Bus {line}", "line": line, "displayNumber": line, "catOutL": "Bus"}}
        if delay or i % 2: x["rtTime"] = f(rt); x["rtDate"] = str(day0)
        if i == 4: x["cancelled"] = True
        rd.append(x)
    abfahrten = {"Departure": rd, "serverVersion": "2.45", "dialectVersion": "2.45"}
if szen == "fehler":
    abfahrten = {"api_version": "6.x", "error": "ambiguous station code/name"}

# ---------------- EAD Darmstadt ----------------
def k(n):
    return (day0 + dt.timedelta(days=n)).strftime("%d.%m.%Y")
muell = {k(-6): ["RM2"], k(1): ["WET", "BIO"], k(8): ["RM2", "BIO"], k(15): ["BIO", "PPK"], k(22): ["RM2", "WET"]}
if szen == "heute":
    muell = {k(0): ["WET", "BIO"], k(7): ["RM2", "BIO"], k(14): ["BIO", "PPK"]}

json.dump({"IDX_0": wetter, "IDX_1": abfahrten, "IDX_2": muell}, open(out, "w"), ensure_ascii=False, indent=1)
utc = int((lokal - dt.timedelta(seconds=OFF)).replace(tzinfo=dt.timezone.utc).timestamp())
print(utc)

if szen == "wetter":
    h0 = lokal.replace(minute=0)
    hr = {"time": [], "temperature_2m": [], "precipitation_probability": [], "weather_code": [], "is_day": []}
    for i in range(25):
        t = h0 + dt.timedelta(hours=i); hh = t.hour
        hr["time"].append(t.strftime("%Y-%m-%dT%H:%M"))
        hr["temperature_2m"].append(round(9.5 + 3.9 * math.sin((hh - 9) / 24 * 2 * math.pi), 1))
        hr["precipitation_probability"].append([5,5,5,5,5,10,10,10,15,15,20,20,30,55,70,80,65,40,25,15,10,5,5,5][hh])
        hr["weather_code"].append(codes[hh]); hr["is_day"].append(1 if 7 <= hh < 17 else 0)
    days = [day0 + dt.timedelta(days=i) for i in range(6)]
    wetter["current"].update({"relative_humidity_2m": 78, "pressure_msl": 1012.4, "wind_direction_10m": 232, "wind_gusts_10m": 31.0})
    wetter["hourly"] = hr
    wetter["daily"] = {"time": [str(d) for d in days], "weather_code": [61, 2, 3, 80, 0, 1],
        "temperature_2m_max": [13.4, 12.1, 10.8, 9.6, 11.2, 12.5], "temperature_2m_min": [6.2, 4.8, 5.1, 3.9, 2.4, 4.0],
        "sunrise": [f"{d}T07:{21 + i * 2:02d}" for i, d in enumerate(days)], "sunset": [f"{d}T17:{5 - i:02d}" for i, d in enumerate(days)],
        "uv_index_max": [1.8, 2.1, 1.2, 1.5, 2.4, 2.2], "precipitation_probability_max": [80, 15, 30, 65, 5, 10]}
    luft = {"latitude": 49.88, "longitude": 8.65, "current": {"time": q.strftime("%Y-%m-%dT%H:%M"), "interval": 3600,
        "european_aqi": 27, "pm2_5": 8.4, "pm10": 13.9, "nitrogen_dioxide": 18.2, "ozone": 41.0,
        "alder_pollen": 0.0, "birch_pollen": 0.0, "grass_pollen": 3.2, "mugwort_pollen": 0.4, "olive_pollen": 0.0, "ragweed_pollen": 1.1}}
    json.dump({"IDX_0": wetter, "IDX_1": luft}, open(out, "w"), ensure_ascii=False, indent=1)
