/* tomenet world main server - copyright 2002 evileye
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "world.h"

#define MAX(x,y) (x > y ? x : y)

struct client *clist=NULL;

void world(int ser){
	int sl;
	struct sockaddr_in cl_in;
	int length=sizeof(struct sockaddr_in);
	char buff[40], *check;
	fd_set fds;

	while(1){
		int mfd=ser;
		struct client *c_cl;

		FD_ZERO(&fds);
		FD_SET(ser, &fds);

		for(c_cl=clist; c_cl; c_cl=c_cl->next){
			mfd=MAX(mfd, c_cl->fd);
			FD_SET(c_cl->fd, &fds);
		}

		sl=select(mfd+1, &fds, NULL, NULL, NULL);
		if(sl==-1){
			fprintf(stderr, "select broke\n");
			return;
		}
		if(FD_ISSET(ser, &fds)){
			sl=accept(ser, (struct sockaddr*)&cl_in, &length);
			if(sl==-1){
				fprintf(stderr, "accept broke\n");
				return;
			}
			check=(char*)inet_ntop(AF_INET, &cl_in.sin_addr, &buff, 40);
			if(check){
				if(cl_in.sin_len){
					printf("accepted connect from %s\n", buff);
				}
			}
			else{
				fprintf(stderr, "Got connection. unable to disply remote host. errno %d\n", errno);
			}
			addclient(sl);
		}

		for(c_cl=clist; c_cl; c_cl=c_cl->next)
			if(FD_ISSET(c_cl->fd, &fds)) handle(c_cl);
		c_cl=clist;
		while(c_cl){
			if(c_cl->flags & CL_QUIT)
				c_cl=remclient(c_cl);
			else c_cl=c_cl->next;
		}
	}
}

void handle(struct client *ccl){
	int x;
	x=recv(ccl->fd, ccl->buf+ccl->blen, 1024-ccl->blen, 0);
	if(x==-1){
		ccl->flags|=CL_QUIT;
		return;
	}
	if(x==0){
		ccl->flags|=CL_QUIT;
		return;
	}
	ccl->blen+=x;
	wproto(ccl);
}

void wproto(struct client *ccl){
	struct wpacket *wpk=ccl->buf;
	while(ccl->blen>=sizeof(struct wpacket)){
		switch(wpk->type){
			case WP_AUTH:
				break;
			case WP_CHAT:
				relay(wpk, ccl);
				break;
			case WP_NPLAYER:
			case WP_QPLAYER:
			default:
				fprintf(stderr, "ignoring undefined packet\n");
		}
		if(ccl->blen>sizeof(struct wpacket)){
			memcpy(ccl->buf, ccl->buf+sizeof(struct wpacket), ccl->blen-sizeof(struct wpacket));
		}
		ccl->blen-=sizeof(struct wpacket);
	}
}

void relay(struct wpacket *wpk, struct client *talker){
	struct client *ccl;
	for(ccl=clist; ccl; ccl=ccl->next){
		if(ccl!=talker){
			send(ccl->fd, wpk, sizeof(struct wpacket), 0); 
		}
	}
}

void addclient(int fd){
	struct client *ncl;
	ncl=malloc(sizeof(struct client));
	if(ncl){
		memset(ncl, 0, sizeof(struct client));
		ncl->fd=fd;
		ncl->next=clist;
		clist=ncl;
	}
}

struct client *remclient(struct client *dcl){
	struct client *ccl;
	ccl=clist;
	if(dcl==clist){
		clist=ccl->next;
		close(dcl->fd);
		free(dcl);
		return(clist);
	}
	while(ccl){
		if(ccl->next==dcl){
			ccl->next=dcl->next;
			close(dcl->fd);
			free(dcl);
			return(ccl->next);
		}
		ccl=ccl->next;
	}
	fprintf(stderr, "Unable to remove connection\n");
	return(clist);
}
