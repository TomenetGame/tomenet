/* evileye meta client - 2003*/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <netinet/in.h>
#include <netdb.h>

#include <errno.h>

#define PORT 8800
#define HOST "meta.tomenet.net"

void callup(int val);
int connsocket(int port, char *host);
void callmeta(void);

char *host=HOST;

char metadata[16384];

int quit=0;

int main(int argc, char *argv[]){
	if(argc==2) host=argv[1];
	printf("Evileye meta client started\n");
	callmeta();
	/* ok so its not sigaction... */
	signal(SIGINT, SIG_IGN);
	signal(SIGUSR1, callup);
	signal(SIGTERM, callup);
	while(!quit){
		sleep(15);
		/* call the meta */
		callmeta();
	}
}

/* the signal handler */
void callup(int val){
	if(val==SIGTERM) quit=1;
	return;
}

/* signalled, so we call the meta */
void callmeta(){
	int ser;
	FILE *fp;
	while((ser=connsocket(PORT, host))==-2);
	if(quit) fprintf(stderr, "Meta client closing\n");
	if(ser!=-1){
		fp=fopen("metadata", "r");
		if(fp!=(FILE*)NULL){
			while(fgets(metadata, 16384, fp))
				write(ser, metadata, strlen(metadata));
			fclose(fp);
		}
		close(ser);
	}
}

int connsocket(int port, char *host){
	int option=1;
	struct sockaddr_in s_in;
	struct hostent *he;
	int check;
	int ss;

	/* Create a socket */
	ss=socket(AF_INET, SOCK_STREAM, 0);
	if(ss < 0) return(-1);
	s_in.sin_len=sizeof(struct sockaddr_in);
	s_in.sin_family=AF_INET;
	s_in.sin_port=htons(port);
	he=gethostbyname(host);
	if(he==(struct hostent*)NULL){
		fprintf(stderr, "Couldnt convert %s\n", host);
		return(-1);
	}
	memcpy(&s_in.sin_addr, he->h_addr_list[0], sizeof(struct in_addr));

	check=connect(ss, (struct sockaddr *)&s_in, sizeof(struct sockaddr_in));
	if(check != -1){
		return(ss);
	}
	fprintf(stderr, "Couldnt connect to %s, %d\n", host, port);
	fprintf(stderr, "errno: %d\n", errno);
	if(errno==4) return(-2);
	return(-1);
}
