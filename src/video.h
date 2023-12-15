#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "SDL.h"

typedef struct Video_Colour
{
	unsigned char red;
	unsigned char green;
	unsigned char blue;
} Video_Colour;

typedef struct Video_Rect
{
	size_t x;
	size_t y;
	size_t width;
	size_t height;
} Video_Rect;

typedef enum Video_Format
{
	VIDEO_FORMAT_0RGB1555 = 0,
	VIDEO_FORMAT_XRGB8888 = 1,
	VIDEO_FORMAT_RGB565 = 2,
	VIDEO_FORMAT_A8 = 3
} Video_Format;

typedef struct Video_Texture Video_Texture;

extern size_t window_width;
extern size_t window_height;

bool Video_Init(size_t window_width, size_t window_height);
void Video_Deinit(void);
void Video_Clear(void);
void Video_Display(void);
void Video_SetFullscreen(bool fullscreen);
void Video_WindowResized(void);

Video_Texture* Video_TextureCreate(size_t width, size_t height, Video_Format format, bool streaming);
void Video_TextureDestroy(Video_Texture *texture);
void Video_TextureUpdate(Video_Texture *texture, const void *pixels, size_t pitch, const Video_Rect *rect);
bool Video_TextureLock(Video_Texture *texture, const Video_Rect *rect, unsigned char **buffer, size_t *pitch);
void Video_TextureUnlock(Video_Texture *texture);
void Video_TextureDraw(Video_Texture *texture, const Video_Rect *dst_rect, const Video_Rect *src_rect, Video_Colour colour);

void Video_ColourFill(const Video_Rect *rect, Video_Colour colour, unsigned char);
void Video_DrawLine(size_t x1, size_t y1, size_t x2, size_t y2);
