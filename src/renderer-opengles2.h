#include <stddef.h>

#include <glad/glad.h>

typedef struct Renderer_Texture
{
	GLuint id;
	Renderer_Format format;
	unsigned char *lock_buffer;
	size_t width, height;
	Renderer_Rect lock_rect;
	unsigned char *convert_buffer;
} Renderer_Texture;

typedef struct Renderer_Framebuffer
{
	Renderer_Texture texture;
	GLuint id, depth_renderbuffer_id, stencil_renderbuffer_id;

	Renderer_Format format;
	unsigned char *lock_buffer;
	size_t width, height;
	Renderer_Rect lock_rect;
	unsigned char *convert_buffer;
} Renderer_Framebuffer;
