/* $Id$ */
/* Main client module */

/*
 * This is intentionally minimal, as system specific stuff may
 * need be done in "main" (or "WinMain" for Windows systems).
 * The real non-system-specific initialization is done in
 * "c-init.c".
 */

#include "angband.h"


static char mangrc_filename[100] = "";

/* linux clients: load subwindow prefs from .tomenetrc - C. Blue */
static void read_mangrc_aux(int t, cptr sec_name) {
	char *val, *val2;

	/* strip leading tabs and leading spaces */
	val = strchr(sec_name, '\t');
	val2 = strchr(sec_name, ' ');
	if (!val && !val2) return; /* paranoia: bad config file */
	if (val && val2) {
		if (val2 < val) val = val2;
	} else if (!val) val = val2;
	val2 = val;
	while (*val == ' ' || *val == '\t') val++;
	strcpy(val2, val);
	/* strip trailing linefeeds */
	while (val2[strlen(val2) - 1] == '\n' || val2[strlen(val2) - 1] == '\r') val2[strlen(val2) - 1] = '\0';

	if (t != 0) { /* exempt main window */
		if ((val = strstr(sec_name, "_Title")))
			strcpy(ang_term_name[t], val + 6);
	}

	if ((val = strstr(sec_name, "_Visible")))
		term_prefs[t].visible = (atoi(val + 8) != 0);

	if ((val = strstr(sec_name, "_X")))
		term_prefs[t].x = atoi(val + 2);
	if ((val = strstr(sec_name, "_Y")))
		term_prefs[t].y = atoi(val + 2);

	if (t != 0) {
		if ((val = strstr(sec_name, "_Columns")))
			term_prefs[t].columns = atoi(val + 8);
		if ((val = strstr(sec_name, "_Lines")))
			term_prefs[t].lines = atoi(val + 6);
	}

	if ((val = strstr(sec_name, "_Font"))) {
#if 0 /* without tab/space stripping */
		while (val[0] && (val[0] < '0' || val[0] > '9')) val++;
		val2 = val;
		while (val2[0] && ((val2[0] >= '0' && val2[0] <= '9') || val2[0] == 'x')) val2++;
		val2[0] = 0;
		strcpy(term_prefs[t].font, val);
#else /* also allow font names consisting of letters! */
		strcpy(term_prefs[t].font, val + 5);
#endif
	}
}
static bool read_mangrc(cptr filename)
{
	FILE *config;
	char buf[1024];
	bool skip = FALSE;

	/* empty filename means use default */
	if (!strlen(filename)) {
#ifdef USE_EMX
		filename = "tomenet.rc";
#else
		filename = ".tomenetrc";
#endif

		/* Try to find home directory */
		if (getenv("HOME")) {
			/* Use home directory as base */
			strcpy(mangrc_filename, getenv("HOME"));
		}
		/* Otherwise use current directory */
		else {
			/* Current directory */
			strcpy(mangrc_filename, ".");
		}

		/* Append filename */
#ifdef USE_EMX
		strcat(mangrc_filename, "\\");
#else
		strcat(mangrc_filename, "/");
#endif
		strcat(mangrc_filename, filename);
	} else {
		strcpy(mangrc_filename, filename);
	}

	/* Attempt to open file */
	if ((config = fopen(mangrc_filename, "r")))
	{
		/* Read until end */
		while (!feof(config))
		{
			/* Get a line */
			if (!fgets(buf, 1024, config)) break;

			/* Skip comments, empty lines */
			if (buf[0] == '\n' || buf[0] == '#')
				continue;

			/* Name line */
			if (!strncmp(buf, "nick", 4))
			{
				char *name;

				/* Extract name */
				name = strtok(buf, " \t\n");
				name = strtok(NULL, "\t\n");

				/* Default nickname */
				strcpy(nick, name);
			}

			/* Password line */
			if (!strncmp(buf, "pass", 4))
			{
				char *p;

				/* Extract password */
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");

				/* Default password */
				strcpy(pass, p);
			}

			if (!strncmp(buf, "name", 4))
			{
				char *name;

				name = strtok(buf, " \t\n");
				name = strtok(NULL, "\t\n");

				strcpy(cname, name);
			}

			/* meta server line */
			if (!strncmp(buf, "meta", 4))
			{
				char *p;

				p = strtok(buf, " :\t\n");
				p = strtok(NULL, ":\t\n");

				if (p) strcpy(meta_address, p);
			}

			/* game server line (note: should be named 'host' for consistency with tomenet.ini) */
			if (!strncmp(buf, "server", 6))
			{
				char *p;

				p = strtok(buf, " :\t\n");
				p = strtok(NULL, ":\t\n");

				if (p)
				{
					strcpy(svname, p);
					p = strtok(NULL, ":\t\n");
					if (p) cfg_game_port = atoi(p);
				}
			}

			/* port line */
			if (!strncmp(buf, "port", 4))
			{
				char *p;

				/* Extract password */
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");

				/* Default password */
				if (p) cfg_game_port = atoi(p);
			}

			/* fps line */
			if (!strncmp(buf, "fps", 3))
			{
				char *p;

				/* Extract fps */
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");

				if (p) cfg_client_fps = atoi(p);
			}

			/* unix username line */
			if (!strncmp(buf, "realname", 8))
			{
				char *p;

				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");

				if (p) strcpy(real_name, p);
			}

			/* Path line */
			if (!strncmp(buf, "path", 4))
			{
				char *p;

				/* Extract path */
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");

				/* Default path */
				strcpy(path, p);
			}

			/* Auto-login */
			if (!strncmp(buf, "fullauto", 8))
				skip = TRUE;

#ifdef USE_GRAPHICS
			/* graphics */
			if (!strncmp(buf, "graphics", 8)) {
				char *p;
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");
				if (p) use_graphics = (atoi(p) != 0);
			}
#endif
#ifdef USE_SOUND
			/* sound */
			if (!strncmp(buf, "sound", 5)) {
				char *p;
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");
				if (p) {
					use_sound_org = use_sound = (atoi(p) != 0);
					if (!use_sound) quiet_mode = TRUE;
				}
			}
#endif
#ifdef USE_SOUND_2010
			/* sound hint */
			if (!strncmp(buf, "hintSound", 9)) sound_hint = FALSE;
			/* don't cache audio (41.5 MB of ogg samples deflated in memory!) */
			if (!strncmp(buf, "cacheAudio", 10)) {
				char *p;
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");
				if (p) no_cache_audio = !(atoi(p) != 0);
			}
			/* audio sample rate */
			if (!strncmp(buf, "audioSampleRate", 15)) {
				char *p;
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");
				if (p) cfg_audio_rate = atoi(p);
			}
			/* maximum number of allocated mixer channels */
			if (!strncmp(buf, "audioChannels", 13)) {
				char *p;
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");
				if (p) cfg_max_channels = atoi(p);
			}
			/* Mixed sample size (larger = more lagging sound, smaller = skipping on slow machines) */
			if (!strncmp(buf, "audioBuffer", 11)) {
				char *p;
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");
				if (p) cfg_audio_buffer = atoi(p);
			}
			/* audio mixer settings */
			if (!strncmp(buf, "audioMaster", 11)) {
				char *p;
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");
				if (p) cfg_audio_master = (atoi(p) != 0);
			}
			if (!strncmp(buf, "audioMusic", 10)) {
				char *p;
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");
				if (p) cfg_audio_music = (atoi(p) != 0);
			}
			if (!strncmp(buf, "audioSound", 10)) {
				char *p;
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");
				if (p) cfg_audio_sound = (atoi(p) != 0);
			}
			if (!strncmp(buf, "audioWeather", 12)) {
				char *p;
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");
				if (p) cfg_audio_weather = (atoi(p) != 0);
			}
			if (!strncmp(buf, "audioVolumeMaster", 17)) {
				char *p;
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");
				if (p) cfg_audio_master_volume = atoi(p);
			}
			if (!strncmp(buf, "audioVolumeMusic", 16)) {
				char *p;
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");
				if (p) cfg_audio_music_volume = atoi(p);
			}
			if (!strncmp(buf, "audioVolumeSound", 16)) {
				char *p;
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");
				if (p) cfg_audio_sound_volume = atoi(p);
			}
			if (!strncmp(buf, "audioVolumeWeather", 18)) {
				char *p;
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");
				if (p) cfg_audio_weather_volume = atoi(p);
			}
#endif

///LINUX_TERM_CFG
			if (!strncmp(buf, "Mainwindow", 10))
				read_mangrc_aux(0, buf);
			if (!strncmp(buf, "Mirrorwindow", 12))
				read_mangrc_aux(1, buf);
			if (!strncmp(buf, "Recallwindow", 12))
				read_mangrc_aux(2, buf);
			if (!strncmp(buf, "Choicewindow", 12))
				read_mangrc_aux(3, buf);
			if (!strncmp(buf, "Term-4window", 12))
				read_mangrc_aux(4, buf);
			if (!strncmp(buf, "Term-5window", 12))
				read_mangrc_aux(5, buf);
			if (!strncmp(buf, "Term-6window", 12))
				read_mangrc_aux(6, buf);
			if (!strncmp(buf, "Term-7window", 12))
				read_mangrc_aux(7, buf);
#if 0 /* keep n/a for now, not really needed */
			if (!strncmp(buf, "Term-8window", 12))
				read_mangrc_aux(8, buf);
			if (!strncmp(buf, "Term-9window", 12))
				read_mangrc_aux(9, buf);
#endif

			/*** Everything else is ignored ***/
		}
		fclose(config);
	}
	return (skip);
}

/* linux clients: save subwindow prefs to .tomenetrc - C. Blue */
static void write_mangrc_aux(int t, cptr sec_name, FILE *cfg_file) {
	int x, y, w, h;
	char font_name[1024];

	x11win_getinfo(t, &x, &y, &w, &h, font_name);
	printf("term %d: x %d, y %d, fnt %s.\n", t, x, y, font_name);

	if (t != 0) {
		fputs(format("%s_Title\t%s\n", sec_name, ang_term_name[t]), cfg_file);
		fputs(format("%s_Visible\t%c\n", sec_name, term_prefs[t].visible ? '1' : '0'), cfg_file);
	}
	if (term_prefs[t].x != -32000) /* don't save windows in minimized state */
		fputs(format("%s_X\t\t%d\n", sec_name, x), cfg_file);
//		fputs(format("%s_X\t\t%d\n", sec_name, term_prefs[t].x), cfg_file);
	if (term_prefs[t].y != -32000) /* don't save windows in minimized state */
		fputs(format("%s_Y\t\t%d\n", sec_name, y), cfg_file);
//		fputs(format("%s_Y\t\t%d\n", sec_name, term_prefs[t].y), cfg_file);
	if (t != 0) {
		fputs(format("%s_Columns\t%d\n", sec_name, term_prefs[t].columns), cfg_file);
		fputs(format("%s_Lines\t%d\n", sec_name, term_prefs[t].lines), cfg_file);
		fputs(format("%s_Font\t%s\n", sec_name, font_name), cfg_file);
	} else {
		/* one more tab, or formatting looks bad ;) */
		fputs(format("%s_Font\t\t%s\n", sec_name, font_name), cfg_file);
	}
	fputs("\n", cfg_file);
}
#ifdef USE_X11
/* linux clients: save one line of subwindow prefs to .tomenetrc - C. Blue */
static void write_mangrc_aux_line(int t, cptr sec_name, char *buf_org) {
	char buf[1024], *ter_name = buf_org + strlen(sec_name), font_name[1024];
	int x, y, w, h;

	x11win_getinfo(t, &x, &y, &w, &h, font_name);
	if (!w) return; /* invisible window? */
	printf("sec_name %s, line %s, term %d: x %d, y %d, w %d, h %d, font %s.\n", sec_name, buf, t, x, y, w, h, font_name);

	if (!strncmp(ter_name, "_Title", 6)) {
		if (t != 0)
			sprintf(buf, "%s_Title\t%s\n", sec_name, ang_term_name[t]);
	} else if (!strncmp(ter_name, "_Visible", 8)) {
		if (t != 0)
			sprintf(buf, "%s_Visible\t%c\n", sec_name, term_prefs[t].visible ? '1' : '0');
	} else if (!strncmp(ter_name, "_X", 2)) {
		if (term_prefs[t].x != -32000) /* don't save windows in minimized state */
			sprintf(buf, "%s_X\t\t%d\n", sec_name, x);
//			sprintf(buf, "%s_X\t\t%d\n", sec_name, term_prefs[t].x);
	} else if (!strncmp(ter_name, "_Y", 2)) {
		if (term_prefs[t].y != -32000) /* don't save windows in minimized state */
			sprintf(buf, "%s_Y\t\t%d\n", sec_name, y);
//			sprintf(buf, "%s_Y\t\t%d\n", sec_name, term_prefs[t].y);
	} else if (!strncmp(ter_name, "_Columns", 8)) {
		if (t != 0)
			sprintf(buf, "%s_Columns\t%d\n", sec_name, term_prefs[t].columns);
	} else if (!strncmp(ter_name, "_Lines", 6)) {
		if (t != 0)
			sprintf(buf, "%s_Lines\t%d\n", sec_name, term_prefs[t].lines);
	} else if (!strncmp(ter_name, "_Font", 5)) {
		if (t != 0)
			sprintf(buf, "%s_Font\t%s\n", sec_name, font_name);
		else
			/* one more tab, or formatting looks bad ;) */
			sprintf(buf, "%s_Font\t\t%s\n", sec_name, font_name);
	}

	strcpy(buf_org, buf);
}
#endif
/* linux clients: save some prefs to .tomenetrc - C. Blue */
bool write_mangrc(void) {
	char config_name2[100];
	FILE *config, *config2;
	char buf[1024];

	strcpy(config_name2, mangrc_filename);
	strcat(config_name2, ".$$$");

	config = fopen(mangrc_filename, "r");
	config2 = fopen(config_name2, "w");

	/* Attempt to open file */
	if (config2) {
	    if (config) {
		/* Read until end */
		while (!feof(config))
		{
			/* Get a line */
			if (!fgets(buf, 1024, config)) break;

#ifdef USE_SOUND_2010
			/* modify the line */
			/* audio mixer settings */
			if (!strncmp(buf, "audioMaster", 11)) {
				strcpy(buf, "audioMaster\t\t");
				strcat(buf, cfg_audio_master ? "1\n" : "0\n");
			} else if (!strncmp(buf, "audioMusic", 10)) {
				strcpy(buf, "audioMusic\t\t");
				strcat(buf, cfg_audio_music ? "1\n" : "0\n");
			} else if (!strncmp(buf, "audioSound", 10)) {
				strcpy(buf, "audioSound\t\t");
				strcat(buf, cfg_audio_sound ? "1\n" : "0\n");
			} else if (!strncmp(buf, "audioWeather", 12)) {
				strcpy(buf, "audioWeather\t\t");
				strcat(buf, cfg_audio_weather ? "1\n" : "0\n");
			} else if (!strncmp(buf, "audioVolumeMaster", 17)) {
				strcpy(buf, "audioVolumeMaster\t");
				strcat(buf, format("%d\n", cfg_audio_master_volume));
			} else if (!strncmp(buf, "audioVolumeMusic", 16)) {
				strcpy(buf, "audioVolumeMusic\t");
				strcat(buf, format("%d\n", cfg_audio_music_volume));
			} else if (!strncmp(buf, "audioVolumeSound", 16)) {
				strcpy(buf, "audioVolumeSound\t");
				strcat(buf, format("%d\n", cfg_audio_sound_volume));
			} else if (!strncmp(buf, "audioVolumeWeather", 18)) {
				strcpy(buf, "audioVolumeWeather\t");
				strcat(buf, format("%d\n", cfg_audio_weather_volume));
			}
#endif

#ifdef USE_X11
			/* new: save window positions/sizes/visibility (and possibly fonts) */
			if (!strncmp(buf, "Mainwindow", 10))
				write_mangrc_aux_line(0, "Mainwindow", buf);
			else if (!strncmp(buf, "Mirrorwindow", 12))
				write_mangrc_aux_line(1, "Mirrorwindow", buf);
			else if (!strncmp(buf, "Recallwindow", 12))
				write_mangrc_aux_line(2, "Recallwindow", buf);
			else if (!strncmp(buf, "Choicewindow", 12))
				write_mangrc_aux_line(3, "Choicewindow", buf);
			else if (!strncmp(buf, "Term-4window", 12))
				write_mangrc_aux_line(4, "Term-4window", buf);
			else if (!strncmp(buf, "Term-5window", 12))
				write_mangrc_aux_line(5, "Term-5window", buf);
			else if (!strncmp(buf, "Term-6window", 12))
				write_mangrc_aux_line(6, "Term-6window", buf);
			else if (!strncmp(buf, "Term-7window", 12))
				write_mangrc_aux_line(7, "Term-7window", buf);
#if 0 /* keep n/a for now, not really needed */
			else if (!strncmp(buf, "Term-8window", 12))
				write_mangrc_aux_line(8, "Term-8window", buf);
			else if (!strncmp(buf, "Term-9window", 12))
				write_mangrc_aux_line(9, "Term-9window", buf);
#endif
#endif /* USE_X11 */


			/*** Everything else is ignored ***/

			/* copy the line over */
			fputs(buf, config2);

#ifdef USE_SOUND_2010
			/* hack: disable one-time hint */
			if (!strncmp(buf, "sound", 5) && sound_hint) fputs("hintSound\n", config2);
#endif
		}
		fclose(config);
		fclose(config2);

		/* replace old by new */
		remove(mangrc_filename);
		rename(config_name2, mangrc_filename);

		return (TRUE); //success
	    } else {
		/* create .tomenetrc file, because it doesn't exist yet */
		fputs("# TomeNET config file\n", config2);
		fputs("# (basic version - generated automatically because it was missing)\n", config2);
		fputs("\n\n", config2);

		fputs(format("#nick\t\t%s\n", nick), config2);
		fputs(format("#pass\t\t%s\n", ""), config2);//keep pass secure maybe
		fputs(format("#name\t\t%s\n", cname), config2);
		fputs("\n", config2);

		fputs(format("#meta\t\t%s\n", ""), config2);//keep using internal defaults
		fputs(format("#server\t\t%s\n", svname), config2);
#if 0 /* let's keep empty in case newbie accidentally went to RPG server first and then uncomments this entry */
		fputs(format("#port\t\t%d\n", cfg_game_port), config2);
#else
		fputs(format("#port\t\t%s\n", ""), config2);
#endif
		fputs("\n", config2);

		fputs(format("fps\t\t%d\n", cfg_client_fps), config2);//or maybe just write '100'?
		fputs(format("#realname\t%s\n", real_name), config2);
		fputs(format("#path\t\t%s\n", path), config2);
		fputs("\n", config2);

		fputs("#fullauto\n", config2);
		fputs("\n", config2);

//#ifdef USE_GRAPHICS
		fputs(format("graphics\t\t%s\n", use_graphics ? "1" : "0"), config2);
		fputs("\n", config2);
//#endif
//#ifdef USE_SOUND
		fputs(format("sound\t\t\t%s\n", use_sound_org ? "1" : "0"), config2);
//#endif
//#ifdef USE_SOUND_2010
		fputs("hintSound\n", config2);
		fputs(format("cacheAudio\t\t%s\n", no_cache_audio ? "0" : "1"), config2);
		fputs(format("audioSampleRate\t\t%d\n", cfg_audio_rate), config2);
		fputs(format("audioChannels\t\t%d\n", cfg_max_channels), config2);
		fputs(format("audioBuffer\t\t%d\n", cfg_audio_buffer), config2);

		fputs(format("audioMaster\t\t%s\n", cfg_audio_master ? "1" : "0"), config2);
		fputs(format("audioMusic\t\t%s\n", cfg_audio_music ? "1" : "0"), config2);
		fputs(format("audioSound\t\t%s\n", cfg_audio_sound ? "1" : "0"), config2);
		fputs(format("audioWeather\t\t%s\n", cfg_audio_weather ? "1" : "0"), config2);
		fputs(format("audioVolumeMaster\t%d\n", cfg_audio_master_volume), config2);
		fputs(format("audioVolumeMusic\t%d\n", cfg_audio_music_volume), config2);
		fputs(format("audioVolumeSound\t%d\n", cfg_audio_sound_volume), config2);
		fputs(format("audioVolumeWeather\t%d\n", cfg_audio_weather_volume), config2);
		fputs("\n", config2);
//#endif

///LINUX_TERM_CFG
		write_mangrc_aux(0, "Mainwindow", config2);
		write_mangrc_aux(1, "Mirrorwindow", config2);
		write_mangrc_aux(2, "Recallwindow", config2);
		write_mangrc_aux(3, "Choicewindow", config2);
		write_mangrc_aux(4, "Term-4window", config2);
		write_mangrc_aux(5, "Term-5window", config2);
		write_mangrc_aux(6, "Term-6window", config2);
		write_mangrc_aux(7, "Term-7window", config2);
#if 0 /* keep n/a for now, not really needed */
		write_mangrc_aux(8, "Term-8window", config2);
		write_mangrc_aux(9, "Term-9window", config2);
#endif

		fclose(config2);

		/* rename temporary file to new ".tomenetrc" */
		rename(config_name2, mangrc_filename);
	    }
	}
	return (FALSE); //failure
}

static void default_set(void)
{
	char *temp;

#ifdef SET_UID
	int player_uid;
	struct passwd *pw;
#endif

	/* Initial defaults */
	strcpy(nick, "PLAYER");
	strcpy(pass, "passwd");
	strcpy(real_name, "PLAYER");


	/* Get login name if a UNIX machine */
#ifdef SET_UID
	/* Get player UID */
	player_uid = getuid();

	/* Get password entry */
	if ((pw = getpwuid(player_uid)))
	{
		/* Pull login id */
		strcpy(nick, pw->pw_name);

		/* Cut */
		nick[16] = '\0';

		/* Copy to real name */
		strcpy(real_name, nick);
	}
#endif

#ifdef AMIGA
        if((GetVar("mang_name",real_name,80,0L))!=-1){
          strcpy(nick,real_name);
	}
#endif
#ifdef SET_UID
	temp=getenv("TOMENET_PLAYER");
	if(temp) strcpy(nick, temp); 
	temp=getenv("TOMENET_USER");
	if(temp) strcpy(real_name, temp); 
#endif
}

int main(int argc, char **argv)
{
	int i, modus = 0;
	bool done = FALSE, skip = FALSE, force_cui = FALSE;

	/* Save the program name */
	argv0 = argv[0];


	/* Set default values */
	default_set();

	/* Acquire the version strings */
	version_build();

	/* read default rc file */
	skip = read_mangrc("");

	/* Process the command line arguments */
	for (i = 1; argv && (i < argc); i++)
	{
		/* Require proper options */
		if (argv[i][0] != '-')
		{
			if (modus && modus < 3)
			{
				if (modus == 2)
				{
					strcpy(pass, argv[i]);
					modus = 3;
				}
				else
				{
					strcpy(nick, argv[i]);
					modus = 2;
				}
			}
			else strcpy(svname, argv[i]);
			continue;
		}

		/* Analyze option */
		switch (argv[i][1])
		{
		/* ignore rc files */
		case 'i':
			skip = FALSE;
			strcpy(cname, "");
			strcpy(svname, "");
#ifdef USE_SOUND_2010
			cfg_audio_rate = 44100;
			cfg_max_channels = 32;
			cfg_audio_buffer = 1024;
			cfg_audio_master = cfg_audio_music = cfg_audio_sound = cfg_audio_weather = TRUE;
			cfg_audio_master_volume = cfg_audio_music_volume = cfg_audio_sound_volume = cfg_audio_weather_volume = 100;
#endif

#if 0 /* This skips command-line arguments, not rc file */
			modus = 2;
			i = argc;
#endif
			break;
			
		/* Source other files */
		case 'f':
			skip = read_mangrc(&argv[i][2]);
			break;

		case 'p':
			cfg_game_port = atoi(&argv[i][2]);
			break;

		case 'F':
			cfg_client_fps = atoi(&argv[i][2]);
			break;
		
		case 'P':
			strcpy(path, argv[i]+2);
			break;
		
		case 'N':
			strcpy(cname, argv[i]+2);
			break;
		
		/* Pull login id */
		case 'l':
			if (argv[i][2])	/* Hack -- allow space after '-l' */
			{
				strcpy(nick, argv[i]+2);
				modus = 2;
			}
			else modus = 1;
			break;

		/* Pull 'real name' */
		case 'n':
			if (argv[i][2])	/* Hack -- allow space after '-l' */
			{
				strcpy(real_name, argv[i]+2);
				break;
			}
			/* Fall through */

		case 'c':
			force_cui = TRUE;
			break;

		case 'C':
			server_protocol = 1;
			break;

		case 'q':
			quiet_mode = TRUE;
			break;

		case 'w':
			noweather_mode = TRUE;
			break;

		case 'u':
			no_lua_updates = TRUE;
			break;

		case 'm':
			skip_motd = TRUE;
			break;

		case 'v':
			save_chat = TRUE;
			break;

		default:
			modus = -1;
			i = argc;
			break;
		}
	}

	if (quiet_mode) use_sound = FALSE;

	if (modus == 1 || modus < 0)
	{
		/* Dump usage information */
		puts(longVersion);
		puts("Usage  : tomenet [options] [servername]");
		puts("Example: tomenet -lMorgoth MorgyPass -p18348 europe.tomenet.net");
		puts("       : tomenet -f.myrc -lOlorin_archer");
		puts("  -c                 Always use CUI(GCU) interface");
		puts("  -C                 Compatibility mode for very old servers");
		puts("  -f                 specify rc File to read");
		puts("  -F                 Client FPS");
		puts("  -i                 Ignore .tomenetrc");
		puts("  -l<nick> <passwd>  Login as");
		puts("  -N<name>           character Name");
		puts("  -p<num>            change game Port number");
		puts("  -P<path>           set the lib directory Path");
		puts("  -q                 disable audio capabilities ('quiet mode')");
		puts("  -w                 disable client-side weather effects");
		puts("  -u                 disable client-side automatic lua updates");
		puts("  -m                 skip motd (message of the day) on login");
		puts("  -v                 save chat log on exit, don't prompt");

#ifdef USE_SOUND_2010
#if 0 //we don't have 'modules' for everything, yet :-p only sound_modules for now - C. Blue
		/* Print the name and help for each available module */
		for (i = 0; i < (int)N_ELEMENTS(modules); i++)
			printf("     %s   %s\n", modules[i].name, modules[i].help);
#endif
#endif

		quit(NULL);
	}


	/* Attempt to initialize a visual module */

	if (!force_cui)
	{
#ifdef USE_GTK
	/* Attempt to use the "main-gtk.c" support */
	if (!done)
	{
		if (0 == init_gtk(argc, argv))
		{
			ANGBAND_SYS = "gtk";
			done = TRUE;
		}
	}
#endif


#ifdef USE_XAW
	/* Attempt to use the "main-xaw.c" support */
	if (!done)
	{
		if (0 == init_xaw()) done = TRUE;
		if (done) ANGBAND_SYS = "xaw";
	}
#endif

#ifdef USE_X11
	/* Attempt to use the "main-x11.c" support */
	if (!done)
	{
		if (0 == init_x11()) done = TRUE;
		if (done) ANGBAND_SYS = "x11";
	}
#endif
	}

#ifdef USE_GCU
	/* Attempt to use the "main-gcu.c" support */
	if (!done)
	{
		if (0 == init_gcu()) done = TRUE;
		if (done) ANGBAND_SYS = "gcu";
	}
#endif

#ifdef USE_CAP
	/* Attempt to use the "main-cap.c" support */
	if (!done)
	{
		if (0 == init_cap()) done = TRUE;
		if (done) ANGBAND_SYS = "cap";
	}
#endif

#ifdef USE_IBM
	/* Attempt to use the "main_ibm.c" support */
	if (!done)
	{
		if (0 == init_ibm()) done = TRUE;
		if (done) ANGBAND_SYS = "ibm";
	}
#endif

#ifdef USE_EMX
	/* Attempt to use the "main-emx.c" support */
	if (!done)
	{
		if (0 == init_emx()) done = TRUE;
		if (done) ANGBAND_SYS = "emx";
	}
#endif

#ifdef USE_AMY
	/* Attempt to use the "main-amy.c" support */
	if (!done)
	{
		if (0 == init_amy()) done = TRUE;
		if (done) ANGBAND_SYS = "amy";
	}
#endif

	/* No visual module worked */
	if (!done)
	{
		Net_cleanup();
		printf("Unable to initialize a display module!\n");
		exit(1);
	}

	{
		u32b seed;

		/* Basic seed */
		seed = (time(NULL));

#ifdef SET_UID

		/* Mutate the seed on Unix machines */
		seed = ((seed >> 3) * (getpid() << 1));

#endif

		/* Use the complex RNG */
		Rand_quick = FALSE;

		/* Seed the "complex" RNG */
		Rand_state_init(seed);
	}

	/* Attempt to read default name/password from mangrc file */

	done = (modus > 2 || skip) ? TRUE : FALSE;

#if 0 //moving this to c-init.c, because ANGBAND_DIR_XTRA isn't initialized here yet- C. Blue
#ifdef USE_SOUND_2010
 #if 0
	/* Hack -- Forget standard args */
	if (TRUE) {//if (args) {
		argc = 1;
		argv[1] = NULL;
	}
 #endif

	/* Try the modules in the order specified by sound_modules[] */
 #if 0//pfft doesnt work, dunno why ('incomplete type' error)
	for (i = 0; i < (int)N_ELEMENTS(sound_modules) - 1; i++) {
 #endif
	for (i = 0; i < 2; i++) {//we should've 2 hard-coded atm: SDL and dummy -_-
		if (0 == sound_modules[i].init(argc, argv)) {
 #if 1//just USE_SOUND_2010 debug
			puts(format("USE_SOUND_2010: successfully loaded module %d.", i));
 #endif
			break;
		}
	}
 #if 1//just USE_SOUND_2010 debug
	puts("USE_SOUND_2010: done loading modules");
 #endif
#endif
#endif


#ifdef UNIX_SOCKETS
	/* Always call with NULL argument */
	client_init(NULL, done);
#else
	if (strlen(svname)>0)
	{
		/* Initialize with given server name */
		client_init(svname, done);
	}
	else
	{
		/* Initialize and query metaserver */
		client_init(NULL, done);
	}
#endif


	return 0;
}
