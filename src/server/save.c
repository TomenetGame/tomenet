/* $Id$ */
/* File: save.c */

/* Purpose: interact with savefiles */

#define SERVER

#include "angband.h"

static void new_wr_wild();
static void new_wr_floors();
void wr_towns();
void wr_byte(byte v);
void wr_u16b(u16b v);
void wr_s16b(s16b v);
void wr_u32b(u32b v);
void wr_s32b(s32b v);
void wr_string(cptr str);
static void write_buffer();



/*
 * Some "local" parameters, used to help write savefiles
 */

static FILE	*fff;		/* Current save "file" */

static char	*fff_buf;	/* Savefile buffer */
static int	fff_buf_pos;	/* Buffer position */

#define MAX_BUF_SIZE	4096

static byte	xor_byte;	/* Simple encryption */

static u32b	v_stamp = 0L;	/* A simple "checksum" on the actual values */
static u32b	x_stamp = 0L;	/* A simple "checksum" on the encoded bytes */



/*
 * These functions place information into a savefile a byte at a time
 */

static void sf_put(byte v) {
	/* Encode the value, write a character */
	xor_byte ^= v;
#if 0
	(void)putc((int)xor_byte, fff);
#else
	/* Buffered output */
	fff_buf[fff_buf_pos++] = xor_byte;

	if (fff_buf_pos == MAX_BUF_SIZE) {
		if (fwrite(fff_buf, 1, MAX_BUF_SIZE, fff) < MAX_BUF_SIZE) {
			s_printf("Writing to savefile failed: %s\n", feof(fff) ? "EOF" : strerror(ferror(fff)));
		}
		fff_buf_pos = 0;
	}
#endif

	/* Maintain the checksum info */
	v_stamp += v;
	x_stamp += xor_byte;
}

void wr_byte(byte v) {
	sf_put(v);
}

void wr_u16b(u16b v) {
	sf_put(v & 0xFF);
	sf_put((v >> 8) & 0xFF);
}

void wr_s16b(s16b v) {
	wr_u16b((u16b)v);
}

void wr_u32b(u32b v) {
	sf_put(v & 0xFF);
	sf_put((v >> 8) & 0xFF);
	sf_put((v >> 16) & 0xFF);
	sf_put((v >> 24) & 0xFF);
}

void wr_s32b(s32b v) {
	wr_u32b((u32b)v);
}

void wr_string(cptr str) {
	while (*str) {
		wr_byte(*str);
		str++;
	}
	wr_byte(*str);
}

static void write_buffer() {
	if (fff_buf_pos > 0) {
		if (fwrite(fff_buf, 1, fff_buf_pos, fff) < fff_buf_pos) {
			s_printf("Writing to savefile failed: %s\n", feof(fff) ? "EOF" : strerror(ferror(fff)));
		}
		fff_buf_pos = 0;
	}
}


/*
 * These functions write info in larger logical records
 */


/*
 * Write an "item" record
 */
static void wr_item(object_type *o_ptr) {
	wr_s32b(o_ptr->owner);
	wr_s16b(o_ptr->level);
	wr_byte(o_ptr->mode);

	wr_s16b(o_ptr->k_idx);

	wr_byte(o_ptr->iy);
	wr_byte(o_ptr->ix);

	wr_s16b(o_ptr->wpos.wx);
	wr_s16b(o_ptr->wpos.wy);
	wr_s16b(o_ptr->wpos.wz);

	wr_byte(o_ptr->tval);
	wr_byte(o_ptr->sval);
	wr_byte(o_ptr->tval2);
	wr_byte(o_ptr->sval2);
	wr_s32b(o_ptr->bpval);
	wr_s32b(o_ptr->pval);
	wr_s32b(o_ptr->pval2);
	wr_s32b(o_ptr->pval3);
	wr_s32b(o_ptr->sigil);
	wr_s32b(o_ptr->sseed);

	wr_byte(o_ptr->discount);
	wr_byte(o_ptr->number);
	wr_s16b(o_ptr->weight);

	wr_u16b(o_ptr->name1);
	wr_u16b(o_ptr->name2);
	wr_s32b(o_ptr->name3);
	wr_s32b(o_ptr->timeout);
	wr_s32b(o_ptr->timeout_magic);
	wr_s32b(o_ptr->recharging);

	wr_s16b(o_ptr->to_h);
	wr_s16b(o_ptr->to_d);
	wr_s16b(o_ptr->to_a);

	/* VAMPIRES_INV_CURSED */
	wr_s16b(o_ptr->to_h_org);
	wr_s16b(o_ptr->to_d_org);
	wr_s16b(o_ptr->to_a_org);
	wr_s32b(o_ptr->pval_org);
	wr_s32b(o_ptr->bpval_org);

/* DEBUGGING PURPOSES - the_sandman */
#if 0
        if (o_ptr->tval == 46)
         {
          s_printf("TRAP_DEBUG: Trap with s_val:%d,to_h:%d,to_d:%d,to_a:%d written\n",
                                o_ptr->sval, o_ptr->to_h, o_ptr->to_d, o_ptr->to_a);
         }
#endif
	wr_s16b(o_ptr->ac);
	wr_byte(o_ptr->dd);
	wr_byte(o_ptr->ds);

	wr_u16b(o_ptr->ident);

	wr_u16b(o_ptr->name2b);

	wr_s16b(o_ptr->xtra1);
	wr_s16b(o_ptr->xtra2);
	/* more info, originally for self-made spellbook feature */
	wr_s16b(o_ptr->xtra3);
	wr_s16b(o_ptr->xtra4);
	wr_s16b(o_ptr->xtra5);
	wr_s16b(o_ptr->xtra6);
	wr_s16b(o_ptr->xtra7);
	wr_s16b(o_ptr->xtra8);
	wr_s16b(o_ptr->xtra9);

	wr_s32b(o_ptr->marked);
	wr_byte(o_ptr->marked2);

	wr_byte(o_ptr->questor);
	wr_s16b(o_ptr->questor_idx);
	wr_s16b(o_ptr->quest);
	wr_s16b(o_ptr->quest_stage);
	wr_byte(o_ptr->questor_invincible);

	/* Save the inscription (if any) */
	if (o_ptr->note) {
		wr_string(quark_str(o_ptr->note));
	} else {
		wr_string("");
	}
	wr_byte(o_ptr->note_utag);

	wr_u16b(o_ptr->next_o_idx);
	wr_byte(o_ptr->stack_pos);
	wr_u16b(o_ptr->held_m_idx);

	wr_s32b((s32b)o_ptr->appraised_value); //HOME_APPRAISAL
	wr_u16b(o_ptr->housed); //EXPORT_PLAYER_STORE_OFFERS
}

/*
 * Write a "monster" record
 */
static void wr_monster_race(monster_race *r_ptr) {
	int i;

	wr_u16b(r_ptr->name);
	wr_u16b(r_ptr->text);
	wr_byte(r_ptr->hdice);
	wr_byte(r_ptr->hside);
	wr_s16b(r_ptr->ac);
	wr_s16b(r_ptr->sleep);
	wr_byte(r_ptr->aaf);
	wr_byte(r_ptr->speed);
	wr_s32b(r_ptr->mexp);
	wr_s16b(r_ptr->extra);
	wr_byte(r_ptr->freq_innate);
	wr_byte(r_ptr->freq_spell);
	wr_u32b(r_ptr->flags1);
	wr_u32b(r_ptr->flags2);
	wr_u32b(r_ptr->flags3);
	wr_u32b(r_ptr->flags4);
	wr_u32b(r_ptr->flags5);
	wr_u32b(r_ptr->flags6);
	wr_u32b(r_ptr->flags7);
	wr_u32b(r_ptr->flags8);
	wr_u32b(r_ptr->flags9);
	wr_u32b(r_ptr->flags0);
	wr_s16b(r_ptr->level);
	wr_byte(r_ptr->rarity);
	wr_byte(r_ptr->d_attr);
	wr_byte(r_ptr->d_char);
	wr_byte(r_ptr->x_attr);
	wr_byte(r_ptr->x_char);
	for (i = 0; i < 4; i++) {
		wr_byte(r_ptr->blow[i].method);
		wr_byte(r_ptr->blow[i].effect);
		wr_byte(r_ptr->blow[i].d_dice);
		wr_byte(r_ptr->blow[i].d_side);
	}
}

/*
 * Write a "monster" record
 */
static void wr_monster(monster_type *m_ptr) {
	int i;
	wr_byte(m_ptr->pet);
	wr_byte(m_ptr->special);
	wr_byte(m_ptr->questor);
	wr_s16b(m_ptr->questor_idx);
	wr_s16b(m_ptr->quest);
	wr_byte(m_ptr->questor_invincible);
	wr_byte(m_ptr->questor_hostile);
	wr_byte(m_ptr->questor_target);

	wr_s32b(m_ptr->owner);
	wr_s16b(m_ptr->r_idx);
	wr_byte(m_ptr->fy);
	wr_byte(m_ptr->fx);

	wr_s16b(m_ptr->wpos.wx);
	wr_s16b(m_ptr->wpos.wy);
	wr_s16b(m_ptr->wpos.wz);

	wr_s16b(m_ptr->ac);
	wr_byte(m_ptr->speed);
	wr_s32b(m_ptr->exp);
	wr_s16b(m_ptr->level);
	for (i = 0; i < 4; i++) {
		wr_byte(m_ptr->blow[i].method);
		wr_byte(m_ptr->blow[i].effect);
		wr_byte(m_ptr->blow[i].d_dice);
		wr_byte(m_ptr->blow[i].d_side);
	}
	wr_s32b(m_ptr->hp);
	wr_s32b(m_ptr->maxhp);
	wr_s16b(m_ptr->csleep);
	wr_byte(m_ptr->mspeed);
//	wr_byte(m_ptr->energy);
	wr_s16b(m_ptr->energy);
	wr_byte(m_ptr->stunned);
	wr_byte(m_ptr->confused);
	wr_byte(m_ptr->monfear);
	wr_byte(m_ptr->paralyzed);
	wr_byte(m_ptr->bleeding);
	wr_byte(m_ptr->poisoned);
	wr_byte(m_ptr->blinded);
	wr_byte(m_ptr->silenced);
	wr_u16b(m_ptr->hold_o_idx);
	wr_u16b(m_ptr->clone);
	wr_s16b(m_ptr->mind);

	if (m_ptr->special || m_ptr->questor) wr_monster_race(m_ptr->r_ptr);

	wr_u16b(m_ptr->ego);
	wr_s32b(m_ptr->name3);

	wr_s16b(m_ptr->status);
	wr_s16b(m_ptr->target);
	wr_s16b(m_ptr->possessor);
	wr_s16b(m_ptr->destx);
	wr_s16b(m_ptr->desty);
	wr_s16b(m_ptr->determination);
	wr_s16b(m_ptr->limit_hp);
}

/*
 * Write a "lore" record
 */
static void wr_global_lore(int r_idx) {
	monster_race *r_ptr = &r_info[r_idx];

	/* Count sights/deaths/kills */
	wr_s32b(r_ptr->r_sights);
	wr_s32b(r_ptr->r_deaths);
	wr_s32b(r_ptr->r_tkills);

	/* Global monster limit */
	wr_s32b(r_ptr->max_num);

	/* Global monster population */
	wr_s32b(r_ptr->cur_num);
}

/*
 * Write an "xtra" record
 */
static void wr_xtra(int Ind, int k_idx) {
	player_type *p_ptr = Players[Ind];
	byte tmp8u = 0;

	if (p_ptr->obj_aware[k_idx]) tmp8u |= 0x01;
	if (p_ptr->obj_tried[k_idx]) tmp8u |= 0x02;
	if (p_ptr->obj_felt[k_idx]) tmp8u |= 0x04;
	if (p_ptr->obj_felt_heavy[k_idx]) tmp8u |= 0x08;

	wr_byte(tmp8u);
}

/*
 * Write a "trap memory" record
 *
 * Of course, we should be able to compress this into 1/8..	- Jir -
 */
static void wr_trap_memory(int Ind, int t_idx) {
	player_type *p_ptr = Players[Ind];
	byte tmp8u = 0;

	if (p_ptr->trap_ident[t_idx]) tmp8u |= 0x01;

	wr_byte(tmp8u);
}


/*
 * Write a "store" record
 */
static void wr_store(store_type *st_ptr) {
	int j;

	/* Save the "open" counter */
	wr_u32b(st_ptr->store_open);

	/* Save the "insults" */
	wr_s16b(st_ptr->insult_cur);

	/* Save the current owner */
	wr_u16b(st_ptr->owner);
//	wr_byte(st_ptr->owner);

	/* Save the stock size */
	wr_byte(st_ptr->stock_num);

	/* Save the "haggle" info */
	wr_s16b(st_ptr->good_buy);
	wr_s16b(st_ptr->bad_buy);

	/* Last visit */
	wr_s32b(st_ptr->last_visit);

	/* Save the stock */
	for (j = 0; j < st_ptr->stock_num; j++) {
		/* Save each item in stock */
		wr_item(&st_ptr->stock[j]);
	}
}

static void wr_bbs() {
	int i, j;

	wr_s16b(BBS_LINES);

	for (i = 0; i < BBS_LINES; i++)
		wr_string(bbs_line[i]);

	/* also write complete party-BBS and guild-BBS (tripling the server file size oO) - C. Blue */

	wr_s16b(MAX_PARTIES);
	for (j = 0; j < MAX_PARTIES; j++)
		for (i = 0; i < BBS_LINES; i++)
			wr_string(pbbs_line[j][i]);

	wr_s16b(MAX_GUILDS);
	for (j = 0; j < MAX_GUILDS; j++)
		for (i = 0; i < BBS_LINES; i++)
			wr_string(gbbs_line[j][i]);
}

static void wr_notes() {
	int i;

	wr_s16b(MAX_NOTES);
	for (i = 0; i < MAX_NOTES; i++) {
		wr_string(priv_note[i]);
		wr_string(priv_note_sender[i]);
		wr_string(priv_note_target[i]);
	}

	wr_s16b(MAX_PARTYNOTES);
	for (i = 0; i < MAX_PARTYNOTES; i++) {
		wr_string(party_note[i]);
		wr_string(party_note_target[i]);
	}

	wr_s16b(MAX_GUILDNOTES);
	for (i = 0; i < MAX_GUILDNOTES; i++) {
		wr_string(guild_note[i]);
		wr_string(guild_note_target[i]);
	}
	//omitted (use custom.lua instead): admin_note[MAX_ADMINNOTES]
}

static void wr_mail() {
#ifdef ENABLE_MERCHANT_MAIL
	int i;

	wr_s16b(MAX_MERCHANT_MAILS);
	for (i = 0; i < MAX_MERCHANT_MAILS; i++) {
		wr_item(&mail_forge[i]);
		wr_string(mail_sender[i]);
		wr_string(mail_target[i]);
		wr_string(mail_target_acc[i]);
		wr_s16b(mail_duration[i]);
		wr_s32b(mail_timeout[i]);
		wr_byte(mail_COD[i]);
		wr_u32b(mail_xfee[i]);
	}
#else
	wr_s16b(0);
#endif
}

static void wr_xorders() {
	int i;
	wr_s16b(questid);
	for (i = 0; i < MAX_XORDERS; i++) {
		wr_s16b(xorders[i].active);
		wr_s16b(xorders[i].id);
		wr_s16b(xorders[i].type);
		wr_u16b(xorders[i].flags);
		wr_s32b(xorders[i].creator);
		wr_s32b(xorders[i].turn);
	}
}

#if 0 /* need to use separate function save_quests() now */
static void wr_quests() {
	int i, j, k;

	wr_s16b(max_q_idx);
	wr_s16b(QI_QUESTORS);
	wr_byte(QI_FLAGS);

	for (i = 0; i < max_q_idx; i++) {
		wr_byte(q_info[i].active);
#if 0 /* actually don't write this, it's more comfortable to use q_info '-2 repeatable' entry instead */
		wr_byte(q_info[i].disabled);
#else
		wr_byte(0);
#endif
		wr_s16b(q_info[i].cur_cooldown);
		wr_s16b(q_info[i].cur_stage);
		wr_s32b(q_info[i].turn_activated);
		wr_s32b(q_info[i].turn_acquired);

		for (j = 0; j < QI_QUESTORS; j++) {
#if 0//restructure
			wr_byte(q_info[i].current_wpos[j].wx);
			wr_byte(q_info[i].current_wpos[j].wy);
			wr_byte(q_info[i].current_wpos[j].wz);
			wr_byte(q_info[i].current_x[j]);
			wr_byte(q_info[i].current_y[j]);

			wr_s16b(q_info[i].questor_m_idx[j]);
#else
			wr_byte(0);wr_byte(0);wr_byte(0);wr_byte(0);wr_byte(0);wr_byte(0);wr_byte(0);
#endif
		}

		wr_u16b(q_info[i].flags);

		for (k = 0; k < QI_STAGES; k++) {
			for (j = 0; j < QI_GOALS; j++) {
#if 0//restructure
				wr_byte(q_info[i].goals[k][j]);
				wr_s16b(q_info[i].kill_number_left[k][j]);
#else
				wr_byte(0);wr_byte(0);wr_byte(0);
#endif
			}
			for (j = 0; j < QI_OPTIONAL; j++) {
#if 0//restructure
				wr_byte(q_info[i].goalsopt[k][j]);
				wr_s16b(q_info[i].killopt_number_left[k][j]);
#else
				wr_byte(0);wr_byte(0);wr_byte(0);
#endif
			}
		}
	}
}
#endif

static void wr_guilds() {
	int i, j;
	u16b tmp16u;

	tmp16u = MAX_GUILDS;
	wr_u16b(tmp16u);

	/* Dump the guilds */
	for (i = 0; i < tmp16u; i++){
		wr_u32b(guilds[i].dna);
		wr_string(guilds[i].name);
		wr_s32b(guilds[i].master);
		wr_s32b(guilds[i].members);
		wr_byte(guilds[i].cmode);
		wr_u32b(guilds[i].flags);
		wr_s16b(guilds[i].minlev);
		for (j = 0; j < 5; j++)
			wr_string(guilds[i].adder[j]);
		wr_s16b(guilds[i].h_idx);
	}
}

static void wr_party(party_type *party_ptr) {
	/* Save the party name */
	wr_string(party_ptr->name);

	/* Save the owner's name */
	wr_string(party_ptr->owner);

	/* Save the number of people and creation time */
	wr_s32b(party_ptr->members);
	wr_s32b(party_ptr->created);

	/* Save the modus and members */
	wr_byte(party_ptr->mode);

	/* Save the creator's character mode */
	wr_byte(party_ptr->cmode);

	/* New - party flags, maybe */
	wr_u32b(party_ptr->flags);

}

static void wr_wild(wilderness_type *w_ptr) {
	/* Reserved for future use */
	wr_u32b(0);
	/* level flags */
	wr_u16b(w_ptr->type);
	wr_u16b(w_ptr->bled);
	wr_u32b(w_ptr->flags);

	wr_s32b(w_ptr->own);

	/* ondepth is not saved? */
}


/*
 * Write the information about a house
 */
static void wr_house(house_type *house) {
	int i;

	wr_byte(house->x);
	wr_byte(house->y);
	wr_byte(house->dx);
	wr_byte(house->dy);
	wr_u32b(house->dna->creator);
	wr_byte(house->dna->mode);
	wr_s32b(house->dna->owner);
	wr_byte(house->dna->owner_type);
	wr_byte(house->dna->a_flags);
	wr_u16b(house->dna->min_level);
	wr_u32b(house->dna->price);
	wr_u16b(house->flags);

	wr_s16b(house->wpos.wx);
	wr_s16b(house->wpos.wy);
	wr_s16b(house->wpos.wz);

	if (house->flags & HF_RECT){
		wr_byte(house->coords.rect.width);
		wr_byte(house->coords.rect.height);
	} else {
		i = -2;
		do {
			i += 2;
			wr_byte(house->coords.poly[i]);
			wr_byte(house->coords.poly[i+1]);
		} while (house->coords.poly[i] || house->coords.poly[i+1]);
	}

	wr_byte(house->colour);
	wr_byte(house->xtra);

#ifndef USE_MANG_HOUSE_ONLY
	wr_s16b(house->stock_num);
	wr_s16b(house->stock_size);
	for (i = 0; i < house->stock_num; i++)
		wr_item(&house->stock[i]);
#endif	// USE_MANG_HOUSE
}


/*
 * Write some "extra" info
 */
static void wr_extra(int Ind) {
	player_type *p_ptr = Players[Ind];

	int i, j;
	u16b tmp16u = 0;
	byte tmp8u = 0;

	dungeon_type *d_ptr;


	wr_string(p_ptr->name);

	wr_string(p_ptr->died_from);
	wr_string(p_ptr->died_from_list);
	wr_s16b(p_ptr->died_from_depth);

	for (i = 0; i < 4; i++)
		wr_string(p_ptr->history[i]);

	wr_byte(p_ptr->has_pet); //pet pet

	/* Race/Class/Gender/Party */
	wr_byte(p_ptr->prace);
	wr_byte(p_ptr->pclass);
	wr_byte(p_ptr->ptrait);
	wr_byte(p_ptr->male);
	wr_u16b(p_ptr->party); /* changed to u16b to allow more parties - mikaelh */
	wr_byte(p_ptr->mode);

	wr_byte(p_ptr->hitdie);
	wr_s16b(p_ptr->expfact);

	wr_s16b(p_ptr->age);
	wr_s16b(p_ptr->ht);
	wr_s16b(p_ptr->wt);
	wr_u16b(p_ptr->align_good);
	wr_u16b(p_ptr->align_law);

	/* Dump the stats (maximum and current) */
	for (i = 0; i < 6; ++i) wr_s16b(p_ptr->stat_max[i]);
	for (i = 0; i < 6; ++i) wr_s16b(p_ptr->stat_cur[i]);

	/* Dump the stats (maximum and current) */
	for (i = 0; i < 6; ++i) wr_s16b(p_ptr->stat_cnt[i]);
	for (i = 0; i < 6; ++i) wr_s16b(p_ptr->stat_los[i]);

	/* Dump the skills */
	wr_u16b(MAX_SKILLS);
	for (i = 0; i < MAX_SKILLS; ++i) {
		wr_s32b(p_ptr->s_info[i].value);
		wr_u16b(p_ptr->s_info[i].mod);
		wr_byte(p_ptr->s_info[i].dev);
#if 0 //SMOOTHSKILLS
		wr_byte(p_ptr->s_info[i].hidden);
		wr_byte(p_ptr->s_info[i].dummy);
#else
		wr_u32b(p_ptr->s_info[i].flags1);
		wr_s32b(p_ptr->s_info[i].base_value);
#endif
	}
	wr_s16b(p_ptr->skill_points);
	//wr_s16b(p_ptr->skill_last_level);

	/* /undoskills - mikaelh */
	for (i = 0; i < MAX_SKILLS; ++i) {
		wr_s32b(p_ptr->s_info_old[i].value);
		wr_u16b(p_ptr->s_info_old[i].mod);
		wr_byte(p_ptr->s_info_old[i].dev);
#if 0 //SMOOTHSKILLS
		wr_byte(p_ptr->s_info_old[i].hidden);
		wr_byte(p_ptr->s_info_old[i].dummy);
#else
		wr_u32b(p_ptr->s_info_old[i].flags1);
		wr_s32b(p_ptr->s_info_old[i].base_value);
#endif
	}
	wr_s16b(p_ptr->skill_points_old);
	wr_byte(p_ptr->reskill_possible);

	wr_s32b(p_ptr->id);
	wr_u32b(p_ptr->dna);
	wr_s32b(p_ptr->turn);
	wr_s32b(p_ptr->turns_online);
	wr_s32b(p_ptr->turns_afk);
	wr_s32b(p_ptr->turns_idle);
	wr_s32b(p_ptr->turns_active);

	/* Ignore the transient stats */
	for (i = 0; i < 10; ++i) wr_s16b(0);

	wr_u32b(p_ptr->au);

	wr_u32b(p_ptr->max_exp);
	wr_u32b(p_ptr->exp);
	wr_u16b(p_ptr->exp_frac);
	wr_s16b(p_ptr->lev);

	wr_s16b(p_ptr->mhp);
	wr_s16b(p_ptr->chp);
	wr_u16b(p_ptr->chp_frac);

	wr_s16b(p_ptr->mst);
	wr_s16b(p_ptr->cst);
	wr_s16b(p_ptr->cst_frac);

	wr_s16b(p_ptr->msp);
	wr_s16b(p_ptr->csp);
	wr_u16b(p_ptr->csp_frac);

	/* Max Player and Dungeon Levels */
	wr_s16b(p_ptr->max_plv);
	wr_s16b(p_ptr->max_dlv);
	for (i = 0; i < MAX_D_IDX * 2; i++) {
		d_ptr = NULL;
		/* hack: don't save max depth for quest dungeons! (they get removed when quest (stage) ends) */
		if (p_ptr->max_depth_wx[i] || p_ptr->max_depth_wy[i]) {
			if (p_ptr->max_depth_tower[i])
				d_ptr = wild_info[p_ptr->max_depth_wy[i]][p_ptr->max_depth_wx[i]].tower;
			else
				d_ptr = wild_info[p_ptr->max_depth_wy[i]][p_ptr->max_depth_wx[i]].dungeon;
		}
		if (d_ptr && d_ptr->quest) {
			wr_byte(0);
			wr_byte(0);
			wr_byte(0);
			wr_byte(0);
		} else {
			wr_byte(p_ptr->max_depth[i]);
			wr_byte(p_ptr->max_depth_wx[i]);
			wr_byte(p_ptr->max_depth_wy[i]);
			wr_byte(p_ptr->max_depth_tower[i] ? 1 : 0);
		}
	}

	/* Player location */
	wr_s16b(p_ptr->py);
	wr_s16b(p_ptr->px);

	wr_s16b(p_ptr->wpos.wx);
	wr_s16b(p_ptr->wpos.wy);
	wr_s16b(p_ptr->wpos.wz);

	wr_u16b(p_ptr->town_x);
	wr_u16b(p_ptr->town_y);

	/* More info */
	wr_s16b(p_ptr->ghost);
	wr_s16b(p_ptr->sc);
	wr_s16b(p_ptr->fruit_bat);

	wr_byte(p_ptr->lives);		/* old "rest" */
	wr_byte(p_ptr->houses_owned);

	wr_byte(0);			/* unused */
	wr_s16b(p_ptr->blind);
	wr_s16b(p_ptr->paralyzed);
	wr_s16b(p_ptr->confused);
	wr_s16b(p_ptr->food);
	wr_s32b(p_ptr->go_turn);
	wr_s16b(p_ptr->energy);
	wr_s16b(p_ptr->fast);
	wr_s16b(p_ptr->fast_mod);
	wr_s16b(p_ptr->slow);
	wr_s16b(p_ptr->afraid);
	wr_s16b(p_ptr->cut);
	wr_s16b(p_ptr->stun);
	wr_s16b(p_ptr->poisoned);
	wr_s16b(p_ptr->image);
	wr_s16b(p_ptr->protevil);
	wr_s16b(p_ptr->invuln);
	wr_s16b(p_ptr->hero);
	wr_s16b(p_ptr->shero);
	wr_s16b(p_ptr->body_monster_prev);
	wr_s16b(p_ptr->shield);
	wr_s16b(p_ptr->shield_power);
	wr_s16b(p_ptr->shield_opt);
	wr_s16b(p_ptr->shield_power_opt);
	wr_s16b(p_ptr->shield_power_opt2);
	wr_s16b(p_ptr->tim_thunder);
	wr_s16b(p_ptr->tim_thunder_p1);
	wr_s16b(p_ptr->tim_thunder_p2);
	wr_s16b(p_ptr->tim_lev);
	wr_s16b(p_ptr->tim_ffall);
	wr_s16b(p_ptr->tim_regen);
	wr_s16b(p_ptr->tim_regen_pow);
	wr_s16b(p_ptr->blessed);
	wr_s16b(p_ptr->tim_invis);
	wr_byte(p_ptr->go_level_top);//ENABLE_GO_GAME
	wr_byte(0x0);				//hole
	wr_s16b(p_ptr->see_infra);
	wr_s16b(p_ptr->tim_infra);
	wr_s16b(p_ptr->oppose_fire);
	wr_s16b(p_ptr->oppose_cold);
	wr_s16b(p_ptr->oppose_acid);
	wr_s16b(p_ptr->oppose_elec);
	wr_s16b(p_ptr->oppose_pois);
	wr_s16b(p_ptr->prob_travel);
	wr_s16b(p_ptr->st_anchor);
	wr_s16b(p_ptr->tim_esp);
	wr_s16b(p_ptr->adrenaline);
	wr_s16b(p_ptr->biofeedback);
	wr_byte(p_ptr->confusing);
	wr_u16b(p_ptr->tim_jail);
	wr_u16b(p_ptr->tim_susp);
	wr_u16b(p_ptr->pkill);
	wr_u16b(p_ptr->tim_pkill);
	wr_s16b(p_ptr->tim_wraith);
	wr_byte(p_ptr->wraith_in_wall);
	wr_byte(p_ptr->searching);
	wr_byte(p_ptr->maximize);
	wr_byte(p_ptr->preserve);
	wr_byte(p_ptr->stunning);

	wr_s16b(p_ptr->body_monster);
	wr_s16b(p_ptr->auto_tunnel);

	wr_s16b(p_ptr->tim_meditation);

	wr_s16b(p_ptr->tim_invisibility);
	wr_s16b(p_ptr->tim_invis_power);
	wr_s16b(p_ptr->tim_invis_power2);

	wr_s16b(p_ptr->fury);

	wr_s16b(p_ptr->tim_manashield);

	wr_s16b(p_ptr->tim_traps);
	wr_s16b(p_ptr->tim_mimic);
	wr_s16b(p_ptr->tim_mimic_what);

	/* Dump the monster lore */
	tmp16u = MAX_R_IDX;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++) {
		wr_s16b(p_ptr->r_killed[i]);
		wr_s16b(p_ptr->r_mimicry[i]);
	}

	wr_u32b(p_ptr->gold_picked_up);
	wr_byte(p_ptr->insta_res);
	wr_byte(p_ptr->castles_owned);
	wr_s16b(p_ptr->flash_self);
	wr_byte(p_ptr->fluent_artifact_reset); /* for automatic artifact resets */
	wr_byte(p_ptr->sanity_bar);
	wr_byte(p_ptr->IDDC_found_rndtown);
	wr_byte(p_ptr->IDDC_logscum);
	wr_byte(p_ptr->IDDC_flags);

	wr_s16b(p_ptr->word_recall);
	wr_s16b(p_ptr->recall_pos.wx);
	wr_s16b(p_ptr->recall_pos.wy);
	wr_s16b(p_ptr->recall_pos.wz);

	/* Future use */
	for (i = 0; i < 6; i++) wr_byte(0);

#ifdef SOLO_REKING
	wr_s32b(p_ptr->solo_reking);
	wr_s32b(p_ptr->solo_reking_au);
	wr_u32b(p_ptr->solo_reking_laston & 0xFFFFFFFF);
	if (sizeof(time_t) == 8) wr_u32b((p_ptr->solo_reking_laston & 0xFFFFFFFF00000000) >> 32);
	else wr_u32b(0);
#else
	for (i = 0; i < 16; i++) wr_byte(0);
#endif

	/* Toggle for possible automatic save-game updates
	   (done via script login-hook, eg custom.lua) - C. Blue */
	wr_byte(p_ptr->updated_savegame);

	/* for automatic artifact resets */
	wr_byte(p_ptr->artifact_reset);

#ifdef ENABLE_ITEM_ORDER
	wr_s16b(p_ptr->item_order_store);
	wr_byte(p_ptr->item_order_town);
	wr_byte(p_ptr->item_order_rarity);
	wr_s32b(p_ptr->item_order_turn);
	wr_s32b(p_ptr->item_order_cost);
	wr_item(&p_ptr->item_order_forge);
#else
	wr_s32b(0x0);
	wr_s32b(0x0);
	wr_s32b(0x0);
	wr_item(&p_ptr->inventory[INVEN_PACK]); //dummy
#endif

	/* Write the "object seeds" */
	/*wr_u32b(seed_flavor);*/
	/*wr_u32b(seed_town);*/
	wr_s32b(p_ptr->mimic_seed);
	wr_byte(p_ptr->mimic_immunity);
	wr_u16b(p_ptr->autoret);
	wr_s16b(p_ptr->martyr_timeout);

	/* Special stuff */
	wr_u16b(panic_save);
	wr_u16b(p_ptr->total_winner);
	wr_u16b(p_ptr->once_winner);
	wr_byte(p_ptr->iron_winner);
	wr_byte(p_ptr->iron_winner_ded);

	wr_s16b(p_ptr->own1.wx);
	wr_s16b(p_ptr->own1.wy);
	wr_s16b(p_ptr->own1.wz);
	wr_s16b(p_ptr->own2.wx);
	wr_s16b(p_ptr->own2.wy);
	wr_s16b(p_ptr->own2.wz);

	wr_u16b(p_ptr->retire_timer);
	wr_u16b(p_ptr->noscore);

	/* Write death */
	wr_byte(p_ptr->death);

	wr_byte(p_ptr->black_breath);

	wr_s16b(p_ptr->msane);
	wr_s16b(p_ptr->csane);
	wr_u16b(p_ptr->csane_frac);

	wr_s32b(p_ptr->balance);
	wr_s32b(p_ptr->tim_blacklist);
	wr_s32b(p_ptr->tim_watchlist);
	wr_s32b(p_ptr->pstealing);

	for (i = 0; i < MAX_GLOBAL_EVENTS; i++) {
		wr_s16b(p_ptr->global_event_type[i]);
		wr_s32b(p_ptr->global_event_signup[i]);
		wr_s32b(p_ptr->global_event_started[i]);
		for (j = 0; j < 4; j++) wr_u32b(p_ptr->global_event_progress[i][j]);
	}

	for (i = 0; i < MAX_GLOBAL_EVENT_TYPES; i++)
		wr_s16b(p_ptr->global_event_participated[i]);

	wr_s16b(p_ptr->combat_stance);
	wr_s16b(p_ptr->combat_stance_power);
	wr_byte(p_ptr->cloaked);
	wr_byte(p_ptr->shadow_running);
	wr_byte(p_ptr->shoot_till_kill);
	wr_byte(p_ptr->dual_mode);

	wr_s16b(p_ptr->kills);
	wr_s16b(p_ptr->kills_own);
	wr_s16b(p_ptr->kills_lower);
	wr_s16b(p_ptr->kills_higher);
	wr_s16b(p_ptr->kills_equal);
	wr_s16b(p_ptr->free_mimic);

	if (p_ptr->aura[0]) tmp8u |= 0x1;
	if (p_ptr->aura[1]) tmp8u |= 0x2;
	if (p_ptr->aura[2]) tmp8u |= 0x4;
	wr_byte(tmp8u); /* aura states (on/off) */

	wr_u16b(p_ptr->deaths);
	wr_u16b(p_ptr->soft_deaths);

	/* array of 'warnings' and hints aimed at newbies */
	tmp16u = 0x00;
	if (p_ptr->warning_technique_melee == 1) tmp16u |= 0x01;
	if (p_ptr->warning_technique_ranged == 1) tmp16u |= 0x02;
	if (p_ptr->warning_drained == 1) tmp16u |= 0x04;
	wr_u16b(tmp16u);

	wr_string(p_ptr->info_msg);

	/* for ENABLE_GO_GAME */
	wr_byte(p_ptr->go_level);
	wr_byte(p_ptr->go_sublevel);

	/* For things like 'Officer' status to add others etc */
	wr_u32b(p_ptr->party_flags);
	wr_u32b(p_ptr->guild_flags);

	/* Runecraft buff */
	wr_u16b(p_ptr->tim_deflect);

	/* for shuffling/dealing a deck of cards */
	wr_u16b(p_ptr->cards_diamonds);
	wr_u16b(p_ptr->cards_hearts);
	wr_u16b(p_ptr->cards_spades);
	wr_u16b(p_ptr->cards_clubs);
}

/*
 * Write the list of players a player is hostile toward
 */
static void wr_hostilities(int Ind) {
	player_type *p_ptr = Players[Ind];
	hostile_type *h_ptr;
	int i;
	u16b tmp16u = 0;

	/* Count hostilities */
	for (h_ptr = p_ptr->hostile; h_ptr; h_ptr = h_ptr->next) {
		/* One more */
		tmp16u++;
	}

	/* Save number */
	wr_u16b(tmp16u);

	/* Start at beginning */
	h_ptr = p_ptr->hostile;

	/* Save each record */
	for (i = 0; i < tmp16u; i++) {
		/* Write ID */
		wr_s32b(h_ptr->id);

		/* Advance pointer */
		h_ptr = h_ptr->next;
	}
}




/*
 * Write a specified depth
 *
 * Each row is broken down into a series of run-length encoded runs.
 * Each run has a constant feature type, and flags.
 *
 * Note that a cave_type's monster index and object indecies are not stored
 * here.  They should be assigned automatically when the objects
 * and monsters are loaded later.
 *
 * This could probably be made more efficient by allowing runs to encompass
 * more than one row.
 *
 * We could also probably get a large efficiency increase by splitting the features
 * and flags into two seperate run-length encoded blocks.
 *
 * -APD
 */

static void wr_floor(struct worldpos *wpos) {
	int y, x, i;
	byte prev_feature = 0xff, n;
	u32b prev_info = 0xffffffff;
	unsigned char runlength;
	struct c_special *cs_ptr;

	cave_type *c_ptr;
	cave_type **zcave;
	if (!(zcave = getcave(wpos))) return;
#if DEBUG_LEVEL > 1
//#if 1 > 0
	s_printf("%d players on %s.\n", players_on_depth(wpos), wpos_format(0, wpos));
#endif

	/* Depth */
	wr_s16b(wpos->wx);
	wr_s16b(wpos->wy);
	wr_s16b(wpos->wz);

	/* Dungeon size */
	wr_u16b(MAX_HGT);
	wr_u16b(MAX_WID);

	/* How many players are on this depth */
	wr_s16b(players_on_depth(wpos));

	/* The staircase locations on this depth */
	/* Hack -- this information is currently not present for the wilderness
	 * levels.
	 */

	wr_byte(level_up_y(wpos));
	wr_byte(level_up_x(wpos));
	wr_byte(level_down_y(wpos));
	wr_byte(level_down_x(wpos));
	wr_byte(level_rand_y(wpos));
	wr_byte(level_rand_x(wpos));

	{
		dun_level *l_ptr = getfloor(wpos);
		if (l_ptr) {
			wr_u32b(l_ptr->id);
			wr_u32b(l_ptr->flags1);
			wr_u32b(l_ptr->flags2);
			wr_byte(l_ptr->hgt);
			wr_byte(l_ptr->wid);
		}
	}

	/*** Simple "Run-Length-Encoding" of cave ***/
	/* for each each row */
	for (y = 0; y < MAX_HGT; y++) {
		/* start a new run */
		runlength = 0;

		/* break the row down into runs */
		for (x = 0; x < MAX_WID; x++) {
			c_ptr = &zcave[y][x];

			/* if we are starting a new run */
			if ((!runlength) || (c_ptr->feat != prev_feature) || (c_ptr->info != prev_info)
					|| (runlength > 254))
			{
				if (runlength) {
					/* if we just finished a run, write it */
					wr_byte(runlength);
					wr_byte(prev_feature);
					wr_u32b(prev_info);
				}

				/* start a new run */
				prev_feature = c_ptr->feat;
				prev_info = c_ptr->info;
				runlength = 1;
			}
			/* otherwise continue our current run */
			else runlength++;
		}
		/* hack -- write the final run of this row */
		wr_byte(runlength);
		wr_byte(prev_feature);
		wr_u32b(prev_info);
	}

	/*** another scan for c_special ***/
	/* for each each row */
	for (y = 0; y < MAX_HGT; y++) {
		/* break the row down into runs */
		for (x = 0; x < MAX_WID; x++) {
			n = 0;
			c_ptr = &zcave[y][x];
			cs_ptr = c_ptr->special;
			/* count the number of cs_things */
			while (cs_ptr) {
				n++;
				cs_ptr = cs_ptr->next;
			}
			/* write them */
			if (n) {
				wr_byte(x);
				wr_byte(y);
				wr_byte(n);
				cs_ptr = c_ptr->special;
				while (cs_ptr) {
					i = cs_ptr->type;
					wr_byte(i);
					csfunc[i].save(cs_ptr);
					cs_ptr = cs_ptr->next;
				}
			}
#if 0
		/* redesign needed - too much of hurry to sort now */
			while (cs_ptr) {
				i = cs_ptr->type;

				/* nothing special */
				if (i != CS_NONE){

					/* TODO: implement CS_DNADOOR and CS_KEYDOOR saving
			 		* currently, their x,y,i is saved in vain.	- Jir -
			 		*/
					wr_byte(x);
					wr_byte(y);
					wr_byte(i);

					/* csfunc will take care of it :) */
					csfunc[i].save(sc_is_pointer(i) ?
					    cs_ptr->sc.ptr : &c_ptr->special);
				}
				cs_ptr = cs_ptr->next;
			}
#endif /* 0 */
		}
	}

	/* hack -- terminate it */
	wr_byte(255);
	wr_byte(255);
	wr_byte(255);
}

/* Write a players memory of a cave, simmilar to the above function. */
static void wr_cave_memory(int Ind) {
	player_type *p_ptr = Players[Ind];
	int y,x;
	char prev_flag = -1;	/* just for definedness. */
	unsigned char runlength = 0;

	/* Remember unique ID of the old floor */
	wr_u32b(p_ptr->dlev_id);

	/* write the number of flags */
	wr_u16b(MAX_HGT);
	wr_u16b(MAX_WID);

	for (x = y = 0; y < MAX_HGT;) {
		/* if we are starting a new run */
		if (!runlength || (p_ptr->cave_flag[y][x] != prev_flag) || (runlength > 254) ) {
			/* if we just finished a run, write it */
			if (runlength) {
				wr_byte(runlength);
				wr_byte(prev_flag);
			}
			/* star the new run */
			prev_flag = p_ptr->cave_flag[y][x];
			runlength = 1;
		}
		/* else lengthen our previous run */
		else runlength++;

		/* update our current position */
		x++;
		if (x >= MAX_WID) {
			x = 0; y++;
			if (y >= MAX_HGT) {
				/* hack -- write the final run */
				wr_byte(runlength);
				wr_byte(prev_flag);
				break;
			}
		}
	}
}



/*
 * Actually write a player save-file
 */
static bool wr_savefile_new(int Ind) {
	player_type *p_ptr = Players[Ind];
	int i, j;

	u32b now, tmp32u;
	byte tmp8u;
	u16b tmp16u;


	/* Guess at the current time */
	now = time((time_t *)0);


	/* Note the operating system */
	sf_xtra = 0L;

	/* Note when the file was saved */
	sf_when = now;

	/* Note the number of saves */
	sf_saves++;


	/*** Actually write the file ***/

	/* Dump the file header */
	xor_byte = 0;
	wr_byte(SF_VERSION_MAJOR);
	xor_byte = 0;
	wr_byte(SF_VERSION_MINOR);
	xor_byte = 0;
	wr_byte(SF_VERSION_PATCH);
	xor_byte = 0;
	tmp8u = rand_int(256);
	wr_byte(tmp8u);


	/* Reset the checksum */
	v_stamp = 0L;
	x_stamp = 0L;


	/* Operating system */
	wr_u32b(sf_xtra);


	/* Time file last saved */
	wr_u32b(sf_when);

	/* Number of past lives */
	wr_u16b(sf_lives);

	/* Number of times saved */
	wr_u16b(sf_saves);


	/* Space */
	wr_u32b(0L);
	wr_u32b(0L);


	/* Write the boolean "options" */
	/*wr_options();*/


#if 0
	/* Dump the monster lore */
	tmp16u = MAX_R_IDX;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++) wr_lore(i);
#endif


	/* Dump the object memory */
	tmp16u = MAX_K_IDX;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++) wr_xtra(Ind, i);

	/* Dump the trap memory (unefficient!) */
	tmp16u = MAX_T_IDX;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++) wr_trap_memory(Ind, i);

#if 0

	/* Hack -- Dump the quests */
	tmp16u = MAX_XO_IDX;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++) {
		wr_byte(xo_list[i].level);
		wr_byte(0);
		wr_byte(0);
		wr_byte(0);
	}

	/* Hack -- Dump the artifacts */
	tmp16u = MAX_A_IDX;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++) {
		artifact_type *a_ptr = &a_info[i];
		wr_byte(a_ptr->cur_num);
		wr_byte(0);
		wr_byte(0);
		wr_byte(0);
	}

#endif


	/* Write the "extra" information */
	wr_extra(Ind);


	/* Dump the "player hp" entries */
	tmp16u = PY_MAX_LEVEL;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++)
		wr_s16b(p_ptr->player_hp[i]);

	/* Write the inventory */
	for (i = 0; i < INVEN_TOTAL; i++) {
		if (p_ptr->inventory[i].k_idx) {
			wr_u16b(i);
			wr_item(&p_ptr->inventory[i]);
		}
	}

	/* Add a sentinel */
	wr_u16b(0xFFFF);

	/* Write the list of hostilities */
	wr_hostilities(Ind);

	/* write the cave flags (our memory of our current level) */
	wr_cave_memory(Ind);
	/* write the wilderness map */
	tmp32u = MAX_WILD_8;
	wr_u32b(tmp32u);
	for (i = 0; i < MAX_WILD_8; i++)
		wr_byte(p_ptr->wild_map[i]);

	wr_byte(p_ptr->guild);
	wr_u32b(p_ptr->guild_dna);

	wr_s16b(p_ptr->xorder_id);
	wr_s16b(p_ptr->xorder_num);


	/* Quest information */
	wr_byte(p_ptr->quest_any_k);
	wr_byte(p_ptr->quest_any_k_target);
	wr_byte(p_ptr->quest_any_k_within_target);
	wr_byte(p_ptr->quest_any_r);
	wr_byte(p_ptr->quest_any_r_target);
	wr_byte(p_ptr->quest_any_r_within_target);
	wr_byte(p_ptr->quest_any_deliver_xy);
	wr_byte(p_ptr->quest_any_deliver_xy_within_target);

	for (i = 0; i < MAX_CONCURRENT_QUESTS; i++) {
		wr_s16b(p_ptr->quest_idx[i]);
		wr_string(p_ptr->quest_codename[i]);
		wr_s16b(p_ptr->quest_stage[i]);
		wr_s16b(p_ptr->quest_stage_timer[i]);
		for (j = 0; j < QI_GOALS; j++) {
			wr_byte(p_ptr->quest_goals[i][j]);
			wr_s16b(p_ptr->quest_kill_number[i][j]);
			wr_s16b(p_ptr->quest_retrieve_number[i][j]);
			/* hack: added in 4, 5, 26 actually: */
			wr_byte(p_ptr->quest_goals_nisi[i][j]);
		}
		for (j = 0; j < 5; j++) {
			/* the 'missing' byte to strip is instead used for 'nisi' above */
	// FREE SPACE:
			wr_s16b(0);
			wr_s16b(0);
		}

		/* helper info */
		wr_byte(p_ptr->quest_kill[i]);
		wr_byte(p_ptr->quest_retrieve[i]);

		wr_byte(p_ptr->quest_deliver_pos[i]);
		wr_byte(p_ptr->quest_deliver_xy[i]);

		wr_s32b(p_ptr->quest_acquired[i]);
		wr_s32b(p_ptr->quest_timed_stage_change[i]);

		/* 'individual' quest type information */
		wr_u16b(p_ptr->quest_flags[i]);
	}

	/* remember quests we completed */
	for (i = 0; i < MAX_Q_IDX; i++) {
		wr_s16b(p_ptr->quest_done[i]);
		wr_s16b(p_ptr->quest_cooldown[i]);
	}


	wr_byte(p_ptr->spell_project);

	/* Special powers */
	wr_s16b(p_ptr->power_num);
	for (i = 0; i < MAX_POWERS; i++)
		wr_s16b(p_ptr->powers[i]);

	/* Write the "value check-sum" */
	wr_u32b(v_stamp);

	/* Write the "encoded checksum" */
	wr_u32b(x_stamp);

	/* Write the remaining contents of the buffer */
	write_buffer();

	/* Error in save */
	if (ferror(fff) || (fflush(fff) == EOF)) return FALSE;

	/* Successful save */
	return TRUE;
}


/*
 * Medium level player saver
 *
 * XXX XXX XXX Angband 2.8.0 will use "fd" instead of "fff" if possible
 */
static bool save_player_aux(int Ind, char *name) {
	bool ok = FALSE;
	int fd = -1;
	int mode = 0644;


	/* No file yet */
	fff = NULL;


	/* File type is "SAVE" */
	FILE_TYPE(FILE_TYPE_SAVE);


	/* Create the savefile */
	fd = fd_make(name, mode);

	/* File is okay */
	if (fd >= 0) {
		/* Close the "fd" */
		(void)fd_close(fd);

		/* Open the savefile */
		fff = my_fopen(name, "wb");

		/* Successful open */
		if (fff) {
			/* Allocate a buffer */
			fff_buf = C_NEW(MAX_BUF_SIZE, char);
			fff_buf_pos = 0;

			/* Write the savefile */
			if (wr_savefile_new(Ind)) ok = TRUE;

			/* Attempt to close it */
			if (my_fclose(fff)) ok = FALSE;

			/* Free the buffer */
			C_FREE(fff_buf, MAX_BUF_SIZE, char);
		}

		/* Remove "broken" files */
		if (!ok) (void)fd_kill(name);
	}


	/* Failure */
	if (!ok) return (FALSE);

	/* Successful save */
	/*server_saved = TRUE;*/

	/* Success */
	return (TRUE);
}



/*
 * Attempt to save the player in a savefile
 */
bool save_player(int Ind) {
	player_type *p_ptr = Players[Ind];
	int result = FALSE;
	char safe[1024];


#ifdef SET_UID

# ifdef SECURE

	/* Get "games" permissions */
	beGames();

# endif

#endif


	/* New savefile */
	strcpy(safe, p_ptr->savefile);
	strcat(safe, ".new");

#ifdef VM
	/* Hack -- support "flat directory" usage on VM/ESA */
	strcpy(safe, p_ptr->savefile);
	strcat(safe, "n");
#endif /* VM */

	/* Remove it */
	fd_kill(safe);

	/* Attempt to save the player */
	if (save_player_aux(Ind, safe)) {
		char temp[1024];

		/* Old savefile */
		strcpy(temp, p_ptr->savefile);
		strcat(temp, ".old");

#ifdef VM
		/* Hack -- support "flat directory" usage on VM/ESA */
		strcpy(temp, p_ptr->savefile);
		strcat(temp, "o");
#endif /* VM */

		/* Remove it */
		fd_kill(temp);

		/* Preserve old savefile */
		fd_move(p_ptr->savefile, temp);

		/* Activate new savefile */
		fd_move(safe, p_ptr->savefile);

		/* Remove preserved savefile */
		fd_kill(temp);

		/* Hack -- Pretend the character was loaded */
		/*character_loaded = TRUE;*/

#ifdef VERIFY_SAVEFILE

		/* Lock on savefile */
		strcpy(temp, savefile);
		strcat(temp, ".lok");

		/* Remove lock file */
		fd_kill(temp);

#endif

		/* Success */
		result = TRUE;
	}


#ifdef SET_UID

# ifdef SECURE

	/* Drop "games" permissions */
	bePlayer();

# endif

#endif


	/* Return the result */
	return (result);
}


static bool file_exist(char *buf) {
	int fd;

	fd = fd_open(buf, O_RDONLY);
	if (fd >= 0) {
		fd_close(fd);
		return (TRUE);
	}
	else return (FALSE);
}


/*
 * Attempt to Load a "savefile"
 *
 * Version 2.7.0 introduced a slightly different "savefile" format from
 * older versions, requiring a completely different parsing method.
 *
 * Note that savefiles from 2.7.0 - 2.7.2 are completely obsolete.
 *
 * Pre-2.8.0 savefiles lose some data, see "load2.c" for info.
 *
 * Pre-2.7.0 savefiles lose a lot of things, see "load1.c" for info.
 *
 * On multi-user systems, you may only "read" a savefile if you will be
 * allowed to "write" it later, this prevents painful situations in which
 * the player loads a savefile belonging to someone else, and then is not
 * allowed to save his game when he quits.
 *
 * We return "TRUE" if the savefile was usable, and we set the global
 * flag "character_loaded" if a real, living, character was loaded.
 *
 * Note that we always try to load the "current" savefile, even if
 * there is no such file, so we must check for "empty" savefile names.
 */
bool load_player(int Ind) {
	player_type *p_ptr = Players[Ind];

	int		fd = -1;

	errr	err = 0;

	byte	vvv[4];

#ifdef VERIFY_TIMESTAMP
	struct stat	statbuf;
#endif

	cptr	what = "generic";
	bool	edit = (cfg.runlevel == 1024) ? TRUE : FALSE;


	/* Paranoia */
	/*turn = 0;*/

	/* Paranoia */
	p_ptr->death = FALSE;


	/* Allow empty savefile name */
	if (!p_ptr->savefile[0]) {
		if (edit) {
			what = "Server is closed for login now";
			err = 1;
		}
		else return (TRUE);
	}


	/* XXX XXX XXX Fix this */

	/* Verify the existence of the savefile */
	if (!err && !file_exist(p_ptr->savefile)) {
		/* Give a message */
		s_printf("Savefile does not exist for player %s.\n", p_ptr->name);
		s_printf("(%s) %d\n", p_ptr->savefile, errno);

		if (errno != ENOENT && errno != EIO) {	/* EMFILE for example */
			if (cfg.runlevel > 1) {
				s_printf("Automatic shutdown\n");
				msg_broadcast(0, "\377r* SERVER EMERGENCY SHUTDOWN *");
				set_runlevel(1);
			}
			return(FALSE);
		}

		/* Allow this */
		if (edit) {
			what = "Server is closed for login now";
			err = 1;
		}
		else return (TRUE);
	}


#ifdef VERIFY_SAVEFILE
	/* Verify savefile usage */
	if (!err) {
		FILE *fkk;

		char temp[MAX_PATH_LENGTH];

		/* Extract name of lock file */
		strcpy(temp, p_ptr->savefile);
		strcat(temp, ".lok");

		/* Check for lock */
		fkk = my_fopen(temp, "rb");

		/* Oops, lock exists */
		if (fkk)
		{
			/* Close the file */
			my_fclose(fkk);

			/* Message */
			msg_print(Ind, "Savefile is currently in use.");
			msg_print(Ind, NULL);

			/* Oops */
			return (FALSE);
		}

		/* Create a lock file */
		fkk = my_fopen(temp, "wb");

		/* Dump a line of info */
		fprintf(fkk, "Lock file for savefile '%s'\n", p_ptr->savefile);

		/* Close the lock file */
		my_fclose(fkk);
	}
#endif


	/* Okay */
	if (!err) {
		/* Open the savefile */
		fd = fd_open(p_ptr->savefile, O_RDONLY);

		/* No file */
		if (fd < 0) err = -1;

		/* Message (below) */
		if (err) what = "Cannot open savefile";
	}

	/* Process file */
	if (!err) {
#ifdef VERIFY_TIMESTAMP
		/* Get the timestamp */
		(void)fstat(fd, &statbuf);
#endif

		/* Read the first four bytes */
		if (fd_read(fd, (char*)(vvv), 4)) err = -1;

		/* What */
		if (err) what = "Cannot read savefile";

		/* Close the file */
		(void)fd_close(fd);
	}

	/* Process file */
	if (!err) {
		/* Extract version */
		sf_major = vvv[0];
		sf_minor = vvv[1];
		sf_patch = vvv[2];
		sf_extra = vvv[3];

		/* Attempt to load */
		err = rd_savefile_new(Ind);

		/* Message (below) */
		/*
		   if (err) {
		   what = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
		   (void)sprintf (what,"Cannot parse savefile error %d",err);
		   };
		   */
		if (err) what = "Cannot parse savefile error";
		if (err == 35) what = edit ? "Server is closed for login now" :
			"Incorrect password";
		if (err == 1) what = "Name already in use";
	}

	/* Paranoia */
	if (!err) {
		/* Invalid turn */
		/* if (!turn) err = -1; */

		/* Message (below) */
		if (err) what = "Broken savefile";
	}

#ifdef VERIFY_TIMESTAMP
	/* Verify timestamp */
	if (!err) {
		/* Hack -- Verify the timestamp */
		if (sf_when > (statbuf.st_ctime + 100) ||
		    sf_when < (statbuf.st_ctime - 100)) {
			/* Message */
			what = "Invalid timestamp";

			/* Oops */
			err = -1;
		}
	}
#endif


	/* Okay */
	if (!err) {
		/* Give a conversion warning */
		if ((version_major != sf_major) ||
		    (version_minor != sf_minor) ||
		    (version_patch != sf_patch)) {
			/* Message */
			printf("Converted a %d.%d.%d savefile.\n",
					sf_major, sf_minor, sf_patch);
		}

		/* Player is dead */
		if (p_ptr->death) {
			/* Player is no longer "dead" */
			p_ptr->death = FALSE;

			/* Count lives */
			sf_lives++;

			/* Done */
			return (TRUE);
		}

		/* A character was loaded */
		character_loaded = TRUE;

		/* Still alive */
		if (p_ptr->chp >= 0) {
			/* Reset cause of death */
			(void)strcpy(p_ptr->died_from, "(alive and well)");
		}

		p_ptr->body_changed = TRUE;

		/* Success */
		return (TRUE);
	}


#ifdef VERIFY_SAVEFILE

	/* Verify savefile usage */
	if (TRUE) {
		char temp[MAX_PATH_LENGTH];

		/* Extract name of lock file */
		strcpy(temp, p_ptr->savefile);
		strcat(temp, ".lok");

		/* Remove lock */
		fd_kill(temp);
	}

#endif


	/* Message */
	Destroy_connection(p_ptr->conn, format("Error (%s) reading %d.%d.%d savefile.",
				what, sf_major, sf_minor, sf_patch));

	/* Oops */
	return (FALSE);
}

/*
 * Write the player name hash table.
 * Much better to write it from here...
 */
static void wr_player_names(void) {
	hash_entry *ptr;

	/*int i, num, *id_list;*/
	s32b *id_list;
	u32b i, num;

	/* Get the list of player ID's */
	num = player_id_list(&id_list, 0L);

	/* Store the number of entries */
	wr_u32b(num);

	/* Store each entry */
	for (i = 0; i < num; i++) {
		/* Store the ID */
		ptr = lookup_player(id_list[i]);
		if (ptr) {
			wr_s32b(id_list[i]);
			wr_u32b(ptr->account);
			wr_s32b(ptr->laston);
			/* 3.4.2 server */
			wr_byte(ptr->race);
			wr_byte(ptr->class);
			wr_byte(ptr->mode);
			wr_byte(ptr->level);
			wr_u16b(ptr->party); /* changed to u16b to allow more parties */
			wr_byte(ptr->guild);
			wr_u32b(ptr->guild_flags);
			wr_u16b(ptr->xorder);
			wr_byte(ptr->admin);
			wr_s16b(ptr->wpos.wx);
			wr_s16b(ptr->wpos.wy);
			wr_s16b(ptr->wpos.wz);
			/* Store the player name */
			wr_string(ptr->name);
			wr_byte(ptr->houses);
			wr_byte(ptr->winner);
		}
	}

	/* Free the memory in the list */
	if (num) C_KILL(id_list, num, int);
}

static void wr_auctions()
{
	int i, j;
	auction_type *auc_ptr;
	bid_type *bid_ptr;

	wr_u32b(auction_alloc);

	for (i = 0; i < auction_alloc; i++)
	{
		auc_ptr = &auctions[i];
		wr_byte(auc_ptr->status);
		wr_byte(auc_ptr->flags);
		wr_byte(auc_ptr->mode);
		wr_s32b(auc_ptr->owner);
		wr_item(&auc_ptr->item);
		if (auc_ptr->desc)
		{
			wr_string(auc_ptr->desc);
		}
		else
		{
			wr_string("");
		}
		wr_s32b(auc_ptr->starting_price);
		wr_s32b(auc_ptr->buyout_price);
		wr_s32b(auc_ptr->bids_cnt);
		for (j = 0; j < auc_ptr->bids_cnt; j++) {
			bid_ptr = &auc_ptr->bids[j];
			wr_s32b(bid_ptr->bid);
			wr_s32b(bid_ptr->bidder);
		}
		wr_s32b(auc_ptr->winning_bid);

		/* Write time_t's as s32b's for now */
		wr_s32b((s32b) auc_ptr->start);
		wr_s32b((s32b) auc_ptr->duration);
	}
}

static bool wr_server_savefile() {
	int i;
	u32b now;

	byte tmp8u;
	u16b tmp16u;
	u32b tmp32u;


	/* Guess at the current time */
	now = time((time_t *)0);


	/* Note the operating system */
	sf_xtra = 0L;

	/* Note when the file was saved */
	sf_when = now;

	/* Note the number of saves */
	sf_saves++;


	/*** Actually write the file ***/

	/* Dump the file header */
	xor_byte = 0;
	wr_byte(SF_VERSION_MAJOR);
	xor_byte = 0;
	wr_byte(SF_VERSION_MINOR);
	xor_byte = 0;
	wr_byte(SF_VERSION_PATCH);
	xor_byte = 0;
	tmp8u = rand_int(256);
	wr_byte(tmp8u);


	/* Reset the checksum */
	v_stamp = 0L;
	x_stamp = 0L;


	/* Operating system */
	wr_u32b(sf_xtra);

	/* Time file last saved */
	wr_u32b(sf_when);

	/* Number of past lives */
	wr_u16b(sf_lives);

	/* Number of times saved */
	wr_u16b(sf_saves);

	/* Is this a panic save? - C. Blue */
	if (panic_save) wr_byte(1); else wr_byte(0);

	/* save server state regarding updates (lua) */
	wr_s16b(updated_server);
	/* save artifact-reset state (lua) */
	wr_s16b(artifact_reset);

	/* Space */
	wr_u32b(0L);
	wr_u32b(0L);

	/* Dump the monster (unique) race information */
	tmp16u = MAX_R_IDX;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++) wr_global_lore(i);

	/* Hack -- Dump the artifacts */
	tmp16u = MAX_A_IDX;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++) {
		artifact_type *a_ptr = &a_info[i];
		wr_byte(a_ptr->cur_num);
		wr_byte(a_ptr->known);
		wr_s32b(a_ptr->carrier);
		wr_s32b(a_ptr->timeout);
		wr_byte((a_ptr->iddc ? 0x1 : 0) + (a_ptr->winner ? 0x2 : 0));
	}


	/* Note the parties */
	tmp16u = MAX_PARTIES;
	wr_u16b(tmp16u);

	/* Dump the parties */
	for (i = 0; i < tmp16u; i++) wr_party(&parties[i]);


	wr_towns();		/* write town data */
	new_wr_wild();		/* must alloc dungeons first on load! ;) */
	new_wr_floors();	/* rename wr_floors(void) later */

	/* Prepare to write the monsters */
	compact_monsters(0, FALSE);
	/* Note the number of monsters */
	tmp32u = m_max - 1;
	wr_u32b(tmp32u);
	/* Dump the monsters */
	for (i = 1; i < m_max; i++) wr_monster(&m_list[i]);

	/* Prepare to write the objects */
	compact_objects(0, FALSE);
	/* Note the number of objects */
	tmp16u = o_max;
	wr_u16b(tmp16u);
	/* Dump the objects */
	for (i = 0; i < tmp16u; i++) wr_item(&o_list[i]);

	tmp32u = 0L;
	for(i = 0; i < num_houses; i++)
		if (!(houses[i].flags&HF_DELETED)) tmp32u++;

	/* Note the number of houses */
	wr_s32b(tmp32u);

	/* Dump the houses */
	for (i = 0; i < num_houses; i++){
		if (!(houses[i].flags&HF_DELETED))
			wr_house(&houses[i]);
	}

	/* Write the player name database */
	wr_player_names();

	wr_u32b(seed_flavor);
	wr_u32b(seed_town);

	wr_guilds();
	wr_xorders();
	//wr_quests();

	wr_u32b(account_id);
	wr_s32b(player_id);
	wr_s32b(turn);

	wr_notes();
	wr_bbs();

	wr_byte(season);
	wr_byte(weather_frequency);

	wr_auctions();

	/* write Ironman Deep Dive Challenge records */
	wr_byte(IDDC_HIGHSCORE_SIZE);
	for (i = 0; i < IDDC_HIGHSCORE_SIZE; i++) {
		wr_s16b(deep_dive_level[i]);
		wr_string(deep_dive_name[i]);
		wr_string(deep_dive_char[i]);
		wr_string(deep_dive_account[i]);
		wr_s16b(deep_dive_class[i]);
	}

	//#ifdef ENABLE_MERCHANT_MAIL
	wr_mail();

	/* Write the remaining contents of the buffer */
	write_buffer();

	/* Error in save */
	if (ferror(fff) || (fflush(fff) == EOF)) return FALSE;

	/* Successful save */
	return TRUE;
}

/* write the wilderness and dungeon structure */
static void new_wr_wild() {
	wilderness_type *w_ptr;
	int x, y, i;
	u32b temp;

	temp = MAX_WILD_Y;
	wr_u32b(temp);
	temp = MAX_WILD_X;
	wr_u32b(temp);

	for (y = 0; y < MAX_WILD_Y; y++) {
		for (x = 0; x < MAX_WILD_X; x++) {
			w_ptr = &wild_info[y][x];
			wr_wild(w_ptr);
			if (w_ptr->flags & WILD_F_DOWN) {
				wr_byte(w_ptr->up_x);
				wr_byte(w_ptr->up_y);
				wr_u16b(w_ptr->dungeon->id);
				wr_u16b(w_ptr->dungeon->type);
				wr_u16b(w_ptr->dungeon->baselevel);
				wr_u32b(w_ptr->dungeon->flags1);
				wr_u32b(w_ptr->dungeon->flags2);
				wr_u32b(w_ptr->dungeon->flags3);
				wr_byte(w_ptr->dungeon->maxdepth);
				for(i = 0; i < 10; i++) {
#if 0	/* unused - mikaelh */
					wr_byte(w_ptr->dungeon->r_char[i]);
					wr_byte(w_ptr->dungeon->nr_char[i]);
#else
					wr_byte(0);
					wr_byte(0);
#endif
				}
#ifdef DUNGEON_VISIT_BONUS
				wr_u16b(dungeon_visit_frequency[w_ptr->dungeon->id]);
#else
				wr_u16b(VISIT_TIME_CAP);
#endif

				wr_byte(w_ptr->dungeon->theme);
				wr_s16b(w_ptr->dungeon->quest);
				wr_s16b(w_ptr->dungeon->quest_stage);
#ifdef GLOBAL_DUNGEON_KNOWLEDGE
				wr_byte(w_ptr->dungeon->known);
#else
				wr_byte(0);
#endif
			}
			if (w_ptr->flags & WILD_F_UP) {
				wr_byte(w_ptr->dn_x);
				wr_byte(w_ptr->dn_y);
				wr_u16b(w_ptr->tower->id);
				wr_u16b(w_ptr->tower->type);
				wr_u16b(w_ptr->tower->baselevel);
				wr_u32b(w_ptr->tower->flags1);
				wr_u32b(w_ptr->tower->flags2);
				wr_u32b(w_ptr->tower->flags3);
				wr_byte(w_ptr->tower->maxdepth);
				for (i = 0; i < 10; i++){
#if 0	/* unused - mikaelh */
					wr_byte(w_ptr->tower->r_char[i]);
					wr_byte(w_ptr->tower->nr_char[i]);
#else
					wr_byte(0);
					wr_byte(0);
#endif
				}
#ifdef DUNGEON_VISIT_BONUS
				wr_u16b(dungeon_visit_frequency[w_ptr->tower->id]);
#else
				wr_u16b(VISIT_TIME_CAP);
#endif
				wr_byte(w_ptr->tower->theme);
				wr_s16b(w_ptr->tower->quest);
				wr_s16b(w_ptr->tower->quest_stage);
#ifdef GLOBAL_DUNGEON_KNOWLEDGE
				wr_byte(w_ptr->tower->known);
#else
				wr_byte(0);
#endif
			}
		}
	}
}

/* write the actual dungeons */
static void new_wr_floors() {
	struct worldpos cwpos;
	wilderness_type *w_ptr;
	int x, y, z;
	cwpos.wz = 0;
	for(y = 0; y < MAX_WILD_Y; y++) {
		cwpos.wy = y;
		for(x = 0; x < MAX_WILD_X; x++) {
			cwpos.wx = x;
			w_ptr = &wild_info[y][x];
			save_guildhalls(&cwpos);
			/*
			 * One problem here; if a wilderness tower/dungeon exists, and
			 * the surface is not static, stair/recall informations are lost
			 * and cause crash/infinite-loop next time.		FIXME
			 */
			if (getcave(&cwpos) && players_on_depth(&cwpos)) wr_floor(&cwpos);
			if (w_ptr->flags & WILD_F_DOWN) {
				struct dungeon_type *d_ptr = w_ptr->dungeon;
				for (z = 1; z <= d_ptr->maxdepth; z++) {
					cwpos.wz = -z;
					if (d_ptr->level[z - 1].ondepth && d_ptr->level[z - 1].cave)
						wr_floor(&cwpos);
				}
			}
			if (w_ptr->flags & WILD_F_UP) {
				struct dungeon_type *d_ptr = w_ptr->tower;
				for (z = 1; z <= d_ptr->maxdepth; z++) {
					cwpos.wz = z;
					if (d_ptr->level[z - 1].ondepth && d_ptr->level[z - 1].cave)
						wr_floor(&cwpos);
				}
			}
			cwpos.wz = 0;
		}
	}
	wr_s16b(0x7fff);	/* this could fail if we had 32767^3 areas */
	wr_s16b(0x7fff);	/* I cant see that happening for a while */
	wr_s16b(0x7fff);
}

static bool save_server_aux(char *name) {
	bool	ok = FALSE;
	int	fd = -1;
	int	mode = 0644;


	/* No file yet */
	fff = NULL;

	/* File type is "SAVE" */
	FILE_TYPE(FILE_TYPE_SAVE);

	/* Create the savefile */
	fd = fd_make(name, mode);

	/* File is okay */
	if (fd >= 0) {
		/* Close the "fd" */
		(void)fd_close(fd);

		/* Open the savefile */
		fff = my_fopen(name, "wb");

		/* Successful open */
		if (fff) {
			/* Allocate a buffer */
			fff_buf = C_NEW(MAX_BUF_SIZE, char);
			fff_buf_pos = 0;

			/* Write the savefile */
			if (wr_server_savefile()) ok = TRUE;

			/* Attempt to close it */
			if (my_fclose(fff)) ok = FALSE;

			/* Free the buffer */
			C_FREE(fff_buf, MAX_BUF_SIZE, char);
		}

		/* Remove "broken" files */
		if (!ok) (void)fd_kill(name);
	}


	/* Failure */
	if (!ok) return (FALSE);

	/* Successful save */
	/*server_saved = TRUE;*/

	/* Success */
	return (TRUE);
}


/*
 * Load the whole server info from one special savefile.
 */
static bool load_server_info_classic(void) {
	int fd = -1;
	byte vvv[4];
	errr err = 0;
	cptr what = "generic";
	char buf[1024];

	path_build(buf, 1024, ANGBAND_DIR_SAVE, "server");

	/* XXX XXX XXX Fix this */
	if (!file_exist(buf)) {
		/* Give message */
		s_printf("Server savefile does not exist\n");

		/* Allow this */
		return (TRUE);
	}

	/* Okay */
	if (!err) {
		/* Open the savefile */
		fd = fd_open(buf, O_RDONLY);

		/* No file */
		if (fd < 0) err = -1;

		/* Message (below) */
		if (err) what = "Cannot open server savefile";
	}

	/* Process file */
	if (!err) {
		/* Read the first four bytes */
		if (fd_read(fd, (char*)(vvv), 4)) err = -1;

		/* What */
		if (err) what = "Cannot read server savefile";

		/* Close the file */
		(void)fd_close(fd);
	}

	/* Process file */
	if (!err) {
		/* Extract version */
		sf_major = vvv[0];
		sf_minor = vvv[1];
		sf_patch = vvv[2];
		sf_extra = vvv[3];

		/* Parse "MAngband" savefiles */
		/* If I ever catch the one that put that STUPID UGLY IDIOT
		 * HACK there he will know what *WRATH* means ... -- DG
		 */

		/* Attempt to load */
		err = rd_server_savefile();

		/* Message (below) */
		/*
		   if (err) {
		   what = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXx";
		   (void)sprintf (what,"Cannot parse savefile error %d",err);
		   };
		   */
		if (err) what ="Cannot parse server savefile error %d";
	}

	/* Okay */
	if (!err) {
		/* Give a conversion warning */
		if ((version_major != sf_major) ||
		    (version_minor != sf_minor) ||
		    (version_patch != sf_patch)) {
			/* Message */
			printf("Converted a %d.%d.%d server savefile.\n",
					sf_major, sf_minor, sf_patch);
		}

		/* The server state was loaded */
		server_state_loaded = TRUE;

		/* Success */
		return (TRUE);
	}

	/* Message */
	s_printf("Error (%s) reading a %d.%d.%d server savefile.\n", what, sf_major, sf_minor, sf_patch);

	return (FALSE);
}

/* Load the complete server info, either
 *   -from one giant 'server' save file (old way) or
 *   -from multiple partial 'server0'..'serverN' save files (new):
 *	server0		wilderness, dungeons, towns, houses
 *	server1		item flavours
 *	server2		parties, guilds, bbs
 */
bool load_server_info(void) {
	char buf[1024];

	/* check for existence of old huge server save file */
	path_build(buf, 1024, ANGBAND_DIR_SAVE, "server");
	if (file_exist(buf)) {
//		s_printf("Found classic 'server' savefile\n");
		return load_server_info_classic();
	}

	/* check for existence of partial server save files */

	//TODO


	/* regenerate server savefile */
	s_printf("Server savefile does not exist\n");

	/* Allow this */
	return (TRUE);
}

/*
 * Save the server state to a "server" savefile.
 */
bool save_server_info() {
	int result = FALSE;
	char safe[MAX_PATH_LENGTH];

#if DEBUG_LEVEL > 1
	printf("saving server info...\n");
#endif
	/* New savefile */
	path_build(safe, MAX_PATH_LENGTH, ANGBAND_DIR_SAVE, "server.new");

	/* Remove it */
	fd_kill(safe);

	/* Attempt to save the server state */
	if (save_server_aux(safe)) {
		char temp[MAX_PATH_LENGTH];
		char prev[MAX_PATH_LENGTH];

		/* Old savefile */
		path_build(temp, MAX_PATH_LENGTH, ANGBAND_DIR_SAVE, "server.old");

		/* Remove it */
		fd_kill(temp);

		/* Name of previous savefile */
		path_build(prev, MAX_PATH_LENGTH, ANGBAND_DIR_SAVE, "server");

		/* Preserve old savefile */
		fd_move(prev, temp);

		/* Activate new savefile */
		fd_move(safe, prev);

		/* Remove preserved savefile */
		fd_kill(temp);

		/* Success */
		result = TRUE;
	}

	/* Return the result */
	return (result);
}

void wr_towns() {
	int i, j;
	wr_u16b(numtowns);
	for (i = 0; i < numtowns; i++){
		wr_u16b(town[i].x);
		wr_u16b(town[i].y);
		wr_u16b(town[i].baselevel);
		wr_u16b(town[i].flags);
		wr_u16b(town[i].num_stores);
		wr_u16b(town[i].type);

		/* Dump the stores */
		for (j = 0; j < town[i].num_stores; j++){
			wr_store(&town[i].townstore[j]);
		}
	}
}

void save_banlist(void) {
	char buf[1024];
	FILE *fp;
	struct combo_ban *ptr;

	path_build(buf, 1024, ANGBAND_DIR_CONFIG, "banlist.txt");

	fp = fopen(buf, "w");
	if (!fp) return;

	for (ptr = banlist; ptr != (struct combo_ban*)NULL; ptr = ptr->next)
		fprintf(fp, "%s|%s|%s|%d|%s\n", ptr->acc[0] ? ptr->acc : " ", ptr->ip[0] ? ptr->ip : " ", ptr->hostname[0] ? ptr->hostname : " ", ptr->time, ptr->reason[0] ? ptr->reason : " "); /* omg fu scanf^^ */
	fclose(fp);
}

/* ---------- Save dynamic quest data.  ---------- */
/* This cannot be done in the server savefile, because it gets read before all
   ?_info.txt files are read from lib/data. So for example the remaining number
   of kills in a kill-quest cannot be stored anywhere, since the stage and
   stage goals are not yet initialised. Oops.
   However, saving this quest data of random/varying lenght is a mess anyway,
   so it's good that we keep it far away from the server savefile. */
static bool save_quests_file(void) {
	int i, j, k;
	u32b now;

	byte tmp8u;


	quest_info *q_ptr;

	qi_questor *q_questor;
	qi_goal *q_goal;
	qi_stage *q_stage;


	now = time((time_t *)0);
	/* Note the operating system */
	sf_xtra = 0L;
	/* Note when the file was saved */
	sf_when = now;

	/* Dump the file header */
	xor_byte = 0;
	wr_byte(QUEST_SF_VERSION_MAJOR);
	xor_byte = 0;
	wr_byte(QUEST_SF_VERSION_MINOR);
	xor_byte = 0;
	wr_byte(QUEST_SF_VERSION_PATCH);
	xor_byte = 0;
	tmp8u = rand_int(256);
	wr_byte(tmp8u);

	/* Reset the checksum */
	v_stamp = 0L;
	x_stamp = 0L;

	wr_u32b(sf_xtra);
	wr_u32b(sf_when);

	/* begin writing the actual quest data */

	wr_s16b(max_q_idx);
	for (i = 0; i < max_q_idx; i++) {
		q_ptr = &q_info[i];

		//to recognize the quest (ID)
		wr_string(q_ptr->codename);
		wr_string(q_ptr->creator);
		wr_u16b(q_ptr->name);

		//main quest data
		wr_byte(q_ptr->active);
#if 0 /* 0'ed for now, use 'x' disable feature from q_info.txt exclusively. */
		wr_byte(q_ptr->disabled);
#else
		wr_byte(0);
#endif

		wr_s16b(q_ptr->cur_cooldown);
		wr_s32b(q_ptr->turn_activated);
		wr_s32b(q_ptr->turn_acquired);

		wr_s16b(q_ptr->cur_stage);
		wr_u16b(q_ptr->flags);
		wr_byte(q_ptr->tainted);
		wr_s16b(q_ptr->objects_registered);

		wr_s16b(q_ptr->timed_countdown);
		wr_s16b(q_ptr->timed_countdown_stage);
		wr_byte(q_ptr->timed_countdown_quiet);

		//dynamically generated random passwords
		for (j = 0; j < QI_PASSWORDS; j++)
			wr_string(q_ptr->password[j]);

		//questors:
		wr_byte(q_ptr->questors);
		for (j = 0; j < q_ptr->questors; j++) {
			q_questor = &q_ptr->questor[j];

			wr_s16b(q_questor->current_wpos.wx);
			wr_s16b(q_questor->current_wpos.wy);
			wr_s16b(q_questor->current_wpos.wz);

			wr_s16b(q_questor->current_x);
			wr_s16b(q_questor->current_y);

			wr_s16b(q_questor->mo_idx);
			wr_s16b(q_questor->talk_focus);//not needed

			wr_byte(q_questor->tainted);
		}

		//stages:
		wr_byte(q_ptr->stages);
		for (j = 0; j < q_ptr->stages; j++) {
			q_stage = &q_ptr->stage[j];

			//questor hostility timers:
			for (k = 0; k < q_ptr->questors; k++) {
				if (q_stage->questor_hostility[k])
					wr_s16b(q_stage->questor_hostility[k]->hostile_revert_timed_countdown);
				else
					wr_s16b(9999);
			}

			//goals:
			wr_byte(q_stage->goals);
			for (k = 0; k < q_stage->goals; k++) {
				q_goal = &q_stage->goal[k];

				wr_byte(q_goal->cleared);
				wr_byte(q_goal->nisi);

				//kill goals:
				if (q_goal->kill) wr_s16b(q_goal->kill->number_left);
				else wr_s16b(-1); /* actually write a value for recognising, in case quest has been edited meanwhile */
			}
		}
	}

	/* finish up */

	/* Write the remaining contents of the buffer */
	write_buffer();
	/* Error in save */
	if (ferror(fff) || (fflush(fff) == EOF)) return FALSE;
	/* Successful save */
	return TRUE;
}
static bool save_quests_aux(char *name) {
	bool	ok = FALSE;
	int	fd = -1;
	int	mode = 0644;

	fff = NULL;
	FILE_TYPE(FILE_TYPE_SAVE);
	fd = fd_make(name, mode);
	if (fd >= 0) {
		(void)fd_close(fd);
		fff = my_fopen(name, "wb");
		if (fff) {
			fff_buf = C_NEW(MAX_BUF_SIZE, char);
			fff_buf_pos = 0;
			if (save_quests_file()) ok = TRUE;
			if (my_fclose(fff)) ok = FALSE;
			C_FREE(fff_buf, MAX_BUF_SIZE, char);
		}
		if (!ok) (void)fd_kill(name);
	}
	if (!ok) return FALSE;
	return TRUE;
}
void save_quests(void) {
	//int result = FALSE;
	char safe[MAX_PATH_LENGTH];

#if DEBUG_LEVEL > 1
	printf("saving quest info...\n");
#endif
	path_build(safe, MAX_PATH_LENGTH, ANGBAND_DIR_SAVE, "quests.new");
	fd_kill(safe);

	if (save_quests_aux(safe)) {
		char temp[MAX_PATH_LENGTH];
		char prev[MAX_PATH_LENGTH];

		path_build(temp, MAX_PATH_LENGTH, ANGBAND_DIR_SAVE, "quests.old");
		fd_kill(temp);
		path_build(prev, MAX_PATH_LENGTH, ANGBAND_DIR_SAVE, "quests");
		fd_move(prev, temp);
		fd_move(safe, prev);
		fd_kill(temp);
		//result = TRUE;
	}
	return;// (result);
}
