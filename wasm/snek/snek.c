#include "../common/game_common.h"

/*
 * Grid 20×20 cell 20px → canvas 400×400px, ukuran wajar buat mobile juga. 
 * Warna format 0xAARRGGBB — nanti di JS tinggal geser bit buat pecah jadi alpha/red/green/blue
 * format ini berlaku sama persis buat tetris.c nanti biar cuma satu fungsi unpack di JS yang dipakai berdua.
 */

#define COLS 20
#define ROWS 20
#define CELL_SIZE 20
#define STEP_INTERVAL 0.12f

#define BG_COLOR   0xFF181818
#define SNAKE_COLOR 0xFF4CAF50
#define FOOD_COLOR  0xFFE53935

typedef struct { i32 x, y; } Cell;

typedef enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } Dir;

/*
 * SNAKE_CAP sengaja COLS*ROWS — itu batas maksimal ular bisa memanjang sebelum memenuhi seluruh grid
 * jadi array-nya gak akan pernah overflow apa pun yang terjadi di gameplay.
 */

#define SNAKE_CAP (COLS * ROWS)

typedef struct {
    Cell items[SNAKE_CAP];
    u32 begin;
    u32 size;
} Snake;

static Cell snake_at(Snake* s, u32 index) {
    return s->items[(s->begin + index) % SNAKE_CAP];
}

static void snake_push_back(Snake* s, Cell c) {
    s->items[(s->begin + s->size) % SNAKE_CAP] = c;
    s->size++;
}

static void snake_pop_front(Snake* s) {
    s->begin = (s->begin + 1) % SNAKE_CAP;
    s->size--;
}

/*
 * Freestanding C gak punya rand()/time() bawaan, jadi bikin sendiri (LCG sederhana)
 * seed-nya harus disuntik dari JS (misal dari Date.now()) lewat fungsi export terpisah
 * karena WASM sendiri gak punya sumber keacakan
 */

static u32 rng_state = 1;

static u32 rand_u32(void) {
    rng_state = rng_state * 1103515245u + 12345u;
    return rng_state;
}

void game_seed(u32 seed) {
    rng_state = seed;
}

/*
 * state game dan init
 */

typedef struct {
    Snake snake;
    Dir dir;
    Dir next_dir;
    Cell food;
    u32 score;
    u8 game_over;
    f32 step_cooldown;
} Game;

static Game game;

static i32 is_on_snake(Cell c) {
    for (u32 i = 0; i < game.snake.size; i++) {
        Cell s = snake_at(&game.snake, i);
        if (s.x == c.x && s.y == c.y) return 1;
    }
    return 0;
}

static void place_food(void) {
    Cell c;
    do {
        c.x = rand_u32() % COLS;
        c.y = rand_u32() % ROWS;
    } while (is_on_snake(c));
    game.food = c;
}

void game_init(void) {
    game.snake.begin = 0;
    game.snake.size = 0;
    snake_push_back(&game.snake, (Cell){ COLS / 2, ROWS / 2 });

    game.dir = DIR_RIGHT;
    game.next_dir = DIR_RIGHT;
    game.score = 0;
    game.game_over = 0;
    game.step_cooldown = STEP_INTERVAL;

    place_food();
}

/*
 * next_dir cuma di-buffer, bukan langsung dipakai — dan ditolak kalau berlawanan arah persis
 */

static i32 is_opposite(Dir a, Dir b) {
    return (a == DIR_UP && b == DIR_DOWN) || (a == DIR_DOWN && b == DIR_UP) ||
           (a == DIR_LEFT && b == DIR_RIGHT) || (a == DIR_RIGHT && b == DIR_LEFT);
}

// key: 0=up, 1=down, 2=left, 3=right — JS yang mapping tombol fisik ke angka ini
void game_keydown(i32 key) {
    if (key < 0 || key > 3) return;
    Dir requested = (Dir)key;

    if (!is_opposite(requested, game.dir)) {
        game.next_dir = requested;
    }
}

/*
 * update, dipanggil tiap frame dengan dt, gerak diskret dikontrol step_cooldown
 */

static const i32 DIR_DX[4] = { 0, 0, -1, 1 };
static const i32 DIR_DY[4] = { -1, 1, 0, 0 };

void game_update(f32 dt) {
    if (game.game_over) return;

    game.step_cooldown -= dt;
    if (game.step_cooldown > 0.0f) return;
    game.step_cooldown = STEP_INTERVAL;

    game.dir = game.next_dir;

    Cell head = snake_at(&game.snake, game.snake.size - 1);
    Cell next_head = { head.x + DIR_DX[game.dir], head.y + DIR_DY[game.dir] };

    if (next_head.x < 0 || next_head.x >= COLS || next_head.y < 0 || next_head.y >= ROWS) {
        game.game_over = 1;
        return;
    }
    if (is_on_snake(next_head)) {
        game.game_over = 1;
        return;
    }

    snake_push_back(&game.snake, next_head);

    if (next_head.x == game.food.x && next_head.y == game.food.y) {
        game.score += 1;
        place_food();
        // ekor sengaja TIDAK dipotong di sini -> badan bertambah panjang
    } else {
        snake_pop_front(&game.snake);
    }
}

/*
 * render & getter buat highscore (dipanggil JS pas game over)
 */

void game_render(void) {
    platform_fill_rect(0, 0, COLS * CELL_SIZE, ROWS * CELL_SIZE, BG_COLOR);

    for (u32 i = 0; i < game.snake.size; i++) {
        Cell c = snake_at(&game.snake, i);
        platform_fill_rect(c.x * CELL_SIZE, c.y * CELL_SIZE, CELL_SIZE, CELL_SIZE, SNAKE_COLOR);
    }

    platform_fill_rect(game.food.x * CELL_SIZE, game.food.y * CELL_SIZE, CELL_SIZE, CELL_SIZE, FOOD_COLOR);
}

u32 game_get_score(void) { return game.score; }
i32 game_is_over(void)   { return game.game_over; }