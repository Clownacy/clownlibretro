#ifndef FONT_H
#define FONT_H

#include <stddef.h>

#ifdef FONT_STB
	#if defined(__GNUC__) && defined(__GNUC_MINOR__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wunused-function"
	#endif

	#define STBTT_STATIC
	#include "stb_truetype.h"

	#if defined(__GNUC__) && defined(__GNUC_MINOR__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))
		#pragma GCC diagnostic pop
	#endif
#else
	#include <ft2build.h>
	#include FT_FREETYPE_H
	#include FT_BITMAP_H
#endif

#include "clowncommon/clowncommon.h"

/* This controls however many glyphs (letters) the game can cache in VRAM at once */
#define TOTAL_GLYPH_SLOTS 0x40

typedef struct Font_Colour
{
	unsigned char red;
	unsigned char green;
	unsigned char blue;
} Font_Colour;

typedef struct Font_Rect
{
	long x;
	long y;
	long width;
	long height;
} Font_Rect;

typedef struct Font_Glyph
{
	unsigned long unicode_value;

	Font_Rect rect;

	long x_offset;
	long y_offset;

	long x_advance;

	struct Font_Glyph *next;
} Font_Glyph;

typedef struct Font
{
#ifdef FONT_STB
	unsigned char *ttf_file_buffer;
	stbtt_fontinfo stb;
	float x_scale, y_scale;
#else
	FT_Library library;
	FT_Face face;
#endif
	cc_bool antialiasing;
	long shadow_thickness;
	Font_Glyph glyphs[TOTAL_GLYPH_SLOTS];
	Font_Glyph *glyph_list_head;
} Font;

typedef struct Font_Callbacks
{
	const void *user_data;

	cc_bool (*create_texture)(size_t width, size_t height, void *user_data);
	void (*destroy_texture)(void *user_data);
	void (*update_texture)(const unsigned char *pixels, const Font_Rect *rect, void *user_data);
	void (*draw_texture)(const Font_Rect *dst_rect, const Font_Rect *src_rect, const Font_Colour *colour, cc_bool do_shadow, void *user_data);
} Font_Callbacks;

cc_bool Font_LoadFromMemory(Font *font, const unsigned char *data, size_t data_size, size_t cell_width, size_t cell_height, cc_bool antialiasing, size_t shadow_thickness, const Font_Callbacks *callbacks);
cc_bool Font_LoadFromFile(Font *font, const char *font_filename, size_t cell_width, size_t cell_height, cc_bool antialiasing, size_t shadow_thickness, const Font_Callbacks *callbacks);
void Font_Unload(Font *font, const Font_Callbacks *callbacks);

void Font_DrawText(Font *font, long x, long y, const Font_Colour *colour, const char *string, size_t string_length, const Font_Callbacks *callbacks);
size_t Font_GetTextWidth(Font *font, const char *string, size_t string_length, const Font_Callbacks *callbacks);
#define Font_DrawTextCentred(font, x, y, colour, string, string_length, callbacks) Font_DrawText(font, (x) - Font_GetTextWidth(font, string, string_length, callbacks) / 2, y, colour, string, string_length, callbacks)

#endif /* FONT_H */
