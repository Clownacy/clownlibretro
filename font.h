#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "video.h"

typedef struct Font Font;

#ifdef FREETYPE_FONTS
Font* LoadFreeTypeFontFromData(const unsigned char *data, size_t data_size, size_t cell_width, size_t cell_height, bool antialiasing);
Font* LoadFreeTypeFont(const char *font_filename, size_t cell_width, size_t cell_height, bool antialiasing);
#else
Font* LoadBitmapFont(const char *bitmap_path, const char *metadata_path);
#endif

void DrawText(Font *font, /*RenderBackend_Surface*/Video_Texture *surface, int x, int y, unsigned long colour, const char *string);
void UnloadFont(Font *font);
