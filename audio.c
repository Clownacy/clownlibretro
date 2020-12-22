#include "audio.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SDL.h"

static SDL_AudioDeviceID audio_device;
static unsigned long sample_rate;
static bool was_init;

bool Audio_Init(unsigned long _sample_rate)
{
	was_init = SDL_WasInit(SDL_INIT_AUDIO);

	if (was_init || SDL_InitSubSystem(SDL_INIT_AUDIO) == 0)
	{
		SDL_AudioSpec spec;
		spec.freq = sample_rate = _sample_rate;
		spec.format = AUDIO_S16SYS;
		spec.channels = 2;
		spec.samples = 1024;
		spec.callback = NULL;
		spec.userdata = NULL;

		audio_device = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);

		if (audio_device > 0)
		{
			SDL_PauseAudioDevice(audio_device, 0);

			return true;
		}

		if (!was_init)
			SDL_QuitSubSystem(SDL_INIT_AUDIO);
	}

	return false;
}

void Audio_Deinit(void)
{
	SDL_CloseAudioDevice(audio_device);

	if (!was_init)
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

size_t Audio_PushFrames(const int16_t *data, size_t frames)
{
	const size_t size_of_frame = 2 * sizeof(int16_t);

	// Don't allow the queued audio to exceed a tenth of a second.
	// This prevents any timing issues from creating an ever-increasing audio delay.
	if (SDL_GetQueuedAudioSize(audio_device) < sample_rate / 10 * size_of_frame)
		SDL_QueueAudio(audio_device, data, frames * size_of_frame);

	return frames;
}