#include "menu2.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "font.h"
#include "input.h"
#include "video.h"

static Font *font;
static size_t font_width;
static size_t font_height;

static void DrawTextCentered(const char *text, size_t x, size_t y)
{
	// TODO - Don't assume monospace. Will require modifications to `font.c`.

	size_t text_width = font_width * strlen(text);
	size_t text_height = font_height;

	DrawText(font, NULL, x, y, (Video_Colour){0xFF, 0xFF, 0xFF}, text);
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

			callbacks(&menu->options[i], MENU_INIT);
		}
	}

	return menu;
}

void Menu_Destroy(Menu *menu)
{
	for (size_t i = 0; i < menu->total_options; ++i)
		menu->options[i].callback(&menu->options[i], MENU_DEINIT);

	free(menu);
}

void Menu_Update(Menu *menu)
{
	if ((retropad.buttons[RETRO_DEVICE_ID_JOYPAD_UP].pressed)
	{
		if (menu->selected_option == 0)
			menu->selected_option = menu->total_options - 1;
		else
			--menu->selected_option;
	}
	else if ((retropad.buttons[RETRO_DEVICE_ID_JOYPAD_DOWN].pressed)
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
			if (retropad.buttons[RETRO_DEVICE_ID_JOYPAD_A].pressed)
				action = MENU_UPDATE_OK;
			else if (retropad.buttons[RETRO_DEVICE_ID_JOYPAD_LEFT].pressed)
				action = MENU_UPDATE_LEFT;
			else if (retropad.buttons[RETRO_DEVICE_ID_JOYPAD_RIGHT].pressed)
				action = MENU_UPDATE_RIGHT;
		}

		menu->options[i].callback(&menu->options[i], action);
	}
}

void Menu_Draw(Menu *menu)
{
	DrawTextCentered(menu->options[menu->selected_option]->label, window_width / 2, window_height / 2);
}
