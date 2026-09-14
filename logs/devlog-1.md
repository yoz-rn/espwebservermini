# Recap Sesi: Dashboard ESP32 & Provisioning WiFi

**Project:** espwebservermini
**Status akhir sesi:** Provisioning WiFi via landing page berhasil end-to-end (AP + STA aktif, kredensial tersimpan di NVS, bertahan lewat restart)

---

## 1. Debugging Server File Statis (awal sesi)

- **404 `/favicon.ico`** — dikonfirmasi sebagai behavior normal browser, bukan bug.
- **ASCII art gagal render** — akar masalah: fetch langsung ke path `.gz` tanpa `Content-Encoding: gzip`, menghasilkan raw binary bytes. Solusi: manfaatkan auto-gzip serving bawaan `ESPAsyncWebServer` (`serveStatic` otomatis mendeteksi file `.gz` dan menambahkan header yang benar saat path diminta tanpa suffix `.gz`).
- **304 Not Modified membingungkan** — dijelaskan sebagai caching normal (ETag/If-None-Match), bukan error. Solusi development: `setCacheControl("no-store")` sementara untuk menghindari kebingungan cache lama vs baru.
- **`esp32.txt.gz` 30 byte (korup)** — root cause: file source kosong saat proses `gzip` manual dijalankan, bukan masalah di pipeline serving. Pelajaran: firmware dan filesystem image (`uploadfs`) adalah dua proses upload terpisah di PlatformIO.

---

## 2. Fase 1 — Dual-Mode WiFi (AP + STA)

- `NetworkManager` didesain dengan `beginAP()` dan `beginSTA()`, mode `WIFI_AP_STA` supaya AP tetap aktif sambil STA mencoba connect ke router.
- mDNS (`ESPmDNS.h`) ditambahkan agar bisa diakses via `http://esp32.local`.
- Kredensial STA disimpan di **NVS** (`Preferences.h`) — dipilih karena lebih sesuai untuk data kecil yang sering berubah, dibanding LittleFS yang lebih cocok untuk file besar.
- Method utama: `saveSTACredentials()`, `hasSavedCredentials()`, `testSTACredentials()` (validasi kredensial dengan timeout, terpisah dari commit ke NVS).
- Fallback ke `secrets.h` akhirnya **dihapus total** — NVS jadi satu-satunya sumber kebenaran kredensial STA di boot-time.

---

## 3. Fase 2 — Provisioning via Landing Page

Bagian paling menantang di sesi ini — 3 insiden crash berturut-turut, masing-masing dengan akar masalah berbeda.

### Insiden 1: Crash use-after-free (`AsyncWebServerRequest`)
- **Desain awal:** task FreeRTOS terpisah memegang `AsyncWebServerRequest*` untuk mengirim response belakangan (setelah validasi WiFi selesai).
- **Gejala:** `Guru Meditation Error (LoadProhibited)`, kadang cuma response JSON korup tanpa crash.
- **Riset:** dikonfirmasi lewat GitHub issue `ESPAsyncWebServer` dan pernyataan langsung dari maintainer (me-no-dev) — memegang `request` lintas-task adalah anti-pattern yang diketahui.
- **Fix:** redesain total ke **arsitektur polling** — handler POST langsung respons instan (ack), task background hanya update status internal (`enum ProvisioningStatus`), browser polling `GET /api/wifi-status` secara berkala.

### Insiden 2: `WiFi.mode()` race condition
- **Gejala:** log `ap bss deleted`, browser dapat `NetworkError`, ESP32 crash.
- **Akar masalah:** `WiFi.mode(WIFI_AP_STA)` dipanggil ulang di setiap `testSTACredentials()`, membongkar-ulang interface AP saat browser sedang aktif terhubung ke AP tersebut.
- **Fix:** `WiFi.mode()` dipindah jadi panggilan **tunggal** di `beginAP()`, tidak pernah disentuh lagi selama runtime.

### Insiden 3: Bug kombinasi (const-correctness, typo, race condition)
- `request->getParam()` di versi library (`ESP32Async/ESPAsyncWebServer` fork) mengembalikan `const AsyncWebParameter*`, bukan `AsyncWebParameter*` — perlu penyesuaian tipe.
- Typo nama parameter (`"pass"` vs `"password"`) sempat bikin `getParam()` selalu gagal.
- Race condition pada `_provisioningMessage` (`String`, bukan tipe atomic) dibaca-tulis lintas-task tanpa proteksi — menghasilkan JSON korup saat polling. **Fix:** `SemaphoreHandle_t` (mutex) membungkus akses baca/tulis status & pesan.
- Typo logic: `_provisioningStatus == ...` (perbandingan) alih-alih `=` (assignment).
- Typo string status (`'SUCCESS'` vs `'success'`) antara backend C++ dan frontend JS — celah yang sudah diidentifikasi sejak awal (perbedaan bahasa, tidak ada validasi lintas-compiler) akhirnya benar-benar terjadi.

### Hasil Akhir
```
STA connected, IP: 10.136.181.58
mDNS responder aktif: http://esp32.local
[Provisioning] Sukses, kredensial disimpan.
```
Reboot berikutnya menunjukkan `POWERON_RESET` bersih (bukan crash), kredensial STA otomatis ter-load dari NVS.

---

## 4. Arsitektur Akhir (Referensi)

**Backend:**
- `NetworkManager.h/.cpp` — AP/STA lifecycle, NVS, provisioning state + mutex
- `ResponseHelper.h/.cpp` — helper `buildStatusJson()` (ArduinoJson), reusable lintas-modul routes
- `routes/WifiRoutes.h/.cpp` — `POST /api/wifi-config` (mulai provisioning), `GET /api/wifi-status` (polling)
- `routes/TaskRoutes.h/.cpp` — task manager routes (existing)

**Frontend:**
- `data/index.html` — dashboard ASCII art
- `data/wifi-setup.html` — form provisioning + polling loop
- `data/assets/` — shared CSS/JS, ASCII art terkompresi gzip

**Pola desain kunci yang dipelajari:**
- Jangan pegang `AsyncWebServerRequest*` lintas-task — respons instan + polling status terpisah.
- `WiFi.mode()` adalah operasi berat (rebuild interface) — panggil sekali di awal, jangan berulang saat ada koneksi aktif.
- Data yang diakses lintas-task butuh proteksi eksplisit (mutex), termasuk tipe yang terlihat "aman" seperti `String`.
- String yang harus cocok antar-bahasa (C++ ↔ JS) adalah titik rawan typo yang tidak tertangkap compiler manapun.

---

## 5. Daftar Saran Pengembangan (belum diprioritaskan)

Referensi untuk diseleksi mandiri ke roadmap:

- [ ] Terapkan `namespace WifiApi { constexpr ... }` untuk path route & string status (mengurangi risiko typo yang sudah 2x terjadi)
- [ ] Cek `common.js` & `style.css` — tercatat 0 byte di LittleFS, kemungkinan belum terisi
- [ ] Hapus dead code `_sta_ssid`/`_sta_password` dari constructor `NetworkManager`
- [ ] Navigasi antar halaman (`index.html` ↔ `wifi-setup.html`)
- [ ] Indikator status koneksi (AP-only vs STA connected) di dashboard
- [ ] Background auto-reconnect loop untuk STA (retry berkala pakai kredensial NVS)
- [ ] Fitur "lupakan WiFi" — reset kredensial NVS tanpa reflash
- [ ] Modul File Manager (lanjutan milestone berikutnya, `FileManager.h/.cpp` sudah ada di project)
- [ ] Catatan post-mortem ringkas soal 3 insiden crash provisioning, untuk referensi cepat di modul lain