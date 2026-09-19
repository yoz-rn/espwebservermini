#pragma once
#define GRID_WIDTH 10
#define GRID_HEIGHT 20
#define N_SHAPES 7

typedef enum {
  QUIT = -1,
  NO_INPUT,
  ANY_INPUT,
  LEFT,
  RIGHT,
  DOWN,
  ROTATE,
  SOFT_DROP,
  HARD_DROP,
  START,
} input_event_t;

typedef struct {
  unsigned int score;
  unsigned int level;

  int grid[GRID_HEIGHT][GRID_WIDTH];

  int current_shape[8];

  int current_shape_kind;

  int current_x;
  int current_y;

  unsigned int fall_elapsed_ms;
  unsigned int fall_period_ms;

  unsigned int game_over;
  unsigned int changed;
} game_state_t;

game_state_t game_state_new();
void game_state_update(game_state_t *state, input_event_t event,
                       unsigned int elapsed_ms);
