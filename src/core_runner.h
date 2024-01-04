#pragma once

#include <stddef.h>

#include "clowncommon/clowncommon.h"

typedef struct Variable
{
	char *key;
	char *desc;
	char *info;
	struct
	{
		char *value;
		char *label;
	} values[128]; /* See `RETRO_NUM_CORE_OPTION_VALUES_MAX` */
	size_t total_values;
	size_t selected_value;
} Variable;

typedef enum CoreRunnerScreenType
{
	CORE_RUNNER_SCREEN_TYPE_FIT,
	CORE_RUNNER_SCREEN_TYPE_PIXEL_PERFECT,
	CORE_RUNNER_SCREEN_TYPE_PIXEL_PERFECT_WITH_SCANLINES
} CoreRunnerScreenType;

cc_bool CoreRunner_Init(
#ifdef DYNAMIC_CORE
	const char *_core_path,
#endif
	const char *_game_path, double *_frames_per_second);
void CoreRunner_Deinit(void);
cc_bool CoreRunner_Update(void);
void CoreRunner_Draw(void);
void CoreRunner_GetVariables(Variable **variables_pointer, size_t *total_variables_pointer);
void CoreRunner_VariablesModified(void);
void CoreRunner_SetAlternateButtonLayout(cc_bool enable);
void CoreRunner_SetScreenType(CoreRunnerScreenType _screen_type);
