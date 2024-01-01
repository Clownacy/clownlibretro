#include "video.h"

#include <stddef.h>

#include "SDL.h"

#include "error.h"

size_t window_width;
size_t window_height;

static SDL_Window *window;

static cc_bool sdl_already_initialised;

/*************
* Main stuff *
*************/

cc_bool Video_Init(size_t window_width, size_t window_height)
{
	sdl_already_initialised = SDL_WasInit(SDL_INIT_VIDEO);

	if (sdl_already_initialised || SDL_InitSubSystem(SDL_INIT_VIDEO) == 0)
	{
		window = Renderer_Init("clownlibretro", window_width, window_height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

		if (window == NULL)
		{
			PrintError("Renderer_Init failed: %s", SDL_GetError());
		}
		else
		{
			Video_WindowResized();

			return cc_true;

			/*SDL_DestroyWindow(window);*/
		}

		if (!sdl_already_initialised)
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
	}

	return cc_false;
}

void Video_Deinit(void)
{
	Renderer_Deinit();

	SDL_DestroyWindow(window);

	if (!sdl_already_initialised)
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void Video_SetFullscreen(cc_bool fullscreen)
{
	SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);

	SDL_ShowCursor(fullscreen ? SDL_DISABLE : SDL_ENABLE);
}

void Video_WindowResized(void)
{
	int width, height;
	SDL_GetWindowSizeInPixels(window, &width, &height);
	window_width = width;
	window_height = height;
	Renderer_WindowResized(width, height);
}

float Video_GetDPIScale(void)
{
	int renderer_width, window_width;
	SDL_GetWindowSizeInPixels(window, &renderer_width, NULL);
	SDL_GetWindowSize(window, &window_width, NULL);
	return (float)renderer_width / window_width;
}
