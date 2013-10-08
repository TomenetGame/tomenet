/* player lists */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "world.h"
#include "externs.h"

extern int bpipe;

static struct list *rpmlist = NULL;

struct list *addlist(struct list **head, int dsize);
struct list *remlist(struct list **head, struct list *dlp);

/* Send server information to other servers */
void send_sinfo(struct client *ccl, struct client *priv) {
	struct wpacket spk;
	spk.type = WP_SINFO;
	spk.serverid = 0;
	strncpy(spk.d.sinfo.name, slist[ccl->authed-1].name, 30);
	spk.d.sinfo.sid = ccl->authed;
	if (priv)
		reply(&spk, priv);
	else
		relay(&spk, ccl);
}

/*
 * This function is called when a new server starts
 * and needs to be given the current state of the
 * world.
 */
void send_rplay(struct client *ccl) {
	struct list *clp, *rlp;
	struct rplist *c_pl;
	struct client *c_cl;
	struct wpacket spk;

	/* Send online server info */
	for (clp = clist; clp; clp = clp->next) {
		c_cl = (struct client*)clp->data;
		if (c_cl == ccl) continue;
		if (c_cl->authed <= 0) continue;
		send_sinfo(c_cl, ccl);
	}

	/* Send online player login packets */
	rlp = rpmlist;
	spk.type = WP_NPLAYER;
	spk.serverid = 0;
	spk.d.play.silent = 1;
	while (rlp) {
		c_pl = (struct rplist*)rlp->data;
		spk.d.play.id = c_pl->id;
		spk.d.play.server = c_pl->server;
		strncpy(spk.d.play.name, c_pl->name, 30);
		reply(&spk, ccl);
		/* Temporary stderr output */
		if (bpipe) {
			fprintf(stderr, "SIGPIPE from send_rplay (fd: %d)\n", ccl->fd);
			bpipe = 0;
		}
		rlp = rlp->next;
	}
}

/* Remove all players from a server */
void rem_players(int16_t id) {
	struct rplist *c_pl;
	struct list *lp;

	lp = rpmlist;
	while (lp) {
		c_pl = (struct rplist*)lp->data;
		if (c_pl->server == id)
			lp = remlist(&rpmlist, lp);
		else lp = lp->next;
	}
}

void add_rplayer(struct wpacket *wpk) {
	struct list *lp;
	struct rplist *n_pl, *c_pl;
	unsigned char found = 0;

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
	else if(wpk->type == WP_QPLAYER && found)
		remlist(&rpmlist, lp);
}
