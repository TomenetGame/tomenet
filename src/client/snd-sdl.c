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


#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>


/* Don't cache audio */
static bool no_cache_audio = FALSE;

/* Path to sound files */
static const char *ANGBAND_DIR_XTRA_SOUND;
static const char *ANGBAND_DIR_XTRA_MUSIC;


/* declare */
static void fadein_next_music(void);
static void clear_channel(int c);
static bool my_fexists(const char *fname);


/* Arbitrary limit of mixer channels */
#define MAX_CHANNELS	16

/* Mixed sample size (larger = more lagging sound, smaller = skipping on slow machines) */
#define CHUNK_SIZE	1024

/* Arbitary limit on number of samples per event */
#define MAX_SAMPLES	8

/* Arbitary limit on number of music songs per situation */
#define MAX_SONGS	3

#if 1 /* use linear volume scale -- poly+50 would be best, but already causes mixer outtages if total volume becomes too small (ie < 1).. =_= */
 #define CALC_MIX_VOLUME(T, V)	((MIX_MAX_VOLUME * (cfg_audio_master ? ((T) ? (V) : 0) : 0) * cfg_audio_master_volume) / 10000)
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
					   stacking of the same sound multiple (read: too many) times - C. Blue */
} sample_list;

/* background music */
typedef struct {
	int num;
	Mix_Music *wavs[MAX_SONGS];
	const char *paths[MAX_SONGS];
} song_list;

/* Just need an array of SampInfos */
static sample_list samples[SOUND_MAX_2010];

/* Array of potential channels, for managing that only one
   sound of a kind is played simultaneously, for efficiency - C. Blue */
static int channel_sample[MAX_CHANNELS];

/* Music Array */
static song_list songs[MUSIC_MAX];


/*
 * Shut down the sound system and free resources.
 */
static void close_audio(void) {
	size_t i;
	int j;

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

	string_free(ANGBAND_DIR_XTRA_SOUND);
	string_free(ANGBAND_DIR_XTRA_MUSIC);

	/* Close the audio */
	Mix_CloseAudio();

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
	audio_rate = cfg_audio_rate;
	audio_format = AUDIO_S16;
	audio_channels = 2;

	/* Initialize the SDL library */
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
#ifdef DEBUG_SOUND
		plog_fmt("Couldn't initialize SDL: %s", SDL_GetError());
		puts(format("Couldn't initialize SDL: %s", SDL_GetError()));
#endif
		return FALSE;
	}

	/* Try to open the audio */
	if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, CHUNK_SIZE) < 0) {
#ifdef DEBUG_SOUND
		plog_fmt("Couldn't open mixer: %s", SDL_GetError());
		puts(format("Couldn't open mixer: %s", SDL_GetError()));
#endif
		return FALSE;
	}

	Mix_AllocateChannels(MAX_CHANNELS);
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
	char buffer[2048];
	FILE *fff;
	int i;

	/* Initialise the mixer  */
	if (!open_audio()) return FALSE;

#ifdef DEBUG_SOUND
	puts(format("sound_sdl_init() opened at %d Hz.", cfg_audio_rate));
#endif

	/* Initialize sound-fx channel management */
	for (i = 0; i < MAX_CHANNELS; i++) channel_sample[i] = -1;


	/* ------------------------------- Init Sounds */

	/* Build the "sound" path */
	path_build(path, sizeof(path), ANGBAND_DIR_XTRA, "sound");
	ANGBAND_DIR_XTRA_SOUND = string_make(path);

	/* Find and open the config file */
	path_build(path, sizeof(path), ANGBAND_DIR_XTRA_SOUND, "sound.cfg");
	fff = my_fopen(path, "r");

	/* Handle errors */
	if (!fff) {
		plog_fmt("Failed to open sound config (%s):\n    %s", path, strerror(errno));
		return FALSE;
	}

#ifdef DEBUG_SOUND
	puts("sound_sdl_init() reading sound.cfg:");
#endif

	/* Parse the file */
	/* Lines are always of the form "name = sample [sample ...]" */
	while (my_fgets(fff, buffer, sizeof(buffer)) == 0) {
		char *cfg_name;
		cptr lua_name;
		char *sample_list;
		char *search;
		char *cur_token;
		char *next_token;
		int event;

		/* Skip anything not beginning with an alphabetic character */
		if (!buffer[0] || !isalpha((unsigned char)buffer[0])) continue;

		/* Split the line into two: message name, and the rest */
		search = strchr(buffer, ' ');
		sample_list = strchr(search + 1, ' ');
		if (!search) continue;
		if (!sample_list) continue;

		/* Set the message name, and terminate at first space */
		cfg_name = buffer;
		search[0] = '\0';

		/* Make sure this is a valid event name */
		for (event = SOUND_MAX_2010 - 1; event >= 0; event--) {
			lua_name = string_exec_lua(0, format("return get_sound_name(%d)", event));
			if (!strlen(lua_name)) continue;
			if (strcmp(cfg_name, lua_name) == 0) break;
		}
		if (event < 0) continue;

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
		while (cur_token) {
			int num = samples[event].num;

			/* Don't allow too many samples */
			if (num >= MAX_SAMPLES) break;

			/* Build the path to the sample */
			path_build(path, sizeof(path), ANGBAND_DIR_XTRA_SOUND, cur_token);
			if (!my_fexists(path)) goto next_token_snd;

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
	}

	/* Close the file */
	my_fclose(fff);


	/* ------------------------------- Init Music */

	/* Build the "music" path */
	path_build(path, sizeof(path), ANGBAND_DIR_XTRA, "music");
	ANGBAND_DIR_XTRA_MUSIC = string_make(path);

	/* Find and open the config file */
	path_build(path, sizeof(path), ANGBAND_DIR_XTRA_MUSIC, "music.cfg");
	fff = my_fopen(path, "r");

	/* Handle errors */
	if (!fff) {
		plog_fmt("Failed to open music config (%s):\n    %s", path, strerror(errno));
		return FALSE;
	}

#ifdef DEBUG_SOUND
	puts("sound_sdl_init() reading music.cfg:");
#endif

	/* Parse the file */
	/* Lines are always of the form "name = music [music ...]" */
	while (my_fgets(fff, buffer, sizeof(buffer)) == 0) {
		char *cfg_name;
		cptr lua_name;
		char *song_list;
		char *search;
		char *cur_token;
		char *next_token;
		int event;

		/* Skip anything not beginning with an alphabetic character */
		if (!buffer[0] || !isalpha((unsigned char)buffer[0])) continue;

		/* Split the line into two: message name, and the rest */
		search = strchr(buffer, ' ');
		song_list = strchr(search + 1, ' ');
		if (!search) continue;
		if (!song_list) continue;

		/* Set the message name, and terminate at first space */
		cfg_name = buffer;
		search[0] = '\0';

		/* Make sure this is a valid event name */
		for (event = MUSIC_MAX - 1; event >= 0; event--) {
			lua_name = string_exec_lua(0, format("return get_music_name(%d)", event));
			if (!strlen(lua_name)) continue;
			if (strcmp(cfg_name, lua_name) == 0) break;
		}
		if (event < 0) continue;

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
		while (cur_token) {
			int num = songs[event].num;

			/* Don't allow too many songs */
			if (num >= MAX_SONGS) break;

			/* Build the path to the sample */
			path_build(path, sizeof(path), ANGBAND_DIR_XTRA_MUSIC, cur_token);
			if (!my_fexists(path)) goto next_token_mus;

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
	}

	/* Close the file */
	my_fclose(fff);

#ifdef DEBUG_SOUND
	puts("sound_sdl_init() done.");
#endif

	/* Success */
	return TRUE;
}

/*
 * Play a sound of type "event". Returns FALSE if sound couldn't be played.
 */
static bool play_sound(int event) {
	Mix_Chunk *wave = NULL;
	int s;

	/* Paranoia */
	if (event < 0 || event >= SOUND_MAX_2010) return FALSE;

	/* Check there are samples for this event */
	if (!samples[event].num) return FALSE;

	/* already playing? prevent multiple sounds of the same kind from being mixed simultaneously, for preventing silliness */
	if (samples[event].current_channel != -1) return TRUE;

	/* Choose a random event */
	s = rand_int(samples[event].num);
	wave = samples[event].wavs[s];

	/* Try loading it, if it's not cached */
	if (!wave) {
		/* Verify it exists */
		const char *filename = samples[event].paths[s];
		if (!my_fexists(filename)) return FALSE;

		/* Load */
		wave = Mix_LoadWAV(filename);
	}

	/* Check to see if we have a wave again */
	if (!wave) {
		plog("SDL sound load failed.");
		return FALSE;
	}

	/* Actually play the thing */
	/* remember, for efficiency management */
	s = Mix_PlayChannel(-1, wave, 0);
	if (s != -1) {
		channel_sample[s] = event;
//		Mix_Volume(s, CALC_MIX_VOLUME(cfg_audio_sound, cfg_audio_sound_volume));
	}
	samples[event].current_channel = s;
	
	return TRUE;
}
/* play the 'page' sound */
extern bool sound_page(void) {
	Mix_Chunk *wave = NULL;
	int s;

	if (page_sound_idx == -1) return FALSE;
	if (!samples[page_sound_idx].num) return FALSE;

	/* already playing? prevent multiple sounds of the same kind from being mixed simultaneously, for preventing silliness */
	if (samples[page_sound_idx].current_channel != -1) return TRUE;

	/* Choose a random event */
	s = rand_int(samples[page_sound_idx].num);
	wave = samples[page_sound_idx].wavs[s];

	/* Try loading it, if it's not cached */
	if (!wave) {
		/* Verify it exists */
		const char *filename = samples[page_sound_idx].paths[s];
		if (!my_fexists(filename)) return FALSE;

		/* Load */
		wave = Mix_LoadWAV(filename);
	}

	/* Check to see if we have a wave again */
	if (!wave) {
		plog("SDL sound load failed.");
		return FALSE;
	}

	/* Actually play the thing */
	/* remember, for efficiency management */
	s = Mix_PlayChannel(-1, wave, 0);
	if (s != -1) {
		channel_sample[s] = page_sound_idx;
		if (c_cfg.paging_max_volume) {
			Mix_Volume(s, MIX_MAX_VOLUME);
		} else if (c_cfg.paging_master_volume) {
			Mix_Volume(s, CALC_MIX_VOLUME(1, 100));
		}
	}
	samples[page_sound_idx].current_channel = s;
	
	return TRUE;
}


/* Release weather channel after fading out has been completed */
static void clear_channel(int c) {
	/* weather has faded out, mark it as gone */
	if (c == weather_channel) {
		weather_channel = -1;
		return;
	}

	/* a sample has finished playing, so allow this kind to be played again */
	/* hack: if the sample was the 'paging' sound, reset the channel's volume to be on the safe side */
	if (channel_sample[c] == page_sound_idx) {
		Mix_Volume(c, CALC_MIX_VOLUME(cfg_audio_sound, cfg_audio_sound_volume));
	}

	samples[channel_sample[c]].current_channel = -1;
	channel_sample[c] = -1;
}

/* Overlay a weather noise */
static void play_sound_weather(int event) {
	Mix_Chunk *wave = NULL;
	int s, new_wc;

	if (event == -2 && weather_channel != -1) {
#ifdef DEBUG_SOUND
		puts(format("w-2: wco %d, ev %d", weather_channel, event));
#endif
		Mix_HaltChannel(weather_channel);
		return;
	}
	else if (event == -1 && weather_channel != -1) {
#ifdef DEBUG_SOUND
		puts(format("w-1: wco %d, ev %d", weather_channel, event));
#endif
		if (Mix_FadingChannel(weather_channel) != MIX_FADING_OUT)
			Mix_FadeOutChannel(weather_channel, 2000);
		return;
	}

	/* we're already in this weather? */
	if (weather_channel != -1 && weather_current == event &&
	    Mix_FadingChannel(weather_channel) != MIX_FADING_OUT)
		return;

	/* Paranoia */
	if (event < 0 || event >= SOUND_MAX_2010) return;

	/* Check there are samples for this event */
	if (!samples[event].num) return;

	/* Choose a random event */
	s = rand_int(samples[event].num);
	wave = samples[event].wavs[s];

	/* Try loading it, if it's not cached */
	if (!wave) {
		/* Verify it exists */
		const char *filename = samples[event].paths[s];
		if (!my_fexists(filename)) return;

		/* Load */
		wave = Mix_LoadWAV(filename);
	}

	/* Check to see if we have a wave again */
	if (!wave) {
		plog("SDL sound load failed.");
		return;
	}

	/* Actually play the thing */
#if 1 /* volume glitch paranoia (first fade-in seems to move volume to 100% instead of designated cfg_audio_... */
	new_wc = Mix_PlayChannel(weather_channel, wave, -1);
	if (new_wc != -1) {
		Mix_Volume(new_wc, CALC_MIX_VOLUME(cfg_audio_weather, cfg_audio_weather_volume));
		new_wc = Mix_FadeInChannel(new_wc, wave, -1, 500);
	}
#else
	new_wc = Mix_FadeInChannel(weather_channel, wave, -1, 500);
#endif
	weather_fading = 1;
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
		Mix_Volume(weather_channel, CALC_MIX_VOLUME(cfg_audio_weather, cfg_audio_weather_volume));
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
			//different channel?
			} else {
				Mix_HaltChannel(weather_channel);
				weather_channel = new_wc;
				weather_current = event;
				Mix_Volume(weather_channel, CALC_MIX_VOLUME(cfg_audio_weather, cfg_audio_weather_volume));
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
		Mix_Volume(weather_channel, CALC_MIX_VOLUME(cfg_audio_weather, cfg_audio_weather_volume));
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
		Mix_Volume(weather_channel, CALC_MIX_VOLUME(cfg_audio_weather, cfg_audio_weather_volume));
		weather_fading = 0;
		return;
	}
}


/*
 * Play a music of type "event".
 */
static void play_music(int event) {
	music_next = event;

	/* check if music is already running, if so, fade it out first! */
	if (Mix_PlayingMusic()) {
		if (Mix_FadingMusic() != MIX_FADING_OUT) Mix_FadeOutMusic(500);
		return;
	} else {
		//play immediately
		fadein_next_music();
	}
}

static void fadein_next_music(void) {
	Mix_Music *wave = NULL;
	int s;

	/* Paranoia */
	if (music_next < 0 || music_next >= MUSIC_MAX) return;

	/* Check there are samples for this event */
	if (!songs[music_next].num) return;

	/* Choose a random event */
	s = rand_int(songs[music_next].num);
	wave = songs[music_next].wavs[s];

	/* Try loading it, if it's not cached */
	if (!wave) {
		/* Verify it exists */
		const char *filename = songs[music_next].paths[s];
		if (!my_fexists(filename)) return;

		/* Load */
		wave = Mix_LoadMUS(filename);
	}

	/* Check to see if we have a wave again */
	if (!wave) {
		plog("SDL music load failed.");
		return;
	}

	/* Actually play the thing */
	music_next = -1;
//	Mix_PlayMusic(wave, -1);//-1 infinite, 0 once, or n times
	Mix_FadeInMusic(wave, -1, 1000);
}

/*
 * Set mixing levels in the SDL module.
 */
static void set_mixing_sdl(void) {
//	puts(format("mixer set to %d, %d, %d.", cfg_audio_music_volume, cfg_audio_sound_volume, cfg_audio_weather_volume));
	Mix_Volume(-1, CALC_MIX_VOLUME(cfg_audio_sound, cfg_audio_sound_volume));
	Mix_VolumeMusic(CALC_MIX_VOLUME(cfg_audio_music, cfg_audio_music_volume));
	if (weather_channel != -1 && Mix_FadingChannel(weather_channel) != MIX_FADING_OUT)
		Mix_Volume(weather_channel, CALC_MIX_VOLUME(cfg_audio_weather, cfg_audio_weather_volume));
}

/*
 * Init the SDL sound "module".
 */
errr init_sound_sdl(int argc, char **argv) {
	int i;

	/* Parse args */
	for (i = 1; i < argc; i++) {
		if (prefix(argv[i], "-c")) {
			no_cache_audio = TRUE;
			plog("Audio cache disabled.");
//puts("\n");//plog seems to mess up display? -- just for following debug puts() for USE_SOUND_2010 - C. Blue
			continue;
		}
	}

#ifdef DEBUG_SOUND
	puts(format("init_sound_sdl() init%s", no_cache_audio == 0 ? "" : " (cached)"));
#endif

	/* Load sound preferences if requested */
	if (!sound_sdl_init(no_cache_audio)) {
		plog("Failed to load audio config");

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

	/* clean-up hook */
	atexit(close_audio);

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
	fd = fopen(fname, "r");
	/* It worked */
	if (fd != NULL) {
		fclose(fd);
		return TRUE;
	} else return FALSE;
}

#endif /* SOUND_SDL */
