#include "menu.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "event.h"
#include "font.h"
#include "input.h"
#include "video.h"

//#define MAX_OPTIONS ((WINDOW_HEIGHT / 20) - 4)	// The maximum number of options we can fit on-screen at once
#define MAX_OPTIONS ((window_height / 20) - 4)	// The maximum number of options we can fit on-screen at once

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define RGB(r,g,b) ((r) | ((g) << 8) | ((b) << 16))

Font *menu_font;

enum
{
	CALLBACK_CONTINUE = -1,
	CALLBACK_PREVIOUS_MENU = -2,
	CALLBACK_RESET = -3,
	CALLBACK_EXIT = -4,
};

typedef enum CallbackAction
{
	ACTION_INIT,
	ACTION_DEINIT,
	ACTION_UPDATE,
	ACTION_OK,
	ACTION_LEFT,
	ACTION_RIGHT
} CallbackAction;

struct OptionsMenu;

typedef struct Option
{
	const char *name;
	int (*callback)(struct OptionsMenu *parent_menu, size_t this_option, CallbackAction action);
	void *user_data;
	const char *value_string;
	long value;
	bool disabled;
} Option;

typedef struct OptionsMenu
{
	const char *title;
	const char *subtitle;
	struct Option *options;
	size_t total_options;
	int x_offset;
	bool submenu;
} OptionsMenu;

static void PutText(int x, int y, const char *text, unsigned long color)
{
	DrawText(menu_font, NULL, x, y, color, text);
}

static void PutTextCentred(int x, int y, const char *text, unsigned long color)
{
	size_t string_width = 0;
	size_t string_height = font_height;

	for (const char *text_pointer = text; *text_pointer != '\0';)
	{
		text_pointer += 1;
		string_width += font_width;
	}

	PutText(x - string_width / 2, y - string_height / 2, text, color);
}

static int EnterOptionsMenu(OptionsMenu *options_menu, size_t selected_option)
{
	int scroll = 0;

	unsigned int anime = 0;

	int return_value;

	// Initialise options
	for (size_t i = 0; i < options_menu->total_options; ++i)
		options_menu->options[i].callback(options_menu, i, ACTION_INIT);

	for (;;)
	{
		// Get pressed keys
//		GetTrg();

		// Allow unpausing by pressing the pause button only when in the main pause menu (not submenus)
		if (!options_menu->submenu && retropad.buttons[RETRO_DEVICE_ID_JOYPAD_R3].pressed/*gKeyTrg & KEY_PAUSE*/)
		{
			return_value = CALLBACK_CONTINUE;
			break;
		}

		// Go back one submenu when the 'cancel' button is pressed
		if (retropad.buttons[RETRO_DEVICE_ID_JOYPAD_B].pressed/*gKeyTrg & gKeyCancel*/)
		{
			return_value = CALLBACK_CONTINUE;
			break;
		}

		// Handling up/down input
		if (retropad.buttons[RETRO_DEVICE_ID_JOYPAD_UP].pressed || retropad.buttons[RETRO_DEVICE_ID_JOYPAD_DOWN].pressed/*/*gKeyTrg & (gKeyUp | gKeyDown)*/)
		{
			const size_t old_selection = selected_option;

			if (retropad.buttons[RETRO_DEVICE_ID_JOYPAD_DOWN].pressed/*gKeyTrg & gKeyDown*/)
				if (selected_option++ == options_menu->total_options - 1)
					selected_option = 0;

			if (retropad.buttons[RETRO_DEVICE_ID_JOYPAD_UP].pressed/*gKeyTrg & gKeyUp*/)
				if (selected_option-- == 0)
					selected_option = options_menu->total_options - 1;

			// Update the menu-scrolling, if there are more options than can be fit on the screen
			if (selected_option < old_selection)
				scroll = MAX(0, MIN(scroll, (int)selected_option - 1));

			if (selected_option > old_selection)
				scroll = MIN(MAX(0, (int)(options_menu->total_options - MAX_OPTIONS)), MAX(scroll, (int)(selected_option - (MAX_OPTIONS - 2))));

			//PlaySoundObject(1, SOUND_MODE_PLAY);
		}

		// Run option callbacks
		for (size_t i = 0; i < options_menu->total_options; ++i)
		{
			if (!options_menu->options[i].disabled)
			{
				CallbackAction action = ACTION_UPDATE;

				if (i == selected_option)
				{
					if (retropad.buttons[RETRO_DEVICE_ID_JOYPAD_A].pressed/*gKeyTrg & gKeyOk*/)
						action = ACTION_OK;
					else if (retropad.buttons[RETRO_DEVICE_ID_JOYPAD_LEFT].pressed/*gKeyTrg & gKeyLeft*/)
						action = ACTION_LEFT;
					else if (retropad.buttons[RETRO_DEVICE_ID_JOYPAD_RIGHT].pressed/*gKeyTrg & gKeyRight*/)
						action = ACTION_RIGHT;
				}

				return_value = options_menu->options[i].callback(options_menu, i, action);

				// If the callback returned something other than CALLBACK_CONTINUE, it's time to exit this submenu
				if (return_value != CALLBACK_CONTINUE)
					break;
			}
		}

		if (return_value != CALLBACK_CONTINUE)
			break;

		// Update Quote animation counter
		if (++anime >= 40)
			anime = 0;

		// Draw screen
		//CortBox(&grcFull, 0x000000);
		Video_Clear();

		const size_t visible_options = MIN(MAX_OPTIONS, options_menu->total_options);

		int y = (window_height / 2) - ((visible_options * 20) / 2) - (40 / 2);

		// Draw title
		PutTextCentred(window_width / 2, y, options_menu->title, RGB(0xFF, 0xFF, 0xFF));

		// Draw subtitle
		if (options_menu->subtitle != NULL)
			PutTextCentred(window_width / 2, y + 14, options_menu->subtitle, RGB(0xFF, 0xFF, 0xFF));

		y += 40;

		for (size_t i = scroll; i < scroll + visible_options; ++i)
		{
			const int x = (window_width / 2) + options_menu->x_offset;

			// Draw Quote next to the selected option
//			if (i == selected_option)
//				PutBitmap3(&grcFull, PixelToScreenCoord(x - 20), PixelToScreenCoord(y - 8), &rcMyChar[anime / 10 % 4], SURFACE_ID_MY_CHAR);

			unsigned long option_colour = options_menu->options[i].disabled ? RGB(0x80, 0x80, 0x80) : RGB(0xFF, 0xFF, 0xFF);

			// Draw option name
			PutText(x, y - font_height / 2, options_menu->options[i].name, option_colour);

			// Draw option value, if it has one
			if (options_menu->options[i].value_string != NULL)
				PutText(x + 100, y - font_height / 2, options_menu->options[i].value_string, option_colour);

			y += 20;
		}

		//PutFramePerSecound();

		Video_Display();

		//if (!Flip_SystemTask())
		if (!HandleEvents())
		{
			// Quit if window is closed
			return_value = CALLBACK_EXIT;
			break;
		}

		Input_Update();
	}

	// Deinitialise options
	for (size_t i = 0; i < options_menu->total_options; ++i)
		options_menu->options[i].callback(options_menu, i, ACTION_DEINIT);

	return return_value;
}

static int Callback_Resume(OptionsMenu *parent_menu, size_t this_option, CallbackAction action)
{
	(void)parent_menu;

	if (action != ACTION_OK)
		return CALLBACK_CONTINUE;

	//PlaySoundObject(18, SOUND_MODE_PLAY);
	return 0;//enum_ESCRETURN_continue;
}

void EnterMenu(Font *font)
{
	menu_font = font;

	Option options[] = {
		{"Resume", Callback_Resume, NULL, NULL, 0, false},
//		{"Reset", Callback_Reset, NULL, NULL, 0, false},
//		{"Options", Callback_Options, NULL, NULL, 0, false},
//		{"Quit", Callback_Quit, NULL, NULL, 0, false}
	};

	OptionsMenu options_menu = {
		"PAUSED",
		NULL,
		options,
		sizeof(options) / sizeof(options[0]),
		-14,
		false
	};

	int return_value = EnterOptionsMenu(&options_menu, 0);

/*
	// Filter internal return values to something Cave Story can understand
	switch (return_value)
	{
		case CALLBACK_CONTINUE:
			return_value = enum_ESCRETURN_continue;
			break;

		case CALLBACK_RESET:
			return_value = enum_ESCRETURN_restart;
			break;

		case CALLBACK_EXIT:
			return_value = enum_ESCRETURN_exit;
			break;
	}

	//gKeyTrg = gKey = 0;	// Avoid input-ghosting

	return return_value;*/
}
