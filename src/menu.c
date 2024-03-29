#include "menu.h"

#include <stddef.h>
#include <stdlib.h>

#include "font.h"
#include "input.h"
#include "video.h"

#define FONT_WIDTH 20
#define FONT_HEIGHT 20

#define DPI_SCALE(x) (unsigned int)(x * dpi_scale)

static
#include "dejavu.h"

static Font font;
static Video_Texture font_texture;
static float dpi_scale;

static void FontRectToVideoRect(Video_Rect* const video, const Font_Rect* const font)
{
	video->x = font->x;
	video->y = font->y;
	video->width = font->width;
	video->height = font->height;
}

static cc_bool FontCallback_CreateTexture(const size_t width, const size_t height, void* const user_data)
{
	(void)user_data;

	return Video_TextureCreate(&font_texture, width, height, VIDEO_FORMAT_A8, false);
}

static void FontCallback_DestroyTexture(void* const user_data)
{
	(void)user_data;

	Video_TextureDestroy(&font_texture);
}

static void FontCallback_UpdateTexture(const unsigned char* const pixels, const Font_Rect* const rect, void* const user_data)
{
	Video_Rect video_rect;

	(void)user_data;

	FontRectToVideoRect(&video_rect, rect);
	/* Note that we skip over the shadow texture here, since we don't need it. */
	Video_TextureUpdate(&font_texture, pixels + rect->width * rect->height, &video_rect);
}

static void FontCallback_DrawTexture(const Font_Rect* const dst_rect, const Font_Rect* const src_rect, const Font_Colour* const colour, const cc_bool do_shadow, void* const user_data)
{
	Video_Colour video_colour;
	Video_Rect video_src_rect, video_dst_rect;

	const Video_Colour video_colour_black = {0, 0, 0};

	(void)user_data;
	(void)do_shadow;

	video_colour.red = colour->red;
	video_colour.green = colour->green;
	video_colour.blue = colour->blue;

	FontRectToVideoRect(&video_src_rect, src_rect);
	FontRectToVideoRect(&video_dst_rect, dst_rect);

	video_dst_rect.x += DPI_SCALE(2);
	video_dst_rect.y += DPI_SCALE(2);
	Video_TextureDraw(&font_texture, &video_dst_rect, &video_src_rect, video_colour_black);
	video_dst_rect.x -= DPI_SCALE(2);
	video_dst_rect.y -= DPI_SCALE(2);
	Video_TextureDraw(&font_texture, &video_dst_rect, &video_src_rect, video_colour);
}

static const Font_Callbacks font_callbacks = {NULL, FontCallback_CreateTexture, FontCallback_DestroyTexture, FontCallback_UpdateTexture, FontCallback_DrawTexture};

static void DrawTextCentered(const char *text, size_t x, size_t y, const Font_Colour *colour)
{
	Font_DrawTextCentred(&font, x, y - DPI_SCALE(FONT_HEIGHT) / 2, colour, text, strlen(text), &font_callbacks);
}

static void DrawOption(Menu *menu, size_t option, size_t x, size_t y, const Font_Colour *colour)
{
	DrawTextCentered(menu->options[option].label, x, y - DPI_SCALE(10), colour);
	DrawTextCentered(menu->options[option].value, x, y + DPI_SCALE(10), colour);
}

cc_bool Menu_Init(const float dpi)
{
	dpi_scale = dpi;
	return Font_LoadFromMemory(&font, dejavu, sizeof(dejavu), FONT_WIDTH * dpi, FONT_HEIGHT * dpi, cc_true, 0, &font_callbacks);
}

void Menu_Deinit(void)
{
	Font_Unload(&font, &font_callbacks);
}

void Menu_ChangeDPI(const float dpi)
{
	Menu_Deinit();
	Menu_Init(dpi);
}

Menu* Menu_Create(Menu_Callback *callbacks, size_t total_callbacks)
{
	Menu *menu = (Menu*)malloc(sizeof(Menu) + (total_callbacks - 1) * sizeof(Menu_Option));

	if (menu != NULL)
	{
		size_t i;

		menu->selected_option = 0;
		menu->total_options = total_callbacks;

		for (i = 0; i < total_callbacks; ++i)
		{
			menu->options[i].callback = callbacks[i];

			callbacks[i].function(&menu->options[i], MENU_INIT, callbacks[i].user_data);
		}
	}

	return menu;
}

void Menu_Destroy(Menu *menu)
{
	size_t i;

	for (i = 0; i < menu->total_options; ++i)
		menu->options[i].callback.function(&menu->options[i], MENU_DEINIT, menu->options[i].callback.user_data);

	free(menu);
}

void Menu_Update(Menu *menu)
{
	size_t i;

	if (retropad.buttons[RETRO_DEVICE_ID_JOYPAD_UP].pressed)
	{
		if (menu->selected_option == 0)
			menu->selected_option = menu->total_options - 1;
		else
			--menu->selected_option;
	}
	else if (retropad.buttons[RETRO_DEVICE_ID_JOYPAD_DOWN].pressed)
	{
		if (menu->selected_option == menu->total_options - 1)
			menu->selected_option = 0;
		else
			++menu->selected_option;
	}

	for (i = 0; i < menu->total_options; ++i)
	{
		Menu_CallbackAction action = MENU_UPDATE_NONE;

		if (menu->selected_option == i)
		{
			if (retropad.buttons[RETRO_DEVICE_ID_JOYPAD_B].pressed)
				action = MENU_UPDATE_OK;
			else if (retropad.buttons[RETRO_DEVICE_ID_JOYPAD_LEFT].pressed)
				action = MENU_UPDATE_LEFT;
			else if (retropad.buttons[RETRO_DEVICE_ID_JOYPAD_RIGHT].pressed)
				action = MENU_UPDATE_RIGHT;
		}

		menu->options[i].callback.function(&menu->options[i], action, menu->options[i].callback.user_data);
	}
}

void Menu_Draw(Menu *menu)
{
	Video_Rect rect;

	const Video_Colour black = {0, 0, 0};
	const Font_Colour white = {0xFF, 0xFF, 0xFF};
	const unsigned int background_height = DPI_SCALE(260);

	rect.x = 0;
	rect.y = window_height / 2 - background_height / 2;
	rect.width = window_width;
	rect.height = background_height;

	Video_ColourFill(&rect, black, 0xA0);

	if (menu->total_options == 0)
	{
		DrawTextCentered("NO OPTIONS", window_width / 2, window_height / 2, &white);
	}
	else
	{
		const Font_Colour yellow = {0xFF, 0xFF, 0x80};
		const unsigned int option_spacing = DPI_SCALE(80);

		if (menu->selected_option != 0)
			DrawOption(menu, menu->selected_option - 1, window_width / 2, window_height / 2 - option_spacing, &white);

		DrawOption(menu, menu->selected_option, window_width / 2, window_height / 2, &yellow);

		if (menu->selected_option != menu->total_options - 1)
			DrawOption(menu, menu->selected_option + 1, window_width / 2, window_height / 2 + option_spacing, &white);
	}
}
