typedef struct Renderer_Texture
{
	SDL_Texture *sdl_texture;
	Renderer_Format format;
} Renderer_Texture;

typedef struct Renderer_Framebuffer
{
	Renderer_Texture texture;
} Renderer_Framebuffer;
