#include "renderer.h"

#if defined(RENDERER_OPENGL3) || defined(RENDERER_OPENGLES2)
	#include "renderer-opengles2.c"
#else
	#include "renderer-sdl.c"
#endif
