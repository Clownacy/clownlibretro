#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "SDL.h"

#include "libretro.h"

typedef struct Video_Rect
{
	size_t x;
	size_t y;
	size_t width;
	size_t height;
} Video_Rect;

typedef struct Video_Texture Video_Texture;

bool Video_Init(const struct retro_game_geometry *geometry, enum retro_pixel_format pixel_format);
void Video_Deinit(void);
void Video_CoreRefresh(const void *data, unsigned int width, unsigned int height, size_t pitch);
void Video_Display(void);
void Video_SetOutputSize(unsigned int width, unsigned int height);
void Video_SetFullscreen(bool fullscreen);

Video_Texture* Video_TextureCreate(size_t width, size_t height);
void Video_TextureDestroy(Video_Texture *texture);
void Video_TextureUpdate(Video_Texture *texture, const void *pixels, size_t pitch, const Video_Rect *rect);
bool Video_TextureLock(Video_Texture *texture, const Video_Rect *rect, unsigned char **buffer, size_t *pitch);
void Video_TextureUnlock(Video_Texture *texture);
void Video_TextureDraw(Video_Texture *texture, size_t dst_x, size_t dst_y, const Video_Rect *src_rect, unsigned char red, unsigned char green, unsigned char blue);
