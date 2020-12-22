#include "event.h"

#include <stdbool.h>

#include "SDL.h"

#include "input.h"
#include "libretro.h"
#include "video.h"

bool HandleEvents(void)
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		static bool alt_held;

		if (event.key.keysym.sym == SDLK_LALT)
			alt_held = event.key.state == SDL_PRESSED;

		switch (event.type)
		{
			case SDL_QUIT:
				return false;

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
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_UP] = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_A:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_LEFT] = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_S:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_DOWN] = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_D:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_RIGHT] = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_P:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_A] = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_O:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_B] = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_0:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_X] = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_9:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_Y] = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_8:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_L] = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_7:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_L2] = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_L:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_L3] = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_MINUS:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_R] = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_EQUALS:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_R2] = event.key.state == SDL_PRESSED;
						break;

					case SDL_SCANCODE_SEMICOLON:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_R3] = event.key.state == SDL_PRESSED;
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
							retropad.buttons[RETRO_DEVICE_ID_JOYPAD_START] = event.key.state == SDL_PRESSED;
						}

						break;

					case SDL_SCANCODE_BACKSPACE:
						retropad.buttons[RETRO_DEVICE_ID_JOYPAD_SELECT] = event.key.state == SDL_PRESSED;
						break;

					default:
						break;
				}

				break;
		}
	}

	return true;
}
