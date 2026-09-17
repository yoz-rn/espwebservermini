# Protokol Testing — espwebservermini

Panduan ini buat dijalankan oleh orang lain (bukan developer proyek ini), supaya perilaku pemakaiannya alami dan nggak ketebak. Ikuti urutan dari atas ke bawah, catat apa pun yang terasa aneh, lambat, error, atau nggak sesuai ekspektasi — nggak perlu tau istilah teknis, cukup deskripsikan apa yang dilihat/dialami.

## Sebelum Mulai

- [ ] Pastikan device dalam kondisi baru di-flash ulang (firmware + filesystem)
- [ ] Siapkan laptop/HP buat konek ke Wi-Fi

## Tahap 1 — Koneksi Awal

1. Cari nama Wi-Fi milik device ini di daftar Wi-Fi kamu, lalu konek
2. Buka browser, akses `http://192.168.1.254`
3. Kalau muncul halaman setup Wi-Fi, coba hubungkan device ke jaringan rumah/kampus kamu pakai halaman itu
4. Setelah device konek ke jaringan itu, coba akses lagi lewat `http://esp32.local` dari perangkat yang sama

**Catat:** apakah semua langkah di atas jalan tanpa nyangkut? Kalau ada yang gagal, di langkah mana persisnya?

## Tahap 2 — Jelajahi File Manager (Tanpa Instruksi Detail)

Buka halaman File Manager. Coba eksplorasi sendiri semaumu — anggap ini file explorer biasa di komputer. Beberapa hal yang bisa dicoba (urutan bebas):

- [ ] Buka folder yang ada
- [ ] Balik ke folder sebelumnya
- [ ] Lihat isi salah satu file (klik file teks maupun gambar kalau ada)
- [ ] Upload file baru dari perangkatmu
- [ ] Upload file dengan nama yang sama persis dengan file yang sudah ada — lihat apa yang terjadi
- [ ] Buat folder baru
- [ ] Ganti nama file atau folder
- [ ] Hapus file atau folder

**Catat:** apa yang bikin bingung? Adakah tombol/aksi yang hasilnya nggak sesuai dugaan kamu?

## Tahap 3 — Coba "Bikin Rusak" (Sengaja)

Bagian ini justru minta kamu coba hal-hal yang mungkin nggak wajar dilakukan orang normal. Tujuannya nyari titik lemah.

- [ ] Coba upload file yang cukup besar (foto resolusi tinggi, video pendek, dll)
- [ ] Klik tombol yang sama berkali-kali dengan cepat berturut-turut
- [ ] Buka banyak tab sekaligus ke halaman yang sama, lakukan aksi berbeda di tiap tab
- [ ] Coba hapus folder yang masih ada isinya
- [ ] Coba ganti nama file jadi nama yang sudah dipakai file lain
- [ ] Kalau ngerti cara edit URL di address bar, coba ubah bagian `path=` jadi sesuatu yang aneh (opsional, lewati kalau nggak familiar)

**Catat:** apakah device masih responsif setelahnya, atau butuh di-reset/reload?

## Tahap 4 — Kondisi Nggak Ideal

- [ ] Jauhkan diri dari device sampai sinyal Wi-Fi lemah, coba tetap pakai halamannya
- [ ] Matikan lalu nyalakan lagi device di tengah-tengah kamu lagi upload/proses sesuatu (kalau memungkinkan)
- [ ] Setelah device nyala lagi, cek apakah file-file yang ada sebelumnya masih ada dan nggak rusak

## Tahap 5 — Kesan Umum

Jawab bebas, nggak perlu formal:

- Bagian mana yang paling nyaman dipakai?
- Bagian mana yang paling bikin bingung atau ilfeel?
- Kalau ini produk beneran, ada yang bikin kamu ragu buat pakai?
- Ada saran tampilan atau alur yang menurutmu lebih enak?

## Cara Melaporkan Temuan

Untuk tiap masalah yang ditemukan, sebutkan:
1. Apa yang kamu lakukan (langkah-langkah sebelum masalah muncul)
2. Apa yang kamu harapkan terjadi
3. Apa yang benar-benar terjadi
4. Perangkat & browser yang dipakai (HP/laptop, nama browser)