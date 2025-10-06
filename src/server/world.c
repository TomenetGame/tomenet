/* experimental code - evileye */
/* this does not necessarily follow any sensible design */

/* added this for consistency in some (unrelated) header-inclusion,
   it IS a server file, isn't it? */
#define SERVER
#include "angband.h"

#ifdef TOMENET_WORLDS

#include <sys/types.h>
#include <sys/socket.h>

#include "../world/world.h"

/* Use character \373 (ASCII 251) to indicate that our message is a reply to an IRC-gameserver-command ('?<command')
   such as ?players, ?who, ?seen. This is used to tell the IRC-relay to not crop our server index number from our reply message.
   Note that this \373 usage has nothing to do with the \373 used in object1.c, files.c, nclient.c and c-files.c.
   The IRC-relay must be recent enough to handle the \373 code, otherwise you must comment out this definition: */
#define USE_IRC_COMMANDREPLY_INDICATOR

/* pfft. i'll use generic lists when i get around to it.
  server list: */
struct svlist {
	struct svlist *next;
	uint16_t sid;
	char name[30];
};

struct list *rpmlist = NULL;
struct list *svlist = NULL;

struct wpacket spk;

uint32_t chk(char *s1, char *s2);
void rem_server(int16_t id);
void add_rplayer(struct wpacket *wpk);
void add_server(struct sinfo *sinfo);
bool world_check_ignore(int Ind, uint32_t id, int16_t server);
void world_update_players(void);
int world_find_server(char *pname);
void world_msg(char *text);
struct list *addlist(struct list **head, int dsize);
struct list *remlist(struct list **head, struct list *dlp);
void world_reboot();

/* Generic list handling function */
struct list *addlist(struct list **head, int dsize) {
	struct list *newlp;

	newlp = malloc(sizeof(struct list));
	if (newlp) {
		newlp->data = malloc(dsize);
		if (newlp->data != NULL) {
			newlp->next = *head;
			*head = newlp;
			return(newlp);
		}
		free(newlp);
	}
	return(NULL);
}

/* Generic list handling function */
struct list *remlist(struct list **head, struct list *dlp) {
	struct list *lp;

	lp = *head;
	if (!lp || !dlp) return(NULL);
	if (dlp == *head) {
		*head = lp->next;
		free(dlp->data);
		free(dlp);
		return(*head);
	}
	while (lp) {
		if (lp->next == dlp) {
			lp->next = dlp->next;
			free(dlp->data);
			free(dlp);
			return(lp->next);
		}
		lp = lp->next;
	}
	return(dlp->next);
}

void world_update_players() {
	int i;

	for (i = 1; i <= NumPlayers; i++) {
		if (Players[i]->conn == NOT_CONNECTED) continue;
		if (Players[i]->admin_dm) continue;

		world_player(Players[i]->id, Players[i]->name, WORLD_INFO(Players[i]), 1, 0);
	}
}

bool world_check_ignore(int Ind, uint32_t id, int16_t server) {
	struct remote_ignore *curr;

	curr = Players[Ind]->w_ignore;
	while (curr) {
		if (curr->serverid == server && curr->id == id) return(TRUE);
		curr = curr->next;
	}
	return(FALSE);
}

/* Colour of private messages received across worlds.
   Keep consistent with util.c definition! */
#define WP_PMSG_DEFAULT_COLOUR 's'
/* Accept issuing IRC ?... commands from Discord too? */
#define DISCORD_IRC_COMMANDS
/* Suppress negative ?... command output to reduce clutter? */
#define NOCLUTTER_IRC_COMMANDS
void world_comm(int fd, int arg) {
	static char buffer[1024], msg[MSG_LEN], *msg_ptr, *wmsg_ptr, *wmsg_ptr2;
	char cbuf[sizeof(struct wpacket)];
	char *p;
	static short bpos = 0;
	static short blen = 0;
	int x, i;
	struct wpacket *wpk;

	//hack: Reinit static vars (used after disconnecting from world server)
	if (fd == -1) {
		bpos = blen = 0;
		return;
	}

	x = recv(fd, buffer + (bpos + blen), 1024 - (bpos + blen), 0);
	if (x == 0) {
		//struct rplist *c_pl, *n_pl;
		/* This happens... we are screwed (fortunately SIGPIPE isnt handled) */
		s_printf("pfft. world server closed\n");
		remove_input(WorldSocket);
		close(WorldSocket);	/* ;) this'll fix it... */
		/* Clear all the world players quietly */
		while (remlist(&rpmlist, rpmlist));
#if 0
		c_pl = rpmlist;
		while (c_pl) {
			n_pl = c_pl->next;
			free(c_pl);
			c_pl = n_pl;
		}
		rpmlist = NULL;
#endif
		WorldSocket = -1;
	}

	blen += x;
	while (blen >= sizeof(struct wpacket)) {
		wpk = (struct wpacket*)(buffer + bpos);
		switch (wpk->type) {
		case WP_SINFO:
			/* Server login information */
			add_server(&wpk->d.sinfo);
			break;
		case WP_CHAT:
			/* TEMPORARY chat broadcast method */
#if 0
			/* strip special chat codes \374/5/6 before testing prefix() */
			p = wpk->d.chat.ctxt;
			if (*p == '\374') p++;
			else if (*p == '\375') p++;
			if (*p == '\376') p++;
#endif

			/* World's 'server' flags decides about filtering our incoming messages */
			for (i = 1; i <= NumPlayers; i++) {
				if (Players[i]->conn == NOT_CONNECTED) continue;
				if (Players[i]->mutedchat == 3) continue;

				/* lame method just now */
				if (world_check_ignore(i, wpk->d.chat.id, wpk->serverid)) continue;
				msg_print(i, wpk->d.chat.ctxt);
			}

#if 1
			/* log */
			wmsg_ptr = wpk->d.chat.ctxt;
			/* strip \374,\375,\376 */
			while (*wmsg_ptr >= '\374' && *wmsg_ptr < '\377') wmsg_ptr++;
			strcpy(msg, wmsg_ptr);
			/* strip next colour code */
			wmsg_ptr = strchr(wmsg_ptr, '\377') + 2;
			msg_ptr = strchr(msg, '\377');
			strcpy(msg_ptr, wmsg_ptr);
			/* strip next colour code */
			wmsg_ptr = strchr(wmsg_ptr, '\377') + 2;
			msg_ptr = strchr(msg, '\377');
			strcpy(msg_ptr, wmsg_ptr);
			/* strip next colour code if at the beginning of the actual message line */
			if (*(wmsg_ptr + 1) == '\377') {
				strcpy(msg_ptr + 1, wmsg_ptr + 3);
				/* strip next colour code if existing  */
				if ((wmsg_ptr = strchr(wmsg_ptr + 3, '\377'))
				    /* not in /me though: ']' check */
				    && (wmsg_ptr2 = strchr(wpk->d.chat.ctxt + 9, ']')) && wmsg_ptr2 < wmsg_ptr) {
					msg_ptr = strchr(msg + 1, '\377');
					strcpy(msg_ptr, wmsg_ptr + 2);
				}
			}
			s_printf("%s\n", msg);
#endif
			break;
		case WP_PMSG:
			/* private message from afar -authed */
			for (i = 1; i <= NumPlayers; i++) {
				if (Players[i]->mutedchat == 3) continue;
				if (!strcmp(Players[i]->name, wpk->d.pmsg.victim)) {
					if (!world_check_ignore(i, wpk->d.pmsg.id, wpk->serverid)) {
						msg_format(i, "\375\377%c[%s:%s] %s", WP_PMSG_DEFAULT_COLOUR, wpk->d.pmsg.player, Players[i]->name, wpk->d.pmsg.ctxt);
						/* Remember sender for quick replying */
						strcpy(Players[i]->reply_name, wpk->d.pmsg.player);
					}
				}
			}
			break;
		case WP_MESSAGE:
			/* A raw message - no data */
			msg_broadcast_format(0, "%s", wpk->d.smsg.stxt);
#if 1
			/* log */
			wmsg_ptr = wpk->d.smsg.stxt;
			/* strip \374,\375,\376 */
			while (*wmsg_ptr >= '\374' && *wmsg_ptr < '\377') wmsg_ptr++;
			strcpy(msg, wmsg_ptr);
			/* strip next colour code */
			wmsg_ptr = strchr(wmsg_ptr, '\377') + 2;
			msg_ptr = strchr(msg, '\377');
			strcpy(msg_ptr, wmsg_ptr);
			/* strip next colour code */
			wmsg_ptr = strchr(wmsg_ptr, '\377') + 2;
			msg_ptr = strchr(msg, '\377');
			strcpy(msg_ptr, wmsg_ptr);
			/* strip next colour code if at the beginning of the actual message line */
			if (*(wmsg_ptr + 1) == '\377') strcpy(msg_ptr + 1, wmsg_ptr + 3);
			s_printf("%s\n", msg);
#endif
			break;
		case WP_NPLAYER:
		case WP_QPLAYER:
		case WP_UPLAYER:
			/* we need to handle a list */
			/* full death must count! */
			add_rplayer(wpk);
			break;
		case WP_AUTH:
			/* Authentication request */
			wpk->d.auth.val = chk(cfg.pass, wpk->d.auth.pass);
			x = sizeof(struct wpacket);
			x = send(WorldSocket, wpk, x, 0);
			world_update_players();
			break;
		case WP_SQUIT:
			/* Remove players */
			rem_server(wpk->d.sid);
			break;
		case WP_RESTART:
			set_runlevel(0);
			break;
		case WP_IRCCHAT:
#if 1
			/* Allow certain status commands from IRC to TomeNET server. */
			if (((p = strchr(wpk->d.chat.ctxt, ']')) && *(p += 2) == '?')
 #ifdef DISCORD_IRC_COMMANDS
			    /* Allow those commands also from Discord. (Format: "\377y(IRC) [TDiscord] [Username] ?...".) */
			    || (p && (p = strchr(p + 1, ']')) && *(p += 2) == '?')
 #endif
			    ) {
				if (!strncasecmp(p, "?help", 5)) {
 //#ifdef NOCLUTTER_IRC_COMMANDS
					if (wpk->d.sid == 1) /* Only the first authenticated server (ie with servers list index '1') gets to reply */
 //#endif
					msg_to_irc("Bot commands are: ?help, ?players, ?who, ?seen.");
					break;
				}
				/* list number + character names of players online */
				else if (!strncasecmp(p, "?players", 8)) {
					char buf[MSG_LEN + MAX_CHARS], bufp[MSG_LEN + MAX_CHARS]; //overspill, will get cut off at MSG_LEN and indicated by '..' chars

					x = 0;
					bufp[0] = 0;
					for (i = 1; i <= NumPlayers; i++) {
						if (Players[i]->conn == NOT_CONNECTED) continue;
						if (Players[i]->admin_dm && cfg.secret_dungeon_master) continue;

						x++;
						if (strlen(bufp) >= MSG_LEN) continue;

						if (x != 1) strcat(bufp, ", ");
						strcat(bufp, Players[i]->name);
						strcat(bufp, " (");
						strcat(bufp, Players[i]->accountname);
						strcat(bufp, ")");
					}
					if (!x) strcpy(buf, "\373No players online.");
					else {
						if (x == 1) strcpy(buf, "\3731 player: ");
						else strcpy(buf, format("\373%d players: ", x));
						strcat(buf, bufp);
						if (buf[MSG_LEN - 1]) {
							buf[MSG_LEN - 3] = '.';
							buf[MSG_LEN - 2] = '.';
							buf[MSG_LEN - 1] = 0;
						}
					}
 #ifdef NOCLUTTER_IRC_COMMANDS
					if (x)
 #endif
					msg_to_irc(buf);
					break;
				}
				else if (!strncasecmp(p, "?seen", 5)) {
					char buf[MSG_LEN];

					get_laston(p + 5 + 1, buf, FALSE, FALSE);
 #ifdef NOCLUTTER_IRC_COMMANDS
					if (!strstr(buf, "Sorry, couldn't find")) /* If noone was found, suppress message */
 #endif
					msg_to_irc(format("\373%s", buf));
					break;
				}
				else if (!strncasecmp(p, "?who", 4)) {
					u32b p_id;
					cptr acc;

					if (strlen(p) < 6) {
 #ifndef NOCLUTTER_IRC_COMMANDS
						msg_to_irc("You must specify a character name.");
 #endif
						break;
					}

					/* char names always start on upper-case */
					p[5] = toupper(p[5]);

					if (!(p_id = lookup_player_id(p + 5))) {
						struct account acc;
						bool done = FALSE;

#if 1 /* hack: also do a 'whowas' here by checking the reserved names list */
						for (i = 0; i < MAX_RESERVED_NAMES; i++) {
							if (!reserved_name_character[i][0]) break;

							if (!strcmp(reserved_name_character[i], p + 5)) {
								msg_to_irc(format("\373That deceased character belonged to account: %s", reserved_name_account[i]));
								done = TRUE;
								break;
							}
						}
						if (done) break;
#endif

#if 0 /* don't check for account name */
 #ifndef NOCLUTTER_IRC_COMMANDS
						msg_to_irc("\373That character name does not exist.");
 #endif
#else /* check for account name */
						if (!GetAccount(&acc, p + 5, NULL, FALSE, NULL, NULL))
 #ifndef NOCLUTTER_IRC_COMMANDS
							msg_to_irc("\373That character or account name does not exist.");
 #else
							;
 #endif
						else
							msg_to_irc("\373There is no such character, but there is an account of that name.");
#endif
						break;
					}

					acc = lookup_accountname(p_id);
					if (!acc) {
 //#ifndef NOCLUTTER_IRC_COMMANDS
						msg_to_irc("\373***ERROR: No account found."); //paranoia except on new server without any players
 //#endif
						break;
					}
					if (lookup_player_admin(p_id))
						msg_to_irc(format("\373That administrative character belongs to: %s", acc));
					else {
						u16b ptype = lookup_player_type(p_id);
						int lev = lookup_player_level(p_id);
						player_type Dummy;

						Dummy.prace = ptype & 0xff;
						Dummy.pclass = (ptype & 0xff00) >> 8;
						Dummy.ptrait = TRAIT_NONE;

						msg_to_irc(format("\373That level %d %s%s belongs to: %s",
						    lev,
						    //race_info[ptype & 0xff].title,
						    //special_prace_lookup[ptype & 0xff],
						    get_prace2(&Dummy),
						    class_info[ptype >> 8].title,
						    acc));
					}
					break;
				}
			}
#endif

#if 0 /* 0ed for consistency: Let world's 'server' flags decide about filtering our incoming messages */
			if (!cfg.worldd_ircchat) break;
#endif

			for (i = 1; i <= NumPlayers; i++) {
				if (Players[i]->conn == NOT_CONNECTED) continue;
				if (Players[i]->ignoring_chat) continue;
				if (Players[i]->limit_chat) continue;
				if (Players[i]->mutedchat == 3) continue;
				msg_print(i, wpk->d.chat.ctxt);
			}

#if 1
			/* log */
			strcpy(msg, "[IRC]");
			wmsg_ptr = wpk->d.chat.ctxt;
			/* strip \374,\375,\376 */
			while (*wmsg_ptr >= '\374' && *wmsg_ptr < '\377') wmsg_ptr++;
			wmsg_ptr += 10;
			strcat(msg, wmsg_ptr);
			/* strip next colour code */
			wmsg_ptr = strchr(wmsg_ptr, '\377') + 2;
			msg_ptr = strchr(msg, '\377');
			strcpy(msg_ptr, wmsg_ptr);
			/* done */
			s_printf("%s\n", msg);
#endif
			break;

		default:
			s_printf("unknown packet from world: %d\n", wpk->type);
		}

		/* update buffer position and remaining data size */
		bpos += sizeof(struct wpacket);
		blen -= sizeof(struct wpacket);
	}
	if (blen) {
		/* copy it back */
		memcpy(cbuf, buffer + bpos, blen);
		memcpy(buffer, cbuf, blen);
	}
	bpos = 0;
}

/* returns authed server id or 0 */
int world_find_server(char *pname) {
	struct list *lp;
	struct rplist *c_pl;

	lp = rpmlist;
	while (lp) {
		c_pl = (struct rplist*)lp->data;
		if (!strcmp(c_pl->name, pname)) return(c_pl->server);
		lp = lp->next;
	}
	return(0);
}

/* returns list entry, or NULL */
struct rplist *world_find_player(char *pname, int16_t server) {
	struct list *lp;
	struct rplist *c_pl;

	lp = rpmlist;
	while (lp) {
		c_pl = (struct rplist*)lp->data;
		if (!stricmp(c_pl->name, pname) && (!server || server == c_pl->server)) return(c_pl);
		lp = lp->next;
	}
	return(NULL);
}

/* proper data will come with merge.
   Returns number of remote players. */
int world_remote_players(FILE *fff) {
	int num = 0;
	struct list *lp, *slp;
	struct rplist *c_pl;
	struct svlist *c_sv;
	char servername[30];

	lp = rpmlist;
	if (lp) fprintf(fff, "\n\377y Remote players on different servers:\n\n");
	while (lp) {
		c_pl = (struct rplist*)lp->data;
		slp = svlist;
		sprintf(servername, "%d", c_pl->server);
		while (slp) {
			c_sv = (struct svlist*)slp->data;
			if (c_sv->sid == c_pl->server) {
				strncpy(servername, c_sv->name, 30);
				break;
			}
			slp = slp->next;
		}

		//fprintf(fff, "\377%c  %s\377s on '%s'\n", c_pl->server ? 'w' : 'W', c_pl->name, servername);
		fprintf(fff, "\377s %s\377%c %s (Lv.%s)\n", servername, c_pl->server ? 'w' : 'W', c_pl->name, c_pl->info);
		num++;
		lp = lp->next;
	}
	return(num);
}

/* When a server logs in, we get information about it */
void add_server(struct sinfo *sinfo) {
	struct list *lp;
	struct svlist *c_sr;

	//c_sr = malloc(sizeof(struct svlist));
	lp = addlist(&svlist, sizeof(struct svlist));
	if (lp) {
		c_sr = (struct svlist*)lp->data;
		strncpy(c_sr->name, sinfo->name, 30);
		c_sr->sid = sinfo->sid;
	}
}

/* This is called when a remote server disconnects */
void rem_server(int16_t id) {
	struct rplist *c_pl;
	struct svlist *c_sr;
	struct list *lp;

	/* remove all the old players */
	lp = rpmlist;
	while (lp) {
		c_pl = (struct rplist*)lp->data;
		if (c_pl->server == id) lp = remlist(&rpmlist, lp);
		else lp = lp->next;
	}

	/* Delete the server info */
	lp = svlist;
	while (lp) {
		c_sr = (struct svlist*)lp->data;
		if (c_sr->sid == id) lp = remlist(&svlist, lp);
		else lp = lp->next;
	}
}


/* This sorting is pointless on the worldd server side, because each game server still receives remote players solo and thereby unsortedly, and has to do sorting by itself.
   It makes sense though when this code is used by an actual game server, so players received are then locally sorted per server type, which looks better in the @ list. - C. Blue */
#define SORT_BY_SERVER
/* This function adds players to a server's list (WP_NPLAYER), removes them again (WP_QPLAYER) or updates their 'info' (WP_UPLAYER). */
void add_rplayer(struct wpacket *wpk) {
	struct list *lp;
	struct rplist *n_pl, *c_pl;
	unsigned char found = 0;
#ifdef SORT_BY_SERVER
	struct rplist *c_pl_scan;
	struct list *lp_prev, *lp_scan, *lp_scan_prev;
	int server_prev = -1;
	bool done;
#endif

	if (!wpk->d.play.silent)
		msg_broadcast_format(0, "\374\377s%s (%d) has %s the game on another server.", wpk->d.play.name, atoi(wpk->d.play.info), (wpk->type == WP_NPLAYER ? "entered" : "left"));

	if (wpk->type == WP_NPLAYER && !wpk->d.play.server) return;

	lp = rpmlist;
	while (lp) {
		c_pl = (struct rplist*)lp->data;
		//if (/* c_pl->id == wpk->d.play.id && */ !(strcmp(c_pl->name, wpk->d.play.name))) {
		if (c_pl->server == wpk->d.play.server && !(strcmp(c_pl->name, wpk->d.play.name))) {
			found = 1;
			break;
		}
		lp = lp->next;
	}

	if (wpk->type == WP_NPLAYER && !found) {
		lp = addlist(&rpmlist, sizeof(struct rplist));
		if (lp) {
			n_pl = (struct rplist*)lp->data;
			n_pl->id = wpk->d.play.id;
			n_pl->server = wpk->d.play.server;
			strncpy(n_pl->name, wpk->d.play.name, 30);
			strncpy(n_pl->info, wpk->d.play.info, 40);
		}
	}
	else if (wpk->type == WP_QPLAYER && found)
		remlist(&rpmlist, lp);
	else if (wpk->type == WP_UPLAYER && found) {
		n_pl = (struct rplist*)lp->data;
		strncpy(n_pl->info, wpk->d.play.info, 40);
		/* Info changes don't affect sorting, which is only done by server name and player name, so we're done */
		return;
	}

#ifdef SORT_BY_SERVER
	/* Sort list of players by server */
	lp = rpmlist;
	while (lp) {
		c_pl = (struct rplist*)lp->data;

		/* Init at first entry read */
		if (server_prev == -1) {
			server_prev = c_pl->server;
			lp_prev = lp;
			lp = lp->next;
			continue;
		}

		/* Critical part - check if server type changed now that we went from previous entry to this one */
		done = FALSE;
		if (c_pl->server != server_prev) {
			/* Server changed.
			   Now all subsequent entries are checked and moved down here if they still refer to the previous server instead of the current, new one. */
			lp_scan_prev = lp_scan = lp;
			while ((lp_scan = lp_scan->next)) {
				c_pl_scan = (struct rplist*)lp_scan->data;
				/* Found one, move it down here by swapping */
				if (c_pl_scan->server == server_prev) {
					lp_scan_prev->next = lp_scan->next;
					lp_prev->next = lp_scan;
					lp_scan->next = lp;

					/* After moving around, advance to next entry normally */
					lp = lp_prev->next;
					done = TRUE;
					break;
				}
				lp_scan_prev = lp_scan;
			}
			/* We did a swap? Continue looking for further hits of the old server type. */
			if (done) continue;

			/* No further servers to sort in, so server type no longer occurs down the list.
			   So now remember next server type that just appeared. */
			server_prev = c_pl->server;
		}

		/* Move on to next server entry */
		lp_prev = lp;
		lp = lp->next;
	}
#endif
}

void world_pmsg_send(uint32_t id, char *name, char *pname, char *text) {
	int len;
	if (WorldSocket == -1) return;

	spk.type = WP_PMSG;
	len = sizeof(struct wpacket);
	snprintf(spk.d.pmsg.ctxt, MSG_LEN, "%s", text);
	spk.d.pmsg.id = id;
	spk.d.pmsg.sid = world_find_server(pname);
	snprintf(spk.d.pmsg.player, 80, "%s", name);
	snprintf(spk.d.pmsg.victim, 80, "%s", pname);
	send(WorldSocket, &spk, len, 0);
}

void world_chat(uint32_t id, const char *text) {
	int len;

#ifdef ARCADE_SERVER
	/* Hack: Don't broadcast game commands, that's intense spam.. */
	char *t = strchr(text, ']');

	if (t && (t = strchr(t, ' '))) {;
		t++;
		if (streq(t, "quit") || streq(t, "begin") || streq(t, "walls") || streq(t, "gates") ||
		    streq(t, "midgate") || streq(t, "rings") || streq(t, "moveups") || streq(t, "float") ||
		    streq(t, "centerups") || streq(t, "dark") || streq(t, "snakes") || streq(t, "options") ||
		    streq(t, "maxwins") || streq(t, "race") || streq(t, "ai") || streq(t, "reset tron") ||

		    streq(t, "reset") || streq(t, "city") || streq(t, "powerups") || streq(t, "prac") ||
		    streq(t, "spectating") || streq(t, "max stuff") ||

		    streq(t, "walls on") || streq(t, "walls off") || streq(t, "gates on") || streq(t, "gates off") ||
		    streq(t, "city on") || streq(t, "city off") || streq(t, "rings on") || streq(t, "rings off") ||
		    streq(t, "dark on") || streq(t, "dark off") || streq(t, "snakes on") || streq(t, "snakes off") ||
		    streq(t, "maxwins on") || streq(t, "maxwins off") || streq(t, "powerups on") || streq(t, "powerups off") ||
		    streq(t, "moveups on") || streq(t, "moveups off") || streq(t, "float on") || streq(t, "float off") ||
		    streq(t, "prac on") || streq(t, "prac off") || streq(t, "centerups on") || streq(t, "centerups off") ||
		    streq(t, "more walls") || streq(t, "less walls") || streq(t, "more speed") || streq(t, "less speed") ||
		    streq(t, "ai on") || streq(t, "ai off") || streq(t, "midgate on") || streq(t, "midgate off") ||
		    streq(t, "spectating on") || streq(t, "spectating off") || streq(t, "more rings") || streq(t, "less rings") ||
		    streq(t, "race on") || streq(t, "race off") || streq(t, "less gate speed") || streq(t, "more gate speed") ||
		    streq(t, "more length") || streq(t, "less length") ||
		    //streq(t, "") || streq(t, "") || streq(t, "") || streq(t, "") ||
		    FALSE)
			return;
	}
#endif

	if (WorldSocket == -1) return;

	spk.type = WP_CHAT;
	len = sizeof(struct wpacket);
	snprintf(spk.d.chat.ctxt, MSG_LEN, "%s", text);
	spk.d.chat.id = id;
	send(WorldSocket, &spk, len, 0);
}

void world_reboot() {
	int len;

	if (WorldSocket == -1) return;
	spk.type = WP_RESTART;
	len = sizeof(struct wpacket);
	send(WorldSocket, &spk, len, 0);
}

void world_msg(char *text) {
	int len;

	if (WorldSocket == -1) return;
	spk.type = WP_MESSAGE;
	len = sizeof(struct wpacket);
	snprintf(spk.d.smsg.stxt, MSG_LEN, "%s", text);
	send(WorldSocket, &spk, len, 0);
}

void msg_to_irc(char *text) {
	int len;

#ifndef USE_IRC_COMMANDREPLY_INDICATOR
	/* Trim the indicator char, as we don't want to use it. */
	if (text[0] == '\373') text++;
#endif

	if (WorldSocket == -1) return;
	spk.type = WP_MSG_TO_IRC;
	len = sizeof(struct wpacket);
	snprintf(spk.d.smsg.stxt, MSG_LEN, "%s", text);
	send(WorldSocket, &spk, len, 0);
}

/* we can rely on ID alone when we merge data */
void world_player(uint32_t id, const char *name, const char *info, int mode, byte quiet) {
	int len;

	if (WorldSocket == -1) return;
	switch (mode) {
	case 0: spk.type = WP_QPLAYER; break;
	case 1: spk.type = WP_NPLAYER; break;
	case 2: spk.type = WP_UPLAYER; break;
	}
	len = sizeof(struct wpacket);
	strncpy(spk.d.play.name, name, 30);
	spk.d.play.name[29] = 0;
	strncpy(spk.d.play.info, info, 40);
	spk.d.play.info[39] = 0;
	spk.d.play.id = id;
	spk.d.play.silent = quiet;
	send(WorldSocket, &spk, len, 0);
}

/* unified, hopefully unique password check function */
uint32_t chk(char *s1, char *s2) {
	unsigned int i, j = 0;
	int m1, m2;
	static uint32_t rval[2] = {0, 0};

	rval[0] = 0L;
	rval[1] = 0L;
	m1 = strlen(s1);
	m2 = strlen(s2);
	for (i = 0; i < m1; i++) {
		rval[0] += s1[i];
		rval[0] <<= 5;
	}
	for (j = 0; j < m2; j++) {
		rval[1] += s2[j];
		rval[1] <<= 3;
	}
	for (i = 0; i < m1; i++) {
		rval[1] += s1[i];
		rval[1] <<= (3 + rval[0] % 5);
		rval[0] += s2[j];
		j = rval[0] % m2;
		rval[0] <<= (3 + rval[1] % 3);
	}
	return(rval[0] + rval[1]);
}
#endif
