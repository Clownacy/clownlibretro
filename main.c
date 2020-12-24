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
						if (!HandleEvents())
							quit = true;

						Input_Update();

						CoreRunner_Update();

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
