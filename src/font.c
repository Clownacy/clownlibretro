#include "font.h"

#include <stddef.h>
#ifdef FONT_STB
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef FONT_STB
	#if defined(__GNUC__) && defined(__GNUC_MINOR__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wunused-function"
	#endif

	#define STB_TRUETYPE_IMPLEMENTATION
	#include "stb_truetype.h"

	#if defined(__GNUC__) && defined(__GNUC_MINOR__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))
		#pragma GCC diagnostic pop
	#endif
#endif

static unsigned long UTF8ToUTF32(const unsigned char* const string, const unsigned char** const string_end)
{
	/* TODO - check for well-formedness */
	size_t length;
	unsigned long charcode;
	unsigned int zero_bit;
	unsigned char lead_byte;
	unsigned int i;

	for (zero_bit = 0, lead_byte = string[0]; zero_bit < 5 && (lead_byte & 0x80); ++zero_bit, lead_byte <<= 1);

	switch (zero_bit)
	{
		case 0:
			/* Single-byte character */
			length = 1;
			charcode = string[0];
			break;

		case 2:
		case 3:
		case 4:
			length = zero_bit;
			charcode = string[0] & ((1 << (8 - zero_bit)) - 1);

			for (i = 1; i < zero_bit; ++i)
			{
				if ((string[i] & 0xC0) == 0x80)
				{
					charcode <<= 6;
					charcode |= string[i] & ~0xC0;
				}
				else
				{
					/* Error: Invalid continuation byte */
					length = 1;
					charcode = 0xFFFD;
					break;
				}
			}

			break;

		default:
			/* Error: Invalid lead byte */
			length = 1;
			charcode = 0xFFFD;
			break;

	}

	if (string_end != NULL)
		*string_end = string + length;

	return charcode;
}

/*
	Generated with...
	for (unsigned int i = 0; i < 0x100; ++i)
		lookup[i] = pow(i / 255.0, 1.0 / 1.8) * 255.0;
*/

static const unsigned char gamma_correct[0x100] = {
	0x00, 0x0B, 0x11, 0x15, 0x19, 0x1C, 0x1F, 0x22, 0x25, 0x27, 0x2A, 0x2C, 0x2E, 0x30, 0x32, 0x34,
	0x36, 0x38, 0x3A, 0x3C, 0x3D, 0x3F, 0x41, 0x43, 0x44, 0x46, 0x47, 0x49, 0x4A, 0x4C, 0x4D, 0x4F,
	0x50, 0x51, 0x53, 0x54, 0x55, 0x57, 0x58, 0x59, 0x5B, 0x5C, 0x5D, 0x5E, 0x60, 0x61, 0x62, 0x63,
	0x64, 0x65, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75,
	0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x82, 0x83, 0x84, 0x84,
	0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93,
	0x94, 0x95, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0x9F, 0xA0,
	0xA1, 0xA2, 0xA3, 0xA3, 0xA4, 0xA5, 0xA6, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAA, 0xAB, 0xAC, 0xAD,
	0xAD, 0xAE, 0xAF, 0xB0, 0xB0, 0xB1, 0xB2, 0xB3, 0xB3, 0xB4, 0xB5, 0xB6, 0xB6, 0xB7, 0xB8, 0xB8,
	0xB9, 0xBA, 0xBB, 0xBB, 0xBC, 0xBD, 0xBD, 0xBE, 0xBF, 0xBF, 0xC0, 0xC1, 0xC2, 0xC2, 0xC3, 0xC4,
	0xC4, 0xC5, 0xC6, 0xC6, 0xC7, 0xC8, 0xC8, 0xC9, 0xCA, 0xCA, 0xCB, 0xCC, 0xCC, 0xCD, 0xCE, 0xCE,
	0xCF, 0xD0, 0xD0, 0xD1, 0xD2, 0xD2, 0xD3, 0xD4, 0xD4, 0xD5, 0xD6, 0xD6, 0xD7, 0xD7, 0xD8, 0xD9,
	0xD9, 0xDA, 0xDB, 0xDB, 0xDC, 0xDC, 0xDD, 0xDE, 0xDE, 0xDF, 0xE0, 0xE0, 0xE1, 0xE1, 0xE2, 0xE3,
	0xE3, 0xE4, 0xE4, 0xE5, 0xE6, 0xE6, 0xE7, 0xE7, 0xE8, 0xE9, 0xE9, 0xEA, 0xEA, 0xEB, 0xEC, 0xEC,
	0xED, 0xED, 0xEE, 0xEF, 0xEF, 0xF0, 0xF0, 0xF1, 0xF1, 0xF2, 0xF3, 0xF3, 0xF4, 0xF4, 0xF5, 0xF5,
	0xF6, 0xF7, 0xF7, 0xF8, 0xF8, 0xF9, 0xF9, 0xFA, 0xFB, 0xFB, 0xFC, 0xFC, 0xFD, 0xFD, 0xFE, 0xFF
};

static Font_Glyph* GetGlyph(Font* const font, const unsigned long unicode_value, const Font_Callbacks* const callbacks)
{
    Font_Glyph **glyph_pointer = &font->glyph_list_head;
    Font_Glyph *glyph;
	unsigned char *pixels;
	long pitch;

	/* See if we have the glyph cached. */
	for (;;)
	{
		glyph = *glyph_pointer;

		if (glyph->unicode_value == unicode_value)
		{
			/* Move it to the front of the list */
			*glyph_pointer = glyph->next;
			glyph->next = font->glyph_list_head;
			font->glyph_list_head = glyph;

			return glyph;
		}

		if (glyph->next == NULL)
			break;

		glyph_pointer = &glyph->next;
	}

	/* Couldn't find glyph - overwrite the old one at the end of the cache,
	   as it hasn't been used in a while anyway. */

#ifdef FONT_STB
	{
		{
			int width, height;
			int x_advance, x_offset, y_offset;
			int ascent;

			pixels = stbtt_GetCodepointBitmap(&font->stb, font->x_scale, font->y_scale, unicode_value, &width, &height, NULL, NULL);
			pitch = width;

			stbtt_GetCodepointHMetrics(&font->stb, unicode_value, &x_advance, NULL);
			stbtt_GetCodepointBox(&font->stb, unicode_value, &x_offset, NULL, NULL, &y_offset);
			stbtt_GetFontVMetrics(&font->stb, &ascent, NULL, NULL);

			if (pixels == NULL)
			{
				glyph->rect.width = 0;
				glyph->rect.height = 0;
			}
			else
			{
				glyph->rect.width = width;
				glyph->rect.height = height;
			}
			glyph->x_offset = x_offset * font->x_scale;
			glyph->y_offset = ceil((ascent - y_offset) * font->y_scale);
			glyph->x_advance = x_advance * font->x_scale;
#else
	if (FT_Load_Glyph(font->face, FT_Get_Char_Index(font->face, unicode_value), FT_LOAD_RENDER | (font->antialiasing ? 0 : FT_LOAD_TARGET_MONO)) != 0)
	{
		glyph = NULL;
	}
	else
	{
		FT_Bitmap bitmap;
		FT_Bitmap_New(&bitmap);

		if (FT_Bitmap_Convert(font->library, &font->face->glyph->bitmap, &bitmap, 1) != 0)
		{
			glyph = NULL;
		}
		else
		{
			unsigned int x, y;

			switch (font->face->glyph->bitmap.pixel_mode)
			{
				case FT_PIXEL_MODE_GRAY:
					for (y = 0; y < bitmap.rows; ++y)
					{
						unsigned char *pixel_pointer = &bitmap.buffer[y * bitmap.pitch];

						for (x = 0; x < bitmap.width; ++x)
						{
							*pixel_pointer = (*pixel_pointer * 0xFF) / (bitmap.num_grays - 1);
							++pixel_pointer;
						}
					}

					break;

				case FT_PIXEL_MODE_MONO:
					for (y = 0; y < bitmap.rows; ++y)
					{
						unsigned char *pixel_pointer = &bitmap.buffer[y * bitmap.pitch];

						for (x = 0; x < bitmap.width; ++x)
						{
							*pixel_pointer = *pixel_pointer ? 0xFF : 0;
							++pixel_pointer;
						}
					}

					break;
			}

			glyph->rect.width = bitmap.width;
			glyph->rect.height = bitmap.rows;
			glyph->x_offset = font->face->glyph->bitmap_left;
			glyph->y_offset = CC_DIVIDE_CEILING(font->face->size->metrics.ascender, 64) - font->face->glyph->bitmap_top;
			glyph->x_advance = font->face->glyph->advance.x / 64;

			pixels = bitmap.buffer;
			pitch = bitmap.pitch;
#endif

			glyph->unicode_value = unicode_value;
			glyph->rect.width = glyph->rect.width == 0 ? 0 : glyph->rect.width + font->shadow_thickness * 2;
			glyph->rect.height = glyph->rect.height == 0 ? 0 : glyph->rect.height + font->shadow_thickness * 2;
			glyph->x_offset -= font->shadow_thickness;
			glyph->y_offset -= font->shadow_thickness;

			if (pixels != NULL && glyph->rect.width != 0 && glyph->rect.height != 0)	/* Some glyphs are just plain empty - don't bother trying to upload them */
			{
				unsigned char* const rgba_pixels = (unsigned char*)malloc(glyph->rect.width * glyph->rect.height * 2);

				if (rgba_pixels != NULL)
				{
					long x, y;
					unsigned char *rgba_pointer;

					rgba_pointer = rgba_pixels;

					/* Gamma-correct the glyph bitmap. This is needed so that the shadow is applied evenly around glyphs.
					   The shadow around the '"' glyph looks wrong without this. */
					for (y = 0; y < glyph->rect.height - font->shadow_thickness * 2; ++y)
					{
						unsigned char *pixel_pointer = &pixels[y * pitch];

						for (x = 0; x < glyph->rect.width - font->shadow_thickness * 2; ++x)
						{
							*pixel_pointer = gamma_correct[*pixel_pointer];
							++pixel_pointer;
						}
					}

					/* Create the shadow bitmap. */
					for (y = 0; y < glyph->rect.height; ++y)
					{
						for (x = 0; x < glyph->rect.width; ++x)
						{
							long sample;
							long sample_y;

							sample = 0;

							for (sample_y = -font->shadow_thickness; sample_y <= font->shadow_thickness; ++sample_y)
							{
								long sample_x;

								for (sample_x = -font->shadow_thickness; sample_x <= font->shadow_thickness; ++sample_x)
								{
									const long final_sample_x = -font->shadow_thickness + x + sample_x;
									const long final_sample_y = -font->shadow_thickness + y + sample_y;

									if (final_sample_x >= 0 && final_sample_x < glyph->rect.width - font->shadow_thickness * 2 && final_sample_y >= 0 && final_sample_y < glyph->rect.height - font->shadow_thickness * 2)
									{
										const double distance = CC_MAX(0.0, sqrt(sample_x * sample_x + sample_y * sample_y) - 1.0);
										sample = CC_MAX(sample, pixels[final_sample_y * pitch + final_sample_x] - distance * (font->shadow_thickness == 0 ? 0 : ((double)0xFF / font->shadow_thickness)));
									}
								}
							}

							*rgba_pointer++ = sample;
						}
					}

					memset(&rgba_pixels[glyph->rect.width * glyph->rect.height], 0, glyph->rect.width * glyph->rect.height);

					/* Create the glyph bitmap. */
					for (y = 0; y < glyph->rect.height - font->shadow_thickness * 2; ++y)
					{
						unsigned char *pixels_pointer;
						long x;

						pixels_pointer = &pixels[y * pitch];
						rgba_pointer = &rgba_pixels[(glyph->rect.height + y + font->shadow_thickness) * glyph->rect.width + font->shadow_thickness];

						for (x = 0; x < glyph->rect.width - font->shadow_thickness * 2; ++x)
						{
							const unsigned char pixel = *pixels_pointer++;

							*rgba_pointer++ = pixel;
						}
					}

					callbacks->update_texture(rgba_pixels, &glyph->rect, (void*)callbacks->user_data);
					free(rgba_pixels);
				}
			}

#ifdef FONT_STB
			stbtt_FreeBitmap(pixels, NULL);
#else
			FT_Bitmap_Done(font->library, &bitmap);
#endif

			*glyph_pointer = glyph->next;
			glyph->next = font->glyph_list_head;
			font->glyph_list_head = glyph;
		}
	}

	return glyph;
}

static cc_bool LoadFontCommon(Font* const font, const size_t cell_width, const size_t cell_height, const cc_bool antialiasing, const size_t shadow_thickness, const Font_Callbacks* const callbacks)
{
	size_t atlas_entry_width, atlas_entry_height;
	size_t atlas_columns, atlas_rows;

#ifdef FONT_STB
	int x0, y0, x1, y1;

	font->x_scale = stbtt_ScaleForPixelHeight(&font->stb, cell_width);
	font->y_scale = stbtt_ScaleForPixelHeight(&font->stb, cell_height);

	stbtt_GetFontBoundingBox(&font->stb, &x0, &y0, &x1, &y1);

	atlas_entry_width = (x1 - x0) * font->x_scale;
	atlas_entry_height = (y1 - y0) * font->y_scale;
#else
	FT_Int i;

	/* Select a rough size, for vector glyphs */
	FT_Size_RequestRec request;
	request.type = FT_SIZE_REQUEST_TYPE_REAL_DIM;
	request.width = cell_width << 6;
	request.height = cell_height << 6;
	request.horiResolution = 0;
	request.vertResolution = 0;
	FT_Request_Size(font->face, &request);

	/* If an accurate bitmap font is available, however, use that instead */
	for (i = 0; i < font->face->num_fixed_sizes; ++i)
	{
		if ((size_t)font->face->available_sizes[i].width == cell_width && (size_t)font->face->available_sizes[i].height == cell_height)
		{
			FT_Select_Size(font->face, i);
			break;
		}
	}

	atlas_entry_width = FT_MulFix(font->face->bbox.xMax - font->face->bbox.xMin + 1, font->face->size->metrics.x_scale) / 64;
	atlas_entry_height = FT_MulFix(font->face->bbox.yMax - font->face->bbox.yMin + 1, font->face->size->metrics.y_scale) / 64;
#endif

	atlas_entry_width += shadow_thickness * 2;
	atlas_entry_height += shadow_thickness * 2;

	atlas_columns = ceil(sqrt(atlas_entry_width * atlas_entry_height * TOTAL_GLYPH_SLOTS) / atlas_entry_width);
	atlas_rows = (TOTAL_GLYPH_SLOTS + (atlas_columns - 1)) / atlas_columns;

	if (callbacks->create_texture(atlas_columns * atlas_entry_width, atlas_rows * atlas_entry_height, (void*)callbacks->user_data))
	{
		size_t i;

		/* Initialise the linked-list */
		for (i = 0; i < TOTAL_GLYPH_SLOTS; ++i)
		{
			font->glyphs[i].next = (i == 0) ? NULL : &font->glyphs[i - 1];

			font->glyphs[i].rect.x = (i % atlas_columns) * atlas_entry_width;
			font->glyphs[i].rect.y = (i / atlas_columns) * atlas_entry_height;

			font->glyphs[i].unicode_value = 0;
		}

		font->glyph_list_head = &font->glyphs[TOTAL_GLYPH_SLOTS - 1];

		font->antialiasing = antialiasing;
		font->shadow_thickness = shadow_thickness;

		return cc_true;
	}

	return cc_false;
}

cc_bool Font_LoadFromMemory(Font* const font, const unsigned char* const data, const size_t data_size, const size_t cell_width, const size_t cell_height, const cc_bool antialiasing, const size_t shadow_thickness, const Font_Callbacks* const callbacks)
{
#ifdef FONT_STB
	(void)data_size;
	font->ttf_file_buffer = NULL;
	stbtt_InitFont(&font->stb, data, stbtt_GetFontOffsetForIndex(data, 0));
	return LoadFontCommon(font, cell_width, cell_height, antialiasing, shadow_thickness, callbacks);
#else
	if (FT_Init_FreeType(&font->library) == 0)
	{
		if (FT_New_Memory_Face(font->library, data, (FT_Long)data_size, 0, &font->face) == 0)
		{
			if (LoadFontCommon(font, cell_width, cell_height, antialiasing, shadow_thickness, callbacks))
				return cc_true;

			FT_Done_Face(font->face);
		}

		FT_Done_FreeType(font->library);
	}

	return cc_false;
#endif
}

cc_bool Font_LoadFromFile(Font* const font, const char* const font_filename, const size_t cell_width, const size_t cell_height, const cc_bool antialiasing, const size_t shadow_thickness, const Font_Callbacks* const callbacks)
{
#ifdef FONT_STB
	FILE* const file = fopen(font_filename, "rb");

	if (file != NULL)
	{
		size_t file_size;
		unsigned char *file_buffer;

		fseek(file, 0, SEEK_END);
		file_size = ftell(file);
		rewind(file);
		file_buffer = (unsigned char*)malloc(file_size);

		if (file_buffer != NULL)
			fread(file_buffer, file_size, 1, file);

		fclose(file);

		if (file_buffer != NULL)
		{
			const cc_bool success = Font_LoadFromMemory(font, file_buffer, file_size, cell_width, cell_height, antialiasing, shadow_thickness, callbacks);
			font->ttf_file_buffer = file_buffer;
			return success;
		}
	}
#else
	if (FT_Init_FreeType(&font->library) == 0)
	{
		if (FT_New_Face(font->library, font_filename, 0, &font->face) == 0)
		{
			if (LoadFontCommon(font, cell_width, cell_height, antialiasing, shadow_thickness, callbacks))
				return cc_true;

			FT_Done_Face(font->face);
		}

		FT_Done_FreeType(font->library);
	}
#endif

	return cc_false;
}

void Font_Unload(Font* const font, const Font_Callbacks* const callbacks)
{
	callbacks->destroy_texture((void*)callbacks->user_data);

#ifdef FONT_STB
	free(font->ttf_file_buffer);
#else
	FT_Done_Face(font->face);
	FT_Done_FreeType(font->library);
#endif
}

static void IterateGlyphs(Font* const font, const char* const string, const size_t string_length, const Font_Callbacks* const callbacks, void (* const callback)(const Font_Glyph *glyph, void *user_data), const void* const user_data)
{
	const unsigned char *string_pointer = (const unsigned char*)string;
	const unsigned char *string_end = (const unsigned char*)string + string_length;

	while (string_pointer != string_end)
	{
		Font_Glyph* const glyph = GetGlyph(font, UTF8ToUTF32(string_pointer, &string_pointer), callbacks);

		if (glyph != NULL)
			callback(glyph, (void*)user_data);
	}
}

typedef struct DrawTextCallbackData
{
	Font *font;
	long x;
	long y;
	const Font_Colour *colour;
	const Font_Callbacks *callbacks;

	size_t pen_x;
	cc_bool do_shadow;
} DrawTextCallbackData;

static void DrawTextCallback(const Font_Glyph* const glyph, void* const user_data)
{
	DrawTextCallbackData* const data = (DrawTextCallbackData*)user_data;

	if (glyph->rect.width != 0 && glyph->rect.height != 0)	/* Some glyphs are just plain empty - don't bother trying to draw them */
	{
		Font_Rect dst_rect;

		const long letter_x = data->x + data->pen_x + glyph->x_offset;
		const long letter_y = data->y + glyph->y_offset;

		dst_rect.x = letter_x;
		dst_rect.y = letter_y;
		dst_rect.width = glyph->rect.width;
		dst_rect.height = glyph->rect.height;

		data->callbacks->draw_texture(&dst_rect, &glyph->rect, data->colour, data->do_shadow, (void*)data->callbacks->user_data);
	}

	data->pen_x += glyph->x_advance;
}

void Font_DrawText(Font* const font, const long x, const long y, const Font_Colour* const colour, const char* const string, const size_t string_length, const Font_Callbacks* const callbacks)
{
	DrawTextCallbackData data;

	data.font = font;
	data.x = x;
	data.y = y;
	data.colour = colour;
	data.callbacks = callbacks;

	if (font->shadow_thickness != 0)
	{
		/* On the first pass, draw the shadow. */
		data.pen_x = 0;
		data.do_shadow = cc_true;
		IterateGlyphs(font, string, string_length, callbacks, DrawTextCallback, &data);
	}

	/* On the second pass, draw the actual glyph. */
	data.pen_x = 0;
	data.do_shadow = cc_false;
	IterateGlyphs(font, string, string_length, callbacks, DrawTextCallback, &data);
}

static void GetTextWidthCallback(const Font_Glyph* const glyph, void* const user_data)
{
	size_t* const width = (size_t*)user_data;

	*width += glyph->x_advance;
}

size_t Font_GetTextWidth(Font* const font, const char* const string, const size_t string_length, const Font_Callbacks* const callbacks)
{
	size_t width = 0;

	IterateGlyphs(font, string, string_length, callbacks, GetTextWidthCallback, &width);

	return width;
}
