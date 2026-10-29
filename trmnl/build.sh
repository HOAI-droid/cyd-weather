#!/usr/bin/env bash
# Baut aus trmnl/plugins/* fertige ZIP-Pakete für den Import in LaraPaper
# (Plugins → Import Recipe Archive). Ergebnis: trmnl/dist/trmnl-<plugin>.zip
set -euo pipefail
cd "$(dirname "$0")"
rm -rf dist && mkdir -p dist
for dir in plugins/*/; do
  [ -f "$dir/settings.yml" ] || continue
  name=$(basename "$dir")
  out="dist/$name/src"
  mkdir -p "$out"
  cp "$dir/settings.yml" "$out/"
  # shared.liquid = gemeinsame Grundlage + Logik der Seite (wird vor jedes Layout gesetzt)
  cat common/basis.liquid $(ls "$dir"/zusatz-*.liquid 2>/dev/null) "$dir/logik.liquid" common/spruch.liquid > "$out/shared.liquid"
  for layout in full half_horizontal half_vertical quadrant; do
    if [ -f "$dir/$layout.liquid" ]; then cp "$dir/$layout.liquid" "$out/"
    elif [ -f "$dir/kompakt.liquid" ] && [ "$layout" != full ]; then cp "$dir/kompakt.liquid" "$out/$layout.liquid"; fi
  done
  (cd "dist/$name" && zip -qr "../trmnl-$name.zip" src)
  echo "dist/trmnl-$name.zip"
done
