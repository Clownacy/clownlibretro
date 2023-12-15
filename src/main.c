#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "SDL.h"

#include "audio.h"
#include "core_runner.h"
#include "input.h"
#include "menu.h"
#include "video.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static bool audio_initialised;

static double frames_per_second;

static bool quit;
static bool menu_open;

static Menu *menu;

  //////////
 // Main //
//////////

static int OptionsMenuCallback(Menu_Option *option, Menu_CallbackAction action, void *user_data)
{
	Variable *variable = user_data;

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

	return 0; // TODO
}

static void ToggleMenu(void)
{
	menu_open = !menu_open;

	if (menu_open)
	{
		Variable *variables;
		size_t total_variables;
		CoreRunner_GetVariables(&variables, &total_variables);

		Menu_Callback callbacks[total_variables]; // TODO: Get rid of this VLA
		for (size_t i = 0; i < total_variables; ++i)
		{
			callbacks[i].function = OptionsMenuCallback;
			callbacks[i].user_data = &variables[i];
		}

		menu = Menu_Create(callbacks, total_variables);
	}
	else
	{
		Menu_Destroy(menu);

		CoreRunner_VariablesModified();
	}
}

int main(int argc, char **argv)
{
	int main_return = EXIT_FAILURE;

	if (argc < 2)
	{
		fputs("Core path not specified\n", stderr);
	}
	else if (argc < 3)
	{
		fputs("Game path not specified\n", stderr);
	}
	else
	{
		// Initialise SDL2 video and audio
		if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) < 0)
		{
			fprintf(stderr, "SDL_Init failed - Error: '%s'\n", SDL_GetError());
		}
		else
		{
			if (!Video_Init(640, 480)) // TODO: Placeholder
			{
				fputs("InitVideo failed\n", stderr);
			}
			else
			{
				audio_initialised = Audio_Init();

				Menu_Init(Video_GetDPIScale());

				if (!CoreRunner_Init(argv[1], argv[2], &frames_per_second))
				{
					fputs("CoreRunner_Init failed\n", stderr);
				}
				else
				{
					main_return = EXIT_SUCCESS;

					// Begin the mainloop
					quit = false;

					while (!quit)
					{
						// Handle events
						SDL_Event event;
						while (SDL_PollEvent(&event))
						{
							static bool alt_held;

							switch (event.type)
							{
								case SDL_QUIT:
									quit = true;
									break;

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
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_UP].raw = event.key.state == SDL_PRESSED;
											break;

										case SDL_SCANCODE_A:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_LEFT].raw = event.key.state == SDL_PRESSED;
											break;

										case SDL_SCANCODE_S:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_DOWN].raw = event.key.state == SDL_PRESSED;
											break;

										case SDL_SCANCODE_D:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_RIGHT].raw = event.key.state == SDL_PRESSED;
											break;

										case SDL_SCANCODE_P:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_A].raw = event.key.state == SDL_PRESSED;
											break;

										case SDL_SCANCODE_O:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_B].raw = event.key.state == SDL_PRESSED;
											break;

										case SDL_SCANCODE_0:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_X].raw = event.key.state == SDL_PRESSED;
											break;

										case SDL_SCANCODE_9:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_Y].raw = event.key.state == SDL_PRESSED;
											break;

										case SDL_SCANCODE_8:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_L].raw = event.key.state == SDL_PRESSED;
											break;

										case SDL_SCANCODE_7:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_L2].raw = event.key.state == SDL_PRESSED;
											break;

										case SDL_SCANCODE_L:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_L3].raw = event.key.state == SDL_PRESSED;
											break;

										case SDL_SCANCODE_MINUS:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_R].raw = event.key.state == SDL_PRESSED;
											break;

										case SDL_SCANCODE_EQUALS:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_R2].raw = event.key.state == SDL_PRESSED;
											break;

										case SDL_SCANCODE_SEMICOLON:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_R3].raw = event.key.state == SDL_PRESSED;
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
												retropad.buttons[RETRO_DEVICE_ID_JOYPAD_START].raw = event.key.state == SDL_PRESSED;
											}

											break;

										case SDL_SCANCODE_BACKSPACE:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_SELECT].raw = event.key.state == SDL_PRESSED;
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
									switch (event.cbutton.button)
									{
										case SDL_CONTROLLER_BUTTON_A:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_B].raw = event.cbutton.state == SDL_PRESSED;
											break;

										case SDL_CONTROLLER_BUTTON_B:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_A].raw = event.cbutton.state == SDL_PRESSED;
											break;

										case SDL_CONTROLLER_BUTTON_X:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_Y].raw = event.cbutton.state == SDL_PRESSED;
											break;

										case SDL_CONTROLLER_BUTTON_Y:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_Y].raw = event.cbutton.state == SDL_PRESSED;
											break;

										case SDL_CONTROLLER_BUTTON_BACK:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_SELECT].raw = event.cbutton.state == SDL_PRESSED;
											break;

										case SDL_CONTROLLER_BUTTON_GUIDE:
											if (event.cbutton.state == SDL_PRESSED)
												ToggleMenu();

											break;

										case SDL_CONTROLLER_BUTTON_START:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_START].raw = event.cbutton.state == SDL_PRESSED;
											break;

										case SDL_CONTROLLER_BUTTON_LEFTSTICK:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_L3].raw = event.cbutton.state == SDL_PRESSED;
											break;

										case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_R3].raw = event.cbutton.state == SDL_PRESSED;
											break;

										case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_L].raw = event.cbutton.state == SDL_PRESSED;
											break;

										case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_R].raw = event.cbutton.state == SDL_PRESSED;
											break;

										case SDL_CONTROLLER_BUTTON_DPAD_UP:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_UP].raw = event.cbutton.state == SDL_PRESSED;
											break;

										case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_DOWN].raw = event.cbutton.state == SDL_PRESSED;
											break;

										case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_LEFT].raw = event.cbutton.state == SDL_PRESSED;
											break;

										case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_RIGHT].raw = event.cbutton.state == SDL_PRESSED;
											break;
									}

									break;

								case SDL_CONTROLLERAXISMOTION:
									switch (event.caxis.axis)
									{
										case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_L2].raw = event.caxis.value >= 32767 / 4;
											break;

										case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
											retropad.buttons[RETRO_DEVICE_ID_JOYPAD_R2].raw = event.caxis.value >= 32767 / 4;
											break;
									}

									break;

								case SDL_CONTROLLERDEVICEADDED:
									SDL_GameControllerOpen(event.cdevice.which);
									break;
							}
						}

						for (size_t i = 0; i < sizeof(retropad.buttons) / sizeof(retropad.buttons[0]); ++i)
						{
							retropad.buttons[i].pressed = retropad.buttons[i].raw != retropad.buttons[i].held && retropad.buttons[i].raw;
							retropad.buttons[i].held = retropad.buttons[i].raw;
						}

						if (menu_open)
						{
							Menu_Update(menu);
						}
						else
						{
							if (!CoreRunner_Update())
								quit = true;
						}

						// Draw stuff
						Video_Clear();

						CoreRunner_Draw();

						if (menu_open)
							Menu_Draw(menu);

						Video_Display();

						// Delay until the next frame
						static double ticks_next;
						const Uint32 ticks_now = SDL_GetTicks();

						if (ticks_now < ticks_next)
							SDL_Delay(ticks_next - ticks_now);

						ticks_next = MAX(ticks_next, ticks_now) + 1000.0 / frames_per_second;
					}

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
