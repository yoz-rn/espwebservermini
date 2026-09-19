/* =========================================
   1. PENGELOLA AUDIO (MIDI)
   ========================================= */
class GameAudio {
    constructor(gameName) {
        this.gameName = gameName;
        this.synth = new WebAudioTinySynth();
        this.synth.setMasterVol(0.5); // Volume default
        this.isReady = false;
    }

    // Wajib dipanggil melalui interaksi user (klik tombol)
    init() {
        if (this.synth.actx && this.synth.actx.state === 'suspended') {
            this.synth.actx.resume();
        }
    }

    // Memuat dan memutar BGM — file statis biasa (sudah ter-serve otomatis
    // lewat serveStatic), BUKAN lewat endpoint API. Konvensi path: filename
    // diasumsikan ada di dalam subfolder senama gameName (mis. "tetris/tetoris.mid"),
    // sesuai struktur folder yang sudah kamu susun.
    playBGM(filename, loop = 1) {
        this.init();

        fetch(`${this.gameName}/${filename}`)
            .then(response => {
                if (!response.ok) throw new Error("Gagal mengambil aset BGM");
                return response.arrayBuffer();
            })
            .then(midiData => {
                this.synth.loadMIDI(midiData);
                this.synth.setLoop(loop);
                this.synth.playMIDI();
                this.isReady = true;
            })
            .catch(error => console.error(`[${this.gameName}] BGM Error:`, error));
    }

    stopBGM() {
        this.synth.stopMIDI();
    }
}

/* =========================================
   2. PENGELOLA API (SKOR)
   ========================================= */
class GameAPI {
    constructor(gameName) {
        this.gameName = gameName;
    }

    // Mengambil high score dari server C++
    async fetchHighScore() {
        try {
            const response = await fetch(`/api/game/score?game=${this.gameName}`, { cache: "no-store" });
            if (!response.ok) return 0;
            const data = await response.json();
            return data.success ? data.highScore : 0; // "highScore" camelCase, sesuai GameRoutes.cpp
        } catch (error) {
            console.error(`[${this.gameName}] Gagal mengambil skor:`, error);
            return 0;
        }
    }

    // Mengirim skor baru ke server C++ — query param, TANPA body,
    // sesuai desain endpoint POST /api/game/score yang baca lewat
    // request->getParam(name) biasa (post=false / query string)
    async submitScore(score) {
        try {
            const response = await fetch(`/api/game/score?game=${this.gameName}&score=${score}`, {
                method: 'POST'
            });
            if (!response.ok) return null;
            return await response.json(); // { success, highScore, isNewRecord }
        } catch (error) {
            console.error(`[${this.gameName}] Gagal submit skor:`, error);
            return null;
        }
    }
}

/* =========================================
   3. PENGELOLA WASM (RENDER & LOGIC)
   ========================================= */
function unpackColor(argb) {
    const a = (argb >>> 24) & 0xFF;
    const r = (argb >>> 16) & 0xFF;
    const g = (argb >>> 8) & 0xFF;
    const b = argb & 0xFF;
    return `rgba(${r},${g},${b},${a / 255})`;
}

// Memuat game WASM dan menghubungkannya dengan Canvas HTML.
// Kontrak export WASM (sama persis di snek.wasm & tetris.wasm):
//   game_seed(u32), game_init(), game_update(f32 dt), game_render(),
//   game_keydown(i32 key), game_get_score() -> u32, game_is_over() -> i32
// Warna game di-hardcode di C (bukan parameter) — sesuai keputusan terakhir.
async function loadGame(wasmPath, canvasId) {
    const canvas = document.getElementById(canvasId);
    const ctx = canvas.getContext("2d");

    const importObject = {
        env: {
            // Satu-satunya import dari JS: WASM manggil ini buat gambar tiap cell
            platform_fill_rect: (x, y, w, h, color) => {
                ctx.fillStyle = unpackColor(color >>> 0);
                ctx.fillRect(x, y, w, h);
            },
        },
    };

    const response = await fetch(wasmPath);
    if (!response.ok) throw new Error(`fetch ${wasmPath} gagal: HTTP ${response.status}`);
    const bytes = await response.arrayBuffer();

    const { instance } = await WebAssembly.instantiate(bytes, importObject);
    const wasm = instance.exports;

    wasm.game_seed(Date.now());
    wasm.game_init();

    let lastTime = 0;
    let gameOverFired = false;

    function frame(timestamp) {
        if (!lastTime) lastTime = timestamp;
        const dt = (timestamp - lastTime) / 1000;
        lastTime = timestamp;

        wasm.game_update(dt);
        wasm.game_render();

        const scoreEl = document.getElementById("score-display");
        if (scoreEl) scoreEl.textContent = wasm.game_get_score();

        const isOver = wasm.game_is_over();
        if (isOver && !gameOverFired) {
            gameOverFired = true;
            // Fungsi onGameOver harus didefinisikan secara global di file HTML game
            if (typeof onGameOver === 'function') {
                onGameOver(wasm.game_get_score());
            }
        } else if (!isOver && gameOverFired) {
            gameOverFired = false; // reset biar bisa fire lagi kalau nanti ada restart
        }

        requestAnimationFrame(frame);
    }

    requestAnimationFrame(frame);
    return wasm; // dipakai input handler buat manggil wasm.game_keydown(key)
}
