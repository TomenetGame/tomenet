/* TomeNET account editor - evileye */
/* Quick account editor for server admin 
   It can be made more visually attractive later maybe.
   I just didn't want to leave accounts unchangeable
   while testing. */
#include <curses.h>
#include <fcntl.h>
#include <errno.h>

#include "account.h"

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

void editor(void);
void edit(void);
unsigned short ask(char *prompt);

FILE *fp;
WINDOW *listwin, *mainwin;

char *fname="tomenet.acc";

int main(int argc, char *argv[]){
	fp=fopen(fname, "r");
	if(fp==(FILE*)NULL){
		fprintf(stderr, "Cannot open %s\n", fname);
		exit(20);
	}
	editor();
	fclose(fp);
}

void setupscreen(){
	attron(A_STANDOUT);
	move(0, 0);
	clrtoeol();
	mvprintw(0, COLS/2-18, "TomeNET account editor - 2002 Evileye");
	attroff(A_STANDOUT);
	mvprintw(LINES-3, COLS/2-19, "N: next      P: previous     D: Delete");
	mvprintw(LINES-4, COLS/2-19, "V: Validate A: Admin S: Score M: Multi");
	mvprintw(LINES-5, COLS/2-19, "L: List/Long accounts");
	mainwin=newwin(LINES-9, COLS, 3, 0);
	listwin=newwin(LINES-9, COLS, 3, 0);
	refresh();
}

unsigned short recwrite(struct account *rec, long filepos){
	int wfd;
#ifdef NETBSD
	wfd=open("tomenet.acc", O_RDWR|O_EXLOCK);	/* blocked open */
#else
	wfd=open("tomenet.acc", O_RDWR);
#endif
	if(wfd<0) return(0);
#ifndef NETBSD
	if((flock(wfd, LOCK_EX))!=0) return(0);
#endif
	lseek(wfd, filepos, SEEK_SET);
	write(wfd, rec, sizeof(struct account));
#ifndef NETBSD
	while((flock(wfd, LOCK_UN))!=0);
#endif
	close(wfd);
#ifdef NETBSD
	fpurge(fp);
#else
	fflush(fp);
#endif
	fseek(fp, filepos+sizeof(struct account), SEEK_SET);
	return(1);
}

/* short form list editor */
int ListAccounts(int fpos){
	int quit=0;
	char ch;
	int ifpos=0;	/* internal file (LIST TOP) position */
	int i, x;
	short reload=1;
	struct account c_acc;

	touchwin(listwin);
	mvwaddstr(listwin, 0, 5, "Name");
	mvwaddstr(listwin, 0, 25, "Valid");
	mvwaddstr(listwin, 0, 33, "Admin");
	mvwaddstr(listwin, 0, 41, "Score");
	mvwaddstr(listwin, 0, 49, "Multi");
	mvwaddstr(listwin, 0, 59, "Account ID");
	if(fpos>LINES-11){
		ifpos=fpos-(LINES-11);
	}
	while(!quit){
		if(reload){
			fseek(fp, ifpos*sizeof(struct account), SEEK_SET);
			for(i=0; i<(LINES-10); i++){
				x=fread(&c_acc, sizeof(struct account), 1, fp);
				if(x==0) break;
				mvwprintw(listwin, i+1, 5, "%-22s%-8c%-8c%-8c%-8c%.10d%10s", c_acc.name,
				c_acc.flags & ACC_TRIAL ? '.' : 'X',
				c_acc.flags & ACC_ADMIN ? 'X' : '.',
				c_acc.flags & ACC_NOSCORE ? '.' : 'X',
				c_acc.flags & ACC_MULTI ? 'X' : '.', c_acc.id, c_acc.flags & ACC_DELD ? "DELETED" : "");
			}
			reload=0;
		}
		mvwaddch(listwin, (fpos-ifpos)+1, 3, '>');
		wrefresh(listwin);
		ch=getch();
		move(LINES-1, 0);
		clrtoeol();
		switch(ch){
			case 'l':
			case 'L':
				quit=1;
				break;
			case 'n':
			case 'N':
				mvwaddch(listwin, (fpos-ifpos)+1, 3, ' ');
				if((fpos-ifpos) < (i-1))
					fpos++;
				else{
					if(i < (LINES-11)) break;
					fseek(fp, (fpos+1)*sizeof(struct account), SEEK_SET);
					x=fread(&c_acc, sizeof(struct account), 1, fp);
					if(x==0)
						status("There are no more records");
					else{
						ifpos++;
						fpos++;
						reload=1;
					}
				}
				break;
			case 'p':
			case 'P':
				mvwaddch(listwin, (fpos-ifpos)+1, 3, ' ');
				if(fpos>ifpos)
					fpos--;
				else{
					if(ifpos>0){
						ifpos--;
						fpos--;
						reload=1;
					}
					else
						status("This is the first record");
				}
				break;
			case 'q':
			case 'Q':
				if(ask("Are you sure you want to quit?")){
					quit=1;
					fpos=-1;
				}
				break;
			default:
				beep();
		}
	}
	mvwaddch(listwin, (fpos-ifpos)+1, 3, ' ');
	return(fpos);
}

void editor(){
	int i, x;
	int quit=0;
	char ch;
	unsigned long fpos=0;
	struct account c_acc;
	unsigned short change=0;

	x=fread(&c_acc, sizeof(struct account), 1, fp);
	if(!x){
		printf("Cannot read from file\n");
		return;
	}

	initscr();
	cbreak();
	noecho();
	setupscreen();

	while(!quit){
		mvwprintw(mainwin, 0, 4, "%-30s   ID: %.10d", c_acc.name, c_acc.id);
		mvwprintw(mainwin, 1, 4, "%-20s", c_acc.pass);
		mvwprintw(mainwin, 3, 4, "Trial: %-3s", c_acc.flags & ACC_TRIAL ? "Yes" : "No");
		mvwprintw(mainwin, 4, 4, "Admin: %-3s", c_acc.flags & ACC_ADMIN ? "Yes" : "No");
		mvwprintw(mainwin, 5, 4, "Score: %-3s", c_acc.flags & ACC_NOSCORE ? "No" : "Yes");
		mvwprintw(mainwin, 6, 4, "Multi: %-3s", c_acc.flags & ACC_MULTI ? "Yes" : "No");
		mvwprintw(mainwin, 3, 37, "%-7s", c_acc.flags & ACC_DELD ? "DELETED" : "");
		wrefresh(mainwin);
		do{
			ch=getch();
			move(LINES-1, 0);
			clrtoeol();
			switch(ch){
				case 'Q':
				case 'q':
					if(ask("Are you sure you want to quit?")){
						if(change)
							if(ask("This record was changed. Save?")){
								recwrite(&c_acc, fpos*sizeof(struct account));
							}
						quit=1;
					}
					break;
				case 'l':
				case 'L':
					/* Short list of accounts */
					fpos=ListAccounts(fpos);
					if(fpos==-1){
						quit=1;
						break;
					}

					fseek(fp, fpos*sizeof(struct account), SEEK_SET);
					x=fread(&c_acc, sizeof(struct account),1, fp);
					touchwin(mainwin);
					continue;
					break;
				case 'n':
				case 'N':
					if(change)
						if(ask("This record was changed. Save?")){
							recwrite(&c_acc, fpos*sizeof(struct account));
							change=0;
						}
					x=fread(&c_acc, sizeof(struct account),1, fp);
					if(!x){
						status("No more records to edit.");
						beep();
						break;
					}
					fpos++;
					change=0;
					break;
				case 'p':
				case 'P':
					if(change)
						if(ask("This record was changed. Save?")){
							recwrite(&c_acc, fpos*sizeof(struct account));
							change=0;
						}
					if(!fpos){
						status("This is the first record");
						break;
					}
					fpos--;
					fseek(fp, fpos*sizeof(struct account), SEEK_SET);
					x=fread(&c_acc, sizeof(struct account),1, fp);
					change=0;
					break;
				case 'D':
					if(ask("Are you sure you wish to delete this record?")){
						change=1;
						c_acc.flags|=ACC_DELD;
					}
					break;
				case 'v':
				case 'V':
					change=1;
					c_acc.flags^=ACC_TRIAL;
					break;
				case 'a':
				case 'A':
					change=1;
					c_acc.flags^=ACC_ADMIN;
					break;
				case 'm':
				case 'M':
					change=1;
					c_acc.flags^=ACC_MULTI;
					break;
				case 's':
				case 'S':
					change=1;
					c_acc.flags^=ACC_NOSCORE;
					break;
				default:
					ch=' ';
					beep();
			}
		}while(ch==' ');
	}
	echo();
	nocbreak();
	endwin();
}

void status(char *info){
	mvprintw(LINES-1, 0, info);
	beep();
}

unsigned short ask(char *prompt){
	char ch;
	mvprintw(LINES-1, 0, prompt);
	do{
		ch=getch();
		if(ch=='Y' || ch=='y') break;
		if(ch=='N' || ch=='n') break;
	}while(1);
	move(LINES-1, 0);
	clrtoeol();
	return((ch=='Y' || ch=='y'));
}
