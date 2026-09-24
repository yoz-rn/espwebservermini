const gameName = "gol";
const audio = new GameAudio(gameName);
let wasmInstance = null;
let isPaused = true; // sinkron sama default game_init() -> paused=1

const GRID_W = 100, GRID_H = 70, CELL_SIZE = 6;

// Tinggal tambah/kurangi baris di sini kalau mau ubah daftar track
const TRACKS = [
    { label: "Track 1", file: "pat.mid" },
    { label: "Track 2", file: "lacrimosa.mid" },
    { label: "Track 3", file: "lionsleeps.mid" },
    { label: "Track 4", file: "moskau.mid" },
    { label: "Track 5", file: "nadne.mid" },
    { label: "Track 6", file: "renai-1.mid" },
    { label: "Track 7", file: "renai-2.mid" },
    { label: "Track 8", file: "sweden.mid" },
    { label: "Track 9", file: "sweetdr.mid" },
];

// Koordinat relatif (0,0 = pojok kiri-atas pattern), akan di-offset ke tengah grid saat dipasang
const PRESETS = {
    glider: [[1, 0], [2, 1], [0, 2], [1, 2], [2, 2]],
    blinker: [[0, 1], [1, 1], [2, 1]],
    block: [[0, 0], [1, 0], [0, 1], [1, 1]],
};

const statusEl = document.getElementById("connection-status");

loadGame("gol/gol.wasm", "game-canvas")
    .then(instance => {
        wasmInstance = instance;
        statusEl.textContent = "Paused";
        populateTrackList();
    })
    .catch(err => {
        console.error(err);
        statusEl.textContent = "Gagal memuat game";
    });

const canvas = document.getElementById("game-canvas");

canvas.addEventListener("click", (event) => {
    if (!wasmInstance || !isPaused) return;

    const rect = canvas.getBoundingClientRect();
    const scaleX = canvas.width / rect.width;   // jaga-jaga kalau canvas di-scale via CSS
    const scaleY = canvas.height / rect.height;

    const px = (event.clientX - rect.left) * scaleX;
    const py = (event.clientY - rect.top) * scaleY;

    const gridX = Math.floor(px / CELL_SIZE);
    const gridY = Math.floor(py / CELL_SIZE);

    wasmInstance.game_toggle_cell(gridX, gridY);
});

function setPausedUI(paused) {
    isPaused = paused;
    document.getElementById("btn-pause").textContent = paused ? "Play" : "Pause";
    statusEl.textContent = paused ? "Paused" : "Running";
}

document.getElementById("btn-pause").addEventListener("click", () => {
    if (!wasmInstance) return;

    const next = !isPaused;
    wasmInstance.game_set_paused(next ? 1 : 0);
    setPausedUI(next);
});

document.getElementById("btn-randomize").addEventListener("click", () => {
    if (!wasmInstance) return;

    wasmInstance.game_randomize(); // C-side sudah otomatis set paused=1
    setPausedUI(true);
});

document.getElementById("btn-clear").addEventListener("click", () => {
    if (!wasmInstance) return;

    wasmInstance.game_init(); // reuse full reset, juga otomatis paused=1
    setPausedUI(true);
});

function applyPreset(name) {
    if (!wasmInstance || !isPaused) return;

    const cells = PRESETS[name];
    if (!cells) return;

    // Cari lebar/tinggi pattern biar bisa di-center
    const maxX = Math.max(...cells.map(c => c[0]));
    const maxY = Math.max(...cells.map(c => c[1]));

    const offsetX = Math.floor((GRID_W - maxX) / 2);
    const offsetY = Math.floor((GRID_H - maxY) / 2);

    for (const [dx, dy] of cells) {
        wasmInstance.game_toggle_cell(offsetX + dx, offsetY + dy);
    }
}

document.querySelectorAll("[data-preset]").forEach(btn => {
    btn.addEventListener("click", () => applyPreset(btn.dataset.preset));
});

const speedSlider = document.getElementById("speed-slider");
const speedValue = document.getElementById("speed-value");

speedSlider.addEventListener("input", (e) => {
    if (!wasmInstance) return;

    const seconds = parseFloat(e.target.value);
    wasmInstance.game_set_interval(seconds);
    speedValue.textContent = `${seconds.toFixed(2)}s`;
});

function populateTrackList() {
    const select = document.getElementById("track-select");

    TRACKS.forEach((track, index) => {
        const opt = document.createElement("option");
        opt.value = track.file;
        opt.textContent = track.label;
        select.appendChild(opt);
    });

    if (TRACKS.length > 0) {
        playTrack(TRACKS[0].file, TRACKS[0].label);
    }
}

function playTrack(file, label) {
    audio.playBGM(file, 1); // loop=1, sama seperti Snek/Tetris
    document.getElementById("now-playing").textContent = `Now Playing: ${label}`;
}

document.getElementById("btn-play-track").addEventListener("click", () => {
    const select = document.getElementById("track-select");
    const selectedTrack = TRACKS.find(t => t.file === select.value);
    if (!selectedTrack) return;

    playTrack(selectedTrack.file, selectedTrack.label);
});