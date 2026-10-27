#!/usr/bin/env bash
# Lokale Vorschau ohne LaraPaper-Server, mit Beispieldaten.
# Nutzt dieselbe Liquid-Engine (keepsuit/liquid 0.12.1) und dieselben Filter wie LaraPaper.
# Braucht: PHP >= 8.2 (getestet mit 8.5), composer, git, node + playwright, python3 + Pillow.
#
#   trmnl/vorschau/vorschau.sh <plugin> <layout> <lokale-zeit> [szenario]
#   z. B. trmnl/vorschau/vorschau.sh abfahrt full 2026-10-27T17:20
#   Szenarien: normal, heute (Abholung heute), fehler (Haltestelle mehrdeutig), ausfall, wetter (für Seite 2)
set -euo pipefail
V=$(cd "$(dirname "$0")" && pwd); T=$(dirname "$V")
p=$1; l=$2; t=$3; s=${4:-normal}
[ "$p" = wetter ] && [ "$s" = normal ] && s=wetter
case $l in full) W=800;H=480;; half_horizontal) W=800;H=240;; half_vertical) W=400;H=480;; quadrant) W=400;H=240;; esac
B=$V/.build
if [ ! -d "$B/vendor" ]; then
  mkdir -p "$B"
  git clone -q --depth 1 https://github.com/usetrmnl/larapaper.git "$B/larapaper"
  cp -r "$B/larapaper/app/Liquid" "$B/" && mkdir -p "$B/app" && mv "$B/Liquid" "$B/app/"
  cp "$V/composer.json" "$V/render.php" "$B/"
  (cd "$B" && composer install -q)
fi
cp "$V/render.php" "$B/"
"$T/build.sh" >/dev/null
mkdir -p "$V/out"; n="$p-$l-$s"
ts=$(python3 "$V/sample.py" "$t" "$V/out/$n.json" "$s")
php "$B/render.php" "$T/dist/$p/src" "$l" "$V/out/$n.json" "$ts" > "$V/out/$n.html"
NODE_PATH=$(npm root -g) node "$V/shot.js" "$V/out/$n.html" "$V/out/$n.png" $W $H
python3 -c "from PIL import Image; Image.open('$V/out/$n.png').convert('L').point(lambda v: 255 if v > 140 else 0).convert('1').save('$V/out/$n-1bit.png')"
echo "$V/out/$n-1bit.png"
