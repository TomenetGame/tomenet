/* #include "files.h" */

/*
 * file transfer handling for tomenet - evileye
 * tcp already handles connection/errors, so we do
 * not need to worry about this.
 */

#include <stdio.h>
#include <fcntl.h>

#define FS_READY 1	/* if ready for more data, set. */
#define FS_SEND 2	/* sending connection? */
#define FS_NEW 4	/* queued? */
#define FS_DONE 8	/* ready to send end packet on final ack */
#define FS_CHECK 16	/* file data block open for sum compare only */

#define MAX_TNF_SEND	128	/* maximum bytes in a transfer
				 * make this a power of 2 if possible
				 */

struct ft_data{
	unsigned short id;	/* unique xfer ID */
	int ind;		/* server security - who has this transfer */
	char fname[30];		/* actual filename */
	int fd;			/* file descriptor */
	unsigned short state;	/* status of transfer */
	char buffer[MAX_TNF_SEND];
};

struct ft_data fdata[8];	/* maximum transfers at once */
				/* testing only - this NEEDS to be
				   expandable or the server will fail */

int getfile(int ind, unsigned short fnum){
	int i;
	for(i=0; i<8 ; i++){
		if(fdata[i].id==fnum && (!fnum || ind==fdata[i].ind)){
			return(i);
		}
	}
	return(-1);
}

int new_fileid(){
	return(123);
}

/* acknowledge recipient ready to receive more */
int local_file_ack(int ind, unsigned short fnum){
	int num;
	num=getfile(ind, fnum);
	if(num==-1) return(0);
	if(fdata[num].state&FS_SEND){
		fdata[num].state|=FS_READY;
	}
	if(fdata[num].state&FS_DONE){
		close(fdata[num].fd);
		fdata[num].id=0;
	}
	return(1);
}

/* for now, just kill the connection completely */
int local_file_err(int ind, unsigned short fnum){
	int num;
	num=getfile(ind, fnum);
	if(num==-1) return(0);
	close(fdata[num].fd);	/* close the file */
	fdata[num].id=0;
	return(1);
}

/* initialise an file to send */
int local_file_send(int ind, char *fname){
	int num, fd;
	num=getfile(ind, 0);
	if(num==-1) return(0);
	fd=open(fname, O_RDONLY);
	if(fd==-1) return(0);
	fdata[num].fd=fd;
	fdata[num].ind=ind;
	fdata[num].id=new_fileid();	/* ALWAYS succeed */
	fdata[num].state=(FS_SEND|FS_NEW);
	strncpy(fdata[num].fname, fname, 30);
	printf("send file %s\n", fname);
	Send_file_init(fdata[num].ind, fdata[num].id, fdata[num].fname);
	return(1);
}

/* request checksum of remote file */
int remote_update(int ind, char *fname){
	int num;
	num=getfile(ind, 0);
	if(num==-1) return(0);
	fdata[num].ind=ind;
	fdata[num].id=new_fileid();	/* ALWAYS succeed */
	fdata[num].state=(FS_CHECK);
	strncpy(fdata[num].fname, fname, 30);
	Send_file_check(fdata[num].ind, fdata[num].id, fdata[num].fname);
	printf("send update (%s)\n", fname);
	return(1);
}

/* compare checksums of local/remote files - update if
   necessary */
int check_return(int ind, unsigned short fnum, unsigned long sum){
	int num, fd;
	unsigned long lsum;
	num=getfile(ind, fnum);
	if(num==-1) return(0);
	printf("check return (%x)\n", sum);
	local_file_check(fdata[num].fname, &lsum);
	printf("local sum (%x)\n", lsum);
	if(!fdata[num].state&FS_CHECK){
		return;
	}
	if(lsum!=sum){
		fd=open(fdata[num].fname, O_RDONLY);
		if(fd==-1){
			fdata[num].id=0;
			return(0);
		}
		printf("update %s\n", fdata[num].fname);
		fdata[num].fd=fd;
		fdata[num].ind=ind;
		fdata[num].state=(FS_SEND|FS_NEW);
		printf("update file %s\n", fdata[num].fname);
		Send_file_init(fdata[num].ind, fdata[num].id, fdata[num].fname);
		return(1);
	}
	printf("checksums match for %s\n", fdata[num].fname);
	fdata[num].id=0;
	return(1);
}

/* handle all current SEND type file transfers */
/* laid out long like this for testing. DO NOT CHANGE */
void do_xfers(){
	int i, x;
	for(i=0; i<8; i++){
		if(!fdata[i].id) continue;	/* non existent */
		if(!(fdata[i].state&FS_SEND)) continue; /* wrong type */
		if(!(fdata[i].state&FS_READY)) continue; /* not ready */
		printf("xfer %d ready\n", i);
		x=read(fdata[i].fd, fdata[i].buffer, MAX_TNF_SEND);
		if(!(fdata[i].state&FS_DONE)){
			if(x==0){
				fdata[i].state|=FS_DONE;
			}
			else{
				Send_file_data(fdata[i].ind, fdata[i].id, fdata[i].buffer, x);
				fdata[i].state&=~(FS_READY);
			}
		}
		else{
			Send_file_end(fdata[i].ind, fdata[i].id);
		}
	}
}

/* Open file for receive/writing */
int local_file_init(int ind, unsigned short fnum, char *fname){
	int num;
	char tname[30]="/tmp/tomexfer.XXXX";
	num=getfile(ind, 0);		/* get empty space */
	if(num==-1) return(0);
	printf("receive file %s\n", fname);
	fdata[num].fd=mkstemp(tname);
	fdata[num].state=FS_READY;
	if(fdata[num].fd!=-1){
		fdata[num].id=fnum;
		fdata[num].ind=ind;	/* not really needed for client */
		strncpy(fdata[num].fname, fname, 30);
		return(1); 
	}
	return(0);
}



/* Write received data to temporary file */
int local_file_write(int ind, unsigned short fnum, unsigned long len){
	int num;
	num=getfile(ind, fnum);
	if(num==-1) return(0);
	Receive_file_data(ind, len, &fdata[num].buffer);
	write(fdata[num].fd, &fdata[num].buffer, len);
	return(1);
}

/* Close file and make the changes */
int local_file_close(int ind, unsigned short fnum){
	int num;
	num=getfile(ind, fnum);
	if(num==-1) return(0);
	printf("Received %s\n", fdata[num].fname);
	fdata[num].id=0;
	close(fdata[num].fd);
	return(1);
}

unsigned long long total;

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
	fp=fopen(fname, "r");
	if(fp==(FILE*)NULL){
		printf("no local file %s\n", fname);
		return(0);
	}
	printf("check local file %s\n", fname);
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
