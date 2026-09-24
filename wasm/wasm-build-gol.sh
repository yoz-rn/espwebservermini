#!/bin/sh
set -xe

clang -Os -fno-builtin --target=wasm32 --no-standard-libraries \
  -Wl,--export=game_init -Wl,--export=game_seed -Wl,--export=game_update \
  -Wl,--export=game_render -Wl,--export=game_get_score -Wl,--export=game_is_over \
  -Wl,--export=game_toggle_cell -Wl,--export=game_set_paused \
  -Wl,--export=game_randomize -Wl,--export=game_set_interval \
  -Wl,--no-entry -Wl,--allow-undefined \
  -o ../data/webapp/assets/game/gol/gol.wasm gol/gol.c