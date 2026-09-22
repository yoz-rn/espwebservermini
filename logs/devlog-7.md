# Recap Sesi: TaskLens — Wrapper Task User untuk Task Monitor

**Project:** espwebservermini
**Status akhir sesi:** Backlog dari devlog-6 ("template Task Wrapper agar task RTOS milik user terlihat di dashboard monitor") selesai di sisi backend dan terverifikasi end-to-end di device fisik. Modul baru `TaskRegistry` (dinamai sementara **TaskLens**) dikembangkan di `lib/TaskLens/`, dengan rencana sadar untuk dijadikan library standalone setelah stabil. Integrasi frontend (badge task user) sudah ditulis tapi belum terverifikasi tampil di UI — ditunda sebagai isu kosmetik. Dua bug ditemukan dan diperbaiki selama sesi (bootloop null-pointer, compile error default-argument), plus satu keterbatasan baru ditemukan dan diinvestigasi tuntas (flash size mismatch antar mesin development).

---

## 1. Keputusan Desain: Modul Terpisah, Bukan Bagian dari `TaskManager`

- Registry task user (`TaskRegistry`) dan wrapper pembuatnya (`monitoredTaskCreate()`) sengaja **dipisah dari `TaskManager`**, bukan digabung.
- Alasan utama: aturan thread-safety keduanya berlawanan. `TaskManager::_buf` cuma boleh disentuh dari `async_tcp` (aturan lama, sudah didokumentasikan sejak devlog-6). Registry sebaliknya harus bisa ditulis dari task mana pun, kapan pun — jadi butuh spinlock (`portMUX_TYPE`) sendiri, bukan mengikuti model `TaskManager`.
- Efek samping yang disengaja: `TaskManager` cuma *membaca* registry lewat `TaskRegistry::lookup()`/`reconcile()`; registry sendiri tidak tahu `TaskManager` ada. Arah dependensi satu arah ini yang nanti membuat ekstraksi ke library jadi murah (lihat §6).

---

## 2. `TaskRegistry` — Struktur Inti

- Tabel statis ukuran tetap (`MAX_USER_TASKS = 16`), mengikuti prinsip yang sama seperti `MAX_TASKS` di `TaskManager` — tanpa alokasi dinamis, RAM bisa dihitung di awal.
- API inti: `add(handle, tag)`, `remove(handle)`, `lookup(handle, out, outLen)` — `lookup` **menyalin** tag ke buffer pemanggil, bukan mengembalikan pointer ke entri internal, supaya tidak ada risiko pointer menggantung kalau entri dihapus task lain sementara `toJson()` masih membacanya (pola bug yang sama seperti yang sudah dicurigai soal `pcTaskName` di devlog-6 §4).
- Proteksi lintas-task pakai `portMUX_TYPE` (spinlock), bukan mutex — karena bagian kritisnya sangat singkat (loop 16 elemen + `strlcpy`) dan registry dibaca dari core lain (`async_tcp`).

### `reconcile()` — pembersih entri basi
- Masalah yang diselesaikan: kalau task user memanggil `vTaskDelete(NULL)` secara manual (bukan lewat `return` biasa), kontrol tidak pernah kembali ke titik yang bisa menghapus entrinya dari registry → entri jadi sampah, menunjuk ke handle yang sudah tidak ada.
- Solusi murah: manfaatkan snapshot yang sudah diambil `uxTaskGetSystemState()` di tiap polling `toJson()` sebagai sumber kebenaran task yang benar-benar masih hidup. `TaskRegistry::reconcile(liveTasks, liveCount)` membandingkan tiap entri registry terhadap snapshot itu, menghapus yang tidak ketemu.
- **Bug ditemukan saat implementasi (urutan salah):** sempat dipasang **sebelum** guard `if (count == 0) return false;`. Kalau `uxTaskGetSystemState()` gagal (`count == 0`, sinyal error, bukan "tidak ada task"), `reconcile` akan membaca `liveCount = 0` dan menganggap **semua** entri user mati, menghapus registry yang sebenarnya masih valid. Diperbaiki: `reconcile()` dipindah ke **setelah** guard tersebut.

---

## 3. `monitoredTaskCreate()` — Wrapper Pembuat Task

- Diputuskan pakai **satu fungsi campuran** (bukan overload terpisah untuk pinned/non-pinned), meniru pola ESP-IDF sendiri: `xTaskCreate` adalah pemanggil `xTaskCreatePinnedToCore` dengan `coreID = tskNO_AFFINITY`. Parameter `coreID` di wrapper diberi default value yang sama, jadi kasus "tanpa pin" otomatis tercakup tanpa fungsi kedua.
- **Pola trampolin:** task tidak langsung menjalankan fungsi milik user. `xTaskCreatePinnedToCore` memanggil fungsi perantara (`trampoline()`) yang mendaftar ke `TaskRegistry` dulu (dari dalam task itu sendiri, pakai `xTaskGetCurrentTaskHandle()`), baru memanggil fungsi user. Ini menghilangkan jendela waktu di mana task sudah jalan tapi belum terdaftar.
- **Ownership argumen (`TrampolineArgs`):** dialokasikan di heap (`pvPortMalloc`, konsisten dengan alokator yang sudah dipakai `TaskManager::printTasks()`/`takeSnap()`), karena trampolin berjalan di stack task baru. Kepemilikan pindah ke task baru tepat saat `xTaskCreatePinnedToCore` sukses; kalau gagal, `monitoredTaskCreate()` sendiri yang membebaskannya (mencegah leak di jalur gagal, karena `trampoline()` tidak akan pernah jalan untuk membebaskannya).

---

## 4. Bug Ditemukan Selama Sesi

### 4.1 Compile error — default argument didefinisikan dua kali
```
error: default argument given for parameter 7 of 'static BaseType_t TaskRegistry::monitoredTaskCreate(...)'
```
- Penyebab: saat mengetik ulang `monitoredTaskCreate()` di `.cpp`, default value (`= nullptr`, `= tskNO_AFFINITY`) ikut disalin dari deklarasi header. C++ hanya mengizinkan default argument dideklarasikan sekali (di header), tidak boleh diulang di definisi (`.cpp`).
- Fix: hapus default value di `.cpp`, cukup tipe + nama parameter.

### 4.2 Bootloop `LoadProhibited` — argumen tertukar
```
Guru Meditation Error: Core 0 panic'ed (LoadProhibited)
EXCCAUSE: 0x0000001c   EXCVADDR: 0x00000000
#0 trampoline(void*) at lib/TaskLens/src/TaskRegistry.cpp:29
```
- `EXCVADDR: 0x00000000` (null pointer) + lokasi crash di baris pertama `trampoline()` → `args` yang diterima trampolin bernilai `nullptr`.
- Root cause terkonfirmasi: argumen yang tertukar saat memanggil `xTaskCreatePinnedToCore(...)` di dalam `monitoredTaskCreate()` — `param` (nilai `nullptr` yang dikirim user untuk dummy task) terkirim menggantikan `args` (pointer ke `TrampolineArgs` yang sudah dialokasikan). Diperbaiki dengan mengirim `args` yang benar sebagai `pvParameters`.

---

## 5. Uji End-to-End

- Dua dummy task dipasang di `setup()`: `DummyA` (tanpa self-delete, tanpa pin), `DummyB` (self-delete via `vTaskDelete(NULL)`, dipin ke core 1).
- **Catatan penting yang ditemukan saat menyusun uji:** `loop()` proyek ini isinya cuma `vTaskDelete(NULL)` (menghapus `loopTask` sekali jalan, pola event-driven yang sudah dipakai sejak awal proyek). Ini berarti kode uji tidak bisa ditaruh di `loop()` — harus jadi task FreeRTOS tersendiri (`checkerTask`, dibuat lewat `xTaskCreate` biasa, bukan wrapper, karena dia sendiri tidak perlu dipantau).
- Window waktu pengujian awalnya terlalu pendek (delay 1 detik), sempat menghasilkan false negative (`stillThere = 1`) karena `reconcile()` belum sempat terpicu polling Task Monitor. Diperpanjang jadi 30 detik (`DummyB`) dan 40 detik (`checkerTask`) untuk menghilangkan kemungkinan race sepenuhnya.
- **Hasil akhir: `[test] DummyB masih di registry? 0`** — seluruh rantai (`monitoredTaskCreate` → `trampoline` mendaftar → task mati → `reconcile` membersihkan) terbukti bekerja.
- Dikonfirmasi lewat screenshot Task Monitor: `DummyB` dan `Checker` tampil di daftar task saat masih berjalan, lalu hilang setelah keduanya selesai — memvalidasi juga bahwa `vTaskDelete(NULL)` manual bekerja seperti yang diharapkan.

---

## 6. Integrasi ke `TaskManager::toJson()`

Sisipan di dalam loop task, setelah guard `count == 0` dan pemanggilan `reconcile()`:
```cpp
char tag[TaskRegistry::TAG_LEN];
bool isUser = TaskRegistry::lookup(s.xHandle, tag, sizeof(tag));
t["user"] = isUser;
if (isUser) t["tag"] = tag;
```
- Soal keamanan `tag` (variabel lokal `char[]`) dipakai ArduinoJson v7: dikonfirmasi aman, karena `tag` masih hidup sampai `serializeJson()` selesai di baris terakhir fungsi — beda dari kasus `pcTaskName` yang menunjuk ke TCB task lain (risiko lama yang masih di backlog, lihat §9).
- Field `up` (uptime sejak boot, ditambahkan sesi sebelumnya untuk deteksi reboot di frontend) dikonfirmasi **sudah terpasang** di `toJson()` — item backlog devlog-6 soal ini otomatis selesai.

---

## 7. Frontend: Badge Task User (Belum Terverifikasi)

- `task-monitor.js`: fungsi `nameCell(r)` baru menggantikan `cell(r.name)` polos, menambahkan `<span class="tag-badge">` kalau `r.user && r.tag`. Baris `<tr>` dapat kelas tambahan `user-task` lewat `classList.add` (bukan `className =`, supaya tidak menimpa kelas `idle` yang mungkin sudah ada).
- `task-monitor.css`: `.tm-table tr.user-task { color: var(--accent); }` + styling `.tag-badge` (border kecil, radius, ukuran font mengecil). `color` di level `<tr>` sengaja tidak bentrok dengan warna kondisional yang sudah ada di level `<td>` (`.warn`, `.crit`, `.st-running`, dst.), karena warna langsung di elemen anak selalu menang atas warisan dari induk.
- **Status: belum terverifikasi tampil di browser.** Diagnosis kandidat (belum dikonfirmasi satu per satu): tidak ada task user aktif saat dicek (kode uji dummy sudah dibersihkan duluan dari `main.cpp`), atau Upload Filesystem Image belum dijalankan ulang setelah perubahan `.js`/`.css`, atau cache browser. Diputuskan **ditunda** — dicatat di sini sebagai isu kosmetik untuk debugging lanjutan di sesi berikutnya, bukan diselesaikan saat itu juga.

---

## 8. Rencana Menjadikan TaskLens sebagai Library Standalone

- Keputusan: **kembangkan dulu di dalam proyek (`lib/TaskLens/`), stabilkan lewat pengujian, baru ekstraksi ke repo/library terpisah di akhir** — bukan membangun struktur library penuh (metadata multi-ekosistem, dsb.) sejak awal.
- Aturan yang dipegang selama pengembangan terpadu supaya ekstraksi nanti murah:
  1. Kode inti (`TaskRegistry`) tidak boleh bergantung pada apa pun di luar FreeRTOS murni — tidak ada `#include` ke modul proyek lain (`NetworkManager`, dst.), tidak pakai `Arduino.h`/`String`/`Serial`. Sudah dipatuhi sejak awal.
  2. Modul tetap terkumpul dalam satu folder (`lib/TaskLens/src/`), bukan tersebar seperti struktur `include/`+`src/` proyek utama — supaya bisa diekstraksi nanti lewat `git subtree split` tanpa kehilangan riwayat commit.
  3. Angka konfigurasi (`MAX_USER_TASKS`, `TAG_LEN`) sudah berupa `constexpr`/makro yang bisa diubah tanpa menyentuh isi kode — kandidat jadi `#ifndef` dengan default, supaya proyek lain bisa override lewat `build_flags`.
- **Uji struktural:** dikonfirmasi `lib/TaskLens/src/*.cpp` **berhasil dikompilasi dan ter-link** oleh build hybrid Arduino+ESP-IDF proyek ini tanpa konfigurasi tambahan apa pun — dibuktikan lewat pemanggilan fungsi uji sederhana dari `main.cpp` dan pembacaan `Library Dependency Graph` di log build. Ini menjawab keraguan awal soal kompatibilitas folder `lib/` dengan setup hybrid proyek.
- Untuk versi library publik nanti (belum dikerjakan, dicatat sebagai arah): pemisahan lapisan inti (bebas Arduino) vs adapter (JSON/ArduinoJson, Serial/Arduino, web/ESPAsyncWebServer) sebagai header-only; penggantian `#error` config-check jadi degradasi fitur (supaya build tidak gagal total di lingkungan Arduino IDE polos); metadata multi-ekosistem (`library.json`, `CMakeLists.txt`, `idf_component.yml`).

---

## 9. Keterbatasan Baru Ditemukan: Flash Size Mismatch Antar Mesin Development

- Saat upload dari mesin lain (device fisik sama, proyek sama), PlatformIO menampilkan:
  ```
  Warning! Flash memory size mismatch detected. Expected 4MB, found 2MB!
  ```
- **Investigasi:** `esptool.py flash_id` dijalankan langsung di mesin tersebut (bukan lewat proses upload biasa) → hasil **"Detected flash size: 4MB"**, manufacturer ID `68` (ISSI), device ID `4016` (mengkode kapasitas 2^22 byte = 4 MB tepat). Cocok dengan boot log board yang sama di mesin pertama.
- **Kesimpulan:** chip fisik memang 4 MB — peringatan "found 2MB" adalah **false alarm**, kemungkinan besar dari pembacaan flash ID di tahap awal (ROM bootloader, sebelum stub esptool aktif) yang lebih rentan gangguan komunikasi serial, dibanding pembacaan `flash_id` manual yang memakai stub (komunikasi lebih stabil).
- **Dugaan penyebab gangguan:** kabel USB dikonfirmasi sama antar kedua mesin, jadi kecurigaan bergeser ke sisi host — mesin yang menunjukkan warning berusia ~4 tahun (vs ~2 bulan di mesin lain). Kandidat: keausan fisik port USB, driver USB-to-serial (CP210x/CH340) yang lebih lama, atau manajemen daya USB level OS yang kurang stabil di mesin lama.
- **Belum dilakukan:** cek `dmesg -w` saat upload berlangsung di mesin bermasalah untuk mengonfirmasi pola disconnect/reset di level driver; belum dicoba ganti port USB.
- **Risiko nyata kalau chip benar-benar lebih kecil dari partition table** (dicatat sebagai pengetahuan umum, bukan kasus proyek ini): partition table proyek ini penuh sampai offset `0x400000` (4 MB persis) — partisi `spiffs` sendiri berada di rentang ~2.5–4 MB. Kalau suatu saat memang ada board dengan chip fisik lebih kecil, Upload Filesystem Image akan gagal atau menulis ke alamat yang tidak ada, bukan sekadar peringatan kosmetik.

---

## 10. Backlog Baru

- [ ] Debug kenapa badge task user (`tag-badge`) belum tampil di UI — cek task user aktif, Upload Filesystem Image, cache browser, sesuai urutan diagnosis §7.
- [ ] Ekstraksi `TaskLens` ke library/repo standalone, setelah dianggap cukup stabil dari pemakaian nyata (bukan cuma dummy task).
- [ ] Sparkline riwayat CPU% per task (frontend murni, data `pct` sudah dihitung tiap polling, tinggal disimpan beberapa sampel terakhir).
- [ ] Peringatan visual di Task Monitor saat sebuah task mendekati ambang task watchdog (5 detik, sudah diketahui dari devlog-6 §6A).
- [ ] Field status opsional per task user (`TaskRegistry::setStatus()`), agar task bisa melaporkan sedang mengerjakan apa, bukan cuma nama/tag statis.
- [ ] Investigasi lebih lanjut flash size mismatch: `dmesg -w` saat upload di mesin lama, coba port USB lain.

## Lampiran

**Berkas baru/berubah sesi ini:**
```
lib/TaskLens/
└── src/
    ├── TaskRegistry.h     (baru — add/remove/lookup/reconcile/monitoredTaskCreate)
    └── TaskRegistry.cpp   (baru — implementasi + trampoline)
src/TaskManager.cpp        (reconcile() + field user/tag di toJson())
data/assets/task-monitor/task-monitor.js   (nameCell(), badge task user)
data/assets/task-monitor/task-monitor.css  (.user-task, .tag-badge)
logs/devlog-7.md           (baru)
```