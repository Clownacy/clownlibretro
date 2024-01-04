#include "core_runner.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#ifdef ENABLE_LIBZIP
#include <zip.h>
#endif

#include "SDL.h"

#include "audio.h"
#include "error.h"
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

	retro_hw_context_reset_t context_reset;
	retro_hw_context_reset_t context_destroy;

	cc_bool hardware_render;
} Core;

static cc_bool quit;

static cc_bool alternate_layout;
static CoreRunnerScreenType screen_type;

static double *frames_per_second;

static char *core_path;
static char *game_path, *game_path_override;
/*static char libretro_path[PATH_MAX];*/
static char *pref_path;
static char *save_file_path;

static Core core;
static Variable *variables;
static size_t total_variables;
static cc_bool variables_modified = cc_true;

static unsigned char *game_buffer;
static size_t game_buffer_size;

static cc_bool core_framebuffer_created;
static Video_Framebuffer core_framebuffer;
static size_t core_framebuffer_display_width;
static size_t core_framebuffer_display_height;
static size_t core_framebuffer_max_width;
static size_t core_framebuffer_max_height;
static float core_framebuffer_display_aspect_ratio;
static Video_Format core_framebuffer_format;
static size_t size_of_framebuffer_pixel;
static cc_bool core_framebuffer_depth;
static cc_bool core_framebuffer_stencil;
static cc_bool core_framebuffer_bottom_left_origin;

static cc_bool audio_stream_created;
static Audio_Stream audio_stream;
static unsigned long audio_stream_sample_rate;

/***************
* Game loading *
***************/

static cc_bool BootGame(void)
{
	struct retro_game_info game_info;

	game_info.path = game_path_override != NULL ? game_path_override : game_path;
	game_info.data = game_buffer;
	game_info.size = game_buffer_size;
	game_info.meta = NULL;

	return core.retro_load_game(&game_info) ? cc_true : cc_false;
}

static cc_bool LoadGame(const char* const _game_path)
{
	/* TODO: handle cores that don't need supplied game data */
	struct retro_system_info system_info;
	cc_bool game_loaded;
#ifdef ENABLE_LIBZIP
	zip_t *zip;
#endif

	/* Grab some info for later */
	core.retro_get_system_info(&system_info);

	game_path = SDL_strdup(_game_path);
	game_path_override = NULL;
	game_buffer = NULL;
	game_buffer_size = 0;

	game_loaded = false;

#ifdef ENABLE_LIBZIP
	/* If the file is a zip archive, then try extracting a useable file.
		If it isn't, just assume it's a plain ROM and load it directly. */
	zip = zip_open(game_path, ZIP_RDONLY, NULL);

	if (zip != NULL)
	{
		zip_int64_t i;

		const zip_int64_t total_files = zip_get_num_entries(zip, 0);

		for (i = 0; i < total_files; ++i)
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
						game_buffer_size = stat.size;

						zip_fread(zip_file, game_buffer, game_buffer_size);

						if (system_info.need_fullpath)
						{
							/* Mesen is weird and demands a file path even for zipped files,
								so extract the ROM to a proper file and give Meson the path to it. */
							SDL_asprintf(&game_path_override, "%stemp", pref_path);

							if (game_path_override == NULL)
							{
								PrintError("Could not obtain temporary filename");
							}
							else
							{
								SDL_RWops* const file = SDL_RWFromFile(game_path_override, "wb");

								if (file == NULL)
								{
									PrintError("Could not open temporary file '%s' for writing", game_path_override);
								}
								else
								{
									SDL_RWwrite(file, game_buffer, 1, game_buffer_size);
									SDL_RWclose(file);

									/* We no longer need the buffer, so free it. */
									SDL_free(game_buffer);
									game_buffer = NULL;

									/* Finally, load the extracted ROM file. */
									game_loaded = BootGame();
								}
							}
						}
						else
						{
							/* The libretro core is sane, so we can just give it the memory buffer. */
							game_loaded = BootGame();
						}

						if (game_loaded)
						{
							zip_fclose(zip_file);
							break;
						}

						SDL_free(game_buffer);
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
		if (system_info.need_fullpath || ReadFileToAllocatedBuffer(game_path, &game_buffer, &game_buffer_size))
			game_loaded = BootGame();
		else
			PrintError("Could not open file '%s'", game_path);
	}

	return game_loaded;
}

void UnloadGame(void)
{
	core.retro_unload_game();

	SDL_free(game_path);
	SDL_free(game_path_override);
	SDL_free(game_buffer);
}

/*************
* Core stuff *
*************/

static cc_bool LoadCore(Core *core, const char *filename)
{
	core->handle = SDL_LoadObject(filename);

	if (core->handle == NULL)
		return cc_false;

#define DO_FUNCTION(FUNCTION_NAME)\
	*(void**)&core->FUNCTION_NAME = SDL_LoadFunction(core->handle, #FUNCTION_NAME);\
\
	if (core->FUNCTION_NAME == NULL)\
		return cc_false

	DO_FUNCTION(retro_set_environment);
	DO_FUNCTION(retro_set_video_refresh);
	DO_FUNCTION(retro_set_audio_sample);
	DO_FUNCTION(retro_set_audio_sample_batch);
	DO_FUNCTION(retro_set_input_poll);
	DO_FUNCTION(retro_set_input_state);
	DO_FUNCTION(retro_init);
	DO_FUNCTION(retro_deinit);
	DO_FUNCTION(retro_api_version);
	DO_FUNCTION(retro_get_system_info);
	DO_FUNCTION(retro_get_system_av_info);
	DO_FUNCTION(retro_set_controller_port_device);
	DO_FUNCTION(retro_reset);
	DO_FUNCTION(retro_run);
	DO_FUNCTION(retro_serialize_size);
	DO_FUNCTION(retro_serialize);
	DO_FUNCTION(retro_unserialize);
	DO_FUNCTION(retro_cheat_reset);
	DO_FUNCTION(retro_cheat_set);
	DO_FUNCTION(retro_load_game);
	DO_FUNCTION(retro_load_game_special);
	DO_FUNCTION(retro_unload_game);
	DO_FUNCTION(retro_get_region);
	DO_FUNCTION(retro_get_memory_data);
	DO_FUNCTION(retro_get_memory_size);
#undef DO_FUNCTION

	core->context_reset = NULL;
	core->context_destroy = NULL;
	core->hardware_render = cc_false;

	return cc_true;
}

static void UnloadCore(Core *core)
{
	SDL_UnloadObject(core->handle);
}

/****************
* Options stuff *
****************/

static void LoadOptions(const struct retro_core_option_definition *options)
{
	const struct retro_core_option_definition *option;

	total_variables = 0;
	for (option = options; option->key != NULL; ++option)
		++total_variables;

	if (variables == NULL)
		variables = (Variable*)SDL_malloc(sizeof(Variable) * total_variables);

	if (variables != NULL)
	{
		size_t i;

		for (i = 0; i < total_variables; ++i)
		{
			const struct retro_core_option_value *value;
			size_t j;

			variables[i].total_values = 0;
			variables[i].selected_value = 0;

			for (value = options[i].values; value->value != NULL; ++value)
			{
				if (options[i].default_value != NULL && !SDL_strcmp(value->value, options[i].default_value))
					variables[i].selected_value = variables[i].total_values;

				++variables[i].total_values;
			}

			variables[i].key = SDL_strdup(options[i].key);
			variables[i].desc = SDL_strdup(options[i].desc);
			variables[i].info = options[i].info == NULL ? NULL : SDL_strdup(options[i].info);

			for (j = 0; j < variables[i].total_values; ++j)
			{
				variables[i].values[j].value = SDL_strdup(options[i].values[j].value);
				variables[i].values[j].label = options[i].values[j].label == NULL ? NULL : SDL_strdup(options[i].values[j].label);
			}
		}
	}
}

/************
* Callbacks *
************/

static void Callback_GetCanDupe(bool *can_dupe)
{
	*can_dupe = true;
}

static void Callback_Shutdown(void)
{
	quit = cc_true;
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

static uintptr_t GetCurrentFramebuffer(void)
{
	return (uintptr_t)Video_FramebufferNative(&core_framebuffer);
}

#if defined(RENDERER_OPENGL3) || defined(RENDERER_OPENGLES2)
static retro_proc_address_t GetProcAddress(const char* const name)
{
#if 0
	PrintDebug("GetProcAddress %s %p", name, SDL_GL_GetProcAddress(name));
#endif
	return SDL_GL_GetProcAddress(name);
}
#endif

static bool Callback_SetHWRender(struct retro_hw_render_callback *renderer)
{
#if defined(RENDERER_OPENGL3) || defined(RENDERER_OPENGLES2)
#ifdef RENDERER_OPENGL3
	if (renderer->context_type != RETRO_HW_CONTEXT_OPENGL_CORE)
		return false;
#endif
#ifdef RENDERER_OPENGLES2
	if (renderer->context_type != RETRO_HW_CONTEXT_OPENGLES2)
		return false;
#endif

	if (renderer->debug_context)
		return false;

	core.context_reset = renderer->context_reset;
	core.context_destroy = renderer->context_destroy;

	renderer->get_current_framebuffer = GetCurrentFramebuffer;
	renderer->get_proc_address = GetProcAddress;

	core_framebuffer_depth = renderer->depth;
	core_framebuffer_stencil = renderer->stencil;
	core_framebuffer_bottom_left_origin = renderer->bottom_left_origin;

	/* TODO: Version numbers. */

	core.hardware_render = cc_true;

	return true;
#else
	return false;
#endif
}

static void Callback_GetVariable(struct retro_variable *variable)
{
	size_t i;

	for (i = 0; i < total_variables; ++i)
		if (!SDL_strcmp(variables[i].key, variable->key))
			variable->value = variables[i].values[variables[i].selected_value].value;
}

static void Callback_SetVariables(const struct retro_variable *variables)
{
	size_t total_options;
	const struct retro_variable *variable;
	struct retro_core_option_definition *options;

	/* Convert the `retro_variable` array to a `retro_core_option_definition` array */
	total_options = 0;

	for (variable = variables; variable->key != NULL; ++variable)
		++total_options;

	options = (struct retro_core_option_definition*)SDL_malloc(sizeof(struct retro_core_option_definition) * (total_options + 1));

	if (options != NULL)
	{
		size_t i;

		for (i = 0; i < total_options; ++i)
		{
			char *value_string_pointer = SDL_strdup(variables[i].value);
			size_t total_values = 0;

			options[i].key = variables[i].key;
			options[i].desc = value_string_pointer;
			options[i].info = NULL;
			options[i].default_value = NULL;

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

		/* Process the `retro_core_option_definition` array */
		LoadOptions(options);

		/* Now get rid of it */
		for (i = 0; i < total_options; ++i)
			SDL_free((char*)options[i].desc);

		SDL_free(options);
	}
}

static void Callback_SetVariableUpdate(bool *update) /* TODO: it's GET */
{
	*update = variables_modified;
	variables_modified = cc_false;
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
	va_list args;

	va_start(args, fmt);

	switch (level)
	{
		default:
		case RETRO_LOG_DEBUG:
			PrintDebugV(fmt, args);
			break;

		case RETRO_LOG_INFO:
			PrintInfoV(fmt, args);
			break;

		case RETRO_LOG_WARN:
			PrintWarningV(fmt, args);
			break;

		case RETRO_LOG_ERROR:
			PrintErrorV(fmt, args);
			break;
	}

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
		if (core_framebuffer_created)
			Video_FramebufferDestroy(&core_framebuffer);

		if (core.hardware_render)
			core_framebuffer_created = Video_FramebufferCreateHardware(&core_framebuffer, system_av_info->geometry.max_width, system_av_info->geometry.max_height, core_framebuffer_depth, core_framebuffer_stencil);
		else
			core_framebuffer_created = Video_FramebufferCreateSoftware(&core_framebuffer, system_av_info->geometry.max_width, system_av_info->geometry.max_height, core_framebuffer_format, cc_true);

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

	return core_framebuffer_created;
}

static void Callback_SetSystemAVInfo(const struct retro_system_av_info *system_av_info)
{
	SetSystemAVInfo(system_av_info);
}

static void Callback_GetCoreOptionsVersion(unsigned int *version)
{
	*version = 1; /* TODO try this with 0 */
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
	*max_users = 1; /* Hardcoded for now */
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

		case RETRO_ENVIRONMENT_SET_HW_RENDER:
			if (!Callback_SetHWRender((struct retro_hw_render_callback*)data))
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
	if (data == NULL)
		return;

	core_framebuffer_display_width = width;
	core_framebuffer_display_height = height;

	SDL_assert(width <= core_framebuffer_max_width);
	SDL_assert(height <= core_framebuffer_max_height);

	if (data != RETRO_HW_FRAME_BUFFER_VALID)
	{
		Video_Rect rect;
		size_t destination_pitch;

		Video_Texture* const texture = Video_FramebufferTexture(&core_framebuffer);
		const unsigned char *source_pixels = (const unsigned char*)data;
		unsigned char *destination_pixels;

		rect.x = 0;
		rect.y = 0;
		rect.width = width;
		rect.height = height;

		if (Video_TextureLock(texture, &rect, &destination_pixels, &destination_pitch))
		{
			unsigned int y;

			for (y = 0; y < height; ++y)
				SDL_memcpy(&destination_pixels[destination_pitch * y], &source_pixels[pitch * y], width * size_of_framebuffer_pixel);

			Video_TextureUnlock(texture);
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
		int16_t buffer[2];

		buffer[0] = left;
		buffer[1] = right;
		Audio_StreamPushFrames(&audio_stream, buffer, 1);
	}
}

static void Callback_InputPoll(void)
{
	/* This function doesn't really suit SDL2 */
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

/*******
* Main *
*******/

cc_bool CoreRunner_Init(const char *_core_path, const char *game_path, double *_frames_per_second)
{
	const char *forward_slash;
#ifdef _WIN32
	const char *backward_slash;
#endif
	const char *game_filename;

	frames_per_second = _frames_per_second;

	/* Calculate some paths which will be needed later */
	core_path = SDL_strdup(_core_path);

	/* TODO: For now, we're just assuming that the user passed an absolute path */
/*	if (realpath(core_path, libretro_path) == NULL)
		PrintError("realpath failed");*/

	pref_path = SDL_GetPrefPath("clownacy", "clownlibretro");

	/* If we cannot get the pref path, just use the working directory */
	if (pref_path == NULL)
		pref_path = SDL_strdup("./");

	/* Extract the file name from the file path */
	forward_slash = SDL_strrchr(game_path, '/');
#ifdef _WIN32
	backward_slash = SDL_strrchr(game_path, '\\');
#endif
	game_filename =
#ifdef _WIN32
		forward_slash != NULL && backward_slash != NULL ? SDL_max(forward_slash, backward_slash) + 1 : backward_slash != NULL ? backward_slash + 1 :
#endif
		forward_slash != NULL ? forward_slash + 1 : game_path;

	SDL_asprintf(&save_file_path, "%s/%s.sav", pref_path, game_filename);

	/* Load the core, set some callbacks, and initialise it */
	if (!LoadCore(&core, core_path))
	{
		PrintError("Could not load core");
	}
	else
	{
		if (core.retro_api_version() != RETRO_API_VERSION)
		{
			PrintError("Core targets an incompatible API");
		}
		else
		{
			/* Set default pixel format */
			SetPixelFormat(RETRO_PIXEL_FORMAT_0RGB1555);

			core_framebuffer_bottom_left_origin = cc_false;

			/* Registers callbacks with the libretro core */
			core.retro_set_environment(Callback_Environment);

			/* Mesen requires that this be called before retro_set_video_refresh. */
			/* TODO: Tell Meson's devs to fix their core or tell libretro's devs to fix their API. */
			core.retro_init();

			core.retro_set_video_refresh(Callback_VideoRefresh);
			core.retro_set_audio_sample(Callback_AudioSample);
			core.retro_set_audio_sample_batch(Callback_AudioSampleBatch);
			core.retro_set_input_poll(Callback_InputPoll);
			core.retro_set_input_state(Callback_InputState);

			if (!LoadGame(game_path))
			{
				PrintError("retro_load_game failed");
			}
			else
			{
				/* Grab more info that will come in handy later */
				struct retro_system_av_info system_av_info;

				core.retro_get_system_av_info(&system_av_info);

				if (!SetSystemAVInfo(&system_av_info))
				{
					PrintError("Failed to create core framebuffer texture");
				}
				else
				{
					/* Read save data from file */
					void* const save_ram = core.retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
					const size_t save_ram_size = core.retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);

					if (save_ram != NULL && save_ram_size != 0)
					{
						if (ReadFileToBuffer(save_file_path, save_ram, save_ram_size))
							PrintError("Save file read");
						else
							PrintError("Save file could not be read");
					}

					if (core.hardware_render)
						core.context_reset();

					return cc_true;
				}

				UnloadGame();
			}

			core.retro_deinit();
		}

		UnloadCore(&core);
	}

	SDL_free(core_path);
	SDL_free(pref_path);
	SDL_free(save_file_path);

	return cc_false;
}

void CoreRunner_Deinit(void)
{
	size_t i;

	void* const save_ram = core.retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
	const size_t save_ram_size = core.retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);

	if (save_ram != NULL && save_ram_size != 0)
	{
		/* Write save data to file */
		if (WriteBufferToFile(save_file_path, save_ram, save_ram_size))
			PrintError("Save file written");
		else
			PrintError("Save file could not be written");
	}

	if (core.hardware_render)
		core.context_destroy();

	if (audio_stream_created)
		Audio_StreamDestroy(&audio_stream);

	Video_FramebufferDestroy(&core_framebuffer);

	UnloadGame();

	core.retro_deinit();

	UnloadCore(&core);

	SDL_free(core_path);
	SDL_free(pref_path);
	SDL_free(save_file_path);

	for (i = 0; i < total_variables; ++i)
	{
		size_t j;

		SDL_free(variables[i].key);
		SDL_free(variables[i].desc);
		SDL_free(variables[i].info);

		for (j = 0; j < variables[i].total_values; ++j)
		{
			SDL_free(variables[i].values[j].value);
			SDL_free(variables[i].values[j].label);
		}
	}

	SDL_free(variables);
}

cc_bool CoreRunner_Update(void)
{
	/* Update the core */
	core.retro_run();

	return !quit;
}

void CoreRunner_Draw(void)
{
	size_t dst_width;
	size_t dst_height;
	Video_Rect src_rect;
	Video_Rect dst_rect;

	const size_t upscale_factor = MAX(1, MIN(window_width / core_framebuffer_display_width, window_height / core_framebuffer_display_height));
	const Video_Colour white = {0xFF, 0xFF, 0xFF};

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

	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.width = core_framebuffer_display_width;
	src_rect.height = core_framebuffer_display_height;

	if (core_framebuffer_bottom_left_origin)
	{
		src_rect.y = src_rect.height;
		src_rect.height = -src_rect.height;
	}

	dst_rect.x = (window_width - dst_width) / 2;
	dst_rect.y = (window_height - dst_height) / 2;
	dst_rect.width = dst_width;
	dst_rect.height = dst_height;

	Video_TextureDraw(Video_FramebufferTexture(&core_framebuffer), &dst_rect, &src_rect, white);

	if (screen_type == CORE_RUNNER_SCREEN_TYPE_PIXEL_PERFECT_WITH_SCANLINES)
	{
		size_t i;

		for (i = 0; i < core_framebuffer_display_height; ++i)
			Video_DrawLine(dst_rect.x, dst_rect.y + i * upscale_factor, dst_rect.x + dst_rect.width, dst_rect.y + i * upscale_factor);
	}
}

void CoreRunner_GetVariables(Variable **variables_pointer, size_t *total_variables_pointer)
{
	*variables_pointer = variables;
	*total_variables_pointer = total_variables;
}

void CoreRunner_VariablesModified(void)
{
	variables_modified = cc_true;
}

void CoreRunner_SetAlternateButtonLayout(cc_bool enable)
{
	alternate_layout = enable;
}

void CoreRunner_SetScreenType(CoreRunnerScreenType _screen_type)
{
	screen_type = _screen_type;
}
