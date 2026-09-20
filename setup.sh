#!/usr/bin/env bash
set -euo pipefail

ESP_LITTLEFS_TAG="v4.4.7"
DEST="components/esp_littlefs"

command -v pio >/dev/null || { echo "PlatformIO CLI (pio) tidak ditemukan"; exit 1; }
command -v git >/dev/null || { echo "git tidak ditemukan"; exit 1; }

if [ ! -d "$DEST" ]; then
  mkdir -p components
  git clone --depth 1 --branch "$ESP_LITTLEFS_TAG" \
      --recurse-submodules --shallow-submodules \
      https://github.com/joltwallet/esp_littlefs.git "$DEST"
fi

if [ "${1:-}" = "flash" ]; then
  pio run -t upload && pio run -t uploadfs
else
  pio run
fi