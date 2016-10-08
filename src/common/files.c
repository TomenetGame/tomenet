/* #include "files.h" */

/*
 * file transfer handling for tomenet - evileye
 * tcp already handles connection/errors, so we do
 * not need to worry about this.
 */

#ifdef WIN32
#include <winsock.h> /* For htonl and ntohl */
#else
#include <arpa/inet.h> /* For htonl and ntohl */
#endif

#include "angband.h"
#include "md5.h"

extern errr path_build(char *buf, int max, cptr path, cptr file);

extern int remote_update(int ind, char *fname, unsigned short chunksize);
extern int check_return(int ind, unsigned short fnum, u32b sum, int Ind);
extern int check_return_new(int ind, unsigned short fnum, const unsigned char digest[16], int Ind);
extern void kill_xfers(int ind);
extern void do_xfers(void);
extern int get_xfers_num(void);
extern int local_file_check(char *fname, u32b *sum);
extern int local_file_check_new(char *fname, unsigned char digest_out[16]);
extern int local_file_ack(int ind, unsigned short fnum);
extern int local_file_err(int ind, unsigned short fnum);
//extern int local_file_send(int ind, char *fname, unsigned short chunksize);
extern int local_file_init(int ind, unsigned short fnum, char *fname);
extern int local_file_write(int ind, unsigned short fnum, unsigned long len);
extern int local_file_close(int ind, unsigned short fnum);
extern void md5_digest_to_bigendian_uint(unsigned digest_out[4], const unsigned char digest[16]);
extern void md5_digest_to_char_array(unsigned char digest_out[16], const unsigned digest[4]);

extern const char *ANGBAND_DIR;

#define FS_READY 1	/* if ready for more data, set. */
#define FS_SEND 2	/* sending connection? */
#define FS_NEW 4	/* queued? */
#define FS_DONE 8	/* ready to send end packet on final ack */
#define FS_CHECK 16	/* file data block open for sum compare only */

#define MAX_TNF_SEND	1024	/* maximum bytes in a transfer
				 * make this a power of 2 if possible
				 */

#define OLD_TNF_SEND	256	/* old value of MAX_TNF_SEND */

struct ft_data {
	char buffer[MAX_TNF_SEND];
	char fname[256];	/* actual filename */
	char tname[256];	/* temporary filename */
	struct ft_data *next;	/* next in list */
	FILE* fp;		/* FILE pointer */
	int ind;		/* server security - who has this transfer */
	unsigned short id;	/* unique xfer ID */
	unsigned short state;	/* status of transfer */
	unsigned short chunksize; /* old clients can handle only 256 bytes of data in one packet */
};

struct ft_data *fdata = NULL;	/* our pointer to the transfer data */

static struct ft_data *getfile(int ind, unsigned short fnum){
	struct ft_data *trav, *new_ft;
	if (fnum == 0) {
		new_ft = (struct ft_data*)malloc(sizeof(struct ft_data));
		if (new_ft == (struct ft_data*)NULL) return(NULL);
		memset(new_ft, 0, sizeof(struct ft_data));
		new_ft->next = fdata;
		fdata = new_ft;
		return(new_ft);
	}
	trav = fdata;
	while (trav) {
		if (trav->id == fnum && trav->ind == ind) {
			return(trav);
		}
		trav = trav->next;
	}
	return(NULL);
}

static void remove_ft(struct ft_data *d_ft) {
	struct ft_data *trav;
	trav = fdata;
	if (trav == d_ft) {
		fdata = trav->next;
		free(trav);
		return;
	}
	while (trav) {
		if (trav->next == d_ft) {
			trav->next = d_ft->next;
			free(d_ft);
			return;
		}
		trav = trav->next;
	}
}

/* server and client need to be synchronised on this
   but for now, clashes are UNLIKELY in the extreme
   so they can generate their own. */
static int new_fileid() {
	static int c_id = 0;
	c_id++;
	if (!c_id) c_id = 1;
	return(c_id);
}

/* Hack: Replace a path aimed at ANGBAND_DIR+user or ANGBAND_DIR+scpt by ANGBAND_DIR_USER and ANGBAND_DIR_SCPT respectively.
   The reason we need to do this is that on Windows clients those two folders might not be in the TomeNET lib folder but
   actually in the OS home path of that user. This function is only really does anything useful if WINDOWS_USER_HOME is defined and used.
   Returns TRUE if newpath has been rebuilt and is therefore ready to use. */
static bool client_user_path(char *newpath, cptr oldpath) {
#ifndef WINDOWS
	strcpy(newpath, oldpath);
	return FALSE;
#endif

	if (!is_client_side) {
		strcpy(newpath, oldpath);
		return FALSE;
	}

	/* replace ANGBAND_DIR by ANGBAND_DIR_SCPT/USER, as those might be in the user's home folder on Windows, instead of in the TomeNET folder. */
	if (!strncmp(oldpath, "scpt/", 5)) {
		strcpy(newpath, ANGBAND_DIR_SCPT);
		strcat(newpath, "\\");
		strcat(newpath, oldpath + 5);
		return TRUE;
	} else if (!strncmp(oldpath, "user/", 5)) {
		strcpy(newpath, ANGBAND_DIR_USER);
		strcat(newpath, "\\");
		strcat(newpath, oldpath + 5);
		return TRUE;
	}

	strcpy(newpath, oldpath);
	return FALSE;
}

/* acknowledge recipient ready to receive more */
int local_file_ack(int ind, unsigned short fnum) {
	struct ft_data *c_fd;
	c_fd = getfile(ind, fnum);
	if (c_fd == (struct ft_data*)NULL) return(0);
	if (c_fd->state&FS_SEND) {
		c_fd->state |= FS_READY;
	}
	if (c_fd->state & FS_DONE) {
		fclose(c_fd->fp);
		remove_ft(c_fd);
	}
	return(1);
}

/* for now, just kill the connection completely */
int local_file_err(int ind, unsigned short fnum) {
	struct ft_data *c_fd;
	c_fd = getfile(ind, fnum);
	if (c_fd == (struct ft_data*)NULL) return(0);
	fclose(c_fd->fp);	/* close the file */
	remove_ft(c_fd);
	return(1);
}

#if 0 /* unused */
/* initialise an file to send */
int local_file_send(int ind, char *fname, unsigned short chunksize) {
	struct ft_data *c_fd;
	FILE* fp;
	char buf[256];

	c_fd = getfile(ind, 0);
	if (c_fd == (struct ft_data*)NULL) return(0);
	if (!client_user_path(buf, fname))
		path_build(buf, sizeof(buf), ANGBAND_DIR, fname);
	fp = fopen(buf, "rb");
	if (!fp) return(0);
	c_fd->fp = fp;
	c_fd->ind = ind;
	c_fd->id = new_fileid();	/* ALWAYS succeed */
	c_fd->state = (FS_SEND|FS_NEW);
	c_fd->chunksize = chunksize;
	strncpy(c_fd->fname, fname, 255);
	Send_file_init(c_fd->ind, c_fd->id, c_fd->fname);
	return(1);
}
#endif

/* request checksum of remote file */
int remote_update(int ind, char *fname, unsigned short chunksize) {
	struct ft_data *c_fd;
	c_fd = getfile(ind, 0);
	if (c_fd == (struct ft_data*)NULL) return(0);
	c_fd->ind = ind;
	c_fd->id = new_fileid();	/* ALWAYS succeed */
	c_fd->state = (FS_CHECK);
	c_fd->chunksize = chunksize;
	strncpy(c_fd->fname, fname, 255);
	Send_file_check(c_fd->ind, c_fd->id, c_fd->fname);
	return(1);
}

/* compare checksums of local/remote files - update if
   necessary */
int check_return(int ind, unsigned short fnum, u32b sum, int Ind) {
	struct ft_data *c_fd;
	FILE* fp;
	u32b lsum;
	char buf[256];

	/* check how many LUA files were already checked (for 4.4.8.1.0.0 crash bug) */
	if (Ind) Players[Ind]->warning_lua_count--;

	c_fd = getfile(ind, fnum);
	if (c_fd == (struct ft_data*)NULL) {
		return(0);
	}
	//client_user_path(tmpname, c_fd->fname);
	local_file_check(c_fd->fname, &lsum);
	if (!(c_fd->state & FS_CHECK)) {
		return(0);
	}
	if (lsum != sum) {
		/* Hack: Client 4.4.8.1 apparently was compiled by a MinGW version
		   that broke lua updating, resulting in a client crash on logon.
		   So skip any lua updates for players using that version.
		   That means that players playing spell-casters that use spells in
		   the affected LUA files will probably have to update their client
		   at some point. */
		if (Ind && is_same_as(&Players[Ind]->version, 4, 4, 8, 1, 0, 0)) {
			/* Don't give him messages if he cannot help it */
			if (!Players[Ind]->v_latest) {
				Players[Ind]->warning_lua_update = 1;
				msg_format(Ind, "\377oLUA file %s is outdated.", c_fd->fname);
			}

			return 0;
		}

		if (!client_user_path(buf, c_fd->fname))
			path_build(buf, sizeof(buf), ANGBAND_DIR, c_fd->fname);
		fp = fopen(buf, "rb");
		if (!fp) {
			remove_ft(c_fd);
			return(0);
		}
		c_fd->fp = fp;
		c_fd->ind = ind;
		c_fd->state = (FS_SEND | FS_NEW);
		//client_user_path(tmpname, c_fd->fname);
		Send_file_init(c_fd->ind, c_fd->id, c_fd->fname);
		return(1);
	}
	remove_ft(c_fd);
	return(1);
}

/*
 * New version with support for MD5 checksums
 */
int check_return_new(int ind, unsigned short fnum, const unsigned char digest[16], int Ind) {
	struct ft_data *c_fd;
	FILE* fp;
	unsigned char local_digest[16];
	char buf[256];

	/* check how many LUA files were already checked (for 4.4.8.1.0.0 crash bug) */
	if (Ind) Players[Ind]->warning_lua_count--;

	c_fd = getfile(ind, fnum);
	if (c_fd == (struct ft_data*)NULL) {
		return(0);
	}
	//client_user_path(tmpname, c_fd->fname);
	local_file_check_new(c_fd->fname, local_digest);
	if (!(c_fd->state & FS_CHECK)) {
		return(0);
	}
	if (memcmp(digest, local_digest, 16) != 0) {
		/* Hack: Client 4.4.8.1 apparently was compiled by a MinGW version
		   that broke lua updating, resulting in a client crash on logon.
		   So skip any lua updates for players using that version.
		   That means that players playing spell-casters that use spells in
		   the affected LUA files will probably have to update their client
		   at some point. */
		if (Ind && is_same_as(&Players[Ind]->version, 4, 4, 8, 1, 0, 0)) {
			/* Don't give him messages if he cannot help it */
			if (!Players[Ind]->v_latest) {
				Players[Ind]->warning_lua_update = 1;
				msg_format(Ind, "\377oLUA file %s is outdated.", c_fd->fname);
			}

			return 0;
		}

		if (!client_user_path(buf, c_fd->fname))
			path_build(buf, sizeof(buf), ANGBAND_DIR, c_fd->fname);
		fp = fopen(buf, "rb");
		if (!fp) {
			remove_ft(c_fd);
			return(0);
		}
		c_fd->fp = fp;
		c_fd->ind = ind;
		c_fd->state = (FS_SEND | FS_NEW);
		//client_user_path(tmpname, c_fd->fname);
		Send_file_init(c_fd->ind, c_fd->id, c_fd->fname);
		return(1);
	}
	remove_ft(c_fd);
	return(1);
}

void kill_xfers(int ind){
	struct ft_data *trav, *next;
	trav = fdata;
	for (; trav; trav = next) {
		next = trav->next;
		if (!trav->id) continue;
		if (trav->ind == ind) {
			if (trav->fp) fclose(trav->fp);
			remove_ft(trav);
		}
	}
}

/* handle all current SEND type file transfers */
/* laid out long like this for testing. DO NOT CHANGE */
void do_xfers(){
	int x;
	struct ft_data *trav;
	trav = fdata;
	unsigned short chunksize;
	for (; trav; trav = trav->next) {
		if (!trav->id) continue;	/* non existent */
		if (!(trav->state & FS_SEND)) continue; /* wrong type */
		if (!(trav->state & FS_READY)) continue; /* not ready */
		chunksize = trav->chunksize;
		/* Sanity check */
		if (!chunksize) {
			fprintf(stderr, "Error: trav->chunksize should not be 0 in do_xfers\n");
			trav->chunksize = chunksize = OLD_TNF_SEND;
		}
		x = fread(trav->buffer, 1, chunksize, trav->fp);
		if (!(trav->state & FS_DONE)) {
			if (x == 0) {
				trav->state |= FS_DONE;
			}
			else {
				Send_file_data(trav->ind, trav->id, trav->buffer, x);
				trav->state &= ~(FS_READY);
			}
		}
		else {
			Send_file_end(trav->ind, trav->id);
			trav->state &= ~(FS_READY);
		}
	}
}

/*
 * Return the number of file transfers in progress
 * Can be used to check if all file transfers are complete.
 */
int get_xfers_num() {
	int num = 0;
	struct ft_data *trav;
	trav = fdata;
	for (; trav; trav = trav->next) {
		num++;
	}
	return num;
}

#ifdef WIN32
/*
 * Create a temporary file on Windows. Code borrowed from ToME.
 */
FILE *ftmpopen(char *template) {
	static u32b tmp_counter;
	static char valid_characters[] =
			"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char f[256];
	FILE *fp;
	char rand_ext[4];

	rand_ext[0] = valid_characters[rand_int(sizeof (valid_characters))];
	rand_ext[1] = valid_characters[rand_int(sizeof (valid_characters))];
	rand_ext[2] = valid_characters[rand_int(sizeof (valid_characters))];
	rand_ext[3] = '\0';
	strnfmt(f, sizeof(f), "%s/xfer_%ud.%s", ANGBAND_DIR, tmp_counter, rand_ext);
	tmp_counter++;

	fp = fopen(f, "wb+");
	strcpy(template, f);	/* give back our filename */
	return fp;
}
#endif

/* Open file for receive/writing */
int local_file_init(int ind, unsigned short fnum, char *fname) {
	struct ft_data *c_fd;
#ifndef WIN32
	int fd;
#endif
	char tname[256] = "tomexfer.XXXXXX";
	//char tmpname[256];

	c_fd = getfile(ind, 0);		/* get empty space */
	if (c_fd == (struct ft_data*)NULL) return(0);

	if (fname[0] == '/') return(0);	/* lame security */
	if (strstr(fname, "..")) return(0);

#ifndef WIN32
	fd = mkstemp(tname);
	c_fd->fp = fdopen(fd, "wb+");
#else
	c_fd->fp = ftmpopen(tname);
#endif
	c_fd->state = FS_READY;
	c_fd->chunksize = OLD_TNF_SEND;	/* doesn't matter when receiving, use the old value of MAX_TNF_SEND */
	if (c_fd->fp) {
#ifndef __MINGW32__
		unlink(tname);		/* don't fill up /tmp */
#endif
		c_fd->id = fnum;
		c_fd->ind = ind;	/* not really needed for client */
		//client_user_path(tmpname, c_fd->fname);
		strncpy(c_fd->fname, fname, 255);
		strncpy(c_fd->tname, tname, 255);
		return(1);
	}
	remove_ft(c_fd);
	return(0);
}



/* Write received data to temporary file */
int local_file_write(int ind, unsigned short fnum, unsigned long len) {
	struct ft_data *c_fd;
	c_fd = getfile(ind, fnum);
	if (c_fd == (struct ft_data*) NULL) return(0);
	if (Receive_file_data(ind, len, c_fd->buffer) == 0) {
		/* Not enough data available */
		return -1;
	}
	if (fwrite(&c_fd->buffer, 1, len, c_fd->fp) < len) {
		fprintf(stderr, "Error: fwrite failed in local_file_write\n");
		fclose(c_fd->fp);	/* close & remove temp file */
#ifdef __MINGW32__
		unlink(c_fd->tname);	/* remove it on Windows OS */
#endif
		remove_ft(c_fd);
		return -2;
	}
	return(1);
}

/* Close file and make the changes */
int local_file_close(int ind, unsigned short fnum) {
	size_t bytes;
	struct ft_data *c_fd;
	char buf[4096];
	int size = 4096;
	int success = 1;
	FILE *wp;
	c_fd = getfile(ind, fnum);
	if(c_fd == (struct ft_data *) NULL) return 0;

	if (!client_user_path(buf, c_fd->fname))
		path_build(buf, 4096, ANGBAND_DIR, c_fd->fname);

	wp = fopen(buf, "wb");	/* b for windows */
	if (wp) {
		fseek(c_fd->fp, 0, SEEK_SET);

		while ((bytes = fread(buf, 1, size, c_fd->fp)) > 0) {
			if (fwrite(buf, 1, bytes, wp) < bytes) {
				fprintf(stderr, "Error: fwrite failed in local_file_close\n");
				success = 0;
				break;
			}
		}

		fclose(wp);
	} else {
		success = 0;
	}

	fclose(c_fd->fp);	/* close & remove temp file */
#ifdef __MINGW32__
	unlink(c_fd->tname);	/* remove it on Windows OS */
#endif
	remove_ft(c_fd);

	return success;
}

/* 
 * This is a broken version of Adler-32.
 */
static void do_sum(const unsigned char *buffer, size_t bytes_read, u32b *total_ptr) {
	u32b s1, s2;
	size_t idx;

	s1 = *total_ptr & 0xffff;
	s2 = (*total_ptr >> 16) & 0xffff;

#if 0
	for (n = 0; n < size; n++) {
		s1 = (s1 + buffer[n]) % 65521;
		s2 = (s1 + s1) % 65521; /* This is WRONG - mikaelh */
	}
#else
	/* Optimized version of the broken implementation above - mikaelh */
	for (idx = 0; idx < bytes_read; idx++) {
		s1 += buffer[idx];
	}

	if (idx) {
		s2 = (s1 + s1) % 65521;
		s1 %= 65521;
	}
#endif

	*total_ptr = (s2 << 16) | s1;
}

/* Get checksum of file */
/* don't waste a file transfer data space locally */
int local_file_check(char *fname, u32b *sum) {
	FILE *fp;
	size_t bytes_read;
	u32b total = 1;
	int success = 0; /* 0 = success, 1 = failure */
	char buffer[4096];
	char pathbuf[256];

	if (!client_user_path(pathbuf, fname))
		path_build(pathbuf, sizeof(pathbuf), ANGBAND_DIR, fname);

	fp = fopen(pathbuf, "rb");	/* b for windows.. */
	if (!fp) {
		*sum = 0;
		return 1;
	}

	while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
		do_sum((unsigned char *) buffer, bytes_read, &total);
	}

	/* Check for read failure */
	if (!ferror(fp)) {
		success = 1;
	}

	fclose(fp);
	*sum = total;
	return success;
}

/*
 * New MD5-based checksum function for files
 */
int local_file_check_new(char *fname, unsigned char digest_out[16]) {
	FILE *fp;
	size_t bytes_read;
	int success = 0; /* 0 = success, 1 = failure */
	char buffer[4096];
	char pathbuf[256];
	MD5_CTX ctx;

	if (!client_user_path(pathbuf, fname))
		path_build(pathbuf, sizeof(pathbuf), ANGBAND_DIR, fname);

	fp = fopen(pathbuf, "rb");	/* b for windows.. */
	if (!fp) {
		memset(digest_out, 0, 16);
		return 1;
	}

	MD5Init(&ctx);

	while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
		MD5Update(&ctx, (unsigned char *) buffer, bytes_read);
	}

	MD5Final(digest_out, &ctx);

	/* Check for read failure */
	if (!ferror(fp)) {
		success = 1;
	}

	fclose(fp);
	return success;
}

/*
 * Convert MD5 checksum stored from 'unsigned char[16]' to 'unsigned[4]' so it
 * can be transmitted over the network.
 */
void md5_digest_to_bigendian_uint(unsigned digest_out[4], const unsigned char digest[16]) {
	const unsigned *digest_in = (const unsigned *) digest;
	digest_out[0] = htonl(digest_in[0]);
	digest_out[1] = htonl(digest_in[1]);
	digest_out[2] = htonl(digest_in[2]);
	digest_out[3] = htonl(digest_in[3]);
}

/*
 * Inverse of the above.
 */
void md5_digest_to_char_array(unsigned char digest_out[16], const unsigned digest[4]) {
	unsigned *digest_out2 = (unsigned *) digest_out;
	digest_out2[0] = ntohl(digest[0]);
	digest_out2[1] = ntohl(digest[1]);
	digest_out2[2] = ntohl(digest[2]);
	digest_out2[3] = ntohl(digest[3]);
}
