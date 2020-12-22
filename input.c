#include "input.h"

#include <stddef.h>

Retropad retropad;

void Input_Update(void)
{
	for (size_t i = 0; i < sizeof(retropad.buttons) / sizeof(retropad.buttons[0]); ++i)
		retropad.buttons[i].pressed = retropad.buttons[i].pressed != retropad.buttons[i].held && retropad.buttons[i].held;
}
