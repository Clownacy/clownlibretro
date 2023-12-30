#pragma once

#include "clowncommon/clowncommon.h"

#include "libretro.h"

typedef struct Retropad
{
	struct
	{
		cc_bool pressed;
		cc_bool held;
		cc_bool raw;
	} buttons[16];
} Retropad;

extern Retropad retropad;
