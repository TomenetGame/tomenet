/* experimental code - evileye */
/* this does not necessarily follow any sensible design */

#include "angband.h"

#ifdef TOMENET_WORLDS

#include "../world/world.h"

struct wpacket spk;

int world_comm(int fd, int arg){
	static char buffer[1024];
	static char bpos=0;
	int x;
	struct wpacket *wpk;
	x=recv(fd, buffer+bpos, 1024-bpos, 0);
	if(x>=sizeof(struct wpacket)){
		wpk=buffer+bpos;
		switch(wpk->type){
			case WP_CHAT:
				msg_broadcast_format(0, "!%s", wpk->d.chat.ctxt);
				break;
			default:
		}
	}
	if(x==0){
		/* This happens... we are screwed (fortunately SIGPIPE isnt handled) */
		printf("pfft. world server closed\n");
		close(WorldSocket);	/* ;) this'll fix it... */
	}
	return(0);
}

void world_chat(char *text){
	int x, len;
	spk.type=WP_CHAT;
	len=sizeof(struct wpacket);
	strncpy(spk.d.chat.ctxt, text, 80);
	x=send(WorldSocket, &spk, len, 0);
}

#endif
