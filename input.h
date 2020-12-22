#pragma once

#include <stdbool.h>

typedef struct Retropad
{
	struct
	{
		bool pressed;
		bool held;
	} buttons[16];
} Retropad;

extern Retropad retropad;

void Input_Update(void);
