#include "video.h"

#include <stddef.h>

#include "SDL.h"

size_t window_width;
size_t window_height;

static SDL_Window *window;
static SDL_Renderer *renderer;

static cc_bool sdl_already_initialised;

/*************
* Main stuff *
*************/

cc_bool Video_Init(size_t window_width, size_t window_height)
{
	sdl_already_initialised = SDL_WasInit(SDL_INIT_VIDEO);

	if (sdl_already_initialised || SDL_InitSubSystem(SDL_INIT_VIDEO) == 0)
	{
		window = SDL_CreateWindow("clownlibretro", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

		if (window != NULL)
		{
			renderer = SDL_CreateRenderer(window, -1, 0);

			if (renderer != NULL)
			{
				Video_WindowResized();
				return cc_true;
			}

			SDL_DestroyWindow(window);
		}

		if (!sdl_already_initialised)
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
	}

	return cc_false;
}

void Video_Deinit(void)
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	if (!sdl_already_initialised)
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void Video_Clear(void)
{
	SDL_RenderClear(renderer);
}

void Video_Display(void)
{
	SDL_RenderPresent(renderer);
}

void Video_SetFullscreen(cc_bool fullscreen)
{
	SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);

	SDL_ShowCursor(fullscreen ? SDL_DISABLE : SDL_ENABLE);
}

void Video_WindowResized(void)
{
	int width, height;
	SDL_GetRendererOutputSize(renderer, &width, &height);
	window_width = width;
	window_height = height;
}

float Video_GetDPIScale(void)
{
	int renderer_width, window_width;
	SDL_GetRendererOutputSize(renderer, &renderer_width, NULL);
	SDL_GetWindowSize(window, &window_width, NULL);
	return (float)renderer_width / window_width;
}

/****************
* Texture stuff *
****************/

cc_bool Video_TextureCreate(Video_Texture *texture, size_t width, size_t height, Video_Format format, cc_bool streaming)
{
	static const Uint32 sdl_formats[] = {SDL_PIXELFORMAT_RGB555, SDL_PIXELFORMAT_RGB888, SDL_PIXELFORMAT_RGB565, SDL_PIXELFORMAT_RGBA32};

	texture->sdl_texture = SDL_CreateTexture(renderer, sdl_formats[format], streaming ? SDL_TEXTUREACCESS_STREAMING : SDL_TEXTUREACCESS_STATIC, width, height);

	if (texture->sdl_texture != NULL)
	{
		texture->format = format;

		SDL_SetTextureBlendMode(texture->sdl_texture, format == VIDEO_FORMAT_A8 ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);

		return cc_true;
	}

	return cc_false;
}

void Video_TextureDestroy(Video_Texture *texture)
{
	SDL_DestroyTexture(texture->sdl_texture);
}

void Video_TextureUpdate(Video_Texture *texture, const void *pixels, size_t pitch, const Video_Rect *rect)
{
	SDL_Rect sdl_rect;

	sdl_rect.x = rect->x;
	sdl_rect.y = rect->y;
	sdl_rect.w = rect->width;
	sdl_rect.h = rect->height;

	if (texture->format == VIDEO_FORMAT_A8)
	{
		unsigned char *rgba_pixels = (unsigned char*)SDL_malloc(rect->width * rect->height * 4);

		if (rgba_pixels != NULL)
		{
			size_t y;

			unsigned char *rgba_pointer = rgba_pixels;

			for (y = 0; y < rect->height; ++y)
			{
				size_t x;

				const unsigned char *pixels_pointer = &((const unsigned char*)pixels)[y * pitch];

				for (x = 0; x < rect->width; ++x)
				{
					*rgba_pointer++ = 0xFF;
					*rgba_pointer++ = 0xFF;
					*rgba_pointer++ = 0xFF;
					*rgba_pointer++ = *pixels_pointer++;
				}
			}

			SDL_UpdateTexture(texture->sdl_texture, &sdl_rect, rgba_pixels, rect->width * 4);

			SDL_free(rgba_pixels);
		}
	}
	else
	{
		SDL_UpdateTexture(texture->sdl_texture, &sdl_rect, pixels, pitch);
	}
}

cc_bool Video_TextureLock(Video_Texture *texture, const Video_Rect *rect, unsigned char **buffer, size_t *pitch)
{
	SDL_Rect sdl_rect;
	cc_bool success;
	int int_pitch;

	sdl_rect.x = rect->x;
	sdl_rect.y = rect->y;
	sdl_rect.w = rect->width;
	sdl_rect.h = rect->height;

	success = SDL_LockTexture(texture->sdl_texture, &sdl_rect, (void**)buffer, &int_pitch) == 0;

	*pitch = (size_t)int_pitch;

	return success;
}

void Video_TextureUnlock(Video_Texture *texture)
{
	SDL_UnlockTexture(texture->sdl_texture);
}

void Video_TextureDraw(Video_Texture *texture, const Video_Rect *dst_rect, const Video_Rect *src_rect, Video_Colour colour)
{
	SDL_Rect src_sdl_rect, dst_sdl_rect;

	src_sdl_rect.x = src_rect->x;
	src_sdl_rect.y = src_rect->y;
	src_sdl_rect.w = src_rect->width;
	src_sdl_rect.h = src_rect->height;

	dst_sdl_rect.x = dst_rect->x;
	dst_sdl_rect.y = dst_rect->y;
	dst_sdl_rect.w = dst_rect->width;
	dst_sdl_rect.h = dst_rect->height;

	SDL_SetTextureColorMod(texture->sdl_texture, colour.red, colour.green, colour.blue);

	SDL_RenderCopy(renderer, texture->sdl_texture, &src_sdl_rect, &dst_sdl_rect);
}

void Video_ColourFill(const Video_Rect *rect, Video_Colour colour, unsigned char alpha)
{
	SDL_Rect sdl_rect;

	sdl_rect.x = rect->x;
	sdl_rect.y = rect->y;
	sdl_rect.w = rect->width;
	sdl_rect.h = rect->height;

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, colour.red, colour.green, colour.blue, alpha);

	SDL_RenderFillRect(renderer, &sdl_rect);

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
}

void Video_DrawLine(size_t x1, size_t y1, size_t x2, size_t y2)
{
	SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}
