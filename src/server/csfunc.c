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

struct sfunc csfunc[]={
	{ defload, defsave, defsee, defhit },	/* CS_NONE */
	{ dnaload, dnasave, defsee, dnahit },	/* DNA_DOOR */
	{ keyload, keysave, defsee, keyhit },	/* KEY_DOOR */
	{ tload, tsave, tsee, thit },			/* CS_TRAPS */
	{ defload, defsave, defsee, defhit },	/* CS_INSCRIP */
	{ defload, defsave, defsee, defhit },	/* CS_FOUNTAIN */
	{ betweenload, betweensave, defsee, betweenhit },	/* CS_FOUNTAIN */
	/* CS_FOUNTAIN, CS_BETWEEN, CS_BETWEEN2 to come */
/*
	{ iload, isave, isee, ihit }
*/
};

void defload(void *ptr, cave_type *c_ptr){
}
void defsave(void *ptr){
}
void defsee(void *ptr, int Ind){
}
void defhit(void *ptr, int Ind){
}

void dnaload(void *ptr, cave_type *c_ptr){
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

void keyload(void *ptr, cave_type *c_ptr){
}
void keysave(void *ptr){
}
void keyhit(void *ptr, int Ind){
	printf("keyhit: %d\n", Ind);
}

/*
 * Traps
 */
/* *ptr is not used, but is still needed. */
void tload(void *ptr, cave_type *c_ptr)
{
	rd_byte(&c_ptr->special.sc.trap.t_idx);
	rd_byte(&c_ptr->special.sc.trap.found);
}
void tsave(void *ptr)
{
	struct c_special *cs_ptr = ptr;
	wr_byte(cs_ptr->sc.trap.t_idx);
	wr_byte(cs_ptr->sc.trap.found);
}
void tsee(void *ptr, int Ind){
	printf("tsee %d\n", Ind);
}
void thit(void *ptr, int Ind){
	printf("thit: %d\n", Ind);
}

/*
 * Between gates (inner-floor version)
 */
void betweenload(void *ptr, cave_type *c_ptr)
{
	rd_byte(&c_ptr->special.sc.between.fy);
	rd_byte(&c_ptr->special.sc.between.fx);
}
void betweensave(void *ptr)
{
	struct c_special *cs_ptr = ptr;
	wr_byte(cs_ptr->sc.between.fy);
	wr_byte(cs_ptr->sc.between.fx);
}
void betweensee(void *ptr, int Ind){
	printf("tsee %d\n", Ind);
}
void betweenhit(void *ptr, int Ind){
	printf("thit: %d\n", Ind);
}


void cs_erase(cave_type *c_ptr)
{
	if (!c_ptr) return;
	c_ptr->special.type = 0;
	c_ptr->special.sc.ptr = NULL;
}
