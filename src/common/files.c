/* #include "files.h" */

/*
 * file transfer handling for tomenet - evileye
 * tcp already handles connection/errors, so we do
 * not need to worry about this.
 */

/*
 * TODO: i shall implement a linked list for file
 * transfers, because an array is totally unsuitable
 * due to the dynamic nature.
 */

#include <stdio.h>
#include <fcntl.h>

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
	char fname[30];		/* actual filename */
	int fd;			/* file descriptor */
	unsigned short state;	/* status of transfer */
	char buffer[MAX_TNF_SEND];
};

struct ft_data *fdata=NULL;	/* our pointer to the transfer data */

struct ft_data *getfile(int ind, unsigned short fnum){
	struct ft_data *trav, *new_ft;
	if(fnum==0){
		new_ft=(struct ft_data*)malloc(sizeof(struct ft_data));
		memset(new_ft, 0, sizeof(struct ft_data));
		if(new_ft==(struct ft_data*)NULL) return(NULL);
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

void remove_ft(struct ft_data *d_ft){
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

int new_fileid(){
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
		close(c_fd->fd);
		remove_ft(c_fd);
	}
	return(1);
}

/* for now, just kill the connection completely */
int local_file_err(int ind, unsigned short fnum){
	struct ft_data *c_fd;
	c_fd=getfile(ind, fnum);
	if(c_fd==(struct ft_data*)NULL) return(0);
	close(c_fd->fd);	/* close the file */
	remove_ft(c_fd);
	return(1);
}

/* initialise an file to send */
int local_file_send(int ind, char *fname){
	struct ft_data *c_fd;
	int fd;
	char buf[1024];

	c_fd=getfile(ind, 0);
	if(c_fd==(struct ft_data*)NULL) return(0);
	path_build(buf, 1024, ANGBAND_DIR, fname);
	fd=open(buf, O_RDONLY);
	if(fd==-1) return(0);
	c_fd->fd=fd;
	c_fd->ind=ind;
	c_fd->id=new_fileid();	/* ALWAYS succeed */
	c_fd->state=(FS_SEND|FS_NEW);
	strncpy(c_fd->fname, fname, 30);
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
	strncpy(c_fd->fname, fname, 30);
	Send_file_check(c_fd->ind, c_fd->id, c_fd->fname);
	return(1);
}

/* compare checksums of local/remote files - update if
   necessary */
int check_return(int ind, unsigned short fnum, unsigned long sum){
	struct ft_data *c_fd;
	int fd;
	unsigned long lsum;
	char buf[1024];

	c_fd=getfile(ind, fnum);
	if(c_fd==(struct ft_data*)NULL) return(0);
	local_file_check(c_fd->fname, &lsum);
	if(!c_fd->state&FS_CHECK){
		return(0);
	}
	if(lsum!=sum){
		path_build(buf, 4096, ANGBAND_DIR, c_fd->fname);
		fd=open(buf, O_RDONLY);
		if(fd==-1){
			remove_ft(c_fd);
			return(0);
		}
		c_fd->fd=fd;
		c_fd->ind=ind;
		c_fd->state=(FS_SEND|FS_NEW);
		Send_file_init(c_fd->ind, c_fd->id, c_fd->fname);
		return(1);
	}
	remove_ft(c_fd);
	return(1);
}

void kill_xfers(int ind){
	struct ft_data *trav;
	trav=fdata;
	for(; trav; trav=trav->next){
		if(!trav->id) continue;
		if(trav->ind==ind){
			close(trav->fd);
			remove_ft(trav);
		}
	}
}

/* handle all current SEND type file transfers */
/* laid out long like this for testing. DO NOT CHANGE */
void do_xfers(){
	int x;
	struct ft_data *trav;
	int ct=1;
	trav=fdata;
	for(; trav; trav=trav->next){
		if(!trav->id) continue;	/* non existent */
		if(!(trav->state&FS_SEND)) continue; /* wrong type */
		if(!(trav->state&FS_READY)) continue; /* not ready */
		x=read(trav->fd, trav->buffer, MAX_TNF_SEND);
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
 * Silly windows
 * DarkGod slaps windows
 */
#ifdef WIN32
int mkstemp(char *template)
{
        char f[30];	/* if it overflows, it overflows.. */
        int fd;

        if (!tmpnam(f)) return -1;
        fd = open(f, O_RDWR | O_CREAT);
	strcpy(template, f);	/* give back our filename */
        return fd;
}
#endif

/* Open file for receive/writing */
int local_file_init(int ind, unsigned short fnum, char *fname){
	struct ft_data *c_fd;
	char tname[30]="/tmp/tomexfer.XXXXXX";
	c_fd=getfile(ind, 0);		/* get empty space */
	if(c_fd==(struct ft_data*)NULL) return(0);

	if(fname[0]=='/') return(0);	/* lame security */
	if(strstr(fname, "..")) return(0);

	c_fd->fd=mkstemp(tname);
	c_fd->state=FS_READY;
	if(c_fd->fd!=-1){
		unlink(tname);		/* don't fill up /tmp */
		c_fd->id=fnum;
		c_fd->ind=ind;	/* not really needed for client */
		strncpy(c_fd->fname, fname, 30);
		return(1); 
	}
	remove_ft(c_fd);
	return(0);
}



/* Write received data to temporary file */
int local_file_write(int ind, unsigned short fnum, unsigned long len){
	struct ft_data *c_fd;
	c_fd=getfile(ind, fnum);
	if(c_fd==(struct ft_data*)NULL) return(0);
	Receive_file_data(ind, len, &c_fd->buffer);
	write(c_fd->fd, &c_fd->buffer, len);
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
	int fd;
	c_fd=getfile(ind, fnum);
	if(c_fd==(struct ft_data*)NULL) return(0);

	path_build(buf, 4096, ANGBAND_DIR, c_fd->fname);

	wp=fopen(buf, "w");
	if(wp!=(FILE*)NULL){
		lseek(c_fd->fd, 0, SEEK_SET);
		do{
			x=read(c_fd->fd, buf, size);
			fwrite(buf, x, 1, wp);
		}while(x>0);
		fclose(wp);
		success=1;
	}
	close(c_fd->fd);	/* close & remove temp file */
	remove_ft(c_fd);
	return(success);
}

unsigned long total;

void do_sum(unsigned char *buffer, int size){
	int i;
	for(i=0; i<size; i++){
		total+=buffer[i];
	}
}

/* Get checksum of file */
/* don't waste a file transfer data space locally */
int local_file_check(char *fname, unsigned long *sum){
	FILE *fp;
	unsigned char *buffer;
	int success=0;
	int size=4096;
	unsigned long tbytes=0;
	unsigned long pos, r;
	char buf[1024];

	path_build(buf, 1024, ANGBAND_DIR, fname);

	fp=fopen(buf, "r");
	if(fp==(FILE*)NULL){
		return(0);
	}
	buffer=(char*)malloc(size);
	if(buffer){
		total=0L;
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
	r=total % 0x10000 + (total % 0x100000000) / 0x10000;
	*sum=(r%0x10000)+r/0x10000;
	fclose(fp);
	return(success);
}
