/* evileye meta client - 2003*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
// #include <sys/signal.h>
/* signal.h should be included instead of sys/signal.h according to mux - mikaelh */
#include <signal.h>
#include <netinet/in.h>
#include <netdb.h>

#include <errno.h>

#define PORT 8800
#define HOST "meta.tomenet.eu"

void callup(int val);
int connsocket(int port, char *host);
void callmeta(void);

char *host = HOST;

char metadata[16384];

int main(int argc, char *argv[]) {
	if (argc == 2) host = argv[1];
	printf("Evileye meta client started\n");
	callmeta();
	/* ok so its not sigaction... */
	signal(SIGINT, SIG_IGN);
	signal(SIGUSR1, callup);
	signal(SIGTERM, callup);
	while (1) {
		sleep(15);

		/* call the meta */
		callmeta();
	}
	return (0);
}

/* the signal handler */
void callup(int val) {
	if (val == SIGTERM) {
		fprintf(stderr, "Meta client closing\n");
		callmeta();
		exit(0);
	}
	else if (val == SIGUSR1) callmeta();
}

/* signalled, so we call the meta */
void callmeta() {
	int ser;
	FILE *fp;
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGTERM);
	sigaddset(&sigset, SIGUSR1);

	sigprocmask(SIG_BLOCK, &sigset, NULL);	
	while ((ser = connsocket(PORT, host)) == -2);
	if (ser != -1) {
		fp = fopen("metadata", "rb");
		if (fp != (FILE*)NULL) {
			while(fgets(metadata, 16384, fp))
				write(ser, metadata, strlen(metadata));
			fclose(fp);
		}
		close(ser);
	}
	sigprocmask(SIG_UNBLOCK, &sigset, NULL);
}

int connsocket(int port, char *host) {
	struct sockaddr_in s_in;
	struct hostent *he;
	int check;
	int ss;

	/* Create a socket */
	ss = socket(AF_INET, SOCK_STREAM, 0);
	if (ss < 0) return (-1);
#ifdef __NetBSD__
	s_in.sin_len = sizeof(struct sockaddr_in);
#endif
	s_in.sin_family = AF_INET;
	s_in.sin_port = htons(port);
	he = gethostbyname(host);
	if (he == (struct hostent*)NULL) {
		fprintf(stderr, "Couldnt convert %s\n", host);

		/* Close the socket - mikaelh */
		close(ss);

		return (-1);
	}
	memcpy(&s_in.sin_addr, he->h_addr_list[0], sizeof(struct in_addr));

	check = connect(ss, (struct sockaddr *)&s_in, sizeof(struct sockaddr_in));
	if (check != -1) {
		return (ss);
	}
	fprintf(stderr, "Couldnt connect to %s, %d\n", host, port);
	fprintf(stderr, "errno: %d\n", errno);

	/* Close the socket - mikaelh */
	close(ss);

	if (errno == 4) return (-2);
	return (-1);
}
