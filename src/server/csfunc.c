/* $Id$ */
/* cave special function main file */

#define SERVER

#include "angband.h"

/* The original code was written by EvilEye */
/*
 * Some notes:		- Jir -
 *
 * 1. save/load (changed, so that c_special is stackable)
 * fx, fy and c_ptr->type should be saved/loaded in save.c/load2.c, and then
 * csfunc[].save/load should be called. format:
 *		byte fx, byte fy, byte number,
 *		byte type, {blabla written by csfunc} (x number)
 * this should be terminated by 255,255,255 or sth.
 * when loading, csfunc will load it to the c_ptr tossed.
 * [implemented - see rd_floor() and wr_floor() for details.]
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

#define CS_LOAD(xxx) void xxx(c_special *cs_ptr)
#define CS_SAVE(xxx) void xxx(c_special *cs_ptr)
#define CS_SEE(xxx) void xxx(c_special *cs_ptr, char *c, byte *a, int Ind)
#define CS_HIT(xxx) int xxx(c_special *cs_ptr, int y, int x, int Ind)

CS_LOAD(defload);
CS_SAVE(defsave);
CS_SEE(defsee);
CS_HIT(defhit);
CS_LOAD(dnaload);
CS_SAVE(dnasave);
CS_SEE(dnasee);
CS_HIT(dnahit);
CS_LOAD(keyload);
CS_SAVE(keysave);
CS_SEE(keysee);
CS_HIT(keyhit);
CS_LOAD(tload);
CS_SAVE(tsave);
CS_SEE(tsee);
CS_HIT(thit);
CS_LOAD(insc_load);
CS_SAVE(insc_save);
CS_HIT(insc_hit);
CS_LOAD(betweenload);
CS_SAVE(betweensave);
CS_SEE(betweensee);
CS_HIT(betweenhit);
CS_LOAD(fountload);
CS_SAVE(fountsave);
CS_SEE(fountsee);
CS_LOAD(montrapload);
CS_SAVE(montrapsave);
CS_LOAD(s32bload);
CS_SAVE(s32bsave);
CS_LOAD(runeload);
CS_SAVE(runesave);

void defload(c_special *cs_ptr){
}
void defsave(c_special *cs_ptr){
}
void defsee(c_special *cs_ptr, char *c, byte *a, int Ind){
	/* really do nothing */
}
int defhit(c_special *cs_ptr, int y, int x, int Ind){
	/* return implied permission */
	return(TRUE);
}

void dnaload(c_special *cs_ptr){
}
void dnasave(c_special *cs_ptr){
}
int dnahit(c_special *cs_ptr, int y, int x, int Ind) {
	/* we have to know from where we are called! */
	struct dna_type *dna = cs_ptr->sc.ptr;
	/* paranoia: for when the poly-house door bug occurred */
	if (!dna) {
		if (admin_p(Ind)) {
			msg_print(Ind, "\377D(Corrupt house dna on this grid)");
			return TRUE;
		} else return FALSE;
	}
	if (access_door(Ind, dna, TRUE) || admin_p(Ind)) {
		if (admin_p(Ind))
			msg_format(Ind, "\377gThis house is owned by %s.", get_house_owner(cs_ptr));
		return(TRUE);
	}
	return(FALSE);
}

void dnasee(c_special *cs_ptr, char *c, byte *a, int Ind){
}

void keyload(c_special *cs_ptr){
	struct key_type *key;
	MAKE(key, struct key_type);
	rd_u16b(&key->id);
	cs_ptr->sc.ptr = key;
}
void keysave(c_special *cs_ptr){
	struct key_type *key;
	key = cs_ptr->sc.ptr;
	wr_u16b(key->id);
}
int keyhit(c_special *cs_ptr, int y, int x, int Ind){
	struct player_type *p_ptr;
	int j;
	struct cave_type **zcave, *c_ptr;
	struct key_type *key = cs_ptr->sc.ptr;

	p_ptr = Players[Ind];
	if (!(zcave = getcave(&p_ptr->wpos))) return(FALSE);
	c_ptr = &zcave[y][x];

	if (c_ptr->feat == FEAT_HOME_OPEN) return(TRUE);
	if (!(cfg.door_bump_open && p_ptr->easy_open)) return(FALSE);
	if (p_ptr == (struct player_type*)NULL) return(FALSE);
	for (j = 0; j < INVEN_PACK; j++) {
		object_type *o_ptr = &p_ptr->inventory[j];
		if (o_ptr->tval == TV_KEY && o_ptr->sval == SV_HOUSE_KEY && o_ptr->pval == key->id) {
			c_ptr->feat = FEAT_HOME_OPEN;
			p_ptr->energy -= level_speed(&p_ptr->wpos) / 2;
			note_spot_depth(&p_ptr->wpos, y, x);
			everyone_lite_spot(&p_ptr->wpos, y, x);
			p_ptr->update |= (PU_VIEW | PU_LITE);
			msg_format(Ind, "\377gThe key fits in the lock. %d:%d",key->id, o_ptr->pval);
			return(TRUE);
		}
	}
	return(FALSE);
}

/* EXPERIMENTAL SEE CODE - I AM NOT INSISTING THAT WE SEE
   KEY DOORS ANY DIFFERENTLY - DO NOT DELETE !!! */
void keysee(c_special *cs_ptr, char *c, byte *a, int Ind){
	struct player_type *p_ptr;
	int j;
	struct key_type *key = cs_ptr->sc.ptr;

	p_ptr = Players[Ind];

	if(*c == FEAT_HOME_OPEN) return;	/* dont bother */
	if(p_ptr == (struct player_type*)NULL) return;
	for(j = 0; j < INVEN_PACK; j++){
		object_type *o_ptr = &p_ptr->inventory[j];
		if(o_ptr->tval == TV_KEY && o_ptr->sval == SV_HOUSE_KEY && o_ptr->pval == key->id){
			/* colours are only test colours! */
			*c = '*';
			*a = TERM_L_DARK;
		}
	}
	return;
}

/*
 * Traps
 */
/* *ptr is not used, but is still needed. */
void tload(c_special *cs_ptr)
{
	byte tmp8u;
	rd_byte(&cs_ptr->sc.trap.t_idx);
	rd_byte(&tmp8u);
	cs_ptr->sc.trap.found = tmp8u;
}

void tsave(c_special *cs_ptr)
{
	wr_byte(cs_ptr->sc.trap.t_idx);
	wr_byte(cs_ptr->sc.trap.found);
}
void tsee(c_special *cs_ptr, char *c, byte *a, int Ind){
//	printf("tsee %d\n", Ind);
}

int thit(c_special *cs_ptr, int y, int x, int Ind){
#if 0	/* temporary while csfunc->activate() is changed */
	if ((cs_ptr = GetCS(c_ptr, CS_TRAPS)) && !p_ptr->ghost) {
		bool hit = TRUE;

		/* Disturb */
		disturb(Ind, 0, 0);

		if (!cs_ptr->sc.trap.found) {
			/* Message */
			msg_print(Ind, "You triggered a trap!");

			/* Pick a trap */
			pick_trap(&p_ptr->wpos, p_ptr->py, p_ptr->px);
		}
		else if (magik(get_skill_scale(p_ptr, SKILL_DISARM, 90)
					- UNAWARENESS(p_ptr)))
		{
 #ifndef ARCADE_SERVER
			msg_print(Ind, "You carefully avoid touching the trap.");
			hit = FALSE;
 #endif
		}

		/* Hit the trap */
		if (hit) hit_trap(Ind);
	}
	return(TRUE);
#endif
	return(TRUE);	/* temp... */
}

void insc_load(c_special *cs_ptr){
	struct floor_insc *insc;
	MAKE(insc, struct floor_insc);
	cs_ptr->sc.ptr = insc;
	rd_string(insc->text, 80);
	rd_u16b(&insc->found);
}

void insc_save(c_special *cs_ptr){
	struct floor_insc *insc = cs_ptr->sc.ptr;
	wr_string(insc->text);
	wr_u16b(insc->found);
}

int insc_hit(c_special *cs_ptr, int y, int x, int Ind){
	struct floor_insc *sptr = cs_ptr->sc.ptr;
	char sign_text[MAX_CHARS], *s = sign_text, *p = NULL;
	strcpy(s, sptr->text);

	/* multi-line inscription? */
	if ((p = strchr(s, '|'))) {
		*p = 0;
		msg_format(Ind, "\377sA sign here reads: \377w%s", s);
		s = p + 1;

		while (*s) {
			p = strchr(s, '|');
			if (p) *p = 0;
			msg_format(Ind, "                   %s", s);
			if (p) s = p + 1;
			else break;
		}
	} else msg_format(Ind, "\377sA sign here reads: \377w%s", sptr->text);
	return(TRUE);
}

/*
 * Between gates (inner-floor version)
 */
void betweenload(c_special *cs_ptr)
{
	rd_byte(&cs_ptr->sc.between.fy);
	rd_byte(&cs_ptr->sc.between.fx);
}
void betweensave(c_special *cs_ptr)
{
	wr_byte(cs_ptr->sc.between.fy);
	wr_byte(cs_ptr->sc.between.fx);
}
void betweensee(c_special *cs_ptr, char *c, byte *a, int Ind){
//	printf("tsee %d\n", Ind);
}
int betweenhit(c_special *cs_ptr, int y, int x, int Ind){
//	printf("bhit: %d\n", Ind);
	return(TRUE);
}

void fountload(c_special *cs_ptr)
{
	byte tmp8u;
	rd_byte(&cs_ptr->sc.fountain.type);
	rd_byte(&cs_ptr->sc.fountain.rest);
	rd_byte(&tmp8u);
	cs_ptr->sc.fountain.known = tmp8u;
}
void fountsave(c_special *cs_ptr)
{
	wr_byte(cs_ptr->sc.fountain.type);
	wr_byte(cs_ptr->sc.fountain.rest);
	wr_byte(cs_ptr->sc.fountain.known);
}
void fountsee(c_special *cs_ptr, char *c, byte *a, int Ind){
	/* TODO: tell what kind if 'known' */
//	printf("fountsee %d\n", Ind);
}

/*
 * Monster_traps (inner-floor version)
 */
void montrapload(c_special *cs_ptr)
{
	rd_u16b(&cs_ptr->sc.montrap.trap_kit);
	rd_byte(&cs_ptr->sc.montrap.difficulty);
	rd_byte(&cs_ptr->sc.montrap.feat);
}

void montrapsave(c_special *cs_ptr)
{
	wr_u16b(cs_ptr->sc.montrap.trap_kit);
	wr_byte(cs_ptr->sc.montrap.difficulty);
	wr_byte(cs_ptr->sc.montrap.feat);
}

void runeload(c_special *cs_ptr)
{
	rd_byte(&cs_ptr->sc.rune.typ);
	rd_byte(&cs_ptr->sc.rune.mod);
	rd_byte(&cs_ptr->sc.rune.lev);
	rd_byte(&cs_ptr->sc.rune.feat);
	rd_s32b(&cs_ptr->sc.rune.id);
}

void runesave(c_special *cs_ptr)
{
	wr_byte(cs_ptr->sc.rune.typ);
	wr_byte(cs_ptr->sc.rune.mod);
	wr_byte(cs_ptr->sc.rune.lev);
	wr_byte(cs_ptr->sc.rune.feat);
	wr_s32b(cs_ptr->sc.rune.id);
}


void s32bload(c_special *cs_ptr)
{
	rd_s32b(&cs_ptr->sc.omni);
}
void s32bsave(c_special *cs_ptr)
{
	wr_s32b(cs_ptr->sc.omni);
}

void cs_erase(cave_type *c_ptr, struct c_special *cs_ptr) {
	struct c_special *trav, *prev;
	bool flag = FALSE;
	if (!c_ptr) return;

	prev = trav = c_ptr->special;
	while (trav) {
		if (trav == cs_ptr) {
			if (flag) prev->next = trav->next;
			else c_ptr->special = trav->next;

			KILL(trav, struct c_special);
			return;
		}
		flag = TRUE;
		prev = trav;
		trav = trav->next;
	}
}

struct sfunc csfunc[] = {
	{ defload, defsave, defsee, defhit },	/* CS_NONE */
	{ dnaload, dnasave, defsee, dnahit },	/* CS_DNADOOR */
	{ keyload, keysave, keysee, keyhit },	/* CS_KEYDOOR */
	{ tload, tsave, tsee, thit },			/* CS_TRAPS */
	{ insc_load, insc_save, defsee, insc_hit },	/* CS_INSCRIP */
	{ fountload, fountsave, fountsee, defhit },	/* CS_FOUNTAIN */
	{ betweenload, betweensave, defsee, betweenhit },	/* CS_BETWEEN */
	{ defload, defsave, defsee, defhit },	/* CS_BETWEEN2 (nowadays renamed to CS_BEACON) */
	{ montrapload, montrapsave, defsee, defhit },	/* CS_MON_TRAP */
	/* CS_FOUNTAIN, CS_BETWEEN, CS_BETWEEN2 to come */
	{ s32bload, s32bsave, defsee, defhit },	/* CS_SHOP */
	{ s32bload, s32bsave, defsee, defhit },	/* CS_MIMIC */
	{ runeload, runesave, defsee, defhit }, /* CS_RUNE */
/*
	{ iload, isave, isee, ihit }
*/
};

