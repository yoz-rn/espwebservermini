# Recap Sesi: Penyelesaian Roadmap Phase 1 (Network Foundation & Core Systems)

**Project:** espwebservermini
**Status akhir sesi:** Seluruh 4 item Phase 1 (README) tuntas dan terverifikasi di device fisik lewat log serial: WiFi Provisioning (sudah selesai sesi sebelumnya), Connection Status Indicator, Auto-Reconnect (kombinasi exponential backoff + hold-saat-ada-client-AP + cap percobaan), dan Forget Wi-Fi (plus fitur bonus: one-click Reconnect dan Disconnect manual). Dua insiden crash device ditemukan dan diselesaikan tuntas selama sesi — satu di antaranya mengoreksi hipotesis akar masalah dari investigasi sebelumnya di sesi yang sama. Empat bug logic senyap (lolos compile, tidak tertangkap warning) ditemukan di implementasi pertama Auto-Reconnect.

---

## 1. Connection Status Indicator

- `NetworkManager::isSTAConnected()` — getter tipis di atas `_staConnected` (`std::atomic<bool>`, sudah ada dari sesi refactor event-driven sebelumnya).
- `GET /api/wifi-status` diperluas: field `staConnected` dan `staIP` ditambahkan berdampingan dengan field provisioning (`status`/`message`) yang sudah ada — **bukan** endpoint baru, karena semantiknya masih "status jaringan".
- **Keputusan penempatan UI:** sidebar (bawah nav, `margin-top: auto` + `border-top` separator), fetch **sekali** saat sidebar di-load (bukan polling) — dipilih user karena sifat halaman-halaman proyek ini yang memang sudah banyak mengandalkan reload/navigasi manual, jadi polling berkelanjutan di semua halaman (termasuk saat idle di file-manager/task-monitor) dianggap boros tanpa manfaat sepadan.
- Styling **mereuse pola warna status task** yang sudah ada di Task Monitor (`.st-running` = accent, `.st-blocked`/`.st-suspended` = muted) — connected = accent, AP-only = muted, gagal fetch **juga** dianggap AP-only (bukan state "Unknown" terpisah, sesuai keputusan eksplisit user).

---

## 2. Auto-Reconnect — Rancangan Awal

- **Bukan** `WiFi.setAutoReconnect(true)` bawaan Arduino core: perilakunya retry tanpa jeda begitu disconnect, kontradiktif dengan filosofi hemat-resource proyek ini (event-driven, bukan polling/retry-storm).
- **Bukan** task FreeRTOS baru: proyek ini baru saja direfactor dari task-based ke event-driven di sesi sebelumnya, jadi dipilih `esp_timer` (software timer, jalan di timer service task milik sistem).
- **One-shot, bukan periodic:** karena interval backoff berubah-ubah tiap kali gagal, `esp_timer_start_once()` dipanggil ulang manual tiap fire (`scheduleReconnect()`), bukan `esp_timer` mode periodic yang intervalnya fixed.
- Skema exponential backoff: `RECONNECT_BASE_MS=15000`, `RECONNECT_MAX_MS=300000`, dilipatgandakan tiap percobaan gagal, di-cap di maksimum.
- **Celah desain awal yang ditemukan lewat log:** kegagalan connect *saat boot* (`beginSTA()` gagal di `setup()`) tidak pernah memicu siklus backoff sama sekali, karena `onWifiEvent()`'s `STA_DISCONNECTED` cuma menjadwalkan reconnect di cabang "baru saja terputus dari kondisi tersambung" (`if (_staConnected)`) — sementara `_staConnected` dari awal memang `false` di kasus gagal-saat-boot. Diperbaiki dengan `notifyInitialSTAFailure()`, dipanggil eksplisit dari `main.cpp` setelah `beginSTA()` return `false`.
- Verifikasi matematis: timestamp connect-berhasil di beberapa log serial dicocokkan manual terhadap jadwal kumulatif backoff (15s → 45s → 105s → 225s → ...) — selisih waktu asosiasi WiFi non-instan (async) dikonfirmasi cukup menjelaskan gap antara jadwal terjadwal dan sukses aktual, tanpa penyimpangan dari skema.

---

## 3. Kombinasi B (Hold saat Ada Client AP) + A (Cap Percobaan)

User melaporkan gangguan nyata: setiap kali `attemptReconnect()` jalan, client yang terhubung ke AP ESP (laptop/HP user sendiri) mengalami instabilitas — diputuskan mengatasi lewat kombinasi dua mekanisme, bukan sekadar memperpanjang interval:

- **Opsi B — tunda selama ada client AP aktif:** counter `_apClientCount` (`std::atomic<uint8_t>`, di-increment/decrement dari event `AP_STACONNECTED`/`AP_STADISCONNECTED`). Kalau `attemptReconnect()` jalan sementara `_apClientCount > 0`, percobaan **tidak** dieksekusi — timer di-reschedule dengan interval yang **sama** (`scheduleReconnect(false)`, parameter `advanceBackoff` baru ditambahkan supaya "ditunda" tidak ikut dihitung sebagai "gagal").
- **Opsi A — cap maksimum:** `MAX_RECONNECT_ATTEMPTS=5` per episode disconnect; setelah limit tercapai, device berhenti mencoba sampai episode baru dimulai (disconnect ulang, atau STA berhasil connect yang me-reset counter).
- Titik reset counter/interval: `STA_GOT_IP` (reset penuh, episode selesai sukses) dan awal episode disconnect baru (`STA_DISCONNECTED` dari kondisi tersambung) — supaya cap tidak "menghukum" episode berikutnya di lain waktu.

### Root cause instabilitas AP client (dikonfirmasi lewat korelasi log)
ESP32 mode AP+STA berbagi **satu radio** (time-sharing, bukan dua radio terpisah). Tiap kali `WiFi.begin()` dipanggil untuk reconnect, radio memaksa channel realignment (CSA — Channel Switch Announcement) demi mencoba asosiasi ke STA, mengganggu AP yang sedang melayani client. Dikonfirmasi lewat dua log independen: churn leave/join AP client (~15-30 detik) muncul **persis** mulai dari titik STA disconnect sampai STA reconnect sukses, dan berhenti total begitu STA stabil — bukan bug di kode kita, melainkan keterbatasan arsitektur radio ESP32.

### Temuan susulan (belum diverifikasi): laptop vs smartphone
Dari log terpisah dengan MAC/IP client yang bisa dibedakan: smartphone (192.168.1.1) tetap stabil ~125 detik tanpa drop, sementara laptop (192.168.1.2) churn tiap ~8-9 detik **bahkan saat STA sudah stabil** (bukan cuma pas reconnect). Menunjukkan sebagian instabilitas mungkin bukan murni sisi ESP. Hipotesis belum diuji: power-saving adapter WiFi laptop, penanganan sinyal CSA yang beda per chipset, jarak/sinyal fisik, aktivitas WiFi background di laptop.

### Empat bug ditemukan di implementasi pertama (semua senyap — compile sukses, tidak ada warning)
1. **Kondisi kredensial terbalik:** `if (hasSavedCredentials()) return;` (seharusnya `if (!hasSavedCredentials())`) — membuat `attemptReconnect()` **selalu** bail out sebelum sempat memanggil `WiFi.begin()` di kasus normal. Seluruh mekanisme backoff efektif tidak pernah benar-benar mencoba reconnect; keberhasilan yang teramati sebelumnya di log kemungkinan besar dari mekanisme reassociation internal ESP-IDF sendiri, bukan kode ini.
2. **Cap `MAX_RECONNECT_ATTEMPTS` hilang total:** counter di-increment tapi tidak pernah dibandingkan terhadap limit — bagian "A" dari kesepakatan tidak ada efeknya.
3. **Cabang hold tidak reschedule:** `esp_timer` bersifat one-shot; cabang "client aktif, tunda" cuma `return` tanpa `scheduleReconnect(false)`, membuat timer **mati permanen** di percobaan pertama yang tertunda, bukan sekadar ditunda.
4. **Typo namespace NVS:** `_prefs.begin("wifi-begin", true)` (seharusnya `"wifi-config"`, namespace yang dipakai konsisten di semua fungsi lain) — `getString("ssid","")` selalu balik string kosong.

Keempatnya diperbaiki dan diverifikasi lewat log serial: cabang hold reschedule dengan interval tetap (15s→30s→30s→30s selama client tetap aktif), counter percobaan mengecualikan siklus hold (baru naik ke `1/5` setelah client AP benar-benar disconnect), dan SSID yang dicoba sesuai NVS (`"ESCAPE_VELOCITY"`, bukan string kosong).

---

## 4. Forget Wi-Fi

- **Keputusan scope:** Opsi 1 (single-slot, sesuai struktur NVS yang sudah ada — list cuma menampilkan satu entry kalau ada) dipilih dibanding Opsi 2 (multi-network storage, list beberapa jaringan tersimpan sekaligus). Opsi 2 butuh restrukturisasi `NetworkManager` yang jauh lebih besar (ganti skema Preferences dari 2 key tunggal jadi array/list) — didokumentasikan sebagai saran pengembangan, bukan dikerjakan sesi ini.
- `NetworkManager::forgetSTACredentials()` — hapus `ssid`/`password` dari NVS, plus `stopReconnect()` (mencegah timer yang mungkin sedang terjadwal mencoba connect ke kredensial yang sudah tidak ada).
- `NetworkManager::disconnectSTA()` — `WiFi.disconnect()` tanpa menghapus kredensial (device kembali AP-only, siap direconnect manual atau lewat restart).
- `getSavedSSID()` ditambahkan supaya frontend bisa render entry "jaringan tersimpan" walau lagi AP-only (`staIP` kosong tidak cukup untuk tahu SSID mana yang tersimpan).
- **UX tambahan atas permintaan user** (bukan cuma tombol Forget polos):
  - **Restrukturisasi `wifi-setup.html`** jadi 2 panel terpisah (`jaringan tersimpan` / `tambah jaringan`), bukan satu panel besar — konsisten dengan pola card di halaman lain.
  - **One-click Reconnect** (`reconnectSaved()`) — pakai kredensial yang sudah tersimpan di NVS, tanpa perlu isi ulang form provisioning. Sengaja **tidak** reuse guard "tunda-jika-ada-client-AP" milik `attemptReconnect()` background, karena klik tombol adalah niat eksplisit user, bukan retry latar belakang.
  - **Forget otomatis disconnect duluan** kalau STA sedang aktif (urutan: disconnect dulu, baru hapus kredensial), bukan sekadar hapus NVS tanpa peduli status koneksi.
  - Tombol "Disconnect"/"Connect" di UI **berubah fungsi** sesuai state (bukan `disabled` statis), styling status-dot mereuse pola yang sama dengan sidebar indicator (§1).

---

## 5. Insiden Crash #1 — `lwip` Assert Setelah Churn Disconnect/Reconnect

- **Gejala:** `assert failed: tcp_update_rcv_ann_wnd ... (new_rcv_ann_wnd <= 0xffff)` di dalam `AsyncTCP::_tcp_recved_api` → panic → reboot. Muncul setelah user menguji tombol Disconnect berkali-kali.
- **Root cause bug fungsional yang memicu:** `disconnectSTA()` sudah manggil `stopReconnect()` sebelum `WiFi.disconnect()`, tapi `onWifiEvent()`'s `STA_DISCONNECTED` **tidak membedakan** disconnect manual vs tak sengaja — begitu event masuk, `scheduleReconnect()` dipanggil lagi otomatis karena `hasSavedCredentials()` masih `true`. Efeknya, klik "Disconnect" user selalu diikuti auto-reconnect ~15 detik kemudian, menghasilkan siklus disconnect-reconnect berulang tanpa diminta.
- **Fix:** flag `std::atomic<bool> _manualDisconnectRequested`, di-set `true` sebelum `WiFi.disconnect()` di `disconnectSTA()`, dicek-dan-dikonsumsi (reset ke `false`) sekali di `onWifiEvent()`'s `STA_DISCONNECTED` untuk skip `scheduleReconnect()` — hanya berlaku untuk **satu** event berikutnya, disconnect tak sengaja di lain waktu tetap memicu backoff normal.
- **Hipotesis awal (kemudian dikoreksi, lihat §6):** churn 3 siklus disconnect-reconnect dalam ~52 detik yang menstabilkan-ulang state TCP di `lwip`/`AsyncTCP` diduga sebagai penyebab crash. Reset reason `SW_CPU_RESET` (bukan watchdog/brownout) dikonfirmasi lewat log — menyingkirkan dua hipotesis alternatif (task watchdog timeout, kegagalan power supply).

---

## 6. Insiden Crash #2 — Koreksi Root Cause: Bukan Churn, tapi Context Thread

- Setelah fix `_manualDisconnectRequested` diterapkan (tidak ada lagi churn — cuma satu kali disconnect per klik), crash **tetap muncul, bahkan instan** (~48ms setelah log "Forget" tercatat) — membuktikan hipotesis "churn beruntun" di §5 **tidak lengkap/kurang tepat**.
- **Akar masalah sebenarnya:** handler route (`WifiRoutes.cpp`) mengeksekusi `forgetSTACredentials()`/`disconnectSTA()`/`reconnectSaved()` **langsung, synchronous, di dalam context thread `AsyncTCP`/`lwip` itu sendiri** — thread yang sama yang masih punya kerjaan tertunda (mengirim response HTTP untuk request yang sama). Operasi berat di dalamnya (`WiFi.disconnect()` yang mengganggu netif, plus `_prefs.remove()` yang blocking ke flash NVS) mengacaukan bookkeeping window TCP (`rcv_ann_wnd`) untuk koneksi yang sedang diproses thread itu, memicu assert yang sama.
- **Fix — pola deferred execution:** route handler tidak lagi mengeksekusi aksi disruptif secara langsung, melainkan **menjadwalkan** lewat `esp_timer` one-shot (`scheduleDeferred(DeferredOp)`, delay 300ms) lalu langsung kirim response. Eksekusi sebenarnya (`runDeferredOp()`) baru terjadi setelah response HTTP tuntas terkirim, di luar context thread `lwip` yang sedang sibuk. Tiga entry point publik baru (`requestDisconnect()`, `requestForget()`, `requestReconnect()`) menggantikan pemanggilan langsung method private lama dari route.
- **Konsekuensi desain:** response HTTP untuk ketiga endpoint ini sekarang selalu optimis (`200 OK`, pesan "...sedang diproses"), tidak lagi bisa melaporkan sukses/gagal secara langsung — hasil sebenarnya baru diketahui lewat polling `/api/wifi-status` setelahnya (frontend diberi jeda `setTimeout` sebelum refresh).
- **Bug kompilasi transisi (senyap-tapi-cepat-ketahuan compiler):** deklarasi `scheduleDeferred()` sempat kelewat di header (implementasi ada di `.cpp` tanpa deklarasi cocok di `.h`), dan `enum class DeferredOp` sempat berada di blok `private` yang tidak terjangkau; route lama yang masih pakai pola `bool ok = networkManager.requestForget()` (padahal method baru bertipe `void`) juga sempat tertinggal. Ketiganya adalah error compile-time biasa, cepat diperbaiki.
- **Prinsip umum yang dicatat dari insiden ini (worth diingat di luar konteks 3 tombol ini):** operasi apa pun yang mengganggu network stack itu sendiri (disconnect WiFi, restart device, dsb.) **tidak boleh** dieksekusi langsung di dalam HTTP request handler — selalu tunda lewat timer, supaya response sempat "keluar" dengan bersih dari context thread yang sama.

---

## 7. Verifikasi Akhir

Log serial pasca-fix menunjukkan seluruh siklus (disconnect manual → diam, tidak auto-reconnect → klik Connect manual → sukses → klik Forget → disconnect otomatis duluan → kredensial terhapus → provisioning ulang sukses) berjalan **tanpa crash sama sekali**. Satu detail tambahan yang teramati sebagai bukti ketahanan desain backoff (bukan bug): router user sempat menolak asosiasi sementara (`Association refused temporarily`, termasuk satu nilai `comeback time` yang aneh/overflow dari sisi router), namun backoff otomatis menaikkan interval dan percobaan berikutnya berhasil normal — tanpa retry mechanism, device berisiko nyangkut gagal connect selamanya.

---

## 8. Roadmap Phase 1 — Status Akhir

- [x] Self-Service Wi-Fi Configuration (selesai sesi sebelumnya)
- [x] Connection Status Indicator
- [x] Auto-Reconnect (backoff + hold-saat-ada-client + cap percobaan)
- [x] Forget Wi-Fi (plus bonus: one-click Reconnect, Disconnect manual, restrukturisasi UI jadi 2 panel)

Seluruh Phase 1 (Network Foundation & Core Systems) di README kini tuntas dan terverifikasi di device fisik.

---

## 9. Backlog Baru

- [ ] Multi-network storage (Opsi 2 dari §4) — restrukturisasi NVS dari 2-key tunggal jadi list/array, supaya dashboard bisa menampilkan beberapa jaringan tersimpan sekaligus seperti pengalaman device pada umumnya.
- [ ] Investigasi hipotesis instabilitas AP-client sisi laptop (§3): cek power-saving adapter WiFi, uji jarak/sinyal, bandingkan penanganan CSA antar chipset.
- [ ] Terapkan pola deferred-execution (§6) di titik lain yang berpotensi sama — mis. kalau nanti ada fitur restart device dari dashboard, atau operasi berat lain yang dipanggil langsung dari HTTP handler.
- [ ] Bersihkan noise log `Preferences.cpp getString(): nvs_get_str len fail: ssid NOT_FOUND` yang muncul (aman, tapi kosmetik) setelah Forget — bisa dihindari dengan cek `hasSavedCredentials()` dulu di titik-titik yang masih rawan.
- [x] Pertimbangkan dokumentasi terpisah (di luar devlog) untuk pola-pola arsitektural yang sudah berulang kali relevan di proyek ini: context-thread-awareness (deferred execution), one-shot vs periodic `esp_timer`, exponential backoff — sebagai referensi cepat, bukan tersebar di banyak devlog.

## Lampiran

**Berkas baru/berubah sesi ini:**

```
include/
├── NetworkManager.h      (isSTAConnected, getSavedSSID, backoff/hold/cap state,
│                           _manualDisconnectRequested, DeferredOp + scheduleDeferred)
└── (WifiRoutes.h — tidak berubah signature)
src/
├── NetworkManager.cpp    (auto-reconnect backoff, forget/disconnect/reconnect,
│                           manual-disconnect flag, deferred execution)
├── main.cpp              (notifyInitialSTAFailure() setelah beginSTA() gagal di boot)
└── routes/
    └── WifiRoutes.cpp    (field staConnected/staIP/savedSSID, endpoint
                            wifi-forget/wifi-disconnect/wifi-reconnect, panggil
                            request*() bukan method langsung)
data/assets/
├── core/
│   ├── sidebar.js        (initSidebarWifiStatus — fetch sekali, bukan polling)
│   └── sidebar.css       (.sidebar-status, .status-dot, wifi-connected/wifi-ap-only)
└── wifi-setup/
    ├── wifi-setup.html   (restrukturisasi 2 panel: jaringan tersimpan + tambah jaringan)
    ├── wifi-setup.css    (.wifi-page, .network-entry, .network-actions, .network-status)
    └── wifi-setup.js     (renderSavedNetwork, handleDisconnect/Forget/Reconnect + polling)
logs/
└── devlog-8.md           (baru)
```