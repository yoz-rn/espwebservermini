#!/bin/sh
# Auto-dijalankan git sebelum tiap commit (via core.hooksPath).
# Mastiin gak ada file .gz yang gak sengaja ke-commit — source mentah
# selalu yang masuk history, .gz cuma artifact build lokal.

./extract_fsimage.sh data/webapp > /dev/null

# Re-stage kalau extract.sh sempat ngubah/nambah file yang udah di-stage
git add -A data/webapp

exit 0