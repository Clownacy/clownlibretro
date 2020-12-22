#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dlfcn.h>
#include <libgen.h>
#include <zip.h>

#include "SDL.h"

#include "audio.h"
#include "event.h"
#include "file.h"
#include "font.h"
#include "input.h"
#include "libretro.h"
#include "video.h"

static double frames_per_second;
static bool audio_initialised;

static Video_Texture *core_framebuffer;
static size_t core_framebuffer_display_width;
static size_t core_framebuffer_display_height;
static float core_framebuffer_display_aspect_ratio;
static Video_Format core_framebuffer_format;
static size_t size_of_framebuffer_pixel;

static char libretro_path[PATH_MAX];
static char *pref_path;

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

static bool Callback_SetPixelFormat(const enum retro_pixel_format *pixel_format)
{
	switch (*pixel_format)
	{
		case RETRO_PIXEL_FORMAT_0RGB1555:
			core_framebuffer_format = VIDEO_FORMAT_0RGB1555;
			size_of_framebuffer_pixel = 2;
			break;

		case RETRO_PIXEL_FORMAT_XRGB8888:
			core_framebuffer_format = VIDEO_FORMAT_XRGB8888;
			size_of_framebuffer_pixel = 4;
			break;

		case RETRO_PIXEL_FORMAT_RGB565:
			core_framebuffer_format = VIDEO_FORMAT_RGB565;
			size_of_framebuffer_pixel = 2;
			break;

		default:
			return false;
	}

	return true;
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
			fputs("Debug: ", stderr);
			break;

		case RETRO_LOG_INFO:
			fputs("Info: ", stderr);
			break;

		case RETRO_LOG_WARN:
			fputs("Warning: ", stderr);
			break;

		case RETRO_LOG_ERROR:
			fputs("Error: ", stderr);
			break;

		default:
			fputs("Unknown log type: ", stderr);
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

	core_framebuffer_display_width = system_av_info->geometry.base_width;
	core_framebuffer_display_height = system_av_info->geometry.base_height;
	core_framebuffer_display_aspect_ratio = system_av_info->geometry.aspect_ratio;

	Video_TextureDestroy(core_framebuffer);
	Video_TextureCreate(system_av_info->geometry.max_width, system_av_info->geometry.max_height, core_framebuffer_format, true);

	if (audio_initialised)
	{
		Audio_Deinit();
		audio_initialised = false;
	}

	audio_initialised = Audio_Init(system_av_info->timing.sample_rate);
}

static void Callback_SetGeometry(const struct retro_game_geometry *geometry)
{
	core_framebuffer_display_width = geometry->base_width;
	core_framebuffer_display_height = geometry->base_height;
	core_framebuffer_display_aspect_ratio = geometry->aspect_ratio;
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

		Video_Rect rect = {0, 0, width, height};
		size_t destination_pitch;

		if (Video_TextureLock(core_framebuffer, &rect, &destination_pixels, &destination_pitch))
		{
			for (unsigned int y = 0; y < height; ++y)
				memcpy(&destination_pixels[destination_pitch * y], &source_pixels[pitch * y], width * size_of_framebuffer_pixel);

			Video_TextureUnlock(core_framebuffer);
		}
	}
}

static size_t Callback_AudioSampleBatch(const int16_t *data, size_t frames)
{
	if (audio_initialised)
		return Audio_PushFrames(data, frames);
	else
		return frames;
}

static void Callback_AudioSample(int16_t left, int16_t right)
{
	if (audio_initialised)
	{
		const int16_t buffer[2] = {left, right};

		Audio_PushFrames(buffer, 1);
	}
}

static void Callback_InputPoll(void)
{
	// This function doesn't really suit SDL2
}

static int16_t Callback_InputState(unsigned int port, unsigned int device, unsigned int index, unsigned int id)
{
	(void)index;

	if (port == 0 && device == RETRO_DEVICE_JOYPAD)
		return retropad.buttons[id];

	return 0;
}

  //////////
 // Main //
//////////

int main(int argc, char **argv)
{
	int main_return = EXIT_FAILURE;

	if (argc < 2)
	{
		fputs("Core path not specified\n", stderr);
	}
	else if (argc < 3)
	{
		fputs("Game path not specified\n", stderr);
	}
	else
	{
		// Calculate some paths which will be needed later
		const char *core_path = argv[1];
		const char *game_path = argv[2];

		if (realpath(game_path, libretro_path) == NULL)
			fputs("realpath failed\n", stderr);

		pref_path = SDL_GetPrefPath("clownacy", "clownlibretro");

		char *game_path_dup = strdup(game_path);
		const char *game_filename = basename(game_path_dup);

		char *save_file_path = malloc(strlen(pref_path) + strlen(game_filename) + 4 + 1);

		sprintf(save_file_path, "%s%s.sav", pref_path, game_filename);

		// Load the core, set some callbacks, and initialise it
		Core core;
		if (LoadCore(&core, core_path))
		{
			if (core.retro_api_version() == RETRO_API_VERSION)
			{
				core_framebuffer_format = VIDEO_FORMAT_0RGB1555;

				core.retro_set_environment(Callback_Environment);
				core.retro_set_video_refresh(Callback_VideoRefresh);
				core.retro_set_audio_sample(Callback_AudioSample);
				core.retro_set_audio_sample_batch(Callback_AudioSampleBatch);
				core.retro_set_input_poll(Callback_InputPoll);
				core.retro_set_input_state(Callback_InputState);

				core.retro_init();

				// Grab some info for later
				struct retro_system_info system_info;
				core.retro_get_system_info(&system_info);

				// Load the game (TODO: handle cores that don't need supplied game data)
				struct retro_game_info game_info;
				game_info.path = game_path;
				game_info.data = NULL;
				game_info.size = 0;
				game_info.meta = NULL;

				if (!system_info.need_fullpath)
				{
					// If the file is a zip archive, then try extracing a useable file.
					// If it isn't, just assume it's a plain ROM and load it to memory.
					zip_t *zip = zip_open(game_path, ZIP_RDONLY, NULL);

					if (zip != NULL)
					{
						const zip_int64_t total_files = zip_get_num_entries(zip, 0);
						for (zip_int64_t i = 0; i < total_files; ++i)
						{
							zip_stat_t stat;
							if (zip_stat_index(zip, i, 0, &stat) == 0 && stat.valid & ZIP_STAT_SIZE)
							{
								zip_file_t *zip_file = zip_fopen_index(zip, i, 0);

								if (zip_file != NULL)
								{
									game_info.data = malloc(stat.size);

									if (game_info.data != NULL)
									{
										game_info.size = stat.size;

										zip_fread(zip_file, (void*)game_info.data, game_info.size);

										zip_fclose(zip_file);

										break;
									}

									zip_fclose(zip_file);
								}
							}
						}

						zip_close(zip);
					}
					else if (!FileToMemory(game_path, (unsigned char**)&game_info.data, &game_info.size))
					{
						fprintf(stderr, "Could not open file '%s'\n", game_path);
					}
				}

				if (core.retro_load_game(&game_info))
				{
					// Grab more info that will come in handy later
					struct retro_system_av_info system_av_info;
					core.retro_get_system_av_info(&system_av_info);

					frames_per_second = system_av_info.timing.fps;

					// Initialise SDL2 video and audio
					if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO) == 0)
					{
						if (Video_Init(system_av_info.geometry.base_width, system_av_info.geometry.base_height))
						{
							window_width = system_av_info.geometry.base_width;
							window_height = system_av_info.geometry.base_height;

							core_framebuffer = Video_TextureCreate(system_av_info.geometry.max_width, system_av_info.geometry.max_height, core_framebuffer_format, true);

							if (core_framebuffer != NULL)
							{
								core_framebuffer_display_width = system_av_info.geometry.base_width;
								core_framebuffer_display_height = system_av_info.geometry.base_height;
								core_framebuffer_display_aspect_ratio = system_av_info.geometry.aspect_ratio;

								audio_initialised = Audio_Init(system_av_info.timing.sample_rate);

								Font *font = LoadFreeTypeFont("font", 10, 20, true);

								if (font != NULL)
								{
									main_return = EXIT_SUCCESS;

									// Read save data from file
									unsigned char *save_file_buffer;
									size_t save_file_size;
									if (FileToMemory(save_file_path, &save_file_buffer, &save_file_size))
									{
										memcpy(core.retro_get_memory_data(RETRO_MEMORY_SAVE_RAM), save_file_buffer, core.retro_get_memory_size(RETRO_MEMORY_SAVE_RAM) < save_file_size ? core.retro_get_memory_size(RETRO_MEMORY_SAVE_RAM) : save_file_size);

										free(save_file_buffer);

										fputs("Save file read\n", stderr);
									}
									else
									{
										fputs("Save file could not be read\n", stderr);
									}

									// Begin the mainloop
									quit = false;

									while (!quit)
									{
										if (!HandleEvents())
											quit = true;

										// Update the core
										core.retro_run();

										// Draw stuff
										Video_Clear();

										size_t dst_width;
										size_t dst_height;

										if ((float)window_width / (float)window_height < core_framebuffer_display_aspect_ratio)
										{
											dst_width = window_width;
											dst_height = window_width / core_framebuffer_display_aspect_ratio;
										}
										else
										{
											dst_width = window_height * core_framebuffer_display_aspect_ratio;
											dst_height = window_height;
										}

										Video_Rect src_rect = {0, 0, core_framebuffer_display_width, core_framebuffer_display_height};
										Video_Rect dst_rect = {(window_width - dst_width) / 2, (window_height - dst_height) / 2, dst_width, dst_height};

										Video_TextureDraw(core_framebuffer, &dst_rect, &src_rect, 0xFF, 0xFF, 0xFF);

										DrawText(font, NULL, 0, 0, 0xFFFFFFFF, "test");

										Video_Display();

										// Delay until the next frame
										static double ticks_next;
										const Uint32 ticks_now = SDL_GetTicks();

										if (ticks_now < ticks_next)
											SDL_Delay(ticks_next - ticks_now);

										ticks_next += 1000.0 / frames_per_second;
									}

									if (core.retro_get_memory_size(RETRO_MEMORY_SAVE_RAM) != 0)
									{
										// Write save data to file
										if (MemoryToFile(save_file_path, core.retro_get_memory_data(RETRO_MEMORY_SAVE_RAM), core.retro_get_memory_size(RETRO_MEMORY_SAVE_RAM)))
											fputs("Save file written\n", stderr);
										else
											fputs("Save file could not be written\n", stderr);
									}

									UnloadFont(font);
								}
								else
								{
									fputs("Failed to load font\n", stderr);
								}

								// Begin teardown
								if (audio_initialised)
								{
									Audio_Deinit();
									audio_initialised = false;
								}

								Video_TextureDestroy(core_framebuffer);
							}
							else
							{
								fputs("Failed to create core framebuffer texture\n", stderr);
							}

							Video_Deinit();
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

		free(save_file_path);
		free(game_path_dup);

		SDL_free(pref_path);
	}

	return main_return;
}
