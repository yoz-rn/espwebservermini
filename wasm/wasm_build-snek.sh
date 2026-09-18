#!/bin/sh
set -xe

clang -Os -fno-builtin --target=wasm32 --no-standard-libraries \
  -Wl,--export=game_init -Wl,--export=game_seed -Wl,--export=game_update \
  -Wl,--export=game_render -Wl,--export=game_keydown \
  -Wl,--export=game_get_score -Wl,--export=game_is_over \
  -Wl,--no-entry -Wl,--allow-undefined \
  -o ../data/assets/game/snek/snek.wasm snek/snek.c