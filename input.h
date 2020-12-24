#pragma once

#include <stdbool.h>

#include "libretro.h"

typedef struct Retropad
{
	struct
	{
		bool pressed;
		bool held;
	} buttons[16];
} Retropad;

extern Retropad retropad;
