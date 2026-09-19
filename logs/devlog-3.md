# Recap Sesi: Backend Hardening + TUI Frontend

**Project:** espwebservermini
**Status akhir sesi:** Path traversal validation, endpoint rename/mkdir, dan TUI Frontend (Dashboard + File Manager + Sidebar shared component) selesai dan tested end-to-end oleh device fisik. Protokol testing end-user disiapkan untuk sesi berikutnya. Beberapa catatan UI/UX baru masuk backlog.

---

## 1. Path Traversal Validation

- Divalidasi di layer `FileRoutes.cpp` (bukan di `FileManager`), lewat helper `isPathSafe()` dalam anonymous namespace — keputusan sadar menjaga separation-of-concerns: `FileManager` tetap "polos", cuma percaya apa yang dikasih routes.
- Validator menolak string kosong dan keberadaan `".."` di mana pun posisinya, dipasang di titik masuk tiap endpoint (`view`, `create`/upload, `delete`, `list`) tepat setelah `path` diambil dari query param.
- Tested manual lewat `curl` (`?path=/../secret`) → `400 "Invalid path"` terkonfirmasi.

---

## 2. Rename & Mkdir Endpoints

- `FileManager::renamePath()` dan `makeDirectory()` — wrapper tipis di atas `LittleFS.rename()`/`LittleFS.mkdir()`, kategori baru "Restructure Element" di header (bukan Create/Delete, karena sifatnya struktural).
- `PATCH /api/files/rename?path=<old>&newPath=<new>` dan `POST /api/files/mkdir?path=<new-dir>` — pola query param dipertahankan konsisten sama endpoint lama (bukan JSON body).
- **Conflict guard eksplisit diminta user:** rename dan mkdir sama-sama cek `fileExists() || isDirectory()` di target sebelum eksekusi → `409` kalau sudah ada, mencegah overwrite diam-diam dari `LittleFS.rename()` yang behaviornya tidak terdokumentasi jelas untuk kasus itu.
- Urutan registrasi route: keduanya didaftarkan sebelum `/api/files` (List/Delete), mengikuti aturan prefix-match yang sudah dipelajari dari kasus `/api/files/view` di sesi sebelumnya.
- Tested lewat `curl`: happy path, conflict (409), path traversal ditolak, parameter kurang (400) — semua sesuai ekspektasi.

---

## 3. Design System — `assets/core/`

- `theme.css` — palet warna dipilih user dari 5-color swatch (bukan salah satu preset di `color.ini` lama): `#233637` (bg), `#B7EBEC` (teks), `#6E9BA4` (accent/border aktif), `#5F8484` (border), `#86B9BB` (teks muted). Disimpan sebagai CSS custom properties satu sumber kebenaran (`--bg-main`, `--text-main`, `--accent`, dst).
- `tui-base.css` — pattern border + panel-label ala referensi *system24* (Discord theme) dan Spicetify *darkthemer-tui*, diimplementasi CSS murni (bukan adopsi framework WebTUI/TuiCss penuh) demi footprint flash minimal. Label panel pakai `content: attr(data-label)` di `::before`, bukan markup terpisah.
- Font: `Courier New` (font sistem), bukan web font custom (`DM Mono` di referensi asli) — keputusan sadar menghindari beban flash/request tambahan.
- **User mengganti warna tema secara mandiri pasca-sesi ini** (di luar chat) — dicatat sebagai perubahan yang sudah terjadi, isi variabel aktual di `theme.css` saat ini kemungkinan sudah beda dari nilai awal di atas.

---

## 4. File Manager Frontend

Fitur lengkap: list, navigasi folder, preview, upload, delete, rename, mkdir — semua pakai `fetch()` native, tanpa library tambahan (alasan sama: AP mode tidak ada akses CDN eksternal).

- **List & sort:** folder ditempatkan di atas file (`isDir` dibandingkan dulu sebelum `localeCompare` nama).
- **Navigasi:** baris `[..]` di dalam tabel (bukan tombol terpisah, keputusan direvisi dari desain awal) — ditandai `data-nav="true"` supaya tidak pernah ikut ke-`selected` (mencegah delete/rename tidak sengaja kena folder induk).
- **Select vs navigasi folder biasa:** single-click = select (untuk delete/rename), double-click = masuk folder. Diputuskan setelah user sadar tester cuma dirinya sendiri, jadi behavior yang lebih dekat ke konvensi file manager OS (bukan single-click auto-navigasi) bisa diterima.
- **Preview:** deteksi tipe by ekstensi (regex whitelist), gambar lewat `<img>` langsung (browser yang fetch), teks lewat `fetch()` + `<pre>`. **Known gap:** `.avif` belum bisa dipreview meski sudah ditambahkan ke regex whitelist — root cause belum diinvestigasi lebih lanjut, backlog untuk sesi depan.
- **Upload:** validasi ukuran client-side (samakan manual dengan `MAX_UPLOAD_SIZE` backend, tidak ada mekanisme sync otomatis), conflict resolution 3 arah (`overwrite`/`duplicate`/`cancel`) lewat `prompt()` native + parsing huruf pertama (`o`/`d`/`c`) — dipilih user dibanding `confirm()` chained maupun UI custom, demi tetap "native dialog" tapi tanpa risiko typo penuh.
- **Delete & Rename digeneralisasi ke folder** setelah awalnya cuma scope file: backend sudah generic dari awal (rename tidak peduli tipe, delete sudah cek `isDirectoryEmpty`), tinggal soal *bagaimana folder bisa ke-`select`* tanpa memicu navigasi — diselesaikan lewat pemisahan event `click` (select) vs `dblclick` (navigasi).
- **Busy state blocking:** `isBusy` flag + `body.busy` class (opacity + `pointer-events: none`) dipasang di semua 6 aksi, mencegah user spam-klik membebani device selagi request masih berjalan.
- **Cache bug:** `fetch()` ke `GET /api/files` dan preview teks kena heuristic browser caching (server tidak kirim header `Cache-Control` eksplisit) — hasilnya listing/preview tidak update walau `loadFiles()` sudah dipanggil ulang, harus hard-reload. **Fix:** `cache: "no-store"` ditambahkan ke kedua `fetch()` call. Catatan: fix ini tidak berlaku untuk `<img>` (tidak lewat `fetch()`), jadi kalau kasus serupa muncul di preview gambar, perlu cache-busting query string terpisah.

---

## 5. Dashboard Restyle & Ikon Fitur

- 4 tombol fitur besar (wifi, file manager aktif; task manager, game disabled dengan badge "coming soon") — ditulis sebagai `<a>` untuk yang aktif dan `<span>` non-interaktif untuk yang disabled (bukan `<a href="#">`/`button disabled`), supaya elemen disabled memang tidak bisa difokus/diklik dari strukturnya sendiri.
- **Ikon SVG monokrom via `mask-image` + `currentColor`**, bukan `<img>` biasa — warna ikon jadi terkontrol penuh lewat CSS, mengikuti warna teks tombol otomatis (termasuk saat hover, tanpa rule tambahan). Prasyarat: source SVG harus single-shape/monokrom.
- **Bug ditemukan & fixed — custom property `url()` resolve path relatif ke file CSS, bukan ke HTML:** `--icon-url: url('assets/icons/x.svg')` ditulis di `index.html` tapi dipakai di `dashboard.css` (`assets/dashboard/`), jadi path relatif ter-resolve jadi `assets/dashboard/assets/icons/x.svg` (salah). **Fix:** pakai absolute path (`/assets/icons/x.svg`). Dicatat sebagai gotcha umum untuk pola serupa ke depan.
- **Kompresi gzip ikon:** disimpan di folder yang sama persis dengan file asli (`assets/icons/x.svg` + `assets/icons/x.svg.gz`), bukan subfolder `gz/` terpisah — supaya mekanisme otomatis `server.serveStatic("/", LittleFS, "/")` bisa serve versi gzip tanpa kode tambahan. Ini sekaligus menghindari masalah yang sama seperti 3 file lama di `assets/gz/` yang membuat `resolveViewPath()` gagal menemukan pasangannya.
- `index.html` lama sempat gagal total tampil bertema karena tidak pernah punya `<link>` ke `theme.css`/`tui-base.css`/`dashboard.css` — cuma warisan `<style>` inline dari versi awal proyek (font Arial, dst). Diperbaiki: `<style>` inline dihapus total (berisiko override), 3 `<link>` ditambahkan, ASCII art dibungkus `.tui-panel` konsisten dengan halaman lain.

---

## 6. Sidebar — Shared Component

- Ditemukan `common.js`/`style.css` (aset lama proyek) ternyata **0 byte, tidak pernah dipakai** — diputuskan tidak perlu diselamatkan/direuse, sidebar ditulis sebagai file baru bersih (`assets/core/sidebar.css`, `sidebar.js`) ditempel manual ke tiap HTML (3 halaman: dashboard, wifi-setup, file-manager) — bukan lewat component loader JS, demi menghindari flash-of-missing-content dan kompleksitas fetch tambahan untuk skala sekecil ini.
- **Fixed di desktop, collapsible (off-canvas) di mobile** murni lewat `@media (max-width: 768px)` — tidak butuh deteksi device via JS.
- Highlight halaman aktif dihitung otomatis (`window.location.pathname` dibanding `href` tiap link), bukan ditulis manual per file — mengurangi risiko lupa update saat sidebar di-copy ke halaman baru.
- **Bug ditemukan & fixed — label panel hilang akibat `overflow`:** `overflow-y: auto`/`overflow: hidden` yang dipasang langsung di elemen `.tui-panel` ikut memotong pseudo-element label (`::before`, posisinya `top: -0.75em`, di luar border box). **Fix:** prinsip baru ditetapkan — `.tui-panel` tidak boleh punya `overflow` apa pun; konten yang butuh scroll/clip dibungkus elemen anak terpisah (`.fm-table-scroll`, `.fm-ascii-scroll`), `overflow` dipasang di situ.
- **Bug ditemukan & fixed — panel grid child yang salah nesting:** panel `system` (ASCII art) sempat berada **di luar** `.file-manager-grid` (mismatch tag penutup `</div>`), sehingga `grid-column`/`grid-row` yang di-set di CSS tidak berlaku sama sekali (`grid-*` cuma efektif untuk direct child grid container). Fix: perbaikan nesting HTML, `grid-template-rows: auto 1fr` + `min-height: 100vh` di grid container untuk membuat panel system mengisi sisa tinggi layar.

---

## 7. Protokol Testing End-User

Disiapkan file checklist testing markdown terpisah untuk dijalankan oleh penguji eksternal (bukan developer), mencakup: koneksi awal & provisioning WiFi, jelajah bebas File Manager tanpa instruksi detail, skenario "sengaja bikin rusak" (upload besar, spam klik, delete folder berisi, path traversal manual), kondisi jaringan tidak ideal, dan form pelaporan temuan terstruktur. Belum dieksekusi — direncanakan dijalankan orang lain di sesi mendatang supaya perilaku pemakaian tidak bisa ditebak developer sendiri.

---

## 8. Catatan Perubahan Mandiri dari User (di luar sesi chat ini)

Dua perubahan berikut dilakukan user sendiri, tidak lewat pembahasan chat, dicatat sebagai referensi kondisi kode saat ini:

1. Tombol aksi file (rename/delete/mkdir) diganti dari emoji (`✏️`/`🗑️`/`📁`) menjadi ikon vector — kemungkinan pakai teknik `mask-image` yang sama seperti ikon dashboard, belum diverifikasi lewat kode aslinya.
2. Warna tema (`theme.css`) diganti dari palet 5-warna yang disepakati di sesi ini — nilai baru belum diketahui/dicatat.

---

## 9. Backlog Baru (Catatan UI/UX dari User, Sesi Berikutnya)

- [x] **Panel `path` full-width** — memanjang ke kanan menghabiskan sisa layar; 2 tombol aksi (upload, mkdir) dipindah ke kiri sebelum elemen path, bukan di kanan seperti sekarang.
- [x] **Align ASCII art panel `system` ke top-left** — supaya tidak "hilang" (ter-scroll keluar area terlihat) saat user sedang melihat teks panjang di panel preview yang sejajar.
- [x] **`GET /api/storage`** — tampilkan sisa kapasitas LittleFS di UI. Backend logic-nya sudah ada (dipakai di validasi Create), tinggal expose sebagai endpoint tersendiri + elemen UI penampil. (Item ini sudah muncul di backlog sesi sebelumnya, belum dikerjakan.)
- [x] **Align gambar di panel preview jadi center vertikal**, bukan top-middle seperti sekarang.

---

## 10. Backlog Lama yang Masih Terbuka

- [x] Preview `.avif` masih belum berhasil ditampilkan meski ekstensi sudah masuk whitelist — root cause belum diinvestigasi.
- [ ] Pesan `409 "Directory not empty"` saat gagal delete folder belum ditampilkan spesifik di UI (masih pesan generik "gagal menghapus") — user memutuskan ditunda sampai proyek berkembang lebih jauh.
- [ ] Protected files list — blocklist path kritikal.
- [ ] Concurrency fix untuk Create (state upload per-request, bukan member variable tunggal) — belum mendesak karena upload masih single-file.
- [x] Restyle responsive design menyeluruh (breakpoint sudah ada parsial di file-manager grid & sidebar, belum diverifikasi di semua halaman/ukuran layar).
- [ ] Task Manager — sisa satu fitur proyeksi utama proyek, belum dimulai sama sekali.

## Lampiran

**Struktur `data/` saat ini (per screenshot sesi ini):**

```
data/
├── assets/
│   ├── core/
│   │   ├── theme.css
│   │   ├── tui-base.css
│   │   └── sidebar.css / sidebar.js
│   ├── dashboard/
│   │   └── dashboard.css
│   ├── file-manager/
│   │   ├── file-manager.css
│   │   └── file-manager.js
│   ├── icons/
│   │   └── *.svg (+ *.svg.gz berdampingan)
│   └── gz/            (legacy, belum dibereskan — lihat devlog-2)
├── index.html
├── file-manager.html
├── wifi-setup.html
└── README.md
```

**Catatan:** `common.js` dan `style.css` (lokasi lama, root `assets/`) dikonfirmasi 0 byte dan tidak direferensikan di manapun — kandidat aman untuk dihapus kapan saja, tidak mendesak.