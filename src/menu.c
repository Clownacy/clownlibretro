#include "menu.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "font.h"
#include "input.h"
#include "video.h"

#define FONT_WIDTH 20
#define FONT_HEIGHT 20

static
#include "dejavu.h"

static Font font;
static Video_Texture *font_texture;

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

	font_texture = Video_TextureCreate(width, height, VIDEO_FORMAT_A8, false);

	return font_texture != NULL;
}

static void FontCallback_DestroyTexture(void* const user_data)
{
	(void)user_data;

	Video_TextureDestroy(font_texture);
}

static void FontCallback_UpdateTexture(const unsigned char* const pixels, const Font_Rect* const rect, void* const user_data)
{
	Video_Rect video_rect;

	(void)user_data;

	FontRectToVideoRect(&video_rect, rect);
	// Note that we skip over the shadow texture here, since we don't need it.
	Video_TextureUpdate(font_texture, pixels + rect->width * rect->height, rect->width, &video_rect);
}

static void FontCallback_DrawTexture(const Font_Rect* const dst_rect, const Font_Rect* const src_rect, const Font_Colour* const colour, const cc_bool do_shadow, void* const user_data)
{
	Video_Rect video_src_rect, video_dst_rect;

	const Video_Colour video_colour_black = {0, 0, 0};
	const Video_Colour video_colour = {colour->red, colour->green, colour->blue};

	(void)user_data;
	(void)do_shadow;

	FontRectToVideoRect(&video_src_rect, src_rect);
	FontRectToVideoRect(&video_dst_rect, dst_rect);

	video_dst_rect.x += 2;
	video_dst_rect.y += 2;
	Video_TextureDraw(font_texture, &video_dst_rect, &video_src_rect, video_colour_black);
	video_dst_rect.x -= 2;
	video_dst_rect.y -= 2;
	Video_TextureDraw(font_texture, &video_dst_rect, &video_src_rect, video_colour);
}

static const Font_Callbacks font_callbacks = {NULL, FontCallback_CreateTexture, FontCallback_DestroyTexture, FontCallback_UpdateTexture, FontCallback_DrawTexture};

static void DrawTextCentered(const char *text, size_t x, size_t y, const Font_Colour *colour)
{
	Font_DrawTextCentred(&font, x, y - FONT_HEIGHT / 2, colour, text, strlen(text), &font_callbacks);
}

static void DrawOption(Menu *menu, size_t option, size_t x, size_t y, const Font_Colour *colour)
{
	DrawTextCentered(menu->options[option].label, x, y - 10, colour);
	DrawTextCentered(menu->options[option].value, x, y + 10, colour);
}

bool Menu_Init(void)
{
	return Font_LoadFromMemory(&font, dejavu, sizeof(dejavu), FONT_WIDTH, FONT_HEIGHT, true, 0, &font_callbacks);
}

void Menu_Deinit(void)
{
	Font_Unload(&font, &font_callbacks);
}

Menu* Menu_Create(Menu_Callback *callbacks, size_t total_callbacks)
{
	Menu *menu = malloc(sizeof(Menu) + total_callbacks * sizeof(Menu_Option));

	if (menu != NULL)
	{
		menu->selected_option = 0;
		menu->total_options = total_callbacks;

		for (size_t i = 0; i < total_callbacks; ++i)
		{
			menu->options[i].callback = callbacks[i];

			callbacks[i].function(&menu->options[i], MENU_INIT, callbacks[i].user_data);
		}
	}

	return menu;
}

void Menu_Destroy(Menu *menu)
{
	for (size_t i = 0; i < menu->total_options; ++i)
		menu->options[i].callback.function(&menu->options[i], MENU_DEINIT, menu->options[i].callback.user_data);

	free(menu);
}

void Menu_Update(Menu *menu)
{
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

	for (size_t i = 0; i < menu->total_options; ++i)
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
	if (menu->total_options == 0)
	{
		DrawTextCentered("NO OPTIONS", window_width / 2, window_height / 2, &(Font_Colour){0xFF, 0xFF, 0xFF});
	}
	else
	{
		if (menu->selected_option != 0)
			DrawOption(menu, menu->selected_option - 1, window_width / 2, window_height / 2 - 80, &(Font_Colour){0xFF, 0xFF, 0xFF});

		DrawOption(menu, menu->selected_option, window_width / 2, window_height / 2, &(Font_Colour){0xFF, 0xFF, 0x80});

		if (menu->selected_option != menu->total_options - 1)
			DrawOption(menu, menu->selected_option + 1, window_width / 2, window_height / 2 + 80, &(Font_Colour){0xFF, 0xFF, 0xFF});
	}
}
