#include "renderer.h"

#include <stddef.h>

#include "SDL.h"

static SDL_Renderer *renderer;

/*************
* Main stuff *
*************/

SDL_Window* Renderer_Init(const char* const window_name, const size_t window_width, size_t const window_height, const Uint32 window_flags)
{
	SDL_Window* const window = SDL_CreateWindow(window_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, window_flags);

	if (window != NULL)
	{
		renderer = SDL_CreateRenderer(window, -1, 0);

		if (renderer != NULL)
			return window;

		SDL_DestroyWindow(window);
	}

	return NULL;
}

void Renderer_Deinit(void)
{
	SDL_DestroyRenderer(renderer);
}

void Renderer_Clear(void)
{
	SDL_RenderClear(renderer);
}

void Renderer_Display(void)
{
	SDL_RenderPresent(renderer);
}

void Renderer_WindowResized(const int width, const int height)
{
	(void)width;
	(void)height;
}

/****************
* Texture stuff *
****************/

cc_bool Renderer_TextureCreate(Renderer_Texture *texture, size_t width, size_t height, Renderer_Format format, cc_bool streaming)
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

void Renderer_TextureDestroy(Renderer_Texture *texture)
{
	SDL_DestroyTexture(texture->sdl_texture);
}

void Renderer_TextureUpdate(Renderer_Texture *texture, const void *pixels, const Renderer_Rect *rect)
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
			const unsigned char *pixels_pointer = (const unsigned char*)pixels;

			for (y = 0; y < rect->height; ++y)
			{
				size_t x;

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
		static const unsigned int sizes[] = {2, 4, 2, 4};

		SDL_UpdateTexture(texture->sdl_texture, &sdl_rect, pixels, rect->width * sizes[texture->format]);
	}
}

cc_bool Renderer_TextureLock(Renderer_Texture *texture, const Renderer_Rect *rect, unsigned char **buffer, size_t *pitch)
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

void Renderer_TextureUnlock(Renderer_Texture *texture)
{
	SDL_UnlockTexture(texture->sdl_texture);
}

void Renderer_TextureDraw(Renderer_Texture *texture, const Renderer_Rect *dst_rect, const Renderer_Rect *src_rect, Renderer_Colour colour)
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

void Renderer_ColourFill(const Renderer_Rect *rect, Renderer_Colour colour, unsigned char alpha)
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

void Renderer_DrawLine(size_t x1, size_t y1, size_t x2, size_t y2)
{
	SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}

/********************
* Framebuffer stuff *
********************/

cc_bool Renderer_FramebufferCreateSoftware(Renderer_Framebuffer* const framebuffer, const size_t width, const size_t height, const Renderer_Format format, const cc_bool streaming)
{
	return Renderer_TextureCreate(&framebuffer->texture, width, height, format, streaming);
}

cc_bool Renderer_FramebufferCreateHardware(Renderer_Framebuffer* const framebuffer, const size_t width, const size_t height, const cc_bool depth, const cc_bool stencil)
{
	(void)framebuffer;
	(void)width;
	(void)height;
	(void)depth;
	(void)stencil;

	return cc_false;
}

void Renderer_FramebufferDestroy(Renderer_Framebuffer* const framebuffer)
{
	Renderer_TextureDestroy(&framebuffer->texture);
}

Renderer_Texture* Renderer_FramebufferTexture(Renderer_Framebuffer* const framebuffer)
{
	return &framebuffer->texture;
}

void* Renderer_FramebufferNative(Renderer_Framebuffer* const framebuffer)
{
	(void)framebuffer;
	return NULL;
}
