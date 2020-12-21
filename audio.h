#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool Audio_Init(unsigned long _sample_rate);
void Audio_Deinit(void);
size_t Audio_PushFrames(const int16_t *data, size_t frames);
