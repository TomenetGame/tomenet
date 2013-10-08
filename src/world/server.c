/* TomeNET world server test code copyright 2002 Richard Smith. */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>
#include <unistd.h>

#include <errno.h>

#include "world.h"
#include "externs.h"

#define PORT 18360
#define ADDR INADDR_ANY

struct sockaddr_in sa;

void loadservers();
void sig_pipe(int num);

int bpipe = 0;

int main(int argc, char *argv[]) {
	int ser;
	initrand();
	loadservers();
	ser = createsocket(PORT, ADDR);
	signal(SIGPIPE, sig_pipe);	/* Catch SIGPIPE... */
	if (ser != -1) {
		world(ser);
		close(ser);
	}
	return(0);
}

int createsocket(int port, uint32_t ip) {
	int option = 1;
	struct sockaddr_in s_in;
	int check;
	int ss;

	/* Create a socket */
	ss = socket(AF_INET, SOCK_STREAM, 0);
	if (ss < 0) return(-1);

	/* Set option on socket not to wait on shutdown */
	setsockopt(ss, SOL_SOCKET, SO_REUSEADDR, (void*)&option, sizeof(int));

	s_in.sin_family = AF_INET;
	s_in.sin_addr.s_addr = ip;
	s_in.sin_port = htons(port);

	check = bind(ss, (struct sockaddr *)&s_in, sizeof(struct sockaddr_in));
	if (check != -1) {
		check = listen(ss, 5);
		if (check != -1) return(ss);
	}
	close(ss);
	return(-1);
}

void loadservers() {
	FILE *fp;
	int i = 0, j, n;
	char flags[20];
	fp = fopen("servers", "r");
	if (fp == (FILE*)NULL) return;
	do {
                fscanf(fp, "%s%s%s\n", slist[i].name, slist[i].pass, flags);

		/* Normalize server name lengths to 15 characters: */
		while (strlen(slist[i].name) < 15) strcat(slist[i].name, " ");

		/* Default packet mask */
		slist[i].rflags = WPF_LOCK | WPF_UNLOCK | WPF_AUTH | WPF_SQUIT | WPF_RESTART | WPF_SINFO;
		/* WPF_LACCOUNT is never relayed */

		/* Default message mask */
		slist[i].mflags = 0; /* Empty */

		/* Packet flags - mikaelh */
		for (j = 0, n = strlen(flags); j < n; j++) {
			switch (flags[j]) {
				case 'C':
					slist[i].rflags |= WPF_CHAT;
					break;
				case 'N':
					slist[i].rflags |= WPF_NPLAYER;
					break;
				case 'Q':
					slist[i].rflags |= WPF_QPLAYER;
					break;
				case 'D':
					slist[i].rflags |= WPF_DEATH;
					break;
				case 'M':
					slist[i].rflags |= WPF_MESSAGE;
					break;
				case 'P':
					slist[i].rflags |= WPF_PMSG;
					break;
				case 'l':
					slist[i].mflags |= WMF_LVLUP;
					break;
				case 'h':
					slist[i].mflags |= WMF_HILVLUP;
					break;
				case 'u':
					slist[i].mflags |= WMF_UNIDEATH;
					break;
				case 'w':
					slist[i].mflags |= WMF_PWIN;
					break;
				case 'd':
					slist[i].mflags |= WMF_PDEATH;
					break;
				case 'n':
					slist[i].mflags |= WMF_PJOIN;
					break;
				case 'q':
					slist[i].mflags |= WMF_PLEAVE;
					break;
				case 'S':
					slist[i].rflags |= WPF_IRCCHAT;
					break;
				case 'e':
					slist[i].mflags |= WMF_EVENTS;
					break;
			}
		}

                printf("server: %s : [%s]\n", slist[i].name, slist[i].pass);
		i++;
	} while (!feof(fp) && i < MAX_SERVERS);
	snum = i;
	fclose(fp);
}

void sig_pipe(int num) {
	signal(SIGPIPE, SIG_IGN);
	fprintf(stderr, "Broken pipe signal received\n");
	bpipe = 1;
	signal(SIGPIPE, &sig_pipe);
}
