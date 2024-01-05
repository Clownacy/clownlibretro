cmake_minimum_required(VERSION 3.0...3.14)

option(CLOWNLIBRETRO_STB_TRUETYPE_FONT_RENDERER "Use stb_truetype as the font renderer instead of FreeType" OFF)
set(CLOWNLIBRETRO_RENDERER SDL2 CACHE STRING "Which rendering API to use")
set_property(CACHE CLOWNLIBRETRO_RENDERER PROPERTY STRINGS SDL2 OpenGL3 OpenGLES2)
option(CLOWNLIBRETRO_DYNAMIC "Produce an executable that dynamically-links with its cores, otherwise produce a static library" ON)

project(clownlibretro LANGUAGES C)

if(CLOWNLIBRETRO_DYNAMIC)
	add_executable(clownlibretro)
else()
	add_library(clownlibretro)
endif()

target_sources(clownlibretro PRIVATE
	"src/audio.c"
	"src/audio.h"
	"src/core_runner.c"
	"src/core_runner.h"
	"src/error.c"
	"src/error.h"
	"src/file.c"
	"src/file.h"
	"src/font.c"
	"src/font.h"
	"src/input.c"
	"src/input.h"
	"src/libretro.h"
	"src/main.c"
	"src/menu.c"
	"src/menu.h"
	"src/renderer.c"
	"src/renderer.h"
	"src/video.c"
	"src/video.h"
)

target_compile_definitions(clownlibretro PRIVATE FREETYPE_FONTS)

find_package(SDL2 REQUIRED)
target_link_libraries(clownlibretro PRIVATE SDL2::SDL2main SDL2::SDL2)

if(CLOWNLIBRETRO_STB_TRUETYPE_FONT_RENDERER)
	target_sources(clownlibretro PRIVATE "src/stb_truetype.h")
	target_compile_definitions(clownlibretro PRIVATE FONT_STB)
else()
	find_package(Freetype REQUIRED)
	target_link_libraries(clownlibretro PRIVATE Freetype::Freetype)
endif()

if(CLOWNLIBRETRO_DYNAMIC)
	target_compile_definitions(clownlibretro PRIVATE DYNAMIC_CORE)
	target_sources(clownlibretro PRIVATE
		"src/core_loader.c"
		"src/core_loader.h"
	)
endif()

find_package(libzip)

if(libzip_FOUND)
	target_link_libraries(clownlibretro PRIVATE libzip::zip)
	target_compile_definitions(clownlibretro PRIVATE ENABLE_LIBZIP)
endif()

find_library(LIBM m)
if(LIBM)
	target_link_libraries(clownlibretro PRIVATE ${LIBM})
endif()

if(CLOWNLIBRETRO_RENDERER STREQUAL "OpenGL3")
	find_package(OpenGL REQUIRED)
	target_link_libraries(clownlibretro PRIVATE OpenGL::OpenGL)
	target_compile_definitions(clownlibretro PRIVATE RENDERER_OPENGL3)
	target_include_directories(clownlibretro PRIVATE "src/glad/include")
	target_sources(clownlibretro PRIVATE "src/glad/src/glad.c")
elseif(CLOWNLIBRETRO_RENDERER STREQUAL "OpenGLES2")
	find_package(OpenGL REQUIRED)
	target_link_libraries(clownlibretro PRIVATE OpenGL::GLES2)
	target_compile_definitions(clownlibretro PRIVATE RENDERER_OPENGLES2)
	target_include_directories(clownlibretro PRIVATE "src/glad/include")
	target_sources(clownlibretro PRIVATE "src/glad/src/glad.c")
endif()
