# Recap Sesi: Task Monitor & Migrasi Hybrid Arduino + ESP-IDF

**Project:** espwebservermini
**Status akhir sesi:** Snek terintegrasi dan berjalan di device, roadmap README diperbarui. Arsitektur build dimigrasi ke **hybrid (Arduino sebagai komponen ESP-IDF)** supaya FreeRTOS trace facility bisa diaktifkan. Build sukses, LittleFS ter-mount, dan `GET /api/tasks` beserta halaman `task-monitor.html` (versi tabel minimal) terverifikasi di device fisik. Redesain UI bergaya btop, pengaktifan tautan sidebar/dashboard, dan uji regresi menyeluruh **sudah disusun tapi belum dikonfirmasi berjalan** (lihat §9). Heap monitor dilanjutkan di sesi terpisah.

---

## 1. Update Roadmap README

Roadmap lama dicocokkan dengan devlog-1 sampai 4. Perubahan dan alasannya:

- **Snek** tadinya dicentang bersama Tetris, padahal devlog-4 mencatat baru backend WASM-nya yang siap. Dipisah jadi item tersendiri, dicentang setelah integrasi sesi ini (§2).
- **Task Monitor** disebut di README (summary dan bagian Navigating) tapi tidak ada di roadmap. Dibuat **Phase 4: Monitoring & Tooling**.
- Sisa pekerjaan dari devlog lama dijadikan item `[ ]`: indikator status koneksi, STA auto-reconnect, "forget Wi-Fi", protected files list, partition awareness, next-piece Tetris, dan pause/reset Tetris.
- Fitur selesai yang belum tercatat ditambahkan: rename file, path traversal protection, high score persisten, sidebar bersama, layout mobile.
- Item kebersihan kode internal (konstanta `namespace WifiApi`, post-mortem, hapus `common.js` kosong) sengaja tidak masuk roadmap.

Ambiguitas yang masih terbuka:

- Devlog-4 mencentang "2 game tambahan" (Game of Life, Minesweeper), padahal isinya baru kandidat. Di roadmap tetap `[ ]`.
- `roadmap-link.svg` di README kemungkinan gambar roadmap dan perlu disegarkan manual (tidak terbaca dari chat).

Temuan lain di README (belum diperbaiki):

- Ukuran flash tertulis "~1,375 MB (1441792 bytes)". Angka itu 1,375 MiB, format ribuannya salah.
- Langkah "copy the command below" pada bagian build tidak diikuti perintah apa pun.
- Instruksi `secrets.h` untuk mode STA kemungkinan usang, karena devlog-1 mencatat fallback ke `secrets.h` sudah dihapus dan NVS jadi satu-satunya sumber kredensial STA.

---

## 2. Integrasi Snek

`snek.html` disusun dari `tetris.html` dan `game-layout.css`, dengan perbedaan:

- Panel `next piece` dan tombol Pause/Reset dihapus (backend Snek belum punya logikanya).
- Canvas **400×400**, dari `COLS × CELL_SIZE` = 20 × 20 di `snek.c`.
- Pemetaan tombol: `0=up, 1=down, 2=left, 3=right`, berbeda dari Tetris.
- Restart lewat tombol Start. Key `4` di `snek.c` tidak terpakai dari JS, karena setelah game over `wasmInstance` di-reset ke `null` dan handler `keydown` langsung `return`. Efeknya WASM dimuat ulang dari nol.
- `try/catch` di handler Start, supaya tombol tidak tertinggal disabled kalau path `.wasm` salah.
- Path aset relatif terhadap halaman: `snek/snek.wasm` dan musik `snek/<file>.mid` (`GameAudio` menyusun `${gameName}/${filename}`).
- Ikut terbukti di device: backend menerima nama game `snek`, dan `GET /api/game/score?game=snek` mengembalikan `highscore` (huruf kecil, sama seperti Tetris).

### Bug dan perbaikan

1. **Layout mengikuti Tetris.** `game-layout.css` didesain untuk canvas portrait 1:2: `#game-canvas { height: 100%; width: auto }` dan `.game-canvas-container { height: 100% }`. Canvas persegi ikut membengkak. Perbaikan lewat kelas pengubah supaya Tetris tidak terpengaruh:
   - `#game-canvas.canvas-square { width: 300px; height: auto; max-width: 100%; max-height: none; }`
   - `.game-canvas-container.container-square { height: auto; }`

   Patch pertama hanya menyentuh canvas dan lupa container-nya, sehingga tampilan tetap portrait. Selector dua kelas (id+class atau class+class) lebih spesifik dari aturan dasar, jadi tidak perlu `!important`.
2. **Atribut canvas vs ukuran tampilan.** `width`/`height` di tag `<canvas>` adalah resolusi gambar internal dan harus sama dengan area yang digambar WASM (400×400). Ukuran tampilan diatur lewat CSS. Mengubah atribut jadi 300 akan memotong gambar (bug yang sama dengan kasus 300×600 vs 200×400 di devlog-4). Pakai kelipatan 20 untuk ukuran CSS supaya tiap sel jatuh di piksel bulat.
3. **Banner ASCII bergeser.** Baris art di dalam `<pre>` yang diberi indentasi HTML ikut dirender sebagai spasi. Perbaikan: rata-kiri-kan isi `<pre>` dan tag penutupnya.

**Keputusan:** ukuran canvas tidak bisa dibuat lebih besar dari sekarang tanpa perubahan lebih besar pada layout bersama, jadi **ditolerir** (known limitation, bukan blocker). Rentetan bug CSS `height: 100%` pada container flex/grid kini muncul di beberapa halaman, dan pola ini layak dijadikan prinsip umum.

---

## 3. Task Monitor: Model Mental & Penghalang

Milestone ini sempat mangkrak sekitar sebulan karena arsitekturnya berganti-ganti dan cara kerja task di ESP32 belum dipahami. Yang dipahami sesi ini:

- ESP32 menjalankan **FreeRTOS**. Task adalah fungsi dengan stack sendiri, dengan state Running / Ready / Blocked / Suspended / Deleted. Scheduler memilih task Ready berprioritas tertinggi per core. Task yang menunggu tanpa batas waktu (mis. `esp_timer`) dilaporkan **Suspended**, bukan Blocked.
- Task yang ada bukan hanya buatan sendiri: `IDLE0/IDLE1` (satu per core), `wifi`, `tiT` (lwIP), `async_tcp` (yang melayani request web), `ipc0/ipc1`, `esp_timer`, `Tmr Svc`, dan lain-lain. Monitor akan menampilkan task yang melayani halamannya sendiri.
- Daftar seluruh task hanya bisa diambil lewat `uxTaskGetSystemState()`, yang **hanya ikut dikompilasi kalau kernel dibangun dengan `configUSE_TRACE_FACILITY=1`**. Query per handle (mis. `uxTaskGetStackHighWaterMark(handle)`) hanya berguna untuk task yang handle-nya dipegang.
- **Penghalang:** framework Arduino di PlatformIO memakai kernel yang sudah dikompilasi. Mendefinisikan makro di kode sendiri tidak berpengaruh (error `undefined reference to uxTaskGetSystemState`).

Tiga jalan keluar yang dipertimbangkan:

1. `framework = espidf` penuh (opsi bisa lewat menuconfig, tapi kode Arduino dan library harus dipindah).
2. **Arduino sebagai komponen ESP-IDF** (`framework = arduino, espidf`), kode lama dipertahankan. **Dipilih.**
3. Tetap di Arduino dengan instrumentasi sendiri (wrapper `xTaskCreate`): hanya task buatan sendiri, tanpa task sistem dan tanpa CPU%.

Catatan: tool acuan "mabutrace" tidak ditemukan lewat pencarian. Fitur diasumsikan sebagai monitor task real-time generik (daftar task, state, prioritas, stack, CPU).

---

## 4. Migrasi Hybrid: Kronologi Troubleshooting

Kronologi awal: tambah `espidf` di `platformio.ini` → langsung edit `CONFIG_FREERTOS_HZ=1000` di `sdkconfig.<env>` yang sudah ada → build → error.

### 4.1 `undefined reference to app_main`
- **Penyebab:** di build Arduino biasa, core menyediakan `app_main()` (membuat `loopTask` lalu memanggil `setup()`/`loop()`). Di hybrid, core hanya menyediakannya kalau mode autostart aktif.
- **Fix:** `CONFIG_AUTOSTART_ARDUINO=y`. Ganti baris `# ... is not set`, jangan ditambah di bawahnya.
- **Catatan konfigurasi:** `sdkconfig.defaults` hanya dibaca kalau `sdkconfig.<env>` belum ada. Kalau file hasil generate sudah ada, edit langsung di sana, atau hapus supaya digenerate ulang dari defaults. Jangan dicampur. Nama opsi yang benar `CONFIG_FREERTOS_HZ` (bukan `CONFIG_RTOS_HZ`); nilai 1000 adalah yang diharapkan core Arduino.

### 4.2 LittleFS: `undefined reference` ke `fs::LittleFSFS::*` dan `LittleFS`
- **Penyebab:** di hybrid, Arduino dikompilasi sebagai komponen dari sumber, pustaka bawaan yang sudah terkompilasi (tempat LittleFS di build Arduino biasa) tidak dipakai, dan LittleFS bukan bagian IDF.
- **Percobaan 1:** `src/idf_component.yml` (dependensi `joltwallet/littlefs`) + baris `list(APPEND EXTRA_COMPONENT_DIRS managed_components)` di `CMakeLists.txt` root. Hasil: error berpindah menjadi `esp_littlefs.h: No such file or directory` di `LittleFS.cpp`.
- **Diagnosis:** `managed_components/joltwallet__littlefs/` terunduh dan header-nya ada, jadi dependensi (dan versinya) **bukan** masalah. `CMakeLists.txt` core Arduino (baris 268-269) memanggil `maybe_add_component(esp_littlefs)`, yang hanya menghubungkan komponen dengan nama persis `esp_littlefs`. Nama komponen mengikuti nama folder, sedangkan yang diunduh bernama `joltwallet__littlefs`.
- **Fix:** pindahkan ke `components/esp_littlefs` (`mkdir -p components && mv managed_components/joltwallet__littlefs components/esp_littlefs`), lalu hapus `managed_components/`, `dependencies.lock`, `src/idf_component.yml`, dan baris `EXTRA_COMPONENT_DIRS`. Hasil: `[SUCCESS] Took 176.97 seconds` (RAM 10.2%, Flash 86.0% dari 1 MB).
- Saran `-I` lewat `build_flags` sempat diajukan lalu dibatalkan: file itu dikompilasi oleh CMake IDF, bukan lewat `build_flags`.

### 4.3 Runtime: `Mounting LittleFS failed! Error: 261`
- **Arti:** 261 = 0x105 = `ESP_ERR_NOT_FOUND`, yaitu partisi LittleFS tidak ditemukan (bukan filesystem rusak).
- **Penyebab:** tabel partisi default hybrid adalah *single app* (satu partisi aplikasi 1 MB, tanpa partisi data). Petunjuknya angka 1048576 pada "maximum program size" dan `CONFIG_PARTITION_TABLE_FILENAME="partitions_singleapp.csv"` di sdkconfig.
- **Fix:** `partitions.csv` ditambahkan ke root proyek (berbasis tabel default Arduino), didaftarkan lewat `board_build.partitions = partitions.csv` dan `board_build.filesystem = littlefs`, sdkconfig hasil generate dihapus lalu build ulang, dan firmware di-flash **sebelum** Upload Filesystem Image.
- **Hasil:** `[LittleFS] Total: 1441792 bytes | Used: 450560 bytes | Free: 991232 bytes`. Totalnya sama persis dengan build Arduino lama dan angka di README.
- **Efek samping yang diperkirakan:** offset partisi `nvs` bisa berubah sehingga kredensial Wi-Fi hilang dan provisioning perlu diulang. Ruang firmware seharusnya naik dari 1 MB (belum diukur ulang).

### 4.4 Versi esp_littlefs dan `setup.sh`
- Versi yang terunduh dan terbukti build: **1.16.4** (tag terbaru di repo: 1.22.3). Diputuskan **memakai 1.16.4** demi reproduksibilitas: toolchain gcc 8.4.0 menunjukkan generasi IDF lama, dan kompatibilitas 1.22.3 belum diperiksa. Tag disimpan di satu variabel skrip.
- Repo contoh yang dirujuk memakai fork platform Tasmota (2024.01.01), bukan platform espressif32 resmi, jadi tidak identik dengan setup ini.
- `components/` sengaja **tidak** di-commit. Resep yang di-commit: `platformio.ini`, `sdkconfig.defaults`, `partitions.csv`, `setup.sh`. `setup.sh` melakukan `git clone --depth 1 --branch <tag> --recurse-submodules --shallow-submodules` ke `components/esp_littlefs` (submodule littlefs upstream wajib ikut). Nama folder tujuan `esp_littlefs` adalah kuncinya (§4.2). Tanpa PIO CLI (memakai ekstensi VS Code), dan `chmod +x` perlu dicek lewat `git ls-files -s` (mode `100755`).
- `.gitignore`: `/components/`, `/managed_components/`, `/dependencies.lock`, `/sdkconfig.*`, lalu `!/sdkconfig.defaults` (urutan penting).
- Build dilaporkan aman di satu komputer. Cache global `~/.platformio` bisa menutupi masalah unduhan, jadi uji di mesin lain masih layak.

### 4.5 Opsi trace di sdkconfig
- Baris `CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS is not set=y` (rusak) di `sdkconfig.defaults` diperbaiki jadi `CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y`.
- Opsi `CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID` tidak ketemu di file defaults karena defaults itu dump penuh dari sdkconfig lama, dan opsi turunan tidak tertulis kalau induknya belum aktif. Yang perlu dicek adalah file **hasil generate**. Setelah `USE_STATS_FORMATTING_FUNCTIONS=y` aktif, opsi core muncul (dependensinya konsisten dengan urutan barisnya, tapi tidak diverifikasi langsung).
- Hasil akhir yang terverifikasi di sdkconfig hasil generate:
  ```
  CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS=y
  CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID=y
  CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y
  ```
  (`USE_TRACE_FACILITY=y` sudah aktif dari awal.) Perlu dicek ulang bahwa keempatnya juga bersih di `sdkconfig.defaults`, supaya clone bersih mendapat konfigurasi yang sama.

---

## 5. Probe Trace Facility

Probe 1, `printTasks()` ke Serial. Error kompilasi `'TaskStatus_t' has no member named 'xCoreID'`: member itu **opsional** dan hanya ada kalau `CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID` aktif. Diperbaiki dengan `#if CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID` di sekitar kolom core (kode aman untuk kedua kondisi). Hasil (13 task, dengan `loopTask` masih hidup): `IDLE0` core 0, `IDLE1` core 1, `wifi`/`esp_timer`/`sys_evt`/`Tmr Svc`/`ipc0` di core 0, `loopTask`/`ipc1`/`NetworkTask`/`arduino_events` di core 1, `async_tcp` dan `tiT` bernilai `2147483647` (tidak dipaku ke core, sesuai `CONFIG_FREERTOS_NO_AFFINITY=0x7FFFFFFF`).

Probe 2, persentase CPU dari selisih dua snapshot (dicocokkan lewat `xTaskNumber`, jeda 1 detik):

- `IDLE0` 99,56% dan `IDLE1` 99,63%, **`SUM` = 199,98%**, `dTotal` = 1.004.263.
- **Bukan bug:** counter berupa waktu dinding dalam mikrodetik (`dTotal` ≈ 1 detik), tersedia sekali untuk tiap core, sehingga dua core bisa berjumlah 200%.
- **Beban per core dari task idle:** core 0 = 100 − 99,56 = 0,44%, core 1 = 100 − 99,63 = 0,37%. Angka ini konsisten dengan data task, dan menjadi cara menghitung beban core yang benar walau task seperti `async_tcp` dan `tiT` tidak terikat core.
- Selisih counter **harus unsigned 32-bit** (`>>> 0` di JS), karena counter melilit sekitar tiap 71 menit (2³² µs, diturunkan dari satuan mikrodetik).

---

## 6. Backend `GET /api/tasks`

Kode lama (`TaskManager`, `TaskRoutes`) ditinjau lalu isi `TaskManager` dirombak, strukturnya dipertahankan.

Temuan di kode lama:
- `toJson()` mengisi `"priority"` dengan `task.state` (salah ketik).
- Buffer 32 slot: kalau task melebihi buffer, `uxTaskGetSystemState()` mengembalikan 0 dan hasilnya daftar kosong tanpa pesan.
- `std::vector<TaskInfo>` dengan `String name` mengalokasikan heap tiap poll hanya untuk disalin ke JSON.
- Sisa percobaan: `// #define configUSE_TRACE_FACILITY 1` (makro tidak bisa diaktifkan dari kode), fungsi `getAllTasks()` yang isinya dikomentari.
- JSON berupa array di root, padahal butuh field `total`.
- `registerTaskRoutes` juga mendaftarkan handler `/`, `onNotFound`, dan `serveStatic("/assets/")` (setup server global yang nyasar). **Belum dipindah**, butuh melihat `main.cpp`.

Desain baru:
- `bool toJson(String& out)`; buffer `TaskStatus_t` 48 slot dipakai ulang (tanpa alokasi heap per request); cek `uxTaskGetNumberOfTasks() > MAX_TASKS` sebelum snapshot supaya kegagalan eksplisit (HTTP 500 lewat `buildStatusJson`).
- `#error` saat kompilasi jika `CONFIG_FREERTOS_USE_TRACE_FACILITY` atau `CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS` tidak aktif (menangkap kasus baris sdkconfig rusak).
- `core` dipetakan `2147483647 → -1` di sisi C++.
- Buffer `_buf` aman selama hanya handler route (task `async_tcp`) yang memanggil `toJson()`. Kalau task lain ikut memanggil, perlu mutex (pelajaran `_provisioningMessage` di devlog-1).

Bentuk respons:
```json
{"success":true,"total":138730706,"tasks":[{"n":15,"name":"async_tcp","state":"Running","prio":10,"free":12940,"rt":26947,"core":-1}, ...]}
```

Hasil uji `curl` di device: 12 task, `total` = 138.730.706 µs (≈ uptime 138,7 detik, tapi counter 32-bit sehingga jangan ditampilkan sebagai uptime). `IDLE0`/`IDLE1` masing-masing ≈ 137 juta.

Pengamatan:
- `async_tcp` berstatus `Running`: itu task yang sedang menjawab request, jadi monitor selalu melihat dirinya sendiri sedang jalan. Untuk melihat siapa memakai CPU, andalkan persentase, bukan kolom state.
- `free` milik `async_tcp` turun dari 15940 (probe) ke 12940 (setelah endpoint menyusun JSON). Sisa stack terkecil sepanjang hidup task hanya bisa turun. 12,9 KB masih aman.
- **`loopTask` tidak muncul** karena `main.cpp:146` memanggil `vTaskDelete(NULL)` (task menghapus dirinya sendiri, pola umum untuk proyek AsyncWebServer). Bukan bug.

---

## 7. Frontend Task Monitor

### Versi 1 (tabel minimal, terverifikasi di device)
Keputusan desain:
- **Selisih dihitung di browser, bukan di ESP.** Server tanpa state, aman untuk banyak tab/klien (kalau ESP menghitung dari "snapshot terakhir", dua klien saling mengacaukan interval), dan sesuai pola respons instan tanpa menahan request lintas task.
- Snapshot dicocokkan lewat `xTaskNumber`, bukan indeks atau nama (urutan array berubah, task bisa muncul/hilang).
- `null` (belum ada pembanding) ditampilkan `--`, bukan 0.
- Polling dijadwalkan dengan `setTimeout` **setelah** request selesai (bukan `setInterval`), supaya request tidak menumpuk saat ESP lambat.
- Data ke DOM lewat `textContent`, bukan `innerHTML`.

Hasil di screenshot: Core 0 ≈ 2,1-2,2%, Core 1 ≈ 0,3-0,5%; `IDLE1` 99,5-99,7% dan `IDLE0` 97,8-97,9%; `async_tcp` 1,1%, `wifi` 0,8%, `tiT` 0,6-0,7%.

Kekurangan yang terlihat: sel tabel tanpa padding (kolom "Suspended" menempel ke "22", header "Stack free" terpecah jadi dua kolom), dan task IDLE di puncak tabel terbaca seperti paling boros padahal 99% itu artinya core menganggur.

### Versi 2 (bergaya btop, **disusun tapi belum dikonfirmasi berjalan**)
- Panel `cpu` dengan bar bersegmen per core, panel `proc` dengan mini-bar di kolom CPU.
- Task IDLE dipindah ke bawah dan diredupkan; warna bar hijau/kuning/merah menurut beban (≥ 50 / ≥ 80%); stack `free` < 512 B kuning, < 256 B merah.
- Polling **berhenti saat tab tersembunyi** (`visibilitychange`, snapshot pembanding di-reset saat kembali) dan interval bisa dipilih (1s/2s/5s).
- File baru: `data/assets/task-monitor/task-monitor.css`; `task-monitor.js` dan `task-monitor.html` direvisi.
- Tautan Task Manager diaktifkan di `sidebar.js` (`data-path="/task-monitor.html"` untuk auto-highlight) dan `index.html` (badge "coming soon" dihapus). Penamaan "Task Manager" (menu) vs "Task Monitor" (README) belum diseragamkan.

---

## 8. Catatan Resource ESP

- Beban core 0 saat idle tanpa halaman monitor ≈ 0,44% (probe). Dengan halaman terbuka dan polling 1 detik ≈ 2,1-2,2% (screenshot). Selisih ≈ 1,7 poin, sebagian besar ongkos jaringan (`async_tcp`, `wifi`, `tiT`), bukan pembacaan kernel. Ini perkiraan kasar dari dua pengukuran yang tidak terisolasi.
- Tuas yang benar adalah **mengurangi jumlah request**. Sudah ada di desain v2: pause saat tab tersembunyi dan interval bisa diubah.
- Kandidat berikutnya, urut manfaat: (1) ganti `setCacheControl("no-store")` di `serveStatic("/assets/")` dengan `max-age` setelah tahap development selesai (saat ini semua aset diunduh ulang dari ESP tiap halaman dibuka, kompromi: perlu hard-reload setelah upload filesystem baru); (2) cache snapshot beberapa ratus ms di ESP; (3) biaya tetap opsi trace dan run-time stats (menambah sedikit kerja tiap pergantian task, besarnya tidak diukur).

---

## 9. Belum Terverifikasi & Backlog

**Belum diuji ulang setelah migrasi hybrid** (perilaku lama diuji di konfigurasi Arduino):
- [ ] Provisioning Wi-Fi + reboot (kredensial NVS terbaca, terutama setelah tabel partisi berubah).
- [ ] mDNS `esp32.local`.
- [x] Upload/hapus file di File Manager, Tetris/Snek, high score (termasuk `result.highScore` pada alert rekor Snek: casing POST belum diverifikasi).
- [ ] Nilai default runtime yang bisa berubah di hybrid: frekuensi CPU (IDF biasanya 160 MHz, Arduino 240 MHz), watchdog idle core 1, stack `loopTask`. Cek dengan `grep -nE "DEFAULT_CPU_FREQ|TASK_WDT|ARDUINO_LOOP_STACK|ARDUINO_RUNNING_CORE" sdkconfig.<env>`.

**Perlu dipastikan sebelum merge ke `main`:**
- [x] `partitions.csv` dan opsi trace di `sdkconfig.defaults` ter-commit; uji clone bersih ulang setelah semuanya final (satu kali saja, tiap perubahan konfigurasi memicu build penuh ≈ 3 menit).
- [ ] `sdkconfig.defaults` masih dump penuh (790+ baris), idealnya hanya berisi opsi yang berbeda dari default. Dipangkas di sesi tersendiri.
- [ ] README: langkah `setup.sh` dan catatan partisi
- [x] Ruang firmware setelah tabel partisi baru (sebelumnya 86% dari 1 MB), ukur ulang.

**Task Monitor:**
- [x] Konfirmasi UI v2 dan tautan sidebar/dashboard berjalan.
- [ ] **Panel `mem` (heap)**: heap bebas, minimum sepanjang hidup, ukuran heap (butuh field tambahan di backend), dikerjakan di sesi terpisah.
- [ ] Sort per kolom lewat klik header.
- [x] Seragamkan penamaan "Task Manager" vs "Task Monitor".
- [x] Pindahkan handler server global (`/`, `onNotFound`, `serveStatic`) keluar dari `registerTaskRoutes`.

**Lama yang masih terbuka:** next-piece dan pause/reset Tetris, protected files list, 409 "Directory not empty" di UI, database emulasi LittleFS 1 MB untuk test lokal, `GET /api/storage` vs partisi fisik.

---

## 10. Asumsi Awal yang Ternyata Salah (Catatan Troubleshooting)

- Kode probe memakai `xCoreID` seolah selalu ada (§5): member opsional.
- Dugaan awal bahwa error LittleFS terkait **versi** dependensi: bukan, dependensi sudah terselesaikan, yang salah **nama komponen** (§4.2).
- Saran `-I` di `build_flags` untuk header komponen: tidak akan berlaku untuk kode yang dikompilasi CMake IDF.
- Saran `pio run` dan `pio pkg list` untuk `setup.sh`: pengguna memakai ekstensi VS Code, bukan CLI.
- Langkah "hapus sdkconfig hasil generate" awalnya diajukan tanpa menjelaskan bahwa pengguna sudah mengedit file hasil generate langsung (dua cara konfigurasi yang tidak boleh dicampur, §4.1).
- Dugaan bahwa `LittleFS.begin()` gagal karena format: kode 261 bukan soal format, tapi partisi tidak ditemukan (§4.3).

---

## 11. Pelajaran Umum

- Di hybrid, hal yang implisit di Arduino menjadi eksplisit: `app_main`, tabel partisi, komponen library (LittleFS), dan `sdkconfig`.
- **Nama komponen ESP-IDF = nama folder.** Core Arduino menyambungkan komponen berdasarkan nama persis.
- Periksa file **hasil generate** (`sdkconfig.<env>`), bukan `sdkconfig.defaults`; opsi turunan tidak tertulis kalau induknya mati.
- Untuk konfigurasi wajib, `#error` saat kompilasi lebih baik daripada angka nol diam-diam.
- Hitung selisih data kumulatif di sisi klien; pencocokan lewat id unik, bukan indeks; selisih counter 32-bit selalu unsigned.
- CSS: pakai kelas pengubah untuk varian layout (jangan ubah aturan dasar yang dipakai bersama); atribut canvas = resolusi internal, CSS = ukuran tampilan; jangan indentasi konten `<pre>`.
- Error linker (`undefined reference`) berarti kodenya kompilasi tapi implementasinya tidak ikut terhubung. Cari sumbernya, bukan mengubah kode pemanggil.

---

## Lampiran

### File baru / berubah di sesi ini

```
.
├── CMakeLists.txt               (dibuat otomatis oleh PlatformIO untuk build hybrid)
├── components/esp_littlefs/     (diunduh oleh setup.sh, TIDAK di-commit)
├── partitions.csv               (baru; wajib di-commit)
├── sdkconfig.defaults           (baru; wajib di-commit)
├── sdkconfig.<env>              (hasil generate, TIDAK di-commit)
├── setup.sh                     (baru; wajib di-commit, mode 100755)
├── platformio.ini               (framework = arduino, espidf; board_build.partitions/filesystem)
├── src/
│   ├── main.cpp                 (probe sementara, dibersihkan sebelum commit akhir)
│   ├── TaskManager.h/.cpp       (dirombak)
│   └── routes/TaskRoutes.cpp    (handler /api/tasks)
└── data/
    ├── task-monitor.html        (baru)
    ├── index.html               (tautan Task Monitor, v2)
    └── assets/
        ├── core/sidebar.js      (tautan Task Monitor, v2)
        ├── game/snek.html       (baru)
        ├── game/game-layout.css (.canvas-square, .container-square)
        └── task-monitor/        (baru: task-monitor.js, task-monitor.css)
```

Struktur folder sekarang
```
.
├── assets
│   ├── esp32.txt
│   └── README.md
├── CMakeLists.txt
├── compress_asset.sh
├── data
├── include
│   ├── FileManager.h
│   ├── GameManager.h
│   ├── NetworkManager.h
│   ├── README.md
│   ├── secrets.EXAMPLES.h
│   └── TaskManager.h
├── lib
│   └── README
├── LICENSE
├── logs
│   ├── devlog-1.md
│   ├── devlog-2.md
│   ├── devlog-3.md
│   ├── devlog-4.md
│   ├── devlog-5.md
│   ├── README.md
│   └── user-testing.md
├── partitions.csv
├── platformio.ini
├── README.md
├── sdkconfig.defaults
├── setup.sh
├── src
│   ├── CMakeLists.txt
│   ├── FileManager.cpp
│   ├── GameManager.cpp
│   ├── main.cpp
│   ├── NetworkManager.cpp
│   ├── README.md
│   ├── routes
│   │   ├── FileRoutes.cpp
│   │   ├── FileRoutes.h
│   │   ├── GameRoutes.cpp
│   │   ├── GameRoutes.h
│   │   ├── ResponseHelper.cpp
│   │   ├── ResponseHelper.h
│   │   ├── TaskRoutes.cpp
│   │   ├── TaskRoutes.h
│   │   ├── WifiRoutes.cpp
│   │   └── WifiRoutes.h
│   └── TaskManager.cpp
├── test
│   └── README
└── wasm
```
> [!NOTE]
> File wasm merupakan bundle proyek yang menggunakan wasm, kurang relevan untuk diperlihatkan

Filesystem Image (Struktur folder yang berada di dalam LittleFS (/data))
```
.
├── assets
│   ├── core
│   │   ├── sidebar.css
│   │   ├── sidebar.js
│   │   ├── theme.css
│   │   ├── tui-base.css
│   │   └── webaudio-tinysynth.js.gz
│   ├── dashboard
│   │   ├── dashboard.css
│   │   └── dashboard-mobile.css
│   ├── esp32.txt.gz
│   ├── file-manager
│   │   ├── file-manager.css
│   │   ├── file-manager.js
│   │   ├── icons
│   │   │   ├── delete.svg.gz
│   │   │   ├── mkdir.svg.gz
│   │   │   ├── rename.svg.gz
│   │   │   └── upload.svg.gz
│   │   └── maria-system.txt.gz
│   ├── game
│   │   ├── game-ascii.txt.gz
│   │   ├── game.css
│   │   ├── game-layout.css
│   │   ├── game-shell.js
│   │   ├── gm-left.txt.gz
│   │   ├── gm-right.txt.gz
│   │   ├── snek
│   │   │   ├── snek.mid
│   │   │   ├── snek.svg.gz
│   │   │   └── snek.wasm
│   │   ├── snek.html
│   │   ├── tetris
│   │   │   ├── tetoris.mid
│   │   │   ├── tetris.svg.gz
│   │   │   └── tetris.wasm
│   │   └── tetris.html
│   ├── gz
│   │   ├── dashboard.svg.gz
│   │   ├── game-ascii.txt.gz
│   │   └── maria-wifi.svg.gz
│   ├── icons
│   │   ├── dashboard.svg.gz
│   │   ├── file-manager.svg.gz
│   │   ├── game.svg.gz
│   │   ├── task-manager.svg.gz
│   │   └── wifi.svg.gz
│   ├── ost-snek.opus.gz
│   ├── task-monitor
│   │   ├── task-monitor.css
│   │   └── task-monitor.js
│   └── wifi-setup
│       ├── wifi-setup.css
│       └── wifi-setup.js
├── file-manager.html
├── game.html
├── index.html
├── myminegw.avif
├── README.md
├── task-monitor.html
└── wifi-setup.html
```

### Konfigurasi kunci (`sdkconfig.defaults`, bagian yang diubah di sesi ini)

```
CONFIG_FREERTOS_HZ=1000
CONFIG_AUTOSTART_ARDUINO=y
CONFIG_FREERTOS_USE_TRACE_FACILITY=y
CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS=y
CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y
CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID=y
```

### Referensi

- `setup.sh` menarik `joltwallet/esp_littlefs` tag `v1.16.4`.
- Info hybrid: contoh `Jason2866/Arduino_IDF_LittleFS` dan diskusi `platformio/platform-espressif32` issue #965 (LittleFS di Arduino-as-component perlu ditambahkan sebagai komponen).