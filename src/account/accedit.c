/* TomeNET account editor - evileye */
/* Quick account editor for server admin 
   It can be made more visually attractive later maybe.
   I just didn't want to leave accounts unchangeable
   while testing. */

#define HAVE_CRYPT 1

#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/file.h>

#include "account.h"

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

static char *t_crypt(char *inbuf, const char *salt);

void editor(void);
void edit(void);
unsigned short ask(char *prompt);
void status(char *info);
void setupscreen(void);
unsigned short recwrite(struct account *rec, long filepos);
int ListAccounts(int fpos);
void statinput(char *prompt, char *string, int max);
void getstring(const char *prompt, char *string, int max);
int findacc(bool next);
void purge_duplicates(void);

FILE *fp;
WINDOW *listwin, *mainwin;

char *fname = "tomenet.acc";
char newpass[20];
char *crypass;

int tfpos;

int main() {
	fp = fopen(fname, "rb");
	if (fp == (FILE*)NULL) {
		fprintf(stderr, "Cannot open %s\n", fname);
		exit(20);
	}
	editor();
	fclose(fp);
	return(0);
}

void setupscreen() {
	attron(A_STANDOUT);
	move(0, 0);
	clrtoeol();
	mvprintw(0, COLS / 2 - 18, "TomeNET account editor - 2002 Evileye");
	mvprintw(1, COLS / 2 - 16, "(Updated by Mikaelh and C. Blue)");
	attroff(A_STANDOUT);
	mvprintw(LINES - 3, COLS / 2 - 29, "N: next P: previous BKSPC: 1st/end D: Delete U: Purge dupes");
	mvprintw(LINES - 4, COLS / 2 - 29, "V+S: Validate+Score A: Admin M: Multi B: Ban C: Quiet H: PW");
	mvprintw(LINES - 5, COLS / 2 - 29, "L: List/Long f/F: Find/Next R: Restrict I: Privilege K: PvP");
	mainwin = newwin(LINES - 9, COLS, 3, 0);
	listwin = newwin(LINES - 9, COLS, 3, 0);
	refresh();
}

unsigned short recwrite(struct account *rec, long filepos) {
	int wfd;
#ifdef NETBSD
	wfd = open("tomenet.acc", O_RDWR|O_EXLOCK);	/* blocked open */
#else
	wfd = open("tomenet.acc", O_RDWR);
#endif
	if (wfd < 0) return(0);
#ifndef NETBSD
	if ((flock(wfd, LOCK_EX)) != 0) return(0);
#endif
	lseek(wfd, filepos, SEEK_SET);
	if (write(wfd, rec, sizeof(struct account)) == -1)
		fprintf(stderr, "write error occurred.");
#ifndef NETBSD
	while((flock(wfd, LOCK_UN)) != 0);
#endif
	close(wfd);
#ifdef NETBSD
	fpurge(fp);
#else
	fflush(fp);
#endif
	fseek(fp, filepos + sizeof(struct account), SEEK_SET);
	return(1);
}

/* short form list editor */
int ListAccounts(int fpos) {
	int quit = 0;
	char ch;
	int ifpos = 0;	/* internal file (LIST TOP) position */
	int i = 0, x;
	short reload = 1;
	short redraw = 0;
	struct account c_acc;
	unsigned short change = 0;

	touchwin(listwin);
	mvwaddstr(listwin, 0, 5, "Name");
	mvwaddstr(listwin, 0, 27, "Val");
	mvwaddstr(listwin, 0, 31, "Adm");
	mvwaddstr(listwin, 0, 35, "Sco");
	mvwaddstr(listwin, 0, 39, "Mul");
	mvwaddstr(listwin, 0, 43, "Res");
	mvwaddstr(listwin, 0, 47, "Pri");
	mvwaddstr(listwin, 0, 51, "PK");
	mvwaddstr(listwin, 0, 55, "Qui");
	mvwaddstr(listwin, 0, 59, "Ban");
	mvwaddstr(listwin, 0, 64, "Account ID");
	if (fpos > LINES-11) {
		ifpos = fpos - (LINES - 11);
	}
	while (!quit) {
		if (reload) {
			fseek(fp, ifpos*sizeof(struct account), SEEK_SET);
			for (i = 0; i < (LINES - 10); i++) {
				x = fread(&c_acc, sizeof(struct account), 1, fp);
				if (x == 0) break;
				mvwprintw(listwin, i+1, 5, "%-22s%-4c%-4c%-4c%-4c%-4c%-4c%-4c%-4c%-4c%.10d%10s", c_acc.name,
				c_acc.flags & ACC_TRIAL ? '.' : 'Y',
				c_acc.flags & ACC_ADMIN ? 'Y' : '.',
				c_acc.flags & ACC_NOSCORE ? '.' : 'Y',
				c_acc.flags & ACC_MULTI ? 'Y' : '.',
				c_acc.flags & ACC_VRESTRICTED ? 'V' : c_acc.flags & ACC_RESTRICTED ? 'Y' : '.',
				c_acc.flags & ACC_VPRIVILEGED ? 'V' : c_acc.flags & ACC_PRIVILEGED ? 'Y' : '.',
				c_acc.flags & ACC_PVP ? 'Y' : c_acc.flags & ACC_ANOPVP ? 'K' : c_acc.flags & ACC_NOPVP ? 'N' : '.',
				c_acc.flags & ACC_VQUIET ? 'V' : c_acc.flags & ACC_QUIET ? 'Y' : '.',
				c_acc.flags & ACC_BANNED ? 'Y' : '.',
				c_acc.id, c_acc.flags & ACC_DELD ? "DELETED" : "");
			}
			reload = 0;
			redraw = 1;
		}
		if (!change) {
			fseek(fp, fpos*sizeof(struct account), SEEK_SET);
			x = fread(&c_acc, sizeof(struct account), 1, fp);
		}
		if (redraw) {
			mvwprintw(listwin, (fpos - ifpos) + 1, 5, "%-22s%-4c%-4c%-4c%-4c%-4c%-4c%-4c%-4c%-4c%.10d%10s", c_acc.name,
			c_acc.flags & ACC_TRIAL ? '.' : 'Y',
			c_acc.flags & ACC_ADMIN ? 'Y' : '.',
			c_acc.flags & ACC_NOSCORE ? '.' : 'Y',
			c_acc.flags & ACC_MULTI ? 'Y' : '.',
			c_acc.flags & ACC_VRESTRICTED ? 'V' : c_acc.flags & ACC_RESTRICTED ? 'Y' : '.',
			c_acc.flags & ACC_VPRIVILEGED ? 'V' : c_acc.flags & ACC_PRIVILEGED ? 'Y' : '.',
			c_acc.flags & ACC_PVP ? 'Y' : c_acc.flags & ACC_ANOPVP ? 'K' : c_acc.flags & ACC_NOPVP ? 'N' : '.',
			c_acc.flags & ACC_VQUIET ? 'V' : c_acc.flags & ACC_QUIET ? 'Y' : '.',
			c_acc.flags & ACC_BANNED ? 'Y' : '.',
			c_acc.id, c_acc.flags & ACC_DELD ? "DELETED" : "");
			redraw = 0;
		}
		mvwaddch(listwin, (fpos - ifpos) + 1, 3, '>');
		wrefresh(listwin);
		ch = getch();
		move(LINES - 1, 0);
		clrtoeol();
		switch (ch) {
			case 'h':
			case 'H':
				getstring("New password: ", newpass, 20);
				crypass = t_crypt(newpass, c_acc.name);
				strncpy(c_acc.pass, crypass, 19);
				change = 1;
				break;
			case 'l':
			case 'L':
				quit = 1;
				break;
			case 'f':
			case 'F':
				mvwaddch(listwin, (fpos - ifpos) + 1, 3, ' ');
				if (change)
					if (ask("This record was changed. Save?")) {
						if (recwrite(&c_acc, fpos * sizeof(struct account)))
							change = 0;
						else {
							if (!ask("Could not write record. Continue anyway?"))
								break;
						}
					}
				tfpos = findacc(ch == 'F');
				if (tfpos >= 0) {
					fpos = tfpos;
					change = 0;
					if (fpos > ifpos + (LINES - 11))
						ifpos = fpos - (LINES - 11);
					else if (fpos < ifpos)
						ifpos = fpos;
					reload = 1;
				}
				break;
			case 'n':
			case 'N':
			case ' ':
				if (change)
					if (ask("This record was changed. Save?")) {
						if (recwrite(&c_acc, fpos * sizeof(struct account)))
							change = 0;
						else {
							if (!ask("Could not write record. Continue anyway?"))
								break;
						}
					}
				mvwaddch(listwin, (fpos - ifpos) + 1, 3, ' ');
				if ((fpos - ifpos) < (i - 1)) {
					fpos++;
					change = 0;
				} else {
					if (i < (LINES - 11)) {
						status("There are no more records");
						break;
					}
					x = fread(&c_acc, sizeof(struct account), 1, fp);
					if (x == 0)
						status("There are no more records");
					else {
						ifpos++;
						fpos++;
						change = 0;
						reload = 1;
					}
				}
				break;
			case 'p':
			case 'P':
				if (change)
					if (ask("This record was changed. Save?")) {
						if (recwrite(&c_acc, fpos * sizeof(struct account)))
							change = 0;
						else {
							if (!ask("Could not write record. Continue anyway?"))
								break;
						}
					}
				mvwaddch(listwin, (fpos - ifpos) + 1, 3, ' ');
				if (fpos > ifpos) {
					fpos--;
					change = 0;
				} else {
					if (ifpos > 0) {
						ifpos--;
						fpos--;
						change = 0;
						reload = 1;
					}
					else
						status("This is the first record");
				}
				break;
			case 8: case 127: //BACKSPACE: Do both, 'Home' and 'End'
				if (change)
					if (ask("This record was changed. Save?")) {
						if (recwrite(&c_acc, fpos * sizeof(struct account)))
							change = 0;
						else {
							if (!ask("Could not write record. Continue anyway?"))
								break;
						}
					}
				if (!fpos) {
					do {
						x = fread(&c_acc, sizeof(struct account), 1, fp);
						fpos++;
					} while (x);
					fpos--;
					ifpos = fpos;
					status("Jumped to last record");
				} else {
					ifpos = fpos = 0;
					fseek(fp, 0, SEEK_SET);
					x = fread(&c_acc, sizeof(struct account), 1, fp);
					status("Jumped to first record");
				}
				change = 0;
				reload = 1;
				break;
			case 'q':
			case 'Q':
				if(ask("Are you sure you want to quit?")) {
					quit = 1;
					fpos = -1;
				}
				break;
			case 'v':
			case 'V':
				change = 1;
				redraw = 1;
				c_acc.flags ^= ACC_TRIAL;
				break;
			case 'a':
			case 'A':
				change = 1;
				redraw = 1;
				c_acc.flags ^= ACC_ADMIN;
				break;
			case 'm':
			case 'M':
				change = 1;
				redraw = 1;
				c_acc.flags ^= ACC_MULTI;
				break;
			case 's':
			case 'S':
				change = 1;
				redraw = 1;
				c_acc.flags ^= ACC_NOSCORE;
				break;
			case 'c':
			case 'C':
				change = 1;
				if (c_acc.flags & ACC_VQUIET) {
					c_acc.flags &= ~(ACC_VQUIET | ACC_QUIET);
				} else if (c_acc.flags & ACC_QUIET) {
					c_acc.flags &= ~ACC_QUIET;
					c_acc.flags |= ACC_VQUIET;
				} else {
					c_acc.flags |= ACC_QUIET;
				}
				break;
			case 'b':
			case 'B':
				change = 1;
				c_acc.flags ^= ACC_BANNED;
				break;
			case 'k':
			case 'K':
				change = 1;
				if (c_acc.flags & ACC_ANOPVP) {
					c_acc.flags &= ~(ACC_ANOPVP | ACC_NOPVP);
				} else if (c_acc.flags & ACC_NOPVP) {
					c_acc.flags &= ~ACC_NOPVP;
					c_acc.flags |= ACC_ANOPVP;
				} else if (c_acc.flags & ACC_PVP) {
					c_acc.flags &= ~ACC_PVP;
					c_acc.flags |= ACC_NOPVP;
				} else {
					c_acc.flags |= ACC_PVP;
				}
				break;
			case 'r':
			case 'R':
				change = 1;
				if (c_acc.flags & ACC_VRESTRICTED) {
					c_acc.flags &= ~(ACC_VRESTRICTED | ACC_RESTRICTED);
				} else if (c_acc.flags & ACC_RESTRICTED) {
					c_acc.flags &= ~ACC_RESTRICTED;
					c_acc.flags |= ACC_VRESTRICTED;
				} else {
					c_acc.flags |= ACC_RESTRICTED;
				}
				break;
			case 'i':
			case 'I':
				change = 1;
				if (c_acc.flags & ACC_VPRIVILEGED) {
					c_acc.flags &= ~(ACC_VPRIVILEGED | ACC_PRIVILEGED);
				} else if (c_acc.flags & ACC_PRIVILEGED) {
					c_acc.flags &= ~ACC_PRIVILEGED;
					c_acc.flags |= ACC_VPRIVILEGED;
				} else {
					c_acc.flags |= ACC_PRIVILEGED;
				}
				break;
			case 'U':
				purge_duplicates();
				break;
			default:
				beep();
		}
	}
	mvwaddch(listwin, (fpos - ifpos) + 1, 3, ' ');
	return(fpos);
}

void editor() {
	int x;
	int quit = 0;
	char ch;
	unsigned long fpos = 0;
	struct account c_acc;
	unsigned short change = 0;

	x = fread(&c_acc, sizeof(struct account), 1, fp);
	if (!x) {
		printf("Cannot read from file\n");
		return;
	}

	initscr();
	cbreak();
	noecho();
	setupscreen();

	while (!quit) {
		mvwprintw(mainwin, 0, 4, "%-30s   ID: %.10d", c_acc.name, c_acc.id);
		mvwprintw(mainwin, 1, 4, "%-20s             (%-30s)", c_acc.pass, c_acc.name_normalised);
		mvwprintw(mainwin, 3, 4, "Trial (inval): %-4s", c_acc.flags & ACC_TRIAL ? "Yes " : "No  ");
		mvwprintw(mainwin, 4, 4, "Administrator: %-4s", c_acc.flags & ACC_ADMIN ? "*** Yes *** " : "No          ");
		mvwprintw(mainwin, 5, 4, "Scoreboard:    %-4s", c_acc.flags & ACC_NOSCORE ? "No  " : "Yes ");
		mvwprintw(mainwin, 6, 4, "Multi-login:   %-4s", c_acc.flags & ACC_MULTI ? "Yes " : "No  ");
		mvwprintw(mainwin, 7, 4, "Restricted:    %-4s", c_acc.flags & ACC_VRESTRICTED ? "Very" : c_acc.flags & ACC_RESTRICTED ? "Yes " : "No  ");
		mvwprintw(mainwin, 8, 4, "Privileged:    %-3s", c_acc.flags & ACC_VPRIVILEGED ? "Very" : c_acc.flags & ACC_PRIVILEGED ? "Yes " : "No  ");
		mvwprintw(mainwin, 9, 4, "May pkill:     %-3s", c_acc.flags & ACC_PVP ? "Yes " : c_acc.flags & ACC_ANOPVP ? "_NO_" : c_acc.flags & ACC_NOPVP ? "No  " : "std.");
		mvwprintw(mainwin, 10, 4, "Muted chat:    %-3s", c_acc.flags & ACC_VQUIET ? "Very" : c_acc.flags & ACC_QUIET ? "Yes " : "No  ");
		mvwprintw(mainwin, 11, 4, "Banned:        %-3s", c_acc.flags & ACC_BANNED ? "** Yes ** " : "No        ");
		mvwprintw(mainwin, 3, 37, "%-7s", c_acc.flags & ACC_DELD ? "DELETED" : "");
		wrefresh(mainwin);
		do {
			ch = getch();
			move(LINES - 1, 0);
			clrtoeol();
			switch (ch) {
				case 'h':
				case 'H':
					getstring("New password: ", newpass, 20);
					crypass = t_crypt(newpass, c_acc.name);
					strncpy(c_acc.pass, crypass, 19);
					change = 1;
					break;
				case 'Q':
				case 'q':
					if (ask("Are you sure you want to quit?")) {
						if (change)
							if (ask("This record was changed. Save?")) {
								recwrite(&c_acc, fpos * sizeof(struct account));
							}
						quit = 1;
					}
					break;
				case 'l':
				case 'L':
					/* Short list of accounts */
					fpos = ListAccounts(fpos);
					if (fpos == (unsigned long) - 1) {
						quit = 1;
						break;
					}

					fseek(fp, fpos * sizeof(struct account), SEEK_SET);
					x = fread(&c_acc, sizeof(struct account), 1, fp);
					change = 0;
					touchwin(mainwin);
					continue;
					break;
				case 'f':
				case 'F':
					if (change)
						if (ask("This record was changed. Save?")) {
							if (recwrite(&c_acc, fpos * sizeof(struct account)))
								change = 0;
							else {
								if (!ask("Could not write record. Continue anyway?"))
									break;
							}
						}
					tfpos = findacc(ch == 'F');
					if (tfpos >= 0) {
						fpos = tfpos;
						change = 0;
						fseek(fp, fpos * sizeof(struct account), SEEK_SET);
						x = fread(&c_acc, sizeof(struct account), 1, fp);
					}
				break;
				case 'n':
				case 'N':
				case ' ':
					if (change)
						if (ask("This record was changed. Save?")) {
							if (recwrite(&c_acc, fpos * sizeof(struct account)))
								change = 0;
							else {
								if (!ask("Could not write record. Continue anyway?"))
									break;
							}
						}
					x = fread(&c_acc, sizeof(struct account), 1, fp);
					if (!x) {
						status("No more records to edit.");
						beep();
						break;
					}
					fpos++;
					change = 0;
					break;
				case 'p':
				case 'P':
					if (change)
						if (ask("This record was changed. Save?")) {
							if (recwrite(&c_acc, fpos * sizeof(struct account)))
								change = 0;
							else {
								if (!ask("Could not write record. Continue anyway?"))
									break;
							}
						}
					if (!fpos) {
						status("This is the first record");
						break;
					}
					fpos--;
					fseek(fp, fpos * sizeof(struct account), SEEK_SET);
					x = fread(&c_acc, sizeof(struct account), 1, fp);
					change = 0;
					break;
				case 8: case 127: //BACKSPACE: Do both, 'Home' and 'End'
					if (change)
						if (ask("This record was changed. Save?")) {
							if (recwrite(&c_acc, fpos * sizeof(struct account)))
								change = 0;
							else {
								if (!ask("Could not write record. Continue anyway?"))
									break;
							}
						}
					if (!fpos) {
						do {
							x = fread(&c_acc, sizeof(struct account), 1, fp);
							fpos++;
						} while (x);
						fpos--;
						status("Jumped to last record");
					} else {
						fpos = 0;
						fseek(fp, 0, SEEK_SET);
						x = fread(&c_acc, sizeof(struct account), 1, fp);
						status("Jumped to first record");
					}
					change = 0;
					break;
				case 'D':
					if (ask("Are you sure you wish to delete this record?")) {
						change = 1;
						c_acc.flags |= ACC_DELD;
					}
					break;
				case 'v':
				case 'V':
					change = 1;
					c_acc.flags ^= ACC_TRIAL;
					break;
				case 'a':
				case 'A':
					change = 1;
					c_acc.flags ^= ACC_ADMIN;
					break;
				case 'm':
				case 'M':
					change = 1;
					c_acc.flags ^= ACC_MULTI;
					break;
				case 's':
				case 'S':
					change = 1;
					c_acc.flags ^= ACC_NOSCORE;
					break;
				case 'c':
				case 'C':
					change = 1;
					if (c_acc.flags & ACC_VQUIET) {
						c_acc.flags &= ~(ACC_VQUIET | ACC_QUIET);
					} else if (c_acc.flags & ACC_QUIET) {
						c_acc.flags &= ~ACC_QUIET;
						c_acc.flags |= ACC_VQUIET;
					} else {
						c_acc.flags |= ACC_QUIET;
					}
					break;
				case 'b':
				case 'B':
					change = 1;
					c_acc.flags ^= ACC_BANNED;
					break;
				case 'k':
				case 'K':
					change = 1;
					if (c_acc.flags & ACC_ANOPVP) {
						c_acc.flags &= ~(ACC_ANOPVP | ACC_NOPVP);
					} else if (c_acc.flags & ACC_NOPVP) {
						c_acc.flags &= ~ACC_NOPVP;
						c_acc.flags |= ACC_ANOPVP;
					} else if (c_acc.flags & ACC_PVP) {
						c_acc.flags &= ~ACC_PVP;
						c_acc.flags |= ACC_NOPVP;
					} else {
						c_acc.flags |= ACC_PVP;
					}
					break;
				case 'r':
				case 'R':
					change = 1;
					if (c_acc.flags & ACC_VRESTRICTED) {
						c_acc.flags &= ~(ACC_VRESTRICTED | ACC_RESTRICTED);
					} else if (c_acc.flags & ACC_RESTRICTED) {
						c_acc.flags &= ~ACC_RESTRICTED;
						c_acc.flags |= ACC_VRESTRICTED;
					} else {
						c_acc.flags |= ACC_RESTRICTED;
					}
					break;
				case 'i':
				case 'I':
					change = 1;
					if (c_acc.flags & ACC_VPRIVILEGED) {
						c_acc.flags &= ~(ACC_VPRIVILEGED | ACC_PRIVILEGED);
					} else if (c_acc.flags & ACC_PRIVILEGED) {
						c_acc.flags &= ~ACC_PRIVILEGED;
						c_acc.flags |= ACC_VPRIVILEGED;
					} else {
						c_acc.flags |= ACC_PRIVILEGED;
					}
					break;
				case 'U':
					purge_duplicates();
					break;
				default:
					ch = ' ';
					beep();
			}
		} while (ch == ' ');
	}
	echo();
	nocbreak();
	endwin();
}

/* account finder */
#define PARTIAL_ANYWHERE /* don't restrict partial search to prefix-only */
int findacc(bool next) {
	int x, l;
	static int i = 0;
	static char sname[30] = { 0 };
#ifdef PARTIAL_ANYWHERE
	static char sname2[30] = { 0 };
#endif
	struct account c_acc;

	if (!next) {
		statinput("Find which name: ", sname, 30);
#ifdef PARTIAL_ANYWHERE
		strcpy(sname2, sname);
#endif
		/* its always upper, and admins can be lazy */
		sname[0] = toupper(sname[0]);

		/* search for direct match */
		i = 0;
		fseek(fp, 0L, SEEK_SET);
		while ((x = fread(&c_acc, sizeof(struct account), 1, fp))) {
			i++;
			if (!strncmp(c_acc.name, sname, 30)) return (i - 1);
		} while(x);

		/* prepare for partial match search */
		i = 0;
		fseek(fp, 0L, SEEK_SET);
	}

	/* search for partial match (prefix) */
	l = strlen(sname);
	while ((x = fread(&c_acc, sizeof(struct account), 1, fp))) {
		i++;
		if (!strncmp(c_acc.name, sname, l)) return (i - 1);
#ifdef PARTIAL_ANYWHERE
		if (strstr(c_acc.name, sname2)) return (i - 1);
#endif
	} while(x);

	status("Could not find that account");
	return (-1);
}

void status(char *info) {
	mvprintw(LINES - 1, 0, info);
	beep();
}

void statinput(char *prompt, char *string, int max) {
	char ch;
	int pos = 0;
	mvprintw(LINES - 1, 0, prompt);
	do {
		ch = getch();
		if (ch == '\n') break;
		if (ch == '\b') {
			if (pos) {
				string[--pos] = '\0';
				mvdelch(LINES - 1, strlen(prompt) + pos);
			}
			else
				beep();
		}
		else{
			if (pos < (max - 1)) {
				string[pos++] = ch;
				addch(ch);
			}
			else
				beep();
		}
		refresh();
	} while (ch != '\n');
	string[pos] = '\0';
	move(LINES - 1, 0);
	clrtoeol();
}

void getstring(const char *prompt, char *string, int max) {
	char ch;
	int i = 0;
	mvprintw(LINES - 1, 0, prompt);
	do {
		ch = getch();
		if (ch == '\b' && i > 0)
			i--;
		if (ch == '\r' || ch == '\n') {
			string[i] = '\0';
			move(LINES - 1, 0);
			clrtoeol();
			return;
		}
		else {
			string[i++] = ch;
			addch(ch);
			if (i == max) {
				string[i] = '\0';
				move(LINES - 1, 0);
				clrtoeol();
				return;
			}
		}
	} while(1);
}

/* our password encryptor */
static char *t_crypt(char *inbuf, const char *salt) {
#ifdef HAVE_CRYPT
	static char out[64];
 #if 0 /* doesn't do anything */
	char setting[9];
	setting[0] = '_';
	strncpy(&setting[1], salt, 8);
 #endif
  #if 1 /* fix for len-1 names */
	char setting[3];
	/* only 1 character long salt? expand to 2 chars length */
	if (!salt[1]) {
		setting[0] = '.';
		setting[1] = salt[0];
		setting[2] = 0;
		strcpy(out, (char*)crypt(inbuf, setting));
	} else
 #endif
 #if 1 /* SPACE _ ! - ' , and probably more as _2nd character_ cause crypt() to return a null pointer ('.' is ok) */
  #define ACCOUNTNAME_LEN 16
	if (!((salt[1] >= 'A' && salt[1] <= 'Z') ||
	    (salt[1] >= 'a' && salt[1] <= 'z') ||
	    (salt[1] >= '0' && salt[1] <= '9') ||
	    salt[1] == '.')) {
		char fixed_name[ACCOUNTNAME_LEN];
		strcpy(fixed_name, salt);
		fixed_name[1] = '.';
		strcpy(out, (char*)crypt(inbuf, fixed_name));
	} else
 #endif
	strcpy(out, (char*)crypt(inbuf, salt));
	return(out);
#else
	return(inbuf);
#endif
}

unsigned short ask(char *prompt) {
	char ch;
	mvprintw(LINES - 1, 0, prompt);
	do {
		ch = getch();
		if (ch == 'Y' || ch == 'y') break;
		if (ch == 'N' || ch == 'n') break;
	} while(1);
	move(LINES - 1, 0);
	clrtoeol();
	return ((ch == 'Y' || ch == 'y'));
}

void purge_duplicates(void) {
	//char accname[];
}
