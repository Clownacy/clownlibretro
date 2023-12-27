#include "audio.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CLOWNRESAMPLER_IMPLEMENTATION
#include "clownresampler/clownresampler.h"

#define TOTAL_CHANNELS 2
#define SIZE_OF_FRAME (TOTAL_CHANNELS * sizeof(int16_t))

static bool sdl_already_initialised;
static bool initialised;
static ClownResampler_Precomputed resampler_precomputed;

static cc_u32f GetTargetFrames(const Audio_Stream* const stream)
{
	return CLOWNRESAMPLER_MAX(stream->total_buffer_frames * 2, stream->output_sample_rate / 20); // 50ms
}

static cc_u32f GetTotalQueuedFrames(const Audio_Stream* const stream)
{
	return SDL_GetQueuedAudioSize(stream->audio_device) / SIZE_OF_FRAME;
}

  ////////////////
 // Main stuff //
////////////////

bool Audio_Init(void)
{
	sdl_already_initialised = SDL_WasInit(SDL_INIT_AUDIO);

	initialised = sdl_already_initialised || SDL_InitSubSystem(SDL_INIT_AUDIO) == 0;

	if (initialised)
		ClownResampler_Precompute(&resampler_precomputed);

	return initialised;
}

void Audio_Deinit(void)
{
	if (!sdl_already_initialised)
		SDL_QuitSubSystem(SDL_INIT_AUDIO);

	initialised = false;
}

  //////////////////
 // Stream stuff //
//////////////////

bool Audio_StreamCreate(Audio_Stream *stream, unsigned long sample_rate)
{
	if (initialised)
	{
		SDL_AudioSpec want, have;
		SDL_zero(want);
		want.freq = sample_rate;
		want.format = AUDIO_S16SYS;
		want.channels = TOTAL_CHANNELS;
		// We want a 10ms buffer (this value must be a power of two).
		want.samples = 1;
		while (want.samples < want.freq / (1000 / 10))
			want.samples <<= 1;

		stream->audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);

		if (stream->audio_device > 0)
		{
			stream->input_sample_rate = sample_rate;
			stream->output_sample_rate = have.freq;
			stream->total_buffer_frames = have.samples;

			SDL_PauseAudioDevice(stream->audio_device, 0);

			// Specify the greatest possible downsample.
			ClownResampler_HighLevel_Init(&stream->resampler, TOTAL_CHANNELS, stream->input_sample_rate * 2, stream->output_sample_rate, stream->output_sample_rate);

			return true;
		}
	}

	return false;
}

void Audio_StreamDestroy(Audio_Stream *stream)
{
	SDL_CloseAudioDevice(stream->audio_device);
}

typedef struct CallbackUserData
{
	const int16_t *data;
	size_t frames;
	SDL_AudioDeviceID audio_device;
} CallbackUserData;

static size_t InputCallback(void* const user_data, cc_s16l* const buffer, const size_t total_frames)
{
	CallbackUserData* const data = (CallbackUserData*)user_data;
	const size_t frames_to_do = CLOWNRESAMPLER_MIN(total_frames, data->frames);

	SDL_memcpy(buffer, data->data, frames_to_do * SIZE_OF_FRAME);

	data->data += frames_to_do * TOTAL_CHANNELS;
	data->frames -= frames_to_do;

	return frames_to_do;
}

static cc_bool OutputCallback(void* const user_data, const cc_s32f* const frame, const cc_u8f total_samples)
{
	CallbackUserData* const data = (CallbackUserData*)user_data;
	const Sint16 s16_frame[TOTAL_CHANNELS] = {frame[0], frame[1]};

	(void)total_samples;

	SDL_QueueAudio(data->audio_device, s16_frame, sizeof(s16_frame));

	return cc_true;
}

size_t Audio_StreamPushFrames(Audio_Stream *stream, const int16_t *data, size_t frames)
{
	const cc_u32f target_frames = GetTargetFrames(stream);
	const cc_u32f queued_frames = GetTotalQueuedFrames(stream);

	// If there is too much audio, just drop it because the dynamic rate control will be unable to handle it.
	if (queued_frames < target_frames * 2)
	{
		// Hans-Kristian Arntzen's Dynamic Rate Control formula.
		// https://github.com/libretro/docs/blob/master/archive/ratecontrol.pdf
		const cc_u32f denominator = target_frames * 0x100; // The number here is the inverse of the formula's 'd' value.
		const cc_u32f numerator = queued_frames - target_frames + denominator;

		const CallbackUserData callback_user_data = {data, frames, stream->audio_device};
		const cc_u32f adjusted_input_sample_rate = (unsigned long long)stream->input_sample_rate * numerator / denominator;

		ClownResampler_HighLevel_Adjust(&stream->resampler, CLOWNRESAMPLER_MIN(adjusted_input_sample_rate, stream->input_sample_rate * 2), stream->output_sample_rate, stream->output_sample_rate);
		ClownResampler_HighLevel_Resample(&stream->resampler, &resampler_precomputed, InputCallback, OutputCallback, &callback_user_data);
		(void)ClownResampler_HighLevel_ResampleEnd; // Stupid 'unused function' warning...
	}

	return frames;
}
