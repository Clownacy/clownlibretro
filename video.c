#include "video.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "SDL.h"

#include "libretro.h"

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;

static size_t size_of_texture_pixel;

static size_t output_width;
static size_t output_height;

bool Video_Init(const struct retro_game_geometry *geometry, enum retro_pixel_format pixel_format)
{
	SDL_PixelFormatEnum surface_format;

	switch (pixel_format)
	{
		case RETRO_PIXEL_FORMAT_0RGB1555:
			surface_format = SDL_PIXELFORMAT_ARGB1555;
			size_of_texture_pixel = 2;
			break;

		case RETRO_PIXEL_FORMAT_XRGB8888:
			surface_format = SDL_PIXELFORMAT_ARGB8888;
			size_of_texture_pixel = 4;
			break;

		case RETRO_PIXEL_FORMAT_RGB565:
			surface_format = SDL_PIXELFORMAT_RGB565;
			size_of_texture_pixel = 2;
			break;

		default:
			return false;
	}

	window = SDL_CreateWindow("clownlibretro", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, geometry->base_width, geometry->base_height, SDL_WINDOW_RESIZABLE);

	if (window != NULL)
	{
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

		if (renderer != NULL)
		{
			SDL_RenderSetLogicalSize(renderer, geometry->base_width, geometry->base_height);

			texture = SDL_CreateTexture(renderer, surface_format, SDL_TEXTUREACCESS_STREAMING, geometry->max_width, geometry->max_height);

			if (texture != NULL)
			{
				SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);

				output_width = geometry->base_width;
				output_height = geometry->base_height;

				return true;
			}

			SDL_DestroyRenderer(renderer);
		}

		SDL_DestroyWindow(window);
	}

	return false;
}

void Video_Deinit(void)
{
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}

void Video_CoreRefresh(const void *data, unsigned int width, unsigned int height, size_t pitch)
{
	const unsigned char *source_pixels = data;
	unsigned char *destination_pixels;

	SDL_Rect rect = {0, 0, width, height};
	int destination_pitch;

	if (SDL_LockTexture(texture, &rect, (void**)&destination_pixels, &destination_pitch) == 0)
	{
		for (unsigned int y = 0; y < height; ++y)
			memcpy(&destination_pixels[destination_pitch * y], &source_pixels[pitch * y], width * size_of_texture_pixel);

		SDL_UnlockTexture(texture);
	}
}

void Video_Display(void)
{
	SDL_Rect rect = {0, 0, output_width, output_height};

	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, &rect, NULL);
	SDL_RenderPresent(renderer);
}

void Video_SetOutputSize(unsigned int width, unsigned int height)
{
	output_width = width;
	output_height = height;

	SDL_RenderSetLogicalSize(renderer, width, height);
}

void Video_SetFullscreen(bool fullscreen)
{
	SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);

	SDL_ShowCursor(fullscreen ? SDL_DISABLE : SDL_ENABLE);
}

Video_Texture* Video_TextureCreate(size_t width, size_t height)
{
	return (Video_Texture*)SDL_CreateTexture(renderer, SDL_TEXTUREACCESS_STATIC, SDL_PIXELFORMAT_RGBA32, width, height);
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

void Video_TextureDraw(Video_Texture *texture, size_t dst_x, size_t dst_y, const Video_Rect *src_rect, unsigned char red, unsigned char green, unsigned char blue)
{
	SDL_Rect src_sdl_rect = {src_rect->x, src_rect->y, src_rect->width, src_rect->height};
	SDL_Rect dst_sdl_rect = {dst_x, dst_y, src_rect->width, src_rect->height};

	SDL_SetTextureColorMod((SDL_Texture*)texture, red, green, blue);

	SDL_RenderCopy(renderer, (SDL_Texture*)texture, &src_sdl_rect, &dst_sdl_rect);
}
