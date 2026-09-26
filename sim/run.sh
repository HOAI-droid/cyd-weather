#!/bin/sh
# Baut den Host-Simulator und erzeugt Screenshots in sim/build/*.png
# Voraussetzung: g++, python3 mit Pillow, ArduinoJson-Quellen (ARDUINOJSON=/pfad/zu/ArduinoJson/src)
set -e
cd "$(dirname "$0")"
mkdir -p build
AJ=${ARDUINOJSON:-../../bblanchon/ArduinoJson/src}
g++ -std=gnu++17 -O2 -Wall -Wno-unused-function -Imock -I../include -I../src -I"$AJ" \
  sim_main.cpp hw_sim.cpp mock/tft_mock.cpp ../src/gfx.cpp ../src/icons.cpp ../src/ui.cpp ../src/eggs.cpp ../src/weather.cpp ../src/air.cpp ../src/radar.cpp \
  -o build/sim
python3 make_air.py > build/sample_air.json
python3 make_radar.py > build/sample_radar.bin
for s in partly sunny rain storm snow fog night; do
  python3 make_sample.py $s > build/sample_$s.json
  ./build/sim $s
done
python3 - <<'PY'
import glob
from PIL import Image
for p in glob.glob("build/*.ppm"):
    Image.open(p).save(p[:-4] + ".png")
PY
