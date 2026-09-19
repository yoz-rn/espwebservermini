#include "../common/game_common.h"
#include "game.h"

static game_state_t state;
static input_event_t pending_event = NO_INPUT;

#define CELL_SIZE 20

static const u32 COLORS[N_SHAPES + 1] = {
    0xFF111111, // Empty
    0xFFFFC82E, // Yellow (O)
    0xFF01EDFA, // Cyan   (T)
    0xFFEA141C, // Red    (L)
    0xFFFF910C, // Orange (J)
    0xFF39892F, // Dark Green (I)
    0xFF0077D3, // Blue   (S)
    0xFF78256F, // Purple (Z)
};

static void draw_cell(i32 x, i32 y, u32 color) {
    platform_fill_rect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE, 0xFF0C0C0C);
    platform_fill_rect(x * CELL_SIZE + 1, y * CELL_SIZE + 1, CELL_SIZE - 2, CELL_SIZE - 2, color);
}

void game_init(void) {
    state = game_state_new();
    pending_event = NO_INPUT;
}

// key: 0=left, 1=right, 2=rotate, 3=soft_drop, 4=hard_drop, 5=restart
void game_keydown(i32 key) {
    switch (key) {
        case 0: pending_event = LEFT; break;
        case 1: pending_event = RIGHT; break;
        case 2: pending_event = ROTATE; break;
        case 3: pending_event = SOFT_DROP; break;
        case 4: pending_event = HARD_DROP; break;
        case 5: pending_event = START; break;
        default: break;
    }
}

void game_update(f32 dt) {
    unsigned int elapsed_ms = (unsigned int)(dt * 1000.0f);
    game_state_update(&state, pending_event, elapsed_ms);
    pending_event = NO_INPUT;
}

void game_render(void) {
    platform_fill_rect(0, 0, GRID_WIDTH * CELL_SIZE, GRID_HEIGHT * CELL_SIZE, 0xFF0C0C0C);

    for (int r = 0; r < GRID_HEIGHT; r++) {
        for (int c = 0; c < GRID_WIDTH; c++) {
            if (state.grid[r][c] != 0) draw_cell(c, r, COLORS[state.grid[r][c]]);
        }
    }

    for (int i = 0; i < 4; i++) {
        int x = state.current_shape[i * 2] + state.current_x;
        int y = state.current_shape[i * 2 + 1] + state.current_y;
        if (y >= 0) draw_cell(x, y, COLORS[state.current_shape_kind + 1]);
    }
}

u32 game_get_score(void) { return state.score; }
i32 game_is_over(void)   { return (i32)state.game_over; }
