/* cave special function main file */

#define SERVER

#include "angband.h"

void defload(void);
void defsave(void);
void defsee(int Ind);
void defhit(int Ind);

void dnaload(void);
void dnasave(void);
void dnahit(int Ind);

void keyload(void);
void keysave(void);
void keyhit(int Ind);

void tload(void);
void tsave(void);
void tsee(int Ind);
void thit(int Ind);

struct sfunc csfunc[]={
	{ defload, defsave, defsee, defhit },
	{ dnaload, dnasave, defsee, dnahit },
	{ keyload, keysave, defsee, keyhit },
	{ tload, tsave, tsee, thit },
	{ defload, defsave, defsee, defhit }
/*
	{ iload, isave, isee, ihit }
*/
};

void defload(void){
	printf("defload\n");
}
void defsave(void){
	printf("defsave\n");
}
void defsee(int Ind){
	printf("defsee: %d\n", Ind);
}
void defhit(int Ind){
	printf("defhit: %d\n", Ind);
}

void dnaload(void){
}
void dnasave(void){
}
void dnahit(int Ind){
	printf("dnahit: %d\n", Ind);
}

void keyload(void){
}
void keysave(void){
}
void keyhit(int Ind){
	printf("keyhit: %d\n", Ind);
}

void tload(void){
}
void tsave(void){
}
void tsee(int Ind){
	printf("tsee %d\n", Ind);
}
void thit(int Ind){
	printf("thit: %d\n", Ind);
}
