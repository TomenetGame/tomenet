/* $Id$ */
/* cave special function main file */

#define SERVER

#include "angband.h"

/* The original code was written by EvilEye */
/*
 * Some notes:		- Jir -
 *
 * 1. save/load
 * fx, fy and c_ptr->type should be saved/loaded in save.c/load2.c, and then
 * csfunc[].save/load should be called. format:
 *		byte fx, byte fy, byte type, {blabla written by csfunc}
 * this should be terminated by 255,255,255 or sth.
 * when loading, csfunc will load it to the c_ptr tossed.
 * [implemented - see rd_dungeon() and wr_dungeon() for details.]
 *
 * 2. see
 * nice if csfunc is called from map_info(); otherwise we should add another
 * switch-case clause there..
 * or this simply handles do_cmd_look etc?
 *
 * 3.activate
 * a player's not always on the spot when activating it (eg. door traps);
 * how should they be handled?
 *
 * 4. union 'sc'
 * in case 'sc' isn't *ptr, do this -
 * csfunc[c_ptr->special.type].load(&c_ptr->special)
 */

/* of course, we can write'em in externs.h .. */
/* save.c */
extern void wr_byte(byte v);
extern void wr_u16b(u16b v);
extern void wr_s16b(s16b v);
extern void wr_u32b(u32b v);
extern void wr_s32b(s32b v);
extern void wr_string(cptr str);

/* load2.c */
extern void rd_byte(byte *ip);
extern void rd_u16b(u16b *ip);
extern void rd_s16b(s16b *ip);
extern void rd_u32b(u32b *ip);
extern void rd_s32b(s32b *ip);
extern void rd_string(char *str, int max);

#if 0
void defload(void *ptr, cave_type *c_ptr);
void defsave(void *ptr);
void defsee(void *ptr, int Ind);
void defhit(void *ptr, int Ind);

void dnaload(void *ptr, cave_type *c_ptr);
void dnasave(void *ptr);
void dnahit(void *ptr, int Ind);

void keyload(void *ptr, cave_type *c_ptr);
void keysave(void *ptr);
void keyhit(void *ptr, int Ind);

void tload(void *ptr, cave_type *c_ptr);
void tsave(void *ptr);
void tsee(void *ptr, int Ind);
void thit(void *ptr, int Ind);

void betweenload(void *ptr, cave_type *c_ptr);
void betweensave(void *ptr);
//void betweensee(void *ptr, int Ind);
void betweenhit(void *ptr, int Ind);
#endif	// 0


//void defload(void *ptr, cave_type *c_ptr){
//void defload(void *ptr, c_special *cs_ptr){
void defload(c_special *cs_ptr){
}
void defsave(c_special *cs_ptr){
}
void defsee(void *ptr, int Ind){
}
int defhit(void *ptr, int Ind){
}

//void dnaload(void *ptr, cave_type *c_ptr){
void dnaload(c_special *cs_ptr){
}
void dnasave(c_special *cs_ptr){
}
int dnahit(void *ptr, int Ind){
	/* we have to know from where we are called! */
	if(access_door(Ind, (struct dna_type *)ptr)){
		printf("Access to door!\n");
		return(1);
	}
	printf("no access!\n");
	return(0);
}

//void keyload(void *ptr, cave_type *c_ptr){
void keyload(c_special *cs_ptr){
}
void keysave(c_special *cs_ptr){
}
int keyhit(void *ptr, int Ind){
	printf("keyhit: %d\n", Ind);
	return(0);
}

/*
 * Traps
 */
/* *ptr is not used, but is still needed. */
#if 0
void tload(void *ptr, cave_type *c_ptr)
{
	struct c_special *cs_ptr;
//	cs_ptr=AddCS(c_ptr);
	cs_ptr=GetCS(c_ptr, CS_TRAPS);
	rd_byte(&cs_ptr->sc.trap.t_idx);
	rd_byte(&cs_ptr->sc.trap.found);
}
#else
void tload(c_special *cs_ptr)
{
	rd_byte(&cs_ptr->sc.trap.t_idx);
	rd_byte(&cs_ptr->sc.trap.found);
}
#endif	// 0
void tsave(c_special *cs_ptr)
{
	wr_byte(cs_ptr->sc.trap.t_idx);
	wr_byte(cs_ptr->sc.trap.found);
}
void tsee(void *ptr, int Ind){
	printf("tsee %d\n", Ind);
}
int thit(void *ptr, int Ind){
	printf("thit: %d\n", Ind);
	return(0);
}

void insc_load(c_special *cs_ptr){
	struct floor_insc *insc;
	MAKE(insc, struct floor_insc);
	cs_ptr->sc.ptr=insc;
	rd_string(insc->text, 80);
	rd_u16b(&insc->found);
}

void insc_save(c_special *cs_ptr){
	struct floor_insc *insc=cs_ptr->sc.ptr;
	wr_string(insc->text);
	wr_u16b(insc->found);
}

int insc_hit(void *ptr, int Ind){
	printf("hit inscr\n");
	return(0);
}

/*
 * Between gates (inner-floor version)
 */
#if 0
void betweenload(void *ptr, cave_type *c_ptr)
{
	struct c_special *cs_ptr;
//	cs_ptr=AddCS(c_ptr);
	cs_ptr=GetCS(c_ptr, CS_BETWEEN);
	rd_byte(&cs_ptr->sc.between.fy);
	rd_byte(&cs_ptr->sc.between.fx);
}
#else
void betweenload(c_special *cs_ptr)
{
	rd_byte(&cs_ptr->sc.between.fy);
	rd_byte(&cs_ptr->sc.between.fx);
}
#endif	// 0
void betweensave(c_special *cs_ptr)
{
	wr_byte(cs_ptr->sc.between.fy);
	wr_byte(cs_ptr->sc.between.fx);
}
void betweensee(void *ptr, int Ind){
	printf("tsee %d\n", Ind);
}
int betweenhit(void *ptr, int Ind){
	printf("bhit: %d\n", Ind);
	return(0);
}

/*
 * Monster_traps (inner-floor version)
 */
#if 0
void montrapload(void *ptr, cave_type *c_ptr)
{
	struct c_special *cs_ptr;
//	cs_ptr=AddCS(c_ptr);
	cs_ptr=GetCS(c_ptr, CS_MON_TRAP);
	rd_u16b(&cs_ptr->sc.montrap.trap_kit);
	rd_byte(&cs_ptr->sc.montrap.difficulty);
	rd_byte(&cs_ptr->sc.montrap.feat);
}
#else	// 0
void montrapload(c_special *cs_ptr)
{
	rd_u16b(&cs_ptr->sc.montrap.trap_kit);
	rd_byte(&cs_ptr->sc.montrap.difficulty);
	rd_byte(&cs_ptr->sc.montrap.feat);
}
#endif	// 0
void montrapsave(c_special *cs_ptr)
{
	wr_u16b(cs_ptr->sc.montrap.trap_kit);
	wr_byte(cs_ptr->sc.montrap.difficulty);
	wr_byte(cs_ptr->sc.montrap.feat);
}

#if 0
void cs_erase(cave_type *c_ptr, struct c_special *cs_ptr)
{
	struct c_special *trav, *prev;
	if (!c_ptr) return;

	prev=trav=c_ptr->special;
	while(trav){
		if(trav==cs_ptr){
			prev=trav->next;
			KILL(trav, struct c_special);
			return;
		}
	}
}
#else // 0
void cs_erase(cave_type *c_ptr, struct c_special *cs_ptr)
{
	struct c_special *trav, *prev;
	bool flag = FALSE;
	if (!c_ptr) return;

	prev=trav=c_ptr->special;
	while(trav){
		if(trav==cs_ptr){
			if (flag) prev->next=trav->next;
			else c_ptr->special = trav->next;

			KILL(trav, struct c_special);
			return;
		}
		flag = TRUE;
		prev=trav;
		trav=trav->next;
	}
}
#endif	// 0


struct sfunc csfunc[]={
	{ defload, defsave, defsee, defhit },	/* CS_NONE */
	{ dnaload, dnasave, defsee, dnahit },	/* CS_DNADOOR */
	{ keyload, keysave, defsee, keyhit },	/* CS_KEYDOOR */
	{ tload, tsave, tsee, thit },			/* CS_TRAPS */
	{ insc_load, insc_save, defsee, insc_hit },	/* CS_INSCRIP */
	{ defload, defsave, defsee, defhit },	/* CS_FOUNTAIN */
	{ betweenload, betweensave, defsee, betweenhit },	/* CS_BETWEEN */
	{ defload, defsave, defsee, defhit },	/* CS_BETWEEN2 */
	{ montrapload, montrapsave, defsee, defhit },	/* CS_MON_TRAP */
	/* CS_FOUNTAIN, CS_BETWEEN, CS_BETWEEN2 to come */
/*
	{ iload, isave, isee, ihit }
*/
};

