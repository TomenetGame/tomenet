/* experimental code - evileye */
/* this does not necessarily follow any sensible design */

#include <sys/types.h>
#include <sys/socket.h>

#include "angband.h"

#ifdef TOMENET_WORLDS

#include "../world/world.h"

struct rplist *rpmlist=NULL;

struct wpacket spk;

unsigned long chk(unsigned char *s1, unsigned char *s2);
void rem_players(short id);
void add_rplayer(struct wpacket *wpk);

void world_comm(int fd, int arg){
	static char buffer[1024];
	static char bpos=0;
	int x;
	struct wpacket *wpk;
	x=recv(fd, buffer+bpos, 1024-bpos, 0);
	if(x>=sizeof(struct wpacket)){
		wpk=(struct wpacket*)(buffer+bpos);
		switch(wpk->type){
			case WP_CHAT:
				/* TEMPORARY chat broadcast method */
				msg_broadcast_format(0, "%s", wpk->d.chat.ctxt);
				break;
			case WP_MESSAGE:
				/* A raw message - no data */
				msg_broadcast_format(0, "%s", wpk->d.smsg.stxt);
				break;
			case WP_NPLAYER:
			case WP_QPLAYER:
				/* we need to handle a list */
				/* full death must count! */
				add_rplayer(wpk);
				break;
			case WP_AUTH:
				/* Authentication request */
				wpk->d.auth.val=chk(cfg.pass, wpk->d.auth.pass);
				x=sizeof(struct wpacket);
				x=send(WorldSocket, wpk, x, 0);
				break;
			case WP_SQUIT:
				/* Remove players */
				rem_players(wpk->d.sid);
				break;
			case WP_RESTART:
				set_runlevel(0);
				break;
			default:
				s_printf("unknown packet from world: %d\n", wpk->type);
		}
	}
	if(x==0){
		/* This happens... we are screwed (fortunately SIGPIPE isnt handled) */
		s_printf("pfft. world server closed\n");
		remove_input(WorldSocket);
		close(WorldSocket);	/* ;) this'll fix it... */
		WorldSocket=-1;
	}
}

/* proper data will come with merge */
void world_remote_players(FILE *fff){
	struct rplist *c_pl;
	c_pl=rpmlist;
	if(c_pl){
		fprintf(fff, "y  Remote players\nr\n");
	}
	while(c_pl){
		fprintf(fff, "%c   %s@%d\n", c_pl->server ? 'o' : 's', c_pl->name, c_pl->server);
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
	if(!wpk->d.play.silent)
		msg_broadcast_format(0, "\377s%s has %s the game.", wpk->d.play.name, (wpk->type==WP_NPLAYER ? "entered" : "left"));

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

void world_chat(unsigned long id, char *text){
	int x, len;
	if(WorldSocket==-1) return;
	spk.type=WP_CHAT;
	len=sizeof(struct wpacket);
	strncpy(spk.d.chat.ctxt, text, 80);
	spk.d.chat.id=id;
	x=send(WorldSocket, &spk, len, 0);
}

void world_reboot(){
	int x, len;
	if(WorldSocket==-1) return;
	spk.type=WP_RESTART;
	len=sizeof(struct wpacket);
	x=send(WorldSocket, &spk, len, 0);
}

void world_msg(char *text){
	int x, len;
	if(WorldSocket==-1) return;
	spk.type=WP_MESSAGE;
	len=sizeof(struct wpacket);
	strncpy(spk.d.smsg.stxt, text, 160);
	x=send(WorldSocket, &spk, len, 0);
}

/* we can rely on ID alone when we merge data */
void world_player(unsigned long id, char *name, unsigned short enter, byte quiet){
	int x, len;
	if(WorldSocket==-1) return;
	spk.type=(enter ? WP_NPLAYER : WP_QPLAYER);
	len=sizeof(struct wpacket);
	strncpy(spk.d.play.name, name, 30);
	spk.d.play.id=id;
	spk.d.play.silent=quiet;
	x=send(WorldSocket, &spk, len, 0);
}

/* unified, hopefully unique password check function */
unsigned long chk(unsigned char *s1, unsigned char *s2){
	unsigned int i, j=0;
	int m1, m2;
	static unsigned long rval[2]={0, 0};
	rval[0]=0L;
	rval[1]=0L;
	m1=strlen(s1);
	m2=strlen(s2);
	for(i=0; i<m1; i++){
		rval[0]+=s1[i];
		rval[0]<<=5;
	}
	for(j=0; j<m2; j++){
		rval[1]+=s2[j];
		rval[1]<<=3;
	}
	for(i=0; i<m1; i++){
		rval[1]+=s1[i];
		rval[1]<<=(3+rval[0]%5);
		rval[0]+=s2[j];
		j=rval[0]%m2;
		rval[0]<<=(3+rval[1]%3);
	}
	return(rval[0]+rval[1]);
}
#endif
