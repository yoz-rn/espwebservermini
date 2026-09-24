#include "../common/game_common.h"

#define GRID_W 100
#define GRID_H 70
#define CELL_SIZE 6

#define BG_COLOR   0xFF181818
#define CELL_COLOR 0xFF4CAF50

typedef struct {
    u8 grid[GRID_H][GRID_W];
    u8 next_grid[GRID_H][GRID_W];

    u32 generation;
    i32 paused;

    f32 interval;       // detik per generasi (dikontrol speed slider)
    f32 step_cooldown;  // sisa waktu sebelum generasi berikutnya
} Game;

static Game game;

static u32 rng_state = 1;

static u32 rand_u32(void) {
    rng_state = rng_state * 1103515245u + 12345u;
    return rng_state;
}

void game_seed(u32 seed) {
    rng_state = seed;
}

void game_init(void) {
    for (i32 y = 0; y < GRID_H; y++) {
        for (i32 x = 0; x < GRID_W; x++) {
            game.grid[y][x] = 0;
        }
    }

    game.generation = 0;
    game.paused = 1;
    game.interval = 0.15f;
    game.step_cooldown = game.interval;
}

static i32 count_live_neighbors(i32 y, i32 x) {
    i32 count = 0;

    for (i32 dy = -1; dy <= 1; dy++) {
        for (i32 dx = -1; dx <= 1; dx++) {
            if (dy == 0 && dx == 0) continue;

            i32 ny = (y + dy + GRID_H) % GRID_H;
            i32 nx = (x + dx + GRID_W) % GRID_W;

            count += game.grid[ny][nx];
        }
    }

    return count;
}

static void compute_next_generation(void) {
    u32 alive_count = 0;

    for (i32 y = 0; y < GRID_H; y++) {
        for (i32 x = 0; x < GRID_W; x++) {
            i32 neighbors = count_live_neighbors(y, x);
            u8 alive = game.grid[y][x];

            u8 next_state;
            if (alive) {
                next_state = (neighbors == 2 || neighbors == 3);
            } else {
                next_state = (neighbors == 3);
            }

            game.next_grid[y][x] = next_state;
            alive_count += next_state;
        }
    }

    for (i32 y = 0; y < GRID_H; y++) {
        for (i32 x = 0; x < GRID_W; x++) {
            game.grid[y][x] = game.next_grid[y][x];
        }
    }

    game.generation = (alive_count == 0) ? 0 : game.generation + 1;
}

void game_update(f32 dt) {
    if (game.paused) return;

    game.step_cooldown -= dt;
    if (game.step_cooldown > 0.0f) return;

    game.step_cooldown = game.interval;
    compute_next_generation();
}

void game_render(void) {
    platform_fill_rect(0, 0, GRID_W * CELL_SIZE, GRID_H * CELL_SIZE, BG_COLOR);

    for (i32 y = 0; y < GRID_H; y++) {
        for (i32 x = 0; x < GRID_W; x++) {
            if (game.grid[y][x]) {
                platform_fill_rect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE, CELL_COLOR);
            }
        }
    }
}

u32 game_get_score(void) { return game.generation; }
i32 game_is_over(void)   { return 0; }

void game_toggle_cell(i32 x, i32 y) {
    if (!game.paused) return;
    if (x < 0 || x >= GRID_W || y < 0 || y >= GRID_H) return;

    game.grid[y][x] = !game.grid[y][x];
}

void game_set_paused(i32 paused) {
    game.paused = paused ? 1 : 0;
}

void game_randomize(void) {
    for (i32 y = 0; y < GRID_H; y++) {
        for (i32 x = 0; x < GRID_W; x++) {
            game.grid[y][x] = (rand_u32() % 100) < 15; // ~15% alive, niru densitas referensi 2
        }
    }

    game.generation = 0;
    game.step_cooldown = game.interval;
    game.paused = 1;
}

void game_set_interval(f32 seconds) {
    if (seconds <= 0.0f) return;
    game.interval = seconds;
}