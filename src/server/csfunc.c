/* cave special function main file */

#define SERVER

#include "angband.h"

void defload(void *ptr);
void defsave(void *ptr);
void defsee(void *ptr, int Ind);
void defhit(void *ptr, int Ind);

void dnaload(void *ptr);
void dnasave(void *ptr);
void dnahit(void *ptr, int Ind);

void keyload(void *ptr);
void keysave(void *ptr);
void keyhit(void *ptr, int Ind);

void tload(void *ptr);
void tsave(void *ptr);
void tsee(void *ptr, int Ind);
void thit(void *ptr, int Ind);

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

void defload(void *ptr){
}
void defsave(void *ptr){
}
void defsee(void *ptr, int Ind){
}
void defhit(void *ptr, int Ind){
}

void dnaload(void *ptr){
}
void dnasave(void *ptr){
}
void dnahit(void *ptr, int Ind){
	/* we have to know from where we are called! */
	if(access_door(Ind, (struct dna_type *)ptr)){
		printf("Access to door!\n");
		return;
	}
	printf("no access!\n");
}

void keyload(void *ptr){
}
void keysave(void *ptr){
}
void keyhit(void *ptr, int Ind){
	printf("keyhit: %d\n", Ind);
}

void tload(void *ptr){
}
void tsave(void *ptr){
}
void tsee(void *ptr, int Ind){
	printf("tsee %d\n", Ind);
}
void thit(void *ptr, int Ind){
	printf("thit: %d\n", Ind);
}
