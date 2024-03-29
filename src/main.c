#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "SDL.h"

#include "audio.h"
#include "core_runner.h"
#include "error.h"
#include "input.h"
#include "menu.h"
#include "video.h"

static bool audio_initialised;

static double frames_per_second;

static bool menu_open;

static Menu *menu;

/*******
* Main *
*******/

static int OptionsMenuCallback(Menu_Option *option, Menu_CallbackAction action, void *user_data)
{
	Variable *variable = (Variable*)user_data;

	switch (action)
	{
		case MENU_INIT:
			option->label = variable->desc;
			option->sublabel = variable->info;
			option->value = variable->values[variable->selected_value].value;
			break;

		case MENU_DEINIT:
			break;

		case MENU_UPDATE_NONE:
			break;

		case MENU_UPDATE_OK:
		case MENU_UPDATE_LEFT:
		case MENU_UPDATE_RIGHT:
			if (action == MENU_UPDATE_LEFT)
			{
				if (variable->selected_value == 0)
					variable->selected_value = variable->total_values - 1;
				else
					--variable->selected_value;
			}
			else
			{
				if (variable->selected_value == variable->total_values - 1)
					variable->selected_value = 0;
				else
					++variable->selected_value;
			}

			option->value = variable->values[variable->selected_value].value;

			break;
	}

	return 0; /* TODO */
}

static void ToggleMenu(void)
{
	menu_open = !menu_open;

	if (menu_open)
	{
		Variable *variables;
		size_t total_variables;
		Menu_Callback *callbacks;

		CoreRunner_GetVariables(&variables, &total_variables);

		callbacks = (Menu_Callback*)SDL_malloc(sizeof(Menu_Callback) * total_variables);

		if (callbacks == NULL)
		{
			PrintError("Could not allocate memory for the menu callbacks");
		}
		else
		{
			size_t i;

			for (i = 0; i < total_variables; ++i)
			{
				callbacks[i].function = OptionsMenuCallback;
				callbacks[i].user_data = &variables[i];
			}

			menu = Menu_Create(callbacks, total_variables);

			SDL_free(callbacks);
		}
	}
	else
	{
		Menu_Destroy(menu);

		CoreRunner_VariablesModified();
	}
}

static cc_bool Iterate(void)
{
	bool quit;
	SDL_Event event;
	size_t i;
	cc_bool previous_held_buttons[CC_COUNT_OF(retropad.buttons)];

	quit = false;

	for (i = 0; i < CC_COUNT_OF(previous_held_buttons); ++i)
		previous_held_buttons[i] = retropad.buttons[i].held;

	/* Handle events */
	while (SDL_PollEvent(&event))
	{
		static bool alt_held;

		switch (event.type)
		{
			case SDL_QUIT:
				return false;

			case SDL_WINDOWEVENT:
				switch (event.window.event)
				{
					case SDL_WINDOWEVENT_SIZE_CHANGED:
						Video_WindowResized();
						Menu_ChangeDPI(Video_GetDPIScale());
						break;
				}

				break;

			case SDL_KEYDOWN:
			case SDL_KEYUP:
				switch (event.key.keysym.sym)
				{
					case SDLK_LALT:
						alt_held = event.key.state == SDL_PRESSED;
						break;

						/* TODO: Move both of these keys' actions to the options menu instead of them being hotkeys. */
					case SDLK_F1:
						if (event.key.state == SDL_PRESSED)
						{
							static bool alternate_layout;

							alternate_layout = !alternate_layout;
							CoreRunner_SetAlternateButtonLayout(alternate_layout);
						}

						break;

					case SDLK_F2:
						if (event.key.state == SDL_PRESSED)
						{
							static CoreRunnerScreenType screen_type;

							const CoreRunnerScreenType next[] = {CORE_RUNNER_SCREEN_TYPE_PIXEL_PERFECT, CORE_RUNNER_SCREEN_TYPE_PIXEL_PERFECT_WITH_SCANLINES, CORE_RUNNER_SCREEN_TYPE_FIT};
							screen_type = next[screen_type];

							CoreRunner_SetScreenType(screen_type);
						}

						break;
				}

				switch (event.key.keysym.scancode)
				{
					case SDL_SCANCODE_W:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_UP].held = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_A:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_LEFT].held = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_S:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_DOWN].held = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_D:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_RIGHT].held = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_P:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_A].held = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_O:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_B].held = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_0:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_X].held = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_9:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_Y].held = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_8:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_L].held = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_7:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_L2].held = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_L:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_L3].held = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_MINUS:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_R].held = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_EQUALS:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_R2].held = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_SEMICOLON:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_R3].held = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_RETURN:
						if (event.key.state == SDL_PRESSED && alt_held)
						{
							static bool fullscreen = false;
							fullscreen = !fullscreen;

							Video_SetFullscreen(fullscreen);
						}
						else
						{
							retropad.buttons[RETRO_DEVICE_ID_JOYPAD_START].held = event.key.state == SDL_PRESSED;
						}

						break;

					case SDL_SCANCODE_BACKSPACE:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_SELECT].held = event.key.state == SDL_PRESSED;
						break;


					case SDL_SCANCODE_ESCAPE:
						if (event.key.state == SDL_PRESSED)
							ToggleMenu();

						break;

					default:
						break;
				}

				break;

			case SDL_CONTROLLERBUTTONDOWN:
			case SDL_CONTROLLERBUTTONUP:
			{
				unsigned int retropad_index = -1;

				switch (event.cbutton.button)
				{
				#define DO_BUTTON(INPUT, OUTPUT)\
					case INPUT:\
						retropad_index = OUTPUT;\
						break

					DO_BUTTON(SDL_CONTROLLER_BUTTON_A, RETRO_DEVICE_ID_JOYPAD_B);
					DO_BUTTON(SDL_CONTROLLER_BUTTON_B, RETRO_DEVICE_ID_JOYPAD_A);
					DO_BUTTON(SDL_CONTROLLER_BUTTON_X, RETRO_DEVICE_ID_JOYPAD_Y);
					DO_BUTTON(SDL_CONTROLLER_BUTTON_Y, RETRO_DEVICE_ID_JOYPAD_X);
					DO_BUTTON(SDL_CONTROLLER_BUTTON_BACK, RETRO_DEVICE_ID_JOYPAD_SELECT);
					DO_BUTTON(SDL_CONTROLLER_BUTTON_START, RETRO_DEVICE_ID_JOYPAD_START);
					DO_BUTTON(SDL_CONTROLLER_BUTTON_LEFTSTICK, RETRO_DEVICE_ID_JOYPAD_L3);
					DO_BUTTON(SDL_CONTROLLER_BUTTON_RIGHTSTICK, RETRO_DEVICE_ID_JOYPAD_R3);
					DO_BUTTON(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, RETRO_DEVICE_ID_JOYPAD_L);
					DO_BUTTON(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, RETRO_DEVICE_ID_JOYPAD_R);
					DO_BUTTON(SDL_CONTROLLER_BUTTON_DPAD_UP, RETRO_DEVICE_ID_JOYPAD_UP);
					DO_BUTTON(SDL_CONTROLLER_BUTTON_DPAD_DOWN, RETRO_DEVICE_ID_JOYPAD_DOWN);
					DO_BUTTON(SDL_CONTROLLER_BUTTON_DPAD_LEFT, RETRO_DEVICE_ID_JOYPAD_LEFT);
					DO_BUTTON(SDL_CONTROLLER_BUTTON_DPAD_RIGHT, RETRO_DEVICE_ID_JOYPAD_RIGHT);
				#undef DO_BUTTON
				}

				if (retropad_index != (unsigned int)-1)
					Input_SetButtonDigital(retropad_index, event.cbutton.state == SDL_PRESSED);

				break;
			}

			case SDL_CONTROLLERAXISMOTION:
				switch (event.caxis.axis)
				{
					case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
						Input_SetButtonAnalog(RETRO_DEVICE_ID_JOYPAD_L2, event.caxis.value);
						break;

					case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
						Input_SetButtonAnalog(RETRO_DEVICE_ID_JOYPAD_R2, event.caxis.value);
						break;

					case SDL_CONTROLLER_AXIS_LEFTX:
						retropad.sticks[0].axis[0] = event.caxis.value;
						break;

					case SDL_CONTROLLER_AXIS_LEFTY:
						retropad.sticks[0].axis[1] = event.caxis.value;
						break;

					case SDL_CONTROLLER_AXIS_RIGHTX:
						retropad.sticks[1].axis[0] = event.caxis.value;
						break;

					case SDL_CONTROLLER_AXIS_RIGHTY:
						retropad.sticks[1].axis[1] = event.caxis.value;
						break;
				}

				break;

			case SDL_CONTROLLERDEVICEADDED:
				SDL_GameControllerOpen(event.cdevice.which);
				break;
		}
	}

	for (i = 0; i < CC_COUNT_OF(retropad.buttons); ++i)
		retropad.buttons[i].pressed = retropad.buttons[i].held && !previous_held_buttons[i];

	if (retropad.buttons[RETRO_DEVICE_ID_JOYPAD_L3].pressed && retropad.buttons[RETRO_DEVICE_ID_JOYPAD_R3].pressed)
		ToggleMenu();

	if (menu_open)
	{
		Menu_Update(menu);
	}
	else
	{
		if (!CoreRunner_Update())
			quit = true;
	}

	/* Draw stuff */
	Video_Clear();

	CoreRunner_Draw();

	if (menu_open)
		Menu_Draw(menu);

	Video_Display();

	{
		/* Delay until the next frame */
		static double ticks_next;
		const Uint32 ticks_now = SDL_GetTicks();

		if (ticks_now < ticks_next)
			SDL_Delay(ticks_next - ticks_now);

		ticks_next = SDL_max(ticks_next, ticks_now) + 1000.0 / frames_per_second;
	}

	return !quit;
}

int main(int argc, char **argv)
{
	int main_return = EXIT_FAILURE;

#ifndef __WIIU__
	if (argc < 2)
	{
#ifdef DYNAMIC_CORE
		PrintError("Core path not specified");
	}
	else if (argc < 3)
	{
#endif
		PrintError("Game path not specified");
	}
	else
#endif
	{
		/* Initialise SDL2 video and audio */
		if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) < 0)
		{
			PrintError("SDL_Init failed - Error: '%s'", SDL_GetError());
		}
		else
		{
			/* Enable high-DPI support on Windows because SDL2 is bad at being a platform abstraction library */
			SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");

			if (!Video_Init(640, 480)) /* TODO: Placeholder */
			{
				PrintError("InitVideo failed");
			}
			else
			{
			#if defined(__WIIU__)
				/*const char* const game_path = "OoTR_1509886_A1HZRRHPQN.z64";*/
				const char* const game_path = "s1built.bin";
			#elif defined(DYNAMIC_CORE)
				const char* const core_path = argv[1];
				const char* const game_path = argv[2];
			#else
				const char* const game_path = argv[1];
			#endif

				audio_initialised = Audio_Init();

				Menu_Init(Video_GetDPIScale());

				if (!CoreRunner_Init(
				#ifdef DYNAMIC_CORE
					core_path,
				#endif
					game_path, &frames_per_second))
				{
					PrintError("CoreRunner_Init failed");
				}
				else
				{
					main_return = EXIT_SUCCESS;

					/* Begin the mainloop */
					while (Iterate());

					CoreRunner_Deinit();
				}

				Menu_Deinit();

				if (audio_initialised)
					Audio_Deinit();

				Video_Deinit();
			}

			SDL_Quit();
		}
	}

	return main_return;
}
