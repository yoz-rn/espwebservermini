# Recap Sesi: Milestone WASM Minigame — Snek & Tetris

**Project:** espwebservermini
**Status akhir sesi:** Arsitektur WASM freestanding-C untuk minigame selesai dirancang & tervalidasi end-to-end (browser test + device fisik) untuk Tetris. Snek sudah punya backend WASM lengkap (termasuk restart) tapi frontend (`snek.html`) belum diintegrasikan. Beberapa bug ditemukan selama sesi, sebagian besar sudah diperbaiki; 2 known issue dicatat belum di-fix di akhir sesi.

---

## 1. Arsitektur WASM — Pivot dari Rencana Awal

- Proyek WASM sebelumnya (`wasm/rmatrix-wasm`, Rust + `wasm-pack`) **resmi dipensiunkan**.
- Rencana awal (JS baca linear memory WASM langsung sebagai grid buffer) **diganti total** setelah review referensi [`tsoding/snake-c-wasm`](https://github.com/tsoding/snake-c-wasm): pola **`platform_*` callback** — WASM manggil balik fungsi JS yang di-import (`platform_fill_rect(x,y,w,h,color)`) buat gambar, bukan JS yang baca memory. Alasan: matching prinsip separation-of-concerns yang sudah dipegang di proyek ini (`FileManager` "polos", validasi di caller → di sini `game.c` "polos", rendering didorong ke platform layer).
- Toolchain: `clang -Os -fno-builtin --target=wasm32 --no-standard-libraries -Wl,--export=<fn> ... -Wl,--no-entry -Wl,--allow-undefined`. Freestanding penuh, tanpa Emscripten (dicoba dulu 1 referensi Tetris berbasis Emscripten — [`olzhasar/sdl-tetris`] — tapi outputnya jauh lebih berat, gak cocok buat flash ESP32 yang terbatas).
- Kontrak export seragam di kedua game (biar `game-shell.js` bisa dipakai bareng): `game_seed(u32)`, `game_init()`, `game_update(f32 dt)`, `game_render()`, `game_keydown(i32 key)`, `game_get_score() -> u32`, `game_is_over() -> i32`. Warna game **hardcode di C** (keputusan sadar — opsi dynamic-theme-inject ke WASM ditolak demi kesederhanaan).

---

## 2. `snek.c`

- Grid 20×20, badan ular pakai **ring buffer** (bukan array di-shift manual tiap tick) — `O(1)` per gerakan.
- RNG LCG manual, direction-queue buat input (nolak reverse-ke-badan-sendiri).
- `STEP_INTERVAL` dikonsumsi via `step_cooldown` yang dikurangi `dt` tiap frame (bukan `setTimeout` fixed-interval).
- **Restart ditambahkan** (key `4`, cuma aktif kalau `game_over`) — tervalidasi lewat headless test (mati → restart → score reset → update jalan normal lagi tanpa crash).
- **Status: backend selesai & tertest. Frontend (`snek.html`) belum ada/belum diintegrasikan.**

---

## 3. `tetris.c` — Port dari Referensi Eksternal

- Logic di-port hampir 1:1 dari [`olzhasar/sdl-tetris`] (source asli 100% bebas dependency SDL di `game.c`-nya — cuma butuh 2 penggantian buat freestanding).
- 10×20 grid, 7-bag Fisher-Yates shuffle, scoring/collision/rotasi/line-clear **tidak diubah** dari source asli.
- **Bug freestanding-C ditemukan saat compile:** compiler nge-generate panggilan **implisit** ke `memset`/`memcpy` (buat zero-init & copy struct besar) walau kode gak pernah manggil keduanya secara eksplisit. Fix: definisikan `memset`/`memcpy` sebagai fungsi **global** (bukan `static`) pakai `__SIZE_TYPE__`, biar linker nyambungin panggilan implisit itu. **Dicatat sebagai gotcha umum buat semua game freestanding-C ke depan, bukan cuma tetris.**
- Key mapping: `0=left, 1=right, 2=rotate, 3=soft_drop, 4=hard_drop, 5=start/restart`. Soft-drop didesain one-shot (bukan tracking key-held) karena efeknya (`fall_period_ms` turun ke 30ms) persisten sampai move/rotate/lock berikutnya reset — jadi gak butuh event `keyup`.
- **Ditunda ke devlog/sesi depan:** next-piece preview (butuh restrukturisasi `shape_bag` jadi two-bag lookahead, bukan cuma nambah 1 getter) dan tombol Pause/Reset (belum ada logic sama sekali, masih stub UI).

---

## 4. `game-shell.js` — Shared Loader

- 3 bagian: `GameAudio` (MIDI via `webaudio-tinysynth.js`), `GameAPI` (highscore GET/POST), `loadGame()` (WASM loader + render loop).
- **Keputusan desain:** semua asset game (`.wasm`, `.mid`) di-serve **langsung lewat `serveStatic`** yang sudah ada, bukan lewat endpoint API baru (draft awal frontend sempat pakai endpoint `/api/game/asset?game=...&file=...` yang gak pernah dibuat — dikoreksi ke `fetch()` relatif biasa).
- `GameAPI.submitScore()` kirim **query param murni, tanpa body** — draft awal pakai `application/x-www-form-urlencoded` body, yang ternyata gak cocok sama `GameRoutes.cpp` (baca `request->getParam()` default cuma nyari query string, bukan POST body) — kalau gak dikoreksi, submit skor bakal selalu gagal diam-diam (400).

### Bug ditemukan & diperbaiki selama sesi:
1. **Nama fungsi mismatch** — draft awal manggil `wasmInstance.set_input()`, padahal export asli `game_keydown()`.
2. **Start Game gak bisa dipencet ulang setelah game over** — `wasmInstance` gak pernah di-reset ke `null`, guard anti-double-start ikut nge-block restart. Fix: reset `wasmInstance = null` + re-enable tombol di `onGameOver()`.
3. **Loop render dobel** — `loadGame()` gak punya mekanisme stop; manggilnya 2x (misal abis fix bug #2) bikin 2 `requestAnimationFrame` loop jalan bareng nge-render ke canvas yang sama. Fix: generation token (`_gameLoopToken`) — loop lama otomatis berhenti begitu ada `loadGame()` baru.
4. **Panel canvas kekecilan** — root cause **sama persis** dengan bug `min-height` vs `height` yang pernah ditemukan di File Manager (devlog-3 §6): `.game-grid` pakai `min-height`, bukan `height`, jadi row `1fr` gak pernah dapat angka pasti buat di-stretch. **Pola bug ini sekarang muncul 2x di proyek — kandidat kuat buat dicatat sebagai prinsip CSS umum, bukan cuma per-kasus.**
5. **Resolusi canvas gak sesuai area gambar WASM** — tag `<canvas width="300" height="600">` padahal C cuma gambar ke 200×400 (`GRID_WIDTH×CELL_SIZE`, `GRID_HEIGHT×CELL_SIZE`). Fix: samakan atribut canvas + tambah `image-rendering: pixelated` biar upscale CSS tetap tajam (gak blur).
6. **Sidebar di-inject 2x** — `sidebar.js` direvisi jadi shared component yang inject `<nav>` via `insertAdjacentHTML` (auto-highlight halaman aktif + auto-buka dropdown Games), tapi `tetris.html` masih punya `<nav id="sidebar">` hardcode sendiri → duplikat ID. Fix: hapus markup hardcode dari `tetris.html`. **Perlu dibersihin juga di `file-manager.html`/`index.html`/`wifi-setup.html` kalau masih pakai markup lama.**
7. **Path Snek/Tetris salah di `sidebar.js`** — `href`/`data-path` masih `/snek.html`/`/tetris.html` (root), padahal lokasi asli `/assets/game/snek.html`/`/assets/game/tetris.html`.

---

## 5. Hasil Test End-to-End (Device Fisik)

Screenshot sesi ini: Tetris jalan di `http://192.168.1.254/assets/game/tetris.html` di device fisik (bukan cuma browser test lokal) — gameplay lancar, score sempat 5918, warna 7-piece semua benar, panel canvas sudah full-height sesuai fix layout.

**Temuan user dari test fisik (BELUM DIPERBAIKI, dicatat sebagai known issue):**

1. **Audio MIDI jalan**, tapi ada sedikit perbedaan pitch dari yang diharapkan — ditoleransi user, gak dianggap blocker.
2. **High score tersimpan & bisa di-fetch dari backend** (`GET /api/game/score?game=tetris` → `{"success":true,"highscore":11746}`), **tapi gak muncul di UI** (`High Score: undefined`, kelihatan jelas di screenshot). **Root cause kemungkinan besar:** response backend pakai key `highscore` (huruf kecil semua), sedangkan `GameAPI.fetchHighScore()` di `game-shell.js` baca `data.highScore` (camelCase) — mismatch casing, `undefined` karena field-nya emang gak ketemu. **Belum dikonfirmasi 100% dan belum di-fix atas permintaan user** (didokumentasikan dulu, bukan prioritas — "nice to have").

---

## 6. Backlog Baru (dari User, Sesi Berikutnya)

- [x] **Integrasikan Snek** — WASM & backend sudah siap, tinggal bikin `snek.html` (ngikutin pola `tetris.html`) dan pasang ke `game.html` (dashboard).
- [x] **2 game tambahan** — kandidat: Conway's Game of Life dan Minesweeper
- [x] **Desain awal Task Monitor** — direncanakan mirip `mtrace`/tooling monitoring task serupa (FreeRTOS task stats real-time ke browser).
- [ ] **Infrastruktur test lokal** — saat ini aset SVG/TXT gak muncul pas ditest lokal (cuma HTML/CSS/JS/WASM yang kepakai di sandbox test), plus rencana bikin **database emulasi LittleFS 1 MB** buat testing tanpa device fisik.
- [ ] **Ekspansi File Manager & storage** — cek partisi LittleFS vs kapasitas flash fisik yang sebenarnya, pakai `partitions.csv` (saat ini `GET /api/storage` cuma laporan `LittleFS.totalBytes()`, belum tau proporsinya terhadap partisi/flash penuh).

---

## 7. Known Issues Belum Diperbaiki (Ringkasan)

- [x] `High Score` tampil `undefined` di UI meski data tersimpan benar di backend — dugaan kuat: mismatch casing `highscore` vs `highScore` antara response JSON dan `game-shell.js`.
- [ ] Pitch audio MIDI sedikit meleset dari ekspektasi (ditoleransi, non-blocking).
- [ ] Next-piece preview Tetris & tombol Pause/Reset — masih stub, ditunda.
- [x] Restart Snek belum terhubung ke frontend apa pun (backend siap, `snek.html` belum ada).

## 8. Lampiran

- Struktur Folder (lokal) per sesi
```
.
├── assets
│   ├── esp32.txt
│   └── README.md
├── compress_asset.sh
├── data
│   ├── assets
│   │   ├── core
│   │   │   ├── sidebar.css
│   │   │   ├── sidebar.js
│   │   │   ├── theme.css
│   │   │   ├── tui-base.css
│   │   │   └── webaudio-tinysynth.js.gz
│   │   ├── dashboard
│   │   │   ├── dashboard.css
│   │   │   └── dashboard-mobile.css
│   │   ├── esp32.txt.gz
│   │   ├── file-manager
│   │   │   ├── file-manager.css
│   │   │   ├── file-manager.js
│   │   │   ├── icons
│   │   │   │   ├── delete.svg.gz
│   │   │   │   ├── mkdir.svg.gz
│   │   │   │   ├── rename.svg.gz
│   │   │   │   └── upload.svg.gz
│   │   │   └── maria-system.txt.gz
│   │   ├── game
│   │   │   ├── game-ascii.txt.gz
│   │   │   ├── game.css
│   │   │   ├── game-layout.css
│   │   │   ├── game-shell.js
│   │   │   ├── gm-left.txt.gz
│   │   │   ├── gm-right.txt.gz
│   │   │   ├── snek
│   │   │   │   └── snek.svg.gz
│   │   │   ├── snek.html
│   │   │   ├── tetris
│   │   │   │   ├── tetoris.mid
│   │   │   │   ├── tetris.svg.gz
│   │   │   │   └── tetris.wasm
│   │   │   └── tetris.html
│   │   ├── gz
│   │   ├── icons
│   │   │   ├── dashboard.svg.gz
│   │   │   ├── file-manager.svg.gz
│   │   │   ├── game.svg.gz
│   │   │   ├── task-manager.svg.gz
│   │   │   └── wifi.svg.gz
│   │   ├── ost-snek.opus.gz
│   │   └── wifi-setup
│   │       ├── wifi-setup.css
│   │       └── wifi-setup.js
│   ├── file-manager.html
│   ├── game.html
│   ├── index.html
│   ├── myminegw.avif
│   ├── README.md
│   └── wifi-setup.html
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
│   ├── README.md
│   └── user-testing.md
├── platformio.ini
├── README.md
├── src
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
    ├── common
    │   └── game_common.h
    ├── README.md
    ├── snek
    │   ├── snek.c
    │   └── snek.wasm
    ├── snek-test.html
    ├── testris
    │   └── assets
    │       ├── core
    │       │   ├── sidebar.css
    │       │   ├── sidebar.js
    │       │   ├── theme.css
    │       │   ├── tui-base.css
    │       │   └── webaudio-tinysynth.js
    │       └── game
    │           ├── game-layout.css
    │           ├── game-shell.js
    │           ├── tetris.html
    │           └── tetris.wasm
    ├── tetris
    │   ├── game.c
    │   ├── game.h
    │   ├── testris.html
    │   ├── tetris.wasm
    │   └── wasm_glue.c
    └── wasm_build-snek.sh
```

