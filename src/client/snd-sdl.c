/*
 * File: snd-sdl.c
 * Purpose: SDL sound support for TomeNET
 *
 * Written in 2010 by C. Blue, inspired by old angband sdl code.
 * New custom sound structure, ambient sound, background music and weather.
 * New in 2022, I added positional audio. Might migrate to OpenAL in the
 * future to support HRTF for z-plane spatial audio. - C. Blue
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


#include "angband.h"

/* Summer 2022 I migrated from SDL to SDL2, requiring SDL2_mixer v2.5 or higher
   for Mix_SetPanning() and Mix_GetMusicPosition(). - C. Blue */
#ifdef SOUND_SDL

/* Enable ALMixer, overriding SDL? */
//#define SOUND_AL_SDL

/* allow pressing RETURN key to play any piece of music (like for sfx) -- only reason to disable: It's spoilery ;) */
#define ENABLE_JUKEBOX
/* if a song is selected, don't fadeout current song first, but halt it instantly. this is needed for playing disabled songs.
   Note that enabling this will also allow to iteratively play through all sub-songs of a specific music event, by activating
   the same song over and over, while otherwise if this option is disabled you'd have to wait for it to end and randomly switch
   to other sub-songs.*/
#ifdef ENABLE_JUKEBOX
 #define JUKEBOX_INSTANT_PLAY
#endif

/* Show filename of currently selected song even if it's not actively playing? */
#define JUKEBOX_SELECTED_FILENAME

/* Smooth dynamic weather volume change depending on amount of particles,
   instead of just on/off depending on particle count window? */
//#define WEATHER_VOL_PARTICLES  -- not yet that cool

/* Maybe this will be cool: Dynamic weather volume calculated from distances to registered clouds */
#define WEATHER_VOL_CLOUDS

/* Resume wilderness music in subsong's index and position, whenever switching from one wilderness music event to another? */
#define WILDERNESS_MUSIC_RESUME
#ifdef WILDERNESS_MUSIC_RESUME
 /* Also resume wilderness music if previous music was not wilderness music? This is mainly for staircase scumming at 0 ft <-> -50 ft. */
 #define WILDERNESS_MUSIC_ALWAYS_RESUME
 /* Also resume town and tavern music the same way wilderness music is resumed */
 #define TOWN_TAVERN_MUSIC_RESUME_TOO
#endif

/* Allow user-defined custom volume factor for each sample or song? ([].volume) */
#define USER_VOLUME_SFX
#define USER_VOLUME_MUS

#ifdef SOUND_AL_SDL
 // port from SDL to AL? Future stuff, if SDL3 still sux (or we want full 3d head model sfx)
#elif 0
 #include <SDL/SDL.h>
 #include <SDL/SDL_mixer.h>
 #include <SDL/SDL_thread.h>
#elif 1
 /* This works for SDL2 on Arm64, Linux and MinGW (i686-w64-mingw32) */
 #include <SDL.h>
 #include <SDL_mixer.h>
 #include <SDL_thread.h>
#else
 /* This is the way recommended by the SDL web page (this was from SDL1-times though) -- this was used till almost end of jan 2024 */
 #include "SDL.h"
 #include "SDL_mixer.h"
 #include "SDL_thread.h"
#endif

/* completely turn off mixing of disabled audio features (music, sound, weather, all)
   as opposed to just muting their volume? */
//#define DISABLE_MUTED_AUDIO

/* load sounds on the fly if the loader thread has not loaded them yet */
/* disabled because this can cause delays up to a few seconds */
bool on_demand_loading = FALSE;

#ifndef WINDOWS //assume POSIX
/* assume normal softlimit for maximum amount of file descriptors, minus some very generous overhead */
int sdl_files_max = 1024 - 32, sdl_files_cur = 0;
#endif

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
//bool kill_load_audio_thread = FALSE; -- not needed, since the thread_load_audio() never takes really long to complete
SDL_mutex *load_sample_mutex_entrance, *load_song_mutex_entrance;
SDL_mutex *load_sample_mutex, *load_song_mutex;


/* declare */
static void fadein_next_music(void);
static void clear_channel(int c);

static int thread_load_audio(void *dummy);
static Mix_Chunk* load_sample(int idx, int subidx);
static Mix_Music* load_song(int idx, int subidx);

static bool play_music_instantly(int event);


/* Arbitrary limit of mixer channels */
#define MAX_CHANNELS	32

/* Arbitary limit on number of samples per event [8] */
#define MAX_SAMPLES	100

/* Arbitary limit on number of music songs per situation [3] */
#define MAX_SONGS	100

/* Exponential volume level table
 * evlt[i] = round(0.09921 * exp(2.31 * i / 100))
 */

/* evtl must range from 0..MIX_MAX_VOLUME (an SDL definition, [128]) */


/* --------------------------- Mixing algorithms, pick one --------------------------- */
/* Normal mixing: */
//#define MIXING_NORMAL

/* Like normal mixing, but trying to use the edge cases better for broader volume spectrum - not finely implemented yet, DON'T USE!: */
//#define MIXING_EXTREME

/* Like normal mixing, but trying to use maximum range of slider multiplication: 0..10 = 11 steps, so 121 steps in total [RECOMMENDED]: */
#define MIXING_ULTRA

/* Actually add up volume sliders (Master + Specific) and average them, instead of multiplying them. (Drawback: Individual sliders cannot vary volume far away from master slider): */
//#define MIXING_ADDITIVE
/* ----------------------------------------------------------------------------------- */


#ifdef MIXING_NORMAL /* good, 12 is the lowest value at which the mixer is never completely muted */
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
#endif
#ifdef MIXING_EXTREME /* barely working: 9 is lowest possible, if we -for rounding errors- take care to always mix volume with all multiplications first, before dividing (otherwise 13 in MIXING_NORMAL is lowest). */
static s16b evlt[101] = { 9,
   9,  9,  9, 10, 10,	 10, 10, 11, 11, 11,
  12, 12, 12, 13, 13,	 13, 14, 14, 14, 15,
  15, 16, 16, 17, 17,	 17, 18, 18, 19, 19,
  20, 21, 21, 22, 22,	 23, 24, 24, 25, 26,
  26, 27, 28, 29, 29,	 30, 31, 32, 33, 34,
  34, 35, 36, 37, 38,	 39, 40, 42, 43, 44,
  45, 46, 48, 49, 50,	 52, 53, 54, 56, 57,
  59, 61, 62, 64, 66,	 67, 69, 71, 73, 75,
  77, 79, 81, 84, 86,	 88, 90, 93, 95, 98,
 101,103,106,109,112,	115,118,121,125,128
};
#endif
#ifdef MIXING_ULTRA /* make full use of the complete slider multiplication spectrum: 0..10 ^ 2 -> 121 values */
/* Purely exponentially we'd get 1 .. 1.04127^120 -> 1..128 (rounded).
   However, it'd again result in lacking diferences for low volume values, so let's try a superlinear mixed approach instead:
   i/3 + 1.038^i	- C. Blue
*/
static s16b evlt[121] = { 1 , //0
1 ,2 ,2 ,2 ,3 ,		3 ,4 ,4 ,4 ,5 ,		//10
5 ,6 ,6 ,6 ,7 ,		7 ,8 ,8 ,8 ,9 ,
9 ,10 ,10 ,10 ,11 ,	11 ,12 ,12 ,13 ,13 ,
14 ,14 ,14 ,15 ,15 ,	16 ,16 ,17 ,17 ,18 ,
18 ,19 ,19 ,20 ,20 ,	21 ,21 ,22 ,23 ,23 ,	//50
24 ,24 ,25 ,25 ,26 ,	27 ,27 ,28 ,29 ,29 ,
30 ,31 ,31 ,32 ,33 ,	34 ,35 ,35 ,36 ,37 ,
38 ,39 ,40 ,40 ,41 ,	42 ,43 ,44 ,45 ,46 ,
48 ,49 ,50 ,51 ,52 ,	53 ,55 ,56 ,57 ,59 ,
60 ,62 ,63 ,65 ,66 ,	68 ,70 ,71 ,73 ,75 ,	//100
77 ,79 ,81 ,83 ,85 ,	87 ,90 ,92 ,95 ,97 ,
100 ,103 ,105 ,108 ,111 ,	114 ,118 ,121 ,124 ,128
};
#endif
#ifdef MIXING_ADDITIVE /* special: for ADDITIVE (and averaging) slider mixing instead of the usual multiplicative! */
static s16b evlt[101] = { 1, //could start on 0 here
/* The values in the beginning up to around 13 are adjusted upwards and more linearly, to ensure that each mixer step (+5 array index) actually causes an audible change.
   The drawback is, that the low-mid range is not really exponential anymore and this will be especially noticable for boosted (>100%) audio, which sounds less boosted
   when the volume sliders' average is around the middle volume range :/ - can't have everything. To perfectly remedy this we'd need finer volume values in SDL. - C. Blue
   Said middle range from hear-testing, subjectively (in added slider step values from 0..10, master+music): 4..7 */
1 ,1 ,1 ,1 ,2 ,		2 ,2 ,2 ,2 ,3 ,
3 ,3 ,3 ,3 ,4 ,		4 ,4 ,4 ,4 ,5 ,
5 ,5 ,5 ,5 ,6 ,		6 ,6 ,6 ,6 ,7 ,
7 ,7 ,7 ,7 ,8 ,		8 ,8 ,8 ,8 ,9 ,
9 ,9 ,9 ,9 ,10,		10 ,10 ,11 ,11 ,11 ,
12 ,12 ,13 ,13 ,14 ,	15 ,15 ,16 ,17 ,18 ,
19 ,20 ,21 ,22 ,23 ,	24 ,25 ,27 ,28 ,29 ,
31 ,32 ,34 ,36 ,38 ,	39 ,41 ,44 ,46 ,48 ,
50 ,53 ,56 ,58 ,61 ,	64 ,68 ,71 ,75 ,78 ,
82 ,86 ,91 ,95 ,100 ,	105 ,110 ,116 ,122 ,128
};
#endif

#if 1 /* Exponential volume scale - mikaelh */
 #if 0 /* Use define */
  #define CALC_MIX_VOLUME(T, V)	(cfg_audio_master * T * evlt[V] * evlt[cfg_audio_master_volume] / MIX_MAX_VOLUME)
 #else /* Make it a function instead - needed for volume 200% boost in the new jukebox/options menu. */
  /* T: 0 or 1 for on/off. V: Volume in range of 0..100 (specific slider, possibly adjusted already with extra volume info).
     boost: 1..200 (percent): Despite it called 'boost' it is actually a volume value from 1 to 200, so could also lower audio (100 = keep normal volume). */
  static int CALC_MIX_VOLUME(int T, int V, int boost) {
  #if defined(MIXING_NORMAL) || defined(MIXING_EXTREME)
	int perc_lower, b_m, b_v;
	int Me, Ve;
	int roothack;
	int Mdiff, Vdiff, totaldiff;
  #endif
	int total;
	int M = cfg_audio_master_volume;

//c_msg_format("V:%d,boost:%d,cfgM:%d,cfgU:%d,*:%d,+:%d", V, boost, cfg_audio_master_volume, cfg_audio_music_volume, V * boost * cfg_audio_master_volume, (V + cfg_audio_master_volume) / 2);
	/* Normal, unboosted audio -- note that anti-boost is applied before evlt[], while actual boost (further below) is applied to evlt[] value. */
  #ifdef MIXING_NORMAL
	if (boost <= 100) return((cfg_audio_master * T * evlt[(V * boost) / 100] * evlt[M]) / MIX_MAX_VOLUME);
  #endif
  #ifdef MIXING_EXTREME
	if (boost <= 100) return(cfg_audio_master * T * evlt[(V * boost * M) / 10000]);
  #endif
  #ifdef MIXING_ULTRA
	if (boost <= 100) {
		total = ((V + 10) * (M + 10) * boost) / 10000 - 1; //0..120 (with boost < 100%, -1 may result)
		if (total < 0) total = 0; //prevent '-1' underflow when boost and volume is all low
//c_msg_format("v_=%d (%d*%d*(%d)=%d - %d)", evlt[total], M+10, V+10, boost, total, M);
		return(cfg_audio_master * T * evlt[total]);
	}
  #endif
  #ifdef MIXING_ADDITIVE
	if (boost <= 100) return(cfg_audio_master * T * evlt[((V + M) * boost) / 200]);
  #endif

  #if defined(MIXING_NORMAL) || defined(MIXING_EXTREME)
	boost -= 100;

	/* To be exact we'd have to balance roots, eg master slider has 3x as much room for boosting as specific slider ->
	   it gets 4rt(boost)^3 while specific slider gets 4rt(boost)^1, but I don't wanna do these calcs here, so I will
	   just hack it for now to use simple division and then subtract 4..11% from the result :-p
	   However, even that hack is inaccurate, as we would like to apply it to evlt[] values, not raw values. However, that requires us to apply
	   the perc_lower root factors to the evlt[] values too, again not to the raw values. BUT.. if we do that, we get anomalities such as a boost
	   value of 110 resulting in slightly LOWER sound than unboosted 100% sound, since the root_hack value doesn't necessarily correspond with
	   the jumps in the evlt[] table.
	   So, for now, I just apply the roothack on evlt[] and we live with the small inaccuracy =P - C. Blue */

	/* If we boost the volume, we have to distribute it on the two sliders <specific audio slider> vs <master audio slider> to be most effective. */
	if (M == 100 && V == 100) {
		/* Extreme case: Cannot boost at all, everything is at max already. */
		Me = evlt[M];
		Ve = evlt[V];
		return((cfg_audio_master * T * Ve * Me) / MIX_MAX_VOLUME);
	} else if (!M) {
		/* Extreme case (just treat it extra cause of rounding issue 99.9 vs 100) */
		roothack = 100;
		//M = (M * (100 + boost)) / 100; //--wrong (always 0)
		M = boost / 10;
	} else if (!V) {
		/* Extreme case (just treat it extra cause of rounding issue 99.9 vs 100) */
		roothack = 100;
		//V = (V * (100 + boost)) / 100; //--wrong (always 0)
		V = boost / 10;
	} else if (M > V) {
		/* Master slider is higher -> less capacity to boost it, instead load boosting on the specific slider more than on the master slider */
		Mdiff = 100 - M;
		Vdiff = 100 - V;
		totaldiff = Mdiff + Vdiff;
		perc_lower = (100 * Mdiff) / totaldiff;
		roothack = 100 - (21 - 1045 / (50 + perc_lower));

		b_m = (boost * perc_lower) / 100;
		b_v = (boost * (100 - perc_lower)) / 100;
		M = (M * (100 + b_m)) / 100;
		V = (V * (100 + b_v)) / 100;
	} else if (M < V) {
		/* Master slider is lower -> more capacity to boost it, so load boosting on the master slider more than on the specific slider */
		Mdiff = 100 - M;
		Vdiff = 100 - V;
		totaldiff = Mdiff + Vdiff;
		perc_lower = (100 * Vdiff) / totaldiff;
		roothack = 100 - (21 - 1045 / (50 + perc_lower));

		b_m = (boost * (100 - perc_lower)) / 100;
		b_v = (boost * perc_lower) / 100;
		M = (M * (100 + b_m)) / 100;
		V = (V * (100 + b_v)) / 100;
	} else {
		/* Both sliders are equal - distribute boost evenly (sqrt(boost)) */
		perc_lower = 50;
		roothack = 100 - (21 - 1045 / (50 + perc_lower));

		b_m = b_v = (boost * perc_lower) / 100;
		M = (M * (100 + b_m)) / 100;
		V = (V * (100 + b_v)) / 100;
	}

	/* Limit against overboosting into invalid values */
	if (M > 100) M = 100;
	if (V > 100) V = 100;

	Me = evlt[M];
	Ve = evlt[V];
	total = ((Me * Ve * roothack) / 100) / MIX_MAX_VOLUME;

  #elif defined(MIXING_ADDITIVE)
	/* A felt duplication of +100% corresponds roughly to 4 slider steps, aka +40 volume */
	boost = (boost - 100) * 4; //0..400
	boost /= 10; //+0..+40

	/* Compensate for less effective boost around low-mids (see evlt[] comment) for added-up slider pos range 4..7 */
	if (M + V >= 40 && M + V <= 70) {
		if (M + V == 40 || M + V == 70) boost += 10; //including 60 too may be disputable~
		else boost += 20;
	}

	/* Boost total volume */
	total = M + V + boost;

	/* Limit against overboosting into invalid values */
	if (total > 200) total = 200;

	/* Map to exponential volume */
	total = evlt[total / 2];
//c_msg_format("vB=%d (%d+%d+%d=%d)", total, M+10, V+10, boost, M + V + boost);

  #else /* MIXING_ULTRA */
	boost = (boost - 100); //+0..+100%
   #if 0 /* multiplicative boosting - doesn't do much at very low volume slider levels */
	boost /= 3;//+0..+33%
	/* Boost total volume */
	//total = ((M + 10) * (V + 10) * (100 + boost)) / 10000 - 1; //strictly rounded down boost, will not boost lowest volume since index is only 1.3 vs 1 -> still rounded down
	total = ((M + 10) * (V + 10) * (100 + boost) + 8400) / 10000 - 1; //will reach at least +1 volume index if at least 150% boosting (50%/3 = 16 ->> 1.16 + .84 = 2)
   #else /* additive boosting */
	/* A felt duplication of +100% corresponds roughly to 4 slider steps */
	boost = (boost * 4) / 10; //+0..+40
	total = ((M + 10 + boost / 2) * (V + 10 + boost / 2)) / 100 - 1; //will reach at least +1 volume index if at least 150% boosting (50%/3 = 16 ->> 1.16 + .84 = 2)
   #endif

	/* Limit against overboosting into invalid values */
	if (total > 120) total = 120;

	/* Map to superlinear volume */
	total = evlt[total];
//c_msg_format("vB=%d (%d+%d+%d)", total, M+10, V+10, 100+boost);
  #endif

	/* Return boosted >100% result */
	return(cfg_audio_master * T * total);
  }
 #endif
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


/* Positional audio - C. Blue
   Keeping it retro-style with this neat sqrt table.. >_>
   Note that it must cover (for pythagoras) the maximum sum of dx^2+dy^2 where distance(0,0,dy,dx) can be up to MAX_SIGHT [20]!
   This is unfortunately full of rounding/approximation issues, so we will just go with dx,dy=14,13 (365) and dx,dy=19,3 (370, max!)
   actually (19,2 but rounding!) as limits and reduce coords beyond this slightly. */
static int fast_sqrt[371] = { /* 0..370, for panning */
    0,1,1,1,2,2,2,2,2,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
    12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
    14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
    16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
    18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,19,19,19,19,19,19,19,19,19,19
};

/* Struct representing all data about an event sample */
typedef struct {
	int num;                        /* Number of samples for this event */
	Mix_Chunk *wavs[MAX_SAMPLES];   /* Sample array */
	const char *paths[MAX_SAMPLES]; /* Relative pathnames for samples */
	int current_channel;		/* Channel it's currently being played on, -1 if none; to avoid
					   stacking of the same sound multiple (read: too many) times - ...
					   note that with 4.4.5.4+ this is deprecated - C. Blue */
	int started_timer_tick;		/* global timer tick on which this sample was started (for efficiency) */
	bool disabled;			/* disabled by user? */
	bool config;
	unsigned char volume;		/* volume reduced by user? */
} sample_list;

/* background music */
typedef struct {
	int num;
	Mix_Music *wavs[MAX_SONGS];
	const char *paths[MAX_SONGS];

	bool is_reference[MAX_SONGS];	/* Is just a reference pointer to another event, so don't accidentally try to "free" this one. */
	int referenced_event[MAX_SONGS];
	int referenced_song[MAX_SONGS];

	bool initial[MAX_SONGS];	/* Is it an 'initial' song? An initial song is played first and only once when a music event gets activated. */
#ifdef WILDERNESS_MUSIC_RESUME
	int bak_song, bak_pos;		/* Specifically for 'wilderness' music: Remember song and position of each wilderness-type event. */
#endif
	bool disabled;			/* disabled by user? */
	bool config;
	unsigned char volume;		/* volume reduced by user? */
} song_list;

/* SFX array (including weather + ambient sfx) */
static sample_list samples[SOUND_MAX_2010];
/* Music array */
static song_list songs[MUSIC_MAX];

/* Array of potential channels, for managing that only one
   sound of a kind is played simultaneously, for efficiency - C. Blue */
static int channel_sample[MAX_CHANNELS];
static int channel_type[MAX_CHANNELS];
static int channel_volume[MAX_CHANNELS];
static s32b channel_player_id[MAX_CHANNELS];

#ifdef ENABLE_JUKEBOX
/* Jukebox music */
static int curmus_timepos = -1, curmus_x, curmus_y = -1, curmus_attr, curmus_song_dur = 0;
static int jukebox_org = -1, jukebox_shuffle_map[MUSIC_MAX], jukebox_paused = FALSE;
static bool jukebox_sfx_screen = FALSE, jukebox_static200vol = FALSE, jukebox_shuffle = FALSE;
#endif


/*
 * Shut down the sound system and free resources.
 */
static void close_audio(void) {
	size_t i;
	int j;

	/* Kill the loading thread if it's still running */
	if (load_audio_thread) {
		//kill_load_audio_thread = TRUE; -- not needed (see far above)
		SDL_WaitThread(load_audio_thread, NULL);
		/* In case the system gets re-initialized, it seems without nulling this the client may sometimes hang indefinitely in SDL_WaitThread() on client quit. */
		load_audio_thread = NULL;
	}

	Mix_HaltMusic();

	/* Free all the sample data*/
	for (i = 0; i < SOUND_MAX_2010; i++) {
		sample_list *smp = &samples[i];

		/* Nuke all samples */
		for (j = 0; j < smp->num; j++) {
			if (smp->wavs[j]) {
				Mix_FreeChunk(smp->wavs[j]);
				smp->wavs[j] = NULL;
			}
			if (smp->paths[j]) {
				string_free(smp->paths[j]);
				smp->paths[j] = NULL;
			}
		}
		/* In case the system gets re-initialized, make sure the sample-counter of then perhaps unused sfx is actually initialized with zero,
		   or we'd get a double-free crash on a subsequent close_audio() call. */
		smp->num = 0;
	}

	/* Free all the music data*/
	for (i = 0; i < MUSIC_MAX; i++) {
		song_list *mus = &songs[i];

		/* Nuke all songs */
		for (j = 0; j < mus->num; j++) {
			if (mus->is_reference[j]) continue; /* Just a reference pointer, not the original -> would result in "double-free" crash. */

			if (mus->wavs[j]) {
				Mix_FreeMusic(mus->wavs[j]);
				mus->wavs[j] = NULL;
			}
			if (mus->paths[j]) {
				string_free(mus->paths[j]);
				mus->paths[j] = NULL;
			}
		}
		/* In case the system gets re-initialized, make sure the song-counter of then perhaps unused music is actually initialized with zero,
		   or we'd get a double-free crash on a subsequent close_audio() call. */
		mus->num = 0;
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

/* Just for external call when using  = I  to install an audio pack while already running */
void close_audio_sdl(void) {
	close_audio();
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
	audio_channels = MIX_DEFAULT_CHANNELS; // this is == 2

	/* Initialize the SDL library */
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
//#ifdef DEBUG_SOUND
		plog_fmt("Couldn't initialize SDL: %s", SDL_GetError());
//		puts(format("Couldn't initialize SDL: %s", SDL_GetError()));
//#endif
		return(FALSE);
	}

	/* Try to open the audio */
	if (cfg_audio_buffer < 128) cfg_audio_buffer = 128;
	if (cfg_audio_buffer > 8192) cfg_audio_buffer = 8192;
	if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, cfg_audio_buffer) < 0) {
//#ifdef DEBUG_SOUND
		plog_fmt("Couldn't open mixer: %s", SDL_GetError());
//		puts(format("Couldn't open mixer: %s", SDL_GetError()));
//#endif
		return(FALSE);
	}

	if (cfg_max_channels > MAX_CHANNELS) cfg_max_channels = MAX_CHANNELS;
	if (cfg_max_channels < 4) cfg_max_channels = 4;
	Mix_AllocateChannels(cfg_max_channels);
	/* set hook for clearing faded-out weather and for managing sound-fx playback */
	Mix_ChannelFinished(clear_channel);
	/* set hook for fading over to next song */
	Mix_HookMusicFinished(fadein_next_music);

	/* Success */
	return(TRUE);
}

#ifndef WINDOWS //assume POSIX
 #include <sys/resource.h> /* for rlimit et al */
static int get_filedescriptor_limit(void) {
	struct rlimit limit;

 #if 0
	limit.rlim_cur = 65535;
	limit.rlim_max = 65535;
	if (setrlimit(RLIMIT_NOFILE, &limit) != 0) {
		printf("setrlimit() failed with errno=%d\n", errno);
		return(0);
	}
 #endif

	/* Get max number of files. */
	if (getrlimit(RLIMIT_NOFILE, &limit) != 0) {
		printf("getrlimit() failed with errno=%d\n", errno);
		return(1024); //assume default
	}

	if (limit.rlim_cur > 65536) limit.rlim_cur = 65536; //cap to sane values to save resources

	return(limit.rlim_cur);
}
/* note: method 1 and method 2 report different amounts of free file descriptors at different times, and also different amounts before and after loading audio files -_- */
 #include <dirent.h>
int count_open_fds1(void) {
	struct dirent *de;
	int count = -3; // '.', '..', dp
	DIR *dp = opendir("/proc/self/fd");

	if (dp == NULL) return(-1);

	while ((de = readdir(dp)) != NULL) count++;
	(void)closedir(dp);

	return(count + 3);
}
 #include <poll.h>
int count_open_fds2(int max) {
	struct pollfd fds[max];
	int ret, i, count = 0;

	for (i = 0; i < max; i++) fds[max].events = 0;

	ret = poll(fds, max, 0);
	if (ret <= 0) return(0);

	for (i = 0; i < max; i++)
		if (fds[i].revents & POLLNVAL) count++;

	return(count);
}
void log_fd_usage(void) {
	int max_files, cur_files1, cur_files2;

	max_files = get_filedescriptor_limit();
	cur_files1 = count_open_fds1();
	cur_files2 = count_open_fds2(max_files);

	logprint(format("max_files = %d, cur_files1/2 = %d/%d -> sdl_files_cur/max = %d\n", max_files, cur_files1, cur_files2, sdl_files_cur, sdl_files_max));
}
#endif


/*
 * Read sound.cfg and map events to sounds; then load all the sounds into
 * memory to avoid I/O latency later.
 */
/* Instead of using one large string buffer, use multiple smaller ones? (Not recommended, was just for debugging/testing) */
#define BUFFERSIZE 4096
static bool sound_sdl_init(bool no_cache) {
	char path[2048];
	char buffer0[BUFFERSIZE] = { 0 }, *buffer = buffer0, bufferx[BUFFERSIZE] = { 0 };
	FILE *fff;
	int i, j, cur_line, k;
	char out_val[160];
	bool disabled, cat_this_line, cat_next_line;

	bool events_loaded_semaphore;

	/* Note: referencef-feature is for music only (music.cfg), not available for sounds (in sound.cfg). */
	int references = 0;
	int referencer[REFERENCES_MAX];
	bool reference_initial[REFERENCES_MAX];
	char referenced_event[REFERENCES_MAX][MAX_CHARS_WIDE];

#ifndef WINDOWS //assume POSIX
	/* for checking whether we have enough available file descriptors for loading a lot of audio files */
	int max_files = get_filedescriptor_limit(), cur_files1 = count_open_fds1(), cur_files2 = count_open_fds2(max_files);


	sdl_files_max = max_files - (cur_files1 > cur_files2 ? cur_files1 : cur_files2);

#ifdef DEBUG_SOUND
	log_fd_usage()
#endif
#endif

	/* Paranoia? null all the pointers */
	for (i = 0; i < SOUND_MAX_2010; i++) {
		for (j = 0; j < MAX_SAMPLES; j++) {
			samples[i].wavs[j] = NULL;
			samples[i].paths[j] = NULL;
		}
		samples[i].num = 0;
		samples[i].config = FALSE;
		samples[i].disabled = FALSE;
	}
	for (i = 0; i < MUSIC_MAX; i++) {
		for (j = 0; j < MAX_SONGS; j++) {
			songs[i].wavs[j] = NULL;
			songs[i].paths[j] = NULL;
			songs[i].is_reference[j] = FALSE;
		}
		songs[i].num = 0;
		songs[i].config = FALSE;
		songs[i].disabled = FALSE;
#ifdef WILDERNESS_MUSIC_RESUME
		songs[i].bak_pos = 0;
#endif
	}


	load_sample_mutex_entrance = SDL_CreateMutex();
	load_song_mutex_entrance = SDL_CreateMutex();
	load_sample_mutex = SDL_CreateMutex();
	load_song_mutex = SDL_CreateMutex();

	/* Initialise the mixer  */
	if (!open_audio()) return(FALSE);

#ifdef DEBUG_SOUND
	puts(format("sound_sdl_init() opened at %d Hz.", cfg_audio_rate));
#endif

	/* Initialize sound-fx channel management */
	for (i = 0; i < cfg_max_channels; i++) channel_sample[i] = -1;


	/* ------------------------------- Init Sounds */

	/* Build the "sound" path */
	path_build(path, sizeof(path), ANGBAND_DIR_XTRA, cfg_soundpackfolder);
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
	if (!win_dontmoveuser && getenv("HOMEDRIVE") && getenv("HOMEPATH"))
		sprintf(path, "%s\\TomeNET-%s.cfg", ANGBAND_DIR_USER, cfg_soundpackfolder);
	else path_build(path, sizeof(path), ANGBAND_DIR_XTRA_SOUND, "TomeNET-sound.cfg"); //paranoia

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
		return(FALSE);
#else /* try to use simple default file */
		FILE *fff2;
		char path2[2048];

		path_build(path, sizeof(path), ANGBAND_DIR_XTRA_SOUND, "sound.cfg.default");
		fff = my_fopen(path, "r");
		if (!fff) {
			plog_fmt("Failed to open default sound config (%s):\n    %s", path, strerror(errno));
			return(FALSE);
		}

		path_build(path2, sizeof(path2), ANGBAND_DIR_XTRA_SOUND, "sound.cfg");
		fff2 = my_fopen(path2, "w");
		if (!fff2) {
			plog_fmt("Failed to write sound config (%s):\n    %s", path2, strerror(errno));
			return(FALSE);
		}

		while (my_fgets(fff, buffer, sizeof(buffer0)) == 0)
			fprintf(fff2, "%s\n", buffer);

		my_fclose(fff2);
		my_fclose(fff);

		/* Try again */
		path_build(path, sizeof(path), ANGBAND_DIR_XTRA_SOUND, "sound.cfg");
		fff = my_fopen(path, "r");
		if (!fff) {
			plog_fmt("Failed to open sound config (%s):\n    %s", path, strerror(errno));
			return(FALSE);
		}
#endif
	}

#ifdef DEBUG_SOUND
	puts("sound_sdl_init() reading sound.cfg:");
#endif

	/* Parse the file */
	/* Lines are always of the form "name = sample [sample ...]" */
	cur_line = 0;
	cat_this_line = cat_next_line = FALSE;
	while (fgets(buffer0, sizeof(buffer0), fff) != 0) {
		char *cfg_name;
		cptr lua_name;
		char *sample_sublist;
		char *search;
		char *cur_token;
		char *next_token;
		int event;
		char *c;

		cur_line++;

		if (strlen(buffer0) >= BUFFERSIZE - 1 && buffer0[strlen(buffer0) - 1] != '\n') {
			logprint(format("Sound.cfg: Discarded line %d as it is too long (must be %d at most)\n", cur_line, BUFFERSIZE - 1));
			continue;
		}

		if (buffer0[strlen(buffer0) - 1] == '\n') buffer0[strlen(buffer0) - 1] = 0; //trim linefeed (the last line in the file might not have one)

		/* Everything after a non-quoted '#' gets ignored */
		c = buffer0;
		while (*c && (c = strchr(c, '#'))) {
			char *c2;
			bool quoted = 0;

			/* Check if the # is caught inside quotes, then part of a legal file name eg "botpack #9.mp3" */
			c2 = buffer0;
			while (*c2 && c2 < c) {
				if (*c2 == '"') quoted = !quoted;
				c2++;
			}
			if (quoted) {
				c++;
				continue;
			}

			/* Ignore everything after the '#' */
			*c = 0;
			break;
		}

		/* strip trailing spaces/tabs */
		c = buffer0;
		while (*c && (c[strlen(c) - 1] == ' ' || c[strlen(c) - 1] == '\t')) c[strlen(c) - 1] = 0;

		/* (2022, 4.9.0) strip preceding spaces/tabs */
		c = buffer0;
		while (*c == ' ' || *c == '\t') c++;

		/* New (2018): Allow linewrapping via trailing ' \' character sequence right before EOL */
		if (cat_next_line) {
			cat_this_line = TRUE;
			cat_next_line = FALSE;
		}
		if (strlen(c) >= 2 && c[strlen(c) - 1] == '\\' && (c[strlen(c) - 2] == ' ' || c[strlen(c) - 2] == '\t')) {
			c[strlen(c) - 2] = 0; /* Discard the '\' and the space (we re-insert the space next, if required) */
			cat_next_line = TRUE;
		}
		if (!cat_this_line) strcpy(bufferx, c);
		else {
			cat_this_line = FALSE;
			if (strlen(bufferx) + strlen(c) + 1 >= BUFFERSIZE) { //+1: we strcat a space sometimes, see below
				logprint(format("Warning: Sound.cfg line #%d is too long to concatinate.\n", cur_line));
				/* String overflow protection: Discard all that is too much. -- fall through and go on without this line */
			} else {
				/* If the continuation of the wrapped line doesn't start on a space, re-insert a space to ensure proper parameter separation */
				if (c[0] != ' ' && c[0] != '\t' && c[0]) strcat(bufferx, " ");
				strcat(bufferx, c);
			}
		}
		if (cat_next_line) continue;
		/* Proceed with the beginning of the whole (multi-)line */
		strcpy(buffer0, bufferx);

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

		/* Skip meta data that we don't need here -- this is for [title] tag introduced in 4.7.1b+ */
		if (!strncmp(buffer, "packname", 8) || !strncmp(buffer, "author", 6) || !strncmp(buffer, "description", 11) || !strncmp(buffer, "version", 7)) {
			char *ckey = buffer, *cval;

			/* Search for key separator */
			if (!(cval = strchr(ckey, '='))) continue;
			*cval = 0;
			cval++;
			/* Trim spaces/tabs */
			while (strlen(ckey) && (ckey[strlen(ckey) - 1] == ' ' || ckey[strlen(ckey) - 1] == '\t')) ckey[strlen(ckey) - 1] = 0;
			while (*cval == ' ' || *cval == '\t') cval++;
			while (strlen(cval) && (cval[strlen(cval) - 1] == ' ' || cval[strlen(cval) - 1] == '\t')) cval[strlen(cval) - 1] = 0;

			if (!strncmp(buffer, "packname", 8)) strncpy(cfg_soundpack_name, cval, MAX_CHARS);
			//if (!strncmp(buffer, "author", 6)) ;
			//if (!strncmp(buffer, "description", 11)) ;
			if (!strncmp(buffer, "version", 7)) strncpy(cfg_soundpack_version, cval, MAX_CHARS);
			continue;
		}

		/* Split the line into two: the key, and the rest */

		search = strchr(buffer, '=');
		/* no event name given? */
		if (!search) continue;
		*search = 0;
		search++;
		/* Event name (key): Trim spaces/tabs */
		while (*buffer && (buffer[strlen(buffer) - 1] == ' ' || buffer[strlen(buffer) - 1] == '\t')) buffer[strlen(buffer) - 1] = 0;
		if (!(*buffer)) continue; /* No key (aka event) name given */
		/* File name (value): Trim spaces/tabs */
		while (*search && (search[strlen(search) - 1] == ' ' || search[strlen(search) - 1] == '\t')) search[strlen(search) - 1] = 0;
		if (!(*search)) continue; /* No value (aka name) given for the key */

		/* Set the event name */
		cfg_name = buffer;

		/* Make sure this is a valid event name */
		for (event = SOUND_MAX_2010 - 1; event >= 0; event--) {
			sprintf(out_val, "return get_sound_name(%d)", event);
			lua_name = string_exec_lua(0, out_val);
			if (!strlen(lua_name)) continue;
			if (strcmp(cfg_name, lua_name) == 0) break;
		}
		if (event < 0) {
			logprint(format("Sound effect '%s' not in audio.lua\n", cfg_name));
			continue;
		}

		/*
		 * Now we find all the sample names and add them one by one
		*/
		events_loaded_semaphore = FALSE;

		/* Songs: Trim spaces/tabs */
		c = search;
		while (*c == ' ' || *c == '\t') c++;
		sample_sublist = c;

		/* no audio filenames listed? */
		if (!sample_sublist[0]) continue;

		/* Terminate the current token */
		cur_token = sample_sublist;
		/* Handle sample names within quotes */
		if (cur_token[0] == '\"') {
			cur_token++;
			search = strchr(cur_token, '\"');
			if (search) {
				search[0] = '\0';
				search = strpbrk(search + 1, " \t");
			} else logprint(format("Sound.cfg error: Missing closing quotes in line %d.\n", cur_line));
		} else {
			search = strpbrk(cur_token, " \t");
		}
		if (search) {
			search[0] = '\0';
			next_token = search + 1;
		} else {
			next_token = NULL;
		}

		/* Sounds: Trim spaces/tabs */
		if (next_token) while (*next_token == ' ' || *next_token == '\t') next_token++;

		while (cur_token) {
			int num = samples[event].num;

			/* Don't allow too many samples */
			if (num >= MAX_SAMPLES) break;

			/* Build the path to the sample */
			path_build(path, sizeof(path), ANGBAND_DIR_XTRA_SOUND, cur_token);
			if (!my_fexists(path)) {
				logprint(format("Can't find sample '%s' (line %d)\n", cur_token, cur_line));
				goto next_token_snd;
			}

			/* Don't load now if we're not caching */
			if (no_cache) {
				/* Just save the path for later */
				samples[event].paths[num] = string_make(path);
			} else {
#ifndef WINDOWS //assume POSIX
				sdl_files_cur++;
				if (sdl_files_cur >= sdl_files_max) {
					logprint(format("Too many audio files. Reached maximum of %d, discarding <%s>.\n", sdl_files_max, path));
					goto next_token_snd;
				}
#ifdef DEBUG_SOUND
				logprint(format("s-sdl_files_cur++ = %d/%d (%s)\n", sdl_files_cur, sdl_files_max, path));
#endif
#endif

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
				/* Handle song names within quotes */
				if (cur_token[0] == '\"') {
					cur_token++;
					search = strchr(cur_token, '\"');
					if (search) {
						search[0] = '\0';
						search = strpbrk(search + 1, " \t");
					} else logprint(format("Sound.cfg error: Missing closing quotes in line %d.\n", cur_line));
				} else {
					/* Try to find a space */
					search = strpbrk(cur_token, " \t");
				}
				/* If we can find one, terminate, and set new "next" */
				if (search) {
					search[0] = '\0';
					next_token = search + 1;
				} else {
					/* Otherwise prevent infinite looping */
					next_token = NULL;
				}

				/* Sounds: Trim spaces/tabs */
				if (next_token) while (*next_token == ' ' || *next_token == '\t') next_token++;
			}
		}

		/* disable this sfx? */
		if (disabled) samples[event].disabled = TRUE;
	}

#ifdef WINDOWS
	/* On Windows we must have a second config file just to store disabled-state, since we cannot write to Program Files folder after Win XP anymore..
	   So if it exists, let it override the normal config file. */
	if (!win_dontmoveuser && getenv("HOMEDRIVE") && getenv("HOMEPATH"))
		sprintf(path, "%s\\TomeNET-%s-disabled.cfg", ANGBAND_DIR_USER, cfg_soundpackfolder);
	else path_build(path, sizeof(path), ANGBAND_DIR_XTRA_SOUND, "TomeNET-nosound.cfg"); //paranoia

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
	my_fclose(fff);
#endif

#ifdef WINDOWS
	if (!win_dontmoveuser && getenv("HOMEDRIVE") && getenv("HOMEPATH"))
		sprintf(path, "%s\\TomeNET-%s-volume.cfg", ANGBAND_DIR_USER, cfg_soundpackfolder);
	else path_build(path, sizeof(path), ANGBAND_DIR_XTRA_SOUND, "TomeNET-soundvol.cfg"); //paranoia
#else
	path_build(path, 1024, ANGBAND_DIR_XTRA_SOUND, "TomeNET-soundvol.cfg");
#endif

	fff = my_fopen(path, "r");
	if (fff) {
		while (my_fgets(fff, buffer0, sizeof(buffer0)) == 0) {
			/* find out event state (disabled/enabled) */
			i = exec_lua(0, format("return get_sound_index(\"%s\")", buffer0));
			if (my_fgets(fff, buffer0, sizeof(buffer0))) break; /* Error, incomplete entry */
			/* unknown (different game version/sound pack?) */
			if (i == -1 || !samples[i].config) continue;
			/* set sample volume in this file */
			samples[i].volume = atoi(buffer0);
		}
	}
	my_fclose(fff);

#ifndef WINDOWS //assume POSIX
#ifdef DEBUG_SOUND
	log_fd_usage();
#endif
#endif


	/* ------------------------------- Init Music */

	buffer = buffer0;

	/* Build the "music" path */
	path_build(path, sizeof(path), ANGBAND_DIR_XTRA, cfg_musicpackfolder);
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
	if (!win_dontmoveuser && getenv("HOMEDRIVE") && getenv("HOMEPATH"))
		sprintf(path, "%s\\TomeNET-%s.cfg", ANGBAND_DIR_USER, cfg_musicpackfolder);
	else path_build(path, sizeof(path), ANGBAND_DIR_XTRA_MUSIC, "TomeNET-music.cfg"); //paranoia

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
		return(FALSE);
#else /* try to use simple default file */
		FILE *fff2;
		char path2[2048];

		path_build(path, sizeof(path), ANGBAND_DIR_XTRA_MUSIC, "music.cfg.default");
		fff = my_fopen(path, "r");
		if (!fff) {
			plog_fmt("Failed to open default music config (%s):\n    %s", path, strerror(errno));
			return(FALSE);
		}

		path_build(path2, sizeof(path2), ANGBAND_DIR_XTRA_MUSIC, "music.cfg");
		fff2 = my_fopen(path2, "w");
		if (!fff2) {
			plog_fmt("Failed to write music config (%s):\n    %s", path2, strerror(errno));
			return(FALSE);
		}

		while (my_fgets(fff, buffer, sizeof(buffer0)) == 0)
			fprintf(fff2, "%s\n", buffer);

		my_fclose(fff2);
		my_fclose(fff);

		/* Try again */
		path_build(path, sizeof(path), ANGBAND_DIR_XTRA_MUSIC, "music.cfg");
		fff = my_fopen(path, "r");
		if (!fff) {
			plog_fmt("Failed to open music config (%s):\n    %s", path, strerror(errno));
			return(FALSE);
		}
#endif
	}

#ifdef DEBUG_SOUND
	puts("sound_sdl_init() reading music.cfg:");
#endif

	/* Parse the file */
	/* Lines are always of the form "name = music [music ...]" */
	cur_line = 0;
	cat_this_line = cat_next_line = FALSE;
	while (fgets(buffer0, sizeof(buffer0), fff) != 0) {
		char *cfg_name;
		cptr lua_name;
		char *song_sublist;
		char *search;
		char *cur_token;
		char *next_token;
		int event;
		bool initial, reference;
		char *c;

		cur_line++;


		if (!buffer0[0]) {
			if (cat_next_line) logprint(format("Warning: Blank line follows ' \\' line concatenator: #%d -> #%d!\n", cur_line - 1, cur_line));
			continue;
		}
		if (strlen(buffer0) >= BUFFERSIZE - 1 && buffer0[strlen(buffer0) - 1] != '\n') {
			logprint(format("Music.cfg: Discarded line %d as it is too long (must be %d at most)\n", cur_line, BUFFERSIZE - 1));
			continue;
		}

		if (buffer0[strlen(buffer0) - 1] == '\n') buffer0[strlen(buffer0) - 1] = 0; //trim linefeed (the last line in the file might not have one)
		if (!buffer0[0]) {
			if (cat_next_line) logprint(format("Warning: Blank line follows ' \\' line concatenator: #%d -> #%d!\n", cur_line - 1, cur_line));
			continue;
		}


		/* Everything after a non-quoted '#' gets ignored */
		c = buffer0;
		while (*c && (c = strchr(c, '#'))) {
			char *c2;
			bool quoted = 0;

			/* Check if the # is caught inside quotes, then part of a legal file name eg "botpack #9.mp3" */
			c2 = buffer0;
			while (*c2 && c2 < c) {
				if (*c2 == '"') quoted = !quoted;
				c2++;
			}
			if (quoted) {
				c++;
				continue;
			}

			/* Ignore everything after the '#' */
			*c = 0;
			break;
		}

		/* strip trailing spaces/tabs */
		c = buffer0;
		while (*c && (c[strlen(c) - 1] == ' ' || c[strlen(c) - 1] == '\t')) c[strlen(c) - 1] = 0;

		/* (2022, 4.9.0) strip preceding spaces/tabs */
		c = buffer0;
		while (*c == ' ' || *c == '\t') c++;

		if (!*c) {
			if (cat_next_line) logprint(format("Warning: Blank line follows ' \\' line concatenator: #%d -> #%d!\n", cur_line - 1, cur_line));
			continue;
		}


		/* New (2018): Allow linewrapping via trailing ' \' character sequence right before EOL */
		if (cat_next_line) {
			cat_this_line = TRUE;
			cat_next_line = FALSE;
		}
		if (strlen(c) >= 2 && c[strlen(c) - 1] == '\\' && (c[strlen(c) - 2] == ' ' || c[strlen(c) - 2] == '\t')) {
			c[strlen(c) - 2] = 0; /* Discard the '\' and the space (we re-insert the space next, if required) */
			cat_next_line = TRUE;
		}
		if (!cat_this_line) strcpy(bufferx, c);
		else {
			cat_this_line = FALSE;
			if (strlen(bufferx) + strlen(c) + 1 >= BUFFERSIZE) { //+1: we strcat a space sometimes, see below
				logprint(format("Warning: Music.cfg line #%d is too long to concatinate.\n", cur_line));
				/* String overflow protection: Discard all that is too much. -- fall through and go on without this line */
			} else {
				/* If the continuation of the wrapped line doesn't start on a space, re-insert a space to ensure proper parameter separation */
				if (c[0] != ' ' && c[0] != '\t' && c[0]) strcat(bufferx, " ");
				strcat(bufferx, c);
			}
		}
		if (cat_next_line) continue;
		/* Proceed with the beginning of the whole (multi-)line */
		strcpy(buffer0, bufferx);
#ifdef DEBUG_SOUND
		printf("<%s>\n", buffer0);
#endif

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

		/* Skip meta data that we don't need here -- this is for [title] tag introduced in 4.7.1b+ */
		if (!strncmp(buffer, "packname", 8) || !strncmp(buffer, "author", 6) || !strncmp(buffer, "description", 11) || !strncmp(buffer, "version", 7)) {
			char *ckey = buffer, *cval;

			/* Search for key separator */
			if (!(cval = strchr(ckey, '='))) continue;
			*cval = 0;
			cval++;
			/* Trim spaces/tabs */
			while (strlen(ckey) && (ckey[strlen(ckey) - 1] == ' ' || ckey[strlen(ckey) - 1] == '\t')) ckey[strlen(ckey) - 1] = 0;
			while (*cval == ' ' || *cval == '\t') cval++;
			while (strlen(cval) && (cval[strlen(cval) - 1] == ' ' || cval[strlen(cval) - 1] == '\t')) cval[strlen(cval) - 1] = 0;

			if (!strncmp(buffer, "packname", 8)) strncpy(cfg_musicpack_name, cval, MAX_CHARS);
			//if (!strncmp(buffer, "author", 6)) ;
			//if (!strncmp(buffer, "description", 11)) ;
			if (!strncmp(buffer, "version", 7)) strncpy(cfg_musicpack_version, cval, MAX_CHARS);
			continue;
		}


		/* Split the line into two: the key, and the rest */

		search = strchr(buffer, '=');
		/* no event name given? */
		if (!search) continue;
		*search = 0;
		search++;
		/* Event name (key): Trim spaces/tabs */
		while (*buffer && (buffer[strlen(buffer) - 1] == ' ' || buffer[strlen(buffer) - 1] == '\t')) buffer[strlen(buffer) - 1] = 0;
		if (!(*buffer)) continue; /* No key (aka event) name given */
		/* File name (value): Trim spaces/tabs */
		while (*search && (search[strlen(search) - 1] == ' ' || search[strlen(search) - 1] == '\t')) search[strlen(search) - 1] = 0;
		if (!(*search)) continue; /* No value (aka name) given for the key */

		/* Set the event name */
		cfg_name = buffer;

		/* Make sure this is a valid event name */
		for (event = MUSIC_MAX - 1; event >= 0; event--) {
			sprintf(out_val, "return get_music_name(%d)", event);
			lua_name = string_exec_lua(0, out_val);
			if (!strlen(lua_name)) continue;
			if (strcmp(cfg_name, lua_name) == 0) break;
		}
		if (event < 0) {
			logprint(format("Music event '%s' not in audio.lua\n", cfg_name));
			continue;
		}

		/*
		 * Now we find all the song names and add them one by one
		*/
		events_loaded_semaphore = FALSE;

		/* Songs: Trim spaces/tabs */
		c = search;
		while (*c == ' ' || *c == '\t') c++;
		song_sublist = c;

		/* no audio filenames listed? */
		if (!song_sublist[0]) continue;

		/* Terminate the current token */
		cur_token = song_sublist;
		/* Handle '!' indicator for 'initial' songs */
		if (cur_token[0] == '!') {
			cur_token++;
			initial = TRUE;
		} else initial = FALSE;
		/* Handle '+' indicator for 'referenced' music events */
		reference = FALSE;
		if (cur_token[0] == '+') {
			reference = TRUE;
			cur_token++;
		}
		/* Handle song names within quotes */
		if (cur_token[0] == '\"') {
			cur_token++;
			search = strchr(cur_token, '\"');
			if (search) {
				search[0] = '\0';
				search = strpbrk(search + 1, " \t");
			} else logprint(format("Music.cfg error: Missing closing quotes in line %d.\n", cur_line));
		} else {
			search = strpbrk(cur_token, " \t");
		}
		if (search) {
			search[0] = '\0';
			next_token = search + 1;
		} else {
			next_token = NULL;
		}

		/* Songs: Trim spaces/tabs */
		if (next_token) while (*next_token == ' ' || *next_token == '\t') next_token++;

		while (cur_token) {
			int num = songs[event].num;

			/* Don't allow too many songs */
			if (num >= MAX_SONGS) break;

			/* Handle reference */
			if (reference) {
				/* Too many references already? Skip it */
				if (references >= REFERENCES_MAX) goto next_token_mus;
				/* Remember reference for handling them all later, to avoid cycling reference problems */
				referencer[references] = event;
				reference_initial[references] = initial;
				strcpy(referenced_event[references], cur_token);
#ifdef DEBUG_SOUND
				logprint(format("added REF #%d <%d> -> <%s>\n", references, event, cur_token));
#endif
				references++;

				if (!events_loaded_semaphore) {
					events_loaded_semaphore = TRUE;
					audio_music++;
					/* for do_cmd_options_...(): remember that this sample was mentioned in our config file */
					songs[event].config = TRUE;
				}

				goto next_token_mus;
			}

			/* Build the path to the sample */
			path_build(path, sizeof(path), ANGBAND_DIR_XTRA_MUSIC, cur_token);
			if (!my_fexists(path)) {
				logprint(format("Can't find song '%s' (line %d)\n", cur_token, cur_line));
				goto next_token_mus;
			}

			songs[event].initial[num] = initial;

			/* Don't load now if we're not caching */
			if (no_cache) {
				/* Just save the path for later */
				songs[event].paths[num] = string_make(path);
			} else {
#ifndef WINDOWS //assume POSIX
				sdl_files_cur++;
				if (sdl_files_cur >= sdl_files_max) {
					logprint(format("Too many audio files. Reached maximum of %d, discarding <%s>.\n", sdl_files_max, path));
					goto next_token_mus;
				}
#ifdef DEBUG_SOUND
				logprint(format("m-sdl_files_cur++ = %d/%d (%s)\n", sdl_files_cur, sdl_files_max, path));
#endif
#endif

				/* Load the file now */
				songs[event].wavs[num] = Mix_LoadMUS(path);
				if (!songs[event].wavs[num]) {
					//puts(format("PRBPATH: lua_name %s, ev %d, num %d, path %s", lua_name, event, num, path));
					plog_fmt("%s: %s", SDL_GetError(), strerror(errno));
					//puts(format("%s: %s", SDL_GetError(), strerror(errno)));//DEBUG USE_SOUND_2010
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
				/* Handle '!' indicator for 'initial' songs */
				if (cur_token[0] == '!') {
					cur_token++;
					initial = TRUE;
				} else initial = FALSE;
				/* Handle '+' indicator for 'referenced' music events */
				reference = FALSE;
				if (cur_token[0] == '+') {
					reference = TRUE;
					cur_token++;
				}
				/* Handle song names within quotes */
				if (cur_token[0] == '\"') {
					cur_token++;
					search = strchr(cur_token, '\"');
					if (search) {
						search[0] = '\0';
						search = strpbrk(search + 1, " \t");
					} else logprint(format("Music.cfg error: Missing closing quotes in line %d.\n", cur_line));
				} else {
					/* Try to find a space */
					search = strpbrk(cur_token, " \t");
				}
				/* If we can find one, terminate, and set new "next" */
				if (search) {
					search[0] = '\0';
					next_token = search + 1;
				} else {
					/* Otherwise prevent infinite looping */
					next_token = NULL;
				}

				/* Songs: Trim spaces/tabs */
				if (next_token) while (*next_token == ' ' || *next_token == '\t') next_token++;
			}
		}

		/* disable this song? */
		if (disabled) songs[event].disabled = TRUE;
	}

#ifndef WINDOWS //assume POSIX
#ifdef DEBUG_SOUND
	log_fd_usage();
#endif
#endif

#ifdef DEBUG_SOUND
	logprint(format("solving REFs: #%d\n", references));
#endif
	/* Solve all stored references now */
	for (i = 0; i < references; i++) {
		int num, event, event_ref, j;
		cptr lua_name;
		bool initial;

		/* Make sure this is a valid event name */
		for (event = MUSIC_MAX - 1; event >= 0; event--) {
			sprintf(out_val, "return get_music_name(%d)", event);
			lua_name = string_exec_lua(0, out_val);
			if (!strlen(lua_name)) continue;
			if (strcmp(referenced_event[i], lua_name) == 0) break;
		}
		if (event < 0) {
			logprint(format("Referenced music event '%s' not in audio.lua\n", referenced_event[i]));
			continue;
		}
		event_ref = event;

		/* Handle.. */
		event = referencer[i];
		initial = reference_initial[i];
		num = songs[event].num;
#ifdef DEBUG_SOUND
		logprint(format("adding REF #%d <%d> -> <%s> (ev = %d, initial = %d, songs %d)\n", i, event, referenced_event[i], event_ref, initial, num));
#endif

		for (j = 0; j < songs[event_ref].num; j++) {
			/* Don't allow too many songs */
			if (num >= MAX_SONGS) break;

			/* Never reference initial songs */
			if (songs[event_ref].initial[j]) continue;

			/* Avoid cross-referencing ourselves! */
			for (k = 0; k < songs[event].num; k++)
				if (streq(songs[event].paths[k], songs[event_ref].paths[j])) break;
			if (k != songs[event].num) {
				logprint(format("Music: Duplicate reference <%s> (%d,%d <-> %d,%d)\n", songs[event].paths[k], event, k, event_ref, j));
				continue;
			}

			songs[event].paths[num] = songs[event_ref].paths[j];
#if 0 /* these are all NULL here, as thread_load_audio() is only called _after_ we return */
			songs[event].wavs[num] = songs[event_ref].wavs[j];
#else /* so we need to remember the reference indices instead to avoid duplicate/multiple loading of the same files over and over, killing our files handle pool (1024)... */
			songs[event].referenced_event[num] = event_ref;
			songs[event].referenced_song[num] = j;
#endif
			songs[event].initial[num] = initial;
			songs[event].is_reference[num] = TRUE;
#ifdef DEBUG_SOUND
			logprint(format("  adding song #%d <%d> : <%s> (initial = %d)\n", event, num, songs[event].paths[num], initial));
#endif
			num++;
			songs[event].num = num;
			/* for do_cmd_options_...(): remember that this sample was mentioned in our config file */
			songs[event].config = TRUE;
		}
	}

	/* Close the file */
	my_fclose(fff);

#ifndef WINDOWS //assume POSIX
#ifdef DEBUG_SOUND
	log_fd_usage();
#endif
#endif

#ifdef WINDOWS
	/* On Windows we must have a second config file just to store disabled-state, since we cannot write to Program Files folder after Win XP anymore..
	   So if it exists, let it override the normal config file. */
	if (!win_dontmoveuser && getenv("HOMEDRIVE") && getenv("HOMEPATH"))
		sprintf(path, "%s\\TomeNET-%s-disabled.cfg", ANGBAND_DIR_USER, cfg_musicpackfolder);
	else path_build(path, sizeof(path), ANGBAND_DIR_XTRA_MUSIC, "TomeNET-nomusic.cfg"); //paranoia

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
	my_fclose(fff);
#endif
#ifdef WINDOWS
	if (!win_dontmoveuser && getenv("HOMEDRIVE") && getenv("HOMEPATH"))
		sprintf(path, "%s\\TomeNET-%s-volume.cfg", ANGBAND_DIR_USER, cfg_musicpackfolder);
	else
#endif
	path_build(path, sizeof(path), ANGBAND_DIR_XTRA_MUSIC, "TomeNET-musicvol.cfg"); //paranoia

	fff = my_fopen(path, "r");
	if (fff) {
		while (my_fgets(fff, buffer0, sizeof(buffer0)) == 0) {
			/* find out event state (disabled/enabled) */
			i = exec_lua(0, format("return get_music_index(\"%s\")", buffer0));
			if (my_fgets(fff, buffer0, sizeof(buffer0))) break; /* Error, incomplete entry */
			/* unknown (different game version/music pack?) */
			if (i == -1 || !songs[i].config) continue;
			/* set volume of songs listed in this file */
			songs[i].volume = atoi(buffer0);
		}
	}
	my_fclose(fff);

#ifdef DEBUG_SOUND
	puts("sound_sdl_init() done.");
#endif

	/* Success */
	return(TRUE);
}

/*
 * Play a sound of type "event". Returns FALSE if sound couldn't be played.
 */
static bool play_sound(int event, int type, int vol, s32b player_id, int dist_x, int dist_y) {
	Mix_Chunk *wave = NULL;
	int s, vols = 100;
	bool test = FALSE, real_event = (event >= 0 && event < SOUND_MAX_2010), allow_overlap = FALSE;

#ifdef DISABLE_MUTED_AUDIO
	if (!cfg_audio_master || !cfg_audio_sound) return(TRUE); /* claim that it 'succeeded' */
#endif

#ifdef USER_VOLUME_SFX
	/* Apply user-defined custom volume modifier */
	if (real_event && samples[event].volume) vols = samples[event].volume;
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
			return(found);
		}
		/* stop sound of this type? */
		else if (real_event) {
			for (s = 0; s < cfg_max_channels; s++) {
				if (channel_sample[s] == event && channel_player_id[s] == player_id) {
					Mix_FadeOutChannel(s, 450); //250..450 (more realistic timing vs smoother sound (avoid final 'spike'))
					return(TRUE);
				}
			}
			return(FALSE);
		}
	}

	/* Paranoia */
	if (!real_event) return(FALSE);

	if (type == SFX_TYPE_AMBIENT_LOCAL) {
		//todo
	}

	if (samples[event].disabled) return(TRUE); /* claim that it 'succeeded' */

	/* Check there are samples for this event */
	if (!samples[event].num) return(FALSE);

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
		allow_overlap = TRUE;
		break;
	case SFX_TYPE_OVERLAP: /* Same as SFX_TYPE_MISC, but never checks against overlapping */
		allow_overlap = TRUE;
		break;
	default:
		test = TRUE;
	}

	if (test) {
#if 0 /* old method before sounds could've come from other players nearby us, too */
		if (samples[event].current_channel != -1) return(TRUE);
#else /* so now we need to allow multiple samples, IF they stem from different sources aka players */
		for (s = 0; s < cfg_max_channels; s++) {
			if (channel_sample[s] == event && channel_player_id[s] == player_id) return(TRUE);
		}
#endif
	}

	/* prevent playing duplicate sfx that were initiated very closely
	   together in time, after one each other? (efficiency) */
	if (c_cfg.no_ovl_close_sfx && ticks == samples[event].started_timer_tick && !allow_overlap) return(TRUE);

	/* Choose a random event (normal gameplay), except when in the jukebox screen where we play everything sequentially  */
	if (jukebox_sfx_screen) {
		if (sound_cur != event) s = 0;
		else s = (sound_cur_wav + 1) % samples[event].num;
	} else s = rand_int(samples[event].num);
	wave = samples[event].wavs[s];
	sound_cur_wav = s; //just for jukebox wav index display
	sound_cur = event;

	/* Try loading it, if it's not cached */
	if (!wave) {
		if (on_demand_loading || no_cache_audio) {
			if (!(wave = load_sample(event, s))) {
				/* we really failed to load it */
				plog(format("SDL sound load failed (%d, %d) [1].", event, s));
				puts(format("SDL sound load failed (%d, %d) [1].", event, s));
				return(FALSE);
			}
		} else {
			/* Fail silently */
			return(TRUE);
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
			Mix_Volume(s, (CALC_MIX_VOLUME(cfg_audio_weather, (cfg_audio_weather_volume * vol) / 100, vols) * grid_weather_volume) / 100);
		else
		if (type == SFX_TYPE_AMBIENT)
			Mix_Volume(s, (CALC_MIX_VOLUME(cfg_audio_sound, (cfg_audio_sound_volume * vol) / 100, vols) * grid_ambient_volume) / 100);
		else
			/* Note: Linear scaling is used here to allow more precise control at the server end */
			Mix_Volume(s, CALC_MIX_VOLUME(cfg_audio_sound, (cfg_audio_sound_volume * vol) / 100, vols));

		/* Simple stereo-positioned audio, only along the x-coords. TODO: HRTF via OpenAL ^^ - C. Blue */
		if (c_cfg.positional_audio) {
#if 0
			/* Just for the heck of it: Trivial (bad) panning, ignoring any y-plane angle and just use basic left/right distance. */
			if (dist_x < 0) Mix_SetPanning(s, 255, (-255 * 4) / (dist_x - 3));
			else if (dist_x > 0) Mix_SetPanning(s, (255 * 4) / (dist_x + 3), 255);
			else Mix_SetPanning(s, 255, 255);
#else
			/* Best we can do with simple stereo for now without HRTF etc.:
			   We don't have a y-audio-plane (aka z-plane really),
			   but at least we pan according to the correct angle. - C. Blue */

			int dy = ABS(dist_y); //we don't differentiate between in front of us / behind us, need HRTF for that.

			/* Hack: Catch edge case: At dist 20 (MAX_SIGHT) there is a +/- 1 leeway orthogonally
			   due to the way distance() works. This is fine, but we only define roots up to 370 for practical reasons.
			   For this tiny angle we can just assume that we receive the same panning as if we stood slightly closer. */
			if (dist_y * dist_y + dist_x * dist_x > 370) {
				if (dy == MAX_SIGHT) dist_x = 0;
				else dist_y = 0;
			}

			if (!dist_x) Mix_SetPanning(s, 255, 255); //shortcut for both, 90 deg and undefined angle aka 'on us'. */
			else if (!dist_y) { //shortcut for 0 deg and 180 deg (ie sin = 0)
				if (dist_x < 0) Mix_SetPanning(s, 255, 0);
				else Mix_SetPanning(s, 0, 255);
			} else { //all other cases (ie sin != 0) -- and d_real cannot be 0 (for division!)
				int pan_l, pan_r;
				int d_real = fast_sqrt[dist_x * dist_x + dist_y * dist_y]; //wow, for once not just an approximated distance (beyond that integer thingy) ^^
				int sin = (10 * dy) / d_real; //sinus (scaled by *10 for accuracy)

				/* Calculate left/right panning weight from angle:
				   The ear with 'los' toward the event gets 100% of the distance-reduced volume (d),
				   while the other ear gets ABS(sin(a)) * d. */
				if (dist_x < 0) { /* somewhere to our left */
					pan_l = 255;
					pan_r = (255 * sin) / 10;
				} else { /* somewhere to our right */
					pan_l = (255 * sin) / 10;
					pan_r = 255;
				}
				Mix_SetPanning(s, pan_l, pan_r);
			}
#endif
		}
	}
	samples[event].current_channel = s;
	samples[event].started_timer_tick = ticks;

	return(TRUE);
}
/* play the 'bell' sound */
#define BELL_REDUCTION 3 /* reduce volume of bell() sounds by this factor */
extern bool sound_bell(void) {
	Mix_Chunk *wave = NULL;
	int s, vols = 100;

	if (bell_sound_idx == -1) return(FALSE);
	if (samples[bell_sound_idx].disabled) return(TRUE); /* claim that it 'succeeded' */
	if (!samples[bell_sound_idx].num) return(FALSE);

	/* already playing? prevent multiple sounds of the same kind from being mixed simultaneously, for preventing silliness */
	if (samples[bell_sound_idx].current_channel != -1) return(TRUE);

	/* Choose a random event */
	s = rand_int(samples[bell_sound_idx].num);
	wave = samples[bell_sound_idx].wavs[s];

#ifdef USER_VOLUME_SFX
	/* Apply user-defined custom volume modifier */
	if (samples[bell_sound_idx].volume) vols = samples[bell_sound_idx].volume;
#endif

	/* Try loading it, if it's not cached */
	if (!wave) {
		if (on_demand_loading || no_cache_audio) {
			if (!(wave = load_sample(bell_sound_idx, s))) {
				/* we really failed to load it */
				plog(format("SDL sound load failed (%d, %d) [2].", bell_sound_idx, s));
				puts(format("SDL sound load failed (%d, %d) [2].", bell_sound_idx, s));
				return(FALSE);
			}
		} else {
			/* Fail silently */
			return(TRUE);
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
			Mix_Volume(s, CALC_MIX_VOLUME(1, MIX_MAX_VOLUME / BELL_REDUCTION, vols));
		}
	}
	samples[bell_sound_idx].current_channel = s;

	return(TRUE);
}
/* play the 'page' sound */
extern bool sound_page(void) {
	Mix_Chunk *wave = NULL;
	int s, vols = 100;

	if (page_sound_idx == -1) return(FALSE);
	if (samples[page_sound_idx].disabled) return(TRUE); /* claim that it 'succeeded' */
	if (!samples[page_sound_idx].num) return(FALSE);

	/* already playing? prevent multiple sounds of the same kind from being mixed simultaneously, for preventing silliness */
	if (samples[page_sound_idx].current_channel != -1) return(TRUE);

	/* Choose a random event */
	s = rand_int(samples[page_sound_idx].num);
	wave = samples[page_sound_idx].wavs[s];
	sound_cur_wav = s; //just for jukebox wav index display
	sound_cur = page_sound_idx;

#ifdef USER_VOLUME_SFX
	/* Apply user-defined custom volume modifier */
	if (samples[page_sound_idx].volume) vols = samples[page_sound_idx].volume;
#endif

	/* Try loading it, if it's not cached */
	if (!wave) {
		if (on_demand_loading || no_cache_audio) {
			if (!(wave = load_sample(page_sound_idx, s))) {
				/* we really failed to load it */
				plog(format("SDL sound load failed (%d, %d) [3].", page_sound_idx, s));
				puts(format("SDL sound load failed (%d, %d) [3].", page_sound_idx, s));
				return(FALSE);
			}
		} else {
			/* Fail silently */
			return(TRUE);
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
			Mix_Volume(s, CALC_MIX_VOLUME(1, MIX_MAX_VOLUME, vols));
		}
	}
	samples[page_sound_idx].current_channel = s;

	return(TRUE);
}
/* play the 'warning' sound */
extern bool sound_warning(void) {
	Mix_Chunk *wave = NULL;
	int s, vols = 100;

	if (warning_sound_idx == -1) return(FALSE);
	if (samples[warning_sound_idx].disabled) return(TRUE); /* claim that it 'succeeded' */
	if (!samples[warning_sound_idx].num) return(FALSE);

	/* already playing? prevent multiple sounds of the same kind from being mixed simultaneously, for preventing silliness */
	if (samples[warning_sound_idx].current_channel != -1) return(TRUE);

	/* Choose a random event */
	s = rand_int(samples[warning_sound_idx].num);
	wave = samples[warning_sound_idx].wavs[s];
	sound_cur_wav = s; //just for jukebox wav index display
	sound_cur = warning_sound_idx;

#ifdef USER_VOLUME_SFX
	/* Apply user-defined custom volume modifier */
	if (samples[warning_sound_idx].volume) vols = samples[warning_sound_idx].volume;
#endif

	/* Try loading it, if it's not cached */
	if (!wave) {
		if (on_demand_loading || no_cache_audio) {
			if (!(wave = load_sample(warning_sound_idx, s))) {
				/* we really failed to load it */
				plog(format("SDL sound load failed (%d, %d) [4].", warning_sound_idx, s));
				puts(format("SDL sound load failed (%d, %d) [4].", warning_sound_idx, s));
				return(FALSE);
			}
		} else {
			/* Fail silently */
			return(TRUE);
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
			Mix_Volume(s, CALC_MIX_VOLUME(1, MIX_MAX_VOLUME, vols));
		}
	}
	samples[warning_sound_idx].current_channel = s;

	return(TRUE);
}


/* Release weather channel after fading out has been completed */
static void clear_channel(int c) {
	/* weather has faded out, mark it as gone */
	if (c == weather_channel) {
		weather_channel = -1;
		return;
	}

	if (c == ambient_channel) {
		/* Note (1/2): -fsanitize=threads gives a warning here that setting this to -1 may collide with it being checked for -1 in ambient_handle_fading(), compare there.
		   However, first, this warning doesn't seem to make sense.
		   Second, there is a years old SDL bug apparently, that has Mix_HasFinished() return 0 after it was already faded out, if looping is true. */
		ambient_channel = -1;
		return;
	}

	/* a sample has finished playing, so allow this kind to be played again */
	/* hack: if the sample was the 'paging' sound, reset the channel's volume to be on the safe side */
	if (channel_sample[c] == page_sound_idx || channel_sample[c] == warning_sound_idx)
		Mix_Volume(c, CALC_MIX_VOLUME(cfg_audio_sound, cfg_audio_sound_volume, 100));

	/* HACK - if the sample was a weather sample, which would be thunder, reset the vol too, paranoia */
	if (channel_type[c] == SFX_TYPE_WEATHER)
		Mix_Volume(c, CALC_MIX_VOLUME(cfg_audio_sound, cfg_audio_sound_volume, 100));

	if (channel_sample[c] == -1) return;
	samples[channel_sample[c]].current_channel = -1;
	channel_sample[c] = -1;
}

/* Overlay a weather noise -- not WEATHER_VOL_PARTICLES or WEATHER_VOL_CLOUDS */
static void play_sound_weather(int event) {
	Mix_Chunk *wave = NULL;
	int s, new_wc, vols = 100;

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
	sound_cur_wav = s; //just for jukebox wav index display
	sound_cur = event;

#ifdef USER_VOLUME_SFX
	/* Apply user-defined custom volume modifier */
	if (samples[event].volume) vols = samples[event].volume;
#endif

	/* Try loading it, if it's not cached */
	if (!wave) {
		if (on_demand_loading || no_cache_audio) {
			if (!(wave = load_sample(event, s))) {
				/* we really failed to load it */
				plog(format("SDL sound load failed (%d, %d) [5].", event, s));
				puts(format("SDL sound load failed (%d, %d) [5].", event, s));
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
		Mix_Volume(new_wc, (CALC_MIX_VOLUME(cfg_audio_weather, cfg_audio_weather_volume, vols) * grid_weather_volume) / 100);

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
		Mix_Volume(weather_channel, (CALC_MIX_VOLUME(cfg_audio_weather, cfg_audio_weather_volume, vols) * grid_weather_volume) / 100);
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
				Mix_Volume(weather_channel, (CALC_MIX_VOLUME(cfg_audio_weather, cfg_audio_weather_volume, vols) * grid_weather_volume) / 100);
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
		Mix_Volume(weather_channel, (CALC_MIX_VOLUME(cfg_audio_weather, cfg_audio_weather_volume, vols) * grid_weather_volume) / 100);
	}
#endif

#ifdef DEBUG_SOUND
	puts(format("now: %d, oev: %d, ev: %d", weather_channel, event, weather_current));
#endif
}


/* Overlay a weather noise, with a certain volume -- WEATHER_VOL_PARTICLES */
static void play_sound_weather_vol(int event, int vol) {
	Mix_Chunk *wave = NULL;
	int s, new_wc, vols = 100;

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

#ifdef USER_VOLUME_SFX
		/* Apply user-defined custom volume modifier */
		if (samples[event].volume) vols = samples[event].volume;
#endif

//c_message_add(format("smooth %d", weather_vol_smooth));
		Mix_Volume(weather_channel, (CALC_MIX_VOLUME(cfg_audio_weather, weather_vol_smooth, vols) * grid_weather_volume) / 100);

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

#ifdef USER_VOLUME_SFX
	/* Apply user-defined custom volume modifier */
	if (samples[event].volume) vols = samples[event].volume;
#endif

	/* Choose a random event */
	s = rand_int(samples[event].num);
	wave = samples[event].wavs[s];
	sound_cur_wav = s; //just for jukebox wav index display
	sound_cur = event;

	/* Try loading it, if it's not cached */
	if (!wave) {
		if (on_demand_loading || no_cache_audio) {
			if (!(wave = load_sample(event, s))) {
				/* we really failed to load it */
				plog(format("SDL sound load failed (%d, %d) [6].", event, s));
				puts(format("SDL sound load failed (%d, %d) [6].", event, s));
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
		Mix_Volume(new_wc, (CALC_MIX_VOLUME(cfg_audio_weather, weather_vol_smooth, vols) * grid_weather_volume) / 100);

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
		Mix_Volume(weather_channel, (CALC_MIX_VOLUME(cfg_audio_weather, weather_vol_smooth, vols) * grid_weather_volume) / 100);
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
				Mix_Volume(weather_channel, (CALC_MIX_VOLUME(cfg_audio_weather, weather_vol_smooth, vols) * grid_weather_volume) / 100);
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
		Mix_Volume(weather_channel, (CALC_MIX_VOLUME(cfg_audio_weather, (cfg_audio_weather_volume * vol) / 100, vols) * grid_weather_volume) / 100);
	}
#endif

#ifdef DEBUG_SOUND
	puts(format("now: %d, oev: %d, ev: %d", weather_channel, event, weather_current));
#endif
}

/* make sure volume is set correct after fading-in has been completed (might be just paranoia) */
void weather_handle_fading(void) {
	int vols = 100;

	if (weather_channel == -1) { //paranoia
		weather_fading = 0;
		return;
	}

#ifdef USER_VOLUME_SFX
	/* Apply user-defined custom volume modifier */
	if (weather_current != -1 && samples[weather_current].volume) vols = samples[weather_current].volume;
#endif

	if (Mix_FadingChannel(weather_channel) == MIX_NO_FADING) {
#ifndef WEATHER_VOL_PARTICLES
		Mix_Volume(weather_channel, (CALC_MIX_VOLUME(cfg_audio_weather, cfg_audio_weather_volume, vols) * grid_weather_volume) / 100);
#else
		Mix_Volume(weather_channel, (CALC_MIX_VOLUME(cfg_audio_weather, weather_vol_smooth, vols) * grid_weather_volume) / 100);
#endif
		weather_fading = 0;
		return;
	}
}

/* Overlay a global, looping, ambient sound effect */
static void play_sound_ambient(int event) {
	Mix_Chunk *wave = NULL;
	int s, new_ac, vols = 100;

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
	sound_cur_wav = s; //just for jukebox wav index display
	sound_cur = event;

#ifdef USER_VOLUME_SFX
	/* Apply user-defined custom volume modifier */
	if (samples[event].volume) vols = samples[event].volume;
#endif

	/* Try loading it, if it's not cached */
	if (!wave) {
#if 0 /* for ambient sounds. we don't drop them as we'd do for "normal " sfx, \
	 because they ought to be looped, so we'd completely lose them. - C. Blue */
		if (on_demand_loading || no_cache_audio) {
#endif
			if (!(wave = load_sample(event, s))) {
				/* we really failed to load it */
				plog(format("SDL sound load failed (%d, %d) [7].", event, s));
				puts(format("SDL sound load failed (%d, %d) [7].", event, s));
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
		Mix_Volume(new_ac, (CALC_MIX_VOLUME(cfg_audio_sound, cfg_audio_sound_volume, vols) * grid_ambient_volume) / 100);

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
		Mix_Volume(ambient_channel, (CALC_MIX_VOLUME(cfg_audio_sound, cfg_audio_sound_volume, vols) * grid_ambient_volume) / 100);
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
				Mix_Volume(ambient_channel, (CALC_MIX_VOLUME(cfg_audio_sound, cfg_audio_sound_volume, vols) * grid_ambient_volume) / 100);
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
		Mix_Volume(ambient_channel, (CALC_MIX_VOLUME(cfg_audio_sound, cfg_audio_sound_volume, vols) * grid_ambient_volume) / 100);
	}
#endif

#ifdef DEBUG_SOUND
	puts(format("now: %d, oev: %d, ev: %d", ambient_channel, event, ambient_current));
#endif
}

/* make sure volume is set correct after fading-in has been completed (might be just paranoia) */
void ambient_handle_fading(void) {
	int vols = 100;

	/* Note (2/2): -fsanitize=threads gives a warning here that checking this for -1 may collide with it being set to -1 in clear_channel(), compare there.
	   However, first, this warning doesn't seem to make sense.
	   Second, there is a years old SDL bug apparently, that has Mix_HasFinished() return 0 after it was already faded out, if looping is true. */
	if (ambient_channel == -1) { //paranoia
		ambient_fading = 0;
		return;
	}

#ifdef USER_VOLUME_SFX
	/* Apply user-defined custom volume modifier */
	if (ambient_current != -1 && samples[ambient_current].volume) vols = samples[ambient_current].volume;
#endif

	if (Mix_FadingChannel(ambient_channel) == MIX_NO_FADING) {
		Mix_Volume(ambient_channel, (CALC_MIX_VOLUME(cfg_audio_sound, cfg_audio_sound_volume, vols) * grid_ambient_volume) / 100);
		ambient_fading = 0;
		return;
	}
}

#ifdef ENABLE_JUKEBOX
/* Hack: Find out song length of currently active music event song by trial and error o_O */
void jukebox_update_songlength(void) {
	int i, lb, l;
	double p; //ohoho -_-

	if (music_cur == -1 || music_cur_song == -1) {
		curmus_song_dur = 0;
		return; //paranoia
	}

	p = Mix_GetMusicPosition(songs[music_cur].wavs[music_cur_song]);
	//Mix_RewindMusic();
	lb = 0;
	l = (99 * 60 + 59) * 2; //asume 99:59 min is max duration of any song
	while (l > 1) {
		l >>= 1;
		Mix_SetMusicPosition(lb + l);

		/* Check for overflow beyond actual song length */
		i = (int)Mix_GetMusicPosition(songs[music_cur].wavs[music_cur_song]);
		/* Too far? */
		if (!i) continue;

		/* We found a minimum duration */
		lb = i;
	}
	/* Reset position */
	Mix_SetMusicPosition(p);

	curmus_song_dur = lb;
}
#endif

/*
 * Play a music of type "event".
 * Hack: If 10000 is added to 'event', the music is played instantly without fade-in effect.
 *       This is only used on client-side for the meta server ist 'meta' music event.
 */
static bool play_music(int event) {
	int n, initials = 0, vols = 100;

	if (event > 9000) return(play_music_instantly(event - 10000)); //scouter (just in case: carry over all event-hax, which might be negative numbers, so we end up <10000 even with +10000 hack specified)

	/* Paranoia */
	if (event < -4 || event >= MUSIC_MAX) return(FALSE);

	/* Don't play anything, just return "success", aka just keep playing what is currently playing.
	   This is used for when the server sends music that doesn't have an alternative option, but
	   should not stop the current music if it fails to play. */
	if (event == -1) return(TRUE);

#ifdef ENABLE_JUKEBOX
	/* Jukebox hack: Don't interrupt current jukebox song, but remember event for later */
	if (jukebox_playing != -1) {
		/* Check there are samples for this event, otherwise don't switch (unless it's a 'stop music' event sort of thing) */
		if (event < 0 || songs[event].num) jukebox_org = (event < 0 ? -1 : event);
		/* Special hack for ghost music (4.7.4b+), see handle_music() in util.c */
		if (event == 89 && songs[event].num && is_atleast(&server_version, 4, 7, 4, 2, 0, 0)) skip_received_music = TRUE;
		return(event < 0 || songs[event].num != 0);
	} else if (jukebox_screen) {
		/* Still update jukebox_org et al, as they WILL be restored even if jukebox wasn't playing at all (could be prevented for efficiency I guess, pft) */
		/* Check there are samples for this event, otherwise don't remember (unless it's a 'stop music' event sort of thing) */
		if (event < 0 || songs[event].num) jukebox_org = (event < 0 ? -1 : event);
	}
#endif

	/* We previously failed to play both music and alternative music.
	   Stop currently playing music before returning */
	if (event == -2) {
		music_next = -1;
		if (Mix_PlayingMusic() && Mix_FadingMusic() != MIX_FADING_OUT)
			Mix_FadeOutMusic(500);
		music_cur = -1;
		return(TRUE); //whatever..
	}

	/* 'shuffle_music' or 'play_all' option changed? */
	if (event == -3) {
		if (c_cfg.shuffle_music) {
			music_next = -1;
			Mix_FadeOutMusic(500);
		} else if (c_cfg.play_all) {
			music_next = -1;
			Mix_FadeOutMusic(500);
		} else {
#if 0 /* wrong? */
			music_next = music_cur; //hack
			music_next_song = music_cur_song;
#else
			music_next = -1; //nothing
#endif
		}
		return(TRUE); //whatever..
	}

#ifdef ATMOSPHERIC_INTRO
	/* New, for title screen -> character screen switch: Halt current music */
	if (event == -4) {
		/* Stop currently playing music though, before returning */
		music_next = -1;
		if (Mix_PlayingMusic() && Mix_FadingMusic() != MIX_FADING_OUT)
			Mix_FadeOutMusic(2000);
		music_cur = -1;
		return(TRUE); /* claim that it 'succeeded' */
	}
#endif

	/* Check there are samples for this event */
	if (!songs[event].num) return(FALSE);

	/* Special hack for ghost music (4.7.4b+), see handle_music() in util.c */
	if (event == 89 && is_atleast(&server_version, 4, 7, 4, 2, 0, 0)) skip_received_music = TRUE;

#ifdef USER_VOLUME_MUS
	/* Apply user-defined custom volume modifier */
	if (songs[event].volume) vols = songs[event].volume;
#endif

	/* In case the current music was played via play_music_vol() at reduced volume, restore to normal */
	if (music_vol != 100) {
		music_vol = 100;
		Mix_VolumeMusic(CALC_MIX_VOLUME(cfg_audio_music, cfg_audio_music_volume, vols));
	}
	jukebox_org_vol = music_vol;

	/* if music event is the same as is already running, don't do anything */
	if (music_cur == event && Mix_PlayingMusic() && Mix_FadingMusic() != MIX_FADING_OUT) return(TRUE); //pretend we played it

	music_next = event;
	/* handle 'initial' songs with priority */
	for (n = 0; n < songs[music_next].num; n++) if (songs[music_next].initial[n]) initials++;
	/* no initial songs - just pick a song normally */
	if (!initials) {
		if (!c_cfg.first_song)
			/* Pick one song randomly */
			music_next_song = rand_int(songs[music_next].num);
		else
			/* Start with the first song of the music.cfg entry */
			music_next_song = 0;
	}
	/* pick an initial song first */
	else {
		if (!c_cfg.first_song) {
			/* Pick an initial song randomly */
			initials = randint(initials);
			for (n = 0; n < songs[music_next].num; n++) {
				if (!songs[music_next].initial[n]) continue;
				initials--;
				if (initials) continue;
				music_next_song = n;
				break;
			}
		} else
			/* Start with the first initial song of the music.cfg entry */
			for (n = 0; n < songs[music_next].num; n++) {
				if (!songs[music_next].initial[n]) continue;
				music_next_song = 0;
				break;
			}
	}

	/* new: if upcoming music _file_ (not music event!) happens to be the same as the currently playing one, don't do anything */
	if (music_cur != -1 && music_cur_song != -1 &&
	    !songs[music_next].disabled && songs[music_next].num /* Check there are samples for this event */
	    && !strcmp(songs[music_next].paths[music_next_song], songs[music_cur].paths[music_cur_song])) {
		/* 'silent transition', ie we don't need to fade in the next song, as we're already playing the very same file, although it was from a different music event! */
		music_cur = (event < 0 ? -1 : event);
		music_cur_song = music_next_song;
		music_next = -1;
		return(TRUE);
	}

	/* check if some other music is already running, if so, fade it out first! */
	if (Mix_PlayingMusic()) {
		if (Mix_FadingMusic() != MIX_FADING_OUT) {
#ifdef WILDERNESS_MUSIC_RESUME
			cptr mc = string_exec_lua(0, format("return get_music_name(%d)", music_cur));

			/* Special: If current music is in category 'wilderness', remember its position to resume it instead of restarting it, next time it happens to play */
			if (prefix(mc, "wilderness_")
 #ifdef TOWN_TAVERN_MUSIC_RESUME_TOO
			    || (c_cfg.tavern_town_resume && (
			    prefix(mc, "town_") || prefix(mc, "tavern_") ||
			    prefix(mc, "Bree") || prefix(mc, "Gondolin") || prefix(mc, "MinasAnor") || prefix(mc, "Lothlorien") || prefix(mc, "Khazaddum") ||
			    prefix(mc, "Menegroth") || prefix(mc, "Nargothrond")))
 #endif
			    ) {
				songs[music_cur].bak_song = music_cur_song;
				songs[music_cur].bak_pos = Mix_GetMusicPosition(songs[music_cur].wavs[music_cur_song]) * 1000 + 500; /* pre-add the fading-out time span (in ms) */
			}
#endif
			Mix_FadeOutMusic(500);
		}
	} else {
		//play immediately
		fadein_next_music();
	}
	return(TRUE);
}
static bool play_music_vol(int event, char vol) {
	int n, initials = 0, vols = 100;

	/* Paranoia */
	if (event < -4 || event >= MUSIC_MAX) return(FALSE);

	/* Don't play anything, just return "success", aka just keep playing what is currently playing.
	   This is used for when the server sends music that doesn't have an alternative option, but
	   should not stop the current music if it fails to play. */
	if (event == -1) return(TRUE);

#ifdef ENABLE_JUKEBOX
	/* Jukebox hack: Don't interrupt current jukebox song, but remember event for later */
	if (jukebox_playing != -1) {
		/* Check there are samples for this event, otherwise don't switch (unless it's a 'stop music' event sort of thing) */
		if (event < 0 || songs[event].num) jukebox_org = (event < 0 ? -1 : event);
		/* Special hack for ghost music (4.7.4b+), see handle_music() in util.c */
		if (event == 89 && songs[event].num && is_atleast(&server_version, 4, 7, 4, 2, 0, 0)) skip_received_music = TRUE;
		return(event < 0 || songs[event].num != 0);
	} else if (jukebox_screen) {
		/* Still update jukebox_org et al, as they WILL be restored even if jukebox wasn't playing at all (could be prevented for efficiency I guess, pft) */
		/* Check there are samples for this event, otherwise don't remember (unless it's a 'stop music' event sort of thing) */
		if (event < 0 || songs[event].num) jukebox_org = (event < 0 ? -1 : event);
	}
#endif

	/* We previously failed to play both music and alternative music.
	   Stop currently playing music before returning */
	if (event == -2) {
		music_next = -1;
		if (Mix_PlayingMusic() && Mix_FadingMusic() != MIX_FADING_OUT)
			Mix_FadeOutMusic(500);
		music_cur = -1;
		return(TRUE); //whatever..
	}

	/* 'shuffle_music' or 'play_all' option changed? */
	if (event == -3) {
		if (c_cfg.shuffle_music) {
			music_next = -1;
			Mix_FadeOutMusic(500);
		} else if (c_cfg.play_all) {
			music_next = -1;
			Mix_FadeOutMusic(500);
		} else {
			music_next = music_cur; //hack
			music_next_song = music_cur_song;
		}
		return(TRUE); //whatever..
	}

#ifdef ATMOSPHERIC_INTRO
	/* New, for title screen -> character screen switch: Halt current music */
	if (event == -4) {
		/* Stop currently playing music though, before returning */
		music_next = -1;
		if (Mix_PlayingMusic() && Mix_FadingMusic() != MIX_FADING_OUT)
			Mix_FadeOutMusic(2000);
		music_cur = -1;
		return(TRUE); /* claim that it 'succeeded' */
	}
#endif

	/* Check there are samples for this event */
	if (!songs[event].num) return(FALSE);

	/* Special hack for ghost music (4.7.4b+), see handle_music() in util.c */
	if (event == 89 && is_atleast(&server_version, 4, 7, 4, 2, 0, 0)) skip_received_music = TRUE;

#ifdef USER_VOLUME_MUS
	/* Apply user-defined custom volume modifier */
	if (songs[event].volume) vols = songs[event].volume;
#endif

	jukebox_org_vol = vol;
	/* Just change volume if requested */
	if (music_vol != vol) Mix_VolumeMusic(CALC_MIX_VOLUME(cfg_audio_music, (cfg_audio_music_volume * evlt[(int)vol]) / MIX_MAX_VOLUME, vols));
	music_vol = vol;

	/* if music event is the same as is already running, don't do anything */
	if (music_cur == event && Mix_PlayingMusic() && Mix_FadingMusic() != MIX_FADING_OUT) return(TRUE); //pretend we played it

	music_next = event;
	/* handle 'initial' songs with priority */
	for (n = 0; n < songs[music_next].num; n++) if (songs[music_next].initial[n]) initials++;
	/* no initial songs - just pick a song normally */
	if (!initials) {
		if (!c_cfg.first_song)
			/* Pick one song randomly */
			music_next_song = rand_int(songs[music_next].num);
		else
			/* Start with the first song of the music.cfg entry */
			music_next_song = 0;
	}
	/* pick an initial song first */
	else {
		if (!c_cfg.first_song) {
			/* Pick an initial song randomly */
			initials = randint(initials);
			for (n = 0; n < songs[music_next].num; n++) {
				if (!songs[music_next].initial[n]) continue;
				initials--;
				if (initials) continue;
				music_next_song = n;
				break;
			}
		} else
			/* Start with the first initial song of the music.cfg entry */
			for (n = 0; n < songs[music_next].num; n++) {
				if (!songs[music_next].initial[n]) continue;
				music_next_song = 0;
				break;
			}
	}

	/* new: if upcoming music file is same as currently playing one, don't do anything */
	if (music_cur != -1 && music_cur_song != -1 &&
	    !songs[music_next].disabled && songs[music_next].num /* Check there are samples for this event */
	    && !strcmp(songs[music_next].paths[music_next_song], songs[music_cur].paths[music_cur_song])) {
		/* 'silent transition', ie we don't need to fade in the next song, as we're already playing the very same file, although it was from a different music event! */
		music_cur = (event < 0 ? -1 : event);
		music_cur_song = music_next_song;
		music_next = -1;
		return(TRUE);
	}

	/* check if music is already running, if so, fade it out first! */
	if (Mix_PlayingMusic()) {
		if (Mix_FadingMusic() != MIX_FADING_OUT)
			Mix_FadeOutMusic(500);
	} else {
		//play immediately
		fadein_next_music();
	}
	return(TRUE);
}

static void fadein_next_music(void) {
	Mix_Music *wave = NULL;
#ifdef WILDERNESS_MUSIC_RESUME
	bool prev_wilderness;
	cptr pmn, mn;
#endif

#ifdef DISABLE_MUTED_AUDIO
	if (!cfg_audio_master || !cfg_audio_music) return;
#endif

	/* Ultra-hack: In jukebox and 'play all' */
	if (jukebox_screen && jukebox_play_all) {
		int j, d = music_cur;
		bool look_for_next = TRUE; //required for handling shuffle

		/* After having played all subsongs, advance to the next music event, starting at song #0 again */
		if (music_cur != -1 && music_cur_song == songs[music_cur].num - 1) {
			for (j = 0; j < MUSIC_MAX; j++) {
				d = jukebox_shuffle_map[j];
				if (look_for_next) {
					if (d != music_cur) continue; //skip all songs we've heard so far
					look_for_next = FALSE;
					continue;
				}
				if (!songs[d].config || songs[d].disabled) continue;
				break;
			}
			if (j == MUSIC_MAX) {
				jukebox_play_all_done = TRUE; //refresh the song list a last time

				jukebox_playing = -1;
				jukebox_static200vol = FALSE;
				jukebox_play_all = FALSE;

				//pseudo-turn off music to indicate that our play-all 'playlist' has finished
				cfg_audio_music = FALSE;
				set_mixing();

				d = jukebox_org;
			} else jukebox_playing = d;
			music_cur = -1; /* Prevent auto-advancing of play_music_instantly(), but freshly start at subsong #0 */
		}
		/* Note that this will auto-advance the subsong if j is already == jukebox_playing: */
		play_music_instantly(d);
		if (jukebox_static200vol) Mix_VolumeMusic(CALC_MIX_VOLUME(cfg_audio_music, cfg_audio_music_volume, 200));

		jukebox_update_songlength();
 #if 0 /* paranoia/not needed */
		/* Reset position */
		Mix_SetMusicPosition(0);
 #endif
		curmus_timepos = 0; //song starts to play, at 0 seconds mark ie the beginning
		return;
	}

	/* Not while we're using play_music_instantly() from within the jukebox, that function already calls Mix_PlayMusic().
	   So we'd needlessly override that with a new Mix_FadeInMusic() here, and also cause two bugs in song sequence,
	   as we'd (1) skip initial songs after first play and (2) try to randomize the song number while play_music_instantly()
	   already set a new song number. Eg for events with 1 initial and 2 normal songs this means the randomizer would avoid
	   the initial song, then pick the one of the other two we're currently NOT playing, but play_music_instantly() would
	   increment that number (music_cur_song) _again_, leaving it unchanging at always the same song. - C. Blue */
	if (jukebox_screen && jukebox_playing != -1) return;
	if (jukebox_screen) jukebox_gamemusicchanged = TRUE; //update jukebox UI's 'current song' (selector etc)

	/* Catch music_next == -1, this can now happen with shuffle_music or play_all option, since songs are no longer looped if it's enabled */
	if (
	    //(c_cfg.shuffle_music || c_cfg.play_all) && //---actually 'initial'-tagged songs will still only play once, so we'd get just silence afterwards if we didn't pick another song here in those cases...
	    music_next == -1) {
		int tries, mcs;
		int n, noinit_map[MAX_SONGS], ni = 0;

		/* We're not currently playing any music? -- This can happen when we quickly exit the game and return to music-less account screen
		   and would cause a segfault when trying to access songs[-1]: */
		if (music_cur == -1) return;

		/* Catch disabled songs */
		if (songs[music_cur].disabled) return;

		if (songs[music_cur].num < 1) return; //paranoia

		/* don't sequence-shuffle 'initial' songs */
		for (n = 0; n < songs[music_cur].num; n++) {
			if (songs[music_cur].initial[n]) continue;
			noinit_map[ni] = n;
			ni++;
		}
		/* no non-initial songs found? Don't play any subsequent music then. */
		if (!ni) return;

		if (c_cfg.shuffle_music) {
			/* stick with music event, but play different song, randomly */
			tries = songs[music_cur].num == 1 ? 1 : 100;
			mcs = music_cur_song;
			while (tries--) {
				mcs = noinit_map[rand_int(ni)];
				if (music_cur_song != mcs) break; //find some other song than then one we've just played, or it's not really 'shuffle'..
			}
		} else {
			/* MAYBE c_cfg.play_all, maybe not^^ (if prev song was 'initial' type, we're here even w/o play_all): */
			/* stick with music event, but play different song, in listed order */
			tries = -1;
			mcs = music_cur_song;
			while (++tries < ni) {
				mcs = noinit_map[tries];
				if (mcs <= music_cur_song) continue;
				break;
			}
			/* We already went through all (non-initial) songs? Start anew then: */
			if (tries == ni) mcs = noinit_map[0];
		}
		music_cur_song = mcs;

		/* Choose the predetermined random event */
		wave = songs[music_cur].wavs[music_cur_song];

		/* Try loading it, if it's not cached */
		if (!wave && !(wave = load_song(music_cur, music_cur_song))) {
			/* we really failed to load it */
			plog(format("SDL music load failed (%d, %d) [1].", music_cur, music_cur_song));
			puts(format("SDL music load failed (%d, %d) [1].", music_cur, music_cur_song));
			return;
		}

		/* Actually play the thing */
		music_cur_repeat = (c_cfg.shuffle_music || c_cfg.play_all ? 0 : -1);
		/* If we only have 1 subsong, ie we cannot shuffle/playall among different songs,
		   set repeat to 'forever' to avoid a new fade-in-sequence each time the song ends and restarts */
		if (songs[music_cur].num == 1) music_cur_repeat = -1;
		Mix_FadeInMusic(wave, music_cur_repeat, 1000); //-1 infinite, 0 once, or n times
#ifdef ENABLE_JUKEBOX
		if (jukebox_screen) jukebox_update_songlength();
#endif
		return;
	}

	/* Paranoia */
	if (music_next < 0 || music_next >= MUSIC_MAX) return;

	/* Music event was disabled? (local audio options) */
	if (songs[music_next].disabled) {
		music_cur = music_next;
		music_cur_song = music_next_song;
		music_next = -1; //pretend we play it
		return;
	}

	/* Check there are samples for this event */
	if (songs[music_next].num < music_next_song + 1) return;

#ifdef WILDERNESS_MUSIC_RESUME
	/* Special: If new music is in category 'wilderness', restore its position to resume it instead of restarting it.
	   However, only do this if the previous music was actually in 'wilderness' too! (except if WILDERNESS_MUSIC_ALWAYS_RESUME)
	   Part 1/2: Restore the song subnumber: */
	if (music_cur != -1) {
		bool day_night_changed = FALSE;

		pmn = string_exec_lua(0, format("return get_music_name(%d)", music_cur));
 //#ifndef WILDERNESS_MUSIC_ALWAYS_RESUME --- changed to client options, so unhardcoding this line by commenting it out
		if (!c_cfg.wild_resume_from_any)
			prev_wilderness = prefix(pmn, "wilderness_");
 //#else /* Always resume (as if previous music was wilderness music ie is eligible for resuming) - except on day/night change still */
		else
			prev_wilderness = TRUE;
 //#endif

		mn = string_exec_lua(0, format("return get_music_name(%d)", music_next));
		/* On day/night change, do not resume. Instead, reset all saved positions */
		/* We detect day/night change by checking if either the current or the next song has '_night' in its name,
		   and if so, we compare the two names without the '_night' and any optional '_day' part:
		   If the name is identical, then we just had a day/night change happen! (eg 'Bree' vs 'Bree_night'.) */
		if (suffix(pmn, "_night") != suffix(mn, "_night")) {
			char tmpname1[MAX_CHARS], tmpname2[MAX_CHARS], *c;

			strcpy(tmpname1, pmn);
			strcpy(tmpname2, mn);
			if ((c = strstr(tmpname1, "_night")) && c == tmpname1 + strlen(tmpname1) - 6) *c = 0;
			if ((c = strstr(tmpname1, "_day")) && c == tmpname1 + strlen(tmpname1) - 4) *c = 0;
			if ((c = strstr(tmpname2, "_night")) && c == tmpname2 + strlen(tmpname2) - 6) *c = 0;
			if ((c = strstr(tmpname2, "_day")) && c == tmpname2 + strlen(tmpname2) - 4) *c = 0;
			if (!strcmp(tmpname1, tmpname2)) day_night_changed = TRUE;
		}
		if (day_night_changed) {
			int n;

			for (n = 0; n < MUSIC_MAX; n++) {
 #if 0 /* just reset positions of day/night specific music? -- problem with this: Some music doesn't have a "_day" suffix but only a "_night" one. So we just reset all. */
				pmn = string_exec_lua(0, format("return get_music_name(%d)", n)); //abuse pmn
				if (!suffix(pmn, "_day") && !suffix(pmn, "_night")) continue;
 #endif
				songs[n].bak_pos = 0;
			}
			prev_wilderness = FALSE; //(efficient discard; not needed as we reset the pos to zero anyway)
		} else if (prev_wilderness && (prefix(mn, "wilderness_")
  #ifdef TOWN_TAVERN_MUSIC_RESUME_TOO
		    || (c_cfg.tavern_town_resume && (
		    prefix(mn, "town_") || prefix(mn, "tavern_") ||
		    prefix(mn, "Bree") || prefix(mn, "Gondolin") || prefix(mn, "MinasAnor") || prefix(mn, "Lothlorien") || prefix(mn, "Khazaddum") ||
		    prefix(mn, "Menegroth") || prefix(mn, "Nargothrond")))
  #endif
		    )) {
			music_next_song = songs[music_next].bak_song;
		}
	} else prev_wilderness = FALSE;
#endif

//logprint(format("fnm mc=%d/%d,mn=%d/%d\n", music_cur, music_cur_song, music_next, music_next_song));
	/* Choose the predetermined random event */
	wave = songs[music_next].wavs[music_next_song];

	/* Try loading it, if it's not cached */
	if (!wave && !(wave = load_song(music_next, music_next_song))) {
		/* we really failed to load it */
		plog(format("SDL music load failed (%d, %d) <%s> [2].", music_next, music_next_song, songs[music_next].paths[music_next_song]));
		puts(format("SDL music load failed (%d, %d) <%s> [2].", music_next, music_next_song, songs[music_next].paths[music_next_song]));
		return;
	}

#ifdef USER_VOLUME_MUS
	/* Apply user-defined custom volume modifier */
	if (songs[music_next].volume) Mix_VolumeMusic(CALC_MIX_VOLUME(cfg_audio_music, cfg_audio_music_volume, songs[music_next].volume));
#endif

	/* Actually play the thing */
//#ifdef DISABLE_MUTED_AUDIO /* now these vars are also used for 'continous' music across music events */
	music_cur = music_next;
	music_cur_song = music_next_song;
//#endif
	music_next = -1;

	/* Actually don't repeat 'initial' songs */
	if (!songs[music_cur].initial[music_cur_song]) {
		Mix_FadeInMusic(wave, c_cfg.shuffle_music || c_cfg.play_all ? 0 : -1, 1000); //-1 infinite, 0 once, or n times
	} else Mix_FadeInMusic(wave, c_cfg.shuffle_music || c_cfg.play_all ? 0 : 0, 1000); //even if play_all is off, continue with another song after an 'initial' song was played instead of repeating it

#ifdef WILDERNESS_MUSIC_RESUME
	/* Special: If new music is in category 'wilderness', restore its position to resume it instead of restarting it.
	   However, only do this if the previous music was actually in 'wilderness' too!
	   Part 2/2: Restore the position: */
	//if (prev_wilderness && prefix(songs[music_cur].paths[music_cur_song], format("%s/wilderness/", ANGBAND_DIR_XTRA_MUSIC))) {
	if (prev_wilderness) {
		mn = string_exec_lua(0, format("return get_music_name(%d)", music_cur));
		if (prefix(mn, "wilderness_")
  #ifdef TOWN_TAVERN_MUSIC_RESUME_TOO
		    || (c_cfg.tavern_town_resume && (
		    prefix(mn, "town_") || prefix(mn, "tavern_") ||
		    prefix(mn, "Bree") || prefix(mn, "Gondolin") || prefix(mn, "MinasAnor") || prefix(mn, "Lothlorien") || prefix(mn, "Khazaddum") ||
		    prefix(mn, "Menegroth") || prefix(mn, "Nargothrond")))
  #endif
		    ) {
			music_cur_song = songs[music_cur].bak_song;
			Mix_SetMusicPosition(songs[music_cur].bak_pos / 1000);
		}
	}
#endif
#ifdef ENABLE_JUKEBOX
	if (jukebox_screen) jukebox_update_songlength();
#endif
}

//#ifdef JUKEBOX_INSTANT_PLAY
/* Hack: event >= 20000 means 'no repeat'. */
/* This function ignores some music-options and is only used for special stuff like meta list or jukebox. */
static bool play_music_instantly(int event) {
	Mix_Music *wave = NULL;
	bool no_repeat = FALSE;

	if (event >= 19000) { //carry over any hacks just in case
		no_repeat = TRUE;
		event -= 20000;
	}

	/* Ultra-hack to avoid unwanted recursion: In jukebox and 'play all': Mix_HaltMusic() will call 'fadein_next_music()' here, but that is too early!
	   We only want to call fadein_next_music() after the song we are intending to start here via play_music_instantly() has actually ended. */
	if (jukebox_screen && jukebox_play_all) {
		jukebox_play_all = FALSE; //hax
		Mix_HaltMusic();
		jukebox_play_all = TRUE; //unhax
	} else
	/* Normal operation (;>_>) */
	{
		/* Mix_HaltMusic() will call 'fadein_next_music()' which would play the next subsong of music_cur, so we need to set it to -1 too for real music-stop. */
		if (event == -2) music_cur = -1;
		Mix_HaltMusic();
	}

	/* We just wanted to stop currently playing music, do nothing more and just return. */
	if (event == -2) {
		music_cur = -1;
		return(TRUE); //whatever..
	}

	/* Catch disabled songs */
	if (songs[event].disabled) {
		music_cur = -1;
		return(TRUE);
	}
	if (songs[event].num < 1) return(FALSE); //paranoia

	/* But play different song, iteratingly instead of randomly:
	   We ignore shuffle_music, play_all or 'initial' song type and just go through all songs
	   one by one in their order listed in music.cfg. */
	if (music_cur != event) {
		music_cur = (event < 0 ? -1 : event);
		music_cur_song = 0;
	} else music_cur_song = (music_cur_song + 1) % songs[music_cur].num;

	/* Choose the predetermined random event */
	wave = songs[music_cur].wavs[music_cur_song];

	/* Try loading it, if it's not cached */
	if (!wave && !(wave = load_song(music_cur, music_cur_song))) {
		/* we really failed to load it */
		plog(format("SDL music load failed (%d, %d) [3].", music_cur, music_cur_song));
		puts(format("SDL music load failed (%d, %d) [3].", music_cur, music_cur_song));
		return(FALSE);
	}

#ifdef USER_VOLUME_MUS
	/* Apply user-defined custom volume modifier */
	if (songs[event].volume) Mix_VolumeMusic(CALC_MIX_VOLUME(cfg_audio_music, cfg_audio_music_volume, songs[event].volume));
	else Mix_VolumeMusic(CALC_MIX_VOLUME(cfg_audio_music, cfg_audio_music_volume, 100));
#endif

	if (!jukebox_play_all) {
		/* Actually play the thing. We loop this specific sub-song infinitely and ignore c_cfg.shuffle_music and c_cfg.play_all (and 'initial' song status) here.
		   To get to hear other sub-songs, the user can press ENTER again to restart this music event with a different sub-song. */
		Mix_PlayMusic(wave, no_repeat ? 0 : -1);
	} else {
		jukebox_playing_song = music_cur_song;
		/* We're going through the complete jukebox with 'a'/'A': Never repeat a single song. */
		Mix_PlayMusic(wave, 0);
	}
	return(TRUE);
}
//#endif


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

#ifdef USER_VOLUME_MUS
	/* Apply user-defined custom volume modifier */
	if (songs[music_cur].volume) Mix_VolumeMusic(CALC_MIX_VOLUME(cfg_audio_music, cfg_audio_music_volume, songs[music_cur].volume));
#endif

	/* Take up playing again immediately, no fading in */
	Mix_PlayMusic(wave, c_cfg.shuffle_music || c_cfg.play_all ? 0 : -1);
}
#endif

/*
 * Set mixing levels in the SDL module.
 */
static void set_mixing_sdl(void) {
	int vols = 100;

#if 0 /* don't use relative sound-effect specific volumes, transmitted from the server? */
	Mix_Volume(-1, CALC_MIX_VOLUME(cfg_audio_sound, cfg_audio_sound_volume, 100));
#else /* use relative volumes (4.4.5b+) */
	int n;

	/* SFX channels */
	for (n = 0; n < cfg_max_channels; n++) {
		if (n == weather_channel) continue;
		if (n == ambient_channel) continue;

		/* HACK - use weather volume for thunder sfx */
		if (channel_sample[n] != -1 && channel_type[n] == SFX_TYPE_WEATHER)
			Mix_Volume(n, (CALC_MIX_VOLUME(cfg_audio_weather, (cfg_audio_weather_volume * channel_volume[n]) / 100, 100) * grid_weather_volume) / 100);
		else
		/* grid_ambient_volume influences non-looped ambient sfx clips */
		if (channel_sample[n] != -1 && channel_type[n] == SFX_TYPE_AMBIENT)
			Mix_Volume(n, (CALC_MIX_VOLUME(cfg_audio_sound, (cfg_audio_sound_volume * channel_volume[n]) / 100, 100) * grid_ambient_volume) / 100);
		else
		/* Note: Linear scaling is used here to allow more precise control at the server end */
			Mix_Volume(n, CALC_MIX_VOLUME(cfg_audio_sound, (cfg_audio_sound_volume * channel_volume[n]) / 100, 100));

 #ifdef DISABLE_MUTED_AUDIO
		if ((!cfg_audio_master || !cfg_audio_sound) && Mix_Playing(n))
			Mix_HaltChannel(n);
 #endif
	}
#endif

	/* Music channel (don't change volume while playing in the jukebox at 200% boosted volume via SHIFT+ENTER).
	   This can happen if for example some ambient sound effect changes/is played and set_mixing() is called in the process house situation changes (quiet_house_sfx),
	   which would not only mix that specific sfx but also reset music volume here from the temporary 200% boost to its actual real value. */
	if (jukebox_static200vol) vols = 200;
#ifdef USER_VOLUME_MUS
	else {
		/* Apply user-defined custom volume modifier */
		if (music_cur != -1 && songs[music_cur].volume) vols = songs[music_cur].volume;
	}
#endif
	//Mix_VolumeMusic(CALC_MIX_VOLUME(cfg_audio_music, cfg_audio_music_volume));
	Mix_VolumeMusic(CALC_MIX_VOLUME(cfg_audio_music, (cfg_audio_music_volume * music_vol) / 100, vols));
#ifdef DISABLE_MUTED_AUDIO
	if (!cfg_audio_master || !cfg_audio_music) {
		if (Mix_PlayingMusic()) Mix_HaltMusic();
	} else if (!Mix_PlayingMusic()) reenable_music();
#endif

	/* SFX channel (weather) */
	if (weather_channel != -1 && Mix_FadingChannel(weather_channel) != MIX_FADING_OUT) {
#ifndef WEATHER_VOL_PARTICLES
		weather_channel_volume = (CALC_MIX_VOLUME(cfg_audio_weather, cfg_audio_weather_volume, 100) * grid_weather_volume) / 100;
		Mix_Volume(weather_channel, weather_channel_volume);
#else
		Mix_Volume(weather_channel, weather_channel_volume);
#endif
	}

	/* SFX channel (ambient) */
	if (ambient_channel != -1 && Mix_FadingChannel(ambient_channel) != MIX_FADING_OUT) {
		ambient_channel_volume = (CALC_MIX_VOLUME(cfg_audio_sound, cfg_audio_sound_volume, 100) * grid_ambient_volume) / 100;
		Mix_Volume(ambient_channel, ambient_channel_volume);
	}

#ifdef DISABLE_MUTED_AUDIO
	/* Halt weather/ambient SFX channel immediately */
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
		return(1);
	}

	/* Set the mixing hook */
	mixing_hook = set_mixing_sdl;

	/* Enable sound */
	sound_hook = play_sound;

	/* Enable music */
	music_hook = play_music;
	music_hook_vol = play_music_vol;

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
		load_audio_thread = SDL_CreateThread(thread_load_audio, NULL, NULL);
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
	return(0);
}
/* Try to allow re-initializing audio.
   Purpose: Switching between audio packs live, without need for client restart. */
errr re_init_sound_sdl(void) {
	int i, j;


	/* --- exit --- */

	/* Close audio first, then try to open it again */
	close_audio();

	/* Reset variables (closing audio doesn't do this since it assumes program exit anyway) */
	for (i = 0; i < SOUND_MAX_2010; i++) {
		for (j = 0; j < MAX_SAMPLES; j++) { //could also just go to < samples[i].num instead, for efficiency...
			samples[i].wavs[j] = NULL;
			samples[i].paths[j] = NULL;
		}
		samples[i].num = 0;
		samples[i].config = FALSE;
		samples[i].disabled = FALSE;
	}
	for (i = 0; i < MUSIC_MAX; i++) {
		for (j = 0; j < MAX_SONGS; j++) { //could also just go to < songs[i].num instead, for efficiency...
			songs[i].wavs[j] = NULL;
			songs[i].paths[j] = NULL;
			songs[i].is_reference[j] = FALSE;
		}
		songs[i].num = 0;
		songs[i].config = FALSE;
		songs[i].disabled = FALSE;
#ifdef WILDERNESS_MUSIC_RESUME
		songs[i].bak_pos = 0;
#endif
	}

	for (i = 0; i < MAX_CHANNELS; i++) {
		channel_sample[i] = -1;
		channel_type[i] = 0;
		channel_volume[i] = 0;
		channel_player_id[i] = 0;
	}

	audio_sfx = 0;
	audio_music = 0;

	/*void (*mixing_hook)(void);
	bool (*sound_hook)(int sound, int type, int vol, s32b player_id, int dist_x, int dist_y);
	void (*sound_ambient_hook)(int sound_ambient);
	void (*sound_weather_hook)(int sound);
	void (*sound_weather_hook_vol)(int sound, int vol);
	bool (*music_hook)(int music);*/

	//cfg_audio_rate = 44100, cfg_max_channels = 32, cfg_audio_buffer = 1024;

	music_cur = -1;
	music_cur_song = -1;
	music_next = -1;
	music_next_song = -1;

	weather_channel = -1;
	weather_current = -1;
	weather_current_vol = -1;
	weather_channel_volume = 100;

	ambient_channel = -1;
	ambient_current = -1;
	ambient_channel_volume = 100;

	//weather_particles_seen, weather_sound_change, weather_fading, ambient_fading;
	//wind_noticable = FALSE;
#if 1 /* WEATHER_VOL_PARTICLES */
	//weather_vol_smooth, weather_vol_smooth_anti_oscill, weather_smooth_avg[20];
#endif

	//cfg_audio_master = TRUE, cfg_audio_music = TRUE, cfg_audio_sound = TRUE, cfg_audio_weather = TRUE, weather_resume = FALSE, ambient_resume = FALSE;
	//cfg_audio_master_volume = cfg_audio_music_volume = cfg_audio_sound_volume = cfg_audio_weather_volume = AUDIO_VOLUME_DEFAULT;

	//grid_weather_volume = grid_ambient_volume = grid_weather_volume_goal = grid_ambient_volume_goal = 100, grid_weather_volume_step, grid_ambient_volume_step;
	bell_sound_idx = -1; page_sound_idx = -1; warning_sound_idx = -1;
	rain1_sound_idx = -1; rain2_sound_idx = -1; snow1_sound_idx = -1; snow2_sound_idx = -1; thunder_sound_idx = -1;
	browse_sound_idx = -1; browsebook_sound_idx = -1; browseinven_sound_idx = -1;
	casino_craps_sound_idx = -1; casino_inbetween_sound_idx = -1; casino_wheel_sound_idx = -1; casino_slots_sound_idx = -1;

#ifndef WINDOWS //assume POSIX
	sdl_files_cur = 0;
#endif

	/* --- init --- */

	//if (no_cache_audio) plog("Audio cache disabled.");

#ifdef DEBUG_SOUND
	puts(format("re_init_sound_sdl() init%s", no_cache_audio == FALSE ? " (cached)" : " (not cached)"));
#endif

	/* Load sound preferences if requested */
#if 0
	if (!sound_sdl_init(no_cache_audio)) {
#else /* never cache audio right at program start, because it creates an annoying delay! */
	if (!sound_sdl_init(TRUE)) {
#endif
		plog("Failed to re-initialise audio.");

		/* Failure */
		return(1);
	}

	/* Set the mixing hook */
	mixing_hook = set_mixing_sdl;

	/* Enable sound */
	sound_hook = play_sound;

	/* Enable music */
	music_hook = play_music;
	music_hook_vol = play_music_vol;

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
		load_audio_thread = SDL_CreateThread(thread_load_audio, NULL, NULL);
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
	puts("re_init_sound_sdl() completed.");
#endif


	/* --- refresh active audio --- */
	Send_redraw(2);

	/* Success */
	return(0);
}

/* on game termination */
void mixer_fadeall(void) {
	music_next = -1;
	Mix_FadeOutMusic(1500);
	Mix_FadeOutChannel(-1, 1500);
}

//extra code I moved here for USE_SOUND_2010, for porting

/* if audioCached is TRUE, load those audio files in a separate
   thread, to avoid startup delay of client - C. Blue */
static int thread_load_audio(void *dummy) {
	(void) dummy; /* suppress compiler warning */
	int idx, subidx;

	/* process all sound fx */
	for (idx = 0; idx < SOUND_MAX_2010; idx++) {
		/* any sounds exist for this event? */
		if (!samples[idx].num) continue;

		/* process all files for each sound event */
		for (subidx = 0; subidx < samples[idx].num; subidx++) {
			load_sample(idx, subidx);
		}
	}

	/* process all music */
	for (idx = 0; idx < MUSIC_MAX; idx++) {
		/* any songs exist for this event? */
		if (!songs[idx].num) continue;

		/* process all files for each sound event */
		for (subidx = 0; subidx < songs[idx].num; subidx++) {
			load_song(idx, subidx);
		}
	}

#ifndef WINDOWS //assume POSIX
#ifdef DEBUG_SOUND
	log_fd_usage();
#endif
	logprint(format("Opened %d audio files (of %d max OS fds. Change via 'ulimit -n').\n", sdl_files_cur, sdl_files_max));
#endif

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
		return(samples[idx].wavs[subidx]);
	}

	/* Try loading it, if it's not yet cached */

	/* Verify it exists */
	if (!my_fexists(filename)) {
#ifdef DEBUG_SOUND
		puts(format("file doesn't exist %d, %d: %s.", idx, subidx, filename));
#endif
		SDL_UnlockMutex(load_sample_mutex);
		return(NULL);
	}

#ifndef WINDOWS //assume POSIX
	sdl_files_cur++;
	if (sdl_files_cur >= sdl_files_max) {
		logprint(format("Too many audio files. Reached maximum of %d, discarding <%s>.\n", sdl_files_max, path));
		// and for now just disable the whole event, to be safe against repeated attempts to load it
		samples[idx].disabled = TRUE;
		SDL_UnlockMutex(load_sample_mutex);
		return(NULL);
	}
#ifdef DEBUG_SOUND
	logprint(format("LS-sdl_files_cur++ = %d/%d (%s)\n", sdl_files_cur, sdl_files_max, filename));
#endif
#endif

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
		return(NULL);
	}

	SDL_UnlockMutex(load_sample_mutex);
	return(wave);
}
static Mix_Music* load_song(int idx, int subidx) {
	const char *filename = songs[idx].paths[subidx];
	Mix_Music *waveMUS = NULL;

	/* check if it's a reference - then check if original is already loaded...
	   - and if so don't load it again as we're just the clone, but point to it.
	   - if original isn't loaded yet, load it and point to it. */
	if (songs[idx].is_reference[subidx]) {
		/* original is not already loaded? */
#ifdef DEBUG_SOUND
		printf("load_song(%d,%d) -- reference %d,%d", idx, subidx, songs[idx].referenced_event[subidx], songs[idx].referenced_song[subidx]);
#endif
		if (!songs[songs[idx].referenced_event[subidx]].wavs[songs[idx].referenced_song[subidx]]) {
			/* Load the original now, ahead of its time */
#ifdef DEBUG_SOUND
			printf("PRELOAD\n");
#endif
			if (!load_song(songs[idx].referenced_event[subidx], songs[idx].referenced_song[subidx]))
				return(NULL); //if original fails to load, we fail too
		}
#ifdef DEBUG_SOUND
		printf("\n");
#endif
		/* point to original */
		songs[idx].paths[subidx] = songs[songs[idx].referenced_event[subidx]].paths[songs[idx].referenced_song[subidx]];
		songs[idx].wavs[subidx] = songs[songs[idx].referenced_event[subidx]].wavs[songs[idx].referenced_song[subidx]];

		return(songs[idx].wavs[subidx]);
	}

	SDL_LockMutex(load_song_mutex_entrance);
	SDL_LockMutex(load_song_mutex);
	SDL_UnlockMutex(load_song_mutex_entrance);

#ifdef DEBUG_SOUND
	printf("load_song(%d,%d)\n", idx, subidx);
#endif

	/* check if it's already loaded */
	if (songs[idx].wavs[subidx]) {
#ifdef DEBUG_SOUND
		puts(format("song already loaded %d, %d: %s.", idx, subidx, filename));
#endif
		SDL_UnlockMutex(load_song_mutex);
		return(songs[idx].wavs[subidx]);
	}

	/* Try loading it, if it's not yet cached */

	/* Verify it exists */
	if (!my_fexists(filename)) {
#ifdef DEBUG_SOUND
		puts(format("file doesn't exist %d, %d: %s.", idx, subidx, filename));
#endif
		SDL_UnlockMutex(load_song_mutex);
		return(NULL);
	}

#ifndef WINDOWS //assume POSIX
	sdl_files_cur++;
	if (sdl_files_cur >= sdl_files_max) {
		logprint(format("Too many audio files. Reached maximum of %d, discarding <%s>.\n", sdl_files_max, path));
		// and for now just disable the whole event, to be safe against repeated attempts to load it
		songs[idx].disabled = TRUE;
		SDL_UnlockMutex(load_sample_mutex);
		return(NULL);
	}
#ifdef DEBUG_SOUND
	logprint(format("LM-sdl_files_cur++ = %d/%d (%s)\n", sdl_files_cur, sdl_files_max, filename));
#endif
#endif

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
		return(NULL);
	}

	SDL_UnlockMutex(load_song_mutex);
	return(waveMUS);
}

/* Display options page UI that allows to comment out sounds easily */
void do_cmd_options_sfx_sdl(void) {
	int i, i2, j, d, k, vertikal_offset = 4, horiz_offset = 5, list_size = 9;
	static int y = 0, j_sel = 0;
	int tmp;
	char ch;
	byte a, a2;
	cptr lua_name;
	bool go = TRUE, dis;
	char buf[1024], buf2[1024], out_val[4096], out_val2[4096], *p, evname[4096];
	FILE *fff, *fff2;
	bool cfg_audio_master_org = cfg_audio_master, cfg_audio_sound_org = cfg_audio_sound;
	bool cfg_audio_music_org = cfg_audio_music, cfg_audio_weather_org = cfg_audio_weather;
	static char searchstr[MAX_CHARS] = { 0 };
	static int searchres = -1, searchoffset = 0;
	static bool searchforfilename = FALSE;
	cptr path_p;

	//ANGBAND_DIR_XTRA_SOUND/MUSIC are NULL in quiet_mode!
	if (quiet_mode) {
		c_msg_print("\377yClient is running in quiet mode, sounds are not available.");
		return;
	}
	if (!audio_sfx) {
		c_msg_print("\377yNo sound effects available.");
		return;
	}

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_XTRA_SOUND, "sound.cfg");

	/* Check if the file exists */
	fff = my_fopen(buf, "r");
	if (!fff) {
		c_msg_format("\377oError: Cannot read sound config file '%s'.", buf);
		return;
	}
	fclose(fff);

	/* Clear screen */
	Term_clear();

	topline_icky = TRUE;

	jukebox_sfx_screen = TRUE;
	sound_cur_wav = -1;

	/* Interact */
	Term_putstr(0, 2, -1, TERM_WHITE, " File:                                                                          ");
	Term_putstr(7, 2, -1, TERM_L_DARK, "-");
	while (go) {
#ifdef USER_VOLUME_SFX
 #ifdef ENABLE_SHIFT_SPECIALKEYS
		if (strcmp(ANGBAND_SYS, "gcu"))
			Term_putstr(0, 0, -1, TERM_WHITE, "  \377ydir\377w/\377y#\377w/\377ys\377w/\377yS\377w/'\377y/\377w', \377yt\377w toggle, \377yy\377w/\377yn\377w on/off, \377yv\377w volume, \377y[SHIFT+]RETURN\377w [boost+]play");
		else /* GCU cannot query shiftkey states easily, see macro triggers too (eg cannot distinguish between ENTER and SHIFT+ENTER on GCU..) */
 #endif
		Term_putstr(0, 0, -1, TERM_WHITE, "  (<\377ydir\377w/\377y#\377w/\377ys\377w/\377yS\377w/'\377y/\377w'>, \377yt\377w (toggle), \377yy\377w/\377yn\377w (on/off), \377yv\377w volume, \377yRETURN\377w (play)");
		Term_putstr(0, 1, -1, TERM_WHITE, "  \377yESC \377wleave and auto-save all changes.                                          ");
#else
		Term_putstr(0, 0, -1, TERM_WHITE, "  (<\377ydir\377w/\377y#\377w/\377ys\377w/\377yS\377w/'\377y/\377w'>, \377yt\377w (toggle), \377yy\377w/\377yn\377w (on/off), \377yRETURN\377w (play), \377yESC\377w)");
		Term_putstr(0, 1, -1, TERM_WHITE, "  (\377wAll changes made here will auto-save as soon as you leave this page)");
#endif

		/* Display the events */
		for (i = y - list_size ; i <= y + list_size; i++) {
			if (i < 0 || i >= audio_sfx) {
				Term_putstr(horiz_offset + 5, vertikal_offset + i + list_size - y, -1, TERM_WHITE, "                                                          ");
				continue;
			}

			/* Map events we've listed in our local config file onto audio.lua indices */
			i2 = -1;
			for (j = 0; j < SOUND_MAX_2010; j++) {
				if (!samples[j].config) continue;
				i2++;
				if (i2 == i) break;
			}
			if (j != SOUND_MAX_2010) { //paranoia, should always be non-equal aka true
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

			Term_putstr(horiz_offset + 5, vertikal_offset + i + list_size - y, -1, a2, format("  %3d [   %2d]", i + 1, samples[j].num));
			if (sound_cur == j && sound_cur_wav != -1) Term_putstr(horiz_offset + 5 + 7, vertikal_offset + i + list_size - y, -1, TERM_YELLOW, format("%2d/", sound_cur_wav + 1));
			Term_putstr(horiz_offset + 12 + 8, vertikal_offset + i + list_size - y, -1, a, "                                            ");
			Term_putstr(horiz_offset + 12 + 8, vertikal_offset + i + list_size - y, -1, a, (char*)lua_name);
			if (j == weather_current || j == ambient_current) {
				if (a != TERM_L_DARK) a = TERM_L_GREEN;
				Term_putstr(horiz_offset + 5, vertikal_offset + i + list_size - y, -1, a, "*");
				Term_putstr(horiz_offset + 12 + 8, vertikal_offset + i + list_size - y, -1, a, format("%-27s  (playing)", (char*)lua_name));
			} else
				Term_putstr(horiz_offset + 12 + 8, vertikal_offset + i + list_size - y, -1, a, (char*)lua_name);

#ifdef USER_VOLUME_SFX
			if (samples[j].volume && samples[j].volume != 100) {
				if (samples[j].volume < 100) a = TERM_UMBER; else a = TERM_L_UMBER;
				Term_putstr(horiz_offset + 1 + 12 + 36 + 1, vertikal_offset + i + list_size - y, -1, a, format("%2d%%", samples[j].volume));
			}
#endif
		}

#ifdef JUKEBOX_SELECTED_FILENAME /* Show currently selected music event's first song's filename */
		Term_putstr(0, 2, -1, TERM_WHITE, " File:                                                                          ");
		if (samples[j_sel].config) {
			path_p = samples[j_sel].paths[0];
			if (path_p) {
				path_p = path_p + strlen(path_p) - 1;
				while (path_p > samples[j_sel].paths[0] && *(path_p - 1) != '/') path_p--;
				Term_putstr(7, 2, -1, TERM_UMBER, format("%s", path_p));
			} else Term_putstr(7, 2, -1, TERM_L_DARK, "%"); //paranoia
		}
		else Term_putstr(7, 2, -1, TERM_L_DARK, "-");
#endif
		/* Specialty for big_map: Display list of all subsamples below the normal jukebox stuff */
		if (screen_hgt == MAX_SCREEN_HGT && samples[j_sel].config) {
			int s, offs = 24, showmax = MAX_WINDOW_HGT - offs - 2;
			char tmp_lastslot[MAX_CHARS] = { 0 };

			clear_from(offs);
			for (s = 0; s < MAX_SONGS; s++) {
				path_p = samples[j_sel].paths[s];
				if (!path_p) break;
				if (s == showmax) { /* add a final entry if it fits exactly */
					path_p = path_p + strlen(path_p) - 1;
					while (path_p > samples[j_sel].paths[s] && *(path_p - 1) != '/') path_p--;
					if (sound_cur == j_sel && sound_cur_wav == s)
						snprintf(tmp_lastslot, MAX_CHARS, "\377y%2d. %s", s + 1, path_p);
					else
						snprintf(tmp_lastslot, MAX_CHARS, "\377u%2d. %s", s + 1, path_p);
				}
				if (s >= showmax) continue;
				path_p = path_p + strlen(path_p) - 1;
				while (path_p > samples[j_sel].paths[s] && *(path_p - 1) != '/') path_p--;
				if (sound_cur == j_sel && sound_cur_wav == s)
					Term_putstr(7 - 4, offs + 1 + s, -1, TERM_YELLOW, format("%2d. %s", s + 1, path_p));
				else
					Term_putstr(7 - 4, offs + 1 + s, -1, TERM_UMBER, format("%2d. %s", s + 1, path_p));
			}
			if (s == showmax + 1) {
				Term_putstr(7 - 4, MAX_WINDOW_HGT - 1, -1, TERM_SLATE, tmp_lastslot);
				Term_putstr(0, offs, -1, TERM_WHITE, format("List of the first %d subsong filenames (found \377y%d\377w) of the selected sound event:", showmax + 1, s));
			} else if (s > showmax + 1) {
				Term_putstr(7 - 4, MAX_WINDOW_HGT - 1, -1, TERM_SLATE, format("... and %d more...", s - showmax));
				Term_putstr(0, offs, -1, TERM_WHITE, format("List of the first %d subsong filenames (found \377y%d\377w) of the selected sound event:", showmax, s));
			} else
				Term_putstr(0, offs, -1, TERM_WHITE, format("List of the first %d subsong filenames (found \377y%d\377w) of the selected sound event:", showmax + 1, s));
		}

		/* display static selector */
		Term_putstr(horiz_offset + 1, vertikal_offset + list_size, -1, TERM_SELECTOR, ">>>");
		Term_putstr(horiz_offset + 1 + 12 + 50 + 1, vertikal_offset + list_size, -1, TERM_SELECTOR, "<<<");

		/* Place Cursor */
		//Term_gotoxy(20, vertikal_offset + y);
		/* hack: hide cursor */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		/* Get key */
#ifdef ENABLE_SHIFT_SPECIALKEYS
		inkey_shift_special = 0x0;
#endif
		ch = inkey();

		/* Analyze */
		switch (ch) {
		case ESCAPE:
			/* Restore real mixer settings */
			cfg_audio_master = cfg_audio_master_org;
			cfg_audio_sound = cfg_audio_sound_org;
			cfg_audio_music = cfg_audio_music_org;
			cfg_audio_weather = cfg_audio_weather_org;

			sound(j_sel, SFX_TYPE_STOP, 100, 0, 0, 0);

			/* auto-save */

			/* -- save disabled info -- */

			path_build(buf, 1024, ANGBAND_DIR_XTRA_SOUND, "sound.cfg");
#ifndef WINDOWS
			path_build(buf2, 1024, ANGBAND_DIR_XTRA_SOUND, "sound.$$$");
#else
			if (!win_dontmoveuser && getenv("HOMEDRIVE") && getenv("HOMEPATH"))
				sprintf(buf2, "%s\\TomeNET-%s-disabled.cfg", ANGBAND_DIR_USER, cfg_soundpackfolder);
			else path_build(buf2, sizeof(path), ANGBAND_DIR_XTRA_SOUND, "TomeNET-nosound.cfg"); //paranoia
#endif
			fff = my_fopen(buf, "r");
			fff2 = my_fopen(buf2, "w");
			if (!fff) {
				c_msg_format("Error: Cannot read sound config file '%s'.", buf);
				jukebox_sfx_screen = FALSE;
				topline_icky = FALSE;
				return;
			}
			if (!fff2) {
				c_msg_format("Error: Cannot write to disabled-sound config file '%s'.", buf2);
				jukebox_sfx_screen = FALSE;
				topline_icky = FALSE;
				return;
			}
			while (TRUE) {
				if (!fgets(out_val, 4096, fff)) {
					if (ferror(fff)) c_msg_format("Error: Failed to read from sound config file '%s'.", buf);
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

			/* -- save volume info -- */

			path_build(buf, 1024, ANGBAND_DIR_XTRA_SOUND, "sound.cfg");
#ifndef WINDOWS
			path_build(buf2, 1024, ANGBAND_DIR_XTRA_SOUND, "TomeNET-soundvol.cfg");
#else
			if (!win_dontmoveuser && getenv("HOMEDRIVE") && getenv("HOMEPATH"))
				sprintf(buf2, "%s\\TomeNET-%s-volume.cfg", ANGBAND_DIR_USER, cfg_soundpackfolder);
			else path_build(buf2, sizeof(path), ANGBAND_DIR_XTRA_SOUND, "TomeNET-soundvol.cfg"); //paranoia
#endif
			fff = my_fopen(buf, "r");
			fff2 = my_fopen(buf2, "w");
			if (!fff) {
				c_msg_format("Error: Cannot read sound config file '%s'.", buf);
				jukebox_sfx_screen = FALSE;
				topline_icky = FALSE;
				return;
			}
			if (!fff2) {
				c_msg_format("Error: Cannot write to sound volume config file '%s'.", buf2);
				jukebox_sfx_screen = FALSE;
				topline_icky = FALSE;
				return;
			}
			while (TRUE) {
				if (!fgets(out_val, 4096, fff)) {
					if (ferror(fff)) c_msg_format("Error: Failed to read from sound config file '%s'.", buf);
					jukebox_sfx_screen = FALSE;
					topline_icky = FALSE;
					break;
				}

				p = out_val;
				/* remove comment-character */
				if (*p == ';') p++;

				/* ignores lines that don't start on a letter */
				if (tolower(*p) < 'a' || tolower(*p) > 'z') continue;

				/* extract event name */
				strcpy(evname, p);
				*(strchr(evname, ' ')) = 0;

				/* find out event state (disabled/enabled) */
				j = exec_lua(0, format("return get_sound_index(\"%s\")", evname));
				if (j == -1 || !samples[j].config) continue;

				/* apply new state */
				if (samples[j].volume) {
					strcpy(out_val2, evname);
					strcat(out_val2, "\n");
					fputs(out_val2, fff2);
					sprintf(out_val2, "%d\n", samples[j].volume);
					fputs(out_val2, fff2);
				}
			}
			fclose(fff);
			fclose(fff2);

			/* -- all done -- */
			go = FALSE;
			break;

#ifdef USER_VOLUME_SFX /* needs work @ actual mixing algo */
		case 'v': {
			//i = c_get_quantity("Enter volume % (1..100): ", -1, 100);
			bool inkey_msg_old = inkey_msg;
			char tmp[80];

			inkey_msg = TRUE;
			Term_putstr(0, 1, -1, TERM_L_BLUE, "                                                                              ");
			Term_putstr(0, 1, -1, TERM_L_BLUE, "  Enter volume % (1..200, other values will reset to 100%): ");
			strcpy(tmp, "100");
			if (!askfor_aux(tmp, 4, 0)) {
				inkey_msg = inkey_msg_old;
				samples[j_sel].volume = 0;
				break;
			}
			inkey_msg = inkey_msg_old;
			i = atoi(tmp);
			if (i < 1 || i == 100) i = 0;
			else if (i > 200) i = 200;
			samples[j_sel].volume = i;
			/* Note: Unlike for music we don't adjust an already playing SFX' volume live here, instead the volume is applied the next time it is played. */
			break;
			}
#endif

		case KTRL('T'):
			/* Take a screenshot */
			xhtml_screenshot("screenshot????", 2);
			break;
		case ':': {
			bool inkey_msg_old = inkey_msg;

			/* specialty: allow chatting from within here */
			cmd_message();
			inkey_msg = inkey_msg_old;
			break;
			}

		case 't': //case ' ':
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
			/* actually advance down the list too */
			sound(j_sel, SFX_TYPE_STOP, 100, 0, 0, 0);
			y = (y + 1 + audio_sfx) % audio_sfx;
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
			/* actually advance down the list too */
			sound(j_sel, SFX_TYPE_STOP, 100, 0, 0, 0);
			y = (y + 1 + audio_sfx) % audio_sfx;
			break;
		case 'n':
			samples[j_sel].disabled = TRUE;
			if (j_sel == weather_current && weather_channel != -1 && Mix_Playing(weather_channel)) Mix_HaltChannel(weather_channel);
			if (j_sel == ambient_current && ambient_channel != -1 && Mix_Playing(ambient_channel)) Mix_HaltChannel(ambient_channel);
			/* actually advance down the list too */
			sound(j_sel, SFX_TYPE_STOP, 100, 0, 0, 0);
			y = (y + 1 + audio_sfx) % audio_sfx;
			break;

		case '\r':
			/* Force-enable the mixer to play music */
			if (!cfg_audio_master) {
				cfg_audio_master = TRUE;
				cfg_audio_music = FALSE;
				cfg_audio_weather = FALSE;
			}
			cfg_audio_sound = TRUE;

			dis = samples[j_sel].disabled;
			samples[j_sel].disabled = FALSE;
#ifdef ENABLE_SHIFT_SPECIALKEYS
			if (inkey_shift_special == 0x1) {
				int v = samples[j_sel].volume;

				samples[j_sel].volume = 200; /* SHIFT+ENTER: Play at maximum allowed volume aka 200% boost. */
				sound(j_sel, SFX_TYPE_MISC, 100, 0, 0, 0);
				samples[j_sel].volume = v;
			} else
#endif
			sound(j_sel, SFX_TYPE_MISC, 100, 0, 0, 0);
			samples[j_sel].disabled = dis;

			Term_putstr(0, 2, -1, TERM_WHITE, " File:                                                                          ");
			path_p = samples[j_sel].paths[sound_cur_wav];
			if (path_p) {
#if 0 /* crop full path? */
				path_p = path_p + strlen(path_p) - 1;
				while (path_p > samples[j_sel].paths[sound_cur_wav] && *(path_p - 1) != '/') path_p--;
#else /* crop only the lib-sound path? */
				path_p += strlen(ANGBAND_DIR_XTRA_SOUND) + 1;
#endif
				Term_putstr(7, 2, -1, TERM_YELLOW, format("%s", path_p));
			} else Term_putstr(7, 2, -1, TERM_L_DARK, "%"); //paranoia

			//cfg_audio_sound = cfg_audio_sound_org;
			break;

		case '#':
			tmp = c_get_quantity(format("  Enter sfx index number (1-%d): ", audio_sfx), 1, audio_sfx) - 1;
			if (!tmp) break;
			sound(j_sel, SFX_TYPE_STOP, 100, 0, 0, 0);
			y = tmp;
			if (y < 0) y = 0;
			if (y >= audio_sfx) y = audio_sfx - 1;
			break;
		case 's': /* Search for event name */
			Term_putstr(0, 0, -1, TERM_WHITE, "  Enter (partial) sound event name: ");
			askfor_aux(searchstr, MAX_CHARS - 1, 0);
			if (!searchstr[0]) break;
			searchres = -1;
			searchforfilename = FALSE;

			/* Map events we've listed in our local config file onto audio.lua indices */
			i2 = -1;
			for (j = 0; j < SOUND_MAX_2010; j++) {
				if (!samples[j].config) continue;
				i2++;
				/* get event name */
				sprintf(out_val, "return get_sound_name(%d)", j);
				lua_name = string_exec_lua(0, out_val);
				if (!my_strcasestr(lua_name, searchstr)) continue;
				/* match */
				y = i2;
				searchoffset = y;
				searchres = j;
				break;
			}
			break;
		case 'S': /* Search for file name */
			Term_putstr(0, 0, -1, TERM_WHITE, "  Enter (partial) sound file name: ");
			askfor_aux(searchstr, MAX_CHARS - 1, 0);
			if (!searchstr[0]) break;
			searchres = -1;
			searchforfilename = TRUE;

			/* Map events we've listed in our local config file onto audio.lua indices */
			i2 = -1;
			for (j = 0; j < SOUND_MAX_2010; j++) {
				if (!samples[j].config) continue;
				i2++;
				/* get file name */
				for (d = 0; d < samples[j].num; d++)
					if (my_strcasestr(samples[j].paths[d], searchstr)) break;
				if (d == samples[j].num) continue;
				/* match */
				y = i2;
				searchoffset = y;
				searchres = j;
				break;
			}
			break;
		case '/': /* Search for next result */
			if (!searchstr[0]) continue;
			/* Map events we've listed in our local config file onto audio.lua indices */
			i2 = -1;
			searchoffset++; //we start searching at searchres + 1!
			for (d = searchres + 1; d <= searchres + SOUND_MAX_2010; d++) {
				j = d % SOUND_MAX_2010;
				if (!j) {
					i2 = -1;
					searchoffset = 0;
				}
				if (!samples[j].config) continue;
				i2++;

				if (searchforfilename) {
					/* get file name */
					for (k = 0; k < samples[j].num; k++)
						if (my_strcasestr(samples[j].paths[k], searchstr)) break;
					if (k == samples[j].num) continue;
				} else {
					/* get event name */
					sprintf(out_val, "return get_sound_name(%d)", j);
					lua_name = string_exec_lua(0, out_val);
					if (!my_strcasestr(lua_name, searchstr)) continue;
				}

				/* match */
				y = i2 + searchoffset;
				searchoffset = y;
				searchres = j;
				break;
			}
			break;

		case NAVI_KEY_PAGEUP:
		case '9':
		case 'p':
			sound(j_sel, SFX_TYPE_STOP, 100, 0, 0, 0);
			y = (y - list_size + audio_sfx) % audio_sfx;
			break;
		case NAVI_KEY_PAGEDOWN:
		case '3':
		case ' ':
			sound(j_sel, SFX_TYPE_STOP, 100, 0, 0, 0);
			y = (y + list_size + audio_sfx) % audio_sfx;
			break;
		case NAVI_KEY_END:
		case '1':
		case 'G':
			sound(j_sel, SFX_TYPE_STOP, 100, 0, 0, 0);
			y = audio_sfx - 1;
			break;
		case NAVI_KEY_POS1:
		case '7':
		case 'g':
			sound(j_sel, SFX_TYPE_STOP, 100, 0, 0, 0);
			y = 0;
			break;
		case NAVI_KEY_UP:
		case '8':
		case NAVI_KEY_DOWN:
		case '2':
			if (ch == NAVI_KEY_UP) ch = '8';
			if (ch == NAVI_KEY_DOWN) ch = '2';
			sound(j_sel, SFX_TYPE_STOP, 100, 0, 0, 0);
			d = keymap_dirs[ch & 0x7F];
			y = (y + ddy[d] + audio_sfx) % audio_sfx;
			break;
		case '\010':
			sound(j_sel, SFX_TYPE_STOP, 100, 0, 0, 0);
			y = (y - 1 + audio_sfx) % audio_sfx;
			break;
		default:
			bell();
		}
	}

	jukebox_sfx_screen = FALSE;
	topline_icky = FALSE;
}

/* Display options page UI that allows to comment out music easily */
#ifdef ENABLE_JUKEBOX
 #define MUSIC_SKIP 10 /* Jukebox backward/forward skip interval in seconds */
/* Shuffle an array of integers using the Fisher-Yates algorithm. */
void intshuffle(int *array, int size) {
	int i, j, tmp;

	for (i = size - 1; i > 0; i--) {
		j = rand_int(i + 1);
		tmp = array[i];
		array[i] = array[j];
		array[j] = tmp;
	}
}
#endif
void do_cmd_options_mus_sdl(void) {
	int i, i2, j, d, k, vertikal_offset = 5, horiz_offset = 1, list_size = 9;
	static int y = 0, j_sel = 0; // j_sel = -1; for initially jumping to playing song, see further below
	char ch;
	byte a, a2;
	cptr lua_name;
	bool go = TRUE, play, jukebox_used = FALSE;
	char buf[1024], buf2[1024], out_val[4096], out_val2[4096], *p, evname[4096];
	FILE *fff, *fff2;
#ifdef ENABLE_JUKEBOX
	bool cfg_audio_master_org = cfg_audio_master, cfg_audio_sound_org = cfg_audio_sound;
	bool cfg_audio_music_org = cfg_audio_music, cfg_audio_weather_org = cfg_audio_weather;
 #ifdef JUKEBOX_INSTANT_PLAY
	bool dis;
 #endif
	cptr path_p;
#endif
	static char searchstr[MAX_CHARS] = { 0 };
	static int searchres = -1, searchoffset = 0;
	static bool searchforfilename = FALSE;

	//ANGBAND_DIR_XTRA_SOUND/MUSIC are NULL in quiet_mode!
	if (quiet_mode) {
		c_msg_print("\377yClient is running in quiet mode, music is not available.");
		return;
	}
	if (!audio_music) {
		c_msg_print("\377yNo music available.");
		return;
	}

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_XTRA_MUSIC, "music.cfg");

	/* Check if the file exists */
	fff = my_fopen(buf, "r");
	if (!fff) {
		c_msg_format("\377oError: Cannot read music config file '%s'.", buf);
		return;
	}
	fclose(fff);

#ifdef ENABLE_JUKEBOX
	jukebox_org = music_cur;
	jukebox_org_song = music_cur_song;
	jukebox_org_repeat = music_cur_repeat;
	jukebox_org_vol = music_vol;
	jukebox_screen = TRUE;
	/* Resolve timing glitch: If music is currently in transition of fading out -> new music going to fade in afterwards
	   (eg town->dungeon character movement), the jukebox will need to update the 'org' stats with the new music. */
	if (Mix_FadingMusic() == MIX_FADING_OUT) jukebox_org = music_next;
#endif

	/* Clear screen */
	Term_clear();

	topline_icky = TRUE;

#if 0 /* instead of this, rather add a 'c' key that jumps to the currently playing song */
	/* Initially jump selection cursor to song that is currently being played */
	if (j_sel == -1) {
		for (j = 0; j < MUSIC_MAX; j++) {
			//if (!songs[j].config) continue;
			/* playing atm? */
			if (j != music_cur) continue;
			/* match */
			j_sel = y = j;
			break;
		}
	}
#endif

	jukebox_update_songlength();

	/* -- Interact -- */
	/* Maybe add missing navigational keys to the key list, but it's spammy and these are always the usual ones...:
		9, pgup; 3, pgdn; 1, end; 7, pos1; 8, bksp, up; 2, down */
	while (go) {
#ifdef ENABLE_JUKEBOX
 #ifdef USER_VOLUME_MUS
		Term_putstr(0, 0, -1, TERM_WHITE, " \377ydir\377w/\377yp\377w/\377ySPC\377w/\377yg\377w/\377yG\377w/\377y#\377w/\377ys\377w/\377yS\377w/'\377y/\377w', \377yc\377w cur, \377yt\377w/\377yy\377w/\377yn\377w toggle/on/off, \377yv\377w/\377y+\377w/\377y-\377w vol, \377yESC \377wsave+quit");
 #else
		Term_putstr(0, 0, -1, TERM_WHITE, " \377ydir\377w/\377yp\377w/\377ySPC\377w/\377yg\377w/\377yG\377w/\377y#\377w/\377ys\377w/\377yS\377w/'\377y/\377w', \377yc\377w cur., \377yt\377w/\377yy\377w/\377yn\377w toggle/on/off, \377yESC \377wsave+quit");
 #endif
 #ifdef ENABLE_SHIFT_SPECIALKEYS
		if (strcmp(ANGBAND_SYS, "gcu"))
			Term_putstr(0, 1, -1, TERM_WHITE, format(" \377y[SHIFT+]RETURN\377w/\377ya\377w/\377yu\377w [200%% vol] play/all/shuffle, \377yLEFT\377w/\377yRIGHT\377w rw/ff %ds, \377yP\377w %s",
			    MUSIC_SKIP, jukebox_paused ? (jukebox_play_all ? "\3777resume" : "\3774resume") : "pause "));
		else /* GCU cannot query shiftkey states easily, see macro triggers too (eg cannot distinguish between ENTER and SHIFT+ENTER on GCU..) */
 #endif
			Term_putstr(0, 1, -1, TERM_WHITE, format(" \377yRETURN\377w/\377ya\377w/\377yA\377w/\377yu\377w/\377yU\377w play/all/shuffle [at 200%%], \377yLEFT\377w/\377yRIGHT\377w rw/ff %ds, \377yP\377w pause", MUSIC_SKIP));
		Term_putstr(0, 2, -1, TERM_WHITE, " Key: [current/max song] - orange colour indicates 'initial' song.              ");
		if (jukebox_play_all) Term_putstr(67, 2, -1, TERM_WHITE, "\377yq\377B/\377yQ\377B/\377yw\377B/\377yW\377B skip");
		else if (jukebox_playing != -1) Term_putstr(67, 2, -1, TERM_WHITE, "\377yq\377w/\377yw\377w skip    "); //for new 'jukebox_subonly_play_all'
		Term_putstr(0, 3, -1, TERM_WHITE, " File:                                                                          ");

		curmus_y = -1; //assume not visible (outside of visible song list)
#else
 #ifdef USER_VOLUME_MUS
		Term_putstr(0, 0, -1, TERM_WHITE, " \377ydir\377w/\377yp\377w/\377ySPC\377w/\377yg\377w/\377yG\377w/\377y#\377w, \377yt\377w toggle, \377yy\377w/\377yn\377w on/off, \377yv\377w/\377y+\377w/\377y-\377w volume, \377yESC\377w save");
 #else
		Term_putstr(0, 0, -1, TERM_WHITE, " \377ydir\377w/\377yp\377w/\377ySPC\377w/\377yg\377w/\377yG\377w/\377y#\377w, \377yt\377w toggle, \377yy\377w/\377yn\377w on/off, \377yESC\377w leave+autosave");
 #endif
#endif

		/* Display the events */
		for (i = y - list_size ; i <= y + list_size ; i++) {
			if (i < 0 || i >= audio_music) {
				Term_putstr(horiz_offset + 5, vertikal_offset + i + list_size - y, -1, TERM_WHITE, "                                                                    ");
				continue;
			}

			/* Map events we've listed in our local config file onto audio.lua indices */
			i2 = -1;
			for (j = 0; j < MUSIC_MAX; j++) {
				if (!songs[j].config) continue;
				i2++;
				if (i2 == i) break;
			}
			if (j != MUSIC_MAX) { //paranoia, should always be non-equal aka true
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

			/* Check if 'initial' songs exist for this event */
			for (d = 0; d < songs[j].num; d++) {
				if (!songs[j].initial[d]) continue;
				break;
			}
			d = (songs[j].num && (d == songs[j].num)) ? 0 : -1;

			Term_putstr(horiz_offset + 12 + 8, vertikal_offset + i + list_size - y, -1, a, "                                                      ");
			if (j == music_cur) {
				Term_putstr(horiz_offset + 5, vertikal_offset + i + list_size - y, -1, a2, format("  %3d [\377%c%2d\377-/\377%c%2d\377-]", i + 1, songs[j].initial[music_cur_song] ? 'o' : 'y', music_cur_song + 1, d ? 'o' : 'y', songs[j].num));

				a = (jukebox_playing != -1) ? TERM_L_BLUE : (a != TERM_L_DARK ? TERM_L_GREEN : TERM_L_DARK); /* blue = user-selected jukebox song, l-green = current game music */
				Term_putstr(horiz_offset + 5, vertikal_offset + i + list_size - y, -1, a, "*");
				/* New via SDL2_mixer: Add the timestamp */
				curmus_x = horiz_offset + 12 + 8;
				curmus_y = vertikal_offset + i + list_size - y;
				curmus_attr = a;
				if (!curmus_song_dur) Term_putstr(curmus_x, curmus_y, -1, curmus_attr, format("%-37s  (     )", (char*)lua_name));
				else Term_putstr(curmus_x, curmus_y, -1, curmus_attr, format("%-37s  (     /%02d:%02d)", (char*)lua_name, curmus_song_dur / 60, curmus_song_dur % 60));
				update_jukebox_timepos();
			} else {
				Term_putstr(horiz_offset + 5, vertikal_offset + i + list_size - y, -1, a2, format("  %3d [\377%c   %2d\377-]", i + 1, d ? 'o' : 'y', songs[j].num));

				Term_putstr(horiz_offset + 12 + 8, vertikal_offset + i + list_size - y, -1, a, (char*)lua_name);
			}

#ifdef USER_VOLUME_MUS
			if (songs[j].volume && songs[j].volume != 100) {
				if (songs[j].volume < 100) a = TERM_UMBER; else a = TERM_L_UMBER;
				Term_putstr(horiz_offset + 1 + 12 + 36 + 1 + 4, vertikal_offset + i + list_size - y, -1, a, format("%2d%%", songs[j].volume)); //-6 to coexist with the new playtime display
			}
#endif
		}

		/* Show currently playing music event's specific song's filename */
		if (music_cur != -1 && songs[music_cur].config
#ifdef JUKEBOX_SELECTED_FILENAME
		    && music_cur == j_sel
#endif
		    ) {
			path_p = songs[music_cur].paths[music_cur_song];
			if (path_p) {
				path_p = path_p + strlen(path_p) - 1;
				while (path_p > songs[music_cur].paths[music_cur_song] && *(path_p - 1) != '/') path_p--;
				Term_putstr(7, 3, -1, songs[music_cur].initial[music_cur_song] ? TERM_ORANGE : TERM_YELLOW, format("%s%s", songs[music_cur].is_reference[music_cur_song] ? "\377s(R) \377-" : "", path_p)); //currently no different colour for songs[].disabled, consistent with 'Key' info
			} else Term_putstr(7, 3, -1, TERM_L_DARK, "%"); //paranoia
		}
#ifdef JUKEBOX_SELECTED_FILENAME /* Show currently selected music event's first song's filename */
		else if (songs[j_sel].config) {
			path_p = songs[j_sel].paths[0];
			if (path_p) {
				path_p = path_p + strlen(path_p) - 1;
				while (path_p > songs[j_sel].paths[0] && *(path_p - 1) != '/') path_p--;
				Term_putstr(7, 3, -1, songs[j_sel].initial[0] ? TERM_L_UMBER : TERM_UMBER, format("%s%s", songs[j_sel].is_reference[0] ? "\377s(R) \377-" : "", path_p)); //currently no different colour for songs[].disabled, consistent with 'Key' info
			} else Term_putstr(7, 3, -1, TERM_L_DARK, "%"); //paranoia
		}
#endif
		else Term_putstr(7, 3, -1, TERM_L_DARK, "-");

		/* Specialty for big_map: Display list of all subsongs below the normal jukebox stuff */
		if (screen_hgt == MAX_SCREEN_HGT && songs[j_sel].config) {
			int s, offs = 25, showmax = MAX_WINDOW_HGT - offs - 2;
			char tmp_lastslot[MAX_CHARS] = { 0 };

			clear_from(offs);
			for (s = 0; s < MAX_SONGS; s++) {
				path_p = songs[j_sel].paths[s];
				if (!path_p) break;
				if (s == showmax) { /* add a final entry if it fits exactly */
					path_p = path_p + strlen(path_p) - 1;
					while (path_p > songs[j_sel].paths[s] && *(path_p - 1) != '/') path_p--;
					if (music_cur == j_sel && music_cur_song == s)
						snprintf(tmp_lastslot, MAX_CHARS, "\377%c%2d. %s", songs[j_sel].initial[s] ? 'o' : 'y', s + 1, path_p);
					else
						snprintf(tmp_lastslot, MAX_CHARS, "\377%c%2d. %s", songs[j_sel].initial[s] ? 'U' : 'u', s + 1, path_p);
				}
				if (s >= showmax) continue;
				path_p = path_p + strlen(path_p) - 1;
				while (path_p > songs[j_sel].paths[s] && *(path_p - 1) != '/') path_p--;
				if (music_cur == j_sel && music_cur_song == s)
					Term_putstr(7 - 4, offs + 1 + s, -1, songs[j_sel].initial[s] ? TERM_ORANGE : TERM_YELLOW, format("%2d. %s%s", s + 1, songs[j_sel].is_reference[s] ? "\377s(R) \377-" : "", path_p)); //currently no different colour for songs[].disabled, consistent with 'Key' info
				else
					Term_putstr(7 - 4, offs + 1 + s, -1, songs[j_sel].initial[s] ? TERM_L_UMBER : TERM_UMBER, format("%2d. %s%s", s + 1, songs[j_sel].is_reference[s] ? "\377s(R) \377-" : "", path_p)); //currently no different colour for songs[].disabled, consistent with 'Key' info
			}
			if (s == showmax + 1) {
				Term_putstr(7 - 4, MAX_WINDOW_HGT - 1, -1, TERM_SLATE, tmp_lastslot);
				Term_putstr(0, offs, -1, TERM_WHITE, format("List of the first %d subsong filenames (found \377y%d\377w) of the selected music event:", showmax + 1, s));
			} else if (s > showmax + 1) {
				Term_putstr(7 - 4, MAX_WINDOW_HGT - 1, -1, TERM_SLATE, format("... and %d more...", s - showmax));
				Term_putstr(0, offs, -1, TERM_WHITE, format("List of the first %d subsong filenames (found \377y%d\377w) of the selected music event:", showmax, s));
			} else
				Term_putstr(0, offs, -1, TERM_WHITE, format("List of the first %d subsong filenames (found \377y%d\377w) of the selected music event:", showmax + 1, s));
		}

		/* display static selector */
		Term_putstr(horiz_offset + 1, vertikal_offset + list_size, -1, jukebox_play_all ? TERM_SEL_BLUE : TERM_SELECTOR, ">>>");
		Term_putstr(horiz_offset + 1 + 12 + 50 + 10, vertikal_offset + list_size, -1, jukebox_play_all ? TERM_SEL_BLUE : TERM_SELECTOR, "<<<");

		/* Place Cursor */
		//Term_gotoxy(20, vertikal_offset + y);
		/* hack: hide cursor */
		Term->scr->cx = Term->wid;
		Term->scr->cu = 1;

		/* Get key */
#ifdef ENABLE_SHIFT_SPECIALKEYS
		inkey_shift_special = 0x0;
#endif
		ch = inkey();

		/* Hack to auto-update song info while in 'play-all' automatic mode, or for jukebox_gamemusicchanged, with inkey hack (-1) */
		if (ch == -1) {
			if (jukebox_play_all) {
				jukebox_play_all_prev = jukebox_playing;
				jukebox_play_all_prev_song = jukebox_playing_song;
			}

			/* Locate currently playing song, select it visually */
			d = -1;
			for (j = 0; j < MUSIC_MAX; j++) {
				if (!songs[j].config) continue;
				d++;
				if (j != jukebox_playing) continue;
				break;
			}
			if (j != MUSIC_MAX) j_sel = y = d; //paranoia, should always be true
			continue;
		}

		/* Analyze */
		switch (ch) {
#if 0 //already filtered above
		case -1: /* this is just the jukebox_play_all inkey hack, used to just initiate a redraw,
			    drop it here (so we don't beep() in 'default:') and thereby proceed to redraw: */
			continue;
#endif
		case ESCAPE:
#ifdef ENABLE_JUKEBOX
			/* Restore real mixer settings */
			cfg_audio_master = cfg_audio_master_org;
			cfg_audio_sound = cfg_audio_sound_org;
			cfg_audio_music = cfg_audio_music_org;
			cfg_audio_weather = cfg_audio_weather_org;

			jukebox_static200vol = FALSE;
			jukebox_playing = -1;
			jukebox_play_all = FALSE;

			/* (Assume we want to stop music:) */
			play = FALSE;
			/* Paranoia: Make sure to catch hacks (-2,-3,-4) and reroute them to 'no music'. These shouldn't really be able to occur here though: */
			if (jukebox_org < -1) jukebox_org = -1;
 #ifdef JUKEBOX_INSTANT_PLAY
			/* --- Note that this will also insta-halt current music if it happens to be <disabled> and different from our jukebox piece,
			   so no need for us to check here for songs[].disabled explicitely for that particular case.
			   However, if the currently jukeboxed song is the same one as the disabled one we do need to halt it. --- */
			/* Check if we want to stop music or play/resume music */
			if (jukebox_org == -1 || songs[jukebox_org].disabled)
				play_music_instantly(-2);//halt song instantly instead of fading out
 #else
			if (jukebox_org == -1 || songs[jukebox_org].disabled) {
				jukebox_playing = -1; //required for play_music() to work correctly
				play_music(-2);//fade-out song
			}
 #endif
			else if (jukebox_org != music_cur)
				play = TRUE;
			/* Handle paused music if it was the currently playing game music */
			else if (jukebox_paused) {
				jukebox_paused = FALSE;
				if (music_cur == jukebox_org) Mix_ResumeMusic();
				/* But if (a) the music event meanwhile changed or (b) if we changed the subsong in the jukebox...
				   a) don't resume the deprecated music but play the new one instead
				   b) return to the old subsong  */
				play = jukebox_used;
			} else play = jukebox_used; //same music event but we changed the subsong or enabled/disabled the active event, thereb halting/replaying it?
			/* Note that even if the music event is the same AND subsong is the same, if we used the jukebox, calling play_music_instantly() in the process,
			   we usually do want to restart the song just to make sure everything is back in clean state (paranoia++) */

			/* Play the correct in-game music again */
			if (play) {
				/* Resume playing the raw way, for better control and restoration of playing state */
				Mix_Music *wave = NULL;

				/* Still the same music event? */
				if (jukebox_org == music_cur) {
					/* Note: Even if the subsong is the same, we still restart playing it, because we want to ensure setting the correct 'repeat' value in any case. */
					music_cur_song = jukebox_org_song;
					music_cur_repeat = jukebox_org_repeat;

					/* Choose the predetermined random event */
					wave = songs[music_cur].wavs[music_cur_song];

					/* Try loading it, if it's not cached */
					if (!wave && !(wave = load_song(music_cur, music_cur_song))) {
						/* we really failed to load it */
						plog(format("SDL music load failed (%d, %d) [1].", music_cur, music_cur_song));
						puts(format("SDL music load failed (%d, %d) [1].", music_cur, music_cur_song));
						return;
					}
					/* In case the current music was played via play_music_vol() at reduced volume */
					if (jukebox_org_vol != 100) {
						int vols = 100;

 #ifdef USER_VOLUME_MUS
						/* Apply user-defined custom volume modifier */
						if (songs[music_cur].volume) vols = songs[music_cur].volume;
 #endif
						music_vol = jukebox_org_vol;
						Mix_VolumeMusic(CALC_MIX_VOLUME(cfg_audio_music, (cfg_audio_music_volume * evlt[(int)music_vol]) / MIX_MAX_VOLUME, vols));
					}
					/* Only really play anything if we actually used the jukebox at all! */
					if (jukebox_used) Mix_FadeInMusic(wave, music_cur_repeat, 1000);
				}
				/* Music event was changed meanwhile */
				else {
					jukebox_playing = -1; //required for play_music() to work correctly
					if (jukebox_org_vol == 100) play_music(jukebox_org);
					else play_music_vol(jukebox_org, jukebox_org_vol);
				}
			} else {
				/* If a song was "playing silently" ie just disabled, restore its (silenced) playing state. Because the -2 call further above would just set music_cur to -1. */
				music_cur = jukebox_org;
				music_cur_song = jukebox_org_song;
				music_cur_repeat = jukebox_org_repeat;
				music_vol = jukebox_org_vol;
			}

			jukebox_org = -1;
			curmus_timepos = -1; //no more song is playing in the jukebox now
			jukebox_screen = FALSE;
			topline_icky = FALSE;

 #if 0 /* this was okay before jukebox_play_all was added... */
			/* If music was actually 'off' in the mixer, apply that - it just means volume is 0, not that music is halted!
			   (Reason: If it gets halted, it won't be reenabled by toggling the music mixer switch anymore, if DISABLE_MUTED_AUDIO is not defined.) */
			if (!cfg_audio_music || !cfg_audio_master) set_mixing();
 #else /* ...it mutes everything when the playlist ends! So we need to re-mix here, to make already enabled music audible again: */
			set_mixing();
 #endif
#endif

			/* auto-save */

			/* -- save disabled info -- */

			path_build(buf, 1024, ANGBAND_DIR_XTRA_MUSIC, "music.cfg");
#ifndef WINDOWS
			path_build(buf2, 1024, ANGBAND_DIR_XTRA_MUSIC, "music.$$$");
#else
			if (!win_dontmoveuser && getenv("HOMEDRIVE") && getenv("HOMEPATH"))\
				sprintf(buf2, "%s\\TomeNET-%s-disabled.cfg", ANGBAND_DIR_USER, cfg_musicpackfolder);
			else path_build(buf2, sizeof(path), ANGBAND_DIR_XTRA_MUSIC, "TomeNET-nomusic.cfg"); //paranoia
#endif
			fff = my_fopen(buf, "r");
			fff2 = my_fopen(buf2, "w");
			if (!fff) {
				c_msg_format("Error: Cannot read music config file '%s'.", buf);
				jukebox_screen = FALSE;
				topline_icky = FALSE;
				return;
			}
			if (!fff2) {
				c_msg_format("Error: Cannot write to disabled-music config file '%s'.", buf2);
				jukebox_screen = FALSE;
				topline_icky = FALSE;
				return;
			}
			while (TRUE) {
				if (!fgets(out_val, 4096, fff)) {
					if (ferror(fff)) c_msg_format("Error: Failed to read from music config file '%s'.", buf);
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

			/* -- save volume info -- */

			path_build(buf, 1024, ANGBAND_DIR_XTRA_MUSIC, "music.cfg");
#ifndef WINDOWS
			path_build(buf2, 1024, ANGBAND_DIR_XTRA_MUSIC, "TomeNET-musicvol.cfg");
#else
			if (!win_dontmoveuser && getenv("HOMEDRIVE") && getenv("HOMEPATH"))
				sprintf(buf2, "%s\\TomeNET-%s-volume.cfg", ANGBAND_DIR_USER, cfg_musicpackfolder);
			else path_build(buf2, sizeof(path), ANGBAND_DIR_XTRA_MUSIC, "TomeNET-musicvol.cfg"); //paranoia
#endif
			fff = my_fopen(buf, "r");
			fff2 = my_fopen(buf2, "w");
			if (!fff) {
				c_msg_format("Error: Cannot read music config file '%s'.", buf);
				jukebox_screen = FALSE;
				topline_icky = FALSE;
				return;
			}
			if (!fff2) {
				c_msg_format("Error: Cannot write to music volume config file '%s'.", buf2);
				jukebox_screen = FALSE;
				topline_icky = FALSE;
				return;
			}
			while (TRUE) {
				if (!fgets(out_val, 4096, fff)) {
					if (ferror(fff)) c_msg_format("Error: Failed to read from music config file '%s'.", buf);
					break;
				}

				p = out_val;
				/* remove comment-character */
				if (*p == ';') p++;

				/* ignores lines that don't start on a letter */
				if (tolower(*p) < 'a' || tolower(*p) > 'z') continue;

				/* extract event name */
				strcpy(evname, p);
				*(strchr(evname, ' ')) = 0;

				/* find out event state (disabled/enabled) */
				j = exec_lua(0, format("return get_music_index(\"%s\")", evname));
				if (j == -1 || !songs[j].config) continue;

				/* apply new state */
				if (songs[j].volume) {
					strcpy(out_val2, evname);
					strcat(out_val2, "\n");
					fputs(out_val2, fff2);
					sprintf(out_val2, "%d\n", songs[j].volume);
					fputs(out_val2, fff2);
				}
			}
			fclose(fff);
			fclose(fff2);

			/* -- all done -- */
			go = FALSE;
			break;

#ifdef USER_VOLUME_MUS
		case 'v': {
			//i = c_get_quantity("Enter volume % (1..100): ", 50, 100);
			bool inkey_msg_old = inkey_msg;
			char tmp[80];

			jukebox_static200vol = FALSE;
			inkey_msg = TRUE;
			Term_putstr(0, 3, -1, TERM_L_BLUE, "                                                                              ");
			Term_putstr(0, 3, -1, TERM_L_BLUE, " Enter volume % (1..200, m to max, other values will reset to 100%): ");
			strcpy(tmp, "100");
			if (!askfor_aux(tmp, 4, 0)) {
				inkey_msg = inkey_msg_old;
				songs[j_sel].volume = 0;
				break;
			}
			inkey_msg = inkey_msg_old;
			if (tmp[0] == 'm') {
				for (i = 100; i < 200; i++)
					if (CALC_MIX_VOLUME(cfg_audio_music, cfg_audio_music_volume, i) == MIX_MAX_VOLUME) break;
			} else {
				i = atoi(tmp);
				if (i < 1 || i == 100) i = 0;
				else if (i > 200) i = 200;
			}
			songs[j_sel].volume = i;

			/* If song is currently playing, adjust volume live.
			   (Note: If the selected song was already playing in-game via play_music_vol() this will ovewrite the volume
			   and cause 'wrong' volume, but when it's actually re-played via play_music_vol() the volume will be correct.) */
			if (!i) i = 100; /* Revert to default volume */
			if (j_sel == music_cur) {
				int vn = CALC_MIX_VOLUME(cfg_audio_music, cfg_audio_music_volume, i);

				/* QoL: Notify if this high a boost value won't make a difference under the current mixer settings */
				if (i > 100 && vn == MIX_MAX_VOLUME && CALC_MIX_VOLUME(cfg_audio_music, cfg_audio_music_volume, i - 1) == MIX_MAX_VOLUME)
					c_msg_format("\377w(%d%% boost exceeds maximum possible volume at current mixer settings.)", i);

				Mix_VolumeMusic(vn);
			}
			break;
			}

		case '+':
			jukebox_static200vol = FALSE;
			i = songs[j_sel].volume;
			if (!i) i = 100;
			i += 10;
			if (i == 11) i = 10; //min is 1, not 0, compensate in this opposite direction
			if (i == 100) i = 0;
			else if (i > 200) i = 200;
			songs[j_sel].volume = i;

			/* If song is currently playing, adjust volume live.
			   (Note: If the selected song was already playing in-game via play_music_vol() this will ovewrite the volume
			   and cause 'wrong' volume, but when it's actually re-played via play_music_vol() the volume will be correct.) */
			if (!i) i = 100; /* Revert to default volume */
			if (j_sel == music_cur) Mix_VolumeMusic(CALC_MIX_VOLUME(cfg_audio_music, cfg_audio_music_volume, i));
			break;

		case '-':
			jukebox_static200vol = FALSE;
			i = songs[j_sel].volume;
			if (!i) i = 100;
			i -= 10;
			if (i == 100) i = 0;
			else if (i < 1) i = 1;
			songs[j_sel].volume = i;

			/* If song is currently playing, adjust volume live.
			   (Note: If the selected song was already playing in-game via play_music_vol() this will ovewrite the volume
			   and cause 'wrong' volume, but when it's actually re-played via play_music_vol() the volume will be correct.) */
			if (!i) i = 100; /* Revert to default volume */
			if (j_sel == music_cur) Mix_VolumeMusic(CALC_MIX_VOLUME(cfg_audio_music, cfg_audio_music_volume, i));
			break;
#endif

		case 'c': /* Jump to currently playing song */
			i = 0;
			for (j = 0; j < MUSIC_MAX; j++) {
				/* skip songs that we don't have defined */
				if (!songs[j].config) {
					i++;
					continue;
				}
				/* playing atm? */
				if (j != music_cur) continue;
				/* match */
				j_sel = y = j - i;
				break;
			}
			break;

		case KTRL('T'):
			/* Take a screenshot */
			xhtml_screenshot("screenshot????", 2);
			break;

		case ':': {
			bool inkey_msg_old = inkey_msg;

			/* specialty: allow chatting from within here */
			cmd_message();
			inkey_msg = inkey_msg_old;
			break;
			}

		case 't': //case ' ':
			songs[j_sel].disabled = !songs[j_sel].disabled;
			if (songs[j_sel].disabled) {
				if (music_cur == j_sel && Mix_PlayingMusic()) {
					d = music_cur; //halting will set cur to -1
					jukebox_used = TRUE; //we actually used the jukebox
					Mix_HaltMusic();
					music_cur = d;
					curmus_song_dur = 0;
				}
			} else {
				if (music_cur == j_sel) {
					int jbo = jukebox_org; //the play_music() call below will modify jukebox_org wrongly, so we have to keep it here and restore it afterwards

					music_cur = -1; //allow restarting it
					jukebox_screen = FALSE; /* Hack: play_music(), unlike play_music_instantly(), aborts if it detects jukebox-specific operations */
					jukebox_paused = FALSE;
					jukebox_used = TRUE; //we actually used the jukebox
					if (jukebox_org_vol == 100) play_music(j_sel);
					else play_music_vol(j_sel, jukebox_org_vol);
					jukebox_screen = TRUE;

					jukebox_org = jbo;
					jukebox_update_songlength();
				}
			}
			break;

		case 'y':
			if (!songs[j_sel].disabled) break;
			songs[j_sel].disabled = FALSE;
			if (music_cur == j_sel) {
				int jbo = jukebox_org; //the play_music() call below will modify jukebox_org wrongly, so we have to keep it here and restore it afterwards

				music_cur = -1; //allow restarting it
				jukebox_screen = FALSE; /* Hack: play_music(), unlike play_music_instantly(), aborts if it detects jukebox-specific operations */
				jukebox_paused = FALSE;
				jukebox_used = TRUE; //we actually used the jukebox
				if (jukebox_org_vol == 100) play_music(j_sel);
				else play_music_vol(j_sel, jukebox_org_vol);
				jukebox_screen = TRUE;

				jukebox_org = jbo;
				jukebox_update_songlength();
			}
			break;

		case 'n':
			if (songs[j_sel].disabled) break;
			songs[j_sel].disabled = TRUE;
			if (music_cur == j_sel && Mix_PlayingMusic()) {
				d = music_cur; //halting will set cur to -1
				jukebox_used = TRUE; //we actually used the jukebox
				Mix_HaltMusic();
				music_cur = d;
				curmus_song_dur = 0;
			}
			break;

#ifdef ENABLE_JUKEBOX
		case '\n':
		case '\r':
			/* Force-enable the mixer to play music */
			if (!cfg_audio_master) {
				cfg_audio_master = TRUE;
				cfg_audio_sound = FALSE;
				cfg_audio_weather = FALSE;
			}
			cfg_audio_music = TRUE;
			jukebox_play_all = FALSE;

 #ifdef JUKEBOX_INSTANT_PLAY
			dis = songs[j_sel].disabled;
			songs[j_sel].disabled = FALSE;

			jukebox_playing = j_sel;
			jukebox_used = TRUE; //we actually used the jukebox
			jukebox_paused = FALSE;
			play_music_instantly(j_sel);
  #ifdef ENABLE_SHIFT_SPECIALKEYS
			if (inkey_shift_special == 0x1) {
				Mix_VolumeMusic(CALC_MIX_VOLUME(cfg_audio_music, cfg_audio_music_volume, 200)); /* SHIFT+ENTER: Play at maximum allowed volume aka 200% boost. */
				jukebox_static200vol = TRUE;
			} else jukebox_static200vol = FALSE;
  #endif

			songs[j_sel].disabled = dis;
 #else
			if (j_sel == music_cur) break;
			if (songs[j_sel].disabled) break;

			jukebox_playing = j_sel;
			jukebox_used = TRUE; //we actually used the jukebox
			jukebox_paused = FALSE;
			play_music(j_sel);
  #ifdef ENABLE_SHIFT_SPECIALKEYS
			if (inkey_shift_special == 0x1) Mix_VolumeMusic(CALC_MIX_VOLUME(cfg_audio_music, cfg_audio_music_volume, 200)); /* SHIFT+ENTER: Play at maximum allowed volume aka 200% boost. */
  #endif
 #endif

			jukebox_update_songlength();
 #if 0 /* paranoia/not needed */
			/* Reset position */
			Mix_SetMusicPosition(0);
 #endif
			curmus_timepos = 0; //song starts to play, at 0 seconds mark ie the beginning
			break;

		case 'a':
		case 'A':
		case 'u':
		case 'U':
			//(Note: j_sel and y will be set by the 'ch == -1' hack above, so we don't need to take care of those here)

			/* Force-enable the mixer to play music */
			if (!cfg_audio_master) {
				cfg_audio_master = TRUE;
				cfg_audio_sound = FALSE;
				cfg_audio_weather = FALSE;
			}
			cfg_audio_music = TRUE;

			jukebox_shuffle = (ch == 'u' || ch == 'U');
			/* Prepare shuffle map in any case, and just use it as non-shuffle (ie identity) map in case we don't want to shuffle.
			   Easier than managing two different routines in each place (ie one for shuffling, one for linear). */
			for (j = 0; j < MUSIC_MAX; j++) jukebox_shuffle_map[j] = j;
			/* Actually do shuffle? */
			if (jukebox_shuffle) intshuffle(jukebox_shuffle_map, MUSIC_MAX);

			/* Find first song */
 #ifdef JUKEBOX_INSTANT_PLAY
			for (j = 0; j < MUSIC_MAX; j++) {
				d = jukebox_shuffle_map[j];
				if (!songs[d].config || songs[d].disabled) continue;
				break;
			}
			if (j == MUSIC_MAX) break; //edge case: a music pack without a single event defined >_>

			jukebox_playing = d;
			jukebox_used = TRUE; //we actually used the jukebox
			jukebox_play_all = TRUE;
			jukebox_play_all_prev = jukebox_play_all_prev_song = -1;

			jukebox_paused = FALSE;
			play_music_instantly(d);
			if (ch == 'A' || ch == 'U') {
				Mix_VolumeMusic(CALC_MIX_VOLUME(cfg_audio_music, cfg_audio_music_volume, 200)); /* SHIFT: Play at maximum allowed volume aka 200% boost. */
				jukebox_static200vol = TRUE;
			} else jukebox_static200vol = FALSE;
 #else
			for (j = 0; j < MUSIC_MAX; j++) {
				d = jukebox_shuffle_map[j];
				if (!songs[d].config || songs[d].disabled) continue;
				break;
			}
			if (j == MUSIC_MAX) break; //edge case: a music pack without a single event defined >_>

			jukebox_playing = d;
			jukebox_used = TRUE; //we actually used the jukebox
			jukebox_play_all = TRUE;
			jukebox_play_all_prev = jukebox_play_all_prev_song = -1;

			jukebox_paused = FALSE;
			play_music(d);
			if (ch == 'A' || ch == 'U') Mix_VolumeMusic(CALC_MIX_VOLUME(cfg_audio_music, cfg_audio_music_volume, 200)); /* SHIFT: Play at maximum allowed volume aka 200% boost. */
 #endif

			jukebox_update_songlength();
 #if 0 /* paranoia/not needed */
			/* Reset position */
			Mix_SetMusicPosition(0);
 #endif
			curmus_timepos = 0; //song starts to play, at 0 seconds mark ie the beginning
			break;
#endif

#ifdef ENABLE_JUKEBOX
 #ifdef JUKEBOX_INSTANT_PLAY
		case 'w': /* Skip to next subsong */
			jukebox_used = TRUE; //we actually used the jukebox
			/* We're playing a single music event - skip through its subsongs, backwards */
			if (jukebox_playing != -1 && !jukebox_play_all) { //jukebox_subonly_play_all) {
				d = music_cur;
				/* Note that this will auto-advance the subsong if d is already == jukebox_playing: */
				jukebox_paused = FALSE;

				dis = songs[d].disabled;
				songs[d].disabled = FALSE;
				play_music_instantly(d);
				songs[d].disabled = dis;

				if (jukebox_static200vol) Mix_VolumeMusic(CALC_MIX_VOLUME(cfg_audio_music, cfg_audio_music_volume, 200));

				jukebox_update_songlength();
  #if 0 /* paranoia/not needed */
				/* Reset position */
				Mix_SetMusicPosition(0);
  #endif
				curmus_timepos = 0; //song starts to play, at 0 seconds mark ie the beginning
				break;
			}

			/* Skip current sub-song, possibly advancing to next music event if this was the last subsong of the current music event */
			if (jukebox_playing == -1 || !jukebox_play_all) continue;

			d = music_cur;
			/* After having played all subsongs, advance to the next music event, starting at song #0 again */
			if (music_cur_song == songs[music_cur].num - 1) {
				bool look_for_next = TRUE; //required for handling shuffle

				for (j = 0; j < MUSIC_MAX; j++) {
					d = jukebox_shuffle_map[j];
					if (look_for_next) {
						if (d != music_cur) continue; //skip all songs we've heard so far
						look_for_next = FALSE;
						continue;
					}
					if (!songs[d].config || songs[d].disabled) continue;
					break;
				}
				if (j == MUSIC_MAX) {
					continue; //hm, instead of ending auto-play, just ignore the key.

					jukebox_playing = -1;
					jukebox_static200vol = FALSE;
					jukebox_play_all = FALSE;

					//pseudo-turn off music to indicate that our play-all 'playlist' has finished
					cfg_audio_music = FALSE;
					set_mixing();

					d = jukebox_org;
				} else jukebox_playing = d;
				music_cur = -1; /* Prevent auto-advancing of play_music_instantly(), but freshly start at subsong #0 */
			}
			/* Note that this will auto-advance the subsong if d is already == jukebox_playing: */
			jukebox_paused = FALSE;

			dis = songs[d].disabled;
			songs[d].disabled = FALSE;
			play_music_instantly(d);
			songs[d].disabled = dis;

			if (jukebox_static200vol) Mix_VolumeMusic(CALC_MIX_VOLUME(cfg_audio_music, cfg_audio_music_volume, 200));

			jukebox_update_songlength();
  #if 0 /* paranoia/not needed */
			/* Reset position */
			Mix_SetMusicPosition(0);
  #endif
			curmus_timepos = 0; //song starts to play, at 0 seconds mark ie the beginning
			break;
		case 'W': { /* Skip to next music event */
			bool look_for_next = TRUE; //required for handling shuffle

			if (jukebox_playing == -1 || !jukebox_play_all) continue;
			for (j = 0; j < MUSIC_MAX; j++) {
				d = jukebox_shuffle_map[j];
				if (look_for_next) {
					if (d != music_cur) continue; //skip all songs we've heard so far
					look_for_next = FALSE;
					continue;
				}
				if (!songs[d].config || songs[d].disabled) continue;
				break;
			}
			if (j == MUSIC_MAX) {
				continue; //hm, instead of ending auto-play, just ignore the key.

				jukebox_playing = -1;
				jukebox_static200vol = FALSE;
				jukebox_play_all = FALSE;

				//pseudo-turn off music to indicate that our play-all 'playlist' has finished
				cfg_audio_music = FALSE;
				set_mixing();

				d = jukebox_org;
			} else jukebox_playing = d;
			music_cur = -1; /* Prevent auto-advancing of play_music_instantly(), but freshly start at subsong #0 */
			/* Note that this will auto-advance the subsong if d is already == jukebox_playing: */
			jukebox_paused = FALSE;
			jukebox_used = TRUE; //we actually used the jukebox

			play_music_instantly(d);

			if (jukebox_static200vol) Mix_VolumeMusic(CALC_MIX_VOLUME(cfg_audio_music, cfg_audio_music_volume, 200));

			jukebox_update_songlength();
  #if 0 /* paranoia/not needed */
			/* Reset position */
			Mix_SetMusicPosition(0);
  #endif
			curmus_timepos = 0; //song starts to play, at 0 seconds mark ie the beginning
			break;
			}

		case 'q': /* Skip to previous subsong */
			jukebox_used = TRUE; //we actually used the jukebox
			/* We're playing a single music event - skip through its subsongs, backwards */
			if (jukebox_playing != -1 && !jukebox_play_all) { //jukebox_subonly_play_all) {
				d = music_cur;
				if (music_cur_song == 0) {
					if (jukebox_playing != -1 && songs[d].num) {
						music_cur_song = songs[d].num - 2;
						if (music_cur_song == -1) music_cur = -1; //music_cur_song = 0;
					}
				} else {
					if (music_cur_song == 1) music_cur = -1; //music_cur_song = 0;
					else music_cur_song = music_cur_song - 2;
				}
				/* Note that this will auto-advance the subsong if d is already == jukebox_playing: */
				jukebox_paused = FALSE;

				dis = songs[d].disabled;
				songs[d].disabled = FALSE;
				play_music_instantly(d);
				songs[d].disabled = dis;

				if (jukebox_static200vol) Mix_VolumeMusic(CALC_MIX_VOLUME(cfg_audio_music, cfg_audio_music_volume, 200));

				jukebox_update_songlength();
  #if 0 /* paranoia/not needed */
				/* Reset position */
				Mix_SetMusicPosition(0);
  #endif
				curmus_timepos = 0; //song starts to play, at 0 seconds mark ie the beginning
				break;
			}

			/* Skip to previous sub-song, possibly regressing to previous music event if this was the first subsong of the current music event */
			if (jukebox_playing == -1 || !jukebox_play_all) continue;

			d = music_cur;
			/* After having played all subsongs, advance to the next music event, starting at song #0 again */
			if (music_cur_song == 0) {
				bool look_for_next = TRUE; //required for handling shuffle

				for (j = MUSIC_MAX - 1; j >= 0; j--) {
					d = jukebox_shuffle_map[j];
					if (look_for_next) {
						if (d != music_cur) continue; //skip all songs we've heard so far
						look_for_next = FALSE;
						continue;
					}
					if (!songs[d].config || songs[d].disabled) continue;
					break;
				}
				if (j == -1) {
					continue; //hm, instead of ending auto-play, just ignore the key.

					jukebox_playing = -1;
					jukebox_static200vol = FALSE;
					jukebox_play_all = FALSE;

					//pseudo-turn off music to indicate that our play-all 'playlist' has finished
					cfg_audio_music = FALSE;
					set_mixing();

					d = jukebox_org;
				} else jukebox_playing = d;
				music_cur = -1; /* Prevent auto-advancing of play_music_instantly(), but freshly start at subsong #0 */
				if (jukebox_playing != -1 && songs[d].num) {
					music_cur = d;
					music_cur_song = songs[d].num - 2;
					if (music_cur_song == -1) music_cur = -1; //music_cur_song = 0;
				}
			} else {
				if (music_cur_song == 1) music_cur = -1; //music_cur_song = 0;
				else music_cur_song = music_cur_song - 2;
			}
			/* Note that this will auto-advance the subsong if d is already == jukebox_playing: */
			jukebox_paused = FALSE;

			dis = songs[d].disabled;
			songs[d].disabled = FALSE;
			play_music_instantly(d);
			songs[d].disabled = dis;

			if (jukebox_static200vol) Mix_VolumeMusic(CALC_MIX_VOLUME(cfg_audio_music, cfg_audio_music_volume, 200));

			jukebox_update_songlength();
  #if 0 /* paranoia/not needed */
			/* Reset position */
			Mix_SetMusicPosition(0);
  #endif
			curmus_timepos = 0; //song starts to play, at 0 seconds mark ie the beginning
			break;
		case 'Q': { /* Skip to previous music event */
			bool look_for_next = TRUE; //required for handling shuffle

			if (jukebox_playing == -1 || !jukebox_play_all) continue;
			for (j = MUSIC_MAX - 1; j >= 0; j--) {
				d = jukebox_shuffle_map[j];
				if (look_for_next) {
					if (d != music_cur) continue; //skip all songs we've heard so far
					look_for_next = FALSE;
					continue;
				}
				if (!songs[d].config || songs[d].disabled) continue;
				break;
			}
			if (j == -1) {
				continue; //hm, instead of ending auto-play, just ignore the key.

				jukebox_playing = -1;
				jukebox_static200vol = FALSE;
				jukebox_play_all = FALSE;

				//pseudo-turn off music to indicate that our play-all 'playlist' has finished
				cfg_audio_music = FALSE;
				set_mixing();

				d = jukebox_org;
			} else jukebox_playing = d;
			music_cur = -1; /* Prevent auto-advancing of play_music_instantly(), but freshly start at subsong #0 */
			if (jukebox_playing != -1 && songs[d].num) {
				music_cur = d;
				music_cur_song = songs[d].num - 2;
				if (music_cur_song == -1) music_cur = -1; //music_cur_song = 0;
			}
			/* Note that this will auto-advance the subsong if d is already == jukebox_playing: */
			jukebox_paused = FALSE;
			jukebox_used = TRUE; //we actually used the jukebox
			play_music_instantly(d);
			if (jukebox_static200vol) Mix_VolumeMusic(CALC_MIX_VOLUME(cfg_audio_music, cfg_audio_music_volume, 200));

			jukebox_update_songlength();
  #if 0 /* paranoia/not needed */
			/* Reset position */
			Mix_SetMusicPosition(0);
  #endif
			curmus_timepos = 0; //song starts to play, at 0 seconds mark ie the beginning
			break;
			}
 #endif

		case 'P':
			if (!jukebox_paused) Mix_PauseMusic();
			else Mix_ResumeMusic();
			jukebox_paused = !jukebox_paused;
			break;
#endif

		case '#':
			i = c_get_quantity(format("  Enter music event index number (1-%d): ", audio_music), 1, audio_music) - 1;
			if (i < 0) break;
			y = i;
			if (y >= audio_music) y = audio_music - 1;
			break;
		case 's': /* Search for event name */
			Term_putstr(0, 0, -1, TERM_WHITE, "  Enter (partial) music event name: ");
			askfor_aux(searchstr, MAX_CHARS - 1, 0);
			if (!searchstr[0]) break;
			searchres = -1;
			searchforfilename = FALSE;

			/* Map events we've listed in our local config file onto audio.lua indices */
			i2 = -1;
			for (j = 0; j < MUSIC_MAX; j++) {
				if (!songs[j].config) continue;
				i2++;
				/* get event name */
				sprintf(out_val, "return get_music_name(%d)", j);
				lua_name = string_exec_lua(0, out_val);
				if (!my_strcasestr(lua_name, searchstr)) continue;
				/* match */
				y = i2;
				searchoffset = y;
				searchres = j;
				break;
			}
			break;
		case 'S': /* Search for file name */
			Term_putstr(0, 0, -1, TERM_WHITE, "  Enter (partial) music file name: ");
			askfor_aux(searchstr, MAX_CHARS - 1, 0);
			if (!searchstr[0]) break;
			searchres = -1;
			searchforfilename = TRUE;

			/* Map events we've listed in our local config file onto audio.lua indices */
			i2 = -1;
			for (j = 0; j < MUSIC_MAX; j++) {
				if (!songs[j].config) continue;
				i2++;
				/* get file name */
				for (d = 0; d < songs[j].num; d++)
					if (my_strcasestr(songs[j].paths[d], searchstr)) break;
				if (d == songs[j].num) continue;
				/* match */
				y = i2;
				searchoffset = y;
				searchres = j;
				break;
			}
			break;
		case '/': /* Search for next result */
			if (!searchstr[0]) continue;
			/* Map events we've listed in our local config file onto audio.lua indices */
			i2 = -1;
			searchoffset++; //we start searching at searchres + 1!
			for (d = searchres + 1; d <= searchres + MUSIC_MAX; d++) {
				j = d % MUSIC_MAX;
				if (!j) {
					i2 = -1;
					searchoffset = 0;
				}
				if (!songs[j].config) continue;
				i2++;

				if (searchforfilename) {
					/* get file name */
					for (k = 0; k < songs[j].num; k++)
						if (my_strcasestr(songs[j].paths[k], searchstr)) break;
					if (k == songs[j].num) continue;
				} else {
					/* get event name */
					sprintf(out_val, "return get_music_name(%d)", j);
					lua_name = string_exec_lua(0, out_val);
					if (!my_strcasestr(lua_name, searchstr)) continue;
				}

				/* match */
				y = i2 + searchoffset;
				searchoffset = y;
				searchres = j;
				break;
			}
			break;

		case NAVI_KEY_PAGEUP:
		case '9':
		case 'p':
			y = (y - list_size + audio_music) % audio_music;
			break;

		case NAVI_KEY_PAGEDOWN:
		case '3':
		case ' ':
			y = (y + list_size + audio_music) % audio_music;
			break;

		case NAVI_KEY_END:
		case '1':
		case 'G':
			y = audio_music - 1;
			break;

		case NAVI_KEY_POS1:
		case '7':
		case 'g':
			y = 0;
			break;

		case NAVI_KEY_UP:
		case '8':
		case NAVI_KEY_DOWN:
		case '2':
			if (ch == NAVI_KEY_UP) ch = '8';
			if (ch == NAVI_KEY_DOWN) ch = '2';
			d = keymap_dirs[ch & 0x7F];
			y = (y + ddy[d] + audio_music) % audio_music;
			break;

		case '\010':
			y = (y - 1 + audio_music) % audio_music;
			break;

		/* Skip forward/backward -- SDL_mixer (1.2.10) only supports ogg,mp3,mod apparently though, and it's a pita,
		   according to https://www.libsdl.org/projects/SDL_mixer/docs/SDL_mixer_65.html :
			MOD
				The double is cast to Uint16 and used for a pattern number in the module.
				Passing zero is similar to rewinding the song.
			OGG
				Jumps to position seconds from the beginning of the song.
			MP3
				Jumps to position seconds from the current position in the stream.
				So you may want to call Mix_RewindMusic before this.
				Does not go in reverse...negative values do nothing.
			Returns: 0 on success, or -1 if the codec doesn't support this function.
		   ..and worst, the is no way to retrieve the current music position, so we have to track it manually: curmus_timepos.
		*/
		case NAVI_KEY_LEFT:
		case '4':
			Mix_RewindMusic();
			curmus_timepos -= MUSIC_SKIP; /* Skip backward n seconds. */
			if (curmus_timepos < 0) {
				curmus_timepos = curmus_song_dur + curmus_timepos;
				if (curmus_timepos < 0) curmus_timepos = 0;
			}
			Mix_SetMusicPosition(curmus_timepos);
			curmus_timepos = (int)Mix_GetMusicPosition(songs[music_cur].wavs[music_cur_song]); //paranoia, sync
			break;

		case NAVI_KEY_RIGHT:
		case '6':
			Mix_RewindMusic();
			curmus_timepos += MUSIC_SKIP; /* Skip forward n seconds. */
			if (curmus_song_dur && curmus_timepos > curmus_song_dur) {
				curmus_timepos = curmus_timepos - curmus_song_dur;
				if (curmus_timepos > curmus_song_dur) curmus_timepos = 0;
			}
			Mix_SetMusicPosition(curmus_timepos);
			/* Overflow beyond actual song length? SDL2_mixer will then restart from 0, so adjust tracking accordingly */
			curmus_timepos = (int)Mix_GetMusicPosition(songs[music_cur].wavs[music_cur_song]);
			break;

		default:
			bell();
		}
	}

	topline_icky = FALSE;
	jukebox_screen = FALSE;
}

/* Assume jukebox_screen is TRUE aka we're currently in the jukebox UI. */
void update_jukebox_timepos(void) {
	int i;

	/* NOTE: Mix_GetMusicPosition() requires SDL2_mixer v2.6.0, which was released on 2022-07-08 and the first src package actually lacks build info for sdl2-config.
	   That means in case you install SDL2_mixer manually into /usr/local instead of /usr, you'll have to edit the makefile and replace sdl2-config calls with the
	   correctl7 prefixed paths to /usr/local/lib and /usr/loca/include instead of /usr/lib and /usr/include. -_-
	   Or you overwrite your repo version at /usr prefix instead. I guess best is to just wait till the SDL2_mixer package is in the official repository.. */
	if (music_cur != -1 && music_cur_song != -1)
		i = (int)Mix_GetMusicPosition(songs[music_cur].wavs[music_cur_song]); //catches when we reach end of song and restart playing at 0s
	else i = 0;

	curmus_timepos = i;
	/* Update jukebox song time stamp */
#if 0 /* just update song position */
	if (curmus_y != -1) Term_putstr(curmus_x + 34 + 7 - 1, curmus_y, -1, curmus_attr, format("%02d:%02d", i / 60, i % 60));
#else /* also update song duration */
	if (curmus_y != -1) {
		Term_putstr(curmus_x + 34 + 7 - 1, curmus_y, -1, curmus_attr, 			format("%02d:%02d", i / 60, i % 60));
		if (!curmus_song_dur) Term_putstr(curmus_x + 34 + 6 - 1, curmus_y, -1, curmus_attr,format("(--:--)"));
		else Term_putstr(curmus_x + 34 + 12 - 1, curmus_y, -1, curmus_attr, 		     format("/%02d:%02d)", curmus_song_dur / 60, curmus_song_dur % 60));
	}
#endif

	/* Hack: Hide the cursor */
	Term->scr->cx = Term->wid;
	Term->scr->cu = 1;
}

#endif /* SOUND_SDL */
