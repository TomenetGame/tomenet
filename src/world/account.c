#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#include "world.h"
#include "../common/defines.h"
#include "../account/account.h"
#include "externs.h"

uint32_t account_id;		/* Current account ID */

static uint32_t new_accid(void);
static char *t_crypt(char *inbuf, unsigned char *salt);
static int GetAccount(struct account *c_acc, unsigned char *name, char *pass);

void l_account(struct wpacket *wpk, struct client *ccl){
	struct pl_auth *login;
	struct account acc;
	login = &wpk->d.login;
	switch (login->stat) {
		case PL_INIT:
			login->stat = PL_FAIL;
			if (!strlen(login->pass) || !strlen(login->name))
				break;
			if (GetAccount(&acc, (unsigned char *)login->name, login->pass)) {
				login->stat = PL_OK;
			}
			break;
		default:
			fprintf(stderr, "Bad account packet from (%d)\n", ccl->fd);
			login->stat = PL_FAIL;
	}
	/* always reset this */
	memset(login->pass, '\0', sizeof(login->pass));
	reply(wpk, ccl);
}

static int GetAccount(struct account *c_acc, unsigned char *name, char *pass) {
	FILE *fp;
	long delpos = 0;

	fp = fopen("tomenet.acc", "rb+");
	if (fp == (FILE*)NULL) {
		if (errno == ENOENT) {	/* ONLY if non-existent */
			fp = fopen("tomenet.acc", "wb+");
			if (fp == (FILE*)NULL) {
				return 0;
			}
			fprintf(stderr, "Generated new account file\n");
		}
		else{
			return 0;	/* failed */
		}
	}
	while (!feof(fp)) {
		fread(c_acc, sizeof(struct account), 1, fp);
		if (c_acc->flags & ACC_DELD) {
			if (!delpos)
				delpos = ftell(fp) - sizeof(struct account);
			continue;
		}
		if (!strcmp((char *)c_acc->name, (char *)name)) {
			int val;
			if (pass == NULL)		/* direct name lookup */
				val = 0;
			else
				val = strcmp(c_acc->pass, t_crypt(pass, name));
			memset((char *)c_acc->pass, 0, 20);
			if (val) {
				fclose(fp);
				return 0;
			}
			fclose(fp);
			return 1;
		}
	}
	/* No account found. Create trial account */ 
	c_acc->id = new_accid();
	if (c_acc->id != 0L) {
		if (delpos)
			fseek(fp, delpos, SEEK_SET);
		c_acc->flags = (ACC_TRIAL | ACC_NOSCORE);
		strcpy(c_acc->name, (char *)name);
		strcpy(c_acc->pass, t_crypt(pass, name));
		fwrite(c_acc, sizeof(struct account), 1, fp);
	}
	memset((char *)c_acc->pass, 0, 20);
	fclose(fp);
	if (c_acc->id) return 1;
	return 0;
}

/* our password encryptor */
static char *t_crypt(char *inbuf, unsigned char *salt) {
#ifdef HAVE_CRYPT
	static char out[64];
	char setting[9];
	setting[0] = '_';
	strncpy(&setting[1], salt, 8);
	strcpy(out, (char*)crypt(inbuf, salt));
	return(out);
#else
	return(inbuf);
#endif
}

int GetAccountID(struct account *c_acc, uint32_t id) {
	FILE *fp;

	/* we may want to store a local index for fast
	   id/name/filepos lookups in the future */
	fp = fopen("tomenet.acc", "rb+");
	if (fp != (FILE*)NULL) {
		while (!feof(fp)) {
			fread(c_acc, sizeof(struct account), 1, fp);
			if (id == c_acc->id && !(c_acc->flags & ACC_DELD)) {
				memset((char *)c_acc->pass, 0, 20);
				fclose(fp);
				return 1;
			}
		}
		fclose(fp);
	}
	return 0;
}

static uint32_t new_accid() {
	uint32_t id;
	FILE *fp;
	char *t_map;
	struct account t_acc;
	id = account_id;
	fp = fopen("tomenet.acc", "rb");
	if (fp == (FILE*)NULL) return(0L);
	t_map = malloc(MAX_ACCOUNTS / 8);
	while (!feof(fp)) {
		if (fread(&t_acc, sizeof(struct account), 1, fp))
			t_map[t_acc.id / 8] |= (1 << (t_acc.id % 8));
	}
	fclose(fp);
	for (id = account_id; id < MAX_ACCOUNTS; id++) {
		if (!(t_map[id / 8] & (1 << (id % 8)))) break;
	}
	if (id == MAX_ACCOUNTS) {
		for (id = 1; id < account_id; id++) {
			if (!(t_map[id / 8] & (1 << (id % 8)))) break;
		}
		if (id == account_id) id = 0;
	}
	free(t_map);
	account_id = id + 1;

	return(id);	/* temporary */
}
