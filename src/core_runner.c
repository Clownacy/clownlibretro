#include "core_runner.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifdef ENABLE_LIBZIP
#include <zip.h>
#endif

#include "SDL.h"

#include "audio.h"
#include "file.h"
#include "input.h"
#include "libretro.h"
#include "video.h"

#define MIN(a, b) SDL_min(a, b)
#define MAX(a, b) SDL_max(a, b)

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

static bool quit;

static bool alternate_layout;
static CoreRunnerScreenType screen_type;

static double *frames_per_second;

static char *core_path;
static char *game_path;
//static char libretro_path[PATH_MAX];
static char *pref_path;
static char *save_file_path;

static Core core;
static Variable *variables;
static size_t total_variables;
static bool variables_modified;

static unsigned char *game_buffer;

static Video_Texture *core_framebuffer;
static size_t core_framebuffer_display_width;
static size_t core_framebuffer_display_height;
static size_t core_framebuffer_max_width;
static size_t core_framebuffer_max_height;
static float core_framebuffer_display_aspect_ratio;
static Video_Format core_framebuffer_format;
static size_t size_of_framebuffer_pixel;

static bool audio_stream_created;
static Audio_Stream audio_stream;
static unsigned long audio_stream_sample_rate;

  ////////////////
 // Core stuff //
////////////////

static bool LoadCore(Core *core, const char *filename)
{
	bool success = true;

	core->handle = SDL_LoadObject(filename);

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

		for (size_t i = 0; i < SDL_arraysize(imports); ++i)
		{
			*imports[i].variable = SDL_LoadFunction(core->handle, imports[i].import_name);

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
	SDL_UnloadObject(core->handle);
}

  ///////////////////
 // Options stuff //
///////////////////

static void LoadOptions(const struct retro_core_option_definition *options)
{
	total_variables = 0;
	for (const struct retro_core_option_definition *option = options; option->key != NULL; ++option)
		++total_variables;

	if (variables == NULL)
		variables = (Variable*)SDL_malloc(sizeof(Variable) * total_variables);

	if (variables != NULL)
	{
		for (size_t i = 0; i < total_variables; ++i)
		{
			variables[i].total_values = 0;
			variables[i].selected_value = 0;

			for (const struct retro_core_option_value *value = options[i].values; value->value != NULL; ++value)
			{
				if (options[i].default_value != NULL && !SDL_strcmp(value->value, options[i].default_value))
					variables[i].selected_value = variables[i].total_values;

				++variables[i].total_values;
			}

			variables[i].key = SDL_strdup(options[i].key);
			variables[i].desc = SDL_strdup(options[i].desc);
			variables[i].info = options[i].info == NULL ? NULL : SDL_strdup(options[i].info);

			for (size_t j = 0; j < variables[i].total_values; ++j)
			{
				variables[i].values[j].value = SDL_strdup(options[i].values[j].value);
				variables[i].values[j].label = options[i].values[j].label == NULL ? NULL : SDL_strdup(options[i].values[j].label);
			}
		}
	}
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

static void Callback_GetSystemDirectory(const char **path)
{
	*path = pref_path;
}

static bool SetPixelFormat(const enum retro_pixel_format pixel_format)
{
	switch (pixel_format)
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

static bool Callback_SetPixelFormat(const enum retro_pixel_format *pixel_format)
{
	return SetPixelFormat(*pixel_format);
}

static void Callback_GetVariable(struct retro_variable *variable)
{
	for (size_t i = 0; i < total_variables; ++i)
		if (!SDL_strcmp(variables[i].key, variable->key))
			variable->value = variables[i].values[variables[i].selected_value].value;
}

static void Callback_SetVariables(const struct retro_variable *variables)
{
	// Convert the `retro_variable` array to a `retro_core_option_definition` array
	size_t total_options = 0;

	for (const struct retro_variable *variable = variables; variable->key != NULL; ++variable)
		++total_options;

	struct retro_core_option_definition *options = (struct retro_core_option_definition*)SDL_malloc(sizeof(struct retro_core_option_definition) * (total_options + 1));

	if (options != NULL)
	{
		for (size_t i = 0; i < total_options; ++i)
		{
			char *value_string_pointer = SDL_strdup(variables[i].value);

			options[i].key = variables[i].key;
			options[i].desc = value_string_pointer;
			options[i].info = NULL;
			options[i].default_value = NULL;

			size_t total_values = 0;

			value_string_pointer = SDL_strchr(value_string_pointer, ';');

			*value_string_pointer++ = '\0';

			for (;;)
			{
				options[i].values[total_values].value = ++value_string_pointer;
				options[i].values[total_values].label = NULL;

				++total_values;

				value_string_pointer = SDL_strchr(value_string_pointer, '|');

				if (value_string_pointer == NULL)
					break;

				*value_string_pointer = '\0';
			}

			options[i].values[total_values].value = NULL;
			options[i].values[total_values].label = NULL;
		}

		options[total_options].key = NULL;
		options[total_options].desc = NULL;
		options[total_options].info = NULL;
		options[total_options].values[0].value = NULL;
		options[total_options].values[0].label = NULL;
		options[total_options].default_value = NULL;

		// Process the `retro_core_option_definition` array
		LoadOptions(options);

		// Now get rid of it
		for (size_t i = 0; i < total_options; ++i)
			SDL_free((char*)options[i].desc);

		SDL_free(options);
	}
}

static void Callback_SetVariableUpdate(bool *update)
{
	*update = variables_modified;
	variables_modified = false;
}

static void Callback_GetLibretroPath(const char **path)
{
	*path = core_path;
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

static void Callback_SetGeometry(const struct retro_game_geometry *geometry)
{
	core_framebuffer_display_width = geometry->base_width;
	core_framebuffer_display_height = geometry->base_height;
	core_framebuffer_display_aspect_ratio = geometry->aspect_ratio <= 0.0f ? (float)geometry->base_width / (float)geometry->base_height : geometry->aspect_ratio;
}

static bool SetSystemAVInfo(const struct retro_system_av_info *system_av_info)
{
	*frames_per_second = system_av_info->timing.fps;

	Callback_SetGeometry(&system_av_info->geometry);

	if (core_framebuffer_max_width != system_av_info->geometry.max_width || core_framebuffer_max_height != system_av_info->geometry.max_height)
	{
		if (core_framebuffer != NULL)
			Video_TextureDestroy(core_framebuffer);

		core_framebuffer = Video_TextureCreate(system_av_info->geometry.max_width, system_av_info->geometry.max_height, core_framebuffer_format, true);

		core_framebuffer_max_width = system_av_info->geometry.max_width;
		core_framebuffer_max_height = system_av_info->geometry.max_height;
	}

	if (audio_stream_sample_rate != system_av_info->timing.sample_rate)
	{
		if (audio_stream_created)
			Audio_StreamDestroy(&audio_stream);

		audio_stream_created = Audio_StreamCreate(&audio_stream, system_av_info->timing.sample_rate);

		audio_stream_sample_rate = system_av_info->timing.sample_rate;
	}

	return core_framebuffer != NULL;
}

static void Callback_SetSystemAVInfo(const struct retro_system_av_info *system_av_info)
{
	SetSystemAVInfo(system_av_info);
}

static void Callback_GetCoreOptionsVersion(unsigned int *version)
{
	*version = 1; // TODO try this with 0
}

static void Callback_SetCoreOptions(const struct retro_core_option_definition *options)
{
	LoadOptions(options);
}

static void Callback_SetCoreOptionsIntl(const struct retro_core_options_intl *options)
{
	LoadOptions(options->us);
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
			Callback_GetCanDupe((bool*)data);
			break;

		case RETRO_ENVIRONMENT_SHUTDOWN:
			Callback_Shutdown();
			break;

		case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
			Callback_GetSystemDirectory((const char**)data);
			break;

		case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
			if (!Callback_SetPixelFormat((const enum retro_pixel_format*)data))
				return false;

			break;

		case RETRO_ENVIRONMENT_GET_VARIABLE:
			Callback_GetVariable((struct retro_variable*)data);
			break;

		case RETRO_ENVIRONMENT_SET_VARIABLES:
			Callback_SetVariables((const struct retro_variable*)data);
			break;

		case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
			Callback_SetVariableUpdate((bool*)data);
			break;

		case RETRO_ENVIRONMENT_GET_LIBRETRO_PATH:
			Callback_GetLibretroPath((const char**)data);
			break;

		case RETRO_ENVIRONMENT_GET_INPUT_DEVICE_CAPABILITIES:
			Callback_GetInputDeviceCapabilities((uint64_t*)data);
			break;

		case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
			Callback_GetLogInterface((struct retro_log_callback*)data);
			break;

		case RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY:
			Callback_GetCoreAssetsDirectory((const char**)data);
			break;

		case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
			Callback_GetSaveDirectory((const char**)data);
			break;

		case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO:
			Callback_SetSystemAVInfo((const struct retro_system_av_info *)data);
			break;

		case RETRO_ENVIRONMENT_SET_GEOMETRY:
			Callback_SetGeometry((const struct retro_game_geometry*)data);
			break;

		case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
			Callback_GetCoreOptionsVersion((unsigned int*)data);
			break;

		case RETRO_ENVIRONMENT_SET_CORE_OPTIONS:
			Callback_SetCoreOptions((const struct retro_core_option_definition*)data);
			break;

		case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL:
			Callback_SetCoreOptionsIntl((const struct retro_core_options_intl*)data);
			break;

		case RETRO_ENVIRONMENT_GET_INPUT_MAX_USERS:
			Callback_GetInputMaxUsers((unsigned int*)data);
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
		const unsigned char *source_pixels = (const unsigned char*)data;
		unsigned char *destination_pixels;

		Video_Rect rect = {0, 0, width, height};
		size_t destination_pitch;

		core_framebuffer_display_width = width;
		core_framebuffer_display_height = height;

		SDL_assert(width <= core_framebuffer_max_width);
		SDL_assert(height <= core_framebuffer_max_height);

		if (Video_TextureLock(core_framebuffer, &rect, &destination_pixels, &destination_pitch))
		{
			for (unsigned int y = 0; y < height; ++y)
				SDL_memcpy(&destination_pixels[destination_pitch * y], &source_pixels[pitch * y], width * size_of_framebuffer_pixel);

			Video_TextureUnlock(core_framebuffer);
		}
	}
}

static size_t Callback_AudioSampleBatch(const int16_t *data, size_t frames)
{
	if (audio_stream_created)
		return Audio_StreamPushFrames(&audio_stream, data, frames);
	else
		return frames;
}

static void Callback_AudioSample(int16_t left, int16_t right)
{
	if (audio_stream_created)
	{
		const int16_t buffer[2] = {left, right};

		Audio_StreamPushFrames(&audio_stream, buffer, 1);
	}
}

static void Callback_InputPoll(void)
{
	// This function doesn't really suit SDL2
}

static int16_t Callback_InputState(unsigned int port, unsigned int device, unsigned int index, unsigned int id)
{
	(void)index;

	if (alternate_layout)
	{
		switch (id)
		{
			case RETRO_DEVICE_ID_JOYPAD_A:
				id = RETRO_DEVICE_ID_JOYPAD_B;
				break;

			case RETRO_DEVICE_ID_JOYPAD_B:
				id = RETRO_DEVICE_ID_JOYPAD_Y;
				break;

			case RETRO_DEVICE_ID_JOYPAD_Y:
				id = RETRO_DEVICE_ID_JOYPAD_X;
				break;

			case RETRO_DEVICE_ID_JOYPAD_X:
				id = RETRO_DEVICE_ID_JOYPAD_A;
				break;
		}
	}

	if (port == 0 && device == RETRO_DEVICE_JOYPAD)
		return retropad.buttons[id].held;

	return 0;
}

  //////////
 // Main //
//////////

bool CoreRunner_Init(const char *_core_path, const char *_game_path, double *_frames_per_second)
{
	frames_per_second = _frames_per_second;

	// Calculate some paths which will be needed later
	core_path = SDL_strdup(_core_path);
	game_path = SDL_strdup(_game_path);

	// TODO: For now, we're just assuming that the user passed an absolute path.
//	if (realpath(core_path, libretro_path) == NULL)
//		fputs("realpath failed\n", stderr);

	pref_path = SDL_GetPrefPath("clownacy", "clownlibretro");

	// If we cannot get the pref path, just use the working directory.
	if (pref_path == NULL)
		pref_path = SDL_strdup("./");

	const char* const forward_slash = SDL_strrchr(game_path, '/');
#ifdef _WIN32
	const char* const backward_slash = SDL_strrchr(game_path, '\\');
#endif
	const char* const game_filename =
#ifdef _WIN32
		forward_slash != NULL && backward_slash != NULL ? SDL_max(forward_slash, backward_slash) + 1 : backward_slash != NULL ? backward_slash + 1 :
#endif
		forward_slash != NULL ? forward_slash + 1 : game_path;

	SDL_asprintf(&save_file_path, "%s/%s.sav", pref_path, game_filename);

	// Load the core, set some callbacks, and initialise it
	if (!LoadCore(&core, core_path))
	{
		fputs("Could not load core\n", stderr);
	}
	else
	{
		if (core.retro_api_version() != RETRO_API_VERSION)
		{
			fputs("Core targets an incompatible API\n", stderr);
		}
		else
		{
			SetPixelFormat(RETRO_PIXEL_FORMAT_0RGB1555);

			core.retro_set_environment(Callback_Environment);

			// Mesen requires that this be called before retro_set_video_refresh.
			// TODO: Tell Meson's devs to fix their core or tell libretro's devs to fix their API.
			core.retro_init();

			core.retro_set_video_refresh(Callback_VideoRefresh);
			core.retro_set_audio_sample(Callback_AudioSample);
			core.retro_set_audio_sample_batch(Callback_AudioSampleBatch);
			core.retro_set_input_poll(Callback_InputPoll);
			core.retro_set_input_state(Callback_InputState);

			// Grab some info for later
			struct retro_system_info system_info;
			core.retro_get_system_info(&system_info);

			// Load the game (TODO: handle cores that don't need supplied game data)
			// TODO: Move this to its own function or something
			struct retro_game_info game_info;
			game_info.path = game_path;
			game_info.data = NULL;
			game_info.size = 0;
			game_info.meta = NULL;

			bool game_loaded = false;

#ifdef ENABLE_LIBZIP
			// If the file is a zip archive, then try extracting a useable file.
			// If it isn't, just assume it's a plain ROM and load it directly.
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
							game_buffer = (unsigned char*)SDL_malloc(stat.size);

							if (game_buffer != NULL)
							{
								game_info.data = game_buffer;
								game_info.size = stat.size;

								zip_fread(zip_file, game_buffer, game_info.size);

								if (system_info.need_fullpath)
								{
									// Mesen is weird and demands a file path even for zipped files,
									// so extract the ROM to a proper file and give Meson the path to it.
									char *temporary_filename;

									SDL_asprintf(&temporary_filename, "%stemp", pref_path);

									if (temporary_filename == NULL)
									{
										fputs("Could not obtain temporary filename\n", stderr);
									}
									else
									{
										SDL_RWops* const file = SDL_RWFromFile(temporary_filename, "wb");

										if (file == NULL)
										{
											fprintf(stderr, "Could not open temporary file '%s' for writing\n", temporary_filename);
										}
										else
										{
											SDL_RWwrite(file, game_buffer, 1, game_info.size);
											SDL_RWclose(file);

											// We no longer need the buffer, so free it.
											SDL_free(game_buffer);
											game_buffer = NULL;

											// Finally, load the extracted ROM file.
											game_info.path = temporary_filename;
											game_loaded = core.retro_load_game(&game_info);
										}

										SDL_free(temporary_filename);
									}
								}
								else
								{
									// The libretro core is sane, so we can just give it the memory buffer.
									game_loaded = core.retro_load_game(&game_info);
								}

								if (game_loaded)
								{
									zip_fclose(zip_file);
									break;
								}
							}

							zip_fclose(zip_file);
						}
					}
				}

				zip_close(zip);
			}
			else
#endif
			{
				if (system_info.need_fullpath)
				{
					game_loaded = core.retro_load_game(&game_info);
				}
				else
				{
					if (ReadFileToAllocatedBuffer(game_path, &game_buffer, &game_info.size))
					{
						game_info.data = game_buffer;
						game_loaded = core.retro_load_game(&game_info);
					}
					else
					{
						fprintf(stderr, "Could not open file '%s'\n", game_path);
					}
				}
			}

			if (!game_loaded)
			{
				fputs("retro_load_game failed\n", stderr);
			}
			else
			{
				// Grab more info that will come in handy later
				struct retro_system_av_info system_av_info;
				core.retro_get_system_av_info(&system_av_info);

				if (!SetSystemAVInfo(&system_av_info))
				{
					fputs("Failed to create core framebuffer texture\n", stderr);
				}
				else
				{
					// Read save data from file
					void* const save_ram = core.retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
					const size_t save_ram_size = core.retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);

					if (save_ram != NULL && save_ram_size != 0)
					{
						if (ReadFileToBuffer(save_file_path, save_ram, save_ram_size))
							fputs("Save file read\n", stderr);
						else
							fputs("Save file could not be read\n", stderr);
					}

					return true;
				}

				core.retro_unload_game();
			}

			SDL_free(game_buffer);

			core.retro_deinit();
		}

		UnloadCore(&core);
	}

	SDL_free(core_path);
	SDL_free(game_path);
	SDL_free(pref_path);
	SDL_free(save_file_path);

	return false;
}

void CoreRunner_Deinit(void)
{
	void* const save_ram = core.retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
	const size_t save_ram_size = core.retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);

	if (save_ram != NULL && save_ram_size != 0)
	{
		// Write save data to file
		if (WriteBufferToFile(save_file_path, save_ram, save_ram_size))
			fputs("Save file written\n", stderr);
		else
			fputs("Save file could not be written\n", stderr);
	}

	if (audio_stream_created)
		Audio_StreamDestroy(&audio_stream);

	Video_TextureDestroy(core_framebuffer);

	core.retro_unload_game();

	SDL_free(game_buffer);

	core.retro_deinit();

	UnloadCore(&core);

	SDL_free(core_path);
	SDL_free(game_path);
	SDL_free(pref_path);
	SDL_free(save_file_path);

	for (size_t i = 0; i < total_variables; ++i)
	{
		SDL_free(variables[i].key);
		SDL_free(variables[i].desc);
		SDL_free(variables[i].info);

		for (size_t j = 0; j < variables[i].total_values; ++j)
		{
			SDL_free(variables[i].values[j].value);
			SDL_free(variables[i].values[j].label);
		}
	}

	SDL_free(variables);
}

bool CoreRunner_Update(void)
{
	// Update the core
	core.retro_run();

	return !quit;
}

void CoreRunner_Draw(void)
{
	const size_t upscale_factor = MAX(1, MIN(window_width / core_framebuffer_display_width, window_height / core_framebuffer_display_height));

	size_t dst_width;
	size_t dst_height;

	if (screen_type == CORE_RUNNER_SCREEN_TYPE_PIXEL_PERFECT || screen_type == CORE_RUNNER_SCREEN_TYPE_PIXEL_PERFECT_WITH_SCANLINES)
	{
		dst_width = core_framebuffer_display_width * upscale_factor;
		dst_height = core_framebuffer_display_height * upscale_factor;
	}
	else
	{
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
	}

	const Video_Rect src_rect = {0, 0, core_framebuffer_display_width, core_framebuffer_display_height};
	const Video_Rect dst_rect = {(window_width - dst_width) / 2, (window_height - dst_height) / 2, dst_width, dst_height};

	Video_TextureDraw(core_framebuffer, &dst_rect, &src_rect, (Video_Colour){0xFF, 0xFF, 0xFF});

	if (screen_type == CORE_RUNNER_SCREEN_TYPE_PIXEL_PERFECT_WITH_SCANLINES)
		for (size_t i = 0; i < core_framebuffer_display_height; ++i)
			Video_DrawLine(dst_rect.x, dst_rect.y + i * upscale_factor, dst_rect.x + dst_rect.width, dst_rect.y + i * upscale_factor);
}

void CoreRunner_GetVariables(Variable **variables_pointer, size_t *total_variables_pointer)
{
	*variables_pointer = variables;
	*total_variables_pointer = total_variables;
}

void CoreRunner_VariablesModified(void)
{
	variables_modified = true;
}

void CoreRunner_SetAlternateButtonLayout(bool enable)
{
	alternate_layout = enable;
}

void CoreRunner_SetScreenType(CoreRunnerScreenType _screen_type)
{
	screen_type = _screen_type;
}
