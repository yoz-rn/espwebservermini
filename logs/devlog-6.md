# Recap Sesi: Memory Monitor, Refactor Network Event-Driven, README & Persiapan Merge

**Project:** espwebservermini
**Branch:** `hybrid-espidf` (target merge: `main`)
**Status akhir sesi:** Panel `mem` (heap) di Task Monitor selesai dan tervalidasi di device fisik. Probe debug FreeRTOS dipindah ke `TaskManager`. Logika Network diubah dari task-wrapper ke event-driven (kode disusun; hasil uji serial belum dicatat). README dirombak untuk pemula, detail teknis dipindah ke `docs/`. Uji pra-merge bagian A dan C selesai, bagian B (jalur STA) dilewati. Arah rilis: Task Monitor sebagai fitur utama untuk memantau task FreeRTOS di aplikasi embedded lewat antarmuka web; fitur lain boleh dipangkas.

---

## 1. Heap di `/api/tasks` (Memory Monitor, backend)

- **Keputusan:** heap ikut respons `/api/tasks` yang sama, bukan endpoint baru. Alasan: satu polling per interval lebih murah dibanding dua request, dan penambahan field tidak menghambat skalabilitas.
- Satu panggilan `heap_caps_get_info(&heap, MALLOC_CAP_INTERNAL)` mengisi semua angka (lebih murah daripada tiga fungsi terpisah yang masing-masing menelusuri heap). Field JSON: `heap.free`, `heap.min` (titik terendah sejak boot), `heap.blk` (blok bebas terbesar), `heap.size` (dihitung `total_free_bytes + total_allocated_bytes`, karena struct tidak punya field total).
- Dibaca **sebelum** `JsonDocument` dialokasikan, supaya `free` adalah keadaan sebelum endpoint ini memakai memori. Catatan: `min` tetap mencatat puncak alokasi request-request sebelumnya.
- Data curl (20 sampel): `size` ≈ 331 KB, `free` ≈ 235 KB, `blk` konstan 110592 byte, `min` hanya turun (230756 → 230028). `free` bergerak ±1–2 KB antar request (buffer koneksi).
- **Koreksi analisis:** `blk` lebih kecil dari `free` itu normal (heap internal terbagi beberapa region), bukan tanda fragmentasi. Yang bermakna adalah *tren* `blk`. Dugaan awal bahwa `size` yang menyusut adalah fragmentasi ditarik: di log serial `size` hanya berfluktuasi (331000 / 330988 / 331004).

---

## 2. Panel `mem` (frontend)

- `task-monitor.html`: panel `mem` di antara `cpu` dan `proc`, bar `MEM` (pemakaian sekarang = `size − free`) dan `PEAK` (`size − min`), plus baris meta total / free / largest block. `blk` sengaja teks biasa, bukan bar (tidak bermakna dibanding `free`).
- `task-monitor.js`: `fmtBytes()` dan `showHeap()`, dipasang di `render()`. Ada guard `if (!h)` supaya frontend baru tetap jalan dengan firmware lama tanpa field heap.
- **Bug lama ditemukan:** `cell(text)` hanya menerima satu parameter, sementara pemanggilnya mengirim dua (`cell(r.free, "num " + stackClass(r.free))`), sehingga kelas CSS dibuang: warna peringatan stack, warna state (`st-running`), dan perataan `num` tidak pernah aktif. Fix: `cell(text, cls)`.
- **CSS (3 masalah dari screenshot):**
  - Label `PEAK` terpotong: `.tm-core-label` lebar `2ch` → `4ch`.
  - Nilai terbungkus 3 baris: `.tm-pct` lebar `6ch` → `17ch` + `white-space: nowrap` (bar CPU ikut memendek supaya semua bar berujung di titik sama).
  - Bar tampak penuh walau nilainya kecil: lintasan dan isi sulit dibedakan, dan `opacity` di `.bar` ikut meredupkan isinya (opacity induk berlaku ke anak). Fix: lintasan dipindah ke `::before` dengan opacity sendiri (0.35, nilai tebakan).
- **Pola bug:** lebar tetap yang terlalu sempit untuk isinya, kerabat dekat bug `min-height` vs `height` di devlog-3/4. Layak jadi prinsip: ukur teks terpanjang sebelum menetapkan lebar kolom.

---

## 3. Uji heap di device (27 menit, idle)

- Screenshot 22:11 → 22:38: `MEM` 96,4 → 96,1 KiB, `free` 226,9 → 227,1 KiB, `blk` tetap 108,0 KiB. **Tidak ada indikasi kebocoran.**
- `PEAK` naik 129,3 → 147,8 KiB: lonjakan sementara, `MEM` kembali ke dasar. Sesudah reboot bersih `PEAK` hanya 98,5 KiB, jadi lonjakan sebelumnya kemungkinan dari aktivitas tab lain (dugaan, belum diatribusikan).
- Batas kesimpulan: kebocoran di bawah ~1–2 KiB per 30 menit tidak akan terlihat. Soak semalam dilewati karena keterbatasan resource.

---

## 4. Review `TaskManager` (catatan yang berdampak stabilitas)

- **Umur objek (terverifikasi aman):** `TaskManager myTask;` global di `main.cpp`, jadi `_buf` tidak ikut hilang saat `loopTask` dihapus lewat `vTaskDelete(NULL)`.
- **Pointer nama task (belum diubah, peluang kecil):** `t["name"] = s.pcTaskName` menyimpan pointer ke TCB (perlu dicek di dokumentasi ArduinoJson v7). Task berumur pendek yang dihapus antara snapshot dan `serializeJson()` bisa meninggalkan pointer menggantung.
- **Aturan thread-safety:** `_buf` hanya untuk `toJson()`, yang hanya dipanggil dari `async_tcp`. Sekarang tertulis sebagai komentar di `TaskManager.h`.
- Belum dikerjakan (backlog): `#error` untuk `CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID`; `out.reserve(measureJson(doc))` untuk mengurangi pertumbuhan `String` bertahap.

---

## 5. Probe debug dipindah ke `TaskManager`

- Blok komentar probe di `main.cpp` (Snap/`takeSnap`/`printCpuLoad`/`printTasks`) dipindah menjadi `TaskManager::printTasks()` dan `TaskManager::printCpuLoad()`, didokumentasikan lewat komentar header.
- Keduanya memakai **buffer sendiri**, bukan `_buf`: probe dulu dipanggil setelah `server.begin()`, jadi bisa berbalapan dengan `toJson()` di `async_tcp`.
- `printCpuLoad()` memblokir ±1 detik (`delay(1000)`): panggil dari `setup()`, jangan dari handler route. Pada dual-core, `SUM` ≈ 200%, bukan bug.
- **Bug di probe lama diperbaiki:** loop dalam `for (j < b.n)` mengindeks `a.t[j]`; kalau ada task baru muncul selama jeda 1 detik, loop membaca melewati ujung buffer `a`. Batas jadi `a.n`.
- Perbaikan lain: pengaman `dTotal == 0` (tanpa itu hasilnya `inf`/`nan`), core `tskNO_AFFINITY` dicetak `-1` (sama dengan JSON), state dicetak sebagai nama lewat `stateName()`.
- Diuji lewat serial: sesuai ekspektasi.

---

## 6. Uji pra-merge

**A. Statis**
- [x] Objek `TaskManager` global (aman).
- [x] Kode probe dipindah dan `main.cpp` dibersihkan; hasil sesuai ekspektasi.
- [x] Isi commit dicek lewat version control VS Code.
- Nilai default runtime di build hybrid (grep `sdkconfig.<env>`): `ARDUINO_RUNNING_CORE=1`, `ARDUINO_LOOP_STACK_SIZE=8192` (sama dengan Arduino); **CPU 160 MHz** (beda: Arduino biasanya 240 MHz); **task watchdog 5 detik** memeriksa `IDLE0` dan `IDLE1`, tanpa panic (hanya peringatan). Blok kedua `CONFIG_TASK_WDT_*` kemungkinan alias kompatibilitas lama (dugaan dari letak dan nilainya, belum diperiksa di sumber IDF).
- **Keputusan:** pertahankan 160 MHz dan dokumentasikan; pemangkasan `sdkconfig.defaults` (790+ baris) ditunda.

**B. Regresi di device**
- **Dilewati.** Koreksi dari sesi ini: Wi-Fi menyala, tetapi sinyal fluktuatif dan tidak pernah penuh walau di samping laptop (bukan "tanpa antena"). Web app tetap lancar, RTO jarang.
- Bukti tambahan: high score (Preferences/NVS) tetap terbaca setelah banyak flash dan reset, jadi baca-tulis NVS di tabel partisi baru sehat.
- Belum diuji: provisioning STA, `GOT_IP`/mDNS tanpa reset, kredensial terbaca dari NVS setelah cabut daya. Kemungkinan bisa dicoba dengan hotspot ponsel 2.4 GHz di samping board.

**C. Task Monitor**
- [x] `#error` diuji dengan membalik sementara kondisi `#if`: pesan terbaca jelas.
- [x] `MAX_TASKS` diturunkan: endpoint membalas **HTTP 500**, UI menampilkan `error: HTTP 500`. Dugaan awal (200 dengan `success:false` → error TypeError di `render()`) keliru; `poll()` sudah menangani `!res.ok`.
- [x] Dua klien sekaligus (laptop + ponsel): aman. Tab yang tersembunyi berhenti polling (sesuai `visibilitychange`).
- [x] Layar sempit (ponsel): cukup responsif, butuh tweak kecil.
- [x] **Reboot ESP dengan tab terbuka:** satu frame ngawur, tujuh task tampil 100,2%. **Bug nyata.** Penyebab: `prev` masih menyimpan counter lama yang jauh lebih besar; selisih unsigned (`>>> 0`) berputar ke angka raksasa untuk semua task. Selisih unsigned tepat untuk wrap-around 32-bit, tetapi tidak bisa membedakan wrap dari reboot.
  - Perbaikan yang diusulkan: field `up` (`esp_timer_get_time() / 1000000`, detik sejak boot) di JSON; frontend membuang `prev` bila `up` mundur. Firmware lama tanpa `up` aman (`undefined < x` bernilai `false`). **Status pemasangan: (isi manual)**
- [ ] Soak semalam: dilewati (resource), dilakukan mandiri kelak.

---

## 7. Refactor Network: task-wrapper → event-driven

- **Temuan:** `NetworkTask` hanya mencetak jumlah klien AP tiap 10 detik. Tidak ada logika sambung-ulang di dalamnya. Penghematan resource kecil (stack 4096 B + TCB, CPU 0,0%); keuntungan utama adalah terminal yang tidak lagi berisik dan arsitektur lebih sederhana.
- **Rancangan (4 langkah):**
  1. `NetworkManager.h`: buang `_networkTaskHandle`/`taskWrapper`/`taskLoop`; tambah `_started` (guard pengganti) dan `std::atomic<bool> _staConnected`.
  2. `beginAP()`: validasi dipindah ke atas (kode lama memanggil `softAPConfig` **sebelum** cek "sudah jalan", sehingga panggilan kedua mengonfigurasi ulang AP yang aktif), `WiFi.onEvent` didaftarkan sekali sebelum Wi-Fi menyala.
  3. `onWifiEvent()`: log hanya saat transisi. `AP_STACONNECTED/DISCONNECTED` (MAC klien), `STA_GOT_IP` (cetak IP + `MDNS.begin`), `STA_DISCONNECTED` (bila sebelumnya tersambung: `MDNS.end` + peringatan; percobaan gagal berulang dicetak di level DEBUG supaya sepi).
  4. `beginSTA()` diperkecil (mDNS dan cetak IP pindah ke handler); "belum ada kredensial" bukan lagi dilaporkan sebagai kegagalan.
- Handler berjalan di task `arduino_events` (stack sisa ~3 KB): dijaga singkat, tanpa `delay`.
- **Dugaan akar bug lama (devlog-1: "pakai DNS harus reset ESP dulu"):** `MDNS.begin` dulu hanya dipanggil di `beginSTA()` saat boot, tidak setelah provisioning lewat landing page. Menyalakannya dari `GOT_IP` seharusnya menutup celah itu. Belum terverifikasi.
- **Logging:** modul Network memakai `ESP_LOGx` dengan tag `network`. `ESP_LOGx` dan `Serial` berdampingan di UART0 (hanya format yang beda). Konversi sisa `Serial.println` di fungsi provisioning ditunda.
- **Efek samping yang diharapkan:** Task Monitor menampilkan 11 task (bukan 12); `MEM` sesudah boot turun sekitar 4–5 KiB dari 93,0 KiB (dugaan). Karena sinyal goyah, log sisi AP bisa menampakkan klien yang sering putus-sambung: itu sinyal, bukan bug handler.
- **Hasil uji serial / Task Monitor: (isi setelah diuji)**

**Catatan dari membaca `NetworkManager.cpp` / `main.cpp` (belum dikerjakan):**
- `NVSTest()` menimpa kredensial asli dengan dummy (`TestSSID`). Tidak dipanggil, tetapi sebaiknya dihapus sebelum rilis.
- Typo string status "Mnguji Koneksi" (tampil di UI provisioning).
- `LittleFS.begin(true)` = format-on-fail: kegagalan mount sesaat bisa menghapus seluruh isi filesystem. Pertimbangkan `false`.
- `beginSTA()` masih memblokir sampai 10 detik saat boot bila router tidak terjangkau; versi non-blocking berbasis event mungkin, tetapi baru bisa diuji lewat hotspot.

---

## 8. README & docs

- **Getting Started** ditulis ulang jadi 5 langkah bahasa awam: clone, `./setup.sh` (mengunduh `esp_littlefs` v1.16.4 ke `components/esp_littlefs`), `secrets.h` (kini hanya SSID/password AP milik ESP, minimal 8 karakter; mode Station lewat halaman Wi-Fi Setup), urutan Build → Upload → Upload Filesystem Image (urutan penting karena tabel partisi ikut tertulis di Upload), buka dashboard.
- Detail teknis dipindah ke dua berkas baru di `docs/` (tidak ikut terflash, tidak memakan jatah filesystem):
  - `docs/TROUBLESHOOTING.md`: per langkah, termasuk `sdkconfig`, partisi, watchdog, error 261, pesan `#error`.
  - `docs/TASK-MONITOR.md`: opsi sdkconfig yang dibutuhkan, skema JSON `/api/tasks`, rumus CPU%, cara membaca bar memori.
- Roadmap: bahasanya disederhanakan; Memory Monitor `[x]`; item Partition Awareness dan Tetris Polish dihapus (keputusan sendiri).
- Bagian "Known Limitations" ditambahkan (Wi-Fi rumah / `esp32.local` belum diuji ulang, belum ada uji multi-hari, uji sampai dua browser).
- Kredit `snek.mid` sudah ditambahkan.
- Temuan devlog-5 §1 yang kini beres: "1,375 MiB" → 1.375 MiB; instruksi `secrets.h` mode Station yang usang dibuang.
- **Klaim yang belum terverifikasi di docs:** catatan Windows/Git Bash untuk `setup.sh`, "password minimal 8 karakter" (perilaku `softAP`), dan penjelasan bahwa task yang menunggu tanpa batas dapat tampil `Suspended` (contoh: `esp_timer`).

---

## 9. Rencana Merge `hybrid-espidf` → `main`

- Gerbang sebelum merge: `git status` bersih dan branch sudah di-push; `git diff main...hybrid-espidf --stat` tanpa file nyasar; **klon bersih dari branch** (`git clone -b hybrid-espidf ...`) lalu ikuti README dari Step 1 sampai dashboard tampil (README menyuruh clone `main`, yang belum memuat hybrid, jadi uji ini harus menyebut branch); devlog-6 ikut ter-commit.
- Merge dengan merge commit (`--no-ff` / "Create a merge commit") supaya riwayat branch terjaga dan ada satu titik revert. Branch dipertahankan sampai klon bersih dari `main` lolos.

---

## 10. Backlog Baru

- [ ] Pastikan field `up` terpasang di firmware dan frontend; tambahkan ke contoh JSON di `docs/TASK-MONITOR.md`.
- [ ] Uji jalur STA lewat hotspot ponsel: provisioning, mDNS tanpa reset, `STA terputus`, kredensial dari NVS setelah cabut daya.
- [ ] Cek alert rekor Snek (casing `highScore` di `game-shell.js`).
- [ ] `#error` untuk `CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID`.
- [ ] Salin nama task ke JSON (atau pastikan perilaku `const char*` di ArduinoJson); `out.reserve(measureJson(doc))`.
- [ ] Hapus `NVSTest()`; perbaiki typo "Mnguji Koneksi"; pertimbangkan `LittleFS.begin(false)`.
- [ ] Ganti sisa `Serial.println` di fungsi provisioning dengan `ESP_LOGx`.
- [ ] `beginSTA()` non-blocking berbasis event.
- [ ] Pangkas `sdkconfig.defaults` (790+ baris).
- [ ] Soak test semalam dan atribusi lonjakan `PEAK` (reboot, lalu satu aksi, bandingkan `min`).
- [ ] Segarkan `roadmap-link.svg` (masih memuat centang lama).
- [ ] Bersihkan sisa di filesystem image (belum dikonfirmasi): folder legacy `assets/gz/`, `ost-snek.opus.gz` di root, `README.md` dan `myminegw.avif` di `data/`.

## Lampiran

**Berkas baru/berubah sesi ini:**

```
docs/
├── TROUBLESHOOTING.md        (baru)
└── TASK-MONITOR.md           (baru)
logs/devlog-6.md              (baru)
src/TaskManager.h/.cpp        (heap, printTasks/printCpuLoad; uptime `up` bila terpasang)
src/NetworkManager.h/.cpp     (event-driven, ESP_LOGx)
data/task-monitor.html
data/assets/task-monitor/task-monitor.{js,css}
README.md
```