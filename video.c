#include "video.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "SDL.h"

static SDL_Window *window;
static SDL_Renderer *renderer;

bool Video_Init(size_t window_width, size_t window_height)
{
	window = SDL_CreateWindow("clownlibretro", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, SDL_WINDOW_RESIZABLE);

	if (window != NULL)
	{
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

		if (renderer != NULL)
			return true;

		SDL_DestroyWindow(window);
	}

	return false;
}

void Video_Deinit(void)
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}

void Video_Clear(void)
{
	SDL_RenderClear(renderer);
}

void Video_Display(void)
{
	SDL_RenderPresent(renderer);
}

void Video_SetFullscreen(bool fullscreen)
{
	SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);

	SDL_ShowCursor(fullscreen ? SDL_DISABLE : SDL_ENABLE);
}

Video_Texture* Video_TextureCreate(size_t width, size_t height, Video_Format format, bool streaming)
{
	SDL_PixelFormatEnum sdl_formats[] = {SDL_PIXELFORMAT_ARGB1555, SDL_PIXELFORMAT_ARGB8888, SDL_PIXELFORMAT_RGB565, SDL_PIXELFORMAT_RGBA32};

	SDL_Texture *texture = SDL_CreateTexture(renderer, sdl_formats[format], streaming ? SDL_TEXTUREACCESS_STREAMING : SDL_TEXTUREACCESS_STATIC, width, height);

	if (texture != NULL)
		if (format != VIDEO_FORMAT_RGBA32)
			SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);

	return (Video_Texture*)texture;
}

void Video_TextureDestroy(Video_Texture *texture)
{
	SDL_DestroyTexture((SDL_Texture*)texture);
}

void Video_TextureUpdate(Video_Texture *texture, const void *pixels, size_t pitch, const Video_Rect *rect)
{
	SDL_Rect sdl_rect = {rect->x, rect->y, rect->width, rect->height};

	SDL_UpdateTexture((SDL_Texture*)texture, &sdl_rect, pixels, pitch);
}

bool Video_TextureLock(Video_Texture *texture, const Video_Rect *rect, unsigned char **buffer, size_t *pitch)
{
	SDL_Rect sdl_rect = {rect->x, rect->y, rect->width, rect->height};

	int int_pitch;

	bool success = SDL_LockTexture((SDL_Texture*)texture, &sdl_rect, (void**)buffer, &int_pitch) == 0;

	*pitch = (size_t)int_pitch;

	return success;
}

void Video_TextureUnlock(Video_Texture *texture)
{
	SDL_UnlockTexture((SDL_Texture*)texture);
}

void Video_TextureDraw(Video_Texture *texture, const Video_Rect *dst_rect, const Video_Rect *src_rect, unsigned char red, unsigned char green, unsigned char blue)
{
	SDL_Rect src_sdl_rect = {src_rect->x, src_rect->y, src_rect->width, src_rect->height};
	SDL_Rect dst_sdl_rect = {dst_rect->x, dst_rect->y, dst_rect->width, dst_rect->height};

	SDL_SetTextureColorMod((SDL_Texture*)texture, red, green, blue);

	SDL_RenderCopy(renderer, (SDL_Texture*)texture, &src_sdl_rect, &dst_sdl_rect);
}
