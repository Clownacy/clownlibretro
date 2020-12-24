#pragma once

#include <stdbool.h>

#include "libretro.h"

typedef struct Variable
{
	struct retro_core_option_definition definition;
	size_t total_values;
	size_t selected_value;
} Variable;

bool CoreRunner_Init(const char *_core_path, const char *_game_path, double *_frames_per_second);
void CoreRunner_Deinit(void);
bool CoreRunner_Update(void);
void CoreRunner_Draw(void);
void CoreRunner_GetVariables(Variable **variables_pointer, size_t *total_variables_pointer);
