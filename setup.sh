#!/usr/bin/env bash
set -euo pipefail

# Pinned to the version verified to build with this project.
# Adjust the tag format if `git ls-remote --tags` shows it without the "v".
ESP_LITTLEFS_TAG="v1.16.4"
DEST="components/esp_littlefs"

command -v git >/dev/null || { echo "git not found"; exit 1; }

# Always run from the project root (where this script lives)
cd "$(dirname "$0")"

if [ -d "$DEST" ]; then
  echo "[setup] $DEST already exists, skipping."
else
  echo "[setup] Fetching esp_littlefs $ESP_LITTLEFS_TAG ..."
  mkdir -p components
  git clone --depth 1 --branch "$ESP_LITTLEFS_TAG" \
      --recurse-submodules --shallow-submodules \
      https://github.com/joltwallet/esp_littlefs.git "$DEST"
fi

echo "[setup] Done. Next in VS Code: PlatformIO > Build, Upload, Upload Filesystem Image."