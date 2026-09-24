#!/bin/sh
set -e

TARGET_DIR="${1:-data/webapp}"

find "$TARGET_DIR" -type f -name "*.gz" \
    ! -name "*.txt.gz" \
    ! -name "*.svg.gz" \
    ! -name "jszip.min.js.gz" \
    ! -name "webaudio-tinysynth.js.gz" \
    | while IFS= read -r gz; do

    src="${gz%.gz}"

    echo "EXTRACT: $gz -> $src"
    gzip -d -f "$gz"
done

echo "Selesai."