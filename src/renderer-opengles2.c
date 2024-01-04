#include "renderer.h"

#include <stddef.h>

#include "SDL.h"

#include "error.h"

#define VERTEX_ATTRIBUTE_POSITION 0
#define VERTEX_ATTRIBUTE_TEXTURE_COORDINATES 1
#define VERTEX_ATTRIBUTE_COLOUR 2

typedef struct Vertex
{
	GLfloat position[2];
	GLfloat texture_coordinates[2];
	GLubyte colour[4];
} Vertex;

static size_t window_width, window_height;

static SDL_Window *window;
static SDL_GLContext context;

static Renderer_Texture colour_fill_texture;

static GLuint program;
#ifndef RENDERER_OPENGLES2
static GLuint vertex_array_object;
#endif
static GLuint vertex_buffer_object;
static GLint previous_vertex_buffer_object;

#if 0
static void CheckError(void)
{
	switch (glGetError())
	{
		case GL_NO_ERROR:
			PrintDebug("GL_NO_ERROR");
			break;

		case GL_INVALID_ENUM:
			PrintDebug("GL_INVALID_ENUM");
			break;

		case GL_INVALID_VALUE:
			PrintDebug("GL_INVALID_VALUE");
			break;

		case GL_INVALID_OPERATION:
			PrintDebug("GL_INVALID_OPERATION");
			break;

		case GL_INVALID_FRAMEBUFFER_OPERATION:
			PrintDebug("GL_INVALID_FRAMEBUFFER_OPERATION");
			break;

		case GL_OUT_OF_MEMORY:
			PrintDebug("GL_OUT_OF_MEMORY");
			break;
	}
}
#endif

static void APIENTRY DebugCallback(GLenum source,
            GLenum type,
            GLuint id,
            GLenum severity,
            GLsizei length,
            const GLchar *message,
            const void *userParam)
{
	if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
		PrintDebug("%s", message);
	else
		PrintWarning("%s", message);
}

static GLuint CompileShader(const GLenum shader_type, const GLchar* const shader_source)
{
	const GLuint shader = glCreateShader(shader_type);

	if (shader != 0)
	{
		GLint shader_compiled;

		glShaderSource(shader, 1, &shader_source, NULL);
		glCompileShader(shader);

		glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_compiled);

		if (shader_compiled != GL_TRUE)
		{
			GLchar log_buffer[0x100];

			glGetShaderInfoLog(shader, sizeof(log_buffer), NULL, log_buffer);
			PrintError("Shader compiling error: %s", log_buffer);
		}
		else
		{
			return shader;
		}

		glDeleteShader(shader);
	}

	return 0;
}

static GLuint CompileProgram(const GLchar* const vertex_shader_source, const GLchar* const fragment_shader_source)
{
	const GLuint program = glCreateProgram();
	const GLuint vertex_shader = CompileShader(GL_VERTEX_SHADER, vertex_shader_source);
	const GLuint fragment_shader = CompileShader(GL_FRAGMENT_SHADER, fragment_shader_source);

	if (program == 0)
	{
		PrintError("glCreateProgram failed");
	}
	else if (vertex_shader == 0)
	{
		PrintError("CreateShader(GL_VERTEX_SHADER) failed");
	}
	else if (fragment_shader == 0)
	{
		PrintError("CreateShader(GL_FRAGMENT_SHADER) failed");
	}
	else
	{
		GLint program_linked;

		glBindAttribLocation(program, VERTEX_ATTRIBUTE_POSITION, "input_vertex_coordinates");
		glBindAttribLocation(program, VERTEX_ATTRIBUTE_TEXTURE_COORDINATES, "input_texture_coordinates");
		glBindAttribLocation(program, VERTEX_ATTRIBUTE_COLOUR, "input_colour");

		glAttachShader(program, vertex_shader);
		glDeleteShader(vertex_shader);
		glAttachShader(program, fragment_shader);
		glDeleteShader(fragment_shader);
		glLinkProgram(program);

		glGetProgramiv(program, GL_LINK_STATUS, &program_linked);

		if (program_linked != GL_TRUE)
		{
			GLchar log_buffer[0x100];
			glGetProgramInfoLog(program, sizeof(log_buffer), NULL, log_buffer);
			PrintError("Program linking error: %s", log_buffer);
		}
		else
		{
			return program;
		}
	}

	glDeleteShader(fragment_shader);
	glDeleteShader(vertex_shader);
	glDeleteProgram(program);

	return 0;
}

/*************
* Main stuff *
*************/

SDL_Window* Renderer_Init(const char* const window_name, const size_t window_width, const size_t window_height, const Uint32 window_flags)
{
#ifdef RENDERER_OPENGLES2
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0
	#ifdef RENDERER_OPENGL3
		| SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG
	#endif
	#ifndef NDEBUG
		| SDL_GL_CONTEXT_DEBUG_FLAG
	#endif
		);

	window = SDL_CreateWindow(window_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, window_flags | SDL_WINDOW_OPENGL);

	if (window == NULL)
	{
		PrintError("SDL_CreateWindow failed: %s", SDL_GetError());
	}
	else
	{
		context = SDL_GL_CreateContext(window);

		if (context == NULL)
		{
			PrintError("SDL_GL_CreateContext failed: %s", SDL_GetError());
		}
		else
		{
			/* TODO: Make these compatible with Desktop OpenGL. */
			static const GLchar vertex_shader_source[] = " \
				#version 100\n \
				attribute vec2 input_vertex_coordinates; \
				attribute vec2 input_texture_coordinates; \
				attribute vec4 input_colour; \
				varying vec2 texture_coordinates; \
				varying vec4 colour; \
				void main() \
				{ \
					gl_Position = vec4(input_vertex_coordinates.xy, 0.0, 1.0); \
					texture_coordinates = input_texture_coordinates; \
					colour = input_colour; \
				} \
			";

			static const GLchar fragment_shader_source[] = " \
				#version 100\n \
				precision mediump float; \
				varying vec2 texture_coordinates; \
				varying vec4 colour; \
				uniform sampler2D sampler; \
				void main() \
				{ \
					gl_FragColor = texture2D(sampler, texture_coordinates) * colour, 1.0; \
				} \
			";

		#ifdef RENDERER_OPENGLES2
			gladLoadGLES2Loader(SDL_GL_GetProcAddress);
		#else
			gladLoadGLLoader(SDL_GL_GetProcAddress);
		#endif
		#ifndef NDEBUG
			if (GLAD_GL_KHR_debug)
			{
				glEnable(GL_DEBUG_OUTPUT);
				glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
				glDebugMessageCallback(DebugCallback, NULL);
			}
		#endif

			program = CompileProgram(vertex_shader_source, fragment_shader_source);

			if (program == 0)
			{
				PrintError("CompileProgram failed");
			}
			else
			{
			#ifndef RENDERER_OPENGLES2
				glGenVertexArrays(1, &vertex_array_object);
			#endif

				glGenBuffers(1, &vertex_buffer_object);

				if (!Renderer_TextureCreate(&colour_fill_texture, 1, 1, VIDEO_FORMAT_RGB565, cc_false))
				{
					PrintError("Failed to create colour-fill texture");
				}
				else
				{
					const GLushort white_pixel = 0xFFFF;
					const Renderer_Rect colour_fill_texture_rect = {0, 0, 1, 1};

					Renderer_TextureUpdate(&colour_fill_texture, &white_pixel, &colour_fill_texture_rect);

					return window;

					/*Renderer_TextureDestroy(&colour_fill_texture);*/
				}

				glDeleteBuffers(1, &vertex_buffer_object);
			#ifndef RENDERER_OPENGLES2
				glDeleteVertexArrays(1, &vertex_array_object);
			#endif
				glDeleteProgram(program);
			}

			SDL_GL_DeleteContext(context);
		}

		SDL_DestroyWindow(window);
	}

	return NULL;
}

void Renderer_Deinit(void)
{
	Renderer_TextureDestroy(&colour_fill_texture);

	glDeleteBuffers(1, &vertex_buffer_object);
#ifndef RENDERER_OPENGLES2
	glDeleteVertexArrays(1, &vertex_array_object);
#endif
	glDeleteProgram(program);

	SDL_GL_DeleteContext(context);
}

void Renderer_Clear(void)
{
	/* Reset a bunch of state that the libretro core probably screwed with. */
	glViewport(0, 0, window_width, window_height);

	glActiveTexture(GL_TEXTURE0);

	glUseProgram(program);

#ifndef RENDERER_OPENGLES2
	glBindVertexArray(vertex_array_object);
#endif
	/* For some reason, we need to preserve the current-bound vertex buffer object for GLideN64 to work. */
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previous_vertex_buffer_object);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
	glEnableVertexAttribArray(VERTEX_ATTRIBUTE_POSITION);
	glVertexAttribPointer(VERTEX_ATTRIBUTE_POSITION, CC_COUNT_OF(((Vertex*)0)->position), GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(VERTEX_ATTRIBUTE_TEXTURE_COORDINATES);
	glVertexAttribPointer(VERTEX_ATTRIBUTE_TEXTURE_COORDINATES, CC_COUNT_OF(((Vertex*)0)->texture_coordinates), GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texture_coordinates));
	glEnableVertexAttribArray(VERTEX_ATTRIBUTE_COLOUR);
	glVertexAttribPointer(VERTEX_ATTRIBUTE_COLOUR, CC_COUNT_OF(((Vertex*)0)->colour), GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)offsetof(Vertex, colour));

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_STENCIL_TEST);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	/* Finally, clear. */
	glClearColor(0, 0, 0, 0xFF);
	glClear(GL_COLOR_BUFFER_BIT /*| GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT*/);
}

void Renderer_Display(void)
{
	SDL_GL_SwapWindow(window);
	glBindBuffer(GL_ARRAY_BUFFER, previous_vertex_buffer_object);
}

void Renderer_WindowResized(const int width, const int height)
{
	window_width = width;
	window_height = height;
}

/****************
* Texture stuff *
****************/

static unsigned int BytesPerPixel(const Renderer_Format format)
{
	return
		format == VIDEO_FORMAT_0RGB1555 ? 2 :
		format == VIDEO_FORMAT_XRGB8888 ? 4 :
		format == VIDEO_FORMAT_RGB565 ? 2 :
		/*format == VIDEO_FORMAT_A8 ?*/ 2;
}

static GLenum TextureFormat(const Renderer_Format format)
{
	return
		format == VIDEO_FORMAT_0RGB1555 ? GL_RGBA :
		format == VIDEO_FORMAT_XRGB8888 ? GL_RGBA :
		format == VIDEO_FORMAT_RGB565 ? GL_RGB :
#ifdef RENDERER_OPENGLES2
		/*format == VIDEO_FORMAT_A8 ?*/ GL_LUMINANCE_ALPHA;
#else
		/*format == VIDEO_FORMAT_A8 ?*/ GL_RG;/* TODO: GL_LUMINANCE_ALPHA; */
#endif
}

static GLenum TextureType(const Renderer_Format format)
{
	return
		format == VIDEO_FORMAT_0RGB1555 ? GL_UNSIGNED_SHORT_5_5_5_1 :
		format == VIDEO_FORMAT_XRGB8888 ? GL_UNSIGNED_BYTE :
		format == VIDEO_FORMAT_RGB565 ? GL_UNSIGNED_SHORT_5_6_5 :
		/*format == VIDEO_FORMAT_A8 ?*/ GL_UNSIGNED_BYTE;
}

cc_bool Renderer_TextureCreate(Renderer_Texture *texture, size_t width, size_t height, Renderer_Format format, cc_bool streaming)
{
	const unsigned int bytes_per_pixel = BytesPerPixel(format);
	const GLenum opengl_format = TextureFormat(format);
	const GLenum opengl_type = TextureType(format);

	texture->format = format;
	texture->width = width;
	texture->height = height;

	glGenTextures(1, &texture->id);

	glBindTexture(GL_TEXTURE_2D, texture->id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, opengl_format, width, height, 0, opengl_format, opengl_type, NULL);

	if (streaming)
	{
		texture->lock_buffer = (unsigned char*)SDL_malloc(width * height * bytes_per_pixel);

		return texture->lock_buffer != NULL;
	}
	else
	{
		return cc_true;
	}
}

void Renderer_TextureDestroy(Renderer_Texture *texture)
{
	SDL_free(texture->lock_buffer);
	glDeleteTextures(1, &texture->id);
}

void Renderer_TextureUpdate(Renderer_Texture *texture, const void *pixels, const Renderer_Rect *rect)
{
	const GLint alignments[8] = {8, 1, 2, 1, 4, 1, 2, 1};

	glPixelStorei(GL_UNPACK_ALIGNMENT, alignments[rect->width % CC_COUNT_OF(alignments)]);
	glBindTexture(GL_TEXTURE_2D, texture->id);

	if (texture->format == VIDEO_FORMAT_A8)
	{
		unsigned char *converted_pixels = (unsigned char*)SDL_malloc(rect->width * rect->height * 2);

		if (converted_pixels != NULL)
		{
			size_t y;

			unsigned char *output_pointer = converted_pixels;
			const unsigned char *input_pointer = (const unsigned char*)pixels;

			for (y = 0; y < rect->height; ++y)
			{
				size_t x;

				for (x = 0; x < rect->width; ++x)
				{
					*output_pointer++ = 0xFF;
					*output_pointer++ = *input_pointer++;
				}
			}

			glTexSubImage2D(GL_TEXTURE_2D, 0, rect->x, rect->y, rect->width, rect->height, TextureFormat(texture->format), TextureType(texture->format), converted_pixels);

			SDL_free(converted_pixels);
		}
	}
	else if (texture->format == VIDEO_FORMAT_XRGB8888)
	{
		unsigned char *converted_pixels = (unsigned char*)SDL_malloc(rect->width * rect->height * 4);

		if (converted_pixels != NULL)
		{
			size_t y;

			unsigned char *output_pointer = converted_pixels;
			const GLuint *input_pointer = (const GLuint*)pixels;

			for (y = 0; y < rect->height; ++y)
			{
				size_t x;

				for (x = 0; x < rect->width; ++x)
				{
					const GLuint pixel = *input_pointer++;
					*output_pointer++ = (pixel >> 8 * 2) & 0xFF;
					*output_pointer++ = (pixel >> 8 * 1) & 0xFF;
					*output_pointer++ = (pixel >> 8 * 0) & 0xFF;
					*output_pointer++ = (pixel >> 8 * 3) & 0xFF;
				}
			}

			glTexSubImage2D(GL_TEXTURE_2D, 0, rect->x, rect->y, rect->width, rect->height, TextureFormat(texture->format), TextureType(texture->format), converted_pixels);

			SDL_free(converted_pixels);
		}
	}
	else
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, rect->x, rect->y, rect->width, rect->height, TextureFormat(texture->format), TextureType(texture->format), pixels);
	}
}

cc_bool Renderer_TextureLock(Renderer_Texture *texture, const Renderer_Rect *rect, unsigned char **buffer, size_t *pitch)
{
	const unsigned int bytes_per_pixel = BytesPerPixel(texture->format);

	if (texture->lock_buffer == NULL)
		return cc_false;

	*buffer = texture->lock_buffer;
	*pitch = rect->width * bytes_per_pixel;
	texture->lock_rect = *rect;

	return cc_true;
}

void Renderer_TextureUnlock(Renderer_Texture *texture)
{
	Renderer_TextureUpdate(texture, texture->lock_buffer, &texture->lock_rect);
}

static void Renderer_TextureDrawAlpha(Renderer_Texture *texture, const Renderer_Rect *dst_rect, const Renderer_Rect *src_rect, Renderer_Colour colour, const unsigned char alpha)
{
	Vertex vertices[4];

#define DO_VERTEX(VERTEX_INDEX, SRC_X_OFFSET, SRC_Y_OFFSET, DST_X_OFFSET, DST_Y_OFFSET)\
	vertices[VERTEX_INDEX].position[0] = ((GLfloat)(dst_rect->x + DST_X_OFFSET) / window_width - 0.5f) * 2.0f;\
	vertices[VERTEX_INDEX].position[1] = ((GLfloat)(dst_rect->y + DST_Y_OFFSET) / window_height - 0.5f) * -2.0f;\
	vertices[VERTEX_INDEX].texture_coordinates[0] = (GLfloat)(src_rect->x + SRC_X_OFFSET) / texture->width;\
	vertices[VERTEX_INDEX].texture_coordinates[1] = (GLfloat)(src_rect->y + SRC_Y_OFFSET) / texture->height;\
	vertices[VERTEX_INDEX].colour[0] = colour.red;\
	vertices[VERTEX_INDEX].colour[1] = colour.green;\
	vertices[VERTEX_INDEX].colour[2] = colour.blue;\
	vertices[VERTEX_INDEX].colour[3] = alpha

	DO_VERTEX(0, 0, 0, 0, 0);
	DO_VERTEX(1, src_rect->width, 0, dst_rect->width, 0);
	DO_VERTEX(2, 0, src_rect->height, 0, dst_rect->height);
	DO_VERTEX(3, src_rect->width, src_rect->height, dst_rect->width, dst_rect->height);

#undef DO_VERTEX

	if (texture->format == VIDEO_FORMAT_A8)
		glEnable(GL_BLEND);
	else
		glDisable(GL_BLEND);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
	glBindTexture(GL_TEXTURE_2D, texture->id);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, CC_COUNT_OF(vertices));
}

void Renderer_TextureDraw(Renderer_Texture *texture, const Renderer_Rect *dst_rect, const Renderer_Rect *src_rect, Renderer_Colour colour)
{
	Renderer_TextureDrawAlpha(texture, dst_rect, src_rect, colour, 0xFF);
}

void Renderer_ColourFill(const Renderer_Rect *rect, Renderer_Colour colour, unsigned char alpha)
{
	const Renderer_Rect fake_rect = {0, 0, 1, 1};

	Renderer_TextureDrawAlpha(&colour_fill_texture, rect, &fake_rect, colour, alpha);
}

void Renderer_DrawLine(size_t x1, size_t y1, size_t x2, size_t y2)
{
	/* TODO: This only works for horizontal lines. */
	Renderer_Rect rect;

	const Renderer_Colour black = {0, 0, 0};

	(void)y2;

	rect.x = x1;
	rect.y = y1;
	rect.width = x2 - x1;
	rect.height = 1;

	Renderer_ColourFill(&rect, black, 0xFF);
}

/********************
* Framebuffer stuff *
********************/

cc_bool Renderer_FramebufferCreateSoftware(Renderer_Framebuffer* const framebuffer, const size_t width, const size_t height, const Renderer_Format format, const cc_bool streaming)
{
	framebuffer->id = 0;
	framebuffer->depth_renderbuffer_id = 0;
	framebuffer->stencil_renderbuffer_id = 0;

	return Renderer_TextureCreate(&framebuffer->texture, width, height, format, streaming);
}

cc_bool Renderer_FramebufferCreateHardware(Renderer_Framebuffer* const framebuffer, const size_t width, const size_t height, const cc_bool depth, const cc_bool stencil)
{
	PrintDebug("width %u height %u", (unsigned int)width, (unsigned int)height); /* TODO: Remove this */
	if (Renderer_TextureCreate(&framebuffer->texture, width, height, VIDEO_FORMAT_XRGB8888, cc_false))
	{
		framebuffer->depth_renderbuffer_id = 0;
		framebuffer->stencil_renderbuffer_id = 0;

		glGenFramebuffers(1, &framebuffer->id);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->id);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer->texture.id, 0);

		if (depth)
		{
			/* TODO: GL_DEPTH24_STENCIL8 is an extension in OpenGL ES 2.0. Hope and pray. */
			glGenRenderbuffers(1, &framebuffer->depth_renderbuffer_id);
			glBindRenderbuffer(GL_RENDERBUFFER, framebuffer->depth_renderbuffer_id);
			glRenderbufferStorage(GL_RENDERBUFFER, stencil ? GL_DEPTH24_STENCIL8 : GL_DEPTH_COMPONENT16, width, height);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebuffer->depth_renderbuffer_id);
		}

		/* TODO: Get rid of this. */
		switch (glCheckFramebufferStatus(GL_FRAMEBUFFER))
		{
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
				PrintDebug("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
				break;

			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
				PrintDebug("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
				break;
/*
			case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
				PrintDebug("GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS");
				break;
*/
			case GL_FRAMEBUFFER_UNSUPPORTED:
				PrintDebug("GL_FRAMEBUFFER_UNSUPPORTED");
				break;
		}

		return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	}

	return cc_false;
}

void Renderer_FramebufferDestroy(Renderer_Framebuffer* const framebuffer)
{
	glDeleteRenderbuffers(1, &framebuffer->stencil_renderbuffer_id);
	glDeleteRenderbuffers(1, &framebuffer->depth_renderbuffer_id);
	glDeleteFramebuffers(1, &framebuffer->id);

	Renderer_TextureDestroy(&framebuffer->texture);
}

Renderer_Texture* Renderer_FramebufferTexture(Renderer_Framebuffer* const framebuffer)
{
	return &framebuffer->texture;
}

void* Renderer_FramebufferNative(Renderer_Framebuffer* const framebuffer)
{
	return (void*)(GLsizeiptr)framebuffer->id;
}
