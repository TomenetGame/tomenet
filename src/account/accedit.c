/* TomeNET account editor - evileye */
/* Quick account editor for server admin 
   It can be made more visually attractive later maybe.
   I just didn't want to leave accounts unchangeable
   while testing. */
#include <curses.h>
#include "account.h"

void editor(void);
void edit(void);
unsigned short ask(char *prompt);

FILE *fp;

char *fname="tomenet.acc";

int main(int argc, char *argv[]){
	fp=fopen(fname, "r+");
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
		mvprintw(7, 4, "%-30s   ID: %.10d", c_acc.name, c_acc.id);
		mvprintw(8, 4, "%-20s", c_acc.pass);
		mvprintw(10, 4, "Trial: %-3s", c_acc.flags & ACC_TRIAL ? "Yes" : "No");
		mvprintw(11, 4, "Admin: %-3s", c_acc.flags & ACC_ADMIN ? "Yes" : "No");
		mvprintw(12, 4, "Score: %-3s", c_acc.flags & ACC_NOSCORE ? "No" : "Yes");
		mvprintw(13, 4, "Multi: %-3s", c_acc.flags & ACC_MULTI ? "Yes" : "No");
		mvprintw(10, 37, "%-7s", c_acc.flags & ACC_DELD ? "DELETED" : "");
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
								fseek(fp, fpos*sizeof(struct account), SEEK_SET);
								fwrite(&c_acc, sizeof(struct account), 1, fp);
							}
						quit=1;
					}
					break;
				case 'n':
				case 'N':
					if(change)
						if(ask("This record was changed. Save?")){
							fseek(fp, fpos*sizeof(struct account), SEEK_SET);
							fwrite(&c_acc, sizeof(struct account), 1, fp);
						}
					x=fread(&c_acc, sizeof(struct account),1, fp);
					if(!x){
						mvprintw(LINES-1, 0, "No more records to edit");
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
							fseek(fp, fpos*sizeof(struct account), SEEK_SET);
							fwrite(&c_acc, sizeof(struct account), 1, fp);
						}
					if(!fpos){
						mvprintw(LINES-1, 0, "This is the first record");
						beep();
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
