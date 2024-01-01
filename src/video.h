#pragma once

#include <stddef.h>

#include "clowncommon/clowncommon.h"

#include "renderer.h"

typedef Renderer_Colour Video_Colour;
typedef Renderer_Rect Video_Rect;
typedef Renderer_Format Video_Format;
typedef Renderer_Texture Video_Texture;

extern size_t window_width;
extern size_t window_height;

cc_bool Video_Init(size_t window_width, size_t window_height);
void Video_Deinit(void);
void Video_Clear(void);
void Video_Display(void);
void Video_SetFullscreen(cc_bool fullscreen);
void Video_WindowResized(void);
float Video_GetDPIScale(void);

#define Video_Clear Renderer_Clear
#define Video_Display Renderer_Display

#define Video_TextureCreate Renderer_TextureCreate
#define Video_TextureDestroy Renderer_TextureDestroy
#define Video_TextureUpdate Renderer_TextureUpdate
#define Video_TextureLock Renderer_TextureLock
#define Video_TextureUnlock Renderer_TextureUnlock
#define Video_TextureDraw Renderer_TextureDraw
#define Video_ColourFill Renderer_ColourFill
#define Video_DrawLine Renderer_DrawLine
