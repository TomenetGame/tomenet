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
int snum = 0;

void handle(struct client *ccl);
void relay(struct wpacket *wpk, struct client *talker);
void wproto(struct client *ccl);
void addclient(int fd);
void remclient(struct list *dlp);
struct list *remlist(struct list **head, struct list *dlp);
uint32_t get_message_type(char *msg);

struct list *clist = NULL;	/* struct client */

void world(int ser) {
	int sl;
	struct sockaddr_in cl_in;
	socklen_t length = sizeof(struct sockaddr_in);
//	char buff[40], *check;
	fd_set fds;

/* Adding some commentary here -.- Also note that gameservers are referred to as 'clients' (c_cl)  - C. Blue */
	secure.secure = 1;	/* 1 = don't allow unauth'ed gameservers to connect! */
	secure.chat = 0;	/* 1 = relay chat from unauth'ed gameservers */
	secure.msgs = 0;	/* 1 = relay messages from unauth'ed gameservers */
	secure.play = 0;	/* 1 = add players from unauth'ed gameservers */

	while (1) {
		int mfd = ser;
		struct client *c_cl;
		struct list *lp;

		FD_ZERO(&fds);
		FD_SET(ser, &fds);

		for (lp = clist; lp; lp = lp->next) {
			c_cl = (struct client*)lp->data;
			mfd = MAX(mfd, c_cl->fd);
			FD_SET(c_cl->fd, &fds);
		}

		sl = select(mfd + 1, &fds, NULL, NULL, NULL);
		if (sl == -1) {
			/* Don't care about signal interruptions */
			if (errno != EINTR) {
	                        fprintf(stderr, "select broke\n");
				perror("select");
				return;
			}
		}
		if (FD_ISSET(ser, &fds)) {
			sl = accept(ser, (struct sockaddr*)&cl_in, &length);
			if (sl == -1) {
				fprintf(stderr, "accept broke\n");
				return;
			}
#if 0
			check = (char*)inet_ntop(AF_INET, &cl_in.sin_addr, &buff, 40);
			if (check) {
				if (cl_in.sin_len)
					printf("accepted connect from %s\n", buff);
			} else {
				fprintf(stderr, "Got connection. unable to display remote host. errno %d\n", errno);
			}
#endif
			addclient(sl);
                        fprintf(stderr, "added!\n");
		}

		for (lp = clist; lp; lp = lp->next) {
			if (FD_ISSET(((struct client*)lp->data)->fd, &fds)) handle((struct client*)lp->data);
		}

		lp = clist;
		while (lp) {
			c_cl = (struct client*)lp->data;
			if (c_cl->flags & CL_QUIT || (c_cl->authed == -1 && secure.secure)) {
				remclient(lp);
				lp = remlist(&clist, lp);
			}
			else lp = lp->next;
		}
	}
}

void handle(struct client *ccl) {
	int x;
        fprintf(stderr, "handling\n");
        x = recv(ccl->fd, ccl->buf + ccl->blen, 1024 - ccl->blen, 0);

	/* Error condition */
	if (x == -1) {
		fprintf(stderr, "Error. killing client %d\n", errno);
		ccl->flags |= CL_QUIT;
		return;
	}

	/* Connection death most likely */
	if (x == 0) {
		fprintf(stderr, "Client quit %d\n", errno);
		ccl->flags |= CL_QUIT;
		return;
	}
	ccl->blen += x;
	wproto(ccl);
}

void wproto(struct client *ccl) {
	int client_chat, client_all, client_ctrlo;
	struct wpacket *wpk = (struct wpacket*)ccl->buf;
        while (ccl->blen >= sizeof(struct wpacket)) {
                fprintf(stderr, "protoing... type %d\n", wpk->type);
		client_chat = client_all = client_ctrlo = 0;
                switch (wpk->type) {
			case WP_LACCOUNT:
				/* ignore unauthed servers
				   only legitimate servers should
				   ever send this */
				if (ccl->authed > 0) l_account(wpk, ccl);
				break;
			case WP_RESTART:
				/* mass restart */
				if (ccl->authed > 0) relay(wpk, ccl);
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

				ccl->authed = pwcheck(wpk->d.auth.pass, wpk->d.auth.val);
				if (ccl->authed) send_sinfo(ccl, NULL);
				/* Send it the current players */
				send_rplay(ccl);
				break;

			/* Integrate chat/private chat */
			case WP_CHAT:
                                /* only relay all for now */
				if (ccl->authed && ((ccl->authed>0) || secure.chat)) {
					char msg[MSG_LEN], *p = wpk->d.chat.ctxt;
					client_all = client_chat = client_ctrlo = 0;
					/* strip chat codes and reinsert them at the beginning */
					if (*p == '\374') {
						client_all = 1;
						p++;
					} else if (*p == '\375') {
						client_chat = 1;
						p++;
					}
					if (*p == '\376') {
						client_ctrlo = 1;
						p++;
					}
//					snprintf(msg, MSG_LEN, "\377o[\377%c%d\377o] %s", (ccl->authed>0 ? 'g' : 'r'), ccl->authed, wpk->d.chat.ctxt);
					snprintf(msg, MSG_LEN, "%s%s\377%c[%d]\377w %s%c",
					    client_all ? "\374" : (client_chat ? "\375" : ""),
					    client_ctrlo ? "\376" : "",
					    (ccl->authed>0 ? 'g' : 'r'), ccl->authed, p, '\0');
//					msg[MSG_LEN - 1] = '\0';
					strncpy(wpk->d.chat.ctxt, msg, MSG_LEN);
					relay(wpk, ccl);
				}
				break;
			/* Integrate server-directed chat */
			case WP_IRCCHAT:
                                /* only relay all for now */
				if (ccl->authed && ((ccl->authed>0) || secure.chat)) {
					char msg[MSG_LEN], *p = wpk->d.chat.ctxt;
					client_all = client_chat = client_ctrlo = 0;
					/* strip chat codes and reinsert them at the beginning */
					if (*p == '\374') {
						client_all = 1;
						p++;
					} else if (*p == '\375') {
						client_chat = 1;
						p++;
					}
					if (*p == '\376') {
						client_ctrlo = 1;
						p++;
					}
//					snprintf(msg, MSG_LEN, "\377o[\377%c%d\377o] %s", (ccl->authed>0 ? 'g' : 'r'), ccl->authed, wpk->d.chat.ctxt);
					snprintf(msg, MSG_LEN, "%s%s\377%c(IRC)\377w %s%c",
					    client_all ? "\374" : (client_chat ? "\375" : ""),
					    client_ctrlo ? "\376" : "",
					    (ccl->authed > 0 ? 'g' : 'r'), p, '\0');
//					msg[MSG_LEN - 1] = '\0';
					strncpy(wpk->d.chat.ctxt, msg, MSG_LEN);
					relay(wpk, ccl);
				}
				break;
			case WP_PMSG:
				/* MUST be authed for private messages */
				if (ccl->authed > 0) {

					/* add same code in front as for WP_CHAT */
//					char msg[MSG_LEN];
//					snprintf(msg, MSG_LEN, "\377%c[%d]\377w %s", (ccl->authed>0 ? 'g' : 'r'), ccl->authed, wpk->d.chat.ctxt);
//					strncpy(wpk->d.chat.ctxt, msg, MSG_LEN);
//d.pmsg.player
//d.pmsg.victim
//d.pmsg.ctxt

					struct list *lp;
					struct client *dcl;
					wpk->serverid = ccl->authed;

					for (lp = clist; lp; lp = lp->next) {
						dcl = (struct client*)lp->data;
						if (dcl->authed == wpk->d.pmsg.sid) {
							send(dcl->fd, wpk, sizeof(struct wpacket), 0); 
						}
					}
				}
				break;
			case WP_NPLAYER:
			case WP_QPLAYER:
				/* STORE players here */
				if (ccl->authed && (ccl->authed > 0 || secure.play)) {
					wpk->d.play.server = ccl->authed;
					add_rplayer(wpk);
					relay(wpk, ccl);
				}
				break;
			case WP_MESSAGE:
			case WP_MSG_TO_IRC:
				/* simple relay message */
				client_all = client_chat = client_ctrlo = 0;
				if (ccl->authed && (ccl->authed>0 || secure.msgs)) {
					/* add same code in front as for WP_CHAT */
					char msg[MSG_LEN], *p = wpk->d.smsg.stxt;
					/* strip chat codes and reinsert them at the beginning */
					if (*p == '\374') {
						client_all = 1;
						p++;
					} else if (*p == '\375') {
						client_chat = 1;
						p++;
					}
					if (*p == '\376') {
						client_ctrlo = 1;
						p++;
					}
					snprintf(msg, MSG_LEN, "%s%s\377%c[%d]\377w %s",
					    client_all ? "\374" : (client_chat ? "\375" : ""),
					    client_ctrlo ? "\376" : "",
					    (ccl->authed>0 ? 'g' : 'r'), ccl->authed, p);
					/* make sure it's null terminated (if snprintf exceeds 160 chars and places no \0) - mikaelh
					   (replaced 160 by MSG_LEN - C. Blue) */
					msg[MSG_LEN - 1] = '\0';
					strncpy(wpk->d.smsg.stxt, msg, MSG_LEN);

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
		if (ccl->blen>sizeof(struct wpacket)) {
			memcpy(ccl->buf, ccl->buf+sizeof(struct wpacket), ccl->blen-sizeof(struct wpacket));
		}
		ccl->blen -= sizeof(struct wpacket);
	}
}

/* Send duplicate packet to all servers except originating one */
void relay(struct wpacket *wpk, struct client *talker){
	struct list *lp;
	struct client *ccl;
	int morph = wpk->type;

	wpk->serverid = talker->authed;

	for(lp = clist; lp; lp = lp->next){
		ccl = (struct client*)lp->data;

		if (ccl == talker) continue;

		if (wpk->type == WP_MSG_TO_IRC) {
			if (ccl->authed <= 0 || !strstr(slist[ccl->authed - 1].name, "Relay_server")) /* usually trailing spaces */
				continue;
			/* hack: convert to normal message type, since we've now picked the IRC server as exclusive receipient */
			wpk->type = WP_MESSAGE;
		}

		/* Check the packet relay mask for authed servers - mikaelh */
		if (ccl->authed > 0) {
			if (!(slist[ccl->authed - 1].rflags & (1 << (wpk->type - 1)))) {
				continue; /* don't relay */
			}

			/* Filter messages (except special replies to IRC channel) */
			if (wpk->type == WP_MESSAGE && morph != WP_MSG_TO_IRC) {
				if (!(slist[ccl->authed - 1].mflags & get_message_type(wpk->d.smsg.stxt))) {
					continue; /* don't relay */
				}
			}
		}

		/* Specialty: Abuse chat.id for determining destination server. */
		if (wpk->type == WP_IRCCHAT && wpk->d.chat.id != ccl->authed) continue;

		send(ccl->fd, wpk, sizeof(struct wpacket), 0);
		wpk->type = morph; /* unhack */

		/* Temporary stderr output */
		if (bpipe) {
			fprintf(stderr, "SIGPIPE from relay (fd: %d)\n", ccl->fd);
			bpipe = 0;
		}
	}
}

void reply(struct wpacket *wpk, struct client *ccl) {
	send(ccl->fd, wpk, sizeof(struct wpacket), 0);
	if (bpipe) {
		fprintf(stderr, "SIGPIPE from reply (fd: %d)\n", ccl->fd);
		bpipe = 0;
	}
}

/* Generic list handling function */
struct list *addlist(struct list **head, int dsize){
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

/* add a new client to the server's list */
void addclient(int fd) {
	struct list *lp;
	struct client *ncl;

	lp = addlist(&clist, sizeof(struct client));
	if (lp != (struct list*)NULL) {
		ncl = (struct client*)lp->data;
		memset(ncl, 0, sizeof(struct client));
		ncl->fd = fd;
		ncl->authed = 0;
		initauth(ncl);
	}
}

/* Generic list handling function */
struct list *remlist(struct list **head, struct list *dlp){
	struct list *lp;
	lp = *head;
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

void remclient(struct list *dlp){
	struct client *ccl;
	ccl = (struct client *)dlp->data;
	if (ccl->authed > 0) {
		/* Tell other servers if an authed server goes down */
		struct wpacket spk;
		rem_players(ccl->authed);
		spk.type = WP_SQUIT;
		spk.d.sid = ccl->authed;
		relay(&spk, ccl);
	}
	/* Close the socket - mikaelh */
	close(ccl->fd);
}

uint32_t get_message_type(char *msg) {
	/* skip to the real message */
	msg = strchr(msg, ' ');
	/* catch usages of get_message_type() in debug places where msg might be empty */
	if (!msg) {
		fprintf(stderr, "Illegal get_message_type() call\n");
		return 0;
	}
	msg++;

	if (strstr(msg, " has left the game.")) {
		return WMF_PLEAVE;
	} else if (strstr(msg, " has entered the game.") || strstr(msg, " sets foot into the world.")) {
		return WMF_PJOIN;
	} else if (strstr(msg, "Morgoth, Lord of Darkness was slain by ")) {
		return (WMF_PWIN | WMF_UNIDEATH);
	} else if (strstr(msg, " is henceforth known as ")) {
		return WMF_PWIN;
	} else if (strstr(msg, " has attained level ") &&
	    atoi(strstr(msg, " has attained level ") + 20) >= 30) {
		return (WMF_HILVLUP | WMF_LVLUP);
	} else if (strstr(msg, " has attained level ")) {
		return WMF_LVLUP;
	} else if (strstr(msg, " was slain by ")) {
		return WMF_UNIDEATH;
	} else if (strstr(msg, " was destroyed by ") ||
		   strstr(msg, " was wasted by ") ||
		   strstr(msg, " was crushed by ") ||
		   strstr(msg, " was shredded by ") ||
		   strstr(msg, " was torn up by ") ||
		   strstr(msg, " and destroyed by ") ||
		   strstr(msg, " was killed by ") ||
		   strstr(msg, " was annihilated by ") ||
		   strstr(msg, " was vaporized by ") ||
		   strstr(msg, " committed suicide.") ||
		   strstr(msg, " has retired to ") ||
		   strstr(msg, " bids farewell to this plane") ||
		   strstr(msg, " was defeated by ")) {
		return WMF_PDEATH;
	} else if ((!strncmp(msg, "\377a>>", 4) && strstr(msg, " wins ")) /* global events */
	    || (strstr(msg, " reached floor ")) /* ironman deep dive challenge - death */
	    || (strstr(msg, " made it through the ")) /* ironman deep dive challenge - win */
	    || (strstr(msg, " withdrew from the ")) /* ironman deep dive challenge - withdrawal */
	    ) {
		return WMF_EVENTS;
	}

	return 0;
}
