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

/* pfft. i'll use generic lists when i get around to it */
struct svlist{
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
struct list *remlist(struct list **head, struct list *dlp){
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

void world_update_players(){
	int i;
	for (i = 1; i <= NumPlayers; i++) {
		if(Players[i]->conn != NOT_CONNECTED){
			if (!Players[i]->admin_dm)
				world_player(Players[i]->id, Players[i]->name, 1, 0);
		}
	}
}

bool world_check_ignore(int Ind, uint32_t id, int16_t server) {
	struct remote_ignore *curr;
	curr = Players[Ind]->w_ignore;
	while (curr) {
		if (curr->serverid == server && curr->id == id)
			return(TRUE);
		curr = curr->next;
	}
	return(FALSE);
}

void world_comm(int fd, int arg) {
	static char buffer[1024], msg[MSG_LEN], *msg_ptr, *wmsg_ptr, *wmsg_ptr2;
	char cbuf[sizeof(struct wpacket)], *p;
	static short bpos = 0;
	static short blen = 0;
	int x, i;
	struct wpacket *wpk;
	x = recv(fd, buffer + (bpos + blen), 1024 - (bpos + blen), 0);
	if (x == 0) {
		//struct rplist *c_pl, *n_pl;
		/* This happens... we are screwed (fortunately SIGPIPE isnt handled) */
		s_printf("pfft. world server closed\n");
		remove_input(WorldSocket);
		close(WorldSocket);	/* ;) this'll fix it... */
		/* Clear all the world players quietly */
		while(remlist(&rpmlist, rpmlist));
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
				/* strip special chat codes \374/5/6 before testing prefix() */
				p = wpk->d.chat.ctxt;
				if (*p == '\374') p++;
				else if (*p == '\375') p++;
				if (*p == '\376') p++;

#if 0 /* see #else below */
				if (!(cfg.worldd_pubchat || (cfg.worldd_broadcast && prefix(p, "\377r[\377"))))
					break; /* Filter incoming public chat and broadcasts here now - mikaelh */
#else /* Consistency: Let world's 'server' flags decide about filtering our incoming messages */
#endif

				for (i = 1; i <= NumPlayers; i++) {
					if (Players[i]->conn != NOT_CONNECTED) {
						/* lame method just now */
						if (world_check_ignore(i, wpk->d.chat.id, wpk->serverid))
							continue;
						msg_print(i, wpk->d.chat.ctxt);
					}
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
					if (!strcmp(Players[i]->name, wpk->d.pmsg.victim)) {
						if (!world_check_ignore(i, wpk->d.pmsg.id, wpk->serverid)) {
							msg_format(i, "\375\377s[%s:%s] %s", wpk->d.pmsg.player, Players[i]->name, wpk->d.pmsg.ctxt);
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
				if (*(wmsg_ptr + 1) == '\377')
					strcpy(msg_ptr + 1, wmsg_ptr + 3);
				s_printf("%s\n", msg);
#endif
				break;
			case WP_NPLAYER:
			case WP_QPLAYER:
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
				/* Allow certain status commands from IRC to TomeNET server */
				if ((p = strchr(wpk->d.chat.ctxt, ']')) && *(p += 2) == '?') {
					if (!strncmp(p, "?help", 5)) {
						msg_to_irc("Bot commands are: ?players, ?who, ?seen.");
						break;
					}
					/* list number + character names of players online */
					else if (!strncmp(p, "?players", 8)) {
						char buf[MSG_LEN];
						strcpy(buf, " 0 Players: ");
						x = 0;
						for (i = 1; i <= NumPlayers; i++) {
							if (Players[i]->conn == NOT_CONNECTED) continue;
							if (Players[i]->admin_dm && cfg.secret_dungeon_master) continue;

							x++;
							if (strlen(buf) + strlen(Players[i]->name) + 2 >= MSG_LEN - 20) continue; /* paranoia reserved */
							if (x != 1) strcat(buf, ", ");
							strcat(buf, Players[i]->name);
							strcat(buf, " (");
							strcat(buf, Players[i]->accountname);
							strcat(buf, ")");
						}
						if (!x) strcpy(buf, "No players online");
						else {
							if (x >= 10) buf[0] = '0' + x / 10;
							buf[1] = '0' + x % 10;
						}
						if (x == 1) buf[9] = ' '; /* Player_s_ */
						msg_to_irc(buf);
						break;
					}
					else if (!strncmp(p, "?seen", 5)) {
						char buf[MSG_LEN];
						get_laston(p + 5 + 1, buf, FALSE, FALSE);
						msg_to_irc(buf);
						break;
					}
					else if (!strncmp(p, "?who", 4)) {
						u32b p_id;
						cptr acc;

						if (strlen(p) < 6) {
							msg_to_irc("You must specify a character name.");
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
									msg_to_irc(format("That deceased character belonged to account: %s", reserved_name_account[i]));
									done = TRUE;
									break;
								}
							}
							if (done) break;
#endif

#if 0 /* don't check for account name */
							msg_to_irc("That character name does not exist.");
#else /* check for account name */
							if (!GetAccount(&acc, p + 5, NULL, FALSE)) msg_to_irc("That character or account name does not exist.");
							else msg_to_irc("There is no such character, but there is an account of that name.");
#endif
							break;
						}
						acc = lookup_accountname(p_id);
						if (!acc) {
							msg_to_irc("***ERROR: No account found.");
							break;
						}
						if (lookup_player_admin(p_id))
							msg_to_irc(format("That administrative character belongs to: %s", acc));
						else {
							u16b ptype = lookup_player_type(p_id);
							int lev = lookup_player_level(p_id);
							msg_to_irc(format("That level %d %s %s belongs to: %s",
							    lev,
							    //race_info[ptype & 0xff].title,
							    special_prace_lookup[ptype & 0xff],
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
		if (!strcmp(c_pl->name, pname)) {
			return(c_pl->server);
		}
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
		if (!stricmp(c_pl->name, pname) && (!server || server == c_pl->server)) {
			return(c_pl);
		}
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
	if (lp) {
		fprintf(fff, "\n\377y Remote players on different servers:\n\n");
	}
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
		
//		fprintf(fff, "\377%c  %s\377s on '%s'\n", c_pl->server ? 'w' : 'W', c_pl->name, servername);
		fprintf(fff, "\377s %s\377%c %s\n", servername, c_pl->server ? 'w' : 'W', c_pl->name);
		num++;
		lp = lp->next;
	}
	return (num);
}

/* When a server logs in, we get information about it */
void add_server(struct sinfo *sinfo) {
	struct list *lp;
	struct svlist *c_sr;
/*
	c_sr = malloc(sizeof(struct svlist));
*/
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
		if (c_pl->server == id) {
			lp = remlist(&rpmlist, lp);
		}
		else lp = lp->next;
	}

	/* Delete the server info */
	lp = svlist;
	while (lp) {
		c_sr = (struct svlist*)lp->data;
		if (c_sr->sid == id) {
			lp = remlist(&svlist, lp);
		}
		else lp = lp->next;
	}
}

void add_rplayer(struct wpacket *wpk) {
	struct list *lp;
	struct rplist *n_pl, *c_pl;
	unsigned char found = 0;
	if (!wpk->d.play.silent)
		msg_broadcast_format(0, "\374\377s%s has %s the game on another server.", wpk->d.play.name, (wpk->type == WP_NPLAYER ? "entered" : "left"));

	if (wpk->type == WP_NPLAYER && !wpk->d.play.server) return;
	lp = rpmlist;
	while (lp) {
		c_pl = (struct rplist*)lp->data;
//		if (/* c_pl->id == wpk->d.play.id && */ !(strcmp(c_pl->name, wpk->d.play.name))) {
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
		}
	}
	else if (wpk->type == WP_QPLAYER && found)
		remlist(&rpmlist, lp);
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

void world_chat(uint32_t id, char *text) {
	int len;
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
	if (WorldSocket == -1) return;
	spk.type = WP_MSG_TO_IRC;
	len = sizeof(struct wpacket);
	snprintf(spk.d.smsg.stxt, MSG_LEN, "%s", text);
	send(WorldSocket, &spk, len, 0);
}

/* we can rely on ID alone when we merge data */
void world_player(uint32_t id, char *name, uint16_t enter, byte quiet) {
	int len;
	if (WorldSocket == -1) return;
	spk.type = (enter ? WP_NPLAYER : WP_QPLAYER);
	len = sizeof(struct wpacket);
	strncpy(spk.d.play.name, name, 30);
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
