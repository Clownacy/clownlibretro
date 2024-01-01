#include "renderer.h"

#ifdef RENDERER_OPENGLES2
	#include "renderer-opengles2.c"
#else
	#include "renderer-sdl.c"
#endif
