#include <stddef.h>

#include <GLES2/gl2.h>

typedef struct Renderer_Texture
{
	GLuint id;
	Renderer_Format format;
	unsigned char *lock_buffer;
	size_t width, height;
	Renderer_Rect lock_rect;
	unsigned char *convert_buffer;
} Renderer_Texture;
