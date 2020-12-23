#include "font.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef FREETYPE_FONTS
	#include <ft2build.h>
	#include FT_FREETYPE_H
	#include FT_BITMAP_H
#endif

//#include "Bitmap.h"
#include "file.h"
#include "video.h"

// This controls however many glyphs (letters) the game can cache in VRAM at once
#define TOTAL_GLYPH_SLOTS 256

typedef struct Glyph
{
	unsigned long unicode_value;

	size_t x;
	size_t y;

	size_t width;
	size_t height;

	size_t x_offset;
	size_t y_offset;

	size_t x_advance;

	struct Glyph *next;
} Glyph;

struct Font
{
#ifdef FREETYPE_FONTS
	FT_Library library;
	FT_Face face;
	unsigned char *data;
	bool antialiasing;
#else
	unsigned char *image_buffer;
	size_t image_buffer_width;
	size_t image_buffer_height;
	size_t glyph_slot_width;
	size_t glyph_slot_height;
	size_t total_local_glyphs;
	Glyph *local_glyphs;
#endif
	Glyph glyphs[TOTAL_GLYPH_SLOTS];
	Glyph *glyph_list_head;
	Video_Texture *atlas;
};

size_t font_width;
size_t font_height;

static unsigned long UTF8ToUTF32(const unsigned char *string, size_t *bytes_read)
{
	// TODO - check for well-formedness
	size_t length;
	unsigned long charcode;

	unsigned int zero_bit = 0;
	for (unsigned char lead_byte = string[0]; zero_bit < 5 && (lead_byte & 0x80); ++zero_bit, lead_byte <<= 1);

	switch (zero_bit)
	{
		case 0:
			// Single-byte character
			length = 1;
			charcode = string[0];
			break;

		case 2:
		case 3:
		case 4:
			length = zero_bit;
			charcode = string[0] & ((1 << (8 - zero_bit)) - 1);

			for (unsigned int i = 1; i < zero_bit; ++i)
			{
				if ((string[i] & 0xC0) == 0x80)
				{
					charcode <<= 6;
					charcode |= string[i] & ~0xC0;
				}
				else
				{
					// Error: Invalid continuation byte
					length = 1;
					charcode = 0xFFFD;
					break;
				}
			}

			break;

		default:
			// Error: Invalid lead byte
			length = 1;
			charcode = 0xFFFD;
			break;

	}

	if (bytes_read)
		*bytes_read = length;

	return charcode;
}

static unsigned char GammaCorrect(unsigned char value)
{
	/*
		Generated with...
		for (unsigned int i = 0; i < 0x100; ++i)
			lookup[i] = pow(i / 255.0, 1.0 / 1.8) * 255.0;
	*/

	const unsigned char lookup[0x100] = {
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

	return lookup[value];
}

static Glyph* GetGlyph(Font *font, unsigned long unicode_value)
{
	Glyph **glyph_pointer = &font->glyph_list_head;
	Glyph *glyph;

	for (;;)
	{
		glyph = *glyph_pointer;

		if (glyph->unicode_value == unicode_value)
		{
			// Move it to the front of the list
			*glyph_pointer = glyph->next;
			glyph->next = font->glyph_list_head;
			font->glyph_list_head = glyph;

			return glyph;
		}

		if (glyph->next == NULL)
			break;

		glyph_pointer = &glyph->next;
	}

	// Couldn't find glyph - overwrite the old one at the end.
	// The one at the end hasn't been used in a while anyway.

#ifdef FREETYPE_FONTS
	unsigned int glyph_index = FT_Get_Char_Index(font->face, unicode_value);

	if (FT_Load_Glyph(font->face, glyph_index, FT_LOAD_RENDER | (font->antialiasing ? 0 : FT_LOAD_TARGET_MONO)) == 0)
	{
		FT_Bitmap bitmap;
		FT_Bitmap_New(&bitmap);

		if (FT_Bitmap_Convert(font->library, &font->face->glyph->bitmap, &bitmap, 1) == 0)
		{
			switch (font->face->glyph->bitmap.pixel_mode)
			{
				case FT_PIXEL_MODE_GRAY:
					for (size_t y = 0; y < bitmap.rows; ++y)
					{
						unsigned char *pixel_pointer = &bitmap.buffer[y * bitmap.pitch];

						for (size_t x = 0; x < bitmap.width; ++x)
						{
							*pixel_pointer = GammaCorrect((*pixel_pointer * 0xFF) / (bitmap.num_grays - 1));
							++pixel_pointer;
						}
					}

					break;

				case FT_PIXEL_MODE_MONO:
					for (size_t y = 0; y < bitmap.rows; ++y)
					{
						unsigned char *pixel_pointer = &bitmap.buffer[y * bitmap.pitch];

						for (size_t x = 0; x < bitmap.width; ++x)
						{
							*pixel_pointer = *pixel_pointer ? 0xFF : 0;
							++pixel_pointer;
						}
					}

					break;
			}

			glyph->unicode_value = unicode_value;
			glyph->width = bitmap.width;
			glyph->height = bitmap.rows;
			glyph->x_offset = font->face->glyph->bitmap_left;
			glyph->y_offset = (font->face->size->metrics.ascender + (64 / 2)) / 64 - font->face->glyph->bitmap_top;
			glyph->x_advance = font->face->glyph->advance.x / 64;

			if (glyph->width != 0 && glyph->height != 0)	// Some glyphs are just plain empty - don't bother trying to upload them
			{
				Video_Rect rect = {glyph->x, glyph->y, glyph->width, glyph->height};
				Video_TextureUpdate(font->atlas, bitmap.buffer, glyph->width, &rect);
			}

			FT_Bitmap_Done(font->library, &bitmap);

			*glyph_pointer = glyph->next;
			glyph->next = font->glyph_list_head;
			font->glyph_list_head = glyph;

			return glyph;
		}

		FT_Bitmap_Done(font->library, &bitmap);
	}
#else
	// Perform a binary search for the glyph
	size_t left = 0;
	size_t right = font->total_local_glyphs;

	while (right - left >= 2)
	{
		const size_t index = (left + right) / 2;

		if (font->local_glyphs[index].unicode_value > unicode_value)
			right = index;
		else
			left = index;
	}

	if (font->local_glyphs[left].unicode_value == unicode_value)
	{
		const Glyph *local_glyph = &font->local_glyphs[left];

		glyph->unicode_value = local_glyph->unicode_value;
		glyph->width = font->glyph_slot_width;
		glyph->height = font->glyph_slot_height;
		glyph->x_offset = 0;
		glyph->y_offset = 0;
		glyph->x_advance = local_glyph->x_advance;

		if (glyph->width != 0 && glyph->height != 0)	// Some glyphs are just plain empty - don't bother trying to upload them
		{
			Video_Rect rect = {glyph->x, glyph->y, glyph->width, glyph->height};
			Video_TextureUpdate(font->atlas, &font->image_buffer[local_glyph->y * font->image_buffer_width + local_glyph->x], font->image_buffer_width, &rect);
		}

		*glyph_pointer = glyph->next;
		glyph->next = font->glyph_list_head;
		font->glyph_list_head = glyph;

		return glyph;
	}
#endif

	return NULL;
}

#ifdef FREETYPE_FONTS
Font* LoadFreeTypeFontFromData(const unsigned char *data, size_t data_size, size_t cell_width, size_t cell_height, bool antialiasing)
{
	Font *font = (Font*)malloc(sizeof(Font));

	if (font != NULL)
	{
		if (FT_Init_FreeType(&font->library) == 0)
		{
			font->data = (unsigned char*)malloc(data_size);

			if (font->data != NULL)
			{
				memcpy(font->data, data, data_size);

				if (FT_New_Memory_Face(font->library, font->data, (FT_Long)data_size, 0, &font->face) == 0)
				{
					// Select a rough size, for vector glyphs
					FT_Size_RequestRec request;
					request.type = FT_SIZE_REQUEST_TYPE_CELL;
					request.width = cell_height << 6;	// 'cell_height << 6' isn't a typo: it's needed by the Japanese font
					request.height = cell_height << 6;
					request.horiResolution = 0;
					request.vertResolution = 0;
					FT_Request_Size(font->face, &request);

					// If an accurate bitmap font is available, however, use that instead
					for (FT_Int i = 0; i < font->face->num_fixed_sizes; ++i)
					{
						if ((size_t)font->face->available_sizes[i].width == cell_width && (size_t)font->face->available_sizes[i].height == cell_height)
						{
							FT_Select_Size(font->face, i);
							break;
						}
					}

					font->glyph_list_head = NULL;

					size_t atlas_entry_width = FT_MulFix(font->face->bbox.xMax - font->face->bbox.xMin + 1, font->face->size->metrics.x_scale) / 64;
					size_t atlas_entry_height = FT_MulFix(font->face->bbox.yMax - font->face->bbox.yMin + 1, font->face->size->metrics.y_scale) / 64;

					size_t atlas_columns = ceil(sqrt(atlas_entry_width * atlas_entry_height * TOTAL_GLYPH_SLOTS) / atlas_entry_width);
					size_t atlas_rows = (TOTAL_GLYPH_SLOTS + (atlas_columns - 1)) / atlas_columns;

					font->atlas = Video_TextureCreate(atlas_columns * atlas_entry_width, atlas_rows * atlas_entry_height, VIDEO_FORMAT_A8, false);

					if (font->atlas != NULL)
					{
						// Initialise the linked-list
						for (size_t i = 0; i < TOTAL_GLYPH_SLOTS; ++i)
						{
							font->glyphs[i].next = (i == 0) ? NULL : &font->glyphs[i - 1];

							font->glyphs[i].x = (i % atlas_columns) * atlas_entry_width;
							font->glyphs[i].y = (i / atlas_columns) * atlas_entry_height;

							font->glyphs[i].unicode_value = 0;
						}

						font->glyph_list_head = &font->glyphs[TOTAL_GLYPH_SLOTS - 1];

						font->antialiasing = antialiasing;

						return font;
					}

					FT_Done_Face(font->face);
				}

				free(font->data);
			}

			FT_Done_FreeType(font->library);
		}

		free(font);
	}

	return NULL;
}

Font* LoadFreeTypeFont(const char *font_filename, size_t cell_width, size_t cell_height, bool antialiasing)
{
	font_width = cell_width;
	font_height = cell_height;

	Font *font = NULL;

	size_t file_size;
	unsigned char *file_buffer;

	if (FileToMemory(font_filename, &file_buffer, &file_size))
	{
		font = LoadFreeTypeFontFromData(file_buffer, file_size, cell_width, cell_height, antialiasing);
		free(file_buffer);
	}

	return font;
}
#else
Font* LoadBitmapFont(const char *bitmap_path, const char *metadata_path)
{
	size_t bitmap_width, bitmap_height;
	unsigned char *image_buffer = DecodeBitmapFromFile(bitmap_path, &bitmap_width, &bitmap_height, 1);

	if (image_buffer != NULL)
	{
		size_t metadata_size;
		unsigned char *metadata_buffer;

		if (FileToMemory(metadata_path, &metadata_buffer, &metadata_size))
		{
			Font *font = (Font*)malloc(sizeof(Font));

			if (font != NULL)
			{
				font->glyph_slot_width = (metadata_buffer[0] << 8) | metadata_buffer[1];
				font->glyph_slot_height = (metadata_buffer[2] << 8) | metadata_buffer[3];
				font->total_local_glyphs = (metadata_buffer[4] << 8) | metadata_buffer[5];

				font->local_glyphs = (Glyph*)malloc(sizeof(Glyph) * font->total_local_glyphs);

				if (font->local_glyphs != NULL)
				{
					for (size_t i = 0; i < font->total_local_glyphs; ++i)
					{
						font->local_glyphs[i].unicode_value = (metadata_buffer[6 + i * 4 + 0] << 8) | metadata_buffer[6 + i * 4 + 1];

						font->local_glyphs[i].x = (i % (bitmap_width / font->glyph_slot_width)) * font->glyph_slot_width;
						font->local_glyphs[i].y = (i / (bitmap_width / font->glyph_slot_width)) * font->glyph_slot_height;

						font->local_glyphs[i].width = font->glyph_slot_width;
						font->local_glyphs[i].height = font->glyph_slot_height;

						font->local_glyphs[i].x_offset = 0;
						font->local_glyphs[i].y_offset = 0;

						font->local_glyphs[i].x_advance = (metadata_buffer[6 + i * 4 + 2] << 8) | metadata_buffer[6 + i * 4 + 3];

						font->local_glyphs[i].next = NULL;
					}

					size_t atlas_entry_width = font->glyph_slot_width;
					size_t atlas_entry_height = font->glyph_slot_height;

					size_t atlas_columns = ceil(sqrt(atlas_entry_width * atlas_entry_height * TOTAL_GLYPH_SLOTS) / atlas_entry_width);
					size_t atlas_rows = (TOTAL_GLYPH_SLOTS + (atlas_columns - 1)) / atlas_columns;

					font->atlas = Video_TextureCreate(atlas_columns * atlas_entry_width, atlas_rows * atlas_entry_height);

					if (font->atlas != NULL)
					{
						// Initialise the linked-list
						for (size_t i = 0; i < TOTAL_GLYPH_SLOTS; ++i)
						{
							font->glyphs[i].next = (i == 0) ? NULL : &font->glyphs[i - 1];

							font->glyphs[i].x = (i % atlas_columns) * atlas_entry_width;
							font->glyphs[i].y = (i / atlas_columns) * atlas_entry_height;

							font->glyphs[i].unicode_value = 0;
						}

						font->glyph_list_head = &font->glyphs[TOTAL_GLYPH_SLOTS - 1];

						font->image_buffer = image_buffer;
						font->image_buffer_width = bitmap_width;
						font->image_buffer_height = bitmap_height;

						free(metadata_buffer);

						return font;
					}
				}

				free(font);
			}

			free(metadata_buffer);
		}

		FreeBitmap(image_buffer);
	}

	return NULL;
}
#endif

void DrawText(Font *font, Video_Texture *surface, int x, int y, unsigned long colour, const char *string)
{
//	if (font != NULL && surface != NULL)
	if (font != NULL)
	{
//		RenderBackend_PrepareToDrawGlyphs(font->atlas, surface, colour & 0xFF, (colour >> 8) & 0xFF, (colour >> 16) & 0xFF);

		size_t pen_x = 0;

		const unsigned char *string_pointer = (const unsigned char*)string;
		const unsigned char *string_end = (const unsigned char*)string + strlen(string);

		while (string_pointer != string_end)
		{
			size_t bytes_read;
			const unsigned long unicode_value = UTF8ToUTF32(string_pointer, &bytes_read);
			string_pointer += bytes_read;

			Glyph *glyph = GetGlyph(font, unicode_value);

			if (glyph != NULL)
			{
				const long letter_x = x + pen_x + glyph->x_offset;
				const long letter_y = y + glyph->y_offset;

				Video_Rect src_rect = {glyph->x, glyph->y, glyph->width, glyph->height};
				Video_Rect dst_rect = {letter_x, letter_y, glyph->width, glyph->height};

//				RenderBackend_DrawGlyph(letter_x, letter_y, glyph->x, glyph->y, glyph->width, glyph->height);
				Video_TextureDraw(font->atlas, &dst_rect, &src_rect, colour & 0xFF, (colour >> 8) & 0xFF, (colour >> 16) & 0xFF);

				pen_x += glyph->x_advance;
			}
		}
	}
}

void UnloadFont(Font *font)
{
	if (font != NULL)
	{
		Video_TextureDestroy(font->atlas);

	#ifdef FREETYPE_FONTS
		FT_Done_Face(font->face);
		free(font->data);
		FT_Done_FreeType(font->library);
	#else
		free(font->local_glyphs);
		FreeBitmap(font->image_buffer);
	#endif

		free(font);
	}
}
