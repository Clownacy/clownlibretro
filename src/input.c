#include "input.h"

#include <assert.h>

#define AXIS_MAX 0x7FFF

Retropad retropad;

void Input_SetButtonDigital(const unsigned int index, const cc_bool held)
{
	Input_SetButtonAnalog(index, held ? AXIS_MAX : 0);
}

void Input_SetButtonAnalog(const unsigned int index, const short axis)
{
	assert(index < CC_COUNT_OF(retropad.buttons));

	retropad.buttons[index].axis = axis;
	retropad.buttons[index].held = axis >= AXIS_MAX / 4;
}
