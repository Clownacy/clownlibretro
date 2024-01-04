#include "core_loader.h"

#include "SDL.h"

static void *handle;

void (*proxy_retro_set_environment)(retro_environment_t);
void (*proxy_retro_set_video_refresh)(retro_video_refresh_t);
void (*proxy_retro_set_audio_sample)(retro_audio_sample_t);
void (*proxy_retro_set_audio_sample_batch)(retro_audio_sample_batch_t);
void (*proxy_retro_set_input_poll)(retro_input_poll_t);
void (*proxy_retro_set_input_state)(retro_input_state_t);
void (*proxy_retro_init)(void);
void (*proxy_retro_deinit)(void);
unsigned int (*proxy_retro_api_version)(void);
void (*proxy_retro_get_system_info)(struct retro_system_info *info);
void (*proxy_retro_get_system_av_info)(struct retro_system_av_info *info);
void (*proxy_retro_set_controller_port_device)(unsigned int port, unsigned int device);
void (*proxy_retro_reset)(void);
void (*proxy_retro_run)(void);
size_t (*proxy_retro_serialize_size)(void);
bool (*proxy_retro_serialize)(void *data, size_t size);
bool (*proxy_retro_unserialize)(const void *data, size_t size);
void (*proxy_retro_cheat_reset)(void);
void (*proxy_retro_cheat_set)(unsigned int index, bool enabled, const char *code);
bool (*proxy_retro_load_game)(const struct retro_game_info *game);
bool (*proxy_retro_load_game_special)(unsigned int game_type, const struct retro_game_info *info, size_t num_info);
void (*proxy_retro_unload_game)(void);
unsigned int (*proxy_retro_get_region)(void);
void* (*proxy_retro_get_memory_data)(unsigned int id);
size_t (*proxy_retro_get_memory_size)(unsigned int id);

cc_bool LoadCore(const char *filename)
{
	handle = SDL_LoadObject(filename);

	if (handle == NULL)
		return cc_false;

#define DO_FUNCTION(FUNCTION_NAME)\
	*(void**)&proxy_##FUNCTION_NAME = SDL_LoadFunction(handle, #FUNCTION_NAME);\
\
	if (proxy_##FUNCTION_NAME == NULL)\
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

	return cc_true;
}

void UnloadCore(void)
{
	SDL_UnloadObject(handle);
}
