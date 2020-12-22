#include "video.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "SDL.h"

struct Video_Texture
{
	SDL_Texture *sdl_texture;
	Video_Format format;
};

size_t window_width;
size_t window_height;

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
	Video_Texture *texture = malloc(sizeof(Video_Texture));

	if (texture != NULL)
	{
		const SDL_PixelFormatEnum sdl_formats[] = {SDL_PIXELFORMAT_ARGB1555, SDL_PIXELFORMAT_ARGB8888, SDL_PIXELFORMAT_RGB565, SDL_PIXELFORMAT_RGBA32};

		texture->sdl_texture = SDL_CreateTexture(renderer, sdl_formats[format], streaming ? SDL_TEXTUREACCESS_STREAMING : SDL_TEXTUREACCESS_STATIC, width, height);

		if (texture->sdl_texture != NULL)
		{
			texture->format = format;

			SDL_SetTextureBlendMode(texture->sdl_texture, format == VIDEO_FORMAT_A8 ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);

			return texture;
		}

		free(texture);
	}

	return NULL;
}

void Video_TextureDestroy(Video_Texture *texture)
{
	SDL_DestroyTexture(texture->sdl_texture);
	free(texture);
}

void Video_TextureUpdate(Video_Texture *texture, const void *pixels, size_t pitch, const Video_Rect *rect)
{
	SDL_Rect sdl_rect = {rect->x, rect->y, rect->width, rect->height};

	if (texture->format == VIDEO_FORMAT_A8)
	{
		unsigned char *rgba_pixels = malloc(rect->width * rect->height * 4);

		if (rgba_pixels != NULL)
		{
			unsigned char *rgba_pointer = rgba_pixels;

			for (size_t y = 0; y < rect->height; ++y)
			{
				const unsigned char *pixels_pointer = &((const unsigned char*)pixels)[y * pitch];

				for (size_t x = 0; x < rect->width; ++x)
				{
					*rgba_pointer++ = 0xFF;
					*rgba_pointer++ = 0xFF;
					*rgba_pointer++ = 0xFF;
					*rgba_pointer++ = *pixels_pointer++;
				}
			}

			SDL_UpdateTexture(texture->sdl_texture, &sdl_rect, rgba_pixels, rect->width * 4);

			free(rgba_pixels);
		}
	}
	else
	{
		SDL_UpdateTexture(texture->sdl_texture, &sdl_rect, pixels, pitch);
	}
}

bool Video_TextureLock(Video_Texture *texture, const Video_Rect *rect, unsigned char **buffer, size_t *pitch)
{
	SDL_Rect sdl_rect = {rect->x, rect->y, rect->width, rect->height};

	int int_pitch;

	bool success = SDL_LockTexture(texture->sdl_texture, &sdl_rect, (void**)buffer, &int_pitch) == 0;

	*pitch = (size_t)int_pitch;

	return success;
}

void Video_TextureUnlock(Video_Texture *texture)
{
	SDL_UnlockTexture(texture->sdl_texture);
}

void Video_TextureDraw(Video_Texture *texture, const Video_Rect *dst_rect, const Video_Rect *src_rect, unsigned char red, unsigned char green, unsigned char blue)
{
	SDL_Rect src_sdl_rect = {src_rect->x, src_rect->y, src_rect->width, src_rect->height};
	SDL_Rect dst_sdl_rect = {dst_rect->x, dst_rect->y, dst_rect->width, dst_rect->height};

	SDL_SetTextureColorMod(texture->sdl_texture, red, green, blue);

	SDL_RenderCopy(renderer, texture->sdl_texture, &src_sdl_rect, &dst_sdl_rect);
}
