/* player lists */

#include <stdlib.h>

#include "world.h"

struct rplist *rpmlist=NULL;

void send_rplay(struct client *ccl){
	struct rplist *c_pl;
	struct wpacket spk;
	int len;
	len=sizeof(struct wpacket);
	spk.type=WP_NPLAYER;
	spk.serverid=0;

	c_pl=rpmlist;
	while(c_pl){
		spk.d.play.id=c_pl->id;
		spk.d.play.server=c_pl->server;
		strncpy(spk.d.play.name, c_pl->name, 30);
		send(ccl->fd, &spk, len, 0);
		c_pl=c_pl->next;
	}
}

void rem_players(short id){
	struct rplist *c_pl, *p_pl, *d_pl;
	c_pl=rpmlist;
	p_pl=c_pl;
	while(c_pl){
		d_pl=NULL;
		if(c_pl->server==id){
			if(c_pl==rpmlist)
				rpmlist=c_pl->next;
			else
				p_pl->next=c_pl->next;
			d_pl=c_pl;
		}
		p_pl=c_pl;
		c_pl=c_pl->next;
		if(d_pl) free(d_pl);
	}
}

void add_rplayer(struct wpacket *wpk){
	struct rplist *n_pl, *c_pl;
	unsigned short found=0;

	if(wpk->type==WP_NPLAYER && !wpk->d.play.server) return;
	c_pl=rpmlist;
	while(c_pl){
		if(c_pl->id==wpk->d.play.id && !(strcmp(c_pl->name, wpk->d.play.name))){
			found=1;
			break;
		}
		n_pl=c_pl;
		c_pl=c_pl->next;
	}
	if(wpk->type==WP_NPLAYER && !found){
		n_pl=malloc(sizeof(struct rplist));
		n_pl->next=rpmlist;
		n_pl->id=wpk->d.play.id;
		n_pl->server=wpk->d.play.server;
		strncpy(n_pl->name, wpk->d.play.name, 30);
		rpmlist=n_pl;
	}
	else if(wpk->type==WP_QPLAYER && found){
		if(c_pl==rpmlist)
			rpmlist=c_pl->next;
		else
			n_pl->next=c_pl->next;
		free(c_pl);
	}
}
