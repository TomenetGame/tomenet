/* random password generator */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "world.h"
#include "externs.h"

char salt[3];
char crypted[40];

void initrand(){
	time_t now;
	long x;
	time(&now);
	x=now;
	srandom(x);
}

void initauth(struct client *ccl){
	struct wpacket spk;
	int x, len=sizeof(struct wpacket);
	spk.type=WP_AUTH;
	rpgen(spk.d.auth.pass);
	x=send(ccl->fd, &spk, len, 0);
}


char *rpgen(char *dest){
	int i=0;
	long x;
	while(i<2){
		x=random();
		salt[i]=58 + (x & 0x3f);
		if(salt[i]!='_') i++;
	}
	salt[2]='\0';
	if(dest!=(char*)NULL){
		strcpy(dest, salt);
		return(dest);
	}
	return(salt);
}

/* return server number, or -1 on failure */
short pwcheck(char *cpasswd){
	int i;
	for(i=0; i<snum; i++){
		if(!strcmp(cpasswd, crypt(slist[i].pass, cpasswd)))
			return(i+1);
	}
	fprintf(stderr, "server failed authentication\n");
	return(-1);
}
