async function loadGame(wasmPath, canvasId, tickIntervalMs, cellColors) {
    const canvas = document.getElementById(canvasId);
    const ctx = canvas.getContext("2d");

    const { instance } = await WebAssembly.instantiateStreaming(fetch(wasmPath));
    const wasm = instance.exports;

    wasm.init();
    const width = wasm.get_grid_width();
    const height = wasm.get_grid_height();
    const cellSize = Math.floor(Math.min(canvas.width / width, canvas.height / height));

    function render() {
        const ptr = wasm.get_state_ptr();
        const grid = new Uint8Array(wasm.memory.buffer, ptr, width * height);

        ctx.clearRect(0, 0, canvas.width, canvas.height);
        for (let i = 0; i < grid.length; i++) {
            if (grid[i] === 0) continue; // cell kosong, gak perlu di-fill
            ctx.fillStyle = cellColors[grid[i]] || cellColors.default;
            ctx.fillRect((i % width) * cellSize, Math.floor(i / width) * cellSize, cellSize, cellSize);
        }
    }

    function tick() {
        const status = wasm.tick(); // 0 = running, 1 = game over
        render();

        if (status === 1) {
            onGameOver(wasm.get_score());
            return;
        }
        setTimeout(tick, tickIntervalMs);
    }

    tick();
    return wasm; // biar input handler bisa panggil wasm.set_input(key)
}

function readThemeColors() {
    const style = getComputedStyle(document.documentElement);
    return {
        default: style.getPropertyValue("--text-main").trim(),
        1: style.getPropertyValue("--accent").trim(),  // misal: body/block
        2: style.getPropertyValue("--border").trim(),  // misal: food/secondary
    };
}