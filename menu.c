#include "menu.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "font.h"
#include "input.h"
#include "video.h"

static Font *font;
static size_t font_width;
static size_t font_height;

static void DrawTextCentered(const char *text, size_t x, size_t y, Video_Colour colour)
{
	// TODO - Don't assume monospace. Will require modifications to `font.c`.

	const size_t text_width = font_width * strlen(text);
	const size_t text_height = font_height;

	DrawText(font, NULL, x - text_width / 2 + 2, y - text_height / 2 + 2, (Video_Colour){0, 0, 0}, text);
	DrawText(font, NULL, x - text_width / 2, y - text_height / 2, colour, text);
}

static void DrawOption(Menu *menu, size_t option, size_t x, size_t y, Video_Colour colour)
{
	DrawTextCentered(menu->options[option].label, x, y - 10, colour);
	DrawTextCentered(menu->options[option].value, x, y + 10, colour);
}

bool Menu_Init(void)
{
	font_width = 10;
	font_height = 20;
	font = LoadFreeTypeFont("font", font_width, font_height, true);

	return font != NULL;
}

void Menu_Deinit(void)
{
	UnloadFont(font);
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
		DrawTextCentered("NO OPTIONS", window_width / 2, window_height / 2, (Video_Colour){0xFF, 0xFF, 0xFF});
	}
	else
	{
		if (menu->selected_option != 0)
			DrawOption(menu, menu->selected_option - 1, window_width / 2, window_height / 2 - 80, (Video_Colour){0xFF, 0xFF, 0xFF});

		DrawOption(menu, menu->selected_option, window_width / 2, window_height / 2, (Video_Colour){0xFF, 0xFF, 0x80});

		if (menu->selected_option != menu->total_options - 1)
			DrawOption(menu, menu->selected_option + 1, window_width / 2, window_height / 2 + 80, (Video_Colour){0xFF, 0xFF, 0xFF});
	}
}
