#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dlfcn.h>
#include <libgen.h>

#include "libretro.h"
#include "SDL.h"

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static size_t size_of_texture_pixel;
static SDL_AudioDeviceID audio_device;
static unsigned long sample_rate;
static double frames_per_second;

static char libretro_path[PATH_MAX];
static char *pref_path;

static enum retro_pixel_format pixel_format;

static bool quit;

static struct
{
	bool up;
	bool down;
	bool left;
	bool right;
	bool a;
	bool b;
	bool x;
	bool y;
	bool start;
	bool select;
	bool l;
	bool l2;
	bool l3;
	bool r;
	bool r2;
	bool r3;
} retropad;

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
	if (pixel_format == RETRO_PIXEL_FORMAT_0RGB1555 || pixel_format == RETRO_PIXEL_FORMAT_XRGB8888 || pixel_format == RETRO_PIXEL_FORMAT_RGB565)
	{
		SDL_PixelFormatEnum surface_format;

		switch (pixel_format)
		{
			default:
			case RETRO_PIXEL_FORMAT_0RGB1555:
				surface_format = SDL_PIXELFORMAT_ARGB1555;
				size_of_texture_pixel = 2;
				break;

			case RETRO_PIXEL_FORMAT_XRGB8888:
				surface_format = SDL_PIXELFORMAT_ARGB8888;
				size_of_texture_pixel = 4;
				break;

			case RETRO_PIXEL_FORMAT_RGB565:
				surface_format = SDL_PIXELFORMAT_RGB565;
				size_of_texture_pixel = 2;
				break;
		}

		window = SDL_CreateWindow("clownlibretro", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, geometry->max_width, geometry->max_height, SDL_WINDOW_RESIZABLE);

		if (window != NULL)
		{
			renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

			if (renderer != NULL)
			{
				texture = SDL_CreateTexture(renderer, surface_format, SDL_TEXTUREACCESS_STREAMING, geometry->max_width, geometry->max_height);

				if (texture != NULL)
				{
					SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);

					return true;
				}
				else
				{
					fprintf(stderr, "SDL_CreateTexture failed - Error: '%s'\n", SDL_GetError());
				}

				SDL_DestroyRenderer(renderer);
			}
			else
			{
				fprintf(stderr, "SDL_CreateRenderer failed - Error: '%s'\n", SDL_GetError());
			}

			SDL_DestroyWindow(window);
		}
		else
		{
			fprintf(stderr, "SDL_CreateWindow failed - Error: '%s'\n", SDL_GetError());
		}
	}

	return false;
}

static void DeinitVideo(void)
{
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}

static bool InitAudio(unsigned long _sample_rate)
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
	else
	{
		fprintf(stderr, "SDL_OpenAudioDevice failed - Error: '%s'\n", SDL_GetError());

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

static void Callback_GetCanDupe(bool *can_dupe)
{
	*can_dupe = true;
}

static void Callback_Shutdown(void)
{
	quit = true;
}

static bool Callback_SetPixelFormat(const enum retro_pixel_format *_pixel_format)
{
	switch (*_pixel_format)
	{
		case RETRO_PIXEL_FORMAT_0RGB1555:
		case RETRO_PIXEL_FORMAT_XRGB8888:
		case RETRO_PIXEL_FORMAT_RGB565:
			pixel_format = *_pixel_format;
			return true;

		default:
			return false;
	}
}

static void Callback_GetLibretroPath(const char **path)
{
	*path = libretro_path;
}

static void Callback_GetInputDeviceCapabilities(uint64_t *capabilities)
{
	*capabilities = (1 << RETRO_DEVICE_NONE) | (1 << RETRO_DEVICE_JOYPAD);
}

static void Callback_Log(enum retro_log_level level, const char *fmt, ...)
{
	switch (level)
	{
		case RETRO_LOG_DEBUG:
			fprintf(stderr, "Debug: ");
			break;

		case RETRO_LOG_INFO:
			fprintf(stderr, "Info: ");
			break;

		case RETRO_LOG_WARN:
			fprintf(stderr, "Warning: ");
			break;

		case RETRO_LOG_ERROR:
			fprintf(stderr, "Error: ");
			break;

		default:
			break;
	}

	va_list args;
	va_start(args, fmt);

	vfprintf(stderr, fmt, args);

	va_end(args);
}

static void Callback_GetLogInterface(struct retro_log_callback *log_callback)
{
	log_callback->log = Callback_Log;
}

static void Callback_GetCoreAssetsDirectory(const char **directory)
{
	*directory = pref_path;
}

static void Callback_GetSaveDirectory(const char **directory)
{
	*directory = pref_path;
}

static void Callback_SetSystemAVInfo(const struct retro_system_av_info *system_av_info)
{
	frames_per_second = system_av_info->timing.fps;

	DeinitVideo();
	InitVideo(&system_av_info->geometry);

	DeinitAudio();
	InitAudio(system_av_info->timing.sample_rate);
}

static void Callback_SetGeometry(const struct retro_game_geometry *geometry)
{
	(void)geometry;

	// Nothing to do here yet
}

static void Callback_GetInputMaxUsers(unsigned int *max_users)
{
	*max_users = 1; // Hardcoded for now
}

static bool Callback_Environment(unsigned int cmd, void *data)
{
	switch (cmd)
	{
		case RETRO_ENVIRONMENT_GET_CAN_DUPE:
			Callback_GetCanDupe(data);
			break;

		case RETRO_ENVIRONMENT_SHUTDOWN:
			Callback_Shutdown();
			break;

		case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
			if (!Callback_SetPixelFormat(data))
				return false;

			break;

		case RETRO_ENVIRONMENT_GET_LIBRETRO_PATH:
			Callback_GetLibretroPath(data);
			break;

		case RETRO_ENVIRONMENT_GET_INPUT_DEVICE_CAPABILITIES:
			Callback_GetInputDeviceCapabilities(data);
			break;

		case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
			Callback_GetLogInterface(data);
			break;

		case RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY:
			Callback_GetCoreAssetsDirectory(data);
			break;

		case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
			Callback_GetSaveDirectory(data);
			break;

		case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO:
			Callback_SetSystemAVInfo(data);
			break;

		case RETRO_ENVIRONMENT_SET_GEOMETRY:
			Callback_SetGeometry(data);
			break;

		case RETRO_ENVIRONMENT_GET_INPUT_MAX_USERS:
			Callback_GetInputMaxUsers(data);
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
		unsigned char *destination_pixels;

		SDL_Rect rect = {0, 0, width, height};
		int destination_pitch;

		if (SDL_LockTexture(texture, &rect, (void**)&destination_pixels, &destination_pitch) == 0)
		{
			for (unsigned int y = 0; y < height; ++y)
				memcpy(&destination_pixels[destination_pitch * y], &source_pixels[pitch * y], width * size_of_texture_pixel);

			SDL_UnlockTexture(texture);

			SDL_RenderSetLogicalSize(renderer, width, height);

			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, texture, &rect, NULL);
			SDL_RenderPresent(renderer);
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
	// This function doesn't really suit SDL2
}

static int16_t Callback_InputState(unsigned int port, unsigned int device, unsigned int index, unsigned int id)
{
	if (port == 0 && device == RETRO_DEVICE_JOYPAD && index == 0)
	{
		switch (id)
		{
			case RETRO_DEVICE_ID_JOYPAD_B:
				return retropad.b;

			case RETRO_DEVICE_ID_JOYPAD_Y:
				return retropad.y;

			case RETRO_DEVICE_ID_JOYPAD_SELECT:
				return retropad.select;

			case RETRO_DEVICE_ID_JOYPAD_START:
				return retropad.start;

			case RETRO_DEVICE_ID_JOYPAD_UP:
				return retropad.up;

			case RETRO_DEVICE_ID_JOYPAD_DOWN:
				return retropad.down;

			case RETRO_DEVICE_ID_JOYPAD_LEFT:
				return retropad.left;

			case RETRO_DEVICE_ID_JOYPAD_RIGHT:
				return retropad.right;

			case RETRO_DEVICE_ID_JOYPAD_A:
				return retropad.a;

			case RETRO_DEVICE_ID_JOYPAD_X:
				return retropad.x;

			case RETRO_DEVICE_ID_JOYPAD_L:
				return retropad.l;

			case RETRO_DEVICE_ID_JOYPAD_R:
				return retropad.r;

			case RETRO_DEVICE_ID_JOYPAD_L2:
				return retropad.l2;

			case RETRO_DEVICE_ID_JOYPAD_R2:
				return retropad.r2;

			case RETRO_DEVICE_ID_JOYPAD_L3:
				return retropad.l3;

			case RETRO_DEVICE_ID_JOYPAD_R3:
				return retropad.r3;
		}
	}

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

	if (realpath(game_path, libretro_path) == NULL)
		fputs("realpath failed\n", stderr);

	pref_path = SDL_GetPrefPath("clownacy", "clownlibretro");

	if (argc > 2)
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

					if (file != NULL)
					{
						fseek(file, 0, SEEK_END);
						game_info.size = ftell(file);
						game_info.data = malloc(game_info.size);
						rewind(file);
						fread((void*)game_info.data, 1, game_info.size, file);
						fclose(file);
					}
					else
					{
						fprintf(stderr, "Could not open file '%s'\n", game_path);
					}
				}

				if (core.retro_load_game(&game_info))
				{
					struct retro_system_av_info system_av_info;
					core.retro_get_system_av_info(&system_av_info);

					frames_per_second = system_av_info.timing.fps;

					if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_VIDEO) == 0)
					{
						SDL_ShowCursor(SDL_DISABLE);

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
										static bool alt_held;

										if (event.key.keysym.sym == SDLK_LALT)
											alt_held = event.key.state == SDL_PRESSED;

										switch (event.type)
										{
											case SDL_QUIT:
												quit = true;
												break;

											case SDL_KEYDOWN:
											case SDL_KEYUP:
												switch (event.key.keysym.scancode)
												{
													case SDL_SCANCODE_W:
														retropad.up = event.key.state == SDL_PRESSED;
														break;

													case SDL_SCANCODE_A:
														retropad.left = event.key.state == SDL_PRESSED;
														break;

													case SDL_SCANCODE_S:
														retropad.down = event.key.state == SDL_PRESSED;
														break;

													case SDL_SCANCODE_D:
														retropad.right = event.key.state == SDL_PRESSED;
														break;

													case SDL_SCANCODE_P:
														retropad.a = event.key.state == SDL_PRESSED;
														break;

													case SDL_SCANCODE_O:
														retropad.b = event.key.state == SDL_PRESSED;
														break;

													case SDL_SCANCODE_0:
														retropad.x = event.key.state == SDL_PRESSED;
														break;

													case SDL_SCANCODE_9:
														retropad.y = event.key.state == SDL_PRESSED;
														break;

													case SDL_SCANCODE_8:
														retropad.l = event.key.state == SDL_PRESSED;
														break;

													case SDL_SCANCODE_7:
														retropad.l2 = event.key.state == SDL_PRESSED;
														break;

													case SDL_SCANCODE_MINUS:
														retropad.r = event.key.state == SDL_PRESSED;
														break;

													case SDL_SCANCODE_EQUALS:
														retropad.r2 = event.key.state == SDL_PRESSED;
														break;

													case SDL_SCANCODE_RETURN:;
														static bool fullscreen = false;

														if (event.key.state == SDL_PRESSED && alt_held)
														{
															SDL_SetWindowFullscreen(window, fullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
															fullscreen = !fullscreen;
														}
														else
														{
															retropad.start = event.key.state == SDL_PRESSED;
														}

														break;

													case SDL_SCANCODE_BACKSPACE:
														retropad.select = event.key.state == SDL_PRESSED;
														break;

													default:
														break;
												}

												break;
										}
									}

									core.retro_run();

									static double ticks_next;
									const Uint32 ticks_now = SDL_GetTicks();

									if (ticks_now < ticks_next)
										SDL_Delay(ticks_next - ticks_now);

									ticks_next += 1000.0 / frames_per_second;
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
		fputs("Core/game path not provided\n", stderr);
	}

	SDL_free(pref_path);

	return main_return;
}
