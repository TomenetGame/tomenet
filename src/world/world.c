/* tomenet world main server - copyright 2002 evileye
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <string.h>

#include "world.h"
#include "externs.h"

#define MAX(x,y) (x > y ? x : y)


extern int bpipe;

struct secure secure;
struct serverinfo slist[MAX_SERVERS];
int snum=0;

void handle(struct client *ccl);
void relay(struct wpacket *wpk, struct client *talker);
void wproto(struct client *ccl);
void addclient(int fd);
void remclient(struct list *dlp);
struct list *remlist(struct list **head, struct list *dlp);

struct list *clist=NULL;	/* struct client */

void world(int ser){
	int sl;
	struct sockaddr_in cl_in;
	socklen_t length = sizeof(struct sockaddr_in);
//	char buff[40], *check;
	fd_set fds;

/* Adding some commentary here -.- Also note that gameservers are referred to as 'clients' (c_cl)  - C. Blue */
	secure.secure=0;	/* 1 = don't allow unauth'ed gameservers to connect! */
	secure.chat=0;		/* 1 = relay chat from unauth'ed gameservers */
	secure.msgs=0;		/* 1 = relay messages from unauth'ed gameservers */
	secure.play=0;		/* 1 = add players from unauth'ed gameservers */

	while(1){
		int mfd=ser;
		struct client *c_cl;
		struct list *lp;

		FD_ZERO(&fds);
		FD_SET(ser, &fds);

		for(lp=clist; lp; lp=lp->next){
			c_cl=(struct client*)lp->data;
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
#if 0
			check=(char*)inet_ntop(AF_INET, &cl_in.sin_addr, &buff, 40);
			if(check){
				if(cl_in.sin_len){
					printf("accepted connect from %s\n", buff);
				}
			}
			else{
				fprintf(stderr, "Got connection. unable to display remote host. errno %d\n", errno);
			}
#endif
			addclient(sl);
                        fprintf(stderr, "added!\n");
		}

		for(lp=clist; lp; lp=lp->next) {
			if(FD_ISSET(((struct client*)lp->data)->fd, &fds)) handle((struct client*)lp->data);
		}

		lp=clist;
		while(lp){
			c_cl=(struct client*)lp->data;
			if(c_cl->flags & CL_QUIT || (c_cl->authed==-1 && secure.secure)){
				remclient(lp);
				lp=remlist(&clist, lp);
			}
			else lp=lp->next;
		}
	}
}

void handle(struct client *ccl){
	int x;
        fprintf(stderr, "handling\n");
        x=recv(ccl->fd, ccl->buf+ccl->blen, 1024-ccl->blen, 0);

	/* Error condition */
	if(x==-1){
		fprintf(stderr, "Error. killing client %d\n", errno);
		ccl->flags|=CL_QUIT;
		return;
	}

	/* Connection death most likely */
	if(x==0){
		fprintf(stderr, "Client quit %d\n", errno);
		ccl->flags|=CL_QUIT;
		return;
	}
	ccl->blen+=x;
	wproto(ccl);
}

void wproto(struct client *ccl){
	struct wpacket *wpk=(struct wpacket*)ccl->buf;
        while(ccl->blen>=sizeof(struct wpacket)){
                fprintf(stderr, "protoing... type %d\n", wpk->type);
                switch(wpk->type){
			case WP_LACCOUNT:
				/* ignore unauthed servers
				   only legitimate servers should
				   ever send this */
				if(ccl->authed>0){
					l_account(wpk, ccl);
				}
				break;
			case WP_RESTART:
				/* mass restart */
				if(ccl->authed>0){
					relay(wpk, ccl);
				}
				break;
			case WP_AUTH:
			/* method - plaintext password on server.
			   server uses this to encrypt an answer to a RANDOMLY
			   generated plaintext password made here.
			   without this anyone can hack,
			   making fake messages etc.
			   It will be done BEFORE savefiles
			   and other data are shared. Some machines may
			   use a dynamic IP, so this is made *more* necessary */

				ccl->authed=pwcheck(wpk->d.auth.pass, wpk->d.auth.val);
				if(ccl->authed) send_sinfo(ccl, NULL);
				/* Send it the current players */
				send_rplay(ccl);
				break;

			/* Integrate chat/private chat */
			case WP_CHAT:
                                /* only relay all for now */
				if(ccl->authed && ((ccl->authed>0) || secure.chat)){
					char msg[160];
//					snprintf(msg, 160, "\377o[\377%c%d\377o] %s", (ccl->authed>0 ? 'g' : 'r'), ccl->authed, wpk->d.chat.ctxt);
					snprintf(msg, 160, "\377%c[%d]\377w %s%c", (ccl->authed>0 ? 'g' : 'r'), ccl->authed, wpk->d.chat.ctxt, '\0');
//					msg[159] = '\0';
					strncpy(wpk->d.chat.ctxt, msg, 160);
					relay(wpk, ccl);
				}
				break;
			case WP_PMSG:
				/* MUST be authed for private messages */
				if(ccl->authed>0){

					/* add same code in front as for WP_CHAT */
//					char msg[160];
//					snprintf(msg, 160, "\377%c[%d]\377w %s", (ccl->authed>0 ? 'g' : 'r'), ccl->authed, wpk->d.chat.ctxt);
//					strncpy(wpk->d.chat.ctxt, msg, 160);
//d.pmsg.player
//d.pmsg.victim
//d.pmsg.ctxt

					struct list *lp;
					struct client *dcl;
					wpk->serverid=ccl->authed;

					for(lp=clist; lp; lp=lp->next){
						dcl=(struct client*)lp->data;
						if(dcl->authed==wpk->d.pmsg.sid){
							send(dcl->fd, wpk, sizeof(struct wpacket), 0); 
						}
					}
				}
				break;
			case WP_NPLAYER:
			case WP_QPLAYER:
				/* STORE players here */
				if(ccl->authed && (ccl->authed>0 || secure.play)){
					wpk->d.play.server=ccl->authed;
					add_rplayer(wpk);
					relay(wpk, ccl);
				}
				break;
			case WP_MESSAGE:
				/* simple relay message */
				if(ccl->authed && (ccl->authed>0 || secure.msgs)){

					/* add same code in front as for WP_CHAT */
					char msg[160];
					snprintf(msg, 160, "\377%c[%d]\377w %s", (ccl->authed>0 ? 'g' : 'r'), ccl->authed, wpk->d.smsg.stxt);
					/* make sure it's null terminated (if snprintf exceeds 160 chars and places no \0) - mikaelh */
					msg[159] = '\0';
					strncpy(wpk->d.smsg.stxt, msg, 160);

					relay(wpk, ccl);
				}
				break;
			case WP_LOCK:
				/* lock server object */
				attempt_lock(ccl, wpk->d.lock.ltype, wpk->d.lock.ttl, wpk->d.lock.obj);
				break;
			case WP_UNLOCK:
				/* unlock server object */
				attempt_unlock(ccl, wpk->d.lock.ltype, wpk->d.lock.ttl, wpk->d.lock.obj);
				break;
			default:
				fprintf(stderr, "ignoring undefined packet %d\n", wpk->type);
		}
		if(ccl->blen>sizeof(struct wpacket)){
			memcpy(ccl->buf, ccl->buf+sizeof(struct wpacket), ccl->blen-sizeof(struct wpacket));
		}
		ccl->blen-=sizeof(struct wpacket);
	}
}

/* Send duplicate packet to all servers except originating
   one */
void relay(struct wpacket *wpk, struct client *talker){
	struct list *lp;
	struct client *ccl;
	wpk->serverid=talker->authed;
	for(lp=clist; lp; lp=lp->next){
		ccl=(struct client*)lp->data;
		if(ccl!=talker){
			send(ccl->fd, wpk, sizeof(struct wpacket), 0); 
			/* Temporary stderr output */
			if(bpipe){
				fprintf(stderr, "SIGPIPE from relay (fd: %d)\n", ccl->fd);
				bpipe=0;
			}
		}
	}
}

void reply(struct wpacket *wpk, struct client *ccl){
	send(ccl->fd, wpk, sizeof(struct wpacket), 0);
	if(bpipe){
		fprintf(stderr, "SIGPIPE from reply (fd: %d)\n", ccl->fd);
		bpipe=0;
	}
}

/* Generic list handling function */
struct list *addlist(struct list **head, int dsize){
	struct list *newlp;
	newlp=malloc(sizeof(struct list));
	if(newlp){
		newlp->data=malloc(dsize);
		if(newlp->data!=NULL){
			newlp->next=*head;
			*head=newlp;
			return(newlp);
		}
		free(newlp);
	}
	return(NULL);
}

/* add a new client to the server's list */
void addclient(int fd){
	struct list *lp;
	struct client *ncl;

	lp=addlist(&clist, sizeof(struct client));
	if(lp!=(struct list*)NULL){
		ncl=(struct client*)lp->data;
		memset(ncl, 0, sizeof(struct client));
		ncl->fd=fd;
		ncl->authed=0;
		initauth(ncl);
	}
}

/* Generic list handling function */
struct list *remlist(struct list **head, struct list *dlp){
	struct list *lp;
	struct client *ncl;
	lp=*head;
	if(dlp==*head){
		*head=lp->next;
		ncl = (struct client*)dlp->data;
		close(ncl->fd); /* the socket needs to be closed - mikaelh */
		free(dlp->data);
		free(dlp);
		return(*head);
	}
	while(lp){
		if(lp->next==dlp){
			lp->next=dlp->next;
			ncl = (struct client*)dlp->data;
			close(ncl->fd); /* the socket needs to be closed - mikaelh */
			free(dlp->data);
			free(dlp);
			return(lp->next);
		}
		lp=lp->next;
	}
	return(dlp->next);
}

void remclient(struct list *dlp){
	struct client *ccl;
	ccl=(struct client *)dlp->data;
	if(ccl->authed>0){
		/* Tell other servers if an authed server goes down */
		struct wpacket spk;
		rem_players(ccl->authed);
		spk.type=WP_SQUIT;
		spk.d.sid=ccl->authed;
		relay(&spk, ccl);
	}
}
