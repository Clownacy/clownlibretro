#ifndef CORE_LOADER_H
#define CORE_LOADER_H

#include "libretro.h"

#ifdef DYNAMIC_CORE
	#include "clowncommon/clowncommon.h"

	cc_bool LoadCore(const char *filename);
	void UnloadCore(void);

	extern void (*proxy_retro_set_environment)(retro_environment_t);
	extern void (*proxy_retro_set_video_refresh)(retro_video_refresh_t);
	extern void (*proxy_retro_set_audio_sample)(retro_audio_sample_t);
	extern void (*proxy_retro_set_audio_sample_batch)(retro_audio_sample_batch_t);
	extern void (*proxy_retro_set_input_poll)(retro_input_poll_t);
	extern void (*proxy_retro_set_input_state)(retro_input_state_t);
	extern void (*proxy_retro_init)(void);
	extern void (*proxy_retro_deinit)(void);
	extern unsigned int (*proxy_retro_api_version)(void);
	extern void (*proxy_retro_get_system_info)(struct retro_system_info *info);
	extern void (*proxy_retro_get_system_av_info)(struct retro_system_av_info *info);
	extern void (*proxy_retro_set_controller_port_device)(unsigned int port, unsigned int device);
	extern void (*proxy_retro_reset)(void);
	extern void (*proxy_retro_run)(void);
	extern size_t (*proxy_retro_serialize_size)(void);
	extern bool (*proxy_retro_serialize)(void *data, size_t size);
	extern bool (*proxy_retro_unserialize)(const void *data, size_t size);
	extern void (*proxy_retro_cheat_reset)(void);
	extern void (*proxy_retro_cheat_set)(unsigned int index, bool enabled, const char *code);
	extern bool (*proxy_retro_load_game)(const struct retro_game_info *game);
	extern bool (*proxy_retro_load_game_special)(unsigned int game_type, const struct retro_game_info *info, size_t num_info);
	extern void (*proxy_retro_unload_game)(void);
	extern unsigned int (*proxy_retro_get_region)(void);
	extern void* (*proxy_retro_get_memory_data)(unsigned int id);
	extern size_t (*proxy_retro_get_memory_size)(unsigned int id);

	#define retro_set_environment proxy_retro_set_environment
	#define retro_set_video_refresh proxy_retro_set_video_refresh
	#define retro_set_audio_sample proxy_retro_set_audio_sample
	#define retro_set_audio_sample_batch proxy_retro_set_audio_sample_batch
	#define retro_set_input_poll proxy_retro_set_input_poll
	#define retro_set_input_state proxy_retro_set_input_state
	#define retro_init proxy_retro_init
	#define retro_deinit proxy_retro_deinit
	#define retro_api_version proxy_retro_api_version
	#define retro_get_system_info proxy_retro_get_system_info
	#define retro_get_system_av_info proxy_retro_get_system_av_info
	#define retro_set_controller_port_device proxy_retro_set_controller_port_device
	#define retro_reset proxy_retro_reset
	#define retro_run proxy_retro_run
	#define retro_serialize_size proxy_retro_serialize_size
	#define retro_serialize proxy_retro_serialize
	#define retro_unserialize proxy_retro_unserialize
	#define retro_cheat_reset proxy_retro_cheat_reset
	#define retro_cheat_set proxy_retro_cheat_set
	#define retro_load_game proxy_retro_load_game
	#define retro_load_game_special proxy_retro_load_game_special
	#define retro_unload_game proxy_retro_unload_game
	#define retro_get_region proxy_retro_get_region
	#define retro_get_memory_data proxy_retro_get_memory_data
	#define retro_get_memory_size proxy_retro_get_memory_size
#endif

#endif /* CORE_LOADER_H */
