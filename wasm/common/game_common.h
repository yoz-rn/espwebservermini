#pragma once

typedef unsigned char u8;
typedef unsigned int u32;
typedef int i32;
typedef float f32;

// Diimpor dari JS saat instantiate — TIDAK didefinisikan di file .c manapun
void platform_fill_rect(i32 x, i32 y, i32 w, i32 h, u32 color);