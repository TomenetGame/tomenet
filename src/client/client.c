/* $Id$ */
/* Main client module */

/*
 * This is intentionally minimal, as system specific stuff may
 * need be done in "main" (or "WinMain" for Windows systems).
 * The real non-system-specific initialization is done in
 * "c-init.c".
 */

#include "angband.h"


char mangrc_filename[100] = "";

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
	//strcpy(val2, val);
	/* strcpy may not work if the strings overlap */
	memmove(val2, val, strlen(val) + 1);
	/* strip trailing linefeeds */
	while (val2[strlen(val2) - 1] == '\n' || val2[strlen(val2) - 1] == '\r') val2[strlen(val2) - 1] = '\0';

	if (t != 0) { /* exempt main window */
		if ((val = strstr(sec_name, "_Title"))) {
			strncpy(ang_term_name[t], val + 6, sizeof(ang_term_name[t]));
			ang_term_name[t][sizeof(ang_term_name[t]) - 1] = '\0';
		}
	}

	if ((val = strstr(sec_name, "_Visible")))
		term_prefs[t].visible = (atoi(val + 8) != 0);

	if ((val = strstr(sec_name, "_X")))
		term_prefs[t].x = atoi(val + 2);
	if ((val = strstr(sec_name, "_Y")))
		term_prefs[t].y = atoi(val + 2);

	if ((val = strstr(sec_name, "_Columns"))) {
		term_prefs[t].columns = atoi(val + 8);
		if (!term_prefs[t].columns) term_prefs[t].columns = DEFAULT_TERM_WID;
	}
	if ((val = strstr(sec_name, "_Lines"))) {
		term_prefs[t].lines = atoi(val + 6);
		if (!term_prefs[t].lines) term_prefs[t].lines = DEFAULT_TERM_HGT;
	}

	if ((val = strstr(sec_name, "_Font"))) {
#if 0 /* without tab/space stripping */
		while (val[0] && (val[0] < '0' || val[0] > '9')) val++;
		val2 = val;
		while (val2[0] && ((val2[0] >= '0' && val2[0] <= '9') || val2[0] == 'x')) val2++;
		val2[0] = 0;
		strncpy(term_prefs[t].font, val, sizeof(term_prefs[t].font));
		term_prefs[t].font[sizeof(term_prefs[t].font) - 1] = '\0';
#else /* also allow font names consisting of letters! */
		strncpy(term_prefs[t].font, val + 5, sizeof(term_prefs[t].font));
		term_prefs[t].font[sizeof(term_prefs[t].font) - 1] = '\0';
#endif
	}
}
static bool read_mangrc(cptr filename) {
	FILE *config;
	char buf[1024];
	bool skip = FALSE;

	lighterdarkblue = FALSE;

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
	if ((config = fopen(mangrc_filename, "r"))) {
		/* Read until end */
		while (!feof(config)) {
			/* Get a line */
			if (!fgets(buf, 1024, config)) break;

			/* Skip comments, empty lines */
			if (buf[0] == '\n' || buf[0] == '#')
				continue;

			/* Name line */
			if (!strncmp(buf, "nick", 4)) {
				char *name;

				/* Extract name */
				name = strtok(buf, " \t\n");
				name = strtok(NULL, "\t\n");

				/* Default nickname */
				strcpy(nick, name);
			}

			/* Password line */
			if (!strncmp(buf, "pass", 4)) {
				char *p;

				/* Extract password */
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");

				/* Default password */
				strcpy(pass, p);
			}

			if (!strncmp(buf, "name", 4)) {
				char *name;

				name = strtok(buf, " \t\n");
				name = strtok(NULL, "\t\n");

				strcpy(cname, name);
			}

			/* meta server line */
			if (!strncmp(buf, "meta", 4)) {
				char *p;

				p = strtok(buf, " :\t\n");
				p = strtok(NULL, ":\t\n");

				if (p) strcpy(meta_address, p);
			}

			/* game server line (note: should be named 'host' for consistency with tomenet.ini) */
			if (!strncmp(buf, "server", 6)) {
				char *p;

				p = strtok(buf, " :\t\n");
				p = strtok(NULL, ":\t\n");

				if (p) {
					strcpy(svname, p);
					p = strtok(NULL, ":\t\n");
					if (p) cfg_game_port = atoi(p);
				}
			}

			/* port line */
			if (!strncmp(buf, "port", 4)) {
				char *p;

				/* Extract password */
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");

				/* Default password */
				if (p) cfg_game_port = atoi(p);
			}

			/* fps line */
			if (!strncmp(buf, "fps", 3)) {
				char *p;

				/* Extract fps */
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");

				if (p) cfg_client_fps = atoi(p);
			}

			/* unix username line */
			if (!strncmp(buf, "realname", 8)) {
				char *p;

				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");

				if (p) strcpy(real_name, p);
			}

			/* Path line */
			if (!strncmp(buf, "path", 4)) {
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

			/* READABILITY_BLUE */
			if (!strncmp(buf, "lighterDarkBlue", 15)) lighterdarkblue = TRUE;

			/* Color map */
			if (!strncmp(buf, "colormap_", 9)) {
				int colornum = atoi(buf + 9);
				char *p;

				/* Extract path */
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");

				u32b c = parse_color_code(p);
				if (colornum >= 0 && colornum < 16 && c < 0x01000000) client_color_map[colornum] = c;
			}

#ifdef USE_GRAPHICS
			/* graphics */
			if (!strncmp(buf, "graphics", 8)) {
				char *p;

				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");
				if (p) use_graphics = (atoi(p) != 0);
			}
			if (!strncmp(buf, "graphic_tiles", 5)) {
				char *p;
				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");
				if (p) strcpy(graphic_tiles, p);
			}
#endif
#ifdef USE_SOUND
			/* sound */
			if (!strncmp(buf, "sound", 5) && strncmp(buf, "soundpackFolder", 15)) {
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
			/* Folder of the currently selected sound pack */
			if (!strncmp(buf, "soundpackFolder", 15)) {
				char *p;

				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");
				if (p) strcpy(cfg_soundpackfolder, p);
			}
			/* Folder of the currently selected music pack */
			if (!strncmp(buf, "musicpackFolder", 15)) {
				char *p;

				p = strtok(buf, " \t\n");
				p = strtok(NULL, "\t\n");
				if (p) strcpy(cfg_musicpackfolder, p);
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

			/* big_map hint */
			if (!strncmp(buf, "hintBigmap", 10)) {
				bigmap_hint = FALSE;
				firstrun = FALSE;
			}

			/*** Everything else is ignored ***/
		}
		fclose(config);

		validate_term_screen_dimensions(&(term_prefs[0].columns), &(term_prefs[0].lines));
		/* Calculate game screen dimensions from main window dimensions. */
		screen_wid = term_prefs[0].columns - SCREEN_PAD_X;
		screen_hgt = term_prefs[0].lines - SCREEN_PAD_Y;

		if (lighterdarkblue && client_color_map[6] == 0x0000ff)
#ifdef USE_X11
			enable_readability_blue_x11();
#else
			enable_readability_blue_gcu();
#endif
	}
	return (skip);
}

#ifdef USE_X11
/* linux clients: save subwindow prefs to .tomenetrc - C. Blue */
static void write_mangrc_aux(int t, cptr sec_name, FILE *cfg_file) {

	if (t != 0) {
		fputs(format("%s_Title\t%s\n", sec_name, ang_term_name[t]), cfg_file);
		fputs(format("%s_Visible\t%c\n", sec_name, term_prefs[t].visible ? '1' : '0'), cfg_file);
	}
	if (term_prefs[t].x != -32000) /* don't save windows in minimized state */
		fputs(format("%s_X\t\t%d\n", sec_name, term_prefs[t].x), cfg_file);
	if (term_prefs[t].y != -32000) /* don't save windows in minimized state */
		fputs(format("%s_Y\t\t%d\n", sec_name, term_prefs[t].y), cfg_file);
	if (t != 0) {
		fputs(format("%s_Columns\t%d\n", sec_name, term_prefs[t].columns), cfg_file);
		fputs(format("%s_Lines\t%d\n", sec_name, term_prefs[t].lines), cfg_file);
		fputs(format("%s_Font\t%s\n", sec_name, term_prefs[t].font), cfg_file);
	} else {
		/* one more tab, or formatting looks bad ;) */
		fputs(format("%s_Font\t\t%s\n", sec_name, term_prefs[t].font), cfg_file);
		fputs(format("%s_Lines\t%d\n", sec_name, term_prefs[t].lines), cfg_file);
	}
	fputs("\n", cfg_file);
}

/* linux clients: save one line of subwindow prefs to .tomenetrc - C. Blue */
static void write_mangrc_aux_line(int t, cptr sec_name, char *buf_org) {
	char buf[1024], *ter_name = buf_org + strlen(sec_name);
#if 0 /* we still want to save at least the new visibility state, if it was toggled via in-game menu */
	if (!c) return; /* invisible window? */
#endif

	/* no line that gets modified? then keep original! */
	strcpy(buf, buf_org);

	if (!strncmp(ter_name, "_Title", 6)) {
		if (t != 0)
			sprintf(buf, "%s_Title\t%s\n", sec_name, ang_term_name[t]);
	} else if (!strncmp(ter_name, "_Visible", 8)) {
		if (t != 0)
			sprintf(buf, "%s_Visible\t%c\n", sec_name, term_prefs[t].visible ? '1' : '0');
	} else if (term_prefs[t].visible && !strncmp(ter_name, "_X", 2)) {
		if (term_prefs[t].x != -32000) /* don't save windows in minimized state */
			sprintf(buf, "%s_X\t\t%d\n", sec_name, term_prefs[t].x);
	} else if (term_prefs[t].visible && !strncmp(ter_name, "_Y", 2)) {
		if (term_prefs[t].y != -32000) /* don't save windows in minimized state */
			sprintf(buf, "%s_Y\t\t%d\n", sec_name, term_prefs[t].y);
	} else if (term_prefs[t].visible && !strncmp(ter_name, "_Columns", 8)) {
		if (t != 0)
			sprintf(buf, "%s_Columns\t%d\n", sec_name, term_prefs[t].columns);
	} else if (term_prefs[t].visible && !strncmp(ter_name, "_Lines", 6)) {
			sprintf(buf, "%s_Lines\t%d\n", sec_name, term_prefs[t].lines);
	} else if (!strncmp(ter_name, "_Font", 5) && term_prefs[t].font[0] != '\0') {
		if (t != 0)
			snprintf(buf, 1024, "%s_Font\t%s\n", sec_name, term_prefs[t].font);
		else
			/* one more tab, or formatting looks bad ;) */
			snprintf(buf, 1024, "%s_Font\t\t%s\n", sec_name, term_prefs[t].font);
	}

	strcpy(buf_org, buf);
}
#endif
/* linux clients: save some prefs to .tomenetrc - C. Blue
   If creds_only is TRUE, all other settings will be skipped. This is used to instantiate nick+pass in case they were still commented out. (1st run)
   Already existing nick+pass will only be updated if update_creds is TRUE.
   If audiopacks_only is TRUE, all other settings except for sound+music pack will be skipped. */
bool write_mangrc(bool creds_only, bool update_creds, bool audiopacks_only) {
	char config_name2[100];
	FILE *config, *config2;
	char buf[1024];
#ifdef USE_SOUND_2010
	/* backward compatibility */
	bool compat_apf = FALSE;
#endif

	buf[0] = 0;//valgrind warning it seems..?

	strcpy(config_name2, mangrc_filename);
	strcat(config_name2, ".$$$");

	config = fopen(mangrc_filename, "r");
	config2 = fopen(config_name2, "w");

	/* Attempt to open file */
	if (config2) {
		if (config) {
			/* Read until end */
			while (!feof(config)) {
				/* Get a line */
				if (!fgets(buf, 1024, config)) break;

				if (audiopacks_only) {
					/* modify the line */
					if (!strncmp(buf, "soundpackFolder", 15)) {
						strcpy(buf, "soundpackFolder\t\t");
						strcat(buf, cfg_soundpackfolder);
						strcat(buf, "\n");
#ifdef USE_SOUND_2010
						compat_apf = TRUE;
#endif
					} else if (!strncmp(buf, "musicpackFolder", 15)) {
						strcpy(buf, "musicpackFolder\t\t");
						strcat(buf, cfg_musicpackfolder);
						strcat(buf, "\n");
					}
				} else {
					if (!creds_only) {
						/* modify the line */
						if (!strncmp(buf, "soundpackFolder", 15)) {
							strcpy(buf, "soundpackFolder\t\t");
							strcat(buf, cfg_soundpackfolder);
							strcat(buf, "\n");
#ifdef USE_SOUND_2010
							compat_apf = TRUE;
#endif
						} else if (!strncmp(buf, "musicpackFolder", 15)) {
							strcpy(buf, "musicpackFolder\t\t");
							strcat(buf, cfg_musicpackfolder);
							strcat(buf, "\n");
						}
#ifdef USE_SOUND_2010
						/* audio mixer settings */
						if (!strncmp(buf, "audioMaster", 11)) {
							strcpy(buf, "audioMaster\t\t");
							strcat(buf, cfg_audio_master ? "1\n" : "0\n");

							/* add newly released entries to older config file versions */
							if (!compat_apf) {
								fputs("\nsoundpackFolder\t\tsound\n", config2);
								fputs("musicpackFolder\t\tmusic\n\n", config2);
							}
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
						/* Don't do this in terminal mode ('-c') */
						if (!strcmp(ANGBAND_SYS, "x11")) {
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
						}
#endif /* USE_X11 */
					}

					if (!strncmp(buf, "#nick", 5) && nick[0] && pass[0]) {
						/* Set accountname for the first time */
						strcpy(buf, "nick\t\t");
						strcat(buf, nick);
						strcat(buf, "\n");
					} else if (update_creds && !strncmp(buf, "nick", 4) && nick[0] && pass[0]) {
						/* Set/update accountname */
						strcpy(buf, "nick\t\t");
						strcat(buf, nick);
						strcat(buf, "\n");
					}

					if (!strncmp(buf, "#pass", 5) && nick[0] && pass[0]) {
						/* Set password for the first time */
						char tmp[MAX_CHARS];

						strcpy(tmp, pass);
						my_memfrob(tmp, strlen(tmp));
						strcpy(buf, "pass\t\t");
						strcat(buf, tmp); //security hole here, buf doesn't get zero'ed
						memset(tmp, 0, MAX_CHARS);
						strcat(buf, "\n");
					} else if (update_creds && !strncmp(buf, "pass", 4) && nick[0] && pass[0]) {
						/* Set/update password */
						char tmp[MAX_CHARS];

						strcpy(tmp, pass);
						my_memfrob(tmp, strlen(tmp));
						strcpy(buf, "pass\t\t");
						strcat(buf, tmp); //security hole here, buf doesn't get zero'ed
						memset(tmp, 0, MAX_CHARS);
						strcat(buf, "\n");
					}

					/*** Everything else is ignored ***/
				}

				/* copy the line over */
				fputs(buf, config2);

#ifdef USE_SOUND_2010
				if (!creds_only) {
					/* hack: disable one-time hint */
					if (!strncmp(buf, "sound", 5) && strncmp(buf, "soundpackFolder", 15) && sound_hint) fputs("hintSound\n", config2);
				}
#endif
			}

			//if (!creds_only) {
				/* hack: disable one-time hint */
				if (bigmap_hint) fputs("\nhintBigmap\n", config2);
			//}

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

			if (nick[0] && pass[0]) {
				char tmp[MAX_CHARS];

				fputs(format("nick\t\t%s\n", nick), config2);
				strcpy(tmp, pass);
				my_memfrob(tmp, strlen(tmp));
				fputs(format("pass\t\t%s\n", tmp), config2);
				memset(tmp, 0, MAX_CHARS);
			} else {
				fputs(format("#nick\t\t%s\n", nick), config2);
				fputs(format("#pass\t\t%s\n", ""), config2);//keep pass secure maybe
			}
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

			fputs("# Use lighter 'dark blue' colour to increase readability on some screens\n", config2);
			fputs("# Sets blue to #0033ff instead of #0000ff\n", config2);
			fputs("lighterDarkBlue\n", config2);
			fputs("\n", config2);

			fputs("# Full color remapping\n", config2);
			fputs("# 0 = black, 1 = white, 2 = gray, 3 = orange, 4 = red, 5 = green, 6 = blue\n", config2);
			fputs("# 7 = umber, 8 = dark gray, 9 = light gray, 10 = violet, 11 = yellow\n", config2);
			fputs("# 12 = light red, 13 = light green, 14 = light blue, 15 = light umber\n", config2);
			fputs("#colormap_0\t\t#000000\n", config2);
			fputs("#colormap_1\t\t#ffffff\n", config2);
			fputs("#colormap_2\t\t#9d9d9d\n", config2);
			fputs("#colormap_3\t\t#ff8d00\n", config2);
			fputs("#colormap_4\t\t#b70000\n", config2);
			fputs("#colormap_5\t\t#009d44\n", config2);
			fputs("#colormap_6\t\t#0000ff\n", config2);
			fputs("#colormap_7\t\t#8d6600\n", config2);
			fputs("#colormap_8\t\t#666666\n", config2);
			fputs("#colormap_9\t\t#d7d7d7\n", config2);
			fputs("#colormap_10\t\t#af00ff\n", config2);
			fputs("#colormap_11\t\t#ffff00\n", config2);
			fputs("#colormap_12\t\t#ff3030\n", config2);
			fputs("#colormap_13\t\t#00ff00\n", config2);
			fputs("#colormap_14\t\t#00ffff\n", config2);
			fputs("#colormap_15\t\t#c79d55\n", config2);
			fputs("\n", config2);

//#ifdef USE_GRAPHICS
			fputs(format("graphics\t\t%s\n", use_graphics ? "1" : "0"), config2);
			fputs(format("graphic_tiles\t\t%s\n", graphic_tiles), config2);
			fputs("\n", config2);
//#endif
//#ifdef USE_SOUND
			fputs(format("sound\t\t\t%s\n", use_sound_org ? "1" : "0"), config2);
//#endif
#ifdef USE_SOUND_2010
			fputs("hintSound\n", config2);
			fputs(format("cacheAudio\t\t%s\n", no_cache_audio ? "0" : "1"), config2);
			fputs(format("audioSampleRate\t\t%d\n", cfg_audio_rate), config2);
			fputs(format("audioChannels\t\t%d\n", cfg_max_channels), config2);
			fputs(format("audioBuffer\t\t%d\n", cfg_audio_buffer), config2);
			fputs(format("soundpackFolder\t\t%s\n", cfg_soundpackfolder), config2);
			fputs(format("musicpackFolder\t\t%s\n", cfg_musicpackfolder), config2);
			fputs(format("audioMaster\t\t%s\n", cfg_audio_master ? "1" : "0"), config2);
			fputs(format("audioMusic\t\t%s\n", cfg_audio_music ? "1" : "0"), config2);
			fputs(format("audioSound\t\t%s\n", cfg_audio_sound ? "1" : "0"), config2);
			fputs(format("audioWeather\t\t%s\n", cfg_audio_weather ? "1" : "0"), config2);
			fputs(format("audioVolumeMaster\t%d\n", cfg_audio_master_volume), config2);
			fputs(format("audioVolumeMusic\t%d\n", cfg_audio_music_volume), config2);
			fputs(format("audioVolumeSound\t%d\n", cfg_audio_sound_volume), config2);
			fputs(format("audioVolumeWeather\t%d\n", cfg_audio_weather_volume), config2);
#endif
			fputs("\n", config2);

#ifdef USE_X11
///LINUX_TERM_CFG
/* Don't do this in terminal mode ('-c') */
			if (!strcmp(ANGBAND_SYS, "x11")) {
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
			}
#endif

			if (!creds_only) {
				fputs("\n", config2);
				fputs("hintBigmap\n", config2);
			}

			fclose(config2);

			/* rename temporary file to new ".tomenetrc" */
			rename(config_name2, mangrc_filename);
		}

	/* shouldn't really happen, but Matfil had a weird bug where only 'hintBigMap' is left in his .tomenetrc as the only line, so maybe: */
	} else if (config) fclose(config);

	return (FALSE); //failure
}

bool write_mangrc_colourmap(void) {
	char config_name2[100];
	FILE *config, *config2;
	char buf[1024];
	bool found_start = FALSE, found = FALSE;
	int i;
	unsigned long c;

	buf[0] = 0;//valgrind warning it seems..?

	strcpy(config_name2, mangrc_filename);
	strcat(config_name2, ".$$$");

	config = fopen(mangrc_filename, "r");
	config2 = fopen(config_name2, "w");

	/* Attempt to open file */
	if (config2) {
		if (config) {
			/* Read until end */
			while (!feof(config)) {
				/* Get a line */
				if (!fgets(buf, 1024, config)) break;

				/* Remove all colourmap lines */
				if (!strncmp(buf, "#colormap_", 10) || !strncmp(buf, ";colormap_", 10) || !strncmp(buf, "colormap_", 9)) {
					found_start = TRUE;
					continue;
				}

				/* Find good place to fit them in (at comment about colourmap) */
				if (buf[0] == '#' && strstr(buf, "Full color remapping")) found_start = TRUE;

				/* Insert colourmap info here */
				if ((!buf[0] || buf[0] == '\n') && found_start && !found) {
					found = TRUE;
					for (i = 1; i < 16; i++) {
						c = client_color_map[i];
						sprintf(buf, "colormap_%d\t\t#%06lx\n", i, c);
						fputs(buf, config2);
					}
					fputs("\n", config2);
					continue;
				}

				/* copy everything else over */
				fputs(buf, config2);
			}

			/* Just append new colourmap? */
			if (!found) {
				fputs("\n", config2);
				for (i = 1; i < 16; i++) {
					c = client_color_map[i];
					sprintf(buf, "colormap_%d\t\t#%06lx\n", i, c);
					fputs(buf, config2);
				}
			}

			fclose(config);
			fclose(config2);

			/* replace old by new */
			remove(mangrc_filename);
			rename(config_name2, mangrc_filename);

			return (TRUE); //success
		}
	}

	return (FALSE); //failure
}

static void default_set(void) {
	char *temp;

#ifdef SET_UID
	int player_uid;
	struct passwd *pw;
#endif

	/* Initial defaults */
	strcpy(nick, "PLAYER");
	strcpy(pass, ""); //was passwd, but players should really be forced to make up different passwords
	strcpy(real_name, "PLAYER");


	/* Get login name if a UNIX machine */
#ifdef SET_UID
	/* Get player UID */
	player_uid = getuid();

	/* Get password entry */
	if ((pw = getpwuid(player_uid))) {
		/* Pull login id */
		strcpy(nick, pw->pw_name);

		/* Cut */
		nick[16] = '\0';

		/* Copy to real name */
		strcpy(real_name, nick);
	}
#endif

#ifdef AMIGA
	if ((GetVar("tomenet_name", real_name, 80, 0L)) != -1) {
	  strcpy(nick, real_name);
	}
#endif
#ifdef SET_UID
	temp = getenv("TOMENET_PLAYER");
	if (temp) strcpy(nick, temp); 
	temp = getenv("TOMENET_USER");
	if (temp) strcpy(real_name, temp); 
#endif
}

int main(int argc, char **argv) {
	int i, modus = 0;
	bool done = FALSE, skip = FALSE, force_cui = FALSE;

	/* Save the program name */
	argv0 = argv[0];


	/* Set default values */
	default_set();

	/* Get file paths as early as possible */
	init_stuff();

	/* Acquire the version strings */
	version_build();

	/* Make a copy to use in colour blindness menu when we want to reset palette to default values.
	   This must happen before we read the config file, as it contains colour-(re)definitions. */
	for (i = 0; i < BASE_PALETTE_SIZE; i++) client_color_map_org[i] = client_color_map[i];

	/* assume defaults */
	strcpy(cfg_soundpackfolder, "sound");
	strcpy(cfg_musicpackfolder, "music");

	/* read default rc file */
	skip = read_mangrc("");

	/* Process the command line arguments */
	for (i = 1; argv && (i < argc); i++) {
		/* Require proper options */
		if (argv[i][0] != '-') {
			if (modus && modus < 3) {
				if (modus == 2) {
					strcpy(pass, argv[i]);
					modus = 3;
				} else {
					strcpy(nick, argv[i]);
					modus = 2;
				}
			}
			else strcpy(svname, argv[i]);
			continue;
		}

		/* Analyze option */
		switch (argv[i][1]) {
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
			cfg_audio_master_volume = cfg_audio_music_volume = cfg_audio_sound_volume = cfg_audio_weather_volume = AUDIO_VOLUME_DEFAULT;
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
			strcpy(path, argv[i] + 2);
			break;

		case 'R':
			auto_reincarnation = TRUE;
			/* Fall through */
		case 'N':
			strcpy(cname, argv[i] + 2);
			break;

		/* Pull login id */
		case 'l':
			if (argv[i][2]) { /* Hack -- allow space after '-l' */
				strcpy(nick, argv[i] + 2);
				modus = 2;
			}
			else modus = 1;
			break;

		/* Pull 'real name' */
		case 'n':
			if (argv[i][2])	/* Hack -- allow space after '-l' */
			{
				strcpy(real_name, argv[i] + 2);
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
			save_chat = 1;
			break;

		case 'V':
			save_chat = 2;
			break;

		case 'x':
			save_chat = 3;
			break;

		case 'e': {
			/* Since ALSA might spam underrun errors.. */
			FILE *fr = freopen("tomenet.log", "w", stderr);
			if (!fr) {
				fprintf(stderr, "Failed to open tomenet.log for writing!\n");
			}
			break;
		}

		default:
			modus = -1;
			i = argc;
			break;
		}
	}

	if (quiet_mode) use_sound = FALSE;

	if (modus == 1 || modus < 0) {
		/* Dump usage information */
		puts(longVersion);
		puts(format("Running on %s.", os_version));
		puts("Usage  : tomenet [options] [servername]");
		puts("Example: tomenet -lMorgoth MorgyPass -p18348 europe.tomenet.eu");
		puts("       : tomenet -f.myrc -lOlorin_archer");
		puts("  -h                 Display this help");
		puts("  -c                 Always use CUI(GCU) interface");
		puts("  -C                 Compatibility mode for very old servers");
		puts("  -e                 Create file 'tomenet.log' instead of displaying");
		puts("                     error messages in the terminal");
		puts("  -f                 Specify rc File to read");
		puts("  -F                 Client FPS");
		puts("  -i                 Ignore .tomenetrc");
		//puts("  -k                 don't disable numlock on client startup");
		puts("  -l<nick> <passwd>  Login as");
		puts("  -m                 Skip motd (message of the day) on login");
		puts("  -N<name>           Character name");
		puts("  -R<name>           Character name, auto-reincarnate");
		puts("  -p<num>            Change game Port number");
		puts("  -P<path>           Set the lib directory Path");
		puts("  -q                 Disable audio capabilities ('quiet mode')");
		puts("  -u                 Disable client-side automatic lua updates");
		puts("                     (you shouldn't use this option!");
		puts("  -v                 Save chat log on exit, don't prompt");
		puts("  -V                 Save complete message log on exit, don't prompt");
		puts("  -x                 Don't save chat/message log on exit (don't prompt)");
		puts("  -w                 Disable client-side weather effects");

#ifdef USE_SOUND_2010
#if 0 //we don't have 'modules' for everything, yet :-p only sound_modules for now - C. Blue
	//also, ANGBAND_DIR_XTRA is not yet initialized at this point, if needed..
		/* Print the name and help for each available module */
		for (i = 0; i < (int)N_ELEMENTS(modules); i++)
			printf("     %s   %s\n", modules[i].name, modules[i].help);
#endif
#endif

		quit(NULL);
	}


	/* Attempt to initialize a visual module */

	if (!force_cui) {
#ifdef USE_GTK
		/* Attempt to use the "main-gtk.c" support */
		if (!done) {
			if (0 == init_gtk(argc, argv)) {
				ANGBAND_SYS = "gtk";
				done = TRUE;
			}
		}
#endif
#ifdef USE_XAW
		/* Attempt to use the "main-xaw.c" support */
		if (!done) {
			if (0 == init_xaw()) done = TRUE;
			if (done) ANGBAND_SYS = "xaw";
		}
#endif
#ifdef USE_X11
		/* Attempt to use the "main-x11.c" support */
		if (!done) {
			if (0 == init_x11()) done = TRUE;
			if (done) ANGBAND_SYS = "x11";
		}
#endif
	}

#ifdef USE_GCU
	/* Attempt to use the "main-gcu.c" support */
	if (!done) {
		if (0 == init_gcu()) done = TRUE;
		if (done) ANGBAND_SYS = "gcu";
	}
#endif

#ifdef USE_CAP
	/* Attempt to use the "main-cap.c" support */
	if (!done) {
		if (0 == init_cap()) done = TRUE;
		if (done) ANGBAND_SYS = "cap";
	}
#endif

#ifdef USE_IBM
	/* Attempt to use the "main_ibm.c" support */
	if (!done) {
		if (0 == init_ibm()) done = TRUE;
		if (done) ANGBAND_SYS = "ibm";
	}
#endif

#ifdef USE_EMX
	/* Attempt to use the "main-emx.c" support */
	if (!done) {
		if (0 == init_emx()) done = TRUE;
		if (done) ANGBAND_SYS = "emx";
	}
#endif

#ifdef USE_AMY
	/* Attempt to use the "main-amy.c" support */
	if (!done) {
		if (0 == init_amy()) done = TRUE;
		if (done) ANGBAND_SYS = "amy";
	}
#endif

	/* No visual module worked */
	if (!done) {
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

#ifdef UNIX_SOCKETS
	/* Always call with NULL argument */
	client_init(NULL, done);
#else
	if (strlen(svname) > 0) {
		/* Initialize with given server name */
		client_init(svname, done);
	} else {
		/* Initialize and query metaserver */
		client_init(NULL, done);
	}
#endif


	return 0;
}
