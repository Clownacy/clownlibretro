#pragma once

#include <stdbool.h>

typedef struct Retropad
{
	bool buttons[16];
} Retropad;

extern Retropad retropad;
