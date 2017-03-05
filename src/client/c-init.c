/* $Id$ */
/* Client initialization module */

/*
 * This file should contain non-system-specific code.  If a
 * specific system needs its own "main" function (such as
 * Windows), then it should be placed in the "main-???.c" file.
 */

#include "angband.h"

/* For opendir */
#include <sys/types.h>
#include <dirent.h>

/* For dirname */
#include <libgen.h>

static int Socket;

#ifdef USE_SOUND_2010
/*
 * List of sound modules in the order they should be tried.
 */
const struct module sound_modules[] = {
 #ifdef SOUND_SDL
	{ "sdl", "SDL_mixer sound module", init_sound_sdl },
 #endif /* SOUND_SDL */
	{ "dummy", "Dummy module", NULL },
};
#endif /* USE_SOUND_2010 */

/* Use improved method for pasting monster/artifact lore into chat */
#define NEW_PASTELINE_METHOD


static void init_arrays(void) {
	int i;

	/* Macro variables */
	C_MAKE(macro__pat, MACRO_MAX, cptr);
	C_MAKE(macro__act, MACRO_MAX, cptr);
	C_MAKE(macro__cmd, MACRO_MAX, bool);
	C_MAKE(macro__hyb, MACRO_MAX, bool);

	/* Macro action buffer */
	C_MAKE(macro__buf, 1024, char);

	/* Message variables */
	C_MAKE(message__ptr, MESSAGE_MAX, s32b);
	C_MAKE(message__buf, MESSAGE_BUF, char);
	C_MAKE(message__ptr_chat, MESSAGE_MAX, s32b);
	C_MAKE(message__buf_chat, MESSAGE_BUF, char);
	C_MAKE(message__ptr_msgnochat, MESSAGE_MAX, s32b);
	C_MAKE(message__buf_msgnochat, MESSAGE_BUF, char);
	C_MAKE(message__ptr_impscroll, MESSAGE_MAX, s32b);
	C_MAKE(message__buf_impscroll, MESSAGE_BUF, char);

	/* Hack -- No messages yet */
	message__tail = MESSAGE_BUF;
	message__tail_chat = MESSAGE_BUF;
	message__tail_msgnochat = MESSAGE_BUF;
	message__tail_impscroll = MESSAGE_BUF;

	/* Initialize room for the store's stock */
	C_MAKE(store.stock, STORE_INVEN_MAX, object_type);

	/* Paranoia, for crash in get_item_hook_find_obj() */
	for (i = 0; i < INVEN_TOTAL; i++) inventory_name[i][0] = 0;
}

void init_schools(s16b new_size) {
	/* allocate the extra memory */
	C_MAKE(schools, new_size, school_type);
	max_schools = new_size;
}

void init_spells(s16b new_size) {
	/* allocate the extra memory */
	C_MAKE(school_spells, new_size, spell_type);
	max_spells = new_size;
}

static bool check_dir2(cptr s) {
	DIR *dp = opendir(s);

	if (dp) {
		closedir(dp);
		return TRUE;
	} else {
		return FALSE;
	}
}

static void validate_dir(cptr s) {
	/* Verify or fail */
	if (!check_dir2(s))
		quit_fmt("Cannot find required directory:\n%s", s);
}

/*
 * Initialize and verify the file paths, and the score file.
 *
 * Use the ANGBAND_PATH environment var if possible, else use
 * DEFAULT_PATH, and in either case, branch off appropriately.
 *
 * First, we'll look for the ANGBAND_PATH environment variable,
 * and then look for the files in there.  If that doesn't work,
 * we'll try the DEFAULT_PATH constant.  So be sure that one of
 * these two things works...
 *
 * We must ensure that the path ends with "PATH_SEP" if needed,
 * since the "init_file_paths()" function will simply append the
 * relevant "sub-directory names" to the given path.
 *
 * Note that the "path" must be "Angband:" for the Amiga, and it
 * is ignored for "VM/ESA", so I just combined the two.
 */
static void init_stuff(void) {
#if defined(AMIGA) || defined(VM)

	/* Hack -- prepare "path" */
	strcpy(path, "TomeNET:");

#else /* AMIGA / VM */

	if (argv0) {
		char *app_path = strdup(argv0);
		char *app_dir;

		app_dir = dirname(app_path);

		/* Change current directory to the location of the binary - mikaelh */
		if (chdir(app_dir) == -1) {
			plog_fmt("chdir(\"%s\") failed", app_dir);
		}

		free(app_path);
	}

	if (!strlen(path))
	{
		cptr tail;

		/* Get the environment variable */
		tail = getenv("TOMENET_PATH");

		/* Use the angband_path, or a default */
		strcpy(path, tail ? tail : DEFAULT_PATH);
	}

	/* Hack -- Add a path separator (only if needed) */
	if (!suffix(path, PATH_SEP)) strcat(path, PATH_SEP);

	/* Validate the path */
	validate_dir(path);

#endif /* AMIGA / VM */

	/* Initialize */
	init_file_paths(path);

	/* Hack -- Validate the paths */
	validate_dir(ANGBAND_DIR_SCPT);
	validate_dir(ANGBAND_DIR_TEXT);
	validate_dir(ANGBAND_DIR_USER);
	validate_dir(ANGBAND_DIR_GAME);
}



/*
 * Open all relevant pref files.
 */
void initialize_main_pref_files(void) {
	char buf[1024];
	int i;

	/* MEGAHACK -- clean up the arrays
	 * I should have made a mess of something somewhere.. */
#if 0
	for (i = 0; i < OPT_MAX; i++) Client_setup.options[i] = FALSE;
#else
	for (i = 0; i < OPT_MAX; i++) {
		if (option_info[i].o_var)
			Client_setup.options[i] = (*option_info[i].o_var) = option_info[i].o_norm;
	}
#endif
	for (i = 0; i < TV_MAX; i++) Client_setup.u_char[i] = Client_setup.u_attr[i] = 0;
	for (i = 0; i < MAX_F_IDX; i++) Client_setup.f_char[i] = Client_setup.f_attr[i] = 0;
	for (i = 0; i < MAX_K_IDX; i++) Client_setup.k_char[i] = Client_setup.k_attr[i] = 0;
	for (i = 0; i < MAX_R_IDX; i++) Client_setup.r_char[i] = Client_setup.r_attr[i] = 0;

	/* Access the "basic" pref file */
	strcpy(buf, "pref.prf");
	/* Process that file */
	process_pref_file(buf);

	/* Access the "user" pref file */
	sprintf(buf, "global.opt");
	/* Process that file */
	process_pref_file(buf);

	/* Access the "basic" system pref file */
	sprintf(buf, "pref-%s.prf", ANGBAND_SYS);
	/* Process that file */
	process_pref_file(buf);

	/* Access the "visual" system pref file (if any) */
	handle_process_font_file();

	/* Access the "user" system pref file */
	sprintf(buf, "global-%s.opt", ANGBAND_SYS);
	/* Process that file */
	process_pref_file(buf);


	/* Hack: command-line client cannot handle big_map */
	if (!strcmp(ANGBAND_SYS, "gcu")) {
		c_cfg.big_map = FALSE;
		(*option_info[CO_BIGMAP].o_var) = FALSE;
		Client_setup.options[CO_BIGMAP] = FALSE;
	}


	/* Hack: Convert old window.prf or user.prf files that
	   were made < 4.4.7.1. - C. Blue */
	for (i = 0; i < ANGBAND_TERM_MAX; i++) {
		/* in 4.7.7.1.0.0 there are only 8 flags up to 0x80: */
		if (window_flag[i] >= 0x0100) {
			i = -1;
			break;
		}
	}
	/* Found outdated flags? */
	if (i == -1) {
		for (i = 0; i < ANGBAND_TERM_MAX; i++) {
			u32b new_flags = 0x0;
			/* translate old->new */
			if (window_flag[i] & 0x00001) new_flags |= PW_INVEN;
			if (window_flag[i] & 0x00002) new_flags |= PW_EQUIP;
			if (window_flag[i] & 0x00008) new_flags |= PW_PLAYER;
			if (window_flag[i] & 0x00010) new_flags |= PW_LAGOMETER;
			if (window_flag[i] & 0x00040) new_flags |= PW_MESSAGE;
			if (window_flag[i] & 0x40000) new_flags |= PW_CHAT;
			if (window_flag[i] & 0x80000) new_flags |= PW_MSGNOCHAT;
			window_flag[i] = new_flags;
		}
		/* and.. save them! */
		(void)options_dump("global.opt");
	}
}

void initialize_player_pref_files(void){
	char buf[1024];


	/* Process character-specific options */
	sprintf(buf, "%s.opt", cname);
	/* Process that file */
	process_pref_file(buf);
	keymap_init();
	prt_level(p_ptr->lev, p_ptr->max_lev, p_ptr->max_plv, p_ptr->max_exp, p_ptr->exp, exp_adv, exp_adv_prev);
	Send_options();

#if 0 /* disabled, since everyone only has 1 account anyway. It just disturbs macros if you have a character of same name. */
	/* Access the "account" pref file */
	sprintf(buf, "%s.prf", nick);
#else /* this should be just fine as replacement */
	sprintf(buf, "global.prf");
#endif

	/* Process that file */
	process_pref_file(buf);

	/* Access the "race" pref file */
	if (race < Setup.max_race) {
		sprintf(buf, "%s.prf", race_info[race].title);
		/* Process that file */
		process_pref_file(buf);
	}

	/* Access the "class" pref file */
	if (class < Setup.max_class) {
		sprintf(buf, "%s.prf", class_info[class].title);
		/* Process that file */
		process_pref_file(buf);
	}

	/* Access the "character" pref file */
	sprintf(buf, "%s.prf", cname);
	/* Process that file */
	process_pref_file(buf);

	/* Special hack: On ARCADE_SERVER, load special arcade macros! */
	if (s_ARCADE) {
		sprintf(buf, "arcade-%s.prf", ANGBAND_SYS);
		process_pref_file(buf);
	}
}

/* handle auto-loading of auto-inscription files (*.ins) on logon */
void initialize_player_ins_files(void) {
	char buf[1024];
	int i;

	/* start with empty auto-inscriptions list */
	for (i = 0; i < MAX_AUTO_INSCRIPTIONS; i++) {
		strcpy(auto_inscription_match[i], "");
		strcpy(auto_inscription_tag[i], "");
	}

#if 0 /* disabled, since everyone only has 1 account anyway. It just disturbs macros if you have a character of same name. */
	/* Access the "account" ins file */
	sprintf(buf, "%s.ins", nick);
#else /* this should be just fine as replacement */
	sprintf(buf, "global.ins");
#endif
	load_auto_inscriptions(buf);

	/* Access the "race" ins file */
	if (race < Setup.max_race) {
		sprintf(buf, "%s.ins", race_info[race].title);
		load_auto_inscriptions(buf);
	}

	/* Access the "class" ins file */
	if (class < Setup.max_class) {
		sprintf(buf, "%s.ins", class_info[class].title);
		load_auto_inscriptions(buf);
	}

	/* Access the "character" ins file */
	sprintf(buf, "%s.ins", cname);
	load_auto_inscriptions(buf);
}


/* Init monster list for polymorph-by-name
   and also for displaying monster lore - C. Blue */
static void init_monster_list() {
	char buf[1024], *p1, *p2;
	int v1 = 0, v2 = 0, v3 = 0;
	FILE *fff;
	bool discard = FALSE, multihued = FALSE, breathhued = FALSE, basehued = FALSE;
	bool halloween; /* Don't display 'level 127' for townies during Halloween */

	/* actually use local r_info.txt - a novum */
	path_build(buf, 1024, ANGBAND_DIR_GAME, "r_info.txt");
	fff = my_fopen(buf, "r");
	if (fff == NULL) {
		//plog("Error: Your r_info.txt file is missing.");
		return;
	}

	while (0 == my_fgets(fff, buf, 1024)) {
		/* strip $/%..$/! conditions */
		p1 = p2 = buf; /* dummy, != NULL */
		halloween = FALSE;
		while (buf[0] == '$' || buf[0] == '%') {
			p1 = strchr(buf + 1, '$');
			p2 = strchr(buf + 1, '!');
			if (!p1 && !p2) break;
			if (!p1) p1 = p2;
			else if (p2 && p2 < p1) p1 = p2;

			/* avoid using Halloween-only monster level (for normal Bree townies) */
			if (buf[0] == '$' && *p1 == '$' && !strncmp(buf + 1, "HALLOWEEN", 9)) halloween = TRUE;

			/* actually handle/assume certain features */
			if (buf[0] == '$' && *p1 == '!' && (
			    (!strncmp(buf + 1, "HALLOWEEN", 9) && strstr(buf, "JOKEANGBAND")) /* allow viewing HALLOWEEN monsters (ie those that are marked as 'JOKEANGBAND when NOT Halloween') */
			    )) {
				p1 = p2 = NULL;
				break;
			}
			if (buf[0] == '$' && *p1 == '$' && (
			    !strncmp(buf + 1, "RPG_SERVER", 10) || /* assume non-rpgserver stats */
			    !strncmp(buf + 1, "TEST_SERVER", 11) || /* ignore test-server */
			    !strncmp(buf + 1, "ARCADE_SERVER", 13) /* assume non-arcadeserver stats */
			    )) {
				p1 = p2 = NULL;
				break;
			}

			//strcpy(buf, p1 + 1); // overlapping strings
			memmove(buf, p1 + 1, strlen(p1 + 1) + 1);
		}
		if (!p1 && !p2) continue;

		if (strlen(buf) < 3) continue;
		if (buf[0] == 'V' && buf[1] == ':') {
			sscanf(buf + 2, "%d.%d.%d", &v1, &v2, &v3);
			continue;
		}
		if (buf[0] != 'N' && buf[0] != 'F' && buf[0] != 'W') continue;


		if (v1 < 3 || (v1 == 3 && (v2 < 4 || (v2 == 4 && v3 < 1)))) {
			//plog("Error: Your r_info.txt file is outdated.");
			return;
		}

		if (buf[0] == 'W') {
			if (!halloween) monster_list_level[monster_list_idx - 1] = atoi(buf + 2);
			continue;
		}

		if (buf[0] == 'N') {
			if (discard) {
				/* flashy-thingy tiem */
				monster_list_idx--;

				discard = FALSE;
			}

			if (!basehued) {
				if (multihued) monster_list_any[monster_list_idx - 1] = TRUE;
				else if (breathhued) monster_list_breath[monster_list_idx - 1] = TRUE;
			}
			multihued = FALSE;
			breathhued = FALSE;
			basehued = FALSE;
		}
		if (buf[0] == 'F' && strstr(buf, "JOKEANGBAND")) discard = TRUE;
		if (buf[0] == 'F' && strstr(buf, "PET")) discard = TRUE;
		if (buf[0] == 'F' && strstr(buf, "NEUTRAL")) discard = TRUE;
		if (buf[0] == 'F' && strstr(buf, "ATTR_ANY")) multihued = TRUE;
		if (buf[0] == 'F' && strstr(buf, "ATTR_MULTI")) breathhued = TRUE;
		if (buf[0] == 'F' && strstr(buf, "ATTR_BASE")) basehued = TRUE;

		if (buf[0] == 'N' && strstr(buf, "Test Blob")) discard = TRUE;

		p1 = buf + 2; /* monster code */
		p2 = strchr(p1, ':'); /* 1 before monster name */
		if (!p2) continue; /* paranoia (broken file) */

		monster_list_code[monster_list_idx] = atoi(p1);
		strcpy(monster_list_name[monster_list_idx], p2 + 1);

		/* fetch symbol (and colour) */
		while (0 == my_fgets(fff, buf, 1024)) {
			/* strip $/%..$/! conditions */
			p1 = p2 = buf; /* dummy, != NULL */
			while (buf[0] == '$' || buf[0] == '%') {
				p1 = strchr(buf + 1, '$');
				p2 = strchr(buf + 1, '!');
				if (!p1 && !p2) break;
				if (!p1) p1 = p2;
				else if (p2 && p2 < p1) p1 = p2;

				/* actually handle/assume certain features */
				if (buf[0] == '$' && *p1 == '!' && (
				    (!strncmp(buf + 1, "HALLOWEEN", 9) && strstr(buf, "JOKEANGBAND")) /* allow viewing HALLOWEEN monsters (ie those that are marked as 'JOKEANGBAND when NOT Halloween') */
				    )) {
					p1 = p2 = NULL;
					break;
				}
				if (buf[0] == '$' && *p1 == '$' && (
				    !strncmp(buf + 1, "RPG_SERVER", 10) || /* assume non-rpgserver stats */
				    !strncmp(buf + 1, "TEST_SERVER", 11) || /* ignore test-server */
				    !strncmp(buf + 1, "ARCADE_SERVER", 13) /* assume non-arcadeserver stats */
				    )) {
					p1 = p2 = NULL;
					break;
				}

				//strcpy(buf, p1 + 1); // overlapping strings
				memmove(buf, p1 + 1, strlen(p1 + 1) + 1);
			}
			if (!p1 && !p2) continue;

			/* This 'F' check here is obsolete? We're checking above already, and these
			   checks down here should never occur, because 'G' line preceeds all 'F' lines. */
			if (buf[0] == 'F' && strstr(buf, "JOKEANGBAND")) discard = TRUE;
			if (buf[0] == 'F' && strstr(buf, "PET")) discard = TRUE;
			if (buf[0] == 'F' && strstr(buf, "NEUTRAL")) discard = TRUE;
			if (buf[0] == 'F' && strstr(buf, "ATTR_ANY")) multihued = TRUE;
			if (buf[0] == 'F' && strstr(buf, "ATTR_MULTI")) breathhued = TRUE;
			if (buf[0] == 'F' && strstr(buf, "ATTR_BASE")) basehued = TRUE;

			if (buf[0] == 'N' && strstr(buf, "Test Blob")) discard = TRUE;

			if (strlen(buf) < 5 || buf[0] != 'G') continue;

			p1 = buf + 2; /* monster symbol */
			p2 = strchr(p1, ':'); /* 1 before monster colour */
			break;
		}
		buf[3] = '\0';
		buf[5] = '\0';
		strcpy(monster_list_symbol[monster_list_idx], p2 + 1);
		strcat(monster_list_symbol[monster_list_idx], p1);

		monster_list_idx++;

		/* outdated client? */
		if (monster_list_idx == MAX_R_IDX) break;
	}

	if (discard) {
		/* flashy-thingy tiem */
		monster_list_idx--;
	}

	my_fclose(fff);
}
void monster_lore_aux(int ridx, int rlidx, char paste_lines[18][MSG_LEN]) {
	char buf[1024], *p1, *p2;
	FILE *fff;
	int l = 0, pl = -1, cl = strlen(cname);
	int pl_len = 80 - 3 - cl - 2; /* 80 - 3 - namelen = chars available per chat line; 3 for brackets+space, 2 for colour code */
	int msg_len_eff = MSG_LEN - cl - 5 - 3 - 8;
	int chars_per_pline = (msg_len_eff / pl_len) * pl_len; /* chars usable in a paste_lines[] */
	char tmp[MSG_LEN];

	/* actually use local r_info.txt - a novum */
	path_build(buf, 1024, ANGBAND_DIR_GAME, "r_info.txt");
	fff = my_fopen(buf, "r");
	if (fff == NULL) {
		//plog("Error: Your r_info.txt file is missing.");
		return;
	}

	while (0 == my_fgets(fff, buf, 1024)) {
		/* strip $/%..$/! conditions */
		p1 = p2 = buf; /* dummy, != NULL */
		while (buf[0] == '$' || buf[0] == '%') {
			p1 = strchr(buf + 1, '$');
			p2 = strchr(buf + 1, '!');
			if (!p1 && !p2) break;
			if (!p1) p1 = p2;
			else if (p2 && p2 < p1) p1 = p2;
			//strcpy(buf, p1 + 1); // overlapping strings
			memmove(buf, p1 + 1, strlen(p1 + 1) + 1);
		}
		if (!p1 && !p2) continue;

		if (strlen(buf) < 3 || buf[0] != 'N') continue;

		p1 = buf + 2; /* monster code */
		p2 = strchr(p1, ':'); /* 1 before monster name */
		if (!p2) continue; /* paranoia (broken file) */

		if (atoi(p1) != ridx) continue;

		/* print info */

		/* name */
		//Term_putstr(5, 5, -1, TERM_YELLOW, p2 + 1);
		strcpy(paste_lines[++pl], format("\377y%s (\377%c%c\377y, L%d, %d) ", /* need 1 space at the end to overwrite 'search result' */
			monster_list_name[rlidx],
			monster_list_symbol[rlidx][0],
			monster_list_symbol[rlidx][1],
			monster_list_level[rlidx],
			ridx));
		Term_putstr(5, 5, -1, TERM_YELLOW, paste_lines[pl] + 2); /* no need for \377y */

		/* fetch diz */
		strcpy(paste_lines[++pl], "\377u");
		while (0 == my_fgets(fff, buf, 1024)) {
			/* strip $/%..$/! conditions */
			p1 = p2 = buf; /* dummy, != NULL */
			while (buf[0] == '$' || buf[0] == '%') {
				p1 = strchr(buf + 1, '$');
				p2 = strchr(buf + 1, '!');
				if (!p1 && !p2) break;
				if (!p1) p1 = p2;
				else if (p2 && p2 < p1) p1 = p2;
				//strcpy(buf, p1 + 1); // overlapping strings
				memmove(buf, p1 + 1, strlen(p1 + 1) + 1);
			}
			if (!p1 && !p2) continue;

			if (strlen(buf) < 3) continue;
			if (buf[0] == 'N') break;
			if (buf[0] != 'D') continue;

			p1 = buf + 2; /* monster diz line */
			/* strip trailing spaces */
			while (p1[strlen(p1) - 1] == ' ') p1[strlen(p1) - 1] = '\0';
			Term_putstr(1, 7 + (l++), -1, TERM_UMBER, p1);

			/* add to chat-paste buffer. */
			while (p1) {
				/* diz line fits in pasteline? */
				if (strlen(paste_lines[pl]) + strlen(p1) <= chars_per_pline) {
					strcat(paste_lines[pl], p1);
					strcat(paste_lines[pl], " ");
					break;
				} else {
					/* diz line cannot be split? */
					if (!(p2 = strchr(p1, ' '))) {
						/* next pasteline */
						strcpy(paste_lines[++pl], "\377u");
						strcat(paste_lines[pl], p1);
						strcat(paste_lines[pl], " ");
						break;
					}
					/* diz line can be split? */
					else {
						strcpy(tmp, p1);
						tmp[p2 - p1] = '\0';
						/* split part is too big? */
						if (strlen(paste_lines[pl]) + strlen(tmp) > chars_per_pline) {
							/* next pasteline */
							strcpy(paste_lines[++pl], "\377u");
							strcat(paste_lines[pl], p1);
							strcat(paste_lines[pl], " ");
							break;
						}
						/* append split part */
						else {
							strcat(paste_lines[pl], tmp);
							strcat(paste_lines[pl], " ");
							p1 = p2 + 1;

							/* go on */
						}
					}
				}
			}
		}

		break;
	}

	my_fclose(fff);
}
const char *mon_flags2highlight[] = {"IM_COLD", "IM_FIRE", "IM_ACID", "IM_ELEC", "IM_POIS", "IM_WATER", ""};
const char *mon_flags2highlight2[] = {"SUSCEP_COLD", "SUSCEP_FIRE", "SUSCEP_ACID", "SUSCEP_ELEC", "SUSCEP_POIS", "HURT_LITE", "HURT_ROCK", ""};
//omitting SPIDER; note: DRAGON and DRAGONRIDER occur mutually exclusively, so we don't have to do extra string checks
const char *mon_flags2highlight3[] = {"ANIMAL", "ORC", "TROLL", "GIANT", "DRAGONRIDER", "DRAGON", "DEMON", "UNDEAD", "EVIL", "GOOD", "NONLIVING", ""};
const char *mon_flags2highlight4[] = {"UNIQUE", "NAZGUL", "PSEUDO_UNIQUE", "NO_DEATH", ""};//no hints about dungeon/game boss status available or used
const char *mon_flags2highlight5[] = {"NEUTRAL", "FRIENDLY", "PET", "QUESTOR", ""};//currently unavailable
const char *mon_flags2highlight6[] = {"AQUATIC", ""};
static int mon_highlit_flags(char *line) {
	const char **f = mon_flags2highlight;
	char *p2;
	int i = 0;

	while (*f[0]) {
		if ((p2 = strstr(line, *f)) &&
		    (p2 == line || *(p2 - 1) == ' ' || *(p2 - 2) == '\377'))//make sure the string is not just part of a different, longer flag string
			i += 4;
		f++;
	}

	f = mon_flags2highlight2;
	while (*f[0]) {
		if ((p2 = strstr(line, *f)) &&
		    (p2 == line || *(p2 - 1) == ' ' || *(p2 - 2) == '\377'))//make sure the string is not just part of a different, longer flag string
			i += 4;
		f++;
	}

	f = mon_flags2highlight3;
	while (*f[0]) {
		if ((p2 = strstr(line, *f)) &&
		    (p2 == line || *(p2 - 1) == ' ' || *(p2 - 2) == '\377')) { //make sure the string is not just part of a different, longer flag string
			if (!strcmp(*f, "DRAGONRIDER")) f++;//skip "DRAGON"
			i += 4;
		}
		f++;
	}

	f = mon_flags2highlight4;
	while (*f[0]) {
		if ((p2 = strstr(line, *f)) &&
		    (p2 == line || *(p2 - 1) == ' ' || *(p2 - 2) == '\377'))//make sure the string is not just part of a different, longer flag string
			i += 4;
		f++;
	}

	f = mon_flags2highlight5;
	while (*f[0]) {
		if ((p2 = strstr(line, *f)) &&
		    (p2 == line || *(p2 - 1) == ' ' || *(p2 - 2) == '\377'))//make sure the string is not just part of a different, longer flag string
			i += 4;
		f++;
	}

	f = mon_flags2highlight6;
	while (*f[0]) {
		if ((p2 = strstr(line, *f)) &&
		    (p2 == line || *(p2 - 1) == ' ' || *(p2 - 2) == '\377'))//make sure the string is not just part of a different, longer flag string
			i += 4;
		f++;
	}

	return i;
}
static void mon_highlight_flags(char *info) {
	const char **f = mon_flags2highlight, a_flag = 's';
	char info_tmp[MSG_LEN], *p2;

	while (*f[0]) {
		if ((p2 = strstr(info, *f)) && (p2 == info || *(p2 - 1) == ' ')) {
			strcpy(info_tmp, info);
			sprintf(info_tmp + (p2 - info), "\377w%s\377%c", *f, a_flag);
			strcat(info_tmp, p2 + strlen(*f));
			strcpy(info, info_tmp);
		}
		f++;
	}

	f = mon_flags2highlight2;
	while (*f[0]) {
		if ((p2 = strstr(info, *f)) && (p2 == info || *(p2 - 1) == ' ')) {
			strcpy(info_tmp, info);
			sprintf(info_tmp + (p2 - info), "\377y%s\377%c", *f, a_flag);
			strcat(info_tmp, p2 + strlen(*f));
			strcpy(info, info_tmp);
		}
		f++;
	}

	f = mon_flags2highlight3;
	while (*f[0]) {
		if ((p2 = strstr(info, *f)) && (p2 == info || *(p2 - 1) == ' ')) {
			strcpy(info_tmp, info);
			sprintf(info_tmp + (p2 - info), "\377o%s\377%c", *f, a_flag);
			strcat(info_tmp, p2 + strlen(*f));
			strcpy(info, info_tmp);
		}
		f++;
	}

	f = mon_flags2highlight4;
	while (*f[0]) {
		if ((p2 = strstr(info, *f)) && (p2 == info || *(p2 - 1) == ' ')) {
			strcpy(info_tmp, info);
			sprintf(info_tmp + (p2 - info), "\377U%s\377%c", *f, a_flag);
			strcat(info_tmp, p2 + strlen(*f));
			strcpy(info, info_tmp);
		}
		f++;
	}

	f = mon_flags2highlight5;
	while (*f[0]) {
		if ((p2 = strstr(info, *f)) && (p2 == info || *(p2 - 1) == ' ')) {
			strcpy(info_tmp, info);
			sprintf(info_tmp + (p2 - info), "\377G%s\377%c", *f, a_flag);
			strcat(info_tmp, p2 + strlen(*f));
			strcpy(info, info_tmp);
		}
		f++;
	}

	f = mon_flags2highlight6;
	while (*f[0]) {
		if ((p2 = strstr(info, *f)) && (p2 == info || *(p2 - 1) == ' ')) {
			strcpy(info_tmp, info);
			sprintf(info_tmp + (p2 - info), "\377B%s\377%c", *f, a_flag);
			strcat(info_tmp, p2 + strlen(*f));
			strcpy(info, info_tmp);
		}
		f++;
	}
}
void monster_stats_aux(int ridx, int rlidx, char paste_lines[18][MSG_LEN]) {
	char buf[1024], *p1, *p2, info[MSG_LEN], info_tmp[MSG_LEN];
	FILE *fff;
	int l = 0, info_val, pl = -1, drops = 0;
	int pf_col_cnt = 0; /* combine multiple [3] flag lines into one */
	/* for multiple definitions via $..$/!: */
	bool got_W_line = FALSE; /* usually only W: lines are affected, right? */
	int got_B_lines = 0; /* just for a header */
	bool got_F_lines = FALSE, got_S_lines = FALSE;
#if 0
	bool set_F_colon = FALSE, set_S_colon = FALSE;
#endif
	int f_col = 0; /* used for both, F flags and S flags */
	const char a_key = 'u', a_val = 's', a_atk = 's', a_flag = 's'; /* 'Speed:', 'Normal', 4xmelee */
	const char ta_key = TERM_UMBER, ta_val = TERM_SLATE, ta_atk = TERM_SLATE, ta_flag = TERM_SLATE; /* 'Speed:', 'Normal', 4xmelee */
	bool hands, fishy, spider;
	bool any_except_fingers, hands_or_claws; //for Disembodied hand/Hand Druj recognition hack

	/* actually use local r_info.txt - a novum */
	path_build(buf, 1024, ANGBAND_DIR_GAME, "r_info.txt");
	fff = my_fopen(buf, "r");
	if (fff == NULL) {
		//plog("Error: Your r_info.txt file is missing.");
		return;
	}

	while (0 == my_fgets(fff, buf, 1024)) {
		/* strip $/%..$/! conditions */
		p1 = p2 = buf; /* dummy, != NULL */
		while (buf[0] == '$' || buf[0] == '%') {
			p1 = strchr(buf + 1, '$');
			p2 = strchr(buf + 1, '!');
			if (!p1 && !p2) break;
			if (!p1) p1 = p2;
			else if (p2 && p2 < p1) p1 = p2;
			//strcpy(buf, p1 + 1); // overlapping strings
			memmove(buf, p1 + 1, strlen(p1 + 1) + 1);
		}
		if (!p1 && !p2) continue;

		if (strlen(buf) < 3 || buf[0] != 'N') continue;

		p1 = buf + 2; /* monster code */
		p2 = strchr(p1, ':'); /* 1 before monster name */
		if (!p2) continue; /* paranoia (broken file) */

		if (atoi(p1) != ridx) continue;

		/* print info */

		/* name */
		//Term_putstr(5, 5, -1, TERM_YELLOW, p2 + 1);
		strcpy(paste_lines[++pl], format("\377y%s (\377%c%c\377y, L%d, %d) ", /* need 1 space at the end to overwrite 'search result' */
			monster_list_name[rlidx],
			monster_list_symbol[rlidx][0],
			monster_list_symbol[rlidx][1],
			monster_list_level[rlidx],
			ridx));
		Term_putstr(5, 5, -1, TERM_YELLOW, paste_lines[pl] + 2); /* no need for \377y */

		/* specialty: tentacles count as finger-limbs (for rings) + hand-limbs (for weapon-wielding) + arm-limbs (shields)
		   (but cannot wear gloves!). So we only need to mention them once (under 'hands' above) */
		if (monster_list_symbol[rlidx][1] == '~') fishy = TRUE;
		else fishy = FALSE;
		if (monster_list_symbol[rlidx][1] == 'S') spider = TRUE;
		else spider = FALSE;

		/* fetch stats: I/W/E/O/B/F/S lines */
		while (0 == my_fgets(fff, buf, 1024)) {
			/* strip $/%..$/! conditions */
			p1 = p2 = buf; /* dummy, != NULL */
			while (buf[0] == '$' || buf[0] == '%') {
				p1 = strchr(buf + 1, '$');
				p2 = strchr(buf + 1, '!');
				if (!p1 && !p2) break;
				if (!p1) p1 = p2;
				else if (p2 && p2 < p1) p1 = p2;

				/* actually handle/assume certain features */
				if (buf[0] == '$' && *p1 == '!' && (
				    (!strncmp(buf + 1, "HALLOWEEN", 9) && strstr(buf, "JOKEANGBAND")) /* allow viewing HALLOWEEN monsters */
				    )) {
					p1 = p2 = NULL;
					break;
				}
				if (buf[0] == '$' && *p1 == '$' && (
				    !strncmp(buf + 1, "RPG_SERVER", 10) || /* assume non-rpgserver stats */
				    !strncmp(buf + 1, "TEST_SERVER", 11) || /* ignore test-server */
				    !strncmp(buf + 1, "ARCADE_SERVER", 13) /* assume non-arcadeserver stats */
				    )) {
					p1 = p2 = NULL;
					break;
				}

				//strcpy(buf, p1 + 1); // overlapping strings
				memmove(buf, p1 + 1, strlen(p1 + 1) + 1);
			}
			if (!p1 && !p2) continue;

			/* line invalid (too short) */
			if (strlen(buf) < 3) continue;

			/* end of this monster (aka beginning of next monster) */
			if (buf[0] == 'N') break;

			p1 = buf + 2; /* line of monster data, beginning of actual data */
			info[0] = '\0'; /* prepare empty info line */
			info_tmp[0] = '\0';

			switch(buf[0]) {
			case 'I': /* speed, hp, vision range, ac, alertness/sleep */
			    /* speed */
				p2 = strchr(p1, ':') + 1;
				info_val = atoi(p1) - 110;
				if (info_val != 0)
					sprintf(info_tmp, "\377%cSpeed: \377%c%s%d\377%c. ", a_key, a_val, info_val < 0 ? "Slow " : "Fast +", info_val, a_key);
				else
					sprintf(info_tmp, "\377%cSpeed: \377%cNormal\377%c. ", a_key, a_val, a_key);
				strcat(info, info_tmp);
				p1 = p2;
			    /* hp */
				p2 = strchr(p1, ':') + 1;
				snprintf(info_tmp, p2 - p1, "%s", p1);
				strcat(info, format("HP: \377%c%s\377%c. ", a_val, info_tmp, a_key));
				p1 = p2;
			    /* vision range */
				p2 = strchr(p1, ':') + 1;
				sprintf(info_tmp, "Radius: \377%c%d\377%c. ", a_val, atoi(p1), a_key);
				strcat(info, info_tmp);
				p1 = p2;
			    /* AC */
				p2 = strchr(p1, ':') + 1;
				sprintf(info_tmp, "AC: \377%c%d\377%c. ", a_val, atoi(p1), a_key);
				strcat(info, info_tmp);
				p1 = p2;
			    /* alertness */
				info_val = atoi(p1);
				if (info_val == 0) strcat(info, format("\377%cAwake\377%c.", a_val, a_key));
				else {
#if 0
					sprintf(info_tmp, "\377%cSleeps (%d)\377%c.", a_val, info_val, a_key);
					strcat(info, info_tmp);
#else
					/* use /cough (wakeup_monsters_somewhat()) effectiveness for reference :D */
					//16:1, 26:1 or 2
					if (info_val <= 26) sprintf(info_tmp, "\377%cSleeps lightly\377%c.", a_val, a_key);//1 or 2 coughs
					else if (info_val <= 80) sprintf(info_tmp, "\377%cSleeps\377%c.", a_val, a_key);//big chunk
					else if (info_val <= 120) sprintf(info_tmp, "\377%cSleeps\377%c.", a_val, a_key);//big chunk
					else sprintf(info_tmp, "\377%cSleeps deeply\377%c.", a_val, a_key);//small amount
					strcat(info, info_tmp);
#endif
				}
			    /* all done, display: */
				strcpy(paste_lines[++pl], format("\377%c", a_key));
				strcat(paste_lines[pl], info);
				Term_putstr(1, 7 + (l++), -1, ta_key, info);
				break;
			case 'W': /* depth, rarity, weight, exp */
				/* only process 1st definition we find (for $..$/! stuff): */
				if (got_W_line) continue;
				got_W_line = TRUE;
			    /* depth */
				p2 = strchr(p1, ':') + 1;
				sprintf(info_tmp, "\377%cLevel (depth): \377%c%d\377%c. ", a_key, a_val, atoi(p1), a_key);
				strcpy(info, info_tmp);
				p1 = p2;
			    /* rarity */
				p2 = strchr(p1, ':') + 1;
				info_val = atoi(p1);
				if (info_val == 1) {
					strcat(info_tmp, format("\377%cCommon\377%c. ", a_val, a_key));
					strcat(info, format("\377%cCommon\377%c. ", a_val, a_key));
				} else if (info_val <= 3) {
					strcat(info_tmp, format("\377%cL.common (%d)\377%c. ", a_val, info_val, a_key));
					strcat(info, format("\377%cLess common (%d)\377%c. ", a_val, info_val, a_key));
				} else if (info_val == 255) {
					strcat(info_tmp, format("\377%cUnfindable (%d)\377%c. ", a_val, info_val, a_key));
					strcat(info, format("\377%cUnfindable (%d)\377%c. ", a_val, info_val, a_key));
				} else {
					strcat(info_tmp, format("\377%cRare (%d)\377%c. ", a_val, info_val, a_key));
					strcat(info, format("\377%cRare (%d)\377%c. ", a_val, info_val, a_key));
				}
				p1 = p2;
			    /* weight */
				p2 = strchr(p1, ':') + 1;
				info_val = atoi(p1) / 10;
				if (info_val <= 100) {
					strcat(info_tmp, format("\377%cLight (%d lb)\377%c. ", a_val, info_val, a_key));
					strcat(info, format("\377%cLight (%d lb)\377%c. ", a_val, info_val, a_key));
				} else if (info_val <= 450) {
					strcat(info_tmp, format("\377%cMedium (%d lb)\377%c. ", a_val, info_val, a_key));
					strcat(info, format("\377%cMedium (%d lb)\377%c. ", a_val, info_val, a_key));
				} else if (info_val <= 2000) {
					strcat(info_tmp, format("\377%cHeavy (%d lb)\377%c. ", a_val, info_val, a_key));
					strcat(info, format("\377%cHeavy (%d lb)\377%c. ", a_val, info_val, a_key));
				} else {
					strcat(info_tmp, format("\377%cS-heavy (%d lb)\377%c. ", a_val, info_val, a_key));
					strcat(info, format("\377%cSuper-heavy (%d lb)\377%c. ", a_val, info_val, a_key));
				}
				p1 = p2;
			    /* exp */
				strcat(info_tmp, format("XP: \377%c%d\377%c.", a_val, atoi(p1), a_key));
				strcat(info, format("XP: \377%c%d\377%c.", a_val, atoi(p1), a_key));
			    /* all done, display: */
				sprintf(paste_lines[++pl], "\377%c", a_key);
				strcat(paste_lines[pl], info_tmp);
				Term_putstr(1, 7 + (l++), -1, ta_key, info);
				break;
			case 'E': /* weapons, torso, arms, fingers, head, legs */
				any_except_fingers = FALSE;
				hands_or_claws = FALSE;
				sprintf(info, "Usable limbs (mimicry users): \377%c", a_val);
				strcpy(info_tmp, "");
				info_val = 0;
			    /* weapons */
				p2 = strchr(p1, ':') + 1;
				if (atoi(p1)) {
					any_except_fingers = TRUE;
					/* specialty: tentacles count as finger-limbs + hand-limbs (for weapon-wielding)*/
					//todo: correct plural, most r_info entries only have a 1 here
					if (fishy) strcat(info_tmp, "Tentacles");
					else strcat(info_tmp, "Hands");
					info_val = 1;
					hands = TRUE;
				} else hands = FALSE;
				p1 = p2;
			    /* torso */
				p2 = strchr(p1, ':') + 1;
				if (atoi(p1)) {
					any_except_fingers = TRUE;
					if (info_val) strcat(info_tmp, format("\377%c, \377%ctorso", a_key, a_val));
					else strcat(info_tmp, "Torso");
					info_val = 1;
				}
				p1 = p2;
			    /* arms */
				p2 = strchr(p1, ':') + 1;
				if (atoi(p1) && !fishy) {
					any_except_fingers = TRUE;
					if (info_val) strcat(info_tmp, format("\377%c, \377%carm%s", a_key, a_val, atoi(p1) == 1 ? "" : "s"));
					else strcat(info_tmp, format("Arm%s", atoi(p1) == 1 ? "" : "s"));
					info_val = 1;
					spider = FALSE; //unhack: for driders
				}
				p1 = p2;
			    /* fingers (or sometimes called claws/tentacles.. no functional difference)
			       -- spiders are hacky, they got pseudo-'fingers' to enable them to be more effective */
				p2 = strchr(p1, ':') + 1;
				if (atoi(p1) && !fishy && !spider) {
					hands_or_claws = TRUE;
					if (hands) {
						if (info_val) strcat(info_tmp, format("\377%c, \377%cfinger%s", a_key, a_val, atoi(p1) == 1 ? "" : "s"));
						else strcat(info_tmp, format("Finger%s", atoi(p1) == 1 ? "" : "s"));
					} else {
						if (info_val) strcat(info_tmp, format("\377%c, \377%cclaw%s", a_key, a_val, atoi(p1) == 1 ? "" : "s"));
						else strcat(info_tmp, format("Claw%s", atoi(p1) == 1 ? "" : "s"));
					}
					info_val = 1;
				}
				p1 = p2;
			    /* head */
				p2 = strchr(p1, ':') + 1;
				if (atoi(p1)) {
					any_except_fingers = TRUE;
					if (info_val) strcat(info_tmp, format("\377%c, \377%chead%s", a_key, a_val, atoi(p1) == 1 ? "" : "s"));
					else strcat(info_tmp, format("Head%s", atoi(p1) == 1 ? "" : "s"));
					info_val = 1;
				}
				p1 = p2;
			    /* legs */
				if (atoi(p1)) {
					any_except_fingers = TRUE;
					if (info_val) strcat(info_tmp, format("\377%c, \377%cleg%s", a_key, a_val, atoi(p1) == 1 ? "" : "s"));
					else strcat(info_tmp, format("Leg%s", atoi(p1) == 1 ? "" : "s"));
					info_val = 1;
				}
			    /* all done, display: */
				/* hack for Disembondied hand and Hand Druj: */
				if (!any_except_fingers && hands_or_claws) {
					strcpy(info_tmp, "Hand, fingers");
					info_val = 1;
				}

				if (!info_val) strcat(info_tmp, "None");
				strcat(info_tmp, ".");
				sprintf(paste_lines[++pl], "\377%cLimbs (mimicry): \377%c", a_key, a_val);
				strcat(paste_lines[pl], info_tmp);
				strcat(info, info_tmp);
				Term_putstr(1, 7 + (l++), -1, ta_key, info);
				break;
			case 'O': /* treasure, combat, magic, tool */
				strcpy(info, "Usual drops: ");
				sprintf(info_tmp, "\377%cDrops: ", a_key);
				info_val = drops = 0;
			    /* treasure */
				p2 = strchr(p1, ':') + 1;
				if (atoi(p1)) {
					strcat(info_tmp, format("\377%c%d%% valuables\377%c", a_val, atoi(p1), a_key));
					strcat(info, format("\377%c%d%% valuables\377%c", a_val, atoi(p1), a_key));
					info_val = 1;
					drops += atoi(p1);
				}
				p1 = p2;
			    /* combat */
				p2 = strchr(p1, ':') + 1;
				if (atoi(p1)) {
					if (info_val) {
						strcat(info_tmp, ", ");
						strcat(info, ", ");
					}
					strcat(info_tmp, format("\377%c%d%% combat\377%c", a_val, atoi(p1), a_key));
					strcat(info, format("\377%c%d%% combat\377%c", a_val, atoi(p1), a_key));
					info_val = 1;
					drops += atoi(p1);
				}
				p1 = p2;
			    /* magic */
				p2 = strchr(p1, ':') + 1;
				if (atoi(p1)) {
					if (info_val) {
						strcat(info_tmp, ", ");
						strcat(info, ", ");
					}
					strcat(info_tmp, format("\377%c%d%% magic\377%c", a_val, atoi(p1), a_key));
					strcat(info, format("\377%c%d%% magic items\377%c", a_val, atoi(p1), a_key));
					info_val = 1;
					drops += atoi(p1);
				}
				p1 = p2;
			    /* tool */
				if (atoi(p1)) {
					if (info_val) {
						strcat(info_tmp, ", ");
						strcat(info, ", ");
					}
					strcat(info_tmp, format("\377%c%d%% tools\377%c", a_val, atoi(p1), a_key));
					strcat(info, format("\377%c%d%% tools\377%c", a_val, atoi(p1), a_key));
					info_val = 1;
					drops += atoi(p1);
				}
			    /* all done, display: */
				if (!info_val) {
					strcat(info_tmp, format("\377%cNothing or any item\377%c", a_val, a_key));
					strcat(info, format("\377%cNothing or any item\377%c", a_val, a_key));
				} else if (drops < 100) {
					strcat(info_tmp, ", ");
					strcat(info, ", ");
					strcat(info_tmp, format("\377%c%d%% junk\377%c", a_val, 100 - drops, a_key));
					strcat(info, format("\377%c%d%% junk\377%c", a_val, 100 - drops, a_key));
				}
				strcat(info_tmp, ".");
				strcat(info, ".");
				strcpy(paste_lines[++pl], info_tmp);
				Term_putstr(1, 7 + (l++), -1, ta_key, info);
				break;
			case 'B': /* attack method, attack effect, attack damage */
				if (!got_B_lines) {
					Term_putstr(1, 7 + (l++), -1, ta_key, "Melee attacks (see guide for explanation):");
					strcpy(paste_lines[++pl], format("\377%c", a_atk));
				}
				got_B_lines++;
				strcat(info, p1);
				strcat(paste_lines[pl], info);
				strcat(paste_lines[pl], " ");
				Term_putstr(2 + (got_B_lines - 1) * 19, 7 + l, -1, ta_atk, info);
				break;
			case 'F': /* flags */
				if (!got_F_lines) {
					/* hack: monster doesn't have melee attacks! */
					if (!got_B_lines) {
						Term_putstr(1, 7 + l, -1, ta_key, "No melee attacks.");
						got_B_lines = 1;
					}

					/* fix missing newline from 'B:' lines */
					l++;
					Term_putstr(1, 7 + l, -1, ta_key, "Flags:");
					strcpy(paste_lines[++pl], format("\377%cFlags: \377%c", a_key, a_flag));
					f_col = 8;
					got_F_lines = TRUE;
				}

				/* strip all spaces, convert | to space */
				p1--;
				while (*(++p1)) {
					switch (*p1) {
					case ' ': continue;
					case '|': *p1 = ' ';
					default: strcat(info, format("%c", *p1));
					}
				}
				/* add a pseudo '|' (ie a space) at the end
				   (for when two lines are merged -> the space would be missing) */
				if (info[strlen(info) - 1] != ' ') strcat(info, " ");

				/* Strip flags that are useless to know */
				if ((p2 = strstr(info, "PLURAL"))) {
					strcpy(info_tmp, info);
					info_tmp[(p2 - info)] = '\0';
					strcat(info_tmp, p2 + 7);
					strcpy(info, info_tmp);
				}
				if ((p2 = strstr(info, "SPECIAL_GENE"))) {/* currently not doing anything for monsters, only for artifacts */
					strcpy(info_tmp, info);
					info_tmp[(p2 - info)] = '\0';
					strcat(info_tmp, p2 + 13);
					strcpy(info, info_tmp);
				}
				if ((p2 = strstr(info, "FORCE_SLEEP"))) {/* doesn't do anything anymore (was: start out with low energy) */
					strcpy(info_tmp, info);
					info_tmp[(p2 - info)] = '\0';
					strcat(info_tmp, p2 + 12);
					strcpy(info, info_tmp);
				}

				/* more unused/inactive flags */
				if ((p2 = strstr(info, "DROP_CORPSE"))) {
					strcpy(info_tmp, info);
					info_tmp[(p2 - info)] = '\0';
					strcat(info_tmp, p2 + 12);
					strcpy(info, info_tmp);
				}
				if ((p2 = strstr(info, "DROP_SKELETON"))) {
					strcpy(info_tmp, info);
					info_tmp[(p2 - info)] = '\0';
					strcat(info_tmp, p2 + 14);
					strcpy(info, info_tmp);
				}
				if ((p2 = strstr(info, "HAS_EGG"))) {
					strcpy(info_tmp, info);
					info_tmp[(p2 - info)] = '\0';
					strcat(info_tmp, p2 + 8);
					strcpy(info, info_tmp);
				}
				if ((p2 = strstr(info, "IMPRESSED"))) {
					strcpy(info_tmp, info);
					info_tmp[(p2 - info)] = '\0';
					strcat(info_tmp, p2 + 10);
					strcpy(info, info_tmp);
				}
				if ((p2 = strstr(info, "MORTAL"))) {
					strcpy(info_tmp, info);
					info_tmp[(p2 - info)] = '\0';
					strcat(info_tmp, p2 + 7);
					strcpy(info, info_tmp);
				}
				if ((p2 = strstr(info, "POSSESSOR"))) {
					strcpy(info_tmp, info);
					info_tmp[(p2 - info)] = '\0';
					strcat(info_tmp, p2 + 10);
					strcpy(info, info_tmp);
				}
				if ((p2 = strstr(info, "DEATH_ORB"))) {
					strcpy(info_tmp, info);
					info_tmp[(p2 - info)] = '\0';
					strcat(info_tmp, p2 + 10);
					strcpy(info, info_tmp);
				}
				if ((p2 = strstr(info, "AI_SPECIAL"))) {
					strcpy(info_tmp, info);
					info_tmp[(p2 - info)] = '\0';
					strcat(info_tmp, p2 + 11);
					strcpy(info, info_tmp);
				}

				/* Highlight certain important flags for quick readability */
				mon_highlight_flags(info);

#ifdef NEW_PASTELINE_METHOD
				/* add flags to existing paste_lines line */
				/* 8 = 6 for colour codes etc around cname + 2 for paranoia + 4 for Party/Guild chat mode tag */
				p1 = info;
				while (*p1 == ' ') p1++;
				if (*p1) {//paranoia

					if (strlen(paste_lines[pl]) + strlen(p1) < MSG_LEN - 1 - strlen(cname) - 8 - 4 - 7) {//7 = world server tax (pure paranoia here)
						strcat(paste_lines[pl], p1);
					} else { /* split it up */
						if (!(p2 = strchr(p1 + 1, ' '))) { /* can't split? */
							/* start next line */
							strcpy(paste_lines[++pl], format("\377%c%s", a_flag, p1));
						} else { /* split ok */
							char *p3;
							int p;

							p3 = p2 = p1;
							while ((p2 = strchr(p2 + 1, ' ')) && strlen(paste_lines[pl]) + strlen(p1) - strlen(p2) < MSG_LEN - 1 - strlen(cname) - 8 - 4 - 7) p3 = p2;
							//paste_lines[pl][strlen(paste_lines[pl]) + strlen(p1) - strlen(p3) - 1] = 0;
							p = strlen(paste_lines[pl]) + strlen(p1) - strlen(p3);
							strncat(paste_lines[pl], p1, strlen(p1) - strlen(p3));
							paste_lines[pl][p] = 0;
							while (*p3 == ' ') p3++;
							strcpy(paste_lines[++pl], format("\377%c%s", a_flag, p3));
						}
					}
				}
#endif

				/* add flags to existing line */
				p1 = info;
				while (*p1) {
					while (*p1 == ' ') p1++;
					if (*p1 == 0) break;

					/* add complete flag line */
					if (strlen(p1) + f_col < 80) {
#ifndef NEW_PASTELINE_METHOD
						strcat(paste_lines[pl], p1);
#endif
						Term_putstr(f_col, 7 + l, -1, ta_flag, p1);
						f_col += strlen(p1);
						f_col -= mon_highlit_flags(p1);

						/* done */
						break;
					}
					/* try to split up the line */
					else {
						/* can't split up F line? */
						if (!(p2 = strchr(p1, ' '))) {
							/* hack: catch special case: only the final space didn't fit into the line -> discard it instead */
							if (*p1 == 0) break;

							/* start next line */
							l++;
							if (++pf_col_cnt == 2) {
								pf_col_cnt = 0;
#ifndef NEW_PASTELINE_METHOD
								pl++;
								strcpy(paste_lines[pl], format("\377%c", a_flag));
#endif
							}
#ifndef NEW_PASTELINE_METHOD
							strcat(paste_lines[pl], p1);
#endif
							Term_putstr(2, 7 + l, -1, ta_flag, p1);
							f_col = 2 + strlen(p1);
							f_col -= mon_highlit_flags(p1);

							/* done */
							break;
						}
						/* add partial flag line */
						else {
							/* split part too large to fit into this line? */
							if (p2 - p1 + f_col >= 80) {
								/* start next line */
								l++;
								if (++pf_col_cnt == 2) {
									pf_col_cnt = 0;
#ifndef NEW_PASTELINE_METHOD
									pl++;
									strcpy(paste_lines[pl], format("\377%c", a_flag));
#endif
								}
#ifndef NEW_PASTELINE_METHOD
								strcat(paste_lines[pl], p1);
#endif
								Term_putstr(2, 7 + l, -1, ta_flag, p1);
								f_col = 2 + strlen(p1);
								f_col -= mon_highlit_flags(p1);

								/* done */
								break;
							}
							/* append split part */
							else {
								strcpy(info_tmp, p1);
								info_tmp[p2 - p1 + 1] = '\0';
#ifndef NEW_PASTELINE_METHOD
								strcat(paste_lines[pl], info_tmp);
#endif
								Term_putstr(f_col, 7 + l, -1, ta_flag, info_tmp);
								f_col += strlen(info_tmp);
								f_col -= mon_highlit_flags(info_tmp);
								p1 = p2 + 1;

								/* go on */
							}
						}
					}
				}
				break;
			case 'S': /* 1st: spell frequency; 2nd..x: spells */
#if 0
				/* add a colon at the end of the FLAGS list */
				if (got_F_lines && !set_F_colon) {
					Term_putstr(f_col - 1, 7 + l, -1, ta_key, ".");
					sprintf(&paste_lines[pl][strlen(paste_lines[pl]) - 1], "\377%c.", a_key);
					set_F_colon = TRUE;
				}
#endif

				/* strip spaces, convert | to space */
				p1--;
				while (*(++p1)) {
					switch (*p1) {
					case ' ': continue;
					case '|': *p1 = ' ';
					default: strcat(info, format("%c", *p1));
					}
				}
				p1 = info;
				/* add a pseudo '|' (ie a space) at the end
				   (for when two lines are merged -> the space would be missing) */
				if (info[strlen(info) - 1] != ' ') strcat(info, " ");

				/* Strip flags that are useless to know */
				if ((p2 = strstr(info, "RAISE_DEAD"))) {
					strcpy(info_tmp, info);
					info_tmp[(p2 - info)] = '\0';
					strcat(info_tmp, p2 + 11);
					strcpy(info, info_tmp);
				}

				if (!got_S_lines) {
					got_S_lines = TRUE;
					/* fix missing newline from 'B:' lines */
					l++;
					/* evaluate spell frequency right away - assume 'S:1_IN_...' exact format */
					info_val = atoi(info + 5);
					sprintf(info_tmp, "Abilities (~1 in %d turns): \377%c", info_val, a_val);
					sprintf(paste_lines[++pl], "\377%cAbilities (1 in %d): \377%c", a_key, info_val, a_val);
					Term_putstr(1, 7 + l, -1, ta_key, info_tmp);
					f_col = 1 + strlen(info_tmp) - 2; /* -2 for colour code! */

					/* advance to first actual flag (if any in this line) */
					sprintf(info_tmp, "%d", info_val);
					p1 = info + 5 + strlen(info_tmp);
					while (*p1 == ' ') p1++;
				}

				/* add flags to existing line */
				while (*p1) {
					while (*p1 == ' ') p1++;
					if (*p1 == 0) break;

					/* add complete flag line */
					if (strlen(p1) + f_col < 80) {
						strcat(paste_lines[pl], p1);
						Term_putstr(f_col, 7 + l, -1, ta_val, p1);
						f_col += strlen(p1);

						/* done */
						break;
					}
					/* try to split up the line */
					else {
						/* can't split up S line? */
						if (!(p2 = strchr(p1, ' '))) {
							/* hack: catch special case: only the final space didn't fit into the line -> discard it instead */
							if (*p1 == 0) break;

							/* start next line */
							l++;
							if (++pf_col_cnt == 2) {
								pf_col_cnt = 0;
								pl++;
								strcpy(paste_lines[pl], format("\377%c", a_val));
							}
							strcat(paste_lines[pl], p1);
							Term_putstr(2, 7 + l, -1, ta_val, p1);
							f_col = 2 + strlen(p1);

							/* done */
							break;
						}
						/* add partial flag line */
						else {
							/* split part too large to fit into this line? */
							if (p2 - p1 + f_col >= 80) {
								/* start next line */
								l++;
								if (++pf_col_cnt == 2) {
									pf_col_cnt = 0;
									pl++;
									strcpy(paste_lines[pl], format("\377%c", a_val));
								}
								strcat(paste_lines[pl], p1);
								Term_putstr(2, 7 + l, -1, ta_val, p1);
								f_col = 2 + strlen(p1);

								/* done */
								break;
							}
							/* append split part */
							else {
								strcpy(info_tmp, p1);
								info_tmp[p2 - p1 + 1] = '\0';
								strcat(paste_lines[pl], info_tmp);
								Term_putstr(f_col, 7 + l, -1, ta_val, info_tmp);
								f_col += strlen(info_tmp);
								p1 = p2 + 1;

								/* go on */
							}
						}
					}
				}
				break;
			}
		}

		break;
	}

	my_fclose(fff);
}

/* Init kind list for displaying artifact lore with full item names - C. Blue */
static void init_kind_list() {
	char buf[1024], *p1, *p2;
	int v1 = 0, v2 = 0, v3 = 0;
	FILE *fff;

	/* actually use local k_info.txt - a novum */
	path_build(buf, 1024, ANGBAND_DIR_GAME, "k_info.txt");
	fff = my_fopen(buf, "r");
	if (fff == NULL) {
		//plog("Error: Your k_info.txt file is missing.");
		return;
	}

	buf[0] = 0;
	while (buf[0] == 'N' || 0 == my_fgets(fff, buf, 1024)) {
		/* strip $/%..$/! conditions */
		p1 = p2 = buf; /* dummy, != NULL */
		while (buf[0] == '$' || buf[0] == '%') {
			p1 = strchr(buf + 1, '$');
			p2 = strchr(buf + 1, '!');
			if (!p1 && !p2) break;
			if (!p1) p1 = p2;
			else if (p2 && p2 < p1) p1 = p2;
			//strcpy(buf, p1 + 1); // overlapping strings
			memmove(buf, p1 + 1, strlen(p1 + 1) + 1);
		}
		if (!p1 && !p2) continue;

		if (strlen(buf) < 3) continue;
		if (buf[0] == 'V' && buf[1] == ':') {
			sscanf(buf + 2, "%d.%d.%d", &v1, &v2, &v3);
			continue;
		}
		if (buf[0] != 'N') continue;
		buf[0] = '_'; /* random dummy */

		if (v1 < 4 || (v1 == 4 && (v2 < 0 || (v2 == 0 && v3 < 1)))) {
			//plog("Error: Your k_info.txt file is outdated.");
			return;
		}

		p1 = buf + 2; /* kind code */
		p2 = strchr(p1, ':'); /* 1 before kind name */
		if (!p2) continue; /* paranoia (broken file) */
		p2++;

		/* hack - skip item 0, because it doesn't have tval/sval */
		if (atoi(p1) == 0) continue;

		/* strip '& ' and '~' */
		strcpy(kind_list_name[kind_list_idx], "");
		while (*p2) {
			if (*p2 == '~') {
				p2++;
				continue;
			}
			if (*p2 == '&') {
				p2 += 2;
				continue;
			}
			strcat(kind_list_name[kind_list_idx], format("%c", *p2));
			p2++;
		}

		/* fetch I line for tval/sval */
		while (0 == my_fgets(fff, buf, 1024)) {
			/* strip $/%..$/! conditions */
			p1 = p2 = buf; /* dummy, != NULL */
			while (buf[0] == '$' || buf[0] == '%') {
				p1 = strchr(buf + 1, '$');
				p2 = strchr(buf + 1, '!');
				if (!p1 && !p2) break;
				if (!p1) p1 = p2;
				else if (p2 && p2 < p1) p1 = p2;
//				strcpy(buf, p1 + 1); // overlapping strings
				memmove(buf, p1 + 1, strlen(p1 + 1) + 1);
			}
			if (!p1 && !p2) continue;

			/* proceeded to next entry? */
			if (buf[0] == 'N') break;

			if (strlen(buf) < 3) continue;
			else if (buf[0] == 'A') { /* depth, rarity +  (--note: Does not account for extra rarity increase due to OOD discrepancy between k-depth and a-depth) */
				int best_rar = 255;
				while (TRUE) {
					if (!strchr(p1, ':')) break;
					/* <ignore depth> */
					p2 = strchr(p1, ':') + 1;
					p1 = p2;

					/* fetch rarity */
					p2 = strchr(p1, '/') + 1;
					p1 = p2;
					if (atoi(p1) < best_rar) best_rar = atoi(p1);
				}
				kind_list_rarity[kind_list_idx] = best_rar;
			} else if (buf[0] == 'I') {
				p1 = buf + 2; /* tval */
				p2 = strchr(p1, ':') + 1; /* sval */
				if (!p2) continue; /* paranoia (broken file) */

				kind_list_tval[kind_list_idx] = atoi(p1);
				kind_list_sval[kind_list_idx] = atoi(p2);

				/* complete certain names that are treated in a special way */
				if (kind_list_tval[kind_list_idx] == TV_TRAPKIT)
					strcat(kind_list_name[kind_list_idx], " Trap Kit");
			} else if (buf[0] == 'G') {
				p1 = buf + 2; /* char */
				p2 = strchr(p1, ':') + 1; /* attr */
				if (!p2) continue; /* paranoia (broken file) */

				kind_list_char[kind_list_idx] = *p1;
				kind_list_attr[kind_list_idx] = *p2;
			}
			continue;
		}

#if 0 /* changed into a hack in init_artifact_list actually! */
		/* hack to initialise INSTA_ARTs' rarity: They don't have a k-rarity! */
		if (!kind_list_rarity[kind_list_idx]) kind_list_rarity[kind_list_idx] = 1;
#endif

		kind_list_idx++;

		/* outdated client? */
		if (kind_list_idx == MAX_K_IDX) break;
	}

	my_fclose(fff);
}

/* Init artifact list for displaying artifact lore - C. Blue */
static void init_artifact_list() {
	char buf[1024], *p1, *p2, art_name[MSG_LEN], buf2[1024];
	FILE *fff;
	int tval = 0, sval = 0, i, v1 = 0, v2 = 0, v3 = 0, rar, lev;
	bool discard, special_gene, flag_break_marker;
	bool next_art_name_ok, premature_next_art = FALSE;

	/* actually use local r_info.txt - a novum */
	path_build(buf, 1024, ANGBAND_DIR_GAME, "a_info.txt");
	fff = my_fopen(buf, "r");
	if (fff == NULL) {
		//plog("Error: Your a_info.txt file is missing.");
		return;
	}

	while (premature_next_art || 0 == my_fgets(fff, buf, 1024)) {
		if (premature_next_art) {
			strcpy(buf, buf2); //for premature_next_art
			premature_next_art = FALSE;
		}
		/* strip $/%..$/! conditions */
		p1 = p2 = buf; /* dummy, != NULL */
		while (buf[0] == '$' || buf[0] == '%') {
			p1 = strchr(buf + 1, '$');
			p2 = strchr(buf + 1, '!');
			if (!p1 && !p2) break;
			if (!p1) p1 = p2;
			else if (p2 && p2 < p1) p1 = p2;
			//strcpy(buf, p1 + 1); // overlapping strings
			memmove(buf, p1 + 1, strlen(p1 + 1) + 1);

			if (buf[0] == '$' && *p1 == '$' && (
			    !strncmp(buf + 1, "RPG_SERVER", 10) || /* assume non-rpgserver stats */
			    !strncmp(buf + 1, "TEST_SERVER", 11) || /* ignore test-server */
			    !strncmp(buf + 1, "ARCADE_SERVER", 13) /* assume non-arcadeserver stats */
			    )) {
				p1 = p2 = NULL;
				break;
			}
		}
		if (!p1 && !p2) continue;
		if (strlen(buf) < 3) continue;
		if (buf[0] == 'V' && buf[1] == ':') {
			sscanf(buf + 2, "%d.%d.%d", &v1, &v2, &v3);
			continue;
		}
		if (buf[0] != 'N') continue;
		if (v1 < 3 || (v1 == 3 && (v2 < 4 || (v2 == 4 && v3 < 1)))) {
			//plog("Error: Your a_info.txt file is outdated.");
			return;
		}

		p1 = buf + 2; /* artifact code */
		p2 = strchr(p1, ':'); /* 1 before artifact name */
		if (!p2 || strlen(p2 + 1) <= 1) continue; /* !p2: paranoia (broken file); strlen: #201 template */

		artifact_list_code[artifact_list_idx] = atoi(p1);
		strcpy(artifact_list_name[artifact_list_idx], "");
		strcpy(art_name, p2 + 1);
		discard = FALSE;
		special_gene = FALSE;
		/* artifact entry ends after F lines.. */
		flag_break_marker = FALSE;
		/* ..but some don't have those (Khazad!), so we need to test for next N line instead */
		next_art_name_ok = FALSE;

		/* fetch tval,sval and lookup type name in k_info */
		rar = 1; lev = 0; /* paranoia/kill compiler warning */
		while (0 == my_fgets(fff, buf, 1024)) {
			strcpy(buf2, buf); //for premature_next_art
			/* strip $/%..$/! conditions */
			p1 = p2 = buf; /* dummy, != NULL */
			while (buf[0] == '$' || buf[0] == '%') {
				p1 = strchr(buf + 1, '$');
				p2 = strchr(buf + 1, '!');
				if (!p1 && !p2) break;
				if (!p1) p1 = p2;
				else if (p2 && p2 < p1) p1 = p2;
				//strcpy(buf, p1 + 1); // overlapping strings
				memmove(buf, p1 + 1, strlen(p1 + 1) + 1);

				if (buf[0] == '$' && *p1 == '$' && (
				    !strncmp(buf + 1, "RPG_SERVER", 10) || /* assume non-rpgserver stats */
				    !strncmp(buf + 1, "TEST_SERVER", 11) || /* ignore test-server */
				    !strncmp(buf + 1, "ARCADE_SERVER", 13) /* assume non-arcadeserver stats */
				    )) {
					p1 = p2 = NULL;
					break;
				}
			}
			if (!p1 && !p2) continue;

			/* HACK: done with scanning flags? -- note this is non-canonical (empty lines are actually allowed), ew */
			if (flag_break_marker && buf[0] != 'F') break;
			/* hack: Next artifact name? then we're done too */
			if (buf[0] == 'N' && next_art_name_ok) {
				premature_next_art = TRUE;
				break;
			}
			if (buf[0] == 'P') {
				next_art_name_ok = TRUE;
				continue;
			}

			if (strlen(buf) < 3 || (buf[0] != 'I' && buf[0] != 'W' && buf[0] != 'F')) continue;
			if (buf[0] == 'I') {
				p1 = buf + 2; /* tval */
				p2 = strchr(p1, ':') + 1; /* sval */
				tval = atoi(p1);
				sval = atoi(p2);
				continue;
			} else if (buf[0] == 'F') {
				flag_break_marker = TRUE;
				if (strstr(buf, "SPECIAL_GENE")) {
					special_gene = TRUE;
					break;
				}
				continue;
			} else { /* 'W' -- scan rarity */
				p1 = buf + 2;
				lev = atoi(p1);
				p2 = strchr(p1, ':') + 1;
				rar = atoi(p2);
				/* check for 'disabled' hack */
				if (rar == 255) discard = TRUE;
				continue;
			}
		}
		/* flashy-thingy us? */
		if (discard) continue;

		/* lookup type name */
		for (i = 0; i < MAX_K_IDX; i++) {
			if (kind_list_tval[i] == tval &&
			    kind_list_sval[i] == sval) {
				strcpy(artifact_list_name[artifact_list_idx], "The ");
				strcat(artifact_list_name[artifact_list_idx], kind_list_name[i]);
				strcat(artifact_list_name[artifact_list_idx], " ");
				break;
			}
		}
		/* complete the artifact name */
		strcat(artifact_list_name[artifact_list_idx], art_name);
		/* new: add [coloured?] symbol to indicate item type (sometimes impossible to recognize otherwise) */
		if (i < MAX_K_IDX) { //paranoia?
#if 0 /* we cannot predict the attr for flavoured items! (rings, amulets) */
			strcpy(buf, format(" <\377%c%c\377->", kind_list_attr[i], kind_list_char[i]));
#else
			strcpy(buf, format("%c  ", kind_list_char[i]));
#endif
			/* new: add artifact level too */
			strcat(buf, format("L%-3d  ", lev));
			/* ..and finish with the actual name */
			strcat(buf, artifact_list_name[artifact_list_idx]);
			strcpy(artifact_list_name[artifact_list_idx], buf);
		} else {
			strcpy(buf, "         ");
			strcat(buf, artifact_list_name[artifact_list_idx]);
			strcpy(artifact_list_name[artifact_list_idx], buf);
		}

		/* remember if it's a dungeon boss drop */
		artifact_list_specialgene[artifact_list_idx] = special_gene;

		/* useful 'byproduct', since we had to scan for 255 rarity anyway */
		if (!rar) rar = 1; /* identical actually (and here to prevent divby0) */
#if 0
		/* hack: recognize insta-arts and calculate their rarity in a funny way (scale up into our 10000 array) */
		if (!kind_list_rarity[i]) artifact_list_rarity[artifact_list_idx] = ((10000 / rar) * (10000 / rar)) / 100;
		/* normal artifacts: */
		else artifact_list_rarity[artifact_list_idx] = ((10000 / kind_list_rarity[i]) * (10000 / rar)) / 100;
#else /* bad hack: manipulate rarity so that if either k-rarity or a-rarity is very high, it outweighs the other one -_- */
		/* hack: recognize insta-arts and calculate their rarity in a funny way (scale up into our 10000 array) */
		if (!kind_list_rarity[i]) artifact_list_rarity[artifact_list_idx] = ((10000 / rar) * (10000 / rar)) / 100;
		/* normal artifacts: */
		else {
			int krar = kind_list_rarity[i], krar_boost, rar_boost;
			krar_boost = krar + (krar * krar) / 500;
			rar_boost = rar + (rar * rar) / 500;
			if (rar < krar) {
				if (rar + krar_boost < 100) rar += krar_boost;
				else rar = 100;
			}
			if (krar < rar) {
				if (krar + rar_boost < 100) krar += rar_boost;
				else krar = 100;
			}
			artifact_list_rarity[artifact_list_idx] = ((10000 / krar) * (10000 / rar)) / 100;
		}
#endif

		artifact_list_idx++;

		/* outdated client? */
		if (artifact_list_idx == MAX_A_IDX) break;
	}

	my_fclose(fff);
}
void artifact_lore_aux(int aidx, int alidx, char paste_lines[18][MSG_LEN]) {
	char buf[1024], *p1, *p2;
	FILE *fff;
	int l = 0, pl = -1, cl = strlen(cname);
	int pl_len = 80 - 3 - cl - 2; /* 80 - 3 - namelen = chars available per chat line; 3 for brackets+space, 2 for colour code */
	int msg_len_eff = MSG_LEN - cl - 5 - 3 - 8;
	int chars_per_pline = (msg_len_eff / pl_len) * pl_len; /* chars usable in a paste_lines[] */
	char tmp[MSG_LEN];

	/* actually use local a_info.txt - a novum */
	path_build(buf, 1024, ANGBAND_DIR_GAME, "a_info.txt");
	fff = my_fopen(buf, "r");
	if (fff == NULL) {
		//plog("Error: Your a_info.txt file is missing.");
		return;
	}

	while (0 == my_fgets(fff, buf, 1024)) {
		/* strip $/%..$/! conditions */
		p1 = p2 = buf; /* dummy, != NULL */
		while (buf[0] == '$' || buf[0] == '%') {
			p1 = strchr(buf + 1, '$');
			p2 = strchr(buf + 1, '!');
			if (!p1 && !p2) break;
			if (!p1) p1 = p2;
			else if (p2 && p2 < p1) p1 = p2;
			//strcpy(buf, p1 + 1); // overlapping strings
			memmove(buf, p1 + 1, strlen(p1 + 1) + 1);
		}
		if (!p1 && !p2) continue;

		if (strlen(buf) < 3 || buf[0] != 'N') continue;

		p1 = buf + 2; /* artifact code */
		p2 = strchr(p1, ':'); /* 1 before artifact name */
		if (!p2) continue; /* paranoia (broken file) */

		if (atoi(p1) != aidx) continue;

		/* print info */

		/* name */
		//Term_putstr(5, 5, -1, TERM_YELLOW, p2 + 1);
		strcpy(paste_lines[++pl], format("\377U%s",
			artifact_list_name[alidx]));
		Term_putstr(5, 5, -1, TERM_L_UMBER, paste_lines[pl] + 2); /* no need for \377U */

		/* fetch diz */
		strcpy(paste_lines[++pl], "\377u");
		while (0 == my_fgets(fff, buf, 1024)) {
			/* strip $/%..$/! conditions */
			p1 = p2 = buf; /* dummy, != NULL */
			while (buf[0] == '$' || buf[0] == '%') {
				p1 = strchr(buf + 1, '$');
				p2 = strchr(buf + 1, '!');
				if (!p1 && !p2) break;
				if (!p1) p1 = p2;
				else if (p2 && p2 < p1) p1 = p2;
				//strcpy(buf, p1 + 1); // overlapping strings
				memmove(buf, p1 + 1, strlen(p1 + 1) + 1);
			}
			if (!p1 && !p2) continue;

			if (strlen(buf) < 3) continue;
			if (buf[0] == 'N') break;
			if (buf[0] != 'D') continue;

			p1 = buf + 2; /* artifact diz line */
			/* strip trailing spaces */
			while (p1[strlen(p1) - 1] == ' ') p1[strlen(p1) - 1] = '\0';
			Term_putstr(1, 7 + (l++), -1, TERM_UMBER, p1);

			/* add to chat-paste buffer. */
			while (*p1) {
				while (*p1 == ' ') p1++;
				if (*p1 == 0) break;

				/* diz line fits in pasteline? */
				if (strlen(paste_lines[pl]) + strlen(p1) <= chars_per_pline) {
					strcat(paste_lines[pl], p1);
					strcat(paste_lines[pl], " ");
					break;
				} else {
					/* diz line cannot be split? */
					if (!(p2 = strchr(p1, ' '))) {
						/* next pasteline */
						strcpy(paste_lines[++pl], "\377u");
						strcat(paste_lines[pl], p1);
						strcat(paste_lines[pl], " ");
						break;
					}
					/* diz line can be split? */
					else {
						strcpy(tmp, p1);
						tmp[p2 - p1] = '\0';
						/* split part is too big? */
						if (strlen(paste_lines[pl]) + strlen(tmp) > chars_per_pline) {
							/* next pasteline */
							strcpy(paste_lines[++pl], "\377u");
							strcat(paste_lines[pl], p1);
							strcat(paste_lines[pl], " ");
							break;
						}
						/* append split part */
						else {
							strcat(paste_lines[pl], tmp);
							strcat(paste_lines[pl], " ");
							p1 = p2 + 1;

							/* go on */
						}
					}
				}
			}
		}

		break;
	}

	my_fclose(fff);
}
const char *obj_flags2highlight[] = {"SPECIAL_GENE", ""};
const char *obj_flags2highlight2[] = {"WINNERS_ONLY", ""};
//collisions requiring extra string checks: SUST_xxx, REGEN_mana, HOLD_life
const char *obj_flags2highlight3[] = {"STR", "INT", "WIS", "DEX", "CON", "CHR", "STEALTH", "BLOWS", "CRIT", "MANA", "SEARCH", "INFRA", "TUNNEL", "SPEED", "LIFE", "LUCK", "DISARM", ""};//flags affected by (b)pval
const char *obj_flags2highlight4[] = {"NO_TELE", "DRAIN_MANA", "DRAIN_HP", "DRAIN_EXP", "AGGRAVATE", "NEVER_BLOW", "BLACK_BREATH", "CLONE", "PERMA_CURSE", "HEAVY_CURSE", "TY_CURSE", "DG_CURSE", "CURSE_NO_DROP", "AUTO_CURSE", "CURSED", ""};
const char *obj_flags2highlight5[] = {"ACTIVATE", ""};
const char *obj_flags2highlight6[] = {"IM_COLD", "IM_FIRE", "IM_ACID", "IM_ELEC", "IM_POISON", "IM_WATER", "IM_NETHER", ""};
static int obj_highlit_flags(char *line) {
	const char **f = obj_flags2highlight;
	char *p2;
	int i = 0;

	char prefixed[9] = { ' ', 0, 0, 0, 0, 0, 0, 0, 0 };

	while (*f[0]) {
		if ((p2 = strstr(line, *f)) &&
		    (p2 == line || *(p2 - 1) == ' ' || *(p2 - 2) == '\377'))//make sure the string is not just part of a different, longer flag string
			i += 4;
		f++;
	}

	f = obj_flags2highlight2;
	while (*f[0]) {
		if ((p2 = strstr(line, *f)) &&
		    (p2 == line || *(p2 - 1) == ' ' || *(p2 - 2) == '\377'))//make sure the string is not just part of a different, longer flag string
			i += 4;
		f++;
	}

	f = obj_flags2highlight3;
	while (*f[0]) {
		if ((p2 = strstr(line, *f)) &&
		    (p2 == line || *(p2 - 1) == ' ' || *(p2 - 2) == '\377'))//make sure the string is not just part of a different, longer flag string
			i += 4;
		else {
			strcpy(prefixed + 1, *f);
			if ((p2 = strstr(line, prefixed)))//make sure the string is not just part of a different, longer flag string
				i += 4;
		}
		f++;
	}

	f = obj_flags2highlight4;
	while (*f[0]) {
		if ((p2 = strstr(line, *f)) &&
		    (p2 == line || *(p2 - 1) == ' ' || *(p2 - 2) == '\377'))//make sure the string is not just part of a different, longer flag string
			i += 4;
		f++;
	}

	f = obj_flags2highlight5;
	while (*f[0]) {
		if ((p2 = strstr(line, *f)) &&
		    (p2 == line || *(p2 - 1) == ' ' || *(p2 - 2) == '\377'))//make sure the string is not just part of a different, longer flag string
			i += 4;
		f++;
	}

	f = obj_flags2highlight6;
	while (*f[0]) {
		if ((p2 = strstr(line, *f)) &&
		    (p2 == line || *(p2 - 1) == ' ' || *(p2 - 2) == '\377'))//make sure the string is not just part of a different, longer flag string
			i += 4;
		f++;
	}

	return i;
}
static void obj_highlight_flags(char *info, bool minus) {
	const char **f = obj_flags2highlight, a_flag = 's';
	char info_tmp[MSG_LEN], *p2;

	char prefixed[9] = { ' ', 0, 0, 0, 0, 0, 0, 0, 0 };

	while (*f[0]) {
		if ((p2 = strstr(info, *f)) && (p2 == info || *(p2 - 1) == ' ')) {
			strcpy(info_tmp, info);
			sprintf(info_tmp + (p2 - info), "\377y%s\377%c", *f, a_flag);
			strcat(info_tmp, p2 + strlen(*f));
			strcpy(info, info_tmp);
		}
		f++;
	}

	f = obj_flags2highlight2;
	while (*f[0]) {
		if ((p2 = strstr(info, *f)) && (p2 == info || *(p2 - 1) == ' ')) {
			strcpy(info_tmp, info);
			sprintf(info_tmp + (p2 - info), "\377v%s\377%c", *f, a_flag);
			strcat(info_tmp, p2 + strlen(*f));
			strcpy(info, info_tmp);
		}
		f++;
	}

	f = obj_flags2highlight3;
	/* flags affected by (b)pval: can increase or decrease our abilities -> different colour! */
	if (minus) while (*f[0]) {
		if ((p2 = strstr(info, *f)) && (p2 == info || *(p2 - 1) == ' ')) {
			strcpy(info_tmp, info);
			sprintf(info_tmp + (p2 - info), "\377R%s\377%c", *f, a_flag);
			strcat(info_tmp, p2 + strlen(*f));
			strcpy(info, info_tmp);
		} else {
			strcpy(prefixed + 1, *f);
			if ((p2 = strstr(info, prefixed))) {
				strcpy(info_tmp, info);
				sprintf(info_tmp + (p2 - info), "\377R%s\377%c", prefixed, a_flag);
				strcat(info_tmp, p2 + strlen(prefixed));
				strcpy(info, info_tmp);
			}
		}
		f++;
	}
	else while (*f[0]) {
		if ((p2 = strstr(info, *f)) && (p2 == info || *(p2 - 1) == ' ')) {
			strcpy(info_tmp, info);
			sprintf(info_tmp + (p2 - info), "\377G%s\377%c", *f, a_flag);
			strcat(info_tmp, p2 + strlen(*f));
			strcpy(info, info_tmp);
		} else {
			strcpy(prefixed + 1, *f);
			if ((p2 = strstr(info, prefixed))) {
				strcpy(info_tmp, info);
				sprintf(info_tmp + (p2 - info), "\377G%s\377%c", prefixed, a_flag);
				strcat(info_tmp, p2 + strlen(prefixed));
				strcpy(info, info_tmp);
			}
		}
		f++;
	}

	f = obj_flags2highlight4;
	while (*f[0]) {
		if ((p2 = strstr(info, *f)) && (p2 == info || *(p2 - 1) == ' ')) {
			strcpy(info_tmp, info);
			sprintf(info_tmp + (p2 - info), "\377D%s\377%c", *f, a_flag);
			strcat(info_tmp, p2 + strlen(*f));
			strcpy(info, info_tmp);
		}
		f++;
	}

	f = obj_flags2highlight5;
	while (*f[0]) {
		if ((p2 = strstr(info, *f)) && (p2 == info || *(p2 - 1) == ' ')) {
			strcpy(info_tmp, info);
			sprintf(info_tmp + (p2 - info), "\377B%s\377%c", *f, a_flag);
			strcat(info_tmp, p2 + strlen(*f));
			strcpy(info, info_tmp);
		}
		f++;
	}

	f = obj_flags2highlight6;
	while (*f[0]) {
		if ((p2 = strstr(info, *f)) && (p2 == info || *(p2 - 1) == ' ')) {
			strcpy(info_tmp, info);
			sprintf(info_tmp + (p2 - info), "\377w%s\377%c", *f, a_flag);
			strcat(info_tmp, p2 + strlen(*f));
			strcpy(info, info_tmp);
		}
		f++;
	}
}
/* assume/handle certain features: */
#define USE_NEW_SHIELDS
void artifact_stats_aux(int aidx, int alidx, char paste_lines[18][MSG_LEN]) {
	char buf[1024], *p1, *p2, info[MSG_LEN], info_tmp[MSG_LEN];
	cptr s_rarity = NULL;
	FILE *fff;
	int l = 0, info_val, pl = -1;
	int pf_col_cnt = 0; /* combine multiple [3] flag lines into one */
	/* for multiple definitions via $..$/!: */
	bool got_W_line = FALSE; /* usually only W: lines are affected, right? */
	bool got_F_lines = FALSE, got_P_line = FALSE;
	int f_col = 0; /* used for F flags */
	const char a_key = 'u', a_val = 's', a_flag = 's'; /* 'Speed:', 'Normal' */
	const char ta_key = TERM_UMBER, ta_flag = TERM_SLATE;//ta_val = TERM_SLATE, /* 'Speed:', 'Normal' */

	int tval, sval = 0, v_ac, v_acx, v_hit, v_dam, rarity, pval = 0;
	char v_ddice[10];
	bool empty;

	/* for TV_BOW multiplier */
	bool xtra_might = FALSE; /* paranoia: if someone really specifies an F-line before the I-line, if that's even possible */
	int mult = 2, mult_line_pl = -1, mult_line_l = 0;


	/* actually use local a_info.txt - a novum */
	path_build(buf, 1024, ANGBAND_DIR_GAME, "a_info.txt");
	fff = my_fopen(buf, "r");
	if (fff == NULL) {
		//plog("Error: Your a_info.txt file is missing.");
		return;
	}

	while (0 == my_fgets(fff, buf, 1024)) {
		/* strip $/%..$/! conditions */
		p1 = p2 = buf; /* dummy, != NULL */
		while (buf[0] == '$' || buf[0] == '%') {
			p1 = strchr(buf + 1, '$');
			p2 = strchr(buf + 1, '!');
			if (!p1 && !p2) break;
			if (!p1) p1 = p2;
			else if (p2 && p2 < p1) p1 = p2;
			//strcpy(buf, p1 + 1); // overlapping strings
			memmove(buf, p1 + 1, strlen(p1 + 1) + 1);
		}
		if (!p1 && !p2) continue;

		if (strlen(buf) < 3 || buf[0] != 'N') continue;

		p1 = buf + 2; /* artifact code */
		p2 = strchr(p1, ':'); /* 1 before artifact name */
		if (!p2) continue; /* paranoia (broken file) */

		if (atoi(p1) != aidx) continue;

		/* print info */

		/* name */
		//Term_putstr(5, 5, -1, TERM_L_UMBER, p2 + 1);
		strcpy(paste_lines[++pl], format("\377U%s",
			artifact_list_name[alidx]));
		Term_putstr(5, 5, -1, TERM_L_UMBER, paste_lines[pl] + 2); /* no need for \377U */

		/* fetch stats: I/W/E/O/B/F/S lines */
		tval = 0;
		while (0 == my_fgets(fff, buf, 1024)) {
			/* strip $/%..$/! conditions */
			p1 = p2 = buf; /* dummy, != NULL */
			while (buf[0] == '$' || buf[0] == '%') {
				p1 = strchr(buf + 1, '$');
				p2 = strchr(buf + 1, '!');
				if (!p1 && !p2) break;
				if (!p1) p1 = p2;
				else if (p2 && p2 < p1) p1 = p2;

				/* actually handle/assume certain features */
				if (buf[0] == '$' && *p1 == '!' && (
#ifdef USE_NEW_SHIELDS
				    !strncmp(buf + 1, "USE_NEW_SHIELDS", 15) ||
#endif
				    FALSE
				    )) {
					p1 = p2 = NULL;
					break;
				}
				if (buf[0] == '$' && *p1 == '$' && (
				    !strncmp(buf + 1, "RPG_SERVER", 10) || /* assume non-rpgserver stats */
				    !strncmp(buf + 1, "TEST_SERVER", 11) || /* ignore test-server */
				    !strncmp(buf + 1, "ARCADE_SERVER", 13) /* assume non-arcadeserver stats */
				    )) {
					p1 = p2 = NULL;
					break;
				}

				//strcpy(buf, p1 + 1); // overlapping strings
				memmove(buf, p1 + 1, strlen(p1 + 1) + 1);
			}
			if (!p1 && !p2) continue;

			/* line invalid (too short) */
			if (strlen(buf) < 3) continue;

			/* end of this artifact (aka beginning of next artifact) */
			if (buf[0] == 'N') break;

			p1 = buf + 2; /* line of artifact data, beginning of actual data */
			info[0] = '\0'; /* prepare empty info line */
			info_tmp[0] = '\0';

			switch(buf[0]) {
			case 'I': /* tval, sval, pval */
			    /* tval */
				p2 = strchr(p1, ':') + 1;
				/* remember for choosing which stats to actually display later */
				tval = atoi(p1);
				p1 = p2;
			    /* sval */
				p2 = strchr(p1, ':') + 1;
				/* remember for hard-coded bow multipliers */
				sval = atoi(p1);
				p1 = p2;

				/* insert rarity */
				/* note: Does not account for extra rarity increases due to..
				         1) OOD discrepancy between k-depth and a-depth)
				         2) treasure class of that artifact not being dropped by many monsters,
				            or those who drop it spawning more rarely.
				*/
				rarity = artifact_list_rarity[alidx]; /* from 1 to 10000 (k-rarity 100, a-rarity 100) */
				if (rarity > 100000) s_rarity = "Common";
				else if (rarity >= 10000) s_rarity = "Uncommon";
				else if (rarity > 1000) s_rarity = "Rare";
				//else if (rarity > 100) s_rarity = "Very rare";
				else if (rarity > 125) s_rarity = "Very rare"; //allow 100*80 or 90*90
				else s_rarity = "Extremely rare";

			    /* pval */
				info_val = atoi(p1);
				pval = info_val;//remember (for flag-colouring)
				if (info_val == 0) {
					/* no boni? move rarity info to the next line (W:) then. */
					break;
				}
				if (tval == TV_FOOD)
					sprintf(info_tmp, "%s.", s_rarity); /* hide saturation bonus */
				else
					sprintf(info_tmp, "%s. Magical bonus (to stats and/or abilities): \377%c(%s%d)\377%c.", s_rarity, a_val, info_val < 0 ? "" : "+", info_val, a_key);
				s_rarity = NULL;
				strcpy(info, info_tmp);
			    /* all done, display: */
				strcpy(paste_lines[++pl], format("\377%c", a_key));
				strcat(paste_lines[pl], info);
				Term_putstr(1, 7 + (l++), -1, ta_key, info);
				break;
			case 'W': /* depth, rarity, weight, price */
				/* only process 1st definition we find (for $..$/! stuff): */
				if (got_W_line) continue;
				got_W_line = TRUE;

			    /* depth */
				p2 = strchr(p1, ':') + 1;
				if (!artifact_list_specialgene[alidx] || aidx == 233) /* hardcoded ART_AMUGROM -_- */
					sprintf(info_tmp, "Found around depth: \377%c%d\377%c. ", a_val, atoi(p1), a_key);
				else
					sprintf(info_tmp, "Carried by a dungeon boss. ");
				/* rarity string left from an otherwise completely empty 'I:' line? */
				if (s_rarity) {
					sprintf(info, "%s. ", s_rarity);
					strcat(info, info_tmp);
				} else strcpy(info, info_tmp);
				p1 = p2;
			    /* rarity */
				p2 = strchr(p1, ':') + 1;
				/* hack, 255 counts as diabled */
				if (atoi(p1) == 255) {
					/* overwrite previous rarity/depth information */
					sprintf(info_tmp, "This artifact is unfindable. ");
					strcpy(info, info_tmp);
				}
				p1 = p2;
			    /* weight */
				p2 = strchr(p1, ':') + 1;
				sprintf(info_tmp, "Weight: \377%c%d.%d lb\377%c. ", a_val, atoi(p1) / 10, atoi(p1) % 10, a_key);
				strcat(info, info_tmp);
				p1 = p2;
			    /* price */
				sprintf(info_tmp, "Value: \377%c%d Au\377%c.", a_val, atoi(p1), a_key);
				strcat(info, info_tmp);
			    /* all done, display: */
				strcpy(paste_lines[++pl], format("\377%c", a_key));
				strcat(paste_lines[pl], info);
				Term_putstr(1, 7 + (l++), -1, ta_key, info);
				break;
			case 'P': /* ac, dam, +hit, +dam, +ac */
				/* only process 1st definition we find (for $..$/! stuff): */
				if (got_P_line) continue;
				got_P_line = TRUE;
			    /* ac */
				p2 = strchr(p1, ':') + 1;
				v_ac = atoi(p1);
				strcpy(info, info_tmp);
				p1 = p2;
			    /* damage dice */
				p2 = strchr(p1, ':') + 1;
				memset(v_ddice, 0, 10);
				strncpy(v_ddice, p1, p2 - p1 - 1);
				p1 = p2;
			    /* +hit */
				p2 = strchr(p1, ':') + 1;
				v_hit = atoi(p1);
				p1 = p2;
			    /* +dam */
				p2 = strchr(p1, ':') + 1;
				v_dam = atoi(p1);
				p1 = p2;
			    /* +ac */
				v_acx = atoi(p1);
			    /* all done, select */
				empty = TRUE;
				if (is_armour(tval)) {
					empty = FALSE;
#ifdef USE_NEW_SHIELDS
					if (tval == TV_SHIELD) {
						if (sflags1 & SFLG1_NEW_SHIELDS_NO_AC)
							sprintf(info_tmp, "Base block chance: \377%c%d%%\377%c", a_val, v_ac, a_key);
						else
							sprintf(info_tmp, "AC: \377%c[%d%%,%s%d]\377%c", a_val, v_ac, v_acx < 0 ? "" : "+", v_acx, a_key);
					} else
#endif
					sprintf(info_tmp, "AC: \377%c[%d,%s%d]\377%c", a_val, v_ac, v_acx < 0 ? "" : "+", v_acx, a_key);
					strcpy(info, info_tmp);
					if (v_dam) { /* in theory v_hit && v_dam, but this also covers (+0,+n)
					                (which currently doesn't exist though).
							ACTUALLY it should test for SHOW_MODS flag instead of hit/dam. ;) */
						strcpy(info_tmp, format(". To-hit/to-dam: \377%c(%s%d,%s%d)\377%c",
						    a_val, v_hit < 0 ? "" : "+", v_hit, v_dam < 0 ? "" : "+", v_dam, a_key));
						strcat(info, info_tmp);
					} else if (v_hit < 0) {
						strcpy(info_tmp, format(". Bulkiness affecting to-hit: \377%c(%d)\377%c",
						    a_val, v_hit, a_key));
						strcat(info, info_tmp);
					}
				} else if (is_melee_weapon(tval) || is_ammo(tval) || tval == TV_BOOMERANG) {
					empty = FALSE;
					sprintf(info_tmp, "Damage dice: \377%c(%s)\377%c. To-hit/to-dam: \377%c(%s%d,%s%d)\377%c",
					    a_val, v_ddice, a_key,
					    a_val, v_hit < 0 ? "" : "+", v_hit, v_dam < 0 ? "" : "+", v_dam, a_key);
					strcpy(info, info_tmp);
					if (v_acx) {
						strcpy(info_tmp, format(". AC bonus: \377%c[%s%d]\377%c",
						    a_val, v_acx < 0 ? "" : "+", v_acx, a_key));
						strcat(info, info_tmp);
					}
				} else if (tval == TV_BOW) {
					empty = FALSE;
					switch (sval) {
					/* 2 is default - SV_SLING, SV_SHORT_BOW */
					case SV_LONG_BOW: case SV_LIGHT_XBOW: mult = 3; break;
					case SV_HEAVY_XBOW: mult = 4; break;
					}
					if (xtra_might) mult++;
					sprintf(info_tmp, "Damage/range multiplier: \377%c(x%d)\377%c. To-hit/to-dam: \377%c(%s%d,%s%d)\377%c",
					    a_val, mult, a_key,
					    a_val, v_hit < 0 ? "" : "+", v_hit, v_dam < 0 ? "" : "+", v_dam, a_key);
					strcpy(info, info_tmp);
					if (v_acx) {
						strcpy(info_tmp, format(". AC bonus: \377%c[%s%d]\377%c",
						    a_val, v_acx < 0 ? "" : "+", v_acx, a_key));
						strcat(info, info_tmp);
					}
				} else {
					strcpy(info_tmp, "");
					strcpy(info, "");
					if (v_hit || v_dam) {
						strcpy(info_tmp, format("To-hit/to-dam: \377%c(%s%d,%s%d)\377%c",
						    a_val, v_hit < 0 ? "" : "+", v_hit, v_dam < 0 ? "" : "+", v_dam, a_key));
						strcat(info, info_tmp);
						empty = FALSE;
					}
					if (v_acx) {
						strcpy(info_tmp, format("%sAC bonus: \377%c[%s%d]\377%c",
						    empty ? "" : ", ",
						    a_val, v_acx < 0 ? "" : "+", v_acx, a_key));
						strcat(info, info_tmp);
						empty = FALSE;
					}
				}
			    /* display: */
				if (!empty) {
					strcat(info, ".");
					strcpy(paste_lines[++pl], format("\377%c", a_key));
					strcat(paste_lines[pl], info);
					Term_putstr(1, 7 + (l++), -1, ta_key, info);
					/* for correcting xtra_might */
					if (tval == TV_BOW) {
						mult_line_pl = pl;
						mult_line_l = l - 1;
					}
				}
				break;
			case 'F': /* flags */
				if (!got_F_lines) {
					Term_putstr(1, 7 + l, -1, ta_key, "Flags:");
					strcpy(paste_lines[++pl], format("\377%cFlags: \377%c", a_key, a_val));
					f_col = 8;
					got_F_lines = TRUE;
				}

				/* strip all spaces, convert | to space */
				p1--;
				while (*(++p1)) {
					switch (*p1) {
					case ' ': continue;
					case '|': *p1 = ' ';
					default: strcat(info, format("%c", *p1));
					}
				}
				/* add a pseudo '|' (ie a space) at the end
				   (for when two lines are merged -> the space would be missing) */
				if (info[strlen(info) - 1] != ' ') strcat(info, " ");

				/* for TV_BOW multipliers - correct it by +1 */
				if ((p2 = strstr(info, "XTRA_MIGHT"))) {
					xtra_might = TRUE;
					/* reprint corrected multiplier info */
					if (mult_line_pl != -1) {
						char *c_mult = strstr(paste_lines[mult_line_pl], "(x");
						if (c_mult) {
							(*(c_mult + 2))++;
							Term_putstr(1, 7 + mult_line_l, -1, ta_key, paste_lines[mult_line_pl] + 2);
						}
					}
				}

				/* Strip flags that are useless to know */
				if ((p2 = strstr(info, "SPECIAL_GENE"))) {
					strcpy(info_tmp, info);
					info_tmp[(p2 - info)] = '\0';
					strcat(info_tmp, p2 + 13);
					strcpy(info, info_tmp);
				}
				if ((p2 = strstr(info, "HIDE_TYPE"))) {
					strcpy(info_tmp, info);
					info_tmp[(p2 - info)] = '\0';
					strcat(info_tmp, p2 + 10);
					strcpy(info, info_tmp);
				}
				if ((p2 = strstr(info, "SHOW_MODS"))) {
					strcpy(info_tmp, info);
					info_tmp[(p2 - info)] = '\0';
					strcat(info_tmp, p2 + 10);
					strcpy(info, info_tmp);
				}
				if ((p2 = strstr(info, "INSTA_ART"))) {
					strcpy(info_tmp, info);
					info_tmp[(p2 - info)] = '\0';
					strcat(info_tmp, p2 + 10);
					strcpy(info, info_tmp);
				}
				if ((p2 = strstr(info, "LEVELS"))) {
					strcpy(info_tmp, info);
					info_tmp[(p2 - info)] = '\0';
					strcat(info_tmp, p2 + 7);
					strcpy(info, info_tmp);
				}

				/* Highlight certain important flags for quick readability */
				obj_highlight_flags(info, pval < 0);

#ifdef NEW_PASTELINE_METHOD
				/* add flags to existing paste_lines line */
				/* 8 = 6 for colour codes etc around cname + 2 for paranoia + 4 for Party/Guild chat mode tag */
				p1 = info;
				while (*p1 == ' ') p1++;
				if (*p1) {//paranoia
					if (strlen(paste_lines[pl]) + strlen(p1) < MSG_LEN - 1 - strlen(cname) - 8 - 4 - 7) {//7 = world server tax (pure paranoia here)
						strcat(paste_lines[pl], p1);
					} else { /* split it up */
						if (!(p2 = strchr(p1 + 1, ' '))) { /* can't split? */
							/* start next line */
							strcpy(paste_lines[++pl], format("\377%c%s", a_flag, p1));
						} else { /* split ok */
							char *p3;
							int p;

							p3 = p2 = p1;
							while ((p2 = strchr(p2 + 1, ' ')) && strlen(paste_lines[pl]) + strlen(p1) - strlen(p2) < MSG_LEN - 1 - strlen(cname) - 8 - 4 - 7) p3 = p2;
							//paste_lines[pl][strlen(paste_lines[pl]) + strlen(p1) - strlen(p3) - 1] = 0;
							p = strlen(paste_lines[pl]) + strlen(p1) - strlen(p3);
							strncat(paste_lines[pl], p1, strlen(p1) - strlen(p3));
							paste_lines[pl][p] = 0;
							while (*p3 == ' ') p3++;
							strcpy(paste_lines[++pl], format("\377%c%s", a_flag, p3));
						}
					}
				}
#endif

				/* add flags to existing line */
				p1 = info;
				while (*p1) {
					while (*p1 == ' ') p1++;
					if (*p1 == 0) break;

					/* add complete flag line */
					if (strlen(p1) + f_col < 80) {
#ifndef NEW_PASTELINE_METHOD
						strcat(paste_lines[pl], p1);
#endif
						Term_putstr(f_col, 7 + l, -1, ta_flag, p1);
						f_col += strlen(p1);
						f_col -= obj_highlit_flags(p1);

						/* done */
						break;
					}
					/* try to split up the line */
					else {
						/* can't split up F line? */
						if (!(p2 = strchr(p1, ' '))) {
							/* hack: catch special case: only the final space didn't fit into the line -> discard it instead */
							if (*p1 == 0) break;

							/* start next line */
							l++;
							if (++pf_col_cnt == 2) {
								pf_col_cnt = 0;
#ifndef NEW_PASTELINE_METHOD
								pl++;
								strcpy(paste_lines[pl], format("\377%c", a_flag));
#endif
							}
#ifndef NEW_PASTELINE_METHOD
							strcat(paste_lines[pl], p1);
#endif
							Term_putstr(2, 7 + l, -1, ta_flag, p1);
							f_col = 2 + strlen(p1);
							f_col -= obj_highlit_flags(p1);

							/* done */
							break;
						}
						/* add partial flag line */
						else {
							/* split part too large to fit into this line? */
							if (p2 - p1 + f_col >= 80) {
								/* start next line */
								l++;
								if (++pf_col_cnt == 2) {
									pf_col_cnt = 0;
#ifndef NEW_PASTELINE_METHOD
									pl++;
									strcpy(paste_lines[pl], format("\377%c", a_flag));
#endif
								}
#ifndef NEW_PASTELINE_METHOD
								strcat(paste_lines[pl], p1);
#endif
								Term_putstr(2, 7 + l, -1, ta_flag, p1);
								f_col = 2 + strlen(p1);
								f_col -= obj_highlit_flags(p1);

								/* done */
								break;
							}
							/* append split part */
							else {
								strcpy(info_tmp, p1);
								info_tmp[p2 - p1 + 1] = '\0';
#ifndef NEW_PASTELINE_METHOD
								strcat(paste_lines[pl], info_tmp);
#endif
								Term_putstr(f_col, 7 + l, -1, ta_flag, info_tmp);
								f_col += strlen(info_tmp);
								f_col -= obj_highlit_flags(info_tmp);
								p1 = p2 + 1;

								/* go on */
							}
						}
					}
				}
				break;
			}
		}

		break;
	}

	my_fclose(fff);
}

static void init_monster_mapping() {
	char buf[1024], *p1, *p2;
	int v1 = 0, v2 = 0, v3 = 0, i;
	FILE *fff;

	for (i = 0; i <= MAX_R_IDX; i++)
		monster_mapping_org[i] = 0;

	/* actually use local r_info.txt - a novum */
	path_build(buf, 1024, ANGBAND_DIR_GAME, "r_info.txt");
	fff = my_fopen(buf, "r");
	if (fff == NULL) {
		//plog("Error: Your r_info.txt file is missing.");
		return;
	}

	while (0 == my_fgets(fff, buf, 1024)) {
		/* strip $/%..$/! conditions */
		p1 = p2 = buf; /* dummy, != NULL */
		while (buf[0] == '$' || buf[0] == '%') {
			p1 = strchr(buf + 1, '$');
			p2 = strchr(buf + 1, '!');
			if (!p1 && !p2) break;
			if (!p1) p1 = p2;
			else if (p2 && p2 < p1) p1 = p2;

			/* actually handle/assume certain features */
			if (buf[0] == '$' && *p1 == '$' && (
			    !strncmp(buf + 1, "RPG_SERVER", 10) || /* assume non-rpgserver stats */
			    !strncmp(buf + 1, "TEST_SERVER", 11) || /* ignore test-server */
			    !strncmp(buf + 1, "ARCADE_SERVER", 13) /* assume non-arcadeserver stats */
			    )) {
				p1 = p2 = NULL;
				break;
			}

			//strcpy(buf, p1 + 1); // overlapping strings
			memmove(buf, p1 + 1, strlen(p1 + 1) + 1);
		}

		if (!p1 && !p2) continue;
		if (strlen(buf) < 3) continue;
		if (buf[0] == 'V' && buf[1] == ':') {
			sscanf(buf + 2, "%d.%d.%d", &v1, &v2, &v3);
			if (v1 < 3 || (v1 == 3 && (v2 < 4 || (v2 == 4 && v3 < 1)))) {
				//plog("Error: Your r_info.txt file is outdated.");
				return;
			}
			continue;
		}
		if (buf[0] != 'N') continue;

		p1 = buf + 2; /* monster index */
		p2 = strchr(p1, ':'); /* 1 before monster name */
		if (!p2) continue; /* paranoia (broken file) */

		i = atoi(p1);

		/* fetch symbol (and colour) */
		while (0 == my_fgets(fff, buf, 1024)) {
			/* strip $/%..$/! conditions */
			p1 = p2 = buf; /* dummy, != NULL */
			while (buf[0] == '$' || buf[0] == '%') {
				p1 = strchr(buf + 1, '$');
				p2 = strchr(buf + 1, '!');
				if (!p1 && !p2) break;
				if (!p1) p1 = p2;
				else if (p2 && p2 < p1) p1 = p2;

				/* actually handle/assume certain features */
				if (buf[0] == '$' && *p1 == '!' && (
				    (!strncmp(buf + 1, "HALLOWEEN", 9) && strstr(buf, "JOKEANGBAND")) /* allow viewing HALLOWEEN monsters (ie those that are marked as 'JOKEANGBAND when NOT Halloween') */
				    )) {
					p1 = p2 = NULL;
					break;
				}
				if (buf[0] == '$' && *p1 == '$' && (
				    !strncmp(buf + 1, "RPG_SERVER", 10) || /* assume non-rpgserver stats */
				    !strncmp(buf + 1, "TEST_SERVER", 11) || /* ignore test-server */
				    !strncmp(buf + 1, "ARCADE_SERVER", 13) /* assume non-arcadeserver stats */
				    )) {
					p1 = p2 = NULL;
					break;
				}

				//strcpy(buf, p1 + 1); // overlapping strings
				memmove(buf, p1 + 1, strlen(p1 + 1) + 1);
			}
			if (!p1 && !p2) continue;
			if (strlen(buf) < 5 || buf[0] != 'G') continue;

			p1 = buf + 2; /* monster symbol */
			p2 = strchr(p1, ':'); /* 1 before monster colour */
			break;
		}
		//buf[3] = '\0';
		//buf[5] = '\0';
		monster_mapping_org[i] = *p1;
		//monster_mapping_mod[(unsigned char)(*p1)] = *p1; //DEBUG

		/* outdated client? */
		if (i > MAX_R_IDX) break;
	}
	my_fclose(fff);
}
static void init_floor_mapping(void) {
	char buf[1024], *p1, *p2;
	int v1 = 0, v2 = 0, v3 = 0, i;
	FILE *fff;

	for (i = 0; i <= MAX_F_IDX; i++)
		floor_mapping_org[i] = 0;

	/* actually use local r_info.txt - a novum */
	path_build(buf, 1024, ANGBAND_DIR_GAME, "f_info.txt");
	fff = my_fopen(buf, "r");
	if (fff == NULL) {
		//plog("Error: Your r_info.txt file is missing.");
		return;
	}

	while (0 == my_fgets(fff, buf, 1024)) {
		/* strip $/%..$/! conditions */
		p1 = p2 = buf; /* dummy, != NULL */
		while (buf[0] == '$' || buf[0] == '%') {
			p1 = strchr(buf + 1, '$');
			p2 = strchr(buf + 1, '!');
			if (!p1 && !p2) break;
			if (!p1) p1 = p2;
			else if (p2 && p2 < p1) p1 = p2;

			/* actually handle/assume certain features */
			if (buf[0] == '$' && *p1 == '$' && (
			    !strncmp(buf + 1, "RPG_SERVER", 10) || /* assume non-rpgserver stats */
			    !strncmp(buf + 1, "TEST_SERVER", 11) || /* ignore test-server */
			    !strncmp(buf + 1, "ARCADE_SERVER", 13) /* assume non-arcadeserver stats */
			    )) {
				p1 = p2 = NULL;
				break;
			}

			//strcpy(buf, p1 + 1); // overlapping strings
			memmove(buf, p1 + 1, strlen(p1 + 1) + 1);
		}

		if (!p1 && !p2) continue;
		if (strlen(buf) < 3) continue;
		if (buf[0] == 'V' && buf[1] == ':') {
			sscanf(buf + 2, "%d.%d.%d", &v1, &v2, &v3);
			if (v1 < 3 || (v1 == 3 && (v2 < 4 || (v2 == 4 && v3 < 1)))) {
				//plog("Error: Your r_info.txt file is outdated.");
				return;
			}
			continue;
		}
		if (buf[0] != 'N') continue;

		p1 = buf + 2; /* floor index */
		p2 = strchr(p1, ':'); /* 1 before floor name */
		if (!p2) continue; /* paranoia (broken file) */

		i = atoi(p1);

		/* fetch symbol (and colour) */
		while (0 == my_fgets(fff, buf, 1024)) {
			/* strip $/%..$/! conditions */
			p1 = p2 = buf; /* dummy, != NULL */
			while (buf[0] == '$' || buf[0] == '%') {
				p1 = strchr(buf + 1, '$');
				p2 = strchr(buf + 1, '!');
				if (!p1 && !p2) break;
				if (!p1) p1 = p2;
				else if (p2 && p2 < p1) p1 = p2;

				/* actually handle/assume certain features */
				if (buf[0] == '$' && *p1 == '$' && (
				    !strncmp(buf + 1, "RPG_SERVER", 10) || /* assume non-rpgserver stats */
				    !strncmp(buf + 1, "TEST_SERVER", 11) || /* ignore test-server */
				    !strncmp(buf + 1, "ARCADE_SERVER", 13) /* assume non-arcadeserver stats */
				    )) {
					p1 = p2 = NULL;
					break;
				}

				//strcpy(buf, p1 + 1); // overlapping strings
				memmove(buf, p1 + 1, strlen(p1 + 1) + 1);
			}
			if (!p1 && !p2) continue;
			if (strlen(buf) < 5 || buf[0] != 'G') continue;

			p1 = buf + 2; /* monster symbol */
			p2 = strchr(p1, ':'); /* 1 before monster colour */
			break;
		}
		//buf[3] = '\0';
		//buf[5] = '\0';
		floor_mapping_org[i] = *p1;
		//floor_mapping_mod[(unsigned char)(*p1)] = *p1; //DEBUG

		/* outdated client? */
		if (i > MAX_F_IDX) break;
	}
	my_fclose(fff);
}

/* Initialize info for the in-client guide search */
static void init_guide(void) {
	int i;

	FILE *fff;
	char path[1024], buf[MAX_CHARS * 2 + 1], *c, *c2;
	byte contents = 0;


	path_build(path, 1024, "", "TomeNET-Guide.txt");
	fff = my_fopen(path, "r");
	if (!fff) return;

	/* count lines */
	while (fgets(buf, 81 , fff)) {
		guide_lastline++;

		/* and also remember chapter titles */
		buf[strlen(buf) - 1] = 0; /* remove trailing newline */
		switch (contents) {
		case 0: /* beginning of contents */
			if (!strcmp(buf, "Contents")) contents = 1;
			continue;
		case 1:
			switch (buf[0]) {
			case 0: /* end of contents */
				contents = 2;
				guide_endofcontents = guide_lastline;
				continue;
			case '(': /* main chapter */
				/* confirm chapter number */
				c = strchr(buf, ')');
				if (!c) continue; //paranoia
				*c = 0;

				/* confirm chapter title */
				c++;
				while (*c) if (*c == ' ') c++; else break;
				if (*c == 0) continue; //paranoia

				strcpy(guide_chapter_no[guide_chapters], buf + 1);
				strcpy(guide_chapter[guide_chapters], c);
				guide_chapters++;
				continue;
			case ' ': /* sub-chapter */
				/* confirm chapter existance (paranoia if convention is obeyed) */
				c2 = strchr(buf, '(');
				if (!c2) continue; //paranoia
				/* confirm chapter number */
				c = strchr(c2, ')');
				if (!c) continue; //paranoia
				*c = 0;

				/* confirm chapter title */
				c++;
				while (*c) if (*c == ' ') c++; else break;
				if (*c == 0) continue; //paranoia

				strcpy(guide_chapter_no[guide_chapters], c2 + 1);
				strcpy(guide_chapter[guide_chapters], c);
				guide_chapters++;
				continue;
			}
			continue;
		}
	}


	/* empty file? */
	if (guide_lastline == -1) return;
	my_fclose(fff);


	guide_races = exec_lua(0, "return guide_races");
	for (i = 0; i < guide_races; i++)
		strcpy(guide_race[i], string_exec_lua(0, format("return guide_race[%d]", i + 1)));

	guide_classes = exec_lua(0, "return guide_classes");
	for (i = 0; i < guide_classes; i++)
		strcpy(guide_class[i], string_exec_lua(0, format("return guide_class[%d]", i + 1)));

	guide_skills = exec_lua(0, "return guide_skills");
	for (i = 0; i < guide_skills; i++)
		strcpy(guide_skill[i], string_exec_lua(0, format("return guide_skill[%d]", i + 1)));

	guide_schools = exec_lua(0, "return guide_schools");
	for (i = 0; i < guide_schools; i++)
		strcpy(guide_school[i], string_exec_lua(0, format("return guide_school[%d]", i + 1)));

	guide_spells = exec_lua(0, "return guide_spells");
	for (i = 0; i < guide_spells; i++)
		strcpy(guide_spell[i], string_exec_lua(0, format("return guide_spell[%d]", i + 1)));
}



/*
 * Loop, looking for net input and responding to keypresses.
 */
static void Input_loop(void) {
	int	netfd, result;

	if (Net_flush() == -1) return;

	if ((netfd = Net_fd()) == -1) {
		plog("Bad socket filedescriptor");
		return;
	}

	/* In case we loaded .prf files that modified our screen dimensions,
	   we have to resend them. - C. Blue */
	Send_screen_dimensions();

	/* For term-resizing hacks */
	in_game = TRUE;
	prev_cname[0] = 0; //(re)init

	for (;;) {
		/* Send out a keepalive packet if need be */
		do_keepalive();
		do_mail();
		do_flicker();
		do_xfers();
		do_ping();

		if (Net_flush() == -1) {
#ifdef RETRY_LOGIN
			/* if player got ghost-killed, we won't have a connection anymore. But that's not an error, so skip.. */
			if (rl_connection_state == 1) {
				plog("Bad net flush");
				return;
			}
#else
			plog("Bad net flush");
			return;
#endif
		}

		/* Set the timeout on the network socket */
		/* This polling should probably be replaced with a select
		 * call that combines the net and keyboard input.
		 * This REALLY needs to be replaced since under my Linux
		 * system calling usleep seems to have a 10 ms overhead
		 * attached to it.
		 */
//		SetTimeout(0, 1000);
		SetTimeout(0, next_frame());

		/* Update the screen */
		Term_fresh();

		/* Only take input if we got some */
		if (SocketReadable(netfd)) {
			if ((result = Net_input()) == -1) {
				/*plog("Bad net input");*/
				return;
			}
		}

		flush_count = 0;

		/* See if we have a command waiting */
		request_command();

		/* Process any commands we got */
		while (command_cmd) {
#ifdef RETRY_LOGIN
			if (!rl_connection_destroyed)
#endif
			/* Process it */
			process_command();

			/* Clear previous command */
			command_cmd = 0;

			/* Ask for another command */
			request_command();
		}

		/*
		 * Update our internal timer, which is used to figure out when
		 * to send keepalive packets.
		 */
		update_ticks();

		/* Flush input (now!) */
		flush_now();

		/* Redraw windows if necessary */
		window_stuff();

#ifdef RETRY_LOGIN
		/* player used quit command? */
		if (rl_connection_state >= 2) return;
#endif
	}
}


/*
 * Display a message on screen.
 */
static void display_message(cptr msg, cptr title) {
	char buf[80];
	cptr tmp, newline;
	int i, len, prt_len, row = 0;

	/* Save the screen */
	Term_save();

	/* Clear the first line */
	Term_erase(0, row, 255);

	/* Print the "Warning" title */
	snprintf(buf, sizeof(buf), "======== %s ========", title);
	c_put_str(TERM_YELLOW, buf, row++, 0);

	/* Leave the second line empty */
	Term_erase(0, row++, 255);

	tmp = msg;

	/* Simple support for multiple lines separated by '\n' */
	while ((newline = strchr(tmp, '\n'))) {
		/* Length of the line in the input string */
		len = newline - tmp;

		/* Split lines that are over 79 characters long */
		for (i = 0; i < len; i += prt_len) {
			prt_len = MIN(len - i, sizeof(buf) - 1);
			strncpy(buf, tmp + i, prt_len);
			buf[prt_len] = '\0';

			/* Print the line */
			c_prt(TERM_WHITE, buf, row++, 0);
		}

		/* Next line in the input string */
		tmp = newline + 1;
	}

	len = strlen(tmp);

	/* Split long lines */
	for (i = 0; i < len; i += prt_len) {
		prt_len = MIN(len - i, sizeof(buf) - 1);
		strncpy(buf, tmp + i, prt_len);
		buf[prt_len] = '\0';

		/* Print the line */
		c_prt(TERM_WHITE, buf, row++, 0);
	}

	/* One empty line */
	Term_erase(0, row++, 255);

	/* Prevent hammering login or anything (when resuming connection for example) */
	Term_fresh();
#ifdef WINDOWS
	if (strstr(msg, " wait for ")) Sleep(800); //if counting down seconds, wait for <1s until we can retry (looks better when hammered)
	else Sleep(1500); //default: wait for 1.5s
#else
	if (strstr(msg, " wait for ")) usleep(800000); //if counting down seconds, wait for <1s until we can retry (looks better when hammered)
	else usleep(1500000); //default: wait for 1.5s
#endif

	/* Print the good old "Press any key to continue..." message */
	c_prt(TERM_L_BLUE, "Press any key to continue...", row++, 0);

	/* Wait for the key */
	(void) inkey();

	/* Reload the screen */
	Term_load();
}


/*
 * A hook for "quit()".
 *
 * Close down, then fall back into "quit()".
 */
static void quit_hook(cptr s) {
	int j, res = save_chat;

#if 0
#ifdef USE_SOUND_2010
	/* let the sound fade out, also helps the user to realize
	   he's been disconnected or something - C. Blue */
 #ifdef SOUND_SDL
	mixer_fadeall();
 #endif
#endif
#endif

	Net_cleanup();

	c_quit = 1;

	/* Display the quit reason */
	if (s && *s) display_message(s, "Quitting");

	if (message_num() && (res || (res = get_3way("Save chat log/all messages?", TRUE)))) {
		FILE *fp;
		char buf[80], buf2[1024];
		int i;
		time_t ct = time(NULL);
		struct tm* ctl = localtime(&ct);

		if (res == 1) strcpy(buf, "tomenet-chat_");
		else strcpy(buf, "tomenet-messages_");
		strcat(buf, format("%04d-%02d-%02d_%02d.%02d.%02d",
		    1900 + ctl->tm_year, ctl->tm_mon + 1, ctl->tm_mday,
		    ctl->tm_hour, ctl->tm_min, ctl->tm_sec));
		strcat(buf, ".txt");

		i = message_num();
		if (!save_chat) get_string("Filename:", buf, 79);
		/* maybe one day we'll get a Mac client */
		FILE_TYPE(FILE_TYPE_TEXT);
		path_build(buf2, 1024, ANGBAND_DIR_USER, buf);
		fp = my_fopen(buf2, "w");
		if (fp != (FILE*)NULL) {
			dump_messages_aux(fp, i, 2 - res, FALSE);//FALSE
			fclose(fp);
		}
	}

#if 1
#ifdef USE_SOUND_2010
	/* let the sound fade out, also helps the user to realize
	   he's been disconnected or something - C. Blue */
 #ifdef SOUND_SDL
	mixer_fadeall();
 #endif
#endif
#endif

#ifdef RETRY_LOGIN
	/* don't kill the windows and all */
	if (rl_connection_state >= 2) return;
#endif

#ifndef WINDOWS
	write_mangrc(FALSE);
#endif

#ifdef UNIX_SOCKETS
	SocketCloseAll();
#endif

#ifdef USE_GCU
	/* clean up USE_GCU's terminal colour modifications */
	if (!strcmp(ANGBAND_SYS, "gcu")) gcu_restore_colours();
#endif

	/* Nuke each term */
	for (j = ANGBAND_TERM_MAX - 1; j >= 0; j--) {
		/* Unused */
		if (!ang_term[j]) continue;

		/* Nuke it */
		term_nuke(ang_term[j]);
	}

	/* plog_hook must not be called anymore because the terminal is gone */
	plog_aux = NULL;
}


static void init_sound() {
#ifdef USE_SOUND_2010
	int i;

	/* One-time popup dialogue, to inform and instruct user of audio capabilities */
	if (sound_hint) plog("*******************************************\nTomeNET supports music and sound effects!\nTo enable those, you need to have audio packs installed,\nsee http://www.tomenet.eu/ forum and downloads.\n*******************************************\n");

	if (!use_sound) {
		/* Don't initialize sound modules */
		return;
	}

	/* Try the modules in the order specified by sound_modules[] */
	for (i = 0; i < N_ELEMENTS(sound_modules); i++) {
		if (sound_modules[i].init && 0 == sound_modules[i].init(0, NULL)) {
 #ifdef DEBUG_SOUND
			puts(format("USE_SOUND_2010: successfully loaded module %d.", i));
 #endif
			break;
		}
	}
 #ifdef DEBUG_SOUND
	puts("USE_SOUND_2010: done loading modules");
 #endif

	/* initialize mixer, putting configuration read from rc file live */
	set_mixing();

	/* remember indices of sounds that are hardcoded on client-side anyway, for efficiency */
	bell_sound_idx = exec_lua(0, "return get_sound_index(\"bell\")");
	page_sound_idx = exec_lua(0, "return get_sound_index(\"page\")");
	warning_sound_idx = exec_lua(0, "return get_sound_index(\"warning\")");
	rain1_sound_idx = exec_lua(0, "return get_sound_index(\"rain_soft\")");
	rain2_sound_idx = exec_lua(0, "return get_sound_index(\"rain_storm\")");
	snow1_sound_idx = exec_lua(0, "return get_sound_index(\"snow_soft\")");
	snow2_sound_idx = exec_lua(0, "return get_sound_index(\"snow_storm\")");
	browse_sound_idx = exec_lua(0, "return get_sound_index(\"browse\")");
	browsebook_sound_idx = exec_lua(0, "return get_sound_index(\"browse_book\")");
#endif
}


/*
 * A default hook function for "plog()" that displays a warning on the screen.
 */
static void plog_hook(cptr s) {
	/* Display a warning */
	if (s) display_message(s, "Warning");
}

#if 0
static void turn_off_numlock(void) {
#ifdef USE_X11
	turn_off_numlock_X11();
	return;
#else
 #ifdef USE_GCU
	//turn_off_numlock_GCU(); <- via console-ioctl?
 #endif
#endif
}
#endif


/*
 * Initialize everything, contact the server, and start the loop.
 */
void client_init(char *argv1, bool skip) {
	sockbuf_t ibuf;
	unsigned magic = 12345;
	unsigned char reply_to, status = 0;
	//int login_port;
	int bytes, retries;
	char host_name[80];
	u16b version = MY_VERSION;
	s32b temp;
	bool BIG_MAP_fallback = FALSE;
#ifdef ATMOSPHERIC_INTRO
 #ifdef USE_SOUND_2010
	bool stop_sfx = FALSE;
 #endif
#endif
#ifdef RETRY_LOGIN
	bool rl_auto_relogin = FALSE;
#endif

#if defined(USE_X11) || defined(USE_GCU)
	/* Force creation of fresh .tomenetrc file in case none existed yet.
	   The reason we do it *right now* is that it generates visual glitches later
	   and prevents plog() output from being displayed.
	   We only call a 'light' version (TRUE) in case .tomenetrc already exists. */
	write_mangrc(TRUE);
#endif

	/* Set the "plog hook" */
	if (!plog_aux) plog_aux = plog_hook;

	/* Setup the file paths */
	init_stuff();

	/* Initialize various arrays */
	init_arrays();

	/* Sound requires Lua */
	init_lua();

	/* Initialize sound */
	init_sound();

	/* Init monster list from local (client-side) r_info.txt file */
	init_monster_list();
	/* Init kind list from local (client-side) k_info.txt file */
	init_kind_list();
	/* Init artifact list from local (client-side) a_info.txt file */
	init_artifact_list();

	/* For unmapping custom fonts, which is required for xhtml-screenshots */
	init_floor_mapping(); //read floor characters from local (client-side) f_info.txt
	init_monster_mapping(); //read monster characters from local (client-side) r_info.txt

	/* For in-client guide search */
	init_guide();

	GetLocalHostName(host_name, 80);

	/* Set the "quit hook" */
	if (!quit_aux) quit_aux = quit_hook;

#ifndef UNIX_SOCKETS
	/* Check whether we should query the metaserver */
	if (argv1 == NULL) {
		server_port = cfg_game_port;
		/* Query metaserver */
		if (!get_server_name())
			quit("No server specified.");
 #ifdef EXPERIMENTAL_META
		cfg_game_port = server_port;
 #endif
	} else {
		/* Set the server's name */
		strcpy(server_name, argv1);
		if (strchr(server_name, ':') &&
		    /* Make sure it's not an IPv6 address. */
		    !strchr(strchr(server_name, ':') + 1, ':')) {

			char *port = strchr(server_name, ':');
			cfg_game_port = atoi(port + 1);
			*port = '\0';
		}
	}

	/* Fix "localhost" */
	if (!strcmp(server_name, "localhost"))
#endif
		strcpy(server_name, host_name);


#ifdef RETRY_LOGIN
	retry_contact:

	/* clear all windows of previous text */
	for (retries = 1; retries < ANGBAND_TERM_MAX; retries++) {
		term *old = Term;

		/* No window */
		if (!ang_term[retries]) continue;

		Term_activate(ang_term[retries]);
		Term_clear();
		Term_fresh();

		/* Restore */
		Term_activate(old);
	}

	/* also wipe all previously loaded macros, so they don't accidentally get merged */
	clear_macros();

	status = 0; //not required, paranoia?
	race = class = trait = sex = 0;
	rl_connection_state = 0;
	rl_connection_destroyed = FALSE;
	rl_connection_destructible = FALSE;
	if (rl_auto_relogin) skip = TRUE;
#endif

	/* Get character name and pass */
	if (!skip) get_char_name();

#ifdef RETRY_LOGIN
	/* don't memfrob an already memfrobbed password */
	if (!rl_auto_relogin)
#endif
	if (server_protocol >= 2) {
		/* Use memfrob on the password */
		my_memfrob(pass, strlen(pass));
	}

	/* Capitalize the name */
	nick[0] = toupper(nick[0]);

	/* Create the net socket and make the TCP connection */
	if ((Socket = CreateClientSocket(server_name, cfg_game_port)) == -1)
		quit("That server either isn't up, or you mistyped the hostname.\n");

	/* Create a socket buffer */
	if (Sockbuf_init(&ibuf, Socket, CLIENT_SEND_SIZE,
	    SOCKBUF_READ | SOCKBUF_WRITE) == -1) {
		quit("No memory for socket buffer\n");
	}

	/* Clear it */
	Sockbuf_clear(&ibuf);

	/* Extended version */
	if (server_protocol >= 2) version = 0xFFFFU;

	/* Put the contact info in it */
	Packet_printf(&ibuf, "%u", magic);
	Packet_printf(&ibuf, "%s%hu%c", real_name, GetPortNum(ibuf.sock), 0xFF);
	Packet_printf(&ibuf, "%s%s%hu", nick, host_name, version);

	/* Extended version */
	if (server_protocol >= 2)
		Packet_printf(&ibuf, "%d%d%d%d%d%d", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_EXTRA, VERSION_BRANCH, VERSION_BUILD + (VERSION_OS) * 1000000);

	/* Connect to server */
#ifdef UNIX_SOCKETS
	if ((DgramConnect(Socket, server_name, cfg_game_port)) == -1)
#endif

	/* Send the info */
	if ((bytes = DgramWrite(Socket, ibuf.buf, ibuf.len) == -1))
		quit("Couldn't send contact information\n");

	/* Listen for reply */
	for (retries = 0; retries < 10; retries++) {
		/* Set timeout */
		SetTimeout(1, 0);

		/* Wait for info */
		if (!SocketReadable(Socket)) continue;

		/* Read reply */
		if (DgramRead(Socket, ibuf.buf, ibuf.size) <= 0) {
			/*printf("DgramReceiveAny failed (errno = %d)\n", errno);*/
			continue;
		}

		/* Extra info from packet */
		if (Packet_scanf(&ibuf, "%c%c%d%d", &reply_to, &status, &temp, &char_creation_flags) <= 0) {
			Net_cleanup();
			quit("Invalid response from server!\n");
		}

		/* Hack -- set the login port correctly */
		//login_port = (int) temp;

		/* Hack - Receive server version - mikaelh */
		if (char_creation_flags & 0x02) {
			if (Packet_scanf(&ibuf, "%d%d%d%d%d%d", &server_version.major, &server_version.minor,
			    &server_version.patch, &server_version.extra, &server_version.branch, &server_version.build) < 0) {
				Net_cleanup();
				quit("Invalid response from server!\n");
			}

			/* Remove the flag */
			char_creation_flags ^= 0x02;

			/* Reset BIG_MAP screen if the server doesn't support it */
			if (!is_newer_than(&server_version, 4, 4, 9, 1, 0, 1))
			    //|| !(sflags1 & SFLG1_BIG_MAP)) -- sflags are not yet initialised!
				if (screen_wid != SCREEN_WID || screen_hgt != SCREEN_HGT) BIG_MAP_fallback = TRUE;
		} else {
			/* Assume that the server is old */
			server_version.major = VERSION_MAJOR;
			server_version.minor = VERSION_MINOR;
			server_version.patch = VERSION_PATCH;
			server_version.extra = 0;
			server_version.branch = 0;
			server_version.build = 0;

			/* Assume server doesn't support BIG_MAP screen */
			if (screen_wid != SCREEN_WID || screen_hgt != SCREEN_HGT) BIG_MAP_fallback = TRUE;
		}

		break;
	}

	if (BIG_MAP_fallback) {
		screen_wid = SCREEN_WID;
		screen_hgt = SCREEN_HGT;
		resize_main_window(CL_WINDOW_WID, CL_WINDOW_HGT);
	}

	/* Check for failure */
	if (retries >= 10) {
		Net_cleanup();
		quit("Server didn't respond!\n");
	}

	/* Server returned error code */
	if (status && status != E_NEED_INFO) {
		/* The server didn't like us.... */
		switch (status) {
			case E_VERSION_OLD:
				quit("Your client is outdated. Please get the latest one from http://www.tomenet.eu/");
			case E_VERSION_UNKNOWN:
				quit("Server responds 'Unknown client version'. Server might be outdated or client is invalid. Latest client is at http://www.tomenet.eu/");
			case E_GAME_FULL:
				quit("Sorry, the game is full.  Try again later.");
			case E_IN_USE:
				quit("That nickname is already in use.  If it is your nickname, wait 30 seconds and try again.");
			case E_INVAL:
				quit("The server didn't like your nickname, realname, or hostname.");
				//note: not a good case for RETRY_LOGIN, since a name or even the hostname might be asian/cyrillic or so.. not easily solvable maybe
			case E_TWO_PLAYERS:
				quit("There is already another character from this user/machine on the server.");
			case E_INVITE:
				quit("Sorry, the server is for members only.  Please retry with name 'guest'.");
			case E_BANNED:
				quit("You are temporarily banned from connecting to this server!");
			default:
				quit(format("Connection failed with status %d.", status));
		}
	}

#if 0
	/* Bam! */
	turn_off_numlock();
#endif

/*	printf("Server sent login port %d\n", login_port);
	printf("Server sent status %u\n", status);  */

	/* Close our current connection */
	/* Dont close the TCP connection DgramClose(Socket); */

	/* Connect to the server on the port it sent */
	if (Net_init(Socket) == -1)
		quit("Network initialization failed!\n");

	/* Verify that we are on the correct port */
	if (Net_verify(real_name, nick, pass) == -1) {
		Net_cleanup();
		quit("Network verify failed!\n");
	}

	/* Receive stuff like the MOTD */
	if (Net_setup() == -1) {
		Net_cleanup();
		quit("Network setup failed!\n");
	}

#ifdef RETRY_LOGIN
	rl_connection_destructible = TRUE; //(will be reset to FALSE in Net_login():Receive_login())
#endif
	status = Net_login();
#ifdef RETRY_LOGIN
	if (rl_connection_destroyed) {
		/* bad account crecedentials? */
		if (status == E_RETRY_CONTACT) {
			Net_cleanup();
			c_quit = FALSE; //un-quit, paranoia at this point though (only needed for Input_loop())
			goto retry_contact;
		}
		/* bad character name? */
		if (status == E_RETRY_LOGIN) {
			Net_cleanup();
			cname[0] = 0; //reset character choice, maybe it was an illegal name (login-fail-spam would be the result?)
			rl_auto_relogin = TRUE; //auto-logon up to character screen
			c_quit = FALSE; //un-quit, paranoia at this point though (only needed for Input_loop())
			goto retry_contact;
		}
		/* paranoia - shouldn't happen */
		rl_connection_destroyed = FALSE;
	}
#endif

#if 0 /* moved to get_char_info(), because of CHAR_COL */
	/* Hack -- display the nick */
	prt(format("Name        : %s", cname), 2, 1);
#endif

	/* Put actual screen size changes from loading option files on hold
	   until we've finished loading ALL option files,
	   instead of switching and glitching back and forth.

	   This glitch occurred seemingly at random,
	   and exactly when global.opt said 'Y' to big_map option,
	   while it would actually _not_ glitch out when it was set to 'X' in global.opt(!) */
	global_big_map_hold = TRUE;

	/* Initialize the pref files */
	initialize_main_pref_files();

	/* Handle asking for big_map mode on first time startup */
#if defined(USE_X11) || defined(WINDOWS)
	if (bigmap_hint && !c_cfg.big_map && strcmp(ANGBAND_SYS, "gcu") && ask_for_bigmap()) {
	        c_cfg.big_map = TRUE;
		Client_setup.options[CO_BIGMAP] = TRUE;
		check_immediate_options(CO_BIGMAP, TRUE, FALSE);
		//(void)options_dump("global.opt");
		(void)options_dump(format("%s.opt", cname));
	}
#endif
	/* If command-line client reads from same config file as X11 one it might
	   read a big-map-enabled screen_hgt, so reset it: */
	if (!strcmp(ANGBAND_SYS, "gcu")) screen_hgt = SCREEN_HGT;

	/* Initiate character creation? */
	if (status == E_NEED_INFO) {
		/* Get sex/race/class/etc */
		/* XXX this function sends PKT_KEEPALIVE */
#ifdef RETRY_LOGIN
		rl_connection_destructible = TRUE; //return to character overview if character creation process is aborted
#endif
		get_char_info();
#ifdef RETRY_LOGIN
		if (rl_connection_destroyed) {
			Net_cleanup();
			cname[0] = 0; //reset character choice, maybe it was an illegal name (login-fail-spam would be the result?)
			rl_auto_relogin = TRUE; //auto-logon up to character screen
			c_quit = FALSE; //un-quit, paranoia at this point though (only needed for Input_loop())
			goto retry_contact;
		}
		rl_connection_destructible = FALSE;
#endif
	}
#ifdef ATMOSPHERIC_INTRO
 #ifdef USE_SOUND_2010
	/* Stop background ambient fireplace sound effect (if enabled) */
	else if (use_sound) stop_sfx = TRUE;
 #endif
#endif

	/* Setup the key mappings */
	keymap_init();

	/* Show the MOTD */
	if (!skip_motd)
		show_motd(0); /* could be 2 for example to have ppl look at it.. */
#ifdef RETRY_LOGIN
	/* Show the MotD only once, ie on the initial login, not on every character switch */
	skip_motd = TRUE;
#endif

	/* Clear the screen again */
	Term_clear();

	/* Start the game */
	if (Net_start(sex, race, class) == -1) {
		Net_cleanup();
		quit("Network start failed!\n");
	}

	/* Hack -- flush the key buffer */
	Term_flush();

	/* Turn the lag-o-meter on after we've logged in */
	lagometer_enabled = TRUE;

#ifdef ATMOSPHERIC_INTRO
 #ifdef USE_SOUND_2010
	/* Stop background ambient fireplace sound effect (if enabled) */
	if (stop_sfx) sound_ambient(-1);
	/* Stop music that might be playing back from account screen still */
	//if (use_sound) music(-4);
 #endif
#endif

	/* Main loop */
#ifdef RETRY_LOGIN
	rl_connection_state = 1;
#endif
	Input_loop();

	/* Cleanup network stuff */
	Net_cleanup();

#ifdef RETRY_LOGIN
	if (rl_connection_state >= 2) {
		/* We quit the game actively? (Otherwise the quit_hook will already have taken care of fading out) */
		if (rl_connection_state == 3) {
 #ifdef USE_SOUND_2010
  #ifdef SOUND_SDL
			mixer_fadeall();
  #endif
 #endif
		}

		/* reset global game states (and activate auto-relogin) */
		rl_auto_relogin = TRUE; //auto-logon up to character screen
		in_game = FALSE; //BIG_MAP stuff? (paranoia?)
		c_quit = FALSE; //un-quit (or Net_fd() will always return -1)
		RL_revert_input(); //reset input parsing behaviour (macros etc) to normal

		/* reset stuff that needs resetting for anew character-choice or anew character-creation specifically: */
		cname[0] = 0; //reset character choice, or relog into character screen won't work
		auto_reincarnation = FALSE;
		player_pref_files_loaded = FALSE; //reload appropriate macros/auto-inscriptions on next character logon

		/* reset in-game related timed stuff */
		lagometer_enabled = FALSE; //don't do lag-o-meter during do_ping()
		weather_type = weather_elements = 0; //don't do do_weather() during do_ping()

		/* reset in-game states */
		shopping = perusing = FALSE; //paraonia?
		recording_macro = FALSE; //paranoia?

 #if 1		/* optional: reset message log? */
		hist_end = hist_chat_end = 0;
		hist_looped = hist_chat_looped = FALSE;
		message__next = message__last = message__head = message__tail = 0;
		message__next_chat =  message__last_chat = message__head_chat = message__tail_chat = 0;
		message__next_msgnochat = message__last_msgnochat = message__head_msgnochat = message__tail_msgnochat = 0;
		message__next_impscroll = message__last_impscroll = message__head_impscroll = message__tail_impscroll = 0;
 #endif

		/* reset inventory */
		for (bytes = 0; bytes < INVEN_TOTAL; bytes++) {
			WIPE(&inventory[bytes], object_type);
			inventory_name[bytes][0] = 0;
			inventory_inscription[bytes] = 0;
			inventory_inscription_len[bytes] = 0;
		}
		item_newest = -1;

		/* retuuurrrnnnn... */
		goto retry_contact;
	}
#endif

	/* Quit, closing term windows */
	quit(NULL);
}

bool ask_for_bigmap_generic(void) {
	int ch;

	Term_clear();
	Term_putstr(10, 3, -1, TERM_ORANGE, "Do you want to double the height of this window?");
	Term_putstr(10, 5, -1, TERM_YELLOW, "It is recommended to do this on desktops,");
	Term_putstr(10, 6, -1, TERM_YELLOW, "but it may not fit on small netbook screens.");
	Term_putstr(10, 7, -1, TERM_YELLOW, "You can change this later anytime in the game's options menu.");
	Term_putstr(10, 9, -1, TERM_ORANGE, "Press 'y' to enable BIG_MAP now, 'n' to not enable.");

	while (TRUE) {
		ch = inkey();
		if (ch == 'y') {
			Term_clear();
			return TRUE;
		} else if (ch == 'n') {
			Term_clear();
			return FALSE;
		}
	}
}
