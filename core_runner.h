#pragma once

#include <stdbool.h>

bool CoreRunner_Init(const char *_core_path, const char *_game_path, double *_frames_per_second);
void CoreRunner_Deinit(void);
bool CoreRunner_Update(void);
void CoreRunner_Draw(void);
