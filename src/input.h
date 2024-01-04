#pragma once

#include "clowncommon/clowncommon.h"

#include "libretro.h"

typedef struct Retropad
{
	struct
	{
		cc_bool pressed;
		cc_bool held;
		short axis;
	} buttons[16];
	struct
	{
		short axis[2];
	} sticks[2];
} Retropad;

extern Retropad retropad;

void Input_SetButtonDigital(unsigned int index, cc_bool held);
void Input_SetButtonAnalog(unsigned int index, short axis);
