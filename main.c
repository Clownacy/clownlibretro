#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dlfcn.h>

#include "libretro.h"
#include "SDL.h"

static SDL_Window *window;
static SDL_Surface *surface;
static SDL_AudioDeviceID audio_device;
static unsigned long sample_rate;
static unsigned int frames_per_second;

static enum retro_pixel_format pixel_format;

static bool quit;

  ////////////////
 // Core stuff //
////////////////

typedef struct Core
{ 
	void *handle;

	void (*retro_set_environment)(retro_environment_t);
	void (*retro_set_video_refresh)(retro_video_refresh_t);
	void (*retro_set_audio_sample)(retro_audio_sample_t);
	void (*retro_set_audio_sample_batch)(retro_audio_sample_batch_t);
	void (*retro_set_input_poll)(retro_input_poll_t);
	void (*retro_set_input_state)(retro_input_state_t);
	void (*retro_init)(void);
	void (*retro_deinit)(void);
	unsigned int (*retro_api_version)(void);
	void (*retro_get_system_info)(struct retro_system_info *info);
	void (*retro_get_system_av_info)(struct retro_system_av_info *info);
	void (*retro_set_controller_port_device)(unsigned int port, unsigned int device);
	void (*retro_reset)(void);
	void (*retro_run)(void);
	size_t (*retro_serialize_size)(void);
	bool (*retro_serialize)(void *data, size_t size);
	bool (*retro_unserialize)(const void *data, size_t size);
	void (*retro_cheat_reset)(void);
	void (*retro_cheat_set)(unsigned int index, bool enabled, const char *code);
	bool (*retro_load_game)(const struct retro_game_info *game);
	bool (*retro_load_game_special)(unsigned int game_type, const struct retro_game_info *info, size_t num_info);
	void (*retro_unload_game)(void);
	unsigned int (*retro_get_region)(void);
	void* (*retro_get_memory_data)(unsigned int id);
	size_t (*retro_get_memory_size)(unsigned int id);
} Core;

static bool LoadCore(Core *core, const char *filename)
{
	bool success = true;

	core->handle = dlopen(filename, RTLD_LAZY);

	if (core->handle != NULL)
	{
		const struct
		{
			void **variable;
			const char *import_name;
		} imports[] = {
			{(void**)&core->retro_set_environment,            "retro_set_environment"           },
			{(void**)&core->retro_set_video_refresh,          "retro_set_video_refresh"         },
			{(void**)&core->retro_set_audio_sample,           "retro_set_audio_sample"          },
			{(void**)&core->retro_set_audio_sample_batch,     "retro_set_audio_sample_batch"    },
			{(void**)&core->retro_set_input_poll,             "retro_set_input_poll"            },
			{(void**)&core->retro_set_input_state,            "retro_set_input_state"           },
			{(void**)&core->retro_init,                       "retro_init"                      },
			{(void**)&core->retro_deinit,                     "retro_deinit"                    },
			{(void**)&core->retro_api_version,                "retro_api_version"               },
			{(void**)&core->retro_get_system_info,            "retro_get_system_info"           },
			{(void**)&core->retro_get_system_av_info,         "retro_get_system_av_info"        },
			{(void**)&core->retro_set_controller_port_device, "retro_set_controller_port_device"},
			{(void**)&core->retro_reset,                      "retro_reset"                     },
			{(void**)&core->retro_run,                        "retro_run"                       },
			{(void**)&core->retro_serialize_size,             "retro_serialize_size"            },
			{(void**)&core->retro_serialize,                  "retro_serialize"                 },
			{(void**)&core->retro_unserialize,                "retro_unserialize"               },
			{(void**)&core->retro_cheat_reset,                "retro_cheat_reset"               },
			{(void**)&core->retro_cheat_set,                  "retro_cheat_set"                 },
			{(void**)&core->retro_load_game,                  "retro_load_game"                 },
			{(void**)&core->retro_load_game_special,          "retro_load_game_special"         },
			{(void**)&core->retro_unload_game,                "retro_unload_game"               },
			{(void**)&core->retro_get_region,                 "retro_get_region"                },
			{(void**)&core->retro_get_memory_data,            "retro_get_memory_data"           },
			{(void**)&core->retro_get_memory_size,            "retro_get_memory_size"           }
		};

		for (size_t i = 0; i < sizeof(imports) / sizeof(imports[0]); ++i)
		{
			*imports[i].variable = dlsym(core->handle, imports[i].import_name);

			if (*imports[i].variable == NULL)
			{
				success = false;
				break;
			}
		}
	}
	else
	{
		success = false;
	}

	return success;
}

static void UnloadCore(Core *core)
{
	dlclose(core->handle);
}

  ///////////////
 // SDL stuff //
///////////////

static bool InitVideo(const struct retro_game_geometry *geometry)
{
	window = SDL_CreateWindow("Dummy title name", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, geometry->max_width, geometry->max_height, 0);

	if (window != NULL)
	{
		if (pixel_format == RETRO_PIXEL_FORMAT_0RGB1555 || pixel_format == RETRO_PIXEL_FORMAT_XRGB8888 || pixel_format == RETRO_PIXEL_FORMAT_RGB565)
		{
			SDL_PixelFormatEnum surface_format;

			switch (pixel_format)
			{
				default:
				case RETRO_PIXEL_FORMAT_0RGB1555:
					surface_format = SDL_PIXELFORMAT_ARGB1555;
					break;

				case RETRO_PIXEL_FORMAT_XRGB8888:
					surface_format = SDL_PIXELFORMAT_ARGB32;
					break;

				case RETRO_PIXEL_FORMAT_RGB565:
					surface_format = SDL_PIXELFORMAT_RGB565;
					break;
			}

			surface = SDL_CreateRGBSurfaceWithFormat(0, geometry->max_width, geometry->max_height, 0, surface_format);

			if (surface != NULL)
			{
				SDL_GetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);

				return true;
			}
			else
			{
				fprintf(stderr, "SDL_CreateRGBSurfaceWithFormat failed - Error: '%s'\n", SDL_GetError());
			}
		}
		else
		{
			fputs("Core requested an invalid surface pixel format\n", stderr);
		}

		SDL_DestroyWindow(window);
	}
	else
	{
		fprintf(stderr, "SDL_CreateWindow failed - Error: '%s'\n", SDL_GetError());
	}

	return false;
}

static void DeinitVideo(void)
{
	SDL_FreeSurface(surface);
	SDL_DestroyWindow(window);
}

static bool InitAudio(unsigned long _sample_rate)
{
	SDL_AudioSpec spec;
	spec.freq = sample_rate = _sample_rate;
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
	else
	{
		fprintf(stderr, "SDL_PauseAudioDevice failed - Error: '%s'\n", SDL_GetError());

		return false;
	}
}

static void DeinitAudio(void)
{
	SDL_CloseAudioDevice(audio_device);
}

  ///////////////
 // Callbacks //
///////////////

static bool Callback_Environment(unsigned int cmd, void *data)
{
	switch (cmd)
	{
		case RETRO_ENVIRONMENT_GET_CAN_DUPE:
			*(bool*)data = true;
			break;

		case RETRO_ENVIRONMENT_SHUTDOWN:
			quit = true;
			break;

		case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
			switch (*(enum retro_pixel_format*)data)
			{
				case RETRO_PIXEL_FORMAT_0RGB1555:
				case RETRO_PIXEL_FORMAT_XRGB8888:
				case RETRO_PIXEL_FORMAT_RGB565:
					pixel_format = *(enum retro_pixel_format*)data;
					break;

				default:
					return false;
			}

			break;

		case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO:;
			const struct retro_system_av_info *system_av_info = data;

			frames_per_second = system_av_info->timing.fps;

			DeinitVideo();
			InitVideo(&system_av_info->geometry);

			DeinitAudio();
			InitAudio(system_av_info->timing.sample_rate);

			break;

		case RETRO_ENVIRONMENT_SET_GEOMETRY:
			// Nothing to do here yet
			break;

		default:
			return false;
	}

	return true;
}

static void Callback_VideoRefresh(const void *data, unsigned int width, unsigned int height, size_t pitch)
{
	if (data != NULL)
	{
		const unsigned char *source_pixels = data;
		unsigned char *destination_pixels = surface->pixels;

		if (SDL_LockSurface(surface) == 0)
		{
			for (unsigned int y = 0; y < height; ++y)
				memcpy(&destination_pixels[surface->pitch * y], &source_pixels[pitch * y], width * (SDL_BITSPERPIXEL(surface->format->format) / 8));

			SDL_UnlockSurface(surface);

			SDL_BlitSurface(surface, NULL, SDL_GetWindowSurface(window), NULL);

			SDL_UpdateWindowSurface(window);
		}
	}
}

static size_t Callback_AudioSampleBatch(const int16_t *data, size_t frames)
{
	const size_t size_of_frame = 2 * sizeof(int16_t);

	if (SDL_GetQueuedAudioSize(audio_device) < sample_rate / 10 * size_of_frame)
		SDL_QueueAudio(audio_device, data, frames * size_of_frame);

	return frames;
}

static void Callback_AudioSample(int16_t left, int16_t right)
{
	const int16_t buffer[2] = {left, right};

	Callback_AudioSampleBatch(buffer, 1);
}

static void Callback_InputPoll(void)
{
	
}

static int16_t Callback_InputState(unsigned int port, unsigned int device, unsigned int index, unsigned int id)
{
	(void)port;
	(void)device;
	(void)index;
	(void)id;

	return 0;
}

  //////////
 // Main //
//////////

int main(int argc, char **argv)
{
	int main_return = EXIT_FAILURE;

	const char *core_path = argv[1];
	const char *game_path = argv[2];

	if (argc > 1)
	{
		Core core;
		if (LoadCore(&core, core_path))
		{
			if (core.retro_api_version() == 1)
			{
				pixel_format = RETRO_PIXEL_FORMAT_0RGB1555;

				core.retro_set_environment(Callback_Environment);
				core.retro_set_video_refresh(Callback_VideoRefresh);
				core.retro_set_audio_sample(Callback_AudioSample);
				core.retro_set_audio_sample_batch(Callback_AudioSampleBatch);
				core.retro_set_input_poll(Callback_InputPoll);
				core.retro_set_input_state(Callback_InputState);

				core.retro_init();

				struct retro_system_info system_info;
				core.retro_get_system_info(&system_info);

				struct retro_game_info game_info;
				game_info.path = NULL;
				game_info.data = NULL;
				game_info.size = 0;
				game_info.meta = NULL;

				if (system_info.need_fullpath)
				{
					game_info.path = game_path;
				}
				else
				{
					FILE *file = fopen(game_path, "rb");
					fseek(file, 0, SEEK_END);
					game_info.size = ftell(file);
					game_info.data = malloc(game_info.size);
					rewind(file);
					fread((void*)game_info.data, 1, game_info.size, file);
					fclose(file);
				}

				if (core.retro_load_game(&game_info))
				{
					struct retro_system_av_info system_av_info;
					core.retro_get_system_av_info(&system_av_info);

					frames_per_second = system_av_info.timing.fps;

					if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_VIDEO) == 0)
					{
						if (InitVideo(&system_av_info.geometry))
						{
							if (InitAudio(system_av_info.timing.sample_rate))
							{
								main_return = EXIT_SUCCESS;

								quit = false;

								while (!quit)
								{
									SDL_Event event;
									while (SDL_PollEvent(&event))
									{
										switch (event.type)
										{
											case SDL_QUIT:
												quit = true;
												break;
										}
									}

									core.retro_run();

									static Uint32 ticks_next;
									const Uint32 ticks_now = SDL_GetTicks();

									if (!SDL_TICKS_PASSED(ticks_now, ticks_next))
										SDL_Delay(ticks_next - ticks_now);

									ticks_next += 1000 / frames_per_second;
								}

								DeinitAudio();
							}
							else
							{
								fputs("InitAudio failed\n", stderr);
							}

							DeinitVideo();
						}
						else
						{
							fputs("InitVideo failed\n", stderr);
						}

						SDL_Quit();
					}
					else
					{
						fprintf(stderr, "SDL_Init failed - Error: '%s'\n", SDL_GetError());
					}

					core.retro_unload_game();
				}
				else
				{
					fputs("retro_load_game failed\n", stderr);
				}

				free((void*)game_info.data);

				core.retro_deinit();
			}
			else
			{
				fputs("Core targets an incompatible API\n", stderr);
			}

			UnloadCore(&core);
		}
		else
		{
			fputs("Could not load core\n", stderr);
		}
	}
	else
	{
		fputs("Core path not provided\n", stderr);
	}

	return main_return;
}
