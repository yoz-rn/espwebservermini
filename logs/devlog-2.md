# Recap Sesi: File Manager (LittleFS CRUD)

**Project:** espwebservermini
**Status akhir sesi:** CRUD File Manager selesai secara fungsional — List, View, Create (dengan conflict resolution), Delete semua tested & working. Eksperimen WebAssembly frontend ditunda (bug belum terselesaikan). Beberapa item backend tersisa untuk sesi berikutnya.

---

## 1. List File — `GET /api/files`

- `FileManager::listDirectory(path, JsonArray& out)` mengisi `JsonArray` langsung lewat referensi — dipilih dibanding return `String` (JSON) atau `std::vector<FileEntry>`, demi hemat resource ESP32 (satu kali alokasi lewat memory pool ArduinoJson, tanpa struktur perantara). Trade-off: `FileManager` jadi "tahu" tipe `JsonArray`, sedikit melenceng dari separation-of-concerns murni (manager vs routes) — disepakati sebagai kompromi wajar untuk embedded.
- **Bug 1 — logic terbalik:** `if (!root || root.isDirectory()) return false;` — tanda `!` di depan `isDirectory()` hilang, jadi validasi kebalik total (direktori valid malah ditolak). Ketemu lewat `Serial.printf` debug manual di dalam function.
- **Bug 2 — field `path` salah isi:** `obj["path"] = path` (path direktori, bukan `fullPath` per-file) — nulis path yang sama di semua entry, padahal `fullPath` sudah dibangun tapi tidak dipakai.
- **Insiden debugging urutan:** trailing slash awalnya dicurigai jadi biang error 404, ternyata false lead — root cause sebenarnya bug 1 di atas. Baru ketemu setelah systematic elimination lewat Serial log per-langkah.

---

## 2. View File — `GET /api/files/view`

- `FileManager::resolveViewPath()` — coba path asli dulu, fallback ke `path + ".gz"` kalau tidak ketemu. Desain ini eksplisit **tidak** menangani kasus user minta `.gz` secara langsung sebagai kasus khusus (dianggap di luar scope pemakaian normal).
- MIME type: **table-driven lookup** (`array of struct { ext, type }` + loop), bukan if-else chain — alasan utama maintainability (nambah tipe file = nambah satu baris data, bukan cabang logic baru), bukan performa (keduanya O(n), beda nyaris nol untuk list sekecil ini).
- **Keputusan gzip:** file `.gz` diserve **mentah** dengan header `Content-Encoding: gzip` manual (browser yang decompress) — bukan decompress di ESP32. Alasan: proses decompress di device jauh lebih mahal (CPU + RAM buffer) dibanding overhead decode gzip native di browser (nyaris nol di kedua sisi kalau pakai header). Trade-off: endpoint ini jadi "browser-only", `curl` polos tanpa `--compressed` akan dapat raw bytes.
- **Bug — signature mismatch:** `ResponseHelper.h` mendeklarasikan `getMimeType(const String* path)` (pointer), sementara `.cpp` mengimplementasikan `const String&` (reference) — typo manual saat transfer kode dari chat ke editor, ketauan dari compiler error langsung.
- **Insiden route collision:** `ESPAsyncWebServer` mencocokkan URI secara prefix (`url == uri` ATAU `url.startsWith(uri + "/")`), bukan exact-match murni. `/api/files/view` "kena tangkap" oleh handler `/api/files` (List) karena didaftarkan lebih dulu. **Fix:** urutan registrasi dibalik — path lebih spesifik (`/view`) harus didaftarkan sebelum prefix-nya (`/api/files`). Ini jadi aturan wajib untuk semua endpoint baru di bawah `/api/files/*`.
- Tested end-to-end: file teks biasa, file `.gz` dengan browser auto-decompress (DevTools Network tab dikonfirmasi header `Content-Encoding: gzip` ada), dan gambar `.webp` 1400×1400 ter-render sempurna.

---

## 3. Create/Update — `POST /api/files`

- Keputusan awal: cap ukuran upload sempat dipertimbangkan 100 KB flat, direvisi jadi **25 KB** setelah klarifikasi bahwa fitur ini untuk *penggantian aset pasca-deploy* (interface utama tetap lewat filesystem image), bukan jalur upload umum — 25 KB dipilih dengan acuan konkret (gambar `.webp` 1400×1400 project ini = ~21 KB).
- Safety margin storage: **15% dari total kapasitas LittleFS** disisihkan (di luar yang sudah terpakai), dihitung dinamis tiap request (bukan hardcode), untuk jaga-jaga wear-leveling LittleFS.
- **Conflict resolution ala file explorer:** cek `fileExists()` dulu sebelum mulai nulis apa pun. Kalau bentrok dan client belum kasih keputusan → `409` dengan `{"conflict": true}`. Client kirim ulang dengan `?onConflict=overwrite` atau `?onConflict=duplicate`. Duplicate pakai konvensi Ubuntu-style (`bar.txt` → `bar-1.txt`, bukan Windows-style `bar (1).txt`) lewat `FileManager::generateDuplicateName()`.
- **Insiden terbesar sesi ini — kesalahan asumsi urutan callback `ESPAsyncWebServer`:**
  - Asumsi awal: `onRequest` (handler pertama) dipanggil **sebelum** body mulai diterima → didesain untuk validasi di situ (ukuran, free space, conflict), baru `onBody` menulis chunk.
  - Kenyataan (dikonfirmasi dari dokumentasi resmi library): untuk request dengan body, `onRequest`/`handleRequest` baru dipanggil **setelah** seluruh body selesai diterima lewat `onBody`.
  - Akibat: semua chunk data dibuang diam-diam di `onBody` (karena `isWriteReady()` masih `false`, `beginWrite()` belum sempat dipanggil), file akhirnya dibuat tapi **kosong**. Tidak ada satupun handler yang memanggil `request->send()`, request nge-hang, library melempar fallback `"Handler did not handle the request"`.
  - **Fix:** semua validasi (path, ukuran, free space, conflict resolution, `beginWrite()`) dipindah ke dalam `onBody`, dicek khusus saat `index == 0`. `onRequest` disederhanakan jadi jaring pengaman untuk kasus body kosong (`contentLength() == 0`) saja.
  - **Gotcha tambahan yang ketauan belakangan:** client (`curl`) wajib set header `Content-Type` eksplisit (mis. `application/octet-stream`). Tanpa itu, `curl --data-binary` default ke `application/x-www-form-urlencoded`, yang membuat `ESPAsyncWebServer` mem-parse body sebagai form field alih-alih memanggil `onBody` — request tidak pernah sampai ke handler sama sekali. Ini juga catatan penting untuk nanti: kode frontend/JS wajib set header ini eksplisit di setiap panggilan Create.
- State upload (`_writeFile`, `_writePath`, `_writeReady`) disimpan sebagai member variable tunggal di `FileManager` — sengaja didesain untuk **satu upload aktif dalam satu waktu**, bukan concurrent-safe. Diterima sebagai trade-off sadar (lihat item roadmap di bawah).

---

## 4. Delete — `HTTP DELETE /api/files`

- Semantik ala `rm`/`rmdir` Unix, bukan `rm -rf`: file langsung dihapus, direktori hanya boleh dihapus **kalau kosong** (`409` kalau berisi), `404` kalau path tidak ada sama sekali.
- `FileManager::isDirectory()`, `isDirectoryEmpty()`, `deleteFile()`, `deleteDirectory()` — masing-masing satu tanggung jawab, konsisten dengan pola function kecil yang sudah dipakai di modul lain.
- Method `HTTP_DELETE` beda dari `GET`/`POST` di URI yang sama, jadi endpoint ini otomatis tidak kena masalah collision urutan registrasi seperti List/View.
- Tested: hapus file ada, hapus file tidak ada (404), hapus direktori berisi (409, dikonfirmasi tidak ada file yang ikut terhapus), tanpa parameter path (400).

---

## 5. Eksperimen WebAssembly Frontend (ditunda)

Sempat dieksplorasi sebagai persiapan milestone berikutnya (frontend berbasis WASM), tapi dihentikan sementara setelah debugging tidak membuahkan hasil.

- Setup awal: `wasm32-unknown-unknown` + `wasm-pack`, project `wasm-bindgen` minimal (`greet()` return string) — berhasil compile, ukuran `.wasm` ~11.7 KB (murni overhead runtime `wasm-bindgen`, sebelum logic apa pun ditambahkan).
- **Bug tidak terselesaikan:** `Uncaught RangeError: failed to grow table` di `__wbindgen_init_externref_table`, muncul di setiap percobaan load. Kandidat penyebab yang **sudah dieliminasi satu-satu**:
  - `wasm-opt` (Binaryen v108 via APT) — dihapus dari pipeline, error tetap sama.
  - Build kotor/cache lama — `rm -rf target pkg` + rebuild bersih, error tetap sama.
  - Kompatibilitas browser — dites di Chrome (modern, harusnya support reference types), error tetap sama.
  - Version mismatch `wasm-bindgen` crate vs CLI — dikonfirmasi **match persis** (`0.2.128` keduanya), error tetap sama.
  - File stale/tidak sinkron antara build dan yang diserve browser — ukuran file di disk vs yang diterima browser dikonfirmasi sama (~12 KB).
- Root cause belum ketemu setelah lima kandidat gugur. Keputusan: **ditunda**, lanjut fokus ke CRUD dulu. Kemungkinan besar terkait `rustc --version` dan flag `reference-types` di codegen — belum diselidiki.
- Rencana pemakaian (kalau dilanjut nanti): port logic `rmatrix` (matrix rain effect, sudah ada source Rust-nya dari project CLI terpisah) ke Canvas API via `web-sys`, sebagai elemen estetika dashboard — bukan asset gambar statis.

---

## 6. Keputusan Desain Frontend (belum diimplementasi, disepakati untuk sesi berikutnya)

- Upload multi-file: dipilih **multi-select tapi upload serial** (satu `POST` tuntas, baru lanjut file berikutnya) — bukan paralel. Alasan ganda: menghindari perlu redesain state upload `FileManager` untuk concurrency, dan realistis secara hardware (flash ESP32 cuma satu jalur SPI, upload paralel tidak benar-benar lebih cepat).
- Behavior saat konflik nama di tengah batch upload: **auto-skip** — file yang bentrok dilewati otomatis tanpa dialog, ringkasan hasil ditampilkan di akhir batch ("3 berhasil, 1 dilewati, 1 gagal"). Backend sudah cukup mendukung ini tanpa perubahan (409 + `conflict: true` sudah tersedia).
- Gunakan tema TUI, refs: https://webtui.ironclad.sh/ https://github.com/vinibiavatti1/TuiCss https://www.npmjs.com/package/@webtui/css
---

## 7. Arsitektur Akhir File Manager (Referensi)

**Backend:**
- `FileManager.h/.cpp` — logic murni LittleFS: list, exists, resolve view path (+ gzip fallback), generate duplicate name, write (begin/chunk/end), delete (file/directory)
- `routes/FileRoutes.h/.cpp` — `GET /api/files` (list), `GET /api/files/view` (baca), `POST /api/files` (create/update), `DELETE /api/files` (hapus)
- `ResponseHelper.h/.cpp` — `buildStatusJson()`, `getMimeType()` (table-driven)

**Pola desain kunci yang dipelajari sesi ini:**
- Urutan registrasi route penting saat ada prefix overlap (`/api/files` vs `/api/files/view`) — spesifik duluan.
- Untuk `ESPAsyncWebServer`, `onRequest` baru jalan **setelah** body selesai kalau request punya body — semua validasi upload harus di `onBody`, dicek di `index == 0`.
- Client wajib set `Content-Type` eksplisit untuk raw body upload, atau library salah parsing jadi form data.
- "Cek dulu, tulis belakangan" (conflict check sebelum `beginWrite()`) menghindari siklus tulis flash sia-sia untuk request yang berpotensi dibatalkan.
- State yang diasumsikan single-instance (`_writeFile` di `FileManager`) adalah trade-off sadar, bukan oversight — tapi jadi prasyarat yang harus direvisit begitu ada kebutuhan concurrent.

---

## 8. Daftar Saran Pengembangan Backend (untuk sesi berikutnya)

Diurutkan berdasarkan prioritas yang disepakati:

- [x] **Path traversal validation** — tolak `path` yang mengandung `..` di semua endpoint. Prioritas tertinggi: device exposed ke siapa pun yang connect ke AP WiFi, belum ada validasi format path sama sekali.
- [x] **Fix regresi `generateDuplicateName`** — pemisah ekstensi pakai `path.indexOf('.')` (titik pertama), seharusnya `lastIndexOf('.')` (titik terakhir). Salah untuk nama file multi-titik (`common.min.js`, `8bit-image.webp`). Sempat benar di versi sebelumnya, regresi saat file di-upload ulang manual.
- [ ] **Rename/Move** — `LittleFS.rename(oldPath, newPath)` sudah tersedia bawaan, tinggal wrap jadi endpoint (`PUT`/`PATCH /api/files/rename`). Saat ini rename cuma bisa lewat download-manual → re-upload → delete lama.
- [ ] **`mkdir`** — `LittleFS.mkdir(path)` bawaan, belum ada endpoint. LittleFS ESP32 tidak otomatis membuat folder dari penulisan file ke path baru, jadi tanpa ini bikin direktori kosong tidak straightforward (kerasa pas testing Delete).
- [ ] **Concurrency fix untuk Create** — state upload (`_writeFile`/`_writeReady`) di `FileManager` cuma aman untuk satu upload aktif. Perlu migrasi ke state per-request (`request->_tempObject`) **sebelum** frontend multi-upload beneran dipakai — walau keputusan sesi ini (upload serial, bukan paralel) membuat ini tidak mendesak untuk saat ini.
- [ ] **Protected files list** — blocklist path kritikal (`index.html`, `wifi-setup.html`) supaya tidak ke-delete lewat API secara tidak sengaja.
- [x] Endpoint pendukung UX (opsional, murah ditambah): `GET /api/storage` (total/used/free, logic sudah ada dari Create, tinggal di-expose).

## Lampiran

### Struktur kode per devlog (lokal, sudah termasuk perubahan sesi ini)

```
.
├── assets
│   ├── esp32.txt
│   ├── MYMINEGUWEH.txt
│   ├── myminegw.avif
│   ├── NOKIA.mp3
│   ├── nokia.opus
│   ├── README.md
│   └── taiga.txt
├── compress_asset.sh
├── data
│   ├── assets
│   │   ├── common.js
│   │   ├── gz
│   │   │   ├── esp32.txt.gz
│   │   │   ├── maria.txt.gz
│   │   │   ├── MYMINEGUWEH.txt.gz
│   │   │   ├── myminegw.avif
│   │   │   └── taiga.txt.gz
│   │   └── style.css
│   ├── dummy
│   │   └── apawe
│   ├── index.html
│   ├── README.md
│   └── wifi-setup.html
├── git_sync.sh
├── include
│   ├── FileManager.h
│   ├── NetworkManager.h
│   ├── README.md
│   ├── secrets.EXAMPLES.h
│   ├── secrets.h
│   └── TaskManager.h
├── lib
│   └── README
├── LICENSE
├── logs
│   ├── devlog-1.md
│   ├── devlog-2.md
│   └── README.md
├── platformio.ini
├── README.md
├── src
│   ├── FileManager.cpp
│   ├── main.cpp
│   ├── NetworkManager.cpp
│   ├── README.md
│   ├── routes
│   │   ├── FileRoutes.cpp
│   │   ├── FileRoutes.h
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
    ├── README
    └── rmatrix-wasm
        ├── Cargo.lock
        ├── Cargo.toml
        ├── pkg
        │   ├── package.json
        │   ├── rmatrix_wasm_bg.wasm
        │   ├── rmatrix_wasm_bg.wasm.d.ts
        │   ├── rmatrix_wasm.d.ts
        │   └── rmatrix_wasm.js
        ├── src
        │   └── lib.rs
        └── test.html
```

**Catatan:** struktur folder `gz/` di dalam `data/assets/` belum final — rencana ke depan setiap fitur akan punya folder HTML + asset sendiri-sendiri. `FileManager::resolveViewPath()` saat ini masih mengasumsikan file `.gz` ada di folder yang sama dengan versi non-gzip-nya (belum di-update untuk struktur `gz/` yang baru), jadi 3 file yang sudah dipindah ke sana untuk sementara tidak bisa diakses lewat View — ini diketahui, bukan bug baru, dan sengaja ditunda sampai restrukturisasi folder final.

### Temuan untuk next milestone

- Bug WASM `failed to grow table` belum terselesaikan — lihat bagian 5.
- Struktur folder `data/assets/` masih dalam transisi menuju pemisahan per-fitur — `resolveViewPath()` perlu di-update begitu strukturnya final.
- Gunakan `#define` atau semacamnya untuk backend guna mencegah typo pada string
- Jika terdapat routine yang berpotensi reusable, jadikan fungsi sendiri seperti `buildJsonStatus` pada file [`ResponseHelper.h`](/src/routes/ResponseHelper.h)