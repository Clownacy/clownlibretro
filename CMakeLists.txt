cmake_minimum_required(VERSION 3.0...3.14)

project(clownlibretro LANGUAGES C)

add_executable(clownlibretro
	"src/audio.c"
	"src/audio.h"
	"src/core_runner.c"
	"src/core_runner.h"
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
	"src/video.c"
	"src/video.h"
)

target_compile_definitions(clownlibretro PRIVATE FREETYPE_FONTS)

find_package(SDL2 REQUIRED)
target_link_libraries(clownlibretro PRIVATE SDL2::SDL2)

find_package(Freetype REQUIRED)
target_link_libraries(clownlibretro PRIVATE Freetype::Freetype)

find_library(LIBM m)
if(LIBM)
	target_link_libraries(clownlibretro PRIVATE ${LIBM})
endif()
