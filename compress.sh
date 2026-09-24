#!/bin/sh
set -e

TARGET_DIR="${1:-data/webapp}"
TOTALS_FILE="/tmp/.compress_asset_totals_$$"  # $$ = PID, biar aman kalau ada run paralel

cleanup() {
    rm -f "$TOTALS_FILE"
}
trap cleanup EXIT

find "$TARGET_DIR" -type f \
    ! -name "*.mid" \
    ! -name "*.wasm" \
    ! -name "*.avif" \
    ! -name "*.png" \
    ! -name "*.jpg" \
    ! -name "*.jpeg" \
    ! -name "*.gz" \
    | while IFS= read -r src; do

    gz="${src}.gz"

    if [ -f "$gz" ] && [ "$gz" -nt "$src" ]; then
        if gzip -t "$gz" 2>/dev/null; then
            echo "SKIP (sudah ada .gz valid & lebih baru): $src"
            rm "$src"
        else
            echo "WARNING: $gz ada tapi corrupt/rusak, kompres ulang dari source"
            size_before=$(stat -c%s "$src" 2>/dev/null || stat -f%z "$src")
            gzip -9 -f "$src"
            size_after=$(stat -c%s "$gz" 2>/dev/null || stat -f%z "$gz")
            saved=$(( 100 - (size_after * 100 / size_before) ))
            printf "COMPRESS: %-50s %8d bytes -> %8d bytes (hemat %d%%)\n" "$src" "$size_before" "$size_after" "$saved"
            echo "$size_before $size_after" >> "$TOTALS_FILE"
        fi
        continue
    fi

    size_before=$(stat -c%s "$src" 2>/dev/null || stat -f%z "$src")

    gzip -9 -f "$src"

    size_after=$(stat -c%s "$gz" 2>/dev/null || stat -f%z "$gz")

    if [ "$size_before" -gt 0 ]; then
        saved=$(( 100 - (size_after * 100 / size_before) ))
    else
        saved=0
    fi

    printf "COMPRESS: %-50s %8d bytes -> %8d bytes (hemat %d%%)\n" "$src" "$size_before" "$size_after" "$saved"

    echo "$size_before $size_after" >> "$TOTALS_FILE"
done

echo "--------------------------------"

if [ -f "$TOTALS_FILE" ]; then
    total_before=$(awk '{sum+=$1} END {print sum+0}' "$TOTALS_FILE")
    total_after=$(awk '{sum+=$2} END {print sum+0}' "$TOTALS_FILE")

    if [ "$total_before" -gt 0 ]; then
        total_saved=$(( 100 - (total_after * 100 / total_before) ))
        echo "Total: $total_before bytes -> $total_after bytes (hemat $total_saved%)"
    fi
else
    echo "Tidak ada file baru yang dikompres."
fi

echo "Selesai."