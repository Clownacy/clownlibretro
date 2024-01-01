#pragma once

#include <stddef.h>

#include "SDL.h"

#include "clowncommon/clowncommon.h"

typedef struct Renderer_Colour
{
	unsigned char red;
	unsigned char green;
	unsigned char blue;
} Renderer_Colour;

typedef struct Renderer_Rect
{
	size_t x;
	size_t y;
	size_t width;
	size_t height;
} Renderer_Rect;

typedef enum Renderer_Format
{
	VIDEO_FORMAT_0RGB1555 = 0,
	VIDEO_FORMAT_XRGB8888 = 1,
	VIDEO_FORMAT_RGB565 = 2,
	VIDEO_FORMAT_A8 = 3
} Renderer_Format;

#ifdef RENDERER_OPENGLES2
	#include "renderer-opengles2.h"
#else
	#include "renderer-sdl.h"
#endif

SDL_Window* Renderer_Init(const char *window_name, size_t window_width, size_t window_height, Uint32 window_flags);
void Renderer_Deinit(void);
void Renderer_Clear(void);
void Renderer_Display(void);
void Renderer_WindowResized(int width, int height);

cc_bool Renderer_TextureCreate(Renderer_Texture *texture, size_t width, size_t height, Renderer_Format format, cc_bool streaming);
void Renderer_TextureDestroy(Renderer_Texture *texture);
void Renderer_TextureUpdate(Renderer_Texture *texture, const void *pixels, const Renderer_Rect *rect);
cc_bool Renderer_TextureLock(Renderer_Texture *texture, const Renderer_Rect *rect, unsigned char **buffer, size_t *pitch);
void Renderer_TextureUnlock(Renderer_Texture *texture);
void Renderer_TextureDraw(Renderer_Texture *texture, const Renderer_Rect *dst_rect, const Renderer_Rect *src_rect, Renderer_Colour colour);

void Renderer_ColourFill(const Renderer_Rect *rect, Renderer_Colour colour, unsigned char alpha);
void Renderer_DrawLine(size_t x1, size_t y1, size_t x2, size_t y2);
