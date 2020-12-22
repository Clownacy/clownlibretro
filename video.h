#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "SDL.h"

#include "libretro.h"

bool Video_Init(const struct retro_game_geometry *geometry, enum retro_pixel_format pixel_format);
void Video_Deinit(void);
void Video_CoreRefresh(const void *data, unsigned int width, unsigned int height, size_t pitch);
void Video_Display(void);
void Video_SetOutputSize(unsigned int width, unsigned int height);
void Video_SetFullscreen(bool fullscreen);
