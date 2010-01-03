/* #include "files.h" */

/*
 * file transfer handling for tomenet - evileye
 * tcp already handles connection/errors, so we do
 * not need to worry about this.
 */

/* #include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> */

#include "angband.h"

extern errr path_build(char *buf, int max, cptr path, cptr file);

int remote_update(int ind, char *fname);
int check_return(int ind, unsigned short fnum, unsigned long int sum);
void kill_xfers(int ind);
void do_xfers(void);
int get_xfers_num(void);
int local_file_check(char *fname, unsigned long int *sum);
int local_file_ack(int ind, unsigned short fnum);
int local_file_err(int ind, unsigned short fnum);
int local_file_send(int ind, char *fname);
int local_file_init(int ind, unsigned short fnum, char *fname);
int local_file_write(int ind, unsigned short fnum, unsigned long len);
int local_file_close(int ind, unsigned short fnum);

extern const char *ANGBAND_DIR;

#define FS_READY 1	/* if ready for more data, set. */
#define FS_SEND 2	/* sending connection? */
#define FS_NEW 4	/* queued? */
#define FS_DONE 8	/* ready to send end packet on final ack */
#define FS_CHECK 16	/* file data block open for sum compare only */

#define MAX_TNF_SEND	256	/* maximum bytes in a transfer
				 * make this a power of 2 if possible
				 */

struct ft_data{
	struct ft_data *next;	/* next in list */
	unsigned short id;	/* unique xfer ID */
	int ind;		/* server security - who has this transfer */
	char fname[256];	/* actual filename */
	char tname[256];	/* temporary filename */
	FILE* fp;		/* FILE pointer */
	unsigned short state;	/* status of transfer */
	char buffer[MAX_TNF_SEND];
};

struct ft_data *fdata=NULL;	/* our pointer to the transfer data */

static struct ft_data *getfile(int ind, unsigned short fnum){
	struct ft_data *trav, *new_ft;
	if(fnum==0){
		new_ft=(struct ft_data*)malloc(sizeof(struct ft_data));
		if(new_ft==(struct ft_data*)NULL) return(NULL);
		memset(new_ft, 0, sizeof(struct ft_data));
		new_ft->next=fdata;
		fdata=new_ft;
		return(new_ft);
	}
	trav=fdata;
	while(trav){
		if(trav->id==fnum && trav->ind==ind){
			return(trav);
		}
		trav=trav->next;
	}
	return(NULL);
}

static void remove_ft(struct ft_data *d_ft){
	struct ft_data *trav;
	trav=fdata;
	if(trav==d_ft){
		fdata=trav->next;
		free(trav);
		return;
	}
	while(trav){
		if(trav->next==d_ft){
			trav->next=d_ft->next;
			free(d_ft);
			return;
		}
		trav=trav->next;
	}
}

/* server and client need to be synchronised on this
   but for now, clashes are UNLIKELY in the extreme
   so they can generate their own. */
static int new_fileid(){
	static int c_id=0;
	c_id++;
	if(!c_id) c_id=1;
	return(c_id);
}

/* acknowledge recipient ready to receive more */
int local_file_ack(int ind, unsigned short fnum){
	struct ft_data *c_fd;
	c_fd=getfile(ind, fnum);
	if(c_fd==(struct ft_data*)NULL) return(0);
	if(c_fd->state&FS_SEND){
		c_fd->state|=FS_READY;
	}
	if(c_fd->state&FS_DONE){
		fclose(c_fd->fp);
		remove_ft(c_fd);
	}
	return(1);
}

/* for now, just kill the connection completely */
int local_file_err(int ind, unsigned short fnum){
	struct ft_data *c_fd;
	c_fd=getfile(ind, fnum);
	if(c_fd==(struct ft_data*)NULL) return(0);
	fclose(c_fd->fp);	/* close the file */
	remove_ft(c_fd);
	return(1);
}

/* initialise an file to send */
int local_file_send(int ind, char *fname){
	struct ft_data *c_fd;
	FILE* fp;
	char buf[256];

	c_fd=getfile(ind, 0);
	if(c_fd==(struct ft_data*)NULL) return(0);
	path_build(buf, 256, ANGBAND_DIR, fname);
	fp=fopen(buf, "rb");
	if(!fp) return(0);
	c_fd->fp=fp;
	c_fd->ind=ind;
	c_fd->id=new_fileid();	/* ALWAYS succeed */
	c_fd->state=(FS_SEND|FS_NEW);
	strncpy(c_fd->fname, fname, 255);
	Send_file_init(c_fd->ind, c_fd->id, c_fd->fname);
	return(1);
}

/* request checksum of remote file */
int remote_update(int ind, char *fname){
	struct ft_data *c_fd;
	c_fd=getfile(ind, 0);
	if(c_fd==(struct ft_data*)NULL) return(0);
	c_fd->ind=ind;
	c_fd->id=new_fileid();	/* ALWAYS succeed */
	c_fd->state=(FS_CHECK);
	strncpy(c_fd->fname, fname, 255);
	Send_file_check(c_fd->ind, c_fd->id, c_fd->fname);
	return(1);
}

/* compare checksums of local/remote files - update if
   necessary */
int check_return(int ind, unsigned short fnum, unsigned long int sum){
	struct ft_data *c_fd;
	FILE* fp;
	unsigned long int lsum;
	char buf[256];

	c_fd=getfile(ind, fnum);
	if(c_fd==(struct ft_data*)NULL){
		return(0);
	}
	local_file_check(c_fd->fname, &lsum);
	if (!(c_fd->state & FS_CHECK)) {
		return(0);
	}
	if(lsum!=sum){
		path_build(buf, 256, ANGBAND_DIR, c_fd->fname);
		fp=fopen(buf, "rb");
		if(!fp) {
			remove_ft(c_fd);
			return(0);
		}
		c_fd->fp=fp;
		c_fd->ind=ind;
		c_fd->state=(FS_SEND|FS_NEW);
		Send_file_init(c_fd->ind, c_fd->id, c_fd->fname);
		return(1);
	}
	remove_ft(c_fd);
	return(1);
}

void kill_xfers(int ind){
	struct ft_data *trav, *next;
	trav=fdata;
	for(; trav; trav=next){
		next=trav->next;
		if(!trav->id) continue;
		if(trav->ind==ind){
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
	trav=fdata;
	for(; trav; trav=trav->next){
		if(!trav->id) continue;	/* non existent */
		if(!(trav->state&FS_SEND)) continue; /* wrong type */
		if(!(trav->state&FS_READY)) continue; /* not ready */
		x=fread(trav->buffer, 1, MAX_TNF_SEND, trav->fp);
		if(!(trav->state&FS_DONE)){
			if(x==0){
				trav->state|=FS_DONE;
			}
			else{
				Send_file_data(trav->ind, trav->id, trav->buffer, x);
				trav->state&=~(FS_READY);
			}
		}
		else{
			Send_file_end(trav->ind, trav->id);
			trav->state&=~(FS_READY);
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

/*
 * Silly windows
 * DarkGod slaps windows
 */
/* Hmm... at least Cygwin seems to have this? - mikaelh */
#ifndef CYGWIN
#ifdef WIN32
int mkstemp(char *template)
{
	char f[256], *fp;	/* was 30 if it overflows, it overflows.. */
	int fd;

#ifndef __MINGW32__
	if (!tmpnam(f)) return -1;
#else /* on Windows, we'd just get files on C:\ from tmpnam. */
//	if (!(fp = tempnam("./lib/temp", "L")) return -1; /* have files start with an L, like LUA ;) */
	path_build(f, 1024, ANGBAND_DIR, "");
	if (!(fp = tempnam(f, "tomenet_"))) return -1; /* have temporary files start with tomenet_ */
	strcpy(f, fp);
#endif
	fd = open(f, O_RDWR | O_CREAT);
	strcpy(template, f);	/* give back our filename */
	return fd;
}
#endif
#endif

/* Open file for receive/writing */
int local_file_init(int ind, unsigned short fnum, char *fname){
	struct ft_data *c_fd;
	int fd;
	char tname[256] = "tomexfer.XXXXXX";
	c_fd=getfile(ind, 0);		/* get empty space */
	if(c_fd==(struct ft_data*)NULL) return(0);

	if(fname[0]=='/') return(0);	/* lame security */
	if(strstr(fname, "..")) return(0);

	fd=mkstemp(tname);
	c_fd->fp=fdopen(fd, "wb+");
	c_fd->state=FS_READY;
	if(c_fd->fp){
#ifndef __MINGW32__
		unlink(tname);		/* don't fill up /tmp */
#endif
		c_fd->id=fnum;
		c_fd->ind=ind;	/* not really needed for client */
		strncpy(c_fd->fname, fname, 255);
		strncpy(c_fd->tname, tname, 255);
		return(1);
	}
	remove_ft(c_fd);
	return(0);
}



/* Write received data to temporary file */
int local_file_write(int ind, unsigned short fnum, unsigned long len){
	struct ft_data *c_fd;
	c_fd = getfile(ind, fnum);
	if(c_fd == (struct ft_data*) NULL) return(0);
	if (Receive_file_data(ind, len, c_fd->buffer) == 0) {
		/* Not enough data available */
		return -1;
	}
	fwrite(&c_fd->buffer, len, 1, c_fd->fp);
	return(1);
}

/* Close file and make the changes */
int local_file_close(int ind, unsigned short fnum){
	int x;
	struct ft_data *c_fd;
	char buf[4096];
	int size=4096;
	int success=0;
	FILE *wp;
	c_fd=getfile(ind, fnum);
	if(c_fd==(struct ft_data*)NULL) return(0);

	path_build(buf, 4096, ANGBAND_DIR, c_fd->fname);

	wp=fopen(buf, "wb");	/* b for windows */
	if(wp){
		fseek(c_fd->fp, 0, SEEK_SET);
		do{
			x=fread(buf, 1, size, c_fd->fp);
			fwrite(buf, x, 1, wp);
		}while(x>0);
		fclose(wp);
		success=1;
	}
	fclose(c_fd->fp);	/* close & remove temp file */
#ifdef __MINGW32__
	unlink(c_fd->tname);	/* remove it on Windows OS */
#endif
	remove_ft(c_fd);
	return(success);
}

unsigned long int total;

/* uses adler checksum now - (client/server) compat essential */
static void do_sum(unsigned char *buffer, int size){
	unsigned long int s1, s2;
	int n;

	s1 = total & 0xffff;
	s2 = (total >> 16) & 0xffff;

	for(n=0; n<size; n++){
		s1=(s1+buffer[n]) % 65521;
		s2=(s1+s1) % 65521;
	}
	total=(s2 << 16)+s1;
}

/* Get checksum of file */
/* don't waste a file transfer data space locally */
int local_file_check(char *fname, unsigned long int *sum){
	FILE *fp;
	unsigned char *buffer;
	int success=0;
	int size=4096;
	unsigned long tbytes=0;
	unsigned long pos;
	char buf[256];

	path_build(buf, 256, ANGBAND_DIR, fname);

	fp=fopen(buf, "rb");	/* b for windows.. */
	if(!fp){
		return(0);
	}
	buffer=(unsigned char*)malloc(size);
	if(buffer){
		total=1L;
		while(size){
			pos=ftell(fp);
			if(fread(buffer, size, 1, fp)){
				tbytes+=size;
				do_sum(buffer, size);
			}
			else{
				if(feof(fp)){
					fseek(fp, pos, SEEK_SET);
					size/=2;
				}
				else if(ferror(fp))
					break;
			}
		}
		if(!size) success=1;
		free(buffer);
	}
	fclose(fp);
	*sum=total;
	return(success);
}
