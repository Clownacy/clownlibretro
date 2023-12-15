#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum Menu_CallbackAction
{
	MENU_INIT,
	MENU_DEINIT,
	MENU_UPDATE_NONE,
	MENU_UPDATE_OK,
	MENU_UPDATE_LEFT,
	MENU_UPDATE_RIGHT
} Menu_CallbackAction;

struct Menu_Option;

typedef int (*Menu_CallbackFunction)(struct Menu_Option *option, Menu_CallbackAction action, void *user_data);

typedef struct Menu_Callback
{
	Menu_CallbackFunction function;
	void *user_data;
} Menu_Callback;

typedef struct Menu_Option
{
	Menu_Callback callback;

	const char *label;
	const char *sublabel;
	const char *value;
	const char *value_description;
} Menu_Option;

typedef struct Menu
{
	size_t selected_option;
	size_t total_options;
	Menu_Option options[];
} Menu;

bool Menu_Init(float dpi);
void Menu_Deinit(void);
void Menu_ChangeDPI(float dpi);

Menu* Menu_Create(Menu_Callback *callbacks, size_t total_callbacks);
void Menu_Destroy(Menu *menu);

void Menu_Update(Menu *menu);
void Menu_Draw(Menu *menu);
