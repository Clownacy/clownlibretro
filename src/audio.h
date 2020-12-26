#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct Audio_Stream Audio_Stream; 

bool Audio_Init(void);
void Audio_Deinit(void);

Audio_Stream* Audio_StreamCreate(unsigned long sample_rate);
void Audio_StreamDestroy(Audio_Stream *stream);
size_t Audio_StreamPushFrames(Audio_Stream *stream, const int16_t *data, size_t frames);
