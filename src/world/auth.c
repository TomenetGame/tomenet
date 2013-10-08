/* random password generator */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "world.h"
#include "externs.h"

char salt[21];

uint32_t chk(unsigned char *s1, unsigned char *s2);
char *rpgen(char *dest);

void initrand(){
	time_t now;
	unsigned int x;
	time(&now);
	x = now;
	srandom(x);
}

/* Send authentication request to game server */
void initauth(struct client *ccl) {
	struct wpacket spk;
	int len = sizeof(struct wpacket);
	spk.type = WP_AUTH;
	rpgen(spk.d.auth.pass);
	send(ccl->fd, &spk, len, 0);
}

/* Generate a random password */
char *rpgen(char *dest) {
	int i = 0;
	int x;
	for (i = 0; i < 20; i++) {
		x = random();
		salt[i] = 58 + (x & 0x3f);
	}
	salt[20] = '\0';
	fprintf(stderr, "(rpgen..");
	if (dest != (char*)NULL) {
		strcpy(dest, salt);
		fprintf(stderr, "dest)\n");
		return(dest);
	}
	fprintf(stderr, "salt)\n");
	return(salt);
}

/* return server number, or -1 on failure */
short pwcheck(char *cpasswd, uint32_t val) {
	int i;
	fprintf(stderr, "authing..\n");
	for (i = 0; i < snum; i++) {
		if (val == chk((unsigned char *)slist[i].pass, (unsigned char *)cpasswd)) {
			printf("auth success\n"); 
			return(i + 1);
		}
	}
	fprintf(stderr, "server failed authentication\n");
	return(-1);
}

/* unified, hopefully unique password check function */
uint32_t chk(unsigned char *s1, unsigned char *s2) {
	unsigned int i, j = 0;
	int m1, m2;
	static uint32_t rval[2] = {0, 0};
	rval[0] = 0L;
	rval[1] = 0L;
	m1 = strlen((char *)s1);
	m2 = strlen((char *)s2);
	for (i = 0; i < m1; i++) {
		rval[0] += s1[i];
		rval[0] <<= 5;
	}
	for (j = 0; j < m2; j++) {
		rval[1] += s2[j];
		rval[1] <<= 3;
	}
	for (i = 0; i < m1; i++) {
		rval[1] += s1[i];
		rval[1] <<= (3 + rval[0] % 5);
		rval[0] += s2[j];
		j = (unsigned int)rval[0] % m2;
		rval[0] <<= (3 + rval[1] % 3);
	}
	return(rval[0] + rval[1]);
}
