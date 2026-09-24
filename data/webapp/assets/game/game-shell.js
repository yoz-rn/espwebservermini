/* =========================================
   1. PENGELOLA AUDIO (MIDI)
   ========================================= */
class GameAudio {
    constructor(gameName) {
        this.gameName = gameName;
        this.synth = new WebAudioTinySynth();
        this.synth.setMasterVol(0.5);
        this.isReady = false;
    }

    init() {
        if (this.synth.actx && this.synth.actx.state === 'suspended') {
            this.synth.actx.resume();
        }
    }

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

    async fetchHighScore() {
        try {
            const response = await fetch(`/api/game/score?game=${this.gameName}`, { cache: "no-store" });
            if (!response.ok) return 0;
            const data = await response.json();
            return data.success ? data.highscore : 0;
        } catch (error) {
            console.error(`[${this.gameName}] Gagal mengambil skor:`, error);
            return 0;
        }
    }

    async submitScore(score) {
        try {
            const response = await fetch(`/api/game/score?game=${this.gameName}&score=${score}`, {
                method: 'POST'
            });
            if (!response.ok) return null;
            return await response.json();
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

// Token generasi: tiap loadGame() baru menaikkan ini, loop lama cek dan
// berhenti sendiri kalau sudah "kadaluarsa" — mencegah 2 loop render
// jalan bersamaan di canvas yang sama kalau loadGame() dipanggil ulang.
let _gameLoopToken = 0;

async function loadGame(wasmPath, canvasId) {
    const myToken = ++_gameLoopToken;
    const canvas = document.getElementById(canvasId);
    const ctx = canvas.getContext("2d");

    const importObject = {
        env: {
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
        if (myToken !== _gameLoopToken) return; // loop ini sudah digantikan loadGame() baru

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
            if (typeof onGameOver === 'function') {
                onGameOver(wasm.game_get_score());
            }
        } else if (!isOver && gameOverFired) {
            gameOverFired = false;
        }

        requestAnimationFrame(frame);
    }

    requestAnimationFrame(frame);
    return wasm;
}