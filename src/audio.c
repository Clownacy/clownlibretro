#include "audio.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SDL.h"

struct Audio_Stream
{
	SDL_AudioDeviceID audio_device;
	unsigned long sample_rate;
};

static bool sdl_already_initialised;
static bool initialised;

  ////////////////
 // Main stuff //
////////////////

bool Audio_Init(void)
{
	sdl_already_initialised = SDL_WasInit(SDL_INIT_AUDIO);

	if (sdl_already_initialised || SDL_InitSubSystem(SDL_INIT_AUDIO) == 0)
		initialised = true;
	else
		initialised = false;

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

Audio_Stream* Audio_StreamCreate(unsigned long sample_rate)
{
	if (initialised)
	{
		Audio_Stream *stream = malloc(sizeof(Audio_Stream));

		if (stream != NULL)
		{
			SDL_AudioSpec spec;
			spec.freq = stream->sample_rate = sample_rate;
			spec.format = AUDIO_S16SYS;
			spec.channels = 2;
			spec.samples = 1024;
			spec.callback = NULL;
			spec.userdata = NULL;

			stream->audio_device = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);

			if (stream->audio_device > 0)
			{
				SDL_PauseAudioDevice(stream->audio_device, 0);

				return stream;
			}

			free(stream);
		}
	}

	return NULL;
}

void Audio_StreamDestroy(Audio_Stream *stream)
{
	SDL_CloseAudioDevice(stream->audio_device);
	free(stream);
}

size_t Audio_StreamPushFrames(Audio_Stream *stream, const int16_t *data, size_t frames)
{
	const size_t size_of_frame = 2 * sizeof(int16_t);

	// Don't allow the queued audio to exceed a tenth of a second.
	// This prevents any timing issues from creating an ever-increasing audio delay.
	if (SDL_GetQueuedAudioSize(stream->audio_device) < stream->sample_rate / 10 * size_of_frame)
		SDL_QueueAudio(stream->audio_device, data, frames * size_of_frame);

	return frames;
}
