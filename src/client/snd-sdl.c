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


/* declare */
static void fadein_next_music(void);


/* Arbitary limit on number of samples per event */
#define MAX_SAMPLES	8

/* Arbitary limit on number of music songs per situation */
#define MAX_SONGS	3

/* Struct representing all data about an event sample */
typedef struct {
	int num;                        /* Number of samples for this event */
	Mix_Chunk *wavs[MAX_SAMPLES];   /* Sample array */
	const char *paths[MAX_SAMPLES]; /* Relative pathnames for samples */
} sample_list;

/* background music */
typedef struct {
	int num;
	Mix_Music *wavs[MAX_SONGS];
	const char *paths[MAX_SONGS];
} song_list;

/* Just need an array of SampInfos */
static sample_list samples[SOUND_MAX_2010];

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
	audio_rate = 44100;
//	audio_rate = 22050;//angband default
	audio_format = AUDIO_S16;
	audio_channels = 2;

	/* Initialize the SDL library */
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		plog_fmt("Couldn't initialize SDL: %s", SDL_GetError());
		puts(format("Couldn't initialize SDL: %s", SDL_GetError()));//DEBUG USE_SOUND_2010
		return FALSE;
	}

	/* Try to open the audio */
	if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, 4096) < 0) {
		plog_fmt("Couldn't open mixer: %s", SDL_GetError());
		puts(format("Couldn't open mixer: %s", SDL_GetError()));//DEBUG USE_SOUND_2010
		return FALSE;
	}

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

	/* Initialise the mixer  */
	if (!open_audio()) return FALSE;

	puts("USE_SOUND_2010: sound_sdl_init() opened audio");//debug

	/* set hook for fading over to next song */
	Mix_HookMusicFinished(fadein_next_music);


	/* ------------------------------- Init Sounds */


	/* Build the "sound" path */
	path_build(path, sizeof(path), ANGBAND_DIR_XTRA, "sound");
	ANGBAND_DIR_XTRA_SOUND = string_make(path);
	if (!my_dexists(ANGBAND_DIR_XTRA_SOUND)) {
		plog_fmt("Folder %s is missing.", ANGBAND_DIR_XTRA_SOUND);
		return FALSE;
	}

	/* Find and open the config file */
	path_build(path, sizeof(path), ANGBAND_DIR_XTRA_SOUND, "sound.cfg");
	fff = my_fopen(path, "r");

	/* Handle errors */
	if (!fff) {
		plog_fmt("Failed to open sound config (%s):\n    %s", path, strerror(errno));
		return FALSE;
	}

	puts("USE_SOUND_2010: sound_sdl_init() reading sound.cfg:");//debug

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
					puts(format("%s: %s", SDL_GetError(), strerror(errno)));//DEBUG USE_SOUND_2010
					goto next_token_snd;
				}
			}
//puts(format("loaded sample %s (ev %d, #%d).", samples[event].paths[num], event, num));
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
	ANGBAND_DIR_XTRA_SOUND = string_make(path);
	if (!my_dexists(ANGBAND_DIR_XTRA_SOUND)) {
		plog_fmt("Folder %s is missing.", ANGBAND_DIR_XTRA_SOUND);
		return FALSE;
	}

	/* Find and open the config file */
	path_build(path, sizeof(path), ANGBAND_DIR_XTRA_SOUND, "music.cfg");
	fff = my_fopen(path, "r");

	/* Handle errors */
	if (!fff) {
		plog_fmt("Failed to open music config (%s):\n    %s", path, strerror(errno));
		return FALSE;
	}

	puts("USE_SOUND_2010: sound_sdl_init() reading music.cfg:");//debug

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
			path_build(path, sizeof(path), ANGBAND_DIR_XTRA_SOUND, cur_token);
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

//puts(format("loaded song %s (ev %d, #%d).", songs[event].paths[num], event, num));
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

	puts("USE_SOUND_2010: sound_sdl_init() done.");//debug

	/* Success */
	return TRUE;
}

/*
 * Play a sound of type "event".
 */
static void play_sound(int event) {
	Mix_Chunk *wave = NULL;
	int s;

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
	Mix_PlayChannel(-1, wave, 0);
}


/*
 * Overlay a weather noise
 */
static void play_sound_weather(int event) {
	Mix_Chunk *wave = NULL;
	int s;

	if (event == -2 && weather_channel != -1) {
		Mix_HaltChannel(weather_channel);
		weather_channel = -1;
		return;
	}
	else if (event == -1 && weather_channel != -1) {
		Mix_FadeOutChannel(weather_channel, 2000);
		weather_channel = -1;
		return;
	}

	/* we're already in this weather? */
	if (weather_channel != -1 && weather_current == event) return;

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
//	weather_channel = Mix_PlayChannel(weather_channel, wave, -1);
	weather_channel = Mix_FadeInChannel(weather_channel, wave, -1, 500);
	if (weather_channel != -1) { //paranoia?
		weather_current = event;
		Mix_Volume(weather_channel, (MIX_MAX_VOLUME * (cfg_audio_weather ? cfg_audio_weather_volume : 0)) / 100);
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
 * Init the SDL sound "module".
 */
errr init_sound_sdl(int argc, char **argv) {
	int i;

	/* Parse args */
	for (i = 1; i < argc; i++) {
		if (prefix(argv[i], "-c")) {
			no_cache_audio = TRUE;
			plog("Audio cache disabled.");
puts("\n");//plog seems to mess up display? -- just for following debug puts() for USE_SOUND_2010 - C. Blue
			continue;
		}
	}

#if 1 //just debugging - C. Blue
	puts(format("USE_SOUND_2010: init_sound_sdl() initializing..(no_cache_audio = %d)", no_cache_audio));
#endif

	/* Load sound preferences if requested */
	if (!sound_sdl_init(no_cache_audio)) {
		plog("Failed to load audio config");

		/* Failure */
		return (1);
	}

	/* Enable sound */
	sound_hook = play_sound;

	/* Enable music */
	music_hook = play_music;

	/* Enable weather noise overlay */
	sound_weather_hook = play_sound_weather;

	/* clean-up hook */
	atexit(close_audio);

#if 1 //just debugging - C. Blue
	puts("USE_SOUND_2010: init_sound_sdl() completed.");
#endif

	/* Success */
	return (0);
}

/* when quitting the game maybe */
//void sdl_fadeout(void) { Mix_FadeOutChannel(-1, 500); }

//extra code I moved here for USE_SOUND_2010, for porting
//this stuff from angband into here. it's part of angband's z-files.c..- C. Blue

//z-files.c:
bool my_fexists(const char *fname) {
	FILE *fd;
	/* Try to open it */
	fd = fopen(fname, "r");
	/* It worked */
	if (fd != NULL) {
		fclose(fd);
		return TRUE;
	} else return FALSE;
}

//check if folder exists on UNIX, using dirent.h - C. Blue
#include <dirent.h>
bool my_dexists(const char *dname) {
	DIR *pdir = opendir(dname);
	if (pdir != NULL) {
		closedir(pdir);
		return TRUE;
	} else return FALSE;
}

void set_mixing(void) {
//	puts(format("mixer set to %d, %d, %d.", cfg_audio_music_volume, cfg_audio_sound_volume, cfg_audio_weather_volume));
	Mix_Volume(-1, (MIX_MAX_VOLUME * (cfg_audio_master ? (cfg_audio_sound ? cfg_audio_sound_volume : 0) : 0) * cfg_audio_master_volume) / 10000);
	Mix_VolumeMusic((MIX_MAX_VOLUME * (cfg_audio_master ? (cfg_audio_music ? cfg_audio_music_volume : 0) : 0) * cfg_audio_master_volume) / 10000);
	if (weather_channel != -1) Mix_Volume(weather_channel, (MIX_MAX_VOLUME * (cfg_audio_master ? (cfg_audio_weather ? cfg_audio_weather_volume : 0) : 0) * cfg_audio_master_volume) / 10000);
}

#endif /* SOUND_SDL */
