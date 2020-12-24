#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "SDL.h"

#include "core_runner.h"
#include "event.h"
#include "input.h"
//#include "menu.h"
//#include "menu2.h"
#include "video.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static double frames_per_second;

static bool quit;

  //////////
 // Main //
//////////

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
		if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO) < 0)
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
				window_width = 640;//system_av_info.geometry.base_width;
				window_height = 480;//system_av_info.geometry.base_height;

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

							if (event.key.keysym.sym == SDLK_LALT)
								alt_held = event.key.state == SDL_PRESSED;

							switch (event.type)
							{
								case SDL_QUIT:
									quit = true;
									break;

								case SDL_WINDOWEVENT:
									switch (event.window.event)
									{
										case SDL_WINDOWEVENT_SIZE_CHANGED:
											window_width = event.window.data1;
											window_height = event.window.data2;
											break;
									}

									break;

								case SDL_KEYDOWN:
								case SDL_KEYUP:
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

										default:
											break;
									}

									break;
							}
						}


						Input_Update();

						if (!CoreRunner_Update())
							quit = true;

						// Draw stuff
						Video_Clear();

						CoreRunner_Draw();

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

				Video_Deinit();
			}

			SDL_Quit();
		}
	}

	return main_return;
}
