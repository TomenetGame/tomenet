/*
 * File: snd-sdl.c
 * Purpose: SDL sound support
 *
 * Copyright (c) 2004-2007 Brendon Oliver, Andrew Sidwell.
 * A large chunk of this file was taken and modified from main-ros.
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *
 * a) the GNU General Public License as published by the Free Software
 *    Foundation, version 2, or
 *
 * b) the "Angband licence":
 *    This software may be copied and distributed for educational, research,
 *    and not for profit purposes provided that this copyright and statement
 *    are included in all such copies.  Other copyrights may also apply.
 */

/* modified for TomeNET to support our custom sound structure and also
   background music and weather - C. Blue */

#include "angband.h"


#ifdef SOUND_SDL

/* Smooth dynamic weather volume change depending on amount of particles,
   instead of just on/off depending on particle count window? */
//#define WEATHER_VOL_PARTICLES  -- not yet that cool

/* Maybe this will be cool: Dynamic weather volume calculated from distances to registered clouds */
#define WEATHER_VOL_CLOUDS

/*
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_thread.h>
*/

/* This is the way recommended by the SDL web page */
#include "SDL.h"
#include "SDL_mixer.h"
#include "SDL_thread.h"

/* completely turn off mixing of disabled audio features (music, sound, weather, all)
   as opposed to just muting their volume? */
//#define DISABLE_MUTED_AUDIO

/* load sounds on the fly if the loader thread has not loaded them yet */
/* disabled because this can cause delays up to a few seconds */
bool on_demand_loading = FALSE;

/* output various status messages about initializing audio */
//#define DEBUG_SOUND

/* Path to sound files */
#define SEGFAULT_HACK
#ifndef SEGFAULT_HACK
static const char *ANGBAND_DIR_XTRA_SOUND;
static const char *ANGBAND_DIR_XTRA_MUSIC; //<-this one in particular segfaults!
#else
char ANGBAND_DIR_XTRA_SOUND[1024];
char ANGBAND_DIR_XTRA_MUSIC[1024];
#endif

/* for threaded caching of audio files */
SDL_Thread *load_audio_thread;
SDL_mutex *load_sample_mutex_entrance, *load_song_mutex_entrance;
SDL_mutex *load_sample_mutex, *load_song_mutex;


/* declare */
static void fadein_next_music(void);
static void clear_channel(int c);
static bool my_fexists(const char *fname);

static int thread_load_audio(void *dummy);
static Mix_Chunk* load_sample(int idx, int subidx);
static Mix_Music* load_song(int idx, int subidx);


/* Arbitrary limit of mixer channels */
#define MAX_CHANNELS	32

/* Arbitary limit on number of samples per event [8] */
#define MAX_SAMPLES	32

/* Arbitary limit on number of music songs per situation [3] */
#define MAX_SONGS	32

/* Exponential volume level table
 * evlt[i] = round(0.09921 * exp(2.31 * i / 100))
 */

static s16b evlt[101] = { 12,
  13,  13,  14,  14,  14,  15,  15,  15,  16,  16,
  16,  17,  17,  18,  18,  18,  19,  19,  20,  20,
  21,  21,  22,  22,  23,  23,  24,  24,  25,  25,
  26,  27,  27,  28,  29,  29,  30,  31,  31,  32,
  33,  34,  34,  35,  36,  37,  38,  38,  39,  40,
  41,  42,  43,  44,  45,  46,  47,  48,  50,  51,
  52,  53,  54,  56,  57,  58,  60,  61,  63,  64,
  65,  67,  69,  70,  72,  73,  75,  77,  79,  81,
  82,  84,  86,  88,  90,  93,  95,  97,  99, 102,
 104, 106, 109, 111, 114, 117, 119, 122, 125, 128
};

#if 1 /* Exponential volume scale - mikaelh */
 #define CALC_MIX_VOLUME(T, V)	(cfg_audio_master * T * evlt[V] * evlt[cfg_audio_master_volume] / MIX_MAX_VOLUME)
#endif
#if 0 /* use linear volume scale -- poly+50 would be best, but already causes mixer outtages if total volume becomes too small (ie < 1).. =_= */
 #define CALC_MIX_VOLUME(T, V)  ((MIX_MAX_VOLUME * (cfg_audio_master ? ((T) ? (V) : 0) : 0) * cfg_audio_master_volume) / 10000)
#endif
#if 0 /* use polynomial volume scale -- '+50' seems best, although some low-low combos already won't produce sound at all due to low mixer resolution. maybe just use linear scale, simply? */
 #define CALC_MIX_VOLUME(T, V)	((MIX_MAX_VOLUME * (cfg_audio_master ? ((T) ? (((V) + 50) * ((V) + 50)) / 100 - 25 : 0) : 0) * (((cfg_audio_master_volume + 50) * (cfg_audio_master_volume + 50)) / 100 - 25)) / 40000)
// #define CALC_MIX_VOLUME(T, V)	((MIX_MAX_VOLUME * (cfg_audio_master ? ((T) ? (((V) + 30) * ((V) + 30)) / 100 - 9 : 0) : 0) * (((cfg_audio_master_volume + 30) * (cfg_audio_master_volume + 30)) / 100 - 9)) / 25600)
// #define CALC_MIX_VOLUME(T, V)	((MIX_MAX_VOLUME * (cfg_audio_master ? ((T) ? (((V) + 10) * ((V) + 10)) / 100 - 1 : 0) : 0) * (((cfg_audio_master_volume + 10) * (cfg_audio_master_volume + 10)) / 100 - 1)) / 14400)
#endif
#if 0 /* use exponential volume scale (whoa) - but let's use a lookup table for efficiency ^^ EDIT: too extreme */
 static int vol[11] = { 0, 1, 2, 4, 6, 10, 16, 26, 42, 68, 100 }; /* this is similar to rounded down 1.6^(0..10) */
 #define CALC_MIX_VOLUME(T, V)	((MIX_MAX_VOLUME * (cfg_audio_master ? ((T) ? vol[(V) / 10] : 0) : 0) * vol[cfg_audio_master_volume / 10]) / 10000)
#endif
#if 0 /* also too extreme, even more so */
 static int vol[11] = { 0, 1, 2, 3, 5, 8, 14, 25, 42, 68, 100 }; /* this is similar to rounded down 1.6^(0..10) */
 #define CALC_MIX_VOLUME(T, V)	((MIX_MAX_VOLUME * (cfg_audio_master ? ((T) ? vol[(V) / 10] : 0) : 0) * vol[cfg_audio_master_volume / 10]) / 10000)
#endif

/* Struct representing all data about an event sample */
typedef struct {
	int num;                        /* Number of samples for this event */
	Mix_Chunk *wavs[MAX_SAMPLES];   /* Sample array */
	const char *paths[MAX_SAMPLES]; /* Relative pathnames for samples */
	int current_channel;		/* Channel it's currently being played on, -1 if none; to avoid
					   stacking of the same sound multiple (read: too many) times - ...
					   note that with 4.4.5.4+ this is deprecated - C. Blue */
	int started_timer_tick;		/* global timer tick on which this sample was started (for efficiency) */
	bool disabled;
	bool config;
} sample_list;

/* background music */
typedef struct {
	int num;
	Mix_Music *wavs[MAX_SONGS];
	const char *paths[MAX_SONGS];
	bool disabled;
	bool config;
} song_list;

/* Just need an array of SampInfos */
static sample_list samples[SOUND_MAX_2010];

/* Array of potential channels, for managing that only one
   sound of a kind is played simultaneously, for efficiency - C. Blue */
static int channel_sample[MAX_CHANNELS];
static int channel_type[MAX_CHANNELS];
static int channel_volume[MAX_CHANNELS];
static s32b channel_player_id[MAX_CHANNELS];

/* Music Array */
static song_list songs[MUSIC_MAX];


/*
 * Shut down the sound system and free resources.
 */
static void close_audio(void) {
	size_t i;
	int j;

	/* Kill the loading thread if it's still running */
	if (load_audio_thread) SDL_KillThread(load_audio_thread);

	Mix_HaltMusic();

	/* Free all the sample data*/
	for (i = 0; i < SOUND_MAX_2010; i++) {
		sample_list *smp = &samples[i];

		/* Nuke all samples */
		for (j = 0; j < smp->num; j++) {
			Mix_FreeChunk(smp->wavs[j]);
			string_free(smp->paths[j]);
		}
	}

	/* Free all the music data*/
	for (i = 0; i < MUSIC_MAX; i++) {
		song_list *mus = &songs[i];

		/* Nuke all samples */
		for (j = 0; j < mus->num; j++) {
			Mix_FreeMusic(mus->wavs[j]);
			string_free(mus->paths[j]);
		}
	}

#ifndef SEGFAULT_HACK
	string_free(ANGBAND_DIR_XTRA_SOUND);
	string_free(ANGBAND_DIR_XTRA_MUSIC);
#endif

	/* Close the audio */
	Mix_CloseAudio();

	SDL_DestroyMutex(load_sample_mutex_entrance);
	SDL_DestroyMutex(load_song_mutex_entrance);
	SDL_DestroyMutex(load_sample_mutex);
	SDL_DestroyMutex(load_song_mutex);

	/* XXX This may conflict with the SDL port */
	SDL_Quit();
}


/*
 * Initialise SDL and open the mixer
 */
static bool open_audio(void) {
	int audio_rate;
	Uint16 audio_format;
	int audio_channels;
	
	/* Initialize variables */
	if (cfg_audio_rate < 4000) cfg_audio_rate = 4000;
	if (cfg_audio_rate > 48000) cfg_audio_rate = 48000;
	audio_rate = cfg_audio_rate;
	audio_format = AUDIO_S16SYS;
	audio_channels = 2;

	/* Initialize the SDL library */
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
//#ifdef DEBUG_SOUND
		plog_fmt("Couldn't initialize SDL: %s", SDL_GetError());
//		puts(format("Couldn't initialize SDL: %s", SDL_GetError()));
//#endif
		return FALSE;
	}

	/* Try to open the audio */
	if (cfg_audio_buffer < 128) cfg_audio_buffer = 128;
	if (cfg_audio_buffer > 8192) cfg_audio_buffer = 8192;
	if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, cfg_audio_buffer) < 0) {
//#ifdef DEBUG_SOUND
		plog_fmt("Couldn't open mixer: %s", SDL_GetError());
//		puts(format("Couldn't open mixer: %s", SDL_GetError()));
//#endif
		return FALSE;
	}

	if (cfg_max_channels > MAX_CHANNELS) cfg_max_channels = MAX_CHANNELS;
	if (cfg_max_channels < 4) cfg_max_channels = 4;
	Mix_AllocateChannels(cfg_max_channels);
	/* set hook for clearing faded-out weather and for managing sound-fx playback */
	Mix_ChannelFinished(clear_channel);
	/* set hook for fading over to next song */
	Mix_HookMusicFinished(fadein_next_music);

	/* Success */
	return TRUE;
}



/*
 * Read sound.cfg and map events to sounds; then load all the sounds into
 * memory to avoid I/O latency later.
 */
static bool sound_sdl_init(bool no_cache) {
	char path[2048];
	char buffer0[2048], *buffer = buffer0;
	FILE *fff;
	int i;
	char out_val[160];
	bool disabled;

	bool events_loaded_semaphore;

	load_sample_mutex_entrance = SDL_CreateMutex();
	load_song_mutex_entrance = SDL_CreateMutex();
	load_sample_mutex = SDL_CreateMutex();
	load_song_mutex = SDL_CreateMutex();

	/* Initialise the mixer  */
	if (!open_audio()) return FALSE;

#ifdef DEBUG_SOUND
	puts(format("sound_sdl_init() opened at %d Hz.", cfg_audio_rate));
#endif

	/* Initialize sound-fx channel management */
	for (i = 0; i < cfg_max_channels; i++) channel_sample[i] = -1;


	/* ------------------------------- Init Sounds */

	/* Build the "sound" path */
	path_build(path, sizeof(path), ANGBAND_DIR_XTRA, "sound");
#ifndef SEGFAULT_HACK
	ANGBAND_DIR_XTRA_SOUND = string_make(path);
#else
	strcpy(ANGBAND_DIR_XTRA_SOUND, path);
#endif

	/* Find and open the config file */
#if 0
#ifdef WINDOWS
	/* On Windows we must have a second config file just to store disabled-state, since we cannot write to Program Files folder after Win XP anymore..
	   So if it exists, let it override the normal config file. */
	if (!win_dontmoveuser && getenv("HOMEDRIVE") && getenv("HOMEPATH")) {
		strcpy(path, getenv("HOMEDRIVE"));
		strcat(path, getenv("HOMEPATH"));
		strcat(path, "\\TomeNET-sound.cfg");
	} else path_build(path, sizeof(path), ANGBAND_DIR_XTRA_SOUND, "TomeNET-sound.cfg"); //paranoia

	fff = my_fopen(path, "r");
	if (!fff) {
		path_build(path, sizeof(path), ANGBAND_DIR_XTRA_SOUND, "sound.cfg");
		fff = my_fopen(path, "r");
	}
#else
	path_build(path, sizeof(path), ANGBAND_DIR_XTRA_SOUND, "sound.cfg");
	fff = my_fopen(path, "r");
#endif
#else
	path_build(path, sizeof(path), ANGBAND_DIR_XTRA_SOUND, "sound.cfg");
	fff = my_fopen(path, "r");
#endif

	/* Handle errors */
	if (!fff) {
#if 0
		plog_fmt("Failed to open sound config (%s):\n    %s", path, strerror(errno));
		return FALSE;
#else /* try to use simple default file */
		FILE *fff2;
		char path2[2048];

		path_build(path, sizeof(path), ANGBAND_DIR_XTRA_SOUND, "sound.cfg.default");
		fff = my_fopen(path, "r");
		if (!fff) {
			plog_fmt("Failed to open default sound config (%s):\n    %s", path, strerror(errno));
			return FALSE;
		}

		path_build(path2, sizeof(path2), ANGBAND_DIR_XTRA_SOUND, "sound.cfg");
		fff2 = my_fopen(path2, "w");
		if (!fff2) {
			plog_fmt("Failed to write sound config (%s):\n    %s", path2, strerror(errno));
			return FALSE;
		}

		while (my_fgets(fff, buffer, sizeof(buffer)) == 0)
			fprintf(fff2, "%s\n", buffer);

		my_fclose(fff2);
		my_fclose(fff);

		/* Try again */
		path_build(path, sizeof(path), ANGBAND_DIR_XTRA_SOUND, "sound.cfg");
		fff = my_fopen(path, "r");
		if (!fff) {
			plog_fmt("Failed to open sound config (%s):\n    %s", path, strerror(errno));
			return FALSE;
		}
#endif
	}

#ifdef DEBUG_SOUND
	puts("sound_sdl_init() reading sound.cfg:");
#endif

	/* Parse the file */
	/* Lines are always of the form "name = sample [sample ...]" */
	while (my_fgets(fff, buffer0, sizeof(buffer0)) == 0) {
		char *cfg_name;
		cptr lua_name;
		char *sample_list;
		char *search;
		char *cur_token;
		char *next_token;
		int event;

		/* Lines starting on ';' count as 'provided event' but actually
		   remain silent, as a way of disabling effects/songs without
		   letting the server know. */
		buffer = buffer0;
		if (buffer0[0] == ';') {
			buffer++;
			disabled = TRUE;
		} else disabled = FALSE;

		/* Skip anything not beginning with an alphabetic character */
		if (!buffer[0] || !isalpha((unsigned char)buffer[0])) continue;

		/* Split the line into two: message name, and the rest */
		search = strchr(buffer, ' ');
		sample_list = strchr(search + 1, ' ');
		/* no event name given? */
		if (!search) continue;
		/* no audio filenames listed? */
		if (!sample_list) continue;

		/* Set the message name, and terminate at first space */
		cfg_name = buffer;
		search[0] = '\0';

		/* Make sure this is a valid event name */
		for (event = SOUND_MAX_2010 - 1; event >= 0; event--) {
			sprintf(out_val, "return get_sound_name(%d)", event);
			lua_name = string_exec_lua(0, out_val);
			if (!strlen(lua_name)) continue;
			if (strcmp(cfg_name, lua_name) == 0) break;
		}
		if (event < 0) {
			fprintf(stderr, "Sample '%s' not in audio.lua\n", cfg_name);
			continue;
		}

		/* Advance the sample list pointer so it's at the beginning of text */
		sample_list++;
		if (!sample_list[0]) continue;

		/* Terminate the current token */
		cur_token = sample_list;
		search = strchr(cur_token, ' ');
		if (search) {
			search[0] = '\0';
			next_token = search + 1;
		} else {
			next_token = NULL;
		}

		/*
		 * Now we find all the sample names and add them one by one
		*/
		events_loaded_semaphore = FALSE;
		while (cur_token) {
			int num = samples[event].num;

			/* Don't allow too many samples */
			if (num >= MAX_SAMPLES) break;

			/* Build the path to the sample */
			path_build(path, sizeof(path), ANGBAND_DIR_XTRA_SOUND, cur_token);
			if (!my_fexists(path)) {
				fprintf(stderr, "Can't find sample '%s'\n", cur_token);
				goto next_token_snd;
			}

			/* Don't load now if we're not caching */
			if (no_cache) {
				/* Just save the path for later */
				samples[event].paths[num] = string_make(path);
			} else {
				/* Load the file now */
				samples[event].wavs[num] = Mix_LoadWAV(path);
				if (!samples[event].wavs[num]) {
					plog_fmt("%s: %s", SDL_GetError(), strerror(errno));
					puts(format("%s: %s (%s)", SDL_GetError(), strerror(errno), path));//DEBUG USE_SOUND_2010
					goto next_token_snd;
				}
			}
			/* Initialize as 'not being played' */
			samples[event].current_channel = -1;

			//puts(format("loaded sample %s (ev %d, #%d).", samples[event].paths[num], event, num));//debug

			/* Imcrement the sample count */
			samples[event].num++;
			if (!events_loaded_semaphore) {
				events_loaded_semaphore = TRUE;
				audio_sfx++;
				/* for do_cmd_options_...(): remember that this sample was mentioned in our config file */
				samples[event].config = TRUE;
			}

			next_token_snd:

			/* Figure out next token */
			cur_token = next_token;
			if (next_token) {
				/* Try to find a space */
				search = strchr(cur_token, ' ');

				/* If we can find one, terminate, and set new "next" */
				if (search) {
					search[0] = '\0';
					next_token = search + 1;
				} else {
					/* Otherwise prevent infinite looping */
					next_token = NULL;
				}
			}
		}

		/* disable this sfx? */
		if (disabled) samples[event].disabled = TRUE;
	}

#ifdef WINDOWS
	/* On Windows we must have a second config file just to store disabled-state, since we cannot write to Program Files folder after Win XP anymore..
	   So if it exists, let it override the normal config file. */
	if (!win_dontmoveuser && getenv("HOMEDRIVE") && getenv("HOMEPATH")) {
		strcpy(path, getenv("HOMEDRIVE"));
		strcat(path, getenv("HOMEPATH"));
		strcat(path, "\\TomeNET-nosound.cfg");
	} else path_build(path, sizeof(path), ANGBAND_DIR_XTRA_SOUND, "TomeNET-nosound.cfg"); //paranoia

	fff = my_fopen(path, "r");
	if (fff) {
		while (my_fgets(fff, buffer0, sizeof(buffer0)) == 0) {
			/* find out event state (disabled/enabled) */
			i = exec_lua(0, format("return get_sound_index(\"%s\")", buffer0));
			/* unknown (different game version/sound pack?) */
			if (i == -1 || !samples[i].config) continue;
			/* disable samples listed in this file */
			samples[i].disabled = TRUE;
		}
	}
#endif

	/* Close the file */
	my_fclose(fff);


	/* ------------------------------- Init Music */

	buffer = buffer0;

	/* Build the "music" path */
	path_build(path, sizeof(path), ANGBAND_DIR_XTRA, "music");
#ifndef SEGFAULT_HACK
	ANGBAND_DIR_XTRA_MUSIC = string_make(path);
#else
	strcpy(ANGBAND_DIR_XTRA_MUSIC, path);
#endif

	/* Find and open the config file */
#if 0
#ifdef WINDOWS
	/* On Windows we must have a second config file just to store disabled-state, since we cannot write to Program Files folder after Win XP anymore..
	   So if it exists, let it override the normal config file. */
	if (!win_dontmoveuser && getenv("HOMEDRIVE") && getenv("HOMEPATH")) {
		strcpy(path, getenv("HOMEDRIVE"));
		strcat(path, getenv("HOMEPATH"));
		strcat(path, "\\TomeNET-music.cfg");
	} else path_build(path, sizeof(path), ANGBAND_DIR_XTRA_MUSIC, "TomeNET-music.cfg"); //paranoia

	fff = my_fopen(path, "r");
	if (!fff) {
		path_build(path, sizeof(path), ANGBAND_DIR_XTRA_MUSIC, "music.cfg");
		fff = my_fopen(path, "r");
	}
#else
	path_build(path, sizeof(path), ANGBAND_DIR_XTRA_MUSIC, "music.cfg");
	fff = my_fopen(path, "r");
#endif
#else
	path_build(path, sizeof(path), ANGBAND_DIR_XTRA_MUSIC, "music.cfg");
	fff = my_fopen(path, "r");
#endif

	/* Handle errors */
	if (!fff) {
#if 0
		plog_fmt("Failed to open music config (%s):\n    %s", path, strerror(errno));
		return FALSE;
#else /* try to use simple default file */
		FILE *fff2;
		char path2[2048];

		path_build(path, sizeof(path), ANGBAND_DIR_XTRA_MUSIC, "music.cfg.default");
		fff = my_fopen(path, "r");
		if (!fff) {
			plog_fmt("Failed to open default music config (%s):\n    %s", path, strerror(errno));
			return FALSE;
		}

		path_build(path2, sizeof(path2), ANGBAND_DIR_XTRA_MUSIC, "music.cfg");
		fff2 = my_fopen(path2, "w");
		if (!fff2) {
			plog_fmt("Failed to write music config (%s):\n    %s", path2, strerror(errno));
			return FALSE;
		}

		while (my_fgets(fff, buffer, sizeof(buffer)) == 0)
			fprintf(fff2, "%s\n", buffer);

		my_fclose(fff2);
		my_fclose(fff);

		/* Try again */
		path_build(path, sizeof(path), ANGBAND_DIR_XTRA_MUSIC, "music.cfg");
		fff = my_fopen(path, "r");
		if (!fff) {
			plog_fmt("Failed to open music config (%s):\n    %s", path, strerror(errno));
			return FALSE;
		}
#endif
	}

#ifdef DEBUG_SOUND
	puts("sound_sdl_init() reading music.cfg:");
#endif

	/* Parse the file */
	/* Lines are always of the form "name = music [music ...]" */
	while (my_fgets(fff, buffer0, sizeof(buffer0)) == 0) {
		char *cfg_name;
		cptr lua_name;
		char *song_list;
		char *search;
		char *cur_token;
		char *next_token;
		int event;

		/* Lines starting on ';' count as 'provided event' but actually
		   remain silent, as a way of disabling effects/songs without
		   letting the server know. */
		buffer = buffer0;
		if (buffer0[0] == ';') {
			buffer++;
			disabled = TRUE;
		} else disabled = FALSE;

		/* Skip anything not beginning with an alphabetic character */
		if (!buffer[0] || !isalpha((unsigned char)buffer[0])) continue;

		/* Split the line into two: message name, and the rest */
		search = strchr(buffer, ' ');
		song_list = strchr(search + 1, ' ');
		/* no event name given? */
		if (!search) continue;
		/* no audio filenames listed? */
		if (!song_list) continue;

		/* Set the message name, and terminate at first space */
		cfg_name = buffer;
		search[0] = '\0';

		/* Make sure this is a valid event name */
		for (event = MUSIC_MAX - 1; event >= 0; event--) {
			sprintf(out_val, "return get_music_name(%d)", event);
			lua_name = string_exec_lua(0, out_val);
			if (!strlen(lua_name)) continue;
			if (strcmp(cfg_name, lua_name) == 0) break;
		}
		if (event < 0) {
			fprintf(stderr, "Song '%s' not in audio.lua\n", cfg_name);
			continue;
		}

		/* Advance the sample list pointer so it's at the beginning of text */
		song_list++;
		if (!song_list[0]) continue;

		/* Terminate the current token */
		cur_token = song_list;
		if (cur_token[0] == '\"') {
			cur_token++;
			search = strchr(cur_token, '\"');
			if (search) {
				search[0] = '\0';
				search = strchr(search + 1, ' ');
			}
		} else {
			search = strchr(cur_token, ' ');
		}
		if (search) {
			search[0] = '\0';
			next_token = search + 1;
		} else {
			next_token = NULL;
		}

		/*
		 * Now we find all the sample names and add them one by one
		*/
		events_loaded_semaphore = FALSE;
		while (cur_token) {
			int num = songs[event].num;

			/* Don't allow too many songs */
			if (num >= MAX_SONGS) break;

			/* Build the path to the sample */
			path_build(path, sizeof(path), ANGBAND_DIR_XTRA_MUSIC, cur_token);
			if (!my_fexists(path)) {
				fprintf(stderr, "Can't find song '%s'\n", cur_token);
				goto next_token_mus;
			}

			/* Don't load now if we're not caching */
			if (no_cache) {
				/* Just save the path for later */
				songs[event].paths[num] = string_make(path);
			} else {
				/* Load the file now */
				songs[event].wavs[num] = Mix_LoadMUS(path);
				if (!songs[event].wavs[num]) {
//					puts(format("PRBPATH: lua_name %s, ev %d, num %d, path %s", lua_name, event, num, path));
					plog_fmt("%s: %s", SDL_GetError(), strerror(errno));
//					puts(format("%s: %s", SDL_GetError(), strerror(errno)));//DEBUG USE_SOUND_2010
					goto next_token_mus;
				}
			}

			//puts(format("loaded song %s (ev %d, #%d).", songs[event].paths[num], event, num));//debug
			/* Imcrement the sample count */
			songs[event].num++;
			if (!events_loaded_semaphore) {
				events_loaded_semaphore = TRUE;
				audio_music++;
				/* for do_cmd_options_...(): remember that this sample was mentioned in our config file */
				songs[event].config = TRUE;
			}

			next_token_mus:

			/* Figure out next token */
			cur_token = next_token;
			if (next_token) {
				/* Try to find a space */
				if (cur_token[0] == '\"') {
					cur_token++;
					search = strchr(cur_token, '\"');
					if (search) {
						search[0] = '\0';
						search = strchr(search + 1, ' ');
					}
				} else {
					search = strchr(cur_token, ' ');
				}
				/* If we can find one, terminate, and set new "next" */
				if (search) {
					search[0] = '\0';
					next_token = search + 1;
				} else {
					/* Otherwise prevent infinite looping */
					next_token = NULL;
				}
			}
		}

		/* disable this song? */
		if (disabled) songs[event].disabled = TRUE;
	}

	/* Close the file */
	my_fclose(fff);

#ifdef WINDOWS
	/* On Windows we must have a second config file just to store disabled-state, since we cannot write to Program Files folder after Win XP anymore..
	   So if it exists, let it override the normal config file. */
	if (!win_dontmoveuser && getenv("HOMEDRIVE") && getenv("HOMEPATH")) {
		strcpy(path, getenv("HOMEDRIVE"));
		strcat(path, getenv("HOMEPATH"));
		strcat(path, "\\TomeNET-nomusic.cfg");
	} else path_build(path, sizeof(path), ANGBAND_DIR_XTRA_MUSIC, "TomeNET-nomusic.cfg"); //paranoia

	fff = my_fopen(path, "r");
	if (fff) {
		while (my_fgets(fff, buffer0, sizeof(buffer0)) == 0) {
			/* find out event state (disabled/enabled) */
			i = exec_lua(0, format("return get_music_index(\"%s\")", buffer0));
			/* unknown (different game version/music pack?) */
			if (i == -1 || !songs[i].config) continue;
			/* disable songs listed in this file */
			songs[i].disabled = TRUE;
		}
	}
#endif

#ifdef DEBUG_SOUND
	puts("sound_sdl_init() done.");
#endif

	/* Success */
	return TRUE;
}

/*
 * Play a sound of type "event". Returns FALSE if sound couldn't be played.
 */
static bool play_sound(int event, int type, int vol, s32b player_id) {
	Mix_Chunk *wave = NULL;
	int s;
	bool test = FALSE;

#ifdef DISABLE_MUTED_AUDIO
	if (!cfg_audio_master || !cfg_audio_sound) return TRUE; /* claim that it 'succeeded' */
#endif

	/* hack: */
	if (type == SFX_TYPE_STOP) {
		/* stop all SFX_TYPE_NO_OVERLAP type sounds? */
		if (event == -1) {
			bool found = FALSE;

			for (s = 0; s < cfg_max_channels; s++) {
				if (channel_type[s] == SFX_TYPE_NO_OVERLAP && channel_player_id[s] == player_id) {
					Mix_FadeOutChannel(s, 450); //250..450 (more realistic timing vs smoother sound (avoid final 'spike'))
					found = TRUE;
				}
			}
			return found;
		}
		/* stop sound of this type? */
		else {
			for (s = 0; s < cfg_max_channels; s++) {
				if (channel_sample[s] == event && channel_player_id[s] == player_id) {
					Mix_FadeOutChannel(s, 450); //250..450 (more realistic timing vs smoother sound (avoid final 'spike'))
					return TRUE;
				}
			}
			return FALSE;
		}
	}

	/* Paranoia */
	if (event < 0 || event >= SOUND_MAX_2010) return FALSE;

	if (samples[event].disabled) return TRUE; /* claim that it 'succeeded' */

	/* Check there are samples for this event */
	if (!samples[event].num) return FALSE;

	/* already playing? allow to prevent multiple sounds of the same kind
	   from being mixed simultaneously, for preventing silliness */
	switch (type) {
	case SFX_TYPE_ATTACK: if (c_cfg.ovl_sfx_attack) break; else test = TRUE;
		break;
	case SFX_TYPE_COMMAND: if (c_cfg.ovl_sfx_command) break; else test = TRUE;
		break;
	case SFX_TYPE_MISC: if (c_cfg.ovl_sfx_misc) break; else test = TRUE;
		break;
	case SFX_TYPE_MON_ATTACK: if (c_cfg.ovl_sfx_mon_attack) break; else test = TRUE;
		break;
	case SFX_TYPE_MON_SPELL: if (c_cfg.ovl_sfx_mon_spell) break; else test = TRUE;
		break;
	case SFX_TYPE_MON_MISC: if (c_cfg.ovl_sfx_mon_misc) break; else test = TRUE;
		break;
	case SFX_TYPE_NO_OVERLAP: /* never overlap! (eg tunnelling) */
		test = TRUE;
		break;
	case SFX_TYPE_WEATHER:
	case SFX_TYPE_AMBIENT:
		/* always allow overlapping, since these should be sent by the
		   server in a completely sensibly timed manner anyway. */
		break;
	default:
		test = TRUE;
	}
	if (test) {
#if 0 /* old method before sounds could've come from other players nearby us, too */
		if (samples[event].current_channel != -1) return TRUE;
#else /* so now we need to allow multiple samples, IF they stem from different sources aka players */
		for (s = 0; s < cfg_max_channels; s++) {
			if (channel_sample[s] == event && channel_player_id[s] == player_id) return TRUE;
		}
#endif
	}

	/* prevent playing duplicate sfx that were initiated very closely
	   together in time, after one each other? (efficiency) */
	if (c_cfg.no_ovl_close_sfx && ticks == samples[event].started_timer_tick) return TRUE;


	/* Choose a random event */
	s = rand_int(samples[event].num);
	wave = samples[event].wavs[s];

	/* Try loading it, if it's not cached */
	if (!wave) {
		if (on_demand_loading || no_cache_audio) {
			if (!(wave = load_sample(event, s))) {
				/* we really failed to load it */
				plog(format("SDL sound load failed (%d, %d).", event, s));
				return FALSE;
			}
		} else {
			/* Fail silently */
			return TRUE;
		}
	}

	/* Actually play the thing */
	/* remember, for efficiency management */
	s = Mix_PlayChannel(-1, wave, 0);
	if (s != -1) {
		channel_sample[s] = event;
		channel_type[s] = type;
		channel_volume[s] = vol;
		channel_player_id[s] = player_id;

		/* HACK - use weather volume for thunder sfx */
		if (type == SFX_TYPE_WEATHER)
			Mix_Volume(s, (CALC_MIX_VOLUME(cfg_audio_weather, (cfg_audio_weather_volume * vol) / 100) * grid_weather_volume) / 100);
		else
		if (type == SFX_TYPE_AMBIENT)
			Mix_Volume(s, (CALC_MIX_VOLUME(cfg_audio_sound, (cfg_audio_sound_volume * vol) / 100) * grid_ambient_volume) / 100);
		else
		/* Note: Linear scaling is used here to allow more precise control at the server end */
			Mix_Volume(s, CALC_MIX_VOLUME(cfg_audio_sound, (cfg_audio_sound_volume * vol) / 100));

//puts(format("playing sample %d at vol %d.\n", event, (cfg_audio_sound_volume * vol) / 100));
	}
	samples[event].current_channel = s;
	samples[event].started_timer_tick = ticks;

	return TRUE;
}
/* play the 'bell' sound */
#define BELL_REDUCTION 3 /* reduce volume of bell() sounds by this factor */
extern bool sound_bell(void) {
	Mix_Chunk *wave = NULL;
	int s;

	if (bell_sound_idx == -1) return FALSE;
	if (samples[bell_sound_idx].disabled) return TRUE; /* claim that it 'succeeded' */
	if (!samples[bell_sound_idx].num) return FALSE;

	/* already playing? prevent multiple sounds of the same kind from being mixed simultaneously, for preventing silliness */
	if (samples[bell_sound_idx].current_channel != -1) return TRUE;

	/* Choose a random event */
	s = rand_int(samples[bell_sound_idx].num);
	wave = samples[bell_sound_idx].wavs[s];

	/* Try loading it, if it's not cached */
	if (!wave) {
		if (on_demand_loading || no_cache_audio) {
			if (!(wave = load_sample(bell_sound_idx, s))) {
				/* we really failed to load it */
				plog(format("SDL sound load failed (%d, %d).", bell_sound_idx, s));
				return FALSE;
			}
		} else {
			/* Fail silently */
			return TRUE;
		}
	}

	/* Actually play the thing */
	/* remember, for efficiency management */
	s = Mix_PlayChannel(-1, wave, 0);
	if (s != -1) {
		channel_sample[s] = bell_sound_idx;
		channel_type[s] = SFX_TYPE_AMBIENT; /* whatever (just so overlapping is possible) */
		if (c_cfg.paging_max_volume) {
			Mix_Volume(s, MIX_MAX_VOLUME / BELL_REDUCTION);
		} else if (c_cfg.paging_master_volume) {
			Mix_Volume(s, CALC_MIX_VOLUME(1, 100 / BELL_REDUCTION));
		}
	}
	samples[bell_sound_idx].current_channel = s;

	return TRUE;
}
/* play the 'page' sound */
extern bool sound_page(void) {
	Mix_Chunk *wave = NULL;
	int s;

	if (page_sound_idx == -1) return FALSE;
	if (samples[page_sound_idx].disabled) return TRUE; /* claim that it 'succeeded' */
	if (!samples[page_sound_idx].num) return FALSE;

	/* already playing? prevent multiple sounds of the same kind from being mixed simultaneously, for preventing silliness */
	if (samples[page_sound_idx].current_channel != -1) return TRUE;

	/* Choose a random event */
	s = rand_int(samples[page_sound_idx].num);
	wave = samples[page_sound_idx].wavs[s];

	/* Try loading it, if it's not cached */
	if (!wave) {
		if (on_demand_loading || no_cache_audio) {
			if (!(wave = load_sample(page_sound_idx, s))) {
				/* we really failed to load it */
				plog(format("SDL sound load failed (%d, %d).", page_sound_idx, s));
				return FALSE;
			}
		} else {
			/* Fail silently */
			return TRUE;
		}
	}

	/* Actually play the thing */
	/* remember, for efficiency management */
	s = Mix_PlayChannel(-1, wave, 0);
	if (s != -1) {
		channel_sample[s] = page_sound_idx;
		channel_type[s] = SFX_TYPE_AMBIENT; /* whatever (just so overlapping is possible) */
		if (c_cfg.paging_max_volume) {
			Mix_Volume(s, MIX_MAX_VOLUME);
		} else if (c_cfg.paging_master_volume) {
			Mix_Volume(s, CALC_MIX_VOLUME(1, 100));
		}
	}
	samples[page_sound_idx].current_channel = s;

	return TRUE;
}
/* play the 'warning' sound */
extern bool sound_warning(void) {
	Mix_Chunk *wave = NULL;
	int s;

	if (warning_sound_idx == -1) return FALSE;
	if (samples[warning_sound_idx].disabled) return TRUE; /* claim that it 'succeeded' */
	if (!samples[warning_sound_idx].num) return FALSE;

	/* already playing? prevent multiple sounds of the same kind from being mixed simultaneously, for preventing silliness */
	if (samples[warning_sound_idx].current_channel != -1) return TRUE;

	/* Choose a random event */
	s = rand_int(samples[warning_sound_idx].num);
	wave = samples[warning_sound_idx].wavs[s];

	/* Try loading it, if it's not cached */
	if (!wave) {
		if (on_demand_loading || no_cache_audio) {
			if (!(wave = load_sample(warning_sound_idx, s))) {
				/* we really failed to load it */
				plog(format("SDL sound load failed (%d, %d).", warning_sound_idx, s));
				return FALSE;
			}
		} else {
			/* Fail silently */
			return TRUE;
		}
	}

	/* Actually play the thing */
	/* remember, for efficiency management */
	s = Mix_PlayChannel(-1, wave, 0);
	if (s != -1) {
		channel_sample[s] = warning_sound_idx;
		channel_type[s] = SFX_TYPE_AMBIENT; /* whatever (just so overlapping is possible) */
		/* use 'page' sound volume settings for warning too */
		if (c_cfg.paging_max_volume) {
			Mix_Volume(s, MIX_MAX_VOLUME);
		} else if (c_cfg.paging_master_volume) {
			Mix_Volume(s, CALC_MIX_VOLUME(1, 100));
		}
	}
	samples[warning_sound_idx].current_channel = s;

	return TRUE;
}


/* Release weather channel after fading out has been completed */
static void clear_channel(int c) {
	/* weather has faded out, mark it as gone */
	if (c == weather_channel) {
		weather_channel = -1;
		return;
	}

	if (c == ambient_channel) {
		ambient_channel = -1;
		return;
	}

	/* a sample has finished playing, so allow this kind to be played again */
	/* hack: if the sample was the 'paging' sound, reset the channel's volume to be on the safe side */
	if (channel_sample[c] == page_sound_idx || channel_sample[c] == warning_sound_idx)
		Mix_Volume(c, CALC_MIX_VOLUME(cfg_audio_sound, cfg_audio_sound_volume));

	/* HACK - if the sample was a weather sample, which would be thunder, reset the vol too, paranoia */
	if (channel_type[c] == SFX_TYPE_WEATHER)
		Mix_Volume(c, CALC_MIX_VOLUME(cfg_audio_sound, cfg_audio_sound_volume));

	samples[channel_sample[c]].current_channel = -1;
	channel_sample[c] = -1;
}

/* Overlay a weather noise -- not WEATHER_VOL_PARTICLES or WEATHER_VOL_CLOUDS */
static void play_sound_weather(int event) {
	Mix_Chunk *wave = NULL;
	int s, new_wc;

	/* allow halting a muted yet playing sound, before checking for DISABLE_MUTED_AUDIO */
	if (event == -2 && weather_channel != -1) {
#ifdef DEBUG_SOUND
		puts(format("w-2: wco %d, ev %d", weather_channel, event));
#endif
		Mix_HaltChannel(weather_channel);
		weather_current = -1;
		return;
	}

	if (event == -1 && weather_channel != -1) {
#ifdef DEBUG_SOUND
		puts(format("w-1: wco %d, ev %d", weather_channel, event));
#endif

		/* if channel is muted anyway, no need to fade out, just halt it instead.
		   HACK: The reason for this is actually to fix this bug:
		   There was a bug where ambient channel wasn't faded out if it was actually
		   muted at the same time, and so it'd continue being active when unmuted
		   again, instead of having been terminated by the fade-out.  */
		if (!cfg_audio_master || !cfg_audio_weather || !cfg_audio_weather_volume) {
			Mix_HaltChannel(weather_channel);
			return;
		}

		if (Mix_FadingChannel(weather_channel) != MIX_FADING_OUT) {
			if (!weather_channel_volume) Mix_HaltChannel(weather_channel); else //hack: workaround SDL bug that doesn't terminate a sample playing at 0 volume after a FadeOut
			Mix_FadeOutChannel(weather_channel, 2000);
		}
		weather_current = -1;
		return;
	}

#ifdef DISABLE_MUTED_AUDIO
	if (!cfg_audio_master || !cfg_audio_weather) return;
#endif

	/* we're already in this weather? */
	if (weather_channel != -1 && weather_current == event &&
	    Mix_FadingChannel(weather_channel) != MIX_FADING_OUT)
		return;

	/* Paranoia */
	if (event < 0 || event >= SOUND_MAX_2010) return;

	if (samples[event].disabled) {
		weather_current = event;
		weather_current_vol = -1;
		return;
	}

	/* Check there are samples for this event */
	if (!samples[event].num) {
		/* stop previous weather sound */
#if 0 /* stop apruptly */
		if (weather_channel != -1) Mix_HaltChannel(weather_channel);
#else /* fade out */
		if (weather_channel != -1 &&
		    Mix_FadingChannel(weather_channel) != MIX_FADING_OUT) {
			if (!weather_channel_volume) Mix_HaltChannel(weather_channel); else //hack: workaround SDL bug that doesn't terminate a sample playing at 0 volume after a FadeOut
			Mix_FadeOutChannel(weather_channel, 2000);
		}
#endif
		return;
	}

	/* Choose a random event */
	s = rand_int(samples[event].num);
	wave = samples[event].wavs[s];

	/* Try loading it, if it's not cached */
	if (!wave) {
		if (on_demand_loading || no_cache_audio) {
			if (!(wave = load_sample(event, s))) {
				/* we really failed to load it */
				plog(format("SDL sound load failed (%d, %d).", event, s));
				return;
			}
		} else {
			/* Fail silently */
			return;
		}
	}

	/* Actually play the thing */
#if 1 /* volume glitch paranoia (first fade-in seems to move volume to 100% instead of designated cfg_audio_... */
	new_wc = Mix_PlayChannel(weather_channel, wave, -1);
	if (new_wc != -1) {
		Mix_Volume(new_wc, (CALC_MIX_VOLUME(cfg_audio_weather, cfg_audio_weather_volume) * grid_weather_volume) / 100);

		/* weird bug (see above) apparently STILL occurs for some people (Dj_wolf) - hack it moar: */
		if (cfg_audio_weather)

		if (!weather_resume) new_wc = Mix_FadeInChannel(new_wc, wave, -1, 500);
	}
#else
	if (!weather_resume) new_wc = Mix_FadeInChannel(weather_channel, wave, -1, 500);
#endif

	if (!weather_resume) weather_fading = 1;
	else weather_resume = FALSE;

#ifdef DEBUG_SOUND
	puts(format("old: %d, new: %d, ev: %d", weather_channel, new_wc, event));
#endif

#if 1
	/* added this <if> after weather seemed to glitch sometimes,
	   with its channel becoming unmutable */
	//we didn't play weather so far?
	if (weather_channel == -1) {
		//failed to start playing?
		if (new_wc == -1) return;

		//successfully started playing the first weather
		weather_channel = new_wc;
		weather_current = event;
		weather_current_vol = -1;
		Mix_Volume(weather_channel, (CALC_MIX_VOLUME(cfg_audio_weather, cfg_audio_weather_volume) * grid_weather_volume) / 100);
	} else {
		//failed to start playing?
		if (new_wc == -1) {
			Mix_HaltChannel(weather_channel);
			return;
		//successfully started playing a follow-up weather
		} else {
			//same channel?
			if (new_wc == weather_channel) {
				weather_current = event;
				weather_current_vol = -1;
			//different channel?
			} else {
				Mix_HaltChannel(weather_channel);
				weather_channel = new_wc;
				weather_current = event;
				weather_current_vol = -1;
				Mix_Volume(weather_channel, (CALC_MIX_VOLUME(cfg_audio_weather, cfg_audio_weather_volume) * grid_weather_volume) / 100);
			}
		}
	}
#endif
#if 0
	/* added this <if> after weather seemed to glitch sometimes,
	   with its channel becoming unmutable */
	if (new_wc != weather_channel) {
		if (weather_channel != -1) Mix_HaltChannel(weather_channel);
		weather_channel = new_wc;
	}
	if (weather_channel != -1) { //paranoia? should always be != -1 at this point
		weather_current = event;
		weather_current_vol = -1;
		Mix_Volume(weather_channel, (CALC_MIX_VOLUME(cfg_audio_weather, cfg_audio_weather_volume) * grid_weather_volume) / 100);
	}
#endif

#ifdef DEBUG_SOUND
	puts(format("now: %d, oev: %d, ev: %d", weather_channel, event, weather_current));
#endif
}


/* Overlay a weather noise, with a certain volume -- WEATHER_VOL_PARTICLES */
static void play_sound_weather_vol(int event, int vol) {
	Mix_Chunk *wave = NULL;
	int s, new_wc;

	/* allow halting a muted yet playing sound, before checking for DISABLE_MUTED_AUDIO */
	if (event == -2 && weather_channel != -1) {
#ifdef DEBUG_SOUND
		puts(format("w-2: wco %d, ev %d", weather_channel, event));
#endif
		Mix_HaltChannel(weather_channel);
		weather_current = -1;
		return;
	}

	if (event == -1 && weather_channel != -1) {
#ifdef DEBUG_SOUND
		puts(format("w-1: wco %d, ev %d", weather_channel, event));
#endif

		/* if channel is muted anyway, no need to fade out, just halt it instead.
		   HACK: The reason for this is actually to fix this bug:
		   There was a bug where ambient channel wasn't faded out if it was actually
		   muted at the same time, and so it'd continue being active when unmuted
		   again, instead of having been terminated by the fade-out.  */
		if (!cfg_audio_master || !cfg_audio_weather || !cfg_audio_weather_volume) {
			Mix_HaltChannel(weather_channel);
			return;
		}

		if (Mix_FadingChannel(weather_channel) != MIX_FADING_OUT) {
			if (!weather_channel_volume) Mix_HaltChannel(weather_channel); else //hack: workaround SDL bug that doesn't terminate a sample playing at 0 volume after a FadeOut
			Mix_FadeOutChannel(weather_channel, 2000);
		}
		weather_current = -1;
		return;
	}

#ifdef DISABLE_MUTED_AUDIO
	if (!cfg_audio_master || !cfg_audio_weather) return;
#endif

	/* we're already in this weather? */
	if (weather_channel != -1 && weather_current == event &&
	    Mix_FadingChannel(weather_channel) != MIX_FADING_OUT) {
		int v = (cfg_audio_weather_volume * vol) / 100, n, va = 0;

		/* shift array and calculate average over the last n volume values */
		for (n = 20 - 1; n > 0; n--) {
			va += weather_smooth_avg[n];
			/* and shift array to make room for new value */
			weather_smooth_avg[n] = weather_smooth_avg[n - 1];
		}
		va += weather_smooth_avg[0];//add the last element
		va /= 20;//calculate average
		/* queue new value */
		weather_smooth_avg[0] = v;

		/* change volume though? */
//c_message_add(format("vol %d, v %d", vol, v));
		if (weather_vol_smooth < va - 20) {
			weather_vol_smooth_anti_oscill = va;
//c_message_add(format("smooth++ %d (vol %d)", weather_vol_smooth, vol));
		}
		else if (weather_vol_smooth > va + 20) {
			weather_vol_smooth_anti_oscill = va;
//c_message_add(format("smooth-- %d (vol %d)", weather_vol_smooth, vol));
		}

		if (weather_vol_smooth < weather_vol_smooth_anti_oscill)
			weather_vol_smooth++;
		else if (weather_vol_smooth > weather_vol_smooth_anti_oscill)
				weather_vol_smooth--;

//c_message_add(format("smooth %d", weather_vol_smooth));
		Mix_Volume(weather_channel, (CALC_MIX_VOLUME(cfg_audio_weather, weather_vol_smooth) * grid_weather_volume) / 100);

		/* Done */
		return;
	}

	/* Paranoia */
	if (event < 0 || event >= SOUND_MAX_2010) return;

	if (samples[event].disabled) {
		weather_current = event; //pretend we play it
		weather_current_vol = vol;
		return;
	}

	/* Check there are samples for this event */
	if (!samples[event].num) return;

	/* Choose a random event */
	s = rand_int(samples[event].num);
	wave = samples[event].wavs[s];

	/* Try loading it, if it's not cached */
	if (!wave) {
		if (on_demand_loading || no_cache_audio) {
			if (!(wave = load_sample(event, s))) {
				/* we really failed to load it */
				plog(format("SDL sound load failed (%d, %d).", event, s));
				return;
			}
		} else {
			/* Fail silently */
			return;
		}
	}

	/* Actually play the thing */
#if 1 /* volume glitch paranoia (first fade-in seems to move volume to 100% instead of designated cfg_audio_... */
	new_wc = Mix_PlayChannel(weather_channel, wave, -1);
	if (new_wc != -1) {
		weather_vol_smooth = (cfg_audio_weather_volume * vol) / 100; /* set initially, instantly */
		Mix_Volume(new_wc, (CALC_MIX_VOLUME(cfg_audio_weather, weather_vol_smooth) * grid_weather_volume) / 100);

		/* weird bug (see above) apparently might STILL occur for some people - hack it moar: */
		if (cfg_audio_weather)

		if (!weather_resume) new_wc = Mix_FadeInChannel(new_wc, wave, -1, 500);
	}
#else
	if (!weather_resume) new_wc = Mix_FadeInChannel(weather_channel, wave, -1, 500);
#endif

	if (!weather_resume) weather_fading = 1;
	else weather_resume = FALSE;

#ifdef DEBUG_SOUND
	puts(format("old: %d, new: %d, ev: %d", weather_channel, new_wc, event));
#endif

#if 1
	/* added this <if> after weather seemed to glitch sometimes,
	   with its channel becoming unmutable */
	//we didn't play weather so far?
	if (weather_channel == -1) {
		//failed to start playing?
		if (new_wc == -1) return;

		//successfully started playing the first weather
		weather_channel = new_wc;
		weather_current = event;
		weather_current_vol = vol;

		weather_vol_smooth = (cfg_audio_weather_volume * vol) / 100; /* set initially, instantly */
		Mix_Volume(weather_channel, (CALC_MIX_VOLUME(cfg_audio_weather, weather_vol_smooth) * grid_weather_volume) / 100);
	} else {
		//failed to start playing?
		if (new_wc == -1) {
			Mix_HaltChannel(weather_channel);
			return;
		//successfully started playing a follow-up weather
		} else {
			//same channel?
			if (new_wc == weather_channel) {
				weather_current = event;
				weather_current_vol = vol;
			//different channel?
			} else {
				Mix_HaltChannel(weather_channel);
				weather_channel = new_wc;
				weather_current = event;
				weather_current_vol = vol;
				weather_vol_smooth = (cfg_audio_weather_volume * vol) / 100; /* set initially, instantly */
				Mix_Volume(weather_channel, (CALC_MIX_VOLUME(cfg_audio_weather, weather_vol_smooth) * grid_weather_volume) / 100);
			}
		}
	}
#endif
#if 0
	/* added this <if> after weather seemed to glitch sometimes,
	   with its channel becoming unmutable */
	if (new_wc != weather_channel) {
		if (weather_channel != -1) Mix_HaltChannel(weather_channel);
		weather_channel = new_wc;
	}
	if (weather_channel != -1) { //paranoia? should always be != -1 at this point
		weather_current = event;
		weather_current_vol = vol;
		Mix_Volume(weather_channel, (CALC_MIX_VOLUME(cfg_audio_weather, (cfg_audio_weather_volume * vol) / 100) * grid_weather_volume) / 100);
	}
#endif

#ifdef DEBUG_SOUND
	puts(format("now: %d, oev: %d, ev: %d", weather_channel, event, weather_current));
#endif
}

/* make sure volume is set correct after fading-in has been completed (might be just paranoia) */
void weather_handle_fading(void) {
	if (weather_channel == -1) { //paranoia
		weather_fading = 0;
		return;
	}

	if (Mix_FadingChannel(weather_channel) == MIX_NO_FADING) {
#ifndef WEATHER_VOL_PARTICLES
		Mix_Volume(weather_channel, (CALC_MIX_VOLUME(cfg_audio_weather, cfg_audio_weather_volume) * grid_weather_volume) / 100);
#else
		Mix_Volume(weather_channel, (CALC_MIX_VOLUME(cfg_audio_weather, weather_vol_smooth) * grid_weather_volume) / 100);
#endif
		weather_fading = 0;
		return;
	}
}

/* Overlay an ambient sound effect */
static void play_sound_ambient(int event) {
	Mix_Chunk *wave = NULL;
	int s, new_ac;

#ifdef DEBUG_SOUND
	puts(format("psa: ch %d, ev %d", ambient_channel, event));
#endif

	/* allow halting a muted yet playing sound, before checking for DISABLE_MUTED_AUDIO */
	if (event == -2 && ambient_channel != -1) {
#ifdef DEBUG_SOUND
		puts(format("w-2: wco %d, ev %d", ambient_channel, event));
#endif
		Mix_HaltChannel(ambient_channel);
		ambient_current = -1;
		return;
	}

	if (event == -1 && ambient_channel != -1) {
#ifdef DEBUG_SOUND
		puts(format("w-1: wco %d, ev %d", ambient_channel, event));
#endif

		/* if channel is muted anyway, no need to fade out, just halt it instead.
		   HACK: The reason for this is actually to fix this bug:
		   There was a bug where ambient channel wasn't faded out if it was actually
		   muted at the same time, and so it'd continue being active when unmuted
		   again, instead of having been terminated by the fade-out.  */
		if (!cfg_audio_master || !cfg_audio_sound || !cfg_audio_sound_volume) {
			Mix_HaltChannel(ambient_channel);
			return;
		}

		if (Mix_FadingChannel(ambient_channel) != MIX_FADING_OUT) {
			if (!ambient_channel_volume) Mix_HaltChannel(ambient_channel); else //hack: workaround SDL bug that doesn't terminate a sample playing at 0 volume after a FadeOut
			Mix_FadeOutChannel(ambient_channel, 2000);
		}
		ambient_current = -1;
		return;
	}

#ifdef DISABLE_MUTED_AUDIO
	if (!cfg_audio_master || !cfg_audio_sound) return;
#endif

	/* we're already in this ambient? */
	if (ambient_channel != -1 && ambient_current == event &&
	    Mix_FadingChannel(ambient_channel) != MIX_FADING_OUT)
		return;

	/* Paranoia */
	if (event < 0 || event >= SOUND_MAX_2010) return;

	if (samples[event].disabled) {
		ambient_current = event; //pretend we play it
		return;
	}

	/* Check there are samples for this event */
	if (!samples[event].num) {
		/* stop previous ambient sound */
#if 0 /* stop apruptly */
		if (ambient_channel != -1) Mix_HaltChannel(ambient_channel);
#else /* fade out */
		if (ambient_channel != -1 &&
		    Mix_FadingChannel(ambient_channel) != MIX_FADING_OUT) {
			if (!ambient_channel_volume) Mix_HaltChannel(ambient_channel); else //hack: workaround SDL bug that doesn't terminate a sample playing at 0 volume after a FadeOut
			Mix_FadeOutChannel(ambient_channel, 2000);
		}
#endif
		return;
	}

	/* Choose a random event */
	s = rand_int(samples[event].num);
	wave = samples[event].wavs[s];

	/* Try loading it, if it's not cached */
	if (!wave) {
#if 0 /* for ambient sounds. we don't drop them as we'd do for "normal " sfx, \
	 because they ought to be looped, so we'd completely lose them. - C. Blue */
		if (on_demand_loading || no_cache_audio) {
#endif
			if (!(wave = load_sample(event, s))) {
				/* we really failed to load it */
				plog(format("SDL sound load failed (%d, %d).", event, s));
				return;
			}
#if 0 /* see above */
		} else {
#ifdef DEBUG_SOUND
			puts(format("on_demand_loading %d, no_cache_audio %d", on_demand_loading, no_cache_audio));
#endif
			/* Fail silently */
			return;
		}
#endif
	}

	/* Actually play the thing */
#if 1 /* volume glitch paranoia (first fade-in seems to move volume to 100% instead of designated cfg_audio_... */
	new_ac = Mix_PlayChannel(ambient_channel, wave, -1);
	if (new_ac != -1) {
		Mix_Volume(new_ac, (CALC_MIX_VOLUME(cfg_audio_sound, cfg_audio_sound_volume) * grid_ambient_volume) / 100);

		/* weird bug (see above) apparently might STILL occur for some people - hack it moar: */
		if (cfg_audio_sound)

		if (!ambient_resume) new_ac = Mix_FadeInChannel(new_ac, wave, -1, 500);
	}
#else
	if (!ambient_resume) new_ac = Mix_FadeInChannel(ambient_channel, wave, -1, 500);
#endif

	if (!ambient_resume) ambient_fading = 1;
	else ambient_resume = FALSE;

#ifdef DEBUG_SOUND
	puts(format("old: %d, new: %d, ev: %d", ambient_channel, new_ac, event));
#endif

#if 1
	/* added this <if> after ambient seemed to glitch sometimes,
	   with its channel becoming unmutable */
	//we didn't play ambient so far?
	if (ambient_channel == -1) {
		//failed to start playing?
		if (new_ac == -1) return;

		//successfully started playing the first ambient
		ambient_channel = new_ac;
		ambient_current = event;
		Mix_Volume(ambient_channel, (CALC_MIX_VOLUME(cfg_audio_sound, cfg_audio_sound_volume) * grid_ambient_volume) / 100);
	} else {
		//failed to start playing?
		if (new_ac == -1) {
			Mix_HaltChannel(ambient_channel);
			return;
		//successfully started playing a follow-up ambient
		} else {
			//same channel?
			if (new_ac == ambient_channel) {
				ambient_current = event;
			//different channel?
			} else {
				Mix_HaltChannel(ambient_channel);
				ambient_channel = new_ac;
				ambient_current = event;
				Mix_Volume(ambient_channel, (CALC_MIX_VOLUME(cfg_audio_sound, cfg_audio_sound_volume) * grid_ambient_volume) / 100);
			}
		}
		
	}
#endif
#if 0
	/* added this <if> after ambient seemed to glitch sometimes,
	   with its channel becoming unmutable */
	if (new_ac != ambient_channel) {
		if (ambient_channel != -1) Mix_HaltChannel(ambient_channel);
		ambient_channel = new_ac;
	}
	if (ambient_channel != -1) { //paranoia? should always be != -1 at this point
		ambient_current = event;
		Mix_Volume(ambient_channel, (CALC_MIX_VOLUME(cfg_audio_sound, cfg_audio_sound_volume) * grid_ambient_volume) / 100);
	}
#endif

#ifdef DEBUG_SOUND
	puts(format("now: %d, oev: %d, ev: %d", ambient_channel, event, ambient_current));
#endif
}

/* make sure volume is set correct after fading-in has been completed (might be just paranoia) */
void ambient_handle_fading(void) {
	if (ambient_channel == -1) { //paranoia
		ambient_fading = 0;
		return;
	}

	if (Mix_FadingChannel(ambient_channel) == MIX_NO_FADING) {
		Mix_Volume(ambient_channel, (CALC_MIX_VOLUME(cfg_audio_sound, cfg_audio_sound_volume) * grid_ambient_volume) / 100);
		ambient_fading = 0;
		return;
	}
}

/*
 * Play a music of type "event".
 */
static bool play_music(int event) {
	/* Paranoia */
	if (event < -4 || event >= MUSIC_MAX) return FALSE;

	/* Don't play anything, just return "success", aka just keep playing what is currently playing.
	   This is used for when the server sends music that doesn't have an alternative option, but
	   should not stop the current music if it fails to play. */
	if (event == -1) return TRUE;

	/* We previously failed to play both music and alternative music.
	   Stop currently playing music before returning */
	if (event == -2) {
		if (Mix_PlayingMusic() && Mix_FadingMusic() != MIX_FADING_OUT)
			Mix_FadeOutMusic(500);
		music_cur = -1;
		return TRUE; //whatever..
	}

	/* 'shuffle_music' option changed? */
	if (event == -3) {
		if (c_cfg.shuffle_music) {
			music_next = -1;
			Mix_FadeOutMusic(500);
		} else {
			music_next = music_cur; //hack
			music_next_song = music_cur_song;
		}
		return TRUE; //whatever..
	}

#ifdef ATMOSPHERIC_INTRO
	/* New, for title screen -> character screen switch: Halt current music */
	if (event == -4) {
		/* Stop currently playing music though, before returning */
		if (Mix_PlayingMusic() && Mix_FadingMusic() != MIX_FADING_OUT)
			Mix_FadeOutMusic(2000);
		music_cur = -1;
		return TRUE; /* claim that it 'succeeded' */
	}
#endif

	/* Check there are samples for this event */
	if (!songs[event].num) return FALSE;

	/* if music event is the same as is already running, don't do anything */
	if (music_cur == event && Mix_PlayingMusic() && Mix_FadingMusic() != MIX_FADING_OUT)
		return TRUE; //pretend we played it

	music_next = event;
	music_next_song = rand_int(songs[music_next].num);

	/* new: if upcoming music file is same as currently playing one, don't do anything */
	if (music_cur != -1 && music_cur_song != -1 &&
	    songs[music_next].num /* Check there are samples for this event */
	    && !strcmp(
	     songs[music_next].paths[music_next_song], /* Choose a random event and pretend it's the one that would've gotten picked */
	     songs[music_cur].paths[music_cur_song]
	     )) {
		music_next = event;
		music_next_song = music_cur_song;
		return TRUE;
	}

	/* check if music is already running, if so, fade it out first! */
	if (Mix_PlayingMusic()) {
		if (Mix_FadingMusic() != MIX_FADING_OUT)
			Mix_FadeOutMusic(500);
	} else {
		//play immediately
		fadein_next_music();
	}
	return TRUE;
}

static void fadein_next_music(void) {
	Mix_Music *wave = NULL;

#ifdef DISABLE_MUTED_AUDIO
	if (!cfg_audio_master || !cfg_audio_music) return;
#endif

	/* Catch music_next == -1, this can now happen with shuffle_music option, since songs are no longer looped if it's enabled */
	if (c_cfg.shuffle_music && music_next == -1) {
		/* Catch disabled songs */
		if (songs[music_cur].disabled) return;

		/* stick with music event, but play different song, randomly */
		int tries = songs[music_cur].num == 1 ? 1 : 100, mcs = music_cur_song;
		if (songs[music_cur].num < 1) return; //paranoia
		while (tries--) {
			mcs = rand_int(songs[music_cur].num);
			if (music_cur_song != mcs) break;
		}
		music_cur_song = mcs;

		/* Choose the predetermined random event */
		wave = songs[music_cur].wavs[music_cur_song];

		/* Try loading it, if it's not cached */
		if (!wave && !(wave = load_song(music_cur, music_cur_song))) {
			/* we really failed to load it */
			plog(format("SDL music load failed (%d, %d).", music_cur, music_cur_song));
			puts(format("SDL music load failed (%d, %d).", music_cur, music_cur_song));
			return;
		}

		/* Actually play the thing */
		//Mix_PlayMusic(wave, 0);//-1 infinite, 0 once, or n times
		Mix_FadeInMusic(wave, 0, 1000);
		return;
	}

	/* Paranoia */
	if (music_next < 0 || music_next >= MUSIC_MAX) return;

	/* Sub-song file was disabled? (local audio options) */
	if (songs[music_next].disabled) {
		music_cur = music_next;
		music_cur_song = music_next_song;
		music_next = -1; //pretend we play it
		return;
	}

	/* Check there are samples for this event */
	if (songs[music_next].num < music_next_song + 1) return;

	/* Choose the predetermined random event */
	wave = songs[music_next].wavs[music_next_song];

	/* Try loading it, if it's not cached */
	if (!wave && !(wave = load_song(music_next, music_next_song))) {
		/* we really failed to load it */
		plog(format("SDL music load failed (%d, %d).", music_next, music_next_song));
		puts(format("SDL music load failed (%d, %d).", music_next, music_next_song));
		return;
	}

	/* Actually play the thing */
//#ifdef DISABLE_MUTED_AUDIO /* now these vars are also used for 'continous' music across music events */
	music_cur = music_next;
	music_cur_song = music_next_song;
//#endif
	music_next = -1;
//	Mix_PlayMusic(wave, c_cfg.shuffle_music ? 0 : -1);//-1 infinite, 0 once, or n times
	Mix_FadeInMusic(wave, c_cfg.shuffle_music ? 0 : -1, 1000);
}

#ifdef DISABLE_MUTED_AUDIO
/* start playing current music again if we reenabled it in the mixer UI after having had it disabled */
static void reenable_music(void) {
	Mix_Music *wave = NULL;

	/* music has changed meanwhile, just play the new one the normal way */
	if (music_next != -1 && music_next != music_cur) {
		fadein_next_music();
		return;
	}

	/* music initialization not yet completed? (at the start of the game) */
	if (music_cur == -1 || music_cur_song == -1) return;

	wave = songs[music_cur].wavs[music_cur_song];

	/* If audio is still being loaded/cached, we might just have to exit here for now */
	if (!wave) return;

	/* Take up playing again immediately, no fading in */
	Mix_PlayMusic(wave, c_cfg.shuffle_music ? 0 : -1);
}
#endif

/*
 * Set mixing levels in the SDL module.
 */
static void set_mixing_sdl(void) {
//	puts(format("mixer set to %d, %d, %d.", cfg_audio_music_volume, cfg_audio_sound_volume, cfg_audio_weather_volume));
#if 0 /* don't use relative sound-effect specific volumes, transmitted from the server? */
	Mix_Volume(-1, CALC_MIX_VOLUME(cfg_audio_sound, cfg_audio_sound_volume));
#else /* use relative volumes (4.4.5b+) */
	int n;
	for (n = 0; n < cfg_max_channels; n++) {
		if (n == weather_channel) continue;
		if (n == ambient_channel) continue;

		/* HACK - use weather volume for thunder sfx */
		if (channel_sample[n] != -1 && channel_type[n] == SFX_TYPE_WEATHER)
			Mix_Volume(n, (CALC_MIX_VOLUME(cfg_audio_weather, (cfg_audio_weather_volume * channel_volume[n]) / 100) * grid_weather_volume) / 100);
		else
		/* grid_ambient_volume influences non-looped ambient sfx clips */
		if (channel_sample[n] != -1 && channel_type[n] == SFX_TYPE_AMBIENT)
			Mix_Volume(n, (CALC_MIX_VOLUME(cfg_audio_sound, (cfg_audio_sound_volume * channel_volume[n]) / 100) * grid_ambient_volume) / 100);
		else
		/* Note: Linear scaling is used here to allow more precise control at the server end */
			Mix_Volume(n, CALC_MIX_VOLUME(cfg_audio_sound, (cfg_audio_sound_volume * channel_volume[n]) / 100));

 #ifdef DISABLE_MUTED_AUDIO
		if ((!cfg_audio_master || !cfg_audio_sound) && Mix_Playing(n))
			Mix_HaltChannel(n);
 #endif
	}
#endif
	Mix_VolumeMusic(CALC_MIX_VOLUME(cfg_audio_music, cfg_audio_music_volume));
#ifdef DISABLE_MUTED_AUDIO
	if (!cfg_audio_master || !cfg_audio_music) {
		if (Mix_PlayingMusic()) Mix_HaltMusic();
	} else if (!Mix_PlayingMusic()) reenable_music();
#endif

	if (weather_channel != -1 && Mix_FadingChannel(weather_channel) != MIX_FADING_OUT) {
#ifndef WEATHER_VOL_PARTICLES
		weather_channel_volume = (CALC_MIX_VOLUME(cfg_audio_weather, cfg_audio_weather_volume) * grid_weather_volume) / 100;
		Mix_Volume(weather_channel, weather_channel_volume);
#else
		Mix_Volume(weather_channel, weather_channel_volume);
#endif
	}

	if (ambient_channel != -1 && Mix_FadingChannel(ambient_channel) != MIX_FADING_OUT) {
		ambient_channel_volume = (CALC_MIX_VOLUME(cfg_audio_sound, cfg_audio_sound_volume) * grid_ambient_volume) / 100;
		Mix_Volume(ambient_channel, ambient_channel_volume);
	}

#ifdef DISABLE_MUTED_AUDIO
	if (!cfg_audio_master || !cfg_audio_weather) {
		weather_resume = TRUE;
		if (weather_channel != -1 && Mix_Playing(weather_channel))
			Mix_HaltChannel(weather_channel);

		/* HACK - use weather volume for thunder sfx */
		for (n = 0; n < cfg_max_channels; n++)
			if (channel_type[n] == SFX_TYPE_WEATHER)
				Mix_HaltChannel(n);
	}

	if (!cfg_audio_master || !cfg_audio_sound) {
		ambient_resume = TRUE;
		if (ambient_channel != -1 && Mix_Playing(ambient_channel))
			Mix_HaltChannel(ambient_channel);
	}
#endif
}

/*
 * Init the SDL sound "module".
 */
errr init_sound_sdl(int argc, char **argv) {
	int i;

	/* Parse args */
	for (i = 1; i < argc; i++) {
		if (prefix(argv[i], "-a")) {
			no_cache_audio = TRUE;
			plog("Audio cache disabled.");
			continue;
		}
	}

#ifdef DEBUG_SOUND
	puts(format("init_sound_sdl() init%s", no_cache_audio == FALSE ? " (cached)" : " (not cached)"));
#endif

	/* Load sound preferences if requested */
#if 0
	if (!sound_sdl_init(no_cache_audio)) {
#else /* never cache audio right at program start, because it creates an annoying delay! */
	if (!sound_sdl_init(TRUE)) {
#endif
		plog("Failed to initialise audio.");

		/* Failure */
		return (1);
	}

	/* Set the mixing hook */
	mixing_hook = set_mixing_sdl;

	/* Enable sound */
	sound_hook = play_sound;

	/* Enable music */
	music_hook = play_music;

	/* Enable weather noise overlay */
	sound_weather_hook = play_sound_weather;
	sound_weather_hook_vol = play_sound_weather_vol;

	/* Enable ambient sfx overlay */
	sound_ambient_hook = play_sound_ambient;

	/* clean-up hook */
	atexit(close_audio);

	/* start caching audio in a separate thread to eliminate startup loading time */
	if (!no_cache_audio) {
#ifdef DEBUG_SOUND
		puts("Audio cache: Creating thread..");
#endif
		load_audio_thread = SDL_CreateThread(thread_load_audio, NULL);
		if (load_audio_thread == NULL) {
#ifdef DEBUG_SOUND
			puts("Audio cache: Thread creation failed.");
#endif
			plog(format("Audio cache: Unable to create thread: %s\n", SDL_GetError()));

			/* load manually instead, with annoying delay ;-p */
			thread_load_audio(NULL);
		}
#ifdef DEBUG_SOUND
		else puts("Audio cache: Thread creation succeeded.");
#endif
	}

#ifdef DEBUG_SOUND
	puts("init_sound_sdl() completed.");
#endif

	/* Success */
	return (0);
}

/* on game termination */
void mixer_fadeall(void) {
	Mix_FadeOutMusic(1500);
	Mix_FadeOutChannel(-1, 1500);
}

//extra code I moved here for USE_SOUND_2010, for porting
//this stuff from angband into here. it's part of angband's z-files.c..- C. Blue

//z-files.c:
static bool my_fexists(const char *fname) {
	FILE *fd;
	/* Try to open it */
	fd = fopen(fname, "rb");
	/* It worked */
	if (fd != NULL) {
		fclose(fd);
		return TRUE;
	} else return FALSE;
}

/* if audioCached is TRUE, load those audio files in a separate
   thread, to avoid startup delay of client - C. Blue */
static int thread_load_audio(void *dummy) {
	(void) dummy; /* suppress compiler warning */
	int idx, subidx;

	/* process all sound fx */
	for (idx = 0; idx < SOUND_MAX_2010; idx++) {
		/* process all files for each sound event */
		for (subidx = 0; subidx < samples[idx].num; subidx++) {
			load_sample(idx, subidx);
		}
	}

	/* process all music */
	for (idx = 0; idx < MUSIC_MAX; idx++) {
		/* process all files for each sound event */
		for (subidx = 0; subidx < songs[idx].num; subidx++) {
			load_song(idx, subidx);
		}
	}

	return(0);
}

/* thread-safe loading of audio files - C. Blue */
static Mix_Chunk* load_sample(int idx, int subidx) {
	const char *filename = samples[idx].paths[subidx];
	Mix_Chunk *wave = NULL;

	SDL_LockMutex(load_sample_mutex_entrance);

	SDL_LockMutex(load_sample_mutex);

	SDL_UnlockMutex(load_sample_mutex_entrance);

	/* paranoia: check if it's already loaded (but how could it..) */
	if (samples[idx].wavs[subidx]) {
#ifdef DEBUG_SOUND
		puts(format("sample already loaded %d, %d: %s.", idx, subidx, filename));
#endif
		SDL_UnlockMutex(load_sample_mutex);
		return (samples[idx].wavs[subidx]);
	}

	/* Try loading it, if it's not yet cached */

	/* Verify it exists */
	if (!my_fexists(filename)) {
#ifdef DEBUG_SOUND
		puts(format("file doesn't exist %d, %d: %s.", idx, subidx, filename));
#endif
		SDL_UnlockMutex(load_sample_mutex);
		return (NULL);
	}

	/* Load */
	wave = Mix_LoadWAV(filename);

	/* Did we get it now? */
	if (wave) {
		samples[idx].wavs[subidx] = wave;
#ifdef DEBUG_SOUND
		puts(format("loaded sample %d, %d: %s.", idx, subidx, filename));
#endif
	} else {
#ifdef DEBUG_SOUND
		puts(format("failed to load sample %d, %d: %s.", idx, subidx, filename));
#endif
		SDL_UnlockMutex(load_sample_mutex);
		return (NULL);
	}

	SDL_UnlockMutex(load_sample_mutex);
	return (wave);
}
static Mix_Music* load_song(int idx, int subidx) {
	const char *filename = songs[idx].paths[subidx];
	Mix_Music *waveMUS = NULL;

	SDL_LockMutex(load_song_mutex_entrance);

	SDL_LockMutex(load_song_mutex);

	SDL_UnlockMutex(load_song_mutex_entrance);

	/* check if it's already loaded */
	if (songs[idx].wavs[subidx]) {
#ifdef DEBUG_SOUND
		puts(format("song already loaded %d, %d: %s.", idx, subidx, filename));
#endif
		SDL_UnlockMutex(load_song_mutex);
		return (songs[idx].wavs[subidx]);
	}

	/* Try loading it, if it's not yet cached */

	/* Verify it exists */
	if (!my_fexists(filename)) {
#ifdef DEBUG_SOUND
		puts(format("file doesn't exist %d, %d: %s.", idx, subidx, filename));
#endif
		SDL_UnlockMutex(load_song_mutex);
		return (NULL);
	}

	/* Load */
	waveMUS = Mix_LoadMUS(filename);

	/* Did we get it now? */
	if (waveMUS) {
		songs[idx].wavs[subidx] = waveMUS;
#ifdef DEBUG_SOUND
		puts(format("loaded song %d, %d: %s.", idx, subidx, filename));
#endif
	} else {
#ifdef DEBUG_SOUND
		puts(format("failed to load song %d, %d: %s.", idx, subidx, filename));
#endif
		SDL_UnlockMutex(load_song_mutex);
		return (NULL);
	}

	SDL_UnlockMutex(load_song_mutex);
	return (waveMUS);
}

/* Display options page UI that allows to comment out sounds easily */
void do_cmd_options_sfx_sdl(void) {
	int i, i2, j, d, vertikal_offset = 3, horiz_offset = 5;
	int y = 0, j_sel = 0, tmp;
	char ch;
	byte a, a2;
	cptr lua_name;
	bool go = TRUE, dis;
	char buf[1024], buf2[1024], out_val[2048], out_val2[2048], *p, evname[2048];
	FILE *fff, *fff2;

	//ANGBAND_DIR_XTRA_SOUND/MUSIC are NULL in quiet_mode!
	if (quiet_mode) {
		c_msg_print("Client is running in quiet mode, sounds are not available.");
		return;
	}

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_XTRA_SOUND, "sound.cfg");

	/* Check if the file exists */
	fff = my_fopen(buf, "r");
	if (!fff) {
		c_msg_print("Error: File 'sound.cfg' not found.");
		return;
	}
	fclose(fff);

	/* Clear screen */
	Term_clear();

	/* Interact */
	while (go) {
		Term_putstr(0, 0, -1, TERM_WHITE, "  (<\377ydir\377w/\377y#\377w>, \377yt\377w (toggle), \377yy\377w/\377yn\377w (enable/disable), \377yRETURN\377w (play), \377yESC\377w)");
		Term_putstr(0, 1, -1, TERM_WHITE, "  (\377wAll changes made here will auto-save as soon as you leave this page)");

		/* Display the events */
		for (i = y - 10 ; i <= y + 10 ; i++) {
			if (i < 0 || i >= audio_sfx) {
				Term_putstr(horiz_offset + 7, vertikal_offset + i + 10 - y, -1, TERM_WHITE, "                                          ");
				continue;
			}

			/* Map events we've listed in our local config file onto audio.lua indices */
			i2 = -1;
			for (j = 0; j < SOUND_MAX_2010; j++) {
				if (!samples[j].config) continue;
				i2++;
				if (i2 == i) break;
			}
			if (j != SOUND_MAX_2010) { //paranoia, should always be false
				/* get event name */
				sprintf(out_val, "return get_sound_name(%d)", j);
				lua_name = string_exec_lua(0, out_val);
			} else lua_name = "<nothing>";
			if (i == y) j_sel = j;

			/* set colour depending on enabled/disabled state */
			//todo - c_cfg.use_color D: yadayada
			if (samples[j].disabled) {
				a = TERM_L_DARK;
				a2 = TERM_UMBER;
			} else {
				a = TERM_WHITE;
				a2 = TERM_YELLOW;
			}

			Term_putstr(horiz_offset + 7, vertikal_offset + i + 10 - y, -1, a2, format("%3d", i + 1));
			Term_putstr(horiz_offset + 12, vertikal_offset + i + 10 - y, -1, a, "                                ");
			Term_putstr(horiz_offset + 12, vertikal_offset + i + 10 - y, -1, a, (char*)lua_name);
			if (j == weather_current || j == ambient_current) {
				if (a != TERM_L_DARK) a = TERM_L_GREEN;
				Term_putstr(horiz_offset + 12, vertikal_offset + i + 10 - y, -1, a, format("%s    (playing)", (char*)lua_name));
			} else
				Term_putstr(horiz_offset + 12, vertikal_offset + i + 10 - y, -1, a, (char*)lua_name);
		}

		/* display static selector */
		Term_putstr(horiz_offset + 1, vertikal_offset + 10, -1, TERM_ORANGE, ">>>");
		Term_putstr(horiz_offset + 1 + 12 + 50 + 1, vertikal_offset + 10, -1, TERM_ORANGE, "<<<");

		/* Place Cursor */
		//Term_gotoxy(20, vertikal_offset + y);
		/* hack: hide cursor */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		/* Get key */
		ch = inkey();

		/* Analyze */
		switch (ch) {
		case ESCAPE:
			sound(j_sel, SFX_TYPE_STOP, 100, 0);

			/* auto-save */
			path_build(buf, 1024, ANGBAND_DIR_XTRA_SOUND, "sound.cfg");
#ifndef WINDOWS
			path_build(buf2, 1024, ANGBAND_DIR_XTRA_SOUND, "sound.$$$");
#else
			if (!win_dontmoveuser && getenv("HOMEDRIVE") && getenv("HOMEPATH")) {
				strcpy(buf2, getenv("HOMEDRIVE"));
				strcat(buf2, getenv("HOMEPATH"));
				strcat(buf2, "\\TomeNET-nosound.cfg");
			} else path_build(buf2, sizeof(path), ANGBAND_DIR_XTRA_SOUND, "TomeNET-nosound.cfg"); //paranoia
#endif
			fff = my_fopen(buf, "r");
			fff2 = my_fopen(buf2, "w");
			if (!fff) {
				c_msg_print("Error: File 'sound.cfg' not found.");
				return;
			}
			if (!fff2) {
				c_msg_print("Error: Cannot write to sound config file.");
				return;
			}
			while (TRUE) {
				if (!fgets(out_val, 2048, fff)) {
					if (ferror(fff)) {
						c_msg_print("Error: Failed to read from file 'sound.cfg'.");
					}
					break;
				}

				p = out_val;
				/* remove comment-character */
				if (*p == ';') p++;

				/* ignores lines that don't start on a letter */
				if (tolower(*p) < 'a' || tolower(*p) > 'z') {
#ifndef WINDOWS
					fputs(out_val, fff2);
#endif
					continue;
				}

				/* extract event name */
				strcpy(evname, p);
				*(strchr(evname, ' ')) = 0;

				/* find out event state (disabled/enabled) */
				j = exec_lua(0, format("return get_sound_index(\"%s\")", evname));
				if (j == -1 || !samples[j].config) {
					/* 'empty' event (no filenames specified), just copy it over same as misc lines */
#ifndef WINDOWS
					fputs(out_val, fff2);
#endif
					continue;
				}

				/* apply new state */
				if (samples[j].disabled) {
#ifndef WINDOWS
					strcpy(out_val2, ";");
					strcat(out_val2, p);
				} else strcpy(out_val2, p);
#else
					strcpy(out_val2, evname);
					strcat(out_val2, "\n");
				} else continue;
#endif

				fputs(out_val2, fff2);
			}
			fclose(fff);
			fclose(fff2);

#if 0
#if 0 /* cannot overwrite the cfg files in Programs (x86) folder on Windows 7 (+?) */
			rename(buf, format("%s.bak", buf));
			rename(buf2, buf);
#endif
#if 1 /* delete target file first instead of 'over-renaming'? Seems to work on my Win 7 box at least. */
			rename(buf, format("%s.bak", buf));
			//fd_kill(file_name);
			remove(buf);
			rename(buf2, buf);
#endif
#if 0 /* use a separate file instead? */
			path_build(buf, 1024, ANGBAND_DIR_XTRA_MUSIC, "sound-override.cfg");
			rename(buf2, buf);
#endif
#endif
#ifndef WINDOWS
			rename(buf, format("%s.bak", buf));
			//fd_kill(file_name);
			remove(buf);
			rename(buf2, buf);
#endif

			go = FALSE;
			break;

		case KTRL('T'):
			/* Take a screenshot */
			xhtml_screenshot("screenshot????");
			break;
		case ':':
			/* specialty: allow chatting from within here */
			cmd_message();
			break;

		case 't':
			samples[j_sel].disabled = !samples[j_sel].disabled;
			if (samples[j_sel].disabled) {
				if (j_sel == weather_current && weather_channel != -1 && Mix_Playing(weather_channel)) Mix_HaltChannel(weather_channel);
				if (j_sel == ambient_current && ambient_channel != -1 && Mix_Playing(ambient_channel)) Mix_HaltChannel(ambient_channel);
			} else {
				if (j_sel == weather_current) {
					weather_current = -1; //allow restarting it
					if (weather_current_vol != -1) play_sound_weather(j_sel);
					else play_sound_weather_vol(j_sel, weather_current_vol);
				}
				if (j_sel == ambient_current) {
					ambient_current = -1; //allow restarting it
					play_sound_ambient(j_sel);
				}
			}
			break;
		case 'y':
			samples[j_sel].disabled = FALSE;
			if (j_sel == weather_current) {
				weather_current = -1; //allow restarting it
				if (weather_current_vol != -1) play_sound_weather(j_sel);
				else play_sound_weather_vol(j_sel, weather_current_vol);
			}
			if (j_sel == ambient_current) {
				ambient_current = -1; //allow restarting it
				play_sound_ambient(j_sel);
			}
			break;
		case 'n':
			samples[j_sel].disabled = TRUE;
			if (j_sel == weather_current && weather_channel != -1 && Mix_Playing(weather_channel)) Mix_HaltChannel(weather_channel);
			if (j_sel == ambient_current && ambient_channel != -1 && Mix_Playing(ambient_channel)) Mix_HaltChannel(ambient_channel);
			break;

		case '\r':
			dis = samples[j_sel].disabled;
			samples[j_sel].disabled = FALSE;
			sound(j_sel, SFX_TYPE_MISC, 100, 0);
			samples[j_sel].disabled = dis;
			break;

		case '#':
			tmp = c_get_quantity("Enter index number: ", audio_sfx) - 1;
			if (!tmp) break;
			sound(j_sel, SFX_TYPE_STOP, 100, 0);
			y = tmp;
			if (y < 0) y = 0;
			if (y >= audio_sfx) y = audio_sfx - 1;
			break;
		case '9':
			sound(j_sel, SFX_TYPE_STOP, 100, 0);
			y = (y - 10 + audio_sfx) % audio_sfx;
			break;
		case '3':
			sound(j_sel, SFX_TYPE_STOP, 100, 0);
			y = (y + 10 + audio_sfx) % audio_sfx;
			break;
		case '1':
			sound(j_sel, SFX_TYPE_STOP, 100, 0);
			y = audio_sfx - 1;
			break;
		case '7':
			sound(j_sel, SFX_TYPE_STOP, 100, 0);
			y = 0;
			break;
		case '8':
		case '2':
			sound(j_sel, SFX_TYPE_STOP, 100, 0);
			d = keymap_dirs[ch & 0x7F];
			y = (y + ddy[d] + audio_sfx) % audio_sfx;
			break;
		default:
			bell();
		}
	}
}

/* Display options page UI that allows to comment out music easily */
void do_cmd_options_mus_sdl(void) {
	int i, i2, j, d, vertikal_offset = 3, horiz_offset = 5;
	int y = 0, j_sel = 0;//, max_events = 0;
	char ch;
	byte a, a2;
	cptr lua_name;
	bool go = TRUE;
	char buf[1024], buf2[1024], out_val[2048], out_val2[2048], *p, evname[2048];
	FILE *fff, *fff2;

	//ANGBAND_DIR_XTRA_SOUND/MUSIC are NULL in quiet_mode!
	if (quiet_mode) {
		c_msg_print("Client is running in quiet mode, music is not available.");
		return;
	}

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_XTRA_MUSIC, "music.cfg");

	/* Check if the file exists */
	fff = my_fopen(buf, "r");
	if (!fff) {
		c_msg_print("Error: File 'music.cfg' not found.");
		return;
	}
	fclose(fff);

	/* Clear screen */
	Term_clear();

	/* Interact */
	while (go) {
		Term_putstr(0, 0, -1, TERM_WHITE, "  (<\377ydir\377w/\377y#\377w>, \377yt\377w (toggle), \377yy\377w/\377yn\377w (enable/disable), \377yESC\377w)");
		Term_putstr(0, 1, -1, TERM_WHITE, "  (\377wAll changes made here will auto-save as soon as you leave this page)");

		/* Display the events */
		for (i = y - 10 ; i <= y + 10 ; i++) {
			if (i < 0 || i >= audio_music) {
				Term_putstr(horiz_offset + 7, vertikal_offset + i + 10 - y, -1, TERM_WHITE, "                                          ");
				continue;
			}

			/* Map events we've listed in our local config file onto audio.lua indices */
			i2 = -1;
			for (j = 0; j < MUSIC_MAX; j++) {
				if (!songs[j].config) continue;
				i2++;
				if (i2 == i) break;
			}
			if (j != MUSIC_MAX) { //paranoia, should always be false
				/* get event name */
				sprintf(out_val, "return get_music_name(%d)", j);
				lua_name = string_exec_lua(0, out_val);
			} else lua_name = "<nothing>";
			if (i == y) j_sel = j;

			/* set colour depending on enabled/disabled state */
			//todo - c_cfg.use_color D: yadayada
			if (songs[j].disabled) {
				a = TERM_L_DARK;
				a2 = TERM_UMBER;
			} else {
				a = TERM_WHITE;
				a2 = TERM_YELLOW;
			}

			Term_putstr(horiz_offset + 7, vertikal_offset + i + 10 - y, -1, a2, format("%3d", i + 1));
			Term_putstr(horiz_offset + 12, vertikal_offset + i + 10 - y, -1, a, "                                ");
			if (j == music_cur) {
				if (a != TERM_L_DARK) a = TERM_L_GREEN;
				Term_putstr(horiz_offset + 12, vertikal_offset + i + 10 - y, -1, a, format("%s    (playing)", (char*)lua_name));
			} else
				Term_putstr(horiz_offset + 12, vertikal_offset + i + 10 - y, -1, a, (char*)lua_name);
		}

		/* display static selector */
		Term_putstr(horiz_offset + 1, vertikal_offset + 10, -1, TERM_ORANGE, ">>>");
		Term_putstr(horiz_offset + 1 + 12 + 50 + 1, vertikal_offset + 10, -1, TERM_ORANGE, "<<<");

		/* Place Cursor */
		//Term_gotoxy(20, vertikal_offset + y);
		/* hack: hide cursor */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		/* Get key */
		ch = inkey();

		/* Analyze */
		switch (ch) {
		case ESCAPE:
			/* auto-save */
			path_build(buf, 1024, ANGBAND_DIR_XTRA_MUSIC, "music.cfg");
#ifndef WINDOWS
			path_build(buf2, 1024, ANGBAND_DIR_XTRA_MUSIC, "music.$$$");
#else
			if (!win_dontmoveuser && getenv("HOMEDRIVE") && getenv("HOMEPATH")) {
				strcpy(buf2, getenv("HOMEDRIVE"));
				strcat(buf2, getenv("HOMEPATH"));
				strcat(buf2, "\\TomeNET-nomusic.cfg");
			} else path_build(buf2, sizeof(path), ANGBAND_DIR_XTRA_MUSIC, "TomeNET-nomusic.cfg"); //paranoia
#endif
			fff = my_fopen(buf, "r");
			fff2 = my_fopen(buf2, "w");
			if (!fff) {
				c_msg_print("Error: File 'music.cfg' not found.");
				return;
			}
			if (!fff2) {
				c_msg_print("Error: Cannot write to music config file.");
				return;
			}
			while (TRUE) {
				if (!fgets(out_val, 2048, fff)) {
					if (ferror(fff)) {
						c_msg_print("Error: Failed to read from file 'music.cfg'.");
					}
					break;
				}

				p = out_val;
				/* remove comment-character */
				if (*p == ';') p++;

				/* ignores lines that don't start on a letter */
				if (tolower(*p) < 'a' || tolower(*p) > 'z') {
#ifndef WINDOWS
					fputs(out_val, fff2);
#endif
					continue;
				}

				/* extract event name */
				strcpy(evname, p);
				*(strchr(evname, ' ')) = 0;

				/* find out event state (disabled/enabled) */
				j = exec_lua(0, format("return get_music_index(\"%s\")", evname));
				if (j == -1 || !songs[j].config) {
					/* 'empty' event (no filenames specified), just copy it over same as misc lines */
#ifndef WINDOWS
					fputs(out_val, fff2);
#endif
					continue;
				}

				/* apply new state */
				if (songs[j].disabled) {
#ifndef WINDOWS
					strcpy(out_val2, ";");
					strcat(out_val2, p);
				} else strcpy(out_val2, p);
#else
					strcpy(out_val2, evname);
					strcat(out_val2, "\n");
				} else continue;
#endif

				fputs(out_val2, fff2);
			}
			fclose(fff);
			fclose(fff2);

#if 0
#if 0 /* cannot overwrite the cfg files in Programs (x86) folder on Windows 7 (+?) */
			rename(buf, format("%s.bak", buf));
			rename(buf2, buf);
#endif
#if 1 /* delete target file first instead of 'over-renaming'? Seems to work on my Win 7 box at least. */
			rename(buf, format("%s.bak", buf));
			//fd_kill(file_name);
			remove(buf);
			rename(buf2, buf);
#endif
#if 0 /* use a separate file instead? */
			path_build(buf, 1024, ANGBAND_DIR_XTRA_MUSIC, "music-override.cfg");
			rename(buf2, buf);
#endif
#endif
#ifndef WINDOWS
			rename(buf, format("%s.bak", buf));
			//fd_kill(file_name);
			remove(buf);
			rename(buf2, buf);
#endif

			go = FALSE;
			break;

		case KTRL('T'):
			/* Take a screenshot */
			xhtml_screenshot("screenshot????");
			break;
		case ':':
			/* specialty: allow chatting from within here */
			cmd_message();
			break;

		case 't':
			songs[j_sel].disabled = !songs[j_sel].disabled;
			if (songs[j_sel].disabled) {
				if (music_cur == j_sel && Mix_PlayingMusic()) Mix_HaltMusic();
			} else {
				if (music_cur == j_sel) {
					music_cur = -1; //allow restarting it
					play_music(j_sel);
				}
			}
			break;
		case 'y':
			songs[j_sel].disabled = FALSE;
			if (music_cur == j_sel) {
				music_cur = -1; //allow restarting it
				play_music(j_sel);
			}
			break;
		case 'n':
			songs[j_sel].disabled = TRUE;
			if (music_cur == j_sel && Mix_PlayingMusic()) Mix_HaltMusic();
			break;

		case '#':
			y = c_get_quantity("Enter index number: ", audio_music) - 1;
			if (y < 0) y = 0;
			if (y >= audio_music) y = audio_music - 1;
			break;
		case '9':
			y = (y - 10 + audio_music) % audio_music;
			break;
		case '3':
			y = (y + 10 + audio_music) % audio_music;
			break;
		case '1':
			y = audio_music - 1;
			break;
		case '7':
			y = 0;
			break;
		case '8':
		case '2':
			d = keymap_dirs[ch & 0x7F];
			y = (y + ddy[d] + audio_music) % audio_music;
			break;
		default:
			bell();
		}
	}
}

#endif /* SOUND_SDL */
