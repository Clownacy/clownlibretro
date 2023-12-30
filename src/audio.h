#pragma once

#include <stddef.h>
#include <stdint.h>

#include "SDL.h"

#include "clowncommon/clowncommon.h"

#define CLOWNRESAMPLER_STATIC
#define CLOWNRESAMPLER_NO_HIGH_LEVEL_RESAMPLE_END
#include "clownresampler/clownresampler.h"

typedef struct Audio_Stream
{
	SDL_AudioDeviceID audio_device;
	cc_u32f input_sample_rate, output_sample_rate, total_buffer_frames;
	ClownResampler_HighLevel_State resampler;
} Audio_Stream; 

cc_bool Audio_Init(void);
void Audio_Deinit(void);

cc_bool Audio_StreamCreate(Audio_Stream *stream, unsigned long sample_rate);
void Audio_StreamDestroy(Audio_Stream *stream);
size_t Audio_StreamPushFrames(Audio_Stream *stream, const int16_t *data, size_t frames);
