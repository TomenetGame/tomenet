/* $Id$ */
/* File: save.c */

/* Purpose: interact with savefiles */

#define SERVER

#include "angband.h"
#include "party.h"

static void new_wr_wild();
static void new_wr_dungeons();
void wr_towns();

#ifdef FUTURE_SAVEFILES

/*
 * XXX XXX XXX Ignore this for now...
 *
 * The basic format of Angband 2.8.0 (and later) savefiles is simple.
 *
 * The savefile itself is a "header" (4 bytes) plus a series of "blocks",
 * plus, perhaps, some form of "footer" at the end.
 *
 * The "header" contains information ab ut the "version" of the savefile.
 * Conveniently, pre-2.8.0 savefiles also use a 4 byte header, though the
 * interpretation of the "sf_extra" byte is very different.  Unfortunately,
 * savefiles from Angband 2.5.X reverse the sf_major and sf_minor fields,
 * and must be handled specially, until we decide to start ignoring them.
 *
 * Each "block" is a "type" (2 bytes), plus a "size" (2 bytes), plus "data",
 * plus a "check" (2 bytes), plus a "stamp" (2 bytes).  The format of the
 * "check" and "stamp" bytes is still being contemplated, but it would be
 * nice for one to be a simple byte-checksum, and the other to be a complex
 * additive checksum of some kind.  Both should be zero if the block is empty.
 *
 * Standard types:
 *   TYPE_BIRTH --> creation info
 *   TYPE_OPTIONS --> option settings
 *   TYPE_MESSAGES --> message recall
 *   TYPE_PLAYER --> player information
 *   TYPE_SPELLS --> spell information
 *   TYPE_INVEN --> player inven/equip
 *   TYPE_STORES --> store information
 *   TYPE_RACES --> monster race data
 *   TYPE_KINDS --> object kind data
 *   TYPE_UNIQUES --> unique info
 *   TYPE_ARTIFACTS --> artifact info
 *   TYPE_QUESTS --> quest info
 *
 * Dungeon information:
 *   TYPE_DUNGEON --> dungeon info
 *   TYPE_FEATURES --> dungeon features
 *   TYPE_OBJECTS --> dungeon objects
 *   TYPE_MONSTERS --> dungeon monsters
 *
 * Conversions:
 *   Break old "races" into normals/uniques
 *   Extract info about the "unique" monsters
 *
 * Question:
 *   Should there be a single "block" for info about all the stores, or one
 *   "block" for each store?  Or one "block", which contains "sub-blocks" of
 *   some kind?  Should we dump every "sub-block", or just the "useful" ones?
 *
 * Question:
 *   Should the normals/uniques be broken for 2.8.0, or should 2.8.0 simply
 *   be a "fixed point" into which older savefiles are converted, and then
 *   future versions could ignore older savefiles, and the "conversions"
 *   would be much simpler.
 */


/*
 * XXX XXX XXX
 */
#define TYPE_OPTIONS 17362


/*
 * Hack -- current savefile
 */
static int data_fd = -1;


/*
 * Hack -- current block type
 */
static u16b data_type;

/*
 * Hack -- current block size
 */
static u16b data_size;

/*
 * Hack -- pointer to the data buffer
 */
static byte *data_head;

/*
 * Hack -- pointer into the data buffer
 */
static byte *data_next;


/*
 * Hack -- write the current "block" to the savefile
 */
static errr wr_block(void)
{
	errr err;

	byte fake[4];

	/* Save the type and size */
	fake[0] = (byte)(data_type);
	fake[1] = (byte)(data_type >> 8);
	fake[2] = (byte)(data_size);
	fake[3] = (byte)(data_size >> 8);

	/* Dump the head */
	err = fd_write(data_fd, (char*)&fake, 4);

	/* Dump the actual data */
	err = fd_write(data_fd, (char*)data_head, data_size);

	/* XXX XXX XXX */
	fake[0] = 0;
	fake[1] = 0;
	fake[2] = 0;
	fake[3] = 0;

	/* Dump the tail */
	err = fd_write(data_fd, (char*)&fake, 4);

	/* Hack -- reset */
	data_next = data_head;

	/* Wipe the data block */
	C_WIPE(data_head, 65535, byte);

	/* Success */
	return (0);
}



/*
 * Hack -- add data to the current block
 */
static void put_byte(byte v)
{
	*data_next++ = v;
}

/*
 * Hack -- add data to the current block
 */
static void put_char(char v)
{
	put_byte((byte)(v));
}

/*
 * Hack -- add data to the current block
 */
static void put_u16b(u16b v)
{
	*data_next++ = (byte)(v);
	*data_next++ = (byte)(v >> 8);
}

/*
 * Hack -- add data to the current block
 */
static void put_s16b(s16b v)
{
	put_u16b((u16b)(v));
}

/*
 * Hack -- add data to the current block
 */
static void put_u32b(u32b v)
{
	*data_next++ = (byte)(v);
	*data_next++ = (byte)(v >> 8);
	*data_next++ = (byte)(v >> 16);
	*data_next++ = (byte)(v >> 24);
}

/*
 * Hack -- add data to the current block
 */
static void put_s32b(s32b v)
{
	put_u32b((u32b)(v));
}

/*
 * Hack -- add data to the current block
 */
static void put_string(char *str)
{
	while ((*data_next++ = *str++) != '\0');
}



/*
 * Write a savefile for Angband 2.8.0
 */
static errr wr_savefile(void)
{
	int		i;

	u32b	now;

	byte	tmp8u;
	u16b	tmp16u;

	errr	err;

	byte	fake[4];


	/*** Hack -- extract some data ***/

	/* Hack -- Acquire the current time */
	now = time((time_t*)(NULL));

	/* Note the operating system */
	sf_xtra = 0L;

	/* Note when the file was saved */
	sf_when = now;

	/* Note the number of saves */
	sf_saves++;


	/*** Actually write the file ***/

	/* Open the file XXX XXX XXX */
	data_fd = -1;

	/* Dump the version */
	fake[0] = (byte)(VERSION_MAJOR);
	fake[1] = (byte)(VERSION_MINOR);
	fake[2] = (byte)(VERSION_PATCH);
	fake[3] = (byte)(VERSION_EXTRA);

	/* Dump the data */
	err = fd_write(data_fd, (char*)&fake, 4);


	/* Make array XXX XXX XXX */
	C_MAKE(data_head, 65535, byte);

	/* Hack -- reset */
	data_next = data_head;


#if 0
	/* Operating system */
	wr_u32b(sf_xtra);

	/* Time file last saved */
	wr_u32b(sf_when);

	/* Number of past lives */
	wr_byte(sf_lives);
	/* XXX XXX XXX */
	wr_byte(0);

	/* Number of times saved */
	wr_u16b(sf_saves);

	/* XXX XXX XXX */

	/* Set the type */
	data_type = TYPE_BASIC;

	/* Set the "options" size */
	data_size = (data_next - data_head);

	/* Write the block */
	wr_block();
#endif


	/* Dump the "options" */
	put_options();

	/* Set the type */
	data_type = TYPE_OPTIONS;

	/* Set the "options" size */
	data_size = (data_next - data_head);

	/* Write the block */
	wr_block();

	/* XXX XXX XXX */

#if 0

	/* Dump the monster lore */
	tmp16u = MAX_R_IDX;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++) wr_lore(i);


	/* Dump the object memory */
	tmp16u = MAX_K_IDX;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++) wr_xtra(i);


	/* Hack -- Dump the quests */
	tmp16u = MAX_Q_IDX;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++)
	{
		wr_byte(q_list[i].level);
		wr_byte(0);
		wr_byte(0);
		wr_byte(0);
	}

	/* Hack -- Dump the artifacts */
	tmp16u = MAX_A_IDX;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++)
	{
		artifact_type *a_ptr = &a_info[i];
		wr_byte(a_ptr->cur_num);
		wr_byte(0);
		wr_byte(0);
		wr_byte(0);
	}



	/* Write the "extra" information */
	wr_extra();


	/* Dump the "player hp" entries */
	tmp16u = PY_MAX_LEVEL;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++)
	{
		wr_s16b(player_hp[i]);
	}


        /* Write spell data */
	wr_u16b(MAX_REALM);
        for (j = 0; j < MAX_REALM; j++)
        {
                wr_u32b(spell_learned1[j]);
                wr_u32b(spell_learned2[j]);
                wr_u32b(spell_worked1[j]);
                wr_u32b(spell_worked2[j]);
                wr_u32b(spell_forgotten1[j]);
                wr_u32b(spell_forgotten2[j]);

                /* Dump the ordered spells */
                for (i = 0; i < 64; i++)
                {
                        wr_byte(spell_order[j][i]);
                }
        }


	/* Write the inventory */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		if (inventory[i].k_idx)
		{
			wr_u16b(i);
			wr_item(&inventory[i]);
		}
	}

	/* Add a sentinel */
	wr_u16b(0xFFFF);


	/* Note the stores */
	tmp16u = MAX_STORES;
	wr_u16b(tmp16u);

	/* Dump the stores */
	for (i = 0; i < tmp16u; i++) wr_store(&store[i]);


	/* Player is not dead, write the dungeon */
	if (!death)
	{
		/* Dump the dungeon */
		wr_dungeon();

		/* Dump the ghost */
		wr_ghost();
	}

#endif

	/* Dump the "final" marker XXX XXX XXX */
	/* Type zero, Size zero, Contents zero, etc */


	/* XXX XXX XXX Check for errors */


	/* Kill array XXX XXX XXX */
	C_KILL(data_head, 65535, byte);


	/* Success */
	return (0);
}





/*
 * Hack -- read the next "block" from the savefile
 */
static errr rd_block(void)
{
	errr err;

	byte fake[4];

	/* Read the head data */
	err = fd_read(data_fd, (char*)&fake, 4);

	/* Extract the type and size */
	data_type = (fake[0] | ((u16b)fake[1] << 8));
	data_size = (fake[2] | ((u16b)fake[3] << 8));

	/* Wipe the data block */
	C_WIPE(data_head, 65535, byte);

	/* Read the actual data */
	err = fd_read(data_fd, (char*)data_head, data_size);

	/* Read the tail data */
	err = fd_read(data_fd, (char*)&fake, 4);

	/* XXX XXX XXX Verify */

	/* Hack -- reset */
	data_next = data_head;

	/* Success */
	return (0);
}


/*
 * Hack -- get data from the current block
 */
static void get_byte(byte *ip)
{
	byte d1;
	d1 = (*data_next++);
	(*ip) = (d1);
}

/*
 * Hack -- get data from the current block
 */
static void get_char(char *ip)
{
	get_byte((byte*)ip);
}

/*
 * Hack -- get data from the current block
 */
static void get_u16b(u16b *ip)
{
	u16b d0, d1;
	d0 = (*data_next++);
	d1 = (*data_next++);
	(*ip) = (d0 | (d1 << 8));
}

/*
 * Hack -- get data from the current block
 */
static void get_s16b(s16b *ip)
{
	get_u16b((u16b*)ip);
}

/*
 * Hack -- get data from the current block
 */
static void get_u32b(u32b *ip)
{
	u32b d0, d1, d2, d3;
	d0 = (*data_next++);
	d1 = (*data_next++);
	d2 = (*data_next++);
	d3 = (*data_next++);
	(*ip) = (d0 | (d1 << 8) | (d2 << 16) | (d3 << 24));
}

/*
 * Hack -- get data from the current block
 */
static void get_s32b(s32b *ip)
{
	get_u32b((u32b*)ip);
}



/*
 * Read a savefile for Angband 2.8.0
 */
static errr rd_savefile(void)
{
	bool	done = FALSE;

	byte	fake[4];


	/* Open the savefile */
	data_fd = fd_open(savefile, O_RDONLY);

	/* No file */
	if (data_fd < 0) return (1);

	/* Strip the first four bytes (see below) */
	if (fd_read(data_fd, (char*)(fake), 4)) return (1);


	/* Make array XXX XXX XXX */
	C_MAKE(data_head, 65535, byte);

	/* Hack -- reset */
	data_next = data_head;


	/* Read blocks */
	while (!done)
	{
		/* Read the block */
		if (rd_block()) break;

		/* Analyze the type */
		switch (data_type)
		{
			/* Done XXX XXX XXX */
			case 0:
				{
					done = TRUE;
					break;
				}

				/* Grab the options */
			case TYPE_OPTIONS:
				{
					if (get_options()) err = -1;
					break;
				}
		}

		/* XXX XXX XXX verify "data_next" */
		if (data_next - data_head > data_size) break;
	}


	/* XXX XXX XXX Check for errors */


	/* Kill array XXX XXX XXX */
	C_KILL(data_head, 65535, byte);


	/* Success */
	return (0);
}


#endif /* FUTURE_SAVEFILES */




/*
 * Some "local" parameters, used to help write savefiles
 */

static FILE	*fff;		/* Current save "file" */

static byte	xor_byte;	/* Simple encryption */

static u32b	v_stamp = 0L;	/* A simple "checksum" on the actual values */
static u32b	x_stamp = 0L;	/* A simple "checksum" on the encoded bytes */



/*
 * These functions place information into a savefile a byte at a time
 */

static void sf_put(byte v)
{
	/* Encode the value, write a character */
	xor_byte ^= v;
	(void)putc((int)xor_byte, fff);

	/* Maintain the checksum info */
	v_stamp += v;
	x_stamp += xor_byte;
}

void wr_byte(byte v)
{
	sf_put(v);
}

void wr_u16b(u16b v)
{
	sf_put(v & 0xFF);
	sf_put((v >> 8) & 0xFF);
}

void wr_s16b(s16b v)
{
	wr_u16b((u16b)v);
}

void wr_u32b(u32b v)
{
	sf_put(v & 0xFF);
	sf_put((v >> 8) & 0xFF);
	sf_put((v >> 16) & 0xFF);
	sf_put((v >> 24) & 0xFF);
}

void wr_s32b(s32b v)
{
	wr_u32b((u32b)v);
}

void wr_string(cptr str)
{
	while (*str)
	{
		wr_byte(*str);
		str++;
	}
	wr_byte(*str);
}


/*
 * These functions write info in larger logical records
 */


/*
 * Write an "item" record
 */
static void wr_item(object_type *o_ptr)
{
	wr_s32b(o_ptr->owner);
	wr_s16b(o_ptr->level);

	wr_s16b(o_ptr->k_idx);

	wr_byte(o_ptr->iy);
	wr_byte(o_ptr->ix);

	wr_s16b(o_ptr->wpos.wx);
	wr_s16b(o_ptr->wpos.wy);
	wr_s16b(o_ptr->wpos.wz);

	wr_byte(o_ptr->tval);
	wr_byte(o_ptr->sval);
	wr_s32b(o_ptr->bpval);
	wr_s32b(o_ptr->pval);

	wr_byte(o_ptr->discount);
	wr_byte(o_ptr->number);
	wr_s16b(o_ptr->weight);

	wr_byte(o_ptr->name1);
	wr_byte(o_ptr->name2);
	wr_s32b(o_ptr->name3);
	wr_s16b(o_ptr->timeout);

	wr_s16b(o_ptr->to_h);
	wr_s16b(o_ptr->to_d);
	wr_s16b(o_ptr->to_a);
	wr_s16b(o_ptr->ac);
	wr_byte(o_ptr->dd);
	wr_byte(o_ptr->ds);

	wr_byte(o_ptr->ident);

//	wr_byte(0);
	wr_byte(o_ptr->name2b);

	wr_u32b(0L);
	wr_u32b(0L);
	wr_u32b(0L);

	wr_u16b(0);

	wr_byte(o_ptr->xtra1);
	wr_byte(o_ptr->xtra2);

	/* Save the inscription (if any) */
	if (o_ptr->note)
	{
		wr_string(quark_str(o_ptr->note));
	}
	else
	{
		wr_string("");
	}
	wr_u16b(o_ptr->next_o_idx);
	wr_u16b(o_ptr->held_m_idx);
}

#if 0	// DELETEME
/*
 * Write a "trap" record
 */
static void wr_trap(trap_type *t_ptr)
{

	wr_s16b(t_ptr->wpos.wx);
	wr_s16b(t_ptr->wpos.wy);
	wr_s16b(t_ptr->wpos.wz);

	wr_byte(t_ptr->t_idx);
	wr_byte(t_ptr->iy);
	wr_byte(t_ptr->ix);
	wr_byte(t_ptr->found);
}
#endif	// 0

/*
 * Write a "monster" record
 */
static void wr_monster_race(monster_race *r_ptr)
{
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
	wr_byte(r_ptr->freq_inate);
	wr_byte(r_ptr->freq_spell);
	wr_u32b(r_ptr->flags1);
	wr_u32b(r_ptr->flags2);
	wr_u32b(r_ptr->flags3);
	wr_u32b(r_ptr->flags4);
	wr_u32b(r_ptr->flags5);
	wr_u32b(r_ptr->flags6);
	wr_s16b(r_ptr->level);
	wr_byte(r_ptr->rarity);
	wr_byte(r_ptr->d_attr);
	wr_byte(r_ptr->d_char);
	wr_byte(r_ptr->x_attr);
	wr_byte(r_ptr->x_char);
	for (i = 0; i < 4; i++)
	{
		wr_byte(r_ptr->blow[i].method);
		wr_byte(r_ptr->blow[i].effect);
		wr_byte(r_ptr->blow[i].d_dice);
		wr_byte(r_ptr->blow[i].d_side);
	}
}

/*
 * Write a "monster" record
 */
static void wr_monster(monster_type *m_ptr)
{
	int i;

	wr_byte(m_ptr->special);
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
	for (i = 0; i < 4; i++)
	{
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
	wr_u16b(m_ptr->hold_o_idx);
	wr_u16b(m_ptr->clone);
	wr_s16b(m_ptr->mind);

	if (m_ptr->special)
	{
		wr_monster_race(m_ptr->r_ptr);
	}

	wr_u16b(m_ptr->ego);
	wr_s32b(m_ptr->name3);
}

/*
 * Write a "lore" record
 */
static void wr_lore(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* Count sights/deaths/kills */
	wr_s16b(r_ptr->r_sights);
	wr_s16b(r_ptr->r_deaths);
	wr_s16b(r_ptr->r_pkills);
	wr_s16b(r_ptr->r_tkills);

	/* Count wakes and ignores */
	wr_byte(r_ptr->r_wake);
	wr_byte(r_ptr->r_ignore);

	/* Save the amount of time until the unique respawns */
	wr_s32b(r_ptr->respawn_timer);

	/* Count drops */
	wr_byte(r_ptr->r_drop_gold);
	wr_byte(r_ptr->r_drop_item);

	/* Count spells */
	wr_byte(r_ptr->r_cast_inate);
	wr_byte(r_ptr->r_cast_spell);

	/* Count blows of each type */
	wr_byte(r_ptr->r_blows[0]);
	wr_byte(r_ptr->r_blows[1]);
	wr_byte(r_ptr->r_blows[2]);
	wr_byte(r_ptr->r_blows[3]);

	/* Memorize flags */
	wr_u32b(r_ptr->r_flags1);
	wr_u32b(r_ptr->r_flags2);
	wr_u32b(r_ptr->r_flags3);
	wr_u32b(r_ptr->r_flags4);
	wr_u32b(r_ptr->r_flags5);
	wr_u32b(r_ptr->r_flags6);


	/* Monster limit per level */
	wr_byte(r_ptr->max_num);

	/* Killer */
	//	wr_s32b(r_ptr->killer);

	/* Later (?) */
	wr_byte(0);
	wr_byte(0);
	wr_byte(0);
}


/*
 * Write an "xtra" record
 */
static void wr_xtra(int Ind, int k_idx)
{
	player_type *p_ptr = Players[Ind];
	byte tmp8u = 0;

	if (p_ptr->obj_aware[k_idx]) tmp8u |= 0x01;
	if (p_ptr->obj_tried[k_idx]) tmp8u |= 0x02;

	wr_byte(tmp8u);
}

/*
 * Write a "trap memory" record
 *
 * Of course, we should be able to compress this into 1/8..	- Jir -
 */
static void wr_trap_memory(int Ind, int t_idx)
{
	player_type *p_ptr = Players[Ind];
	byte tmp8u = 0;

	if (p_ptr->trap_ident[t_idx]) tmp8u |= 0x01;

	wr_byte(tmp8u);
}


/*
 * Write a "store" record
 */
static void wr_store(store_type *st_ptr)
{
	int j;

	/* Save the "open" counter */
	wr_u32b(st_ptr->store_open);

	/* Save the "insults" */
	wr_s16b(st_ptr->insult_cur);

	/* Save the current owner */
	wr_byte(st_ptr->owner);

	/* Save the stock size */
	wr_byte(st_ptr->stock_num);

	/* Save the "haggle" info */
	wr_s16b(st_ptr->good_buy);
	wr_s16b(st_ptr->bad_buy);

	/* Save the stock */
	for (j = 0; j < st_ptr->stock_num; j++)
	{
		/* Save each item in stock */
		wr_item(&st_ptr->stock[j]);
	}
}

static void wr_quests(){
	int i;
	wr_s16b(questid);
	for(i=0; i<20; i++){
		wr_s16b(quests[i].active);
		wr_s16b(quests[i].id);
		wr_s16b(quests[i].type);
		wr_u16b(quests[i].flags);
		wr_s32b(quests[i].creator);
	}
}

static void wr_guilds(){
	int i;
	u16b tmp16u;

	tmp16u = MAX_GUILDS;
	wr_u16b(tmp16u);

	/* Dump the guilds */
	for (i = 0; i < tmp16u; i++){
		wr_string(guilds[i].name);
		wr_s32b(guilds[i].master);
		wr_s32b(guilds[i].num);
		wr_u32b(guilds[i].flags);
		wr_s16b(guilds[i].minlev);
	}
}

static void wr_party(party_type *party_ptr)
{
	/* Save the party name */
	wr_string(party_ptr->name);

	/* Save the owner's name */
	wr_string(party_ptr->owner);

	/* Save the number of people and creation time */
	wr_s32b(party_ptr->num);
	wr_s32b(party_ptr->created);
}

static void wr_wild(wilderness_type *w_ptr)
{
	/* Reserved for future use */
	wr_u32b(0);
	/* level flags */
	wr_u16b(w_ptr->type);
	wr_u32b(w_ptr->flags);

	wr_s32b(w_ptr->own);

	/* ondepth is not saved? */
}

/*
 * Write RNG state
 */
#if 0
static errr wr_randomizer(void)
{
	int i;

	/* Zero */
	wr_u16b(0);

	/* Place */
	wr_u16b(Rand_place);

	/* State */
	for (i = 0; i < RAND_DEG; i++)
	{
		wr_u32b(Rand_state[i]);
	}

	/* Success */
	return (0);
}
#endif

/*
 * Hack -- Write the "ghost" info
 */
#if 0
static void wr_ghost(void)
{
	int i;

	/* Name */
	wr_string("Broken Ghost");

	/* Hack -- stupid data */
	for (i = 0; i < 60; i++) wr_byte(0);
}
#endif


/*
 * Write the information about a house
 */
static void wr_house(house_type *house)
{
	wr_byte(house->x);
	wr_byte(house->y);
	wr_byte(house->dx);
	wr_byte(house->dy);
	wr_u32b(house->dna->creator);
	wr_u32b(house->dna->owner);
	wr_byte(house->dna->owner_type);
	wr_byte(house->dna->a_flags);
	wr_u16b(house->dna->min_level);
	wr_u32b(house->dna->price);
	wr_u16b(house->flags);

	wr_s16b(house->wpos.wx);
	wr_s16b(house->wpos.wy);
	wr_s16b(house->wpos.wz);

	if(house->flags & HF_RECT){
		wr_byte(house->coords.rect.width);
		wr_byte(house->coords.rect.height);
	}
	else{
		int i=-2;
		do{
			i+=2;
			wr_byte(house->coords.poly[i]);
			wr_byte(house->coords.poly[i+1]);
		}while(house->coords.poly[i] || house->coords.poly[i+1]);
	}
}


/*
 * Write some "extra" info
 */
static void wr_extra(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int i;
	u16b tmp16u;

	wr_string(p_ptr->name);

	wr_string(p_ptr->died_from);
	wr_string(p_ptr->died_from_list);
	wr_s16b(p_ptr->died_from_depth);

	for (i = 0; i < 4; i++)
	{
		wr_string(p_ptr->history[i]);
	}

	/* Race/Class/Gender/Party */
	wr_byte(p_ptr->prace);
	wr_byte(p_ptr->pclass);
	wr_byte(p_ptr->male);
	wr_byte(p_ptr->party);
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
        for (i = 0; i < MAX_SKILLS; ++i)
        {
                wr_s32b(p_ptr->s_info[i].value);
                wr_u16b(p_ptr->s_info[i].mod);
                wr_byte(p_ptr->s_info[i].dev);
                wr_byte(p_ptr->s_info[i].hidden);
        }
	wr_s16b(p_ptr->skill_points);
//	wr_s16b(p_ptr->skill_last_level);

	wr_s32b(p_ptr->id);
	wr_u32b(p_ptr->dna);

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

	wr_s16b(p_ptr->msp);
	wr_s16b(p_ptr->csp);
	wr_u16b(p_ptr->csp_frac);

	/* Max Player and Dungeon Levels */
	wr_s16b(p_ptr->max_plv);
	wr_s16b(p_ptr->max_dlv);

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
	wr_byte(0);			/* unused */
	wr_s16b(p_ptr->blind);
	wr_s16b(p_ptr->paralyzed);
	wr_s16b(p_ptr->confused);
	wr_s16b(p_ptr->food);
	wr_s16b(0);	/* old "food_digested" */
	wr_s16b(0);	/* old "protection" */
	wr_s16b(p_ptr->energy);
	wr_s16b(p_ptr->fast);
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
	wr_s16b(p_ptr->shield);
	wr_s16b(p_ptr->blessed);
	wr_s16b(p_ptr->tim_invis);
	wr_s16b(p_ptr->word_recall);
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

	wr_s16b(p_ptr->fury);

	wr_s16b(p_ptr->tim_manashield);

	wr_s16b(p_ptr->tim_traps);
	wr_s16b(p_ptr->tim_mimic);
	wr_s16b(p_ptr->tim_mimic_what);

	/* Dump the monster lore */
	tmp16u = MAX_R_IDX;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++) wr_s16b(p_ptr->r_killed[i]);

	/* Future use */
	for (i = 0; i < 44; i++) wr_byte(0);

	/* Ignore some flags */
	wr_u32b(0L);	/* oops */
	wr_u32b(0L);	/* oops */
	wr_u32b(0L);	/* oops */


	/* Write the "object seeds" */
	/*wr_u32b(seed_flavor);*/
	/*wr_u32b(seed_town);*/


	/* Special stuff */
	wr_u16b(panic_save);
	wr_u16b(p_ptr->total_winner);

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
}

/*
 * Write the list of players a player is hostile toward
 */
static void wr_hostilities(int Ind)
{
	player_type *p_ptr = Players[Ind];
	hostile_type *h_ptr;
	int i;
	u16b tmp16u = 0;

	/* Count hostilities */
	for (h_ptr = p_ptr->hostile; h_ptr; h_ptr = h_ptr->next)
	{
		/* One more */
		tmp16u++;
	}

	/* Save number */
	wr_u16b(tmp16u);

	/* Start at beginning */
	h_ptr = p_ptr->hostile;

	/* Save each record */
	for (i = 0; i < tmp16u; i++)
	{
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

static void wr_dungeon(struct worldpos *wpos)
{
	int y, x, i;
	byte prev_feature, n;
	u16b prev_info;
	unsigned char runlength;
	struct c_special *cs_ptr;

	cave_type *c_ptr;
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return;
#if DEBUG_LEVEL > 1
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
		if (l_ptr)
		{
			wr_u32b(l_ptr->flags1);
			wr_byte(l_ptr->hgt);
			wr_byte(l_ptr->wid);
		}
	}

	/*** Simple "Run-Length-Encoding" of cave ***/
	/* for each each row */
	for (y = 0; y < MAX_HGT; y++)
	{
		/* start a new run */
		runlength = 0;

		/* break the row down into runs */
		for (x = 0; x < MAX_WID; x++)
		{
			c_ptr = &zcave[y][x];

			/* if we are starting a new run */
			if ((!runlength) || (c_ptr->feat != prev_feature) || (c_ptr->info != prev_info)
					|| (runlength > 254))
			{
				if (runlength)
				{
					/* if we just finished a run, write it */
					wr_byte(runlength);
					wr_byte(prev_feature);
					wr_u16b(prev_info);
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
		wr_u16b(prev_info);
	}

	/*** another scan for c_special ***/
	/* for each each row */
	for (y = 0; y < MAX_HGT; y++)
	{
		/* break the row down into runs */
		for (x = 0; x < MAX_WID; x++)
		{
			n=0;
			c_ptr = &zcave[y][x];
			cs_ptr=c_ptr->special;
			/* count the number of cs_things */
			while(cs_ptr){
				n++;
				cs_ptr=cs_ptr->next;
			}
			/* write them */
			if(n){
				wr_byte(x);
				wr_byte(y);
				wr_byte(n);
				cs_ptr=c_ptr->special;
				while(cs_ptr){
					i = cs_ptr->type;
					wr_byte(i);
					/* csfunc will take care of it :) */
#if 0
					csfunc[i].save(sc_is_pointer(i) ?
//						cs_ptr->sc.ptr : &c_ptr->special);
						cs_ptr->sc.ptr : cs_ptr);
#endif	/* 0 */
					csfunc[i].save(cs_ptr);
					cs_ptr=cs_ptr->next;
				}
			}
#if 0
		/* redesign needed - too much of hurry to sort now */
			while(cs_ptr){
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
				cs_ptr=cs_ptr->next;
			}
#endif
		}
	}

	/* hack -- terminate it */
	wr_byte(255);
	wr_byte(255);
	wr_byte(255);
}

/* Write a players memory of a cave, simmilar to the above function. */
void wr_cave_memory(Ind)
{
	player_type *p_ptr = Players[Ind];
	int y,x;
	char prev_flag;
	unsigned char runlength = 0;

	/* write the number of flags */
	wr_u16b(MAX_HGT);
	wr_u16b(MAX_WID);
	for (x = y = 0; y < MAX_HGT;)
	{
		/* if we are starting a new run */
		if (!runlength || (p_ptr->cave_flag[y][x] != prev_flag) || (runlength > 254) )
		{
			/* if we just finished a run, write it */
			if (runlength)
			{
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
		if (x >= MAX_WID)
		{
			x = 0; y++;
			if (y >= MAX_HGT) 
			{
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
static bool wr_savefile_new(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int        i,j ;

	u32b              now, tmp32u;

	byte		tmp8u;
	u16b		tmp16u;


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
	wr_byte(VERSION_MAJOR);
	xor_byte = 0;
	wr_byte(VERSION_MINOR);
	xor_byte = 0;
	wr_byte(VERSION_PATCH);
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


	/* Write the RNG state */
	/*wr_randomizer();*/


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
	tmp16u = MAX_Q_IDX;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++)
	{
		wr_byte(q_list[i].level);
		wr_byte(0);
		wr_byte(0);
		wr_byte(0);
	}

	/* Hack -- Dump the artifacts */
	tmp16u = MAX_A_IDX;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++)
	{
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
	{
		wr_s16b(p_ptr->player_hp[i]);
	}


	/* Write spell data */
	wr_u16b(MAX_REALM);
        for (j = 0; j < MAX_REALM; j++)
        {
                wr_u32b(p_ptr->spell_learned1[j]);
                wr_u32b(p_ptr->spell_learned2[j]);
                wr_u32b(p_ptr->spell_worked1[j]);
                wr_u32b(p_ptr->spell_worked2[j]);
                wr_u32b(p_ptr->spell_forgotten1[j]);
                wr_u32b(p_ptr->spell_forgotten2[j]);

                /* Dump the ordered spells */
                for (i = 0; i < 64; i++)
                {
                        wr_byte(p_ptr->spell_order[j][i]);
                }
        }


	/* Write the inventory */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		if (p_ptr->inventory[i].k_idx)
		{
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
	for (i = 0; i < tmp32u; i++)
	{
		wr_byte(p_ptr->wild_map[i]);
	}
	wr_byte(p_ptr->guild);

	wr_s16b(p_ptr->quest_id);
	wr_s16b(p_ptr->quest_num);

	/* Write the "value check-sum" */
	wr_u32b(v_stamp);

	/* Write the "encoded checksum" */
	wr_u32b(x_stamp);


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
static bool save_player_aux(int Ind, char *name)
{
	bool	ok = FALSE;

	int		fd = -1;

	int		mode = 0644;


	/* No file yet */
	fff = NULL;


	/* File type is "SAVE" */
	FILE_TYPE(FILE_TYPE_SAVE);


	/* Create the savefile */
	fd = fd_make(name, mode);

	/* File is okay */
	if (fd >= 0)
	{
		/* Close the "fd" */
		(void)fd_close(fd);

		/* Open the savefile */
		fff = my_fopen(name, "wb");

		/* Successful open */
		if (fff)
		{
			/* Write the savefile */
			if (wr_savefile_new(Ind)) ok = TRUE;

			/* Attempt to close it */
			if (my_fclose(fff)) ok = FALSE;
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
bool save_player(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int		result = FALSE;

	char	safe[1024];


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
	if (save_player_aux(Ind, safe))
	{
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


static bool file_exist(char *buf)
{
	int fd;

	fd = fd_open(buf, O_RDONLY);
	if (fd >= 0)
	{
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
bool load_player(int Ind)
{
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
	if (!p_ptr->savefile[0])
	{
		if (edit)
		{
			what = "Server is closed for login now";
			err = 1;
		}
		else return (TRUE);
	}


	/* XXX XXX XXX Fix this */

	/* Verify the existance of the savefile */
	if (!err && !file_exist(p_ptr->savefile))
	{
		/* Give a message */
		s_printf("Savefile does not exist for player %s.\n", p_ptr->name);
		s_printf("(%s) %d\n", p_ptr->savefile, errno);

		if(errno!=ENOENT){	/* EMFILE for example */
			if(cfg.runlevel>1){
				s_printf("Automatic shutdown\n");
				msg_broadcast(0, "\377r* SERVER EMERGENCY SHUTDOWN *");
				set_runlevel(1);
			}
			return(FALSE);
		}

		/* Allow this */
		if (edit)
		{
			what = "Server is closed for login now";
			err = 1;
		}
		else return (TRUE);
	}


#ifdef VERIFY_SAVEFILE

	/* Verify savefile usage */
	if (!err)
	{
		FILE *fkk;

		char temp[MAX_PATH_LENGTH];

		/* Extract name of lock file */
		strcpy(temp, p_ptr->savefile);
		strcat(temp, ".lok");

		/* Check for lock */
		fkk = my_fopen(temp, "r");

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
		fkk = my_fopen(temp, "w");

		/* Dump a line of info */
		fprintf(fkk, "Lock file for savefile '%s'\n", p_ptr->savefile);

		/* Close the lock file */
		my_fclose(fkk);
	}

#endif


	/* Okay */
	if (!err)
	{
		/* Open the savefile */
		fd = fd_open(p_ptr->savefile, O_RDONLY);

		/* No file */
		if (fd < 0) err = -1;

		/* Message (below) */
		if (err) what = "Cannot open savefile";
	}

	/* Process file */
	if (!err)
	{

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
	if (!err)
	{
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
		if (err)what="Cannot parse savefile error";
		if (err == 35) what = edit ? "Server is closed for login now" :
			"Incorrect password";
	}

	/* Paranoia */
	if (!err)
	{
		/* Invalid turn */
		/* if (!turn) err = -1; */

		/* Message (below) */
		if (err) what = "Broken savefile";
	}

#ifdef VERIFY_TIMESTAMP
	/* Verify timestamp */
	if (!err)
	{
		/* Hack -- Verify the timestamp */
		if (sf_when > (statbuf.st_ctime + 100) ||
				sf_when < (statbuf.st_ctime - 100))
		{
			/* Message */
			what = "Invalid timestamp";

			/* Oops */
			err = -1;
		}
	}
#endif


	/* Okay */
	if (!err)
	{
		/* Give a conversion warning */
		if ((version_major != sf_major) ||
				(version_minor != sf_minor) ||
				(version_patch != sf_patch))
		{
			/* Message */
			printf("Converted a %d.%d.%d savefile.\n",
					sf_major, sf_minor, sf_patch);
		}

		/* Player is dead */
		if (p_ptr->death)
		{
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
		if (p_ptr->chp >= 0)
		{
			/* Reset cause of death */
			(void)strcpy(p_ptr->died_from, "(alive and well)");
		}

		p_ptr->body_changed = TRUE;

		/* Success */
		return (TRUE);
	}


#ifdef VERIFY_SAVEFILE

	/* Verify savefile usage */
	if (TRUE)
	{
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
static void wr_player_names(void)
{
	hash_entry *ptr;

	/*int i, num, *id_list;*/
	int i,  *id_list;
	u32b num;

	/* Get the list of player ID's */
	num = player_id_list(&id_list, 0L);

	/* Store the number of entries */
	wr_u32b(num);

	/* Store each entry */
	for (i = 0; i < num; i++)
	{
		/* Store the ID */
		ptr=lookup_player(id_list[i]);
		if(ptr){
			wr_s32b(id_list[i]);
			wr_u32b(ptr->account);
			wr_s32b(ptr->laston);
			/* 3.4.2 server */
			wr_byte(ptr->race);
			wr_byte(ptr->class);
			wr_byte(ptr->level);
			wr_byte(ptr->party);
			wr_byte(ptr->guild);
			wr_u16b(ptr->quest);
			/* Store the player name */
			wr_string(ptr->name);
		}
	}

	/* Free the memory in the list */
	C_KILL(id_list, num, int);
}

static bool wr_server_savefile(void)
{
        int        i;
	u32b              now;

	byte            tmp8u;
	u16b            tmp16u;
	u32b		tmp32u;


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
	wr_byte(VERSION_MAJOR);
	xor_byte = 0;
	wr_byte(VERSION_MINOR);
	xor_byte = 0;
	wr_byte(VERSION_PATCH);
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

	/* Dump the monster (unique) lore */
	tmp16u = MAX_R_IDX;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++) wr_lore(i);

	/* Hack -- Dump the artifacts */
	tmp16u = MAX_A_IDX;
	wr_u16b(tmp16u);
	for (i = 0; i < tmp16u; i++)
	{
		artifact_type *a_ptr = &a_info[i];
		wr_byte(a_ptr->cur_num);
		wr_byte(a_ptr->known);
//		wr_byte(0);
		wr_byte(0);
		wr_byte(0);
	}


#if 0
	/* Note the stores */
	tmp16u = MAX_STORES;
	wr_u16b(tmp16u);

	/* Dump the stores */
	for (i = 0; i < tmp16u; i++) wr_store(&store[i]);
#endif

	/* Note the parties */
	tmp16u = MAX_PARTIES;
	wr_u16b(tmp16u);

	/* Dump the parties */
	for (i = 0; i < tmp16u; i++) wr_party(&parties[i]);


	
	wr_towns();		/* write town data */
	new_wr_wild();		/* must alloc dungeons first on load! ;) */
	new_wr_dungeons();	/* rename wr_dungeons(void) later */

	/* Prepare to write the monsters */
	compact_monsters(0, FALSE);
	/* Note the number of monsters */
	tmp32u = m_max;
	wr_u32b(tmp32u);
	/* Dump the monsters */
	for (i = 0; i < tmp32u; i++) wr_monster(&m_list[i]);

	/* Prepare to write the objects */
	compact_objects(0, FALSE);
	/* Note the number of objects */
	tmp16u = o_max;
	wr_u16b(tmp16u);
	/* Dump the objects */
	for (i = 0; i < tmp16u; i++) wr_item(&o_list[i]);

#if 0	/* done in wr_dungeon, using csfunc */
	/* Prepare to write the traps */
	compact_traps(0, FALSE);
	/* Note the number of traps */
	tmp16u = t_max;
	wr_u16b(tmp16u);
	/* Dump the traps */
	for (i = 0; i < tmp16u; i++) wr_trap(&t_list[i]);
#endif	// 0

	tmp32u=0L;
	for(i=0;i<num_houses;i++){
		if(!(houses[i].flags&HF_DELETED)) tmp32u++;
	}

	/* Note the number of houses */
	wr_u32b(tmp32u);

	/* Dump the houses */
	for (i = 0; i < num_houses; i++){
		if(!(houses[i].flags&HF_DELETED))
			wr_house(&houses[i]); 
	}

	/* Write the player name database */
	wr_player_names();

	wr_u32b(seed_flavor);
	wr_u32b(seed_town);

	wr_guilds();
	wr_quests();

	wr_u32b(account_id);
	wr_s32b(player_id);
	wr_s32b(turn);

	/* Error in save */
	if (ferror(fff) || (fflush(fff) == EOF)) return FALSE;

	/* Successful save */
	return TRUE;
}

/* write the wilderness and dungeon structure */
static void new_wr_wild(){
	wilderness_type *w_ptr;
	int x,y,i;
	u32b temp;

	temp=MAX_WILD_Y;
	wr_u32b(temp);
	temp=MAX_WILD_X;
	wr_u32b(temp);

	for(y=0;y<MAX_WILD_Y;y++){
		for(x=0;x<MAX_WILD_X;x++){
			w_ptr=&wild_info[y][x];
			wr_wild(w_ptr);
			if(w_ptr->flags & WILD_F_DOWN){
				wr_byte(w_ptr->up_x);
				wr_byte(w_ptr->up_y);
				wr_u16b(w_ptr->dungeon->id);
				wr_u16b(w_ptr->dungeon->baselevel);
				wr_u32b(w_ptr->dungeon->flags);
				wr_byte(w_ptr->dungeon->maxdepth);
				for(i=0;i<10;i++){
					wr_byte(w_ptr->dungeon->r_char[i]);
					wr_byte(w_ptr->dungeon->nr_char[i]);
				}
			}
			if(w_ptr->flags & WILD_F_UP){
				wr_byte(w_ptr->dn_x);
				wr_byte(w_ptr->dn_y);
				wr_u16b(w_ptr->tower->id);
				wr_u16b(w_ptr->tower->baselevel);
				wr_u32b(w_ptr->tower->flags);
				wr_byte(w_ptr->tower->maxdepth);
				for(i=0;i<10;i++){
					wr_byte(w_ptr->tower->r_char[i]);
					wr_byte(w_ptr->tower->nr_char[i]);
				}
			}
		}
	}
}

/* write the actual dungeons */
static void new_wr_dungeons(){
	struct worldpos cwpos;
	wilderness_type *w_ptr;
	int x,y,z;
	cwpos.wz=0;
	for(y=0;y<MAX_WILD_Y;y++){
		cwpos.wy=y;
		for(x=0;x<MAX_WILD_X;x++){
			cwpos.wx=x;
			w_ptr=&wild_info[y][x];
			save_guildhalls(&cwpos);
			/*
			 * One problem here; if a wilderness tower/dungeon exists, and
			 * the surface is not static, stair/recall informations are lost
			 * and cause crash/infinite-loop next time.		FIXME
			 */
			if(getcave(&cwpos) && players_on_depth(&cwpos)) wr_dungeon(&cwpos);
			if(w_ptr->flags & WILD_F_DOWN){
				struct dungeon_type *d_ptr=w_ptr->dungeon;
				for(z=1;z<=d_ptr->maxdepth;z++){
					cwpos.wz=-z;
					if(d_ptr->level[z-1].ondepth && d_ptr->level[z-1].cave);
						wr_dungeon(&cwpos);
				}
			}
			if(w_ptr->flags & WILD_F_UP){
				struct dungeon_type *d_ptr=w_ptr->tower;
				for(z=1;z<=d_ptr->maxdepth;z++){
					cwpos.wz=z;
					if(d_ptr->level[z-1].ondepth && d_ptr->level[z-1].cave);
						wr_dungeon(&cwpos);
				}
			}
			cwpos.wz=0;
		}
	}
	wr_s16b(0x7fff);	/* this could fail if we had 32767^3 areas */
	wr_s16b(0x7fff);	/* I cant see that happening for a while */
	wr_s16b(0x7fff);
}

static bool save_server_aux(char *name)
{
	bool    ok = FALSE;

	int             fd = -1;

	int             mode = 0644;


	/* No file yet */
	fff = NULL;


	/* File type is "SAVE" */
	FILE_TYPE(FILE_TYPE_SAVE);


	/* Create the savefile */
	fd = fd_make(name, mode);

	/* File is okay */
	if (fd >= 0)
	{
		/* Close the "fd" */
		(void)fd_close(fd);

		/* Open the savefile */
		fff = my_fopen(name, "wb");

		/* Successful open */
		if (fff)
		{
			/* Write the savefile */
			if (wr_server_savefile()) ok = TRUE;

			/* Attempt to close it */
			if (my_fclose(fff)) ok = FALSE;
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
 * Load the server info (artifacts created and uniques killed)
 * from a special savefile.
 */
bool load_server_info(void)
{
	int fd = -1;

	byte vvv[4];

	errr err = 0;

	cptr what = "generic";

	char buf[1024];

	path_build(buf, 1024, ANGBAND_DIR_SAVE, "server");

	/* XXX XXX XXX Fix this */
	if (!file_exist(buf))
	{
		/* Give message */
		s_printf("Server savefile does not exist\n");

		/* Allow this */
		return (TRUE);
	}

	/* Okay */
	if (!err)
	{
		/* Open the savefile */
		fd = fd_open(buf, O_RDONLY);

		/* No file */
		if (fd < 0) err = -1;

		/* Message (below) */
		if (err) what = "Cannot open savefile";
	}

	/* Process file */
	if (!err)
	{
		/* Read the first four bytes */
		if (fd_read(fd, (char*)(vvv), 4)) err = -1;

		/* What */
		if (err) what = "Cannot read savefile";

		/* Close the file */
		(void)fd_close(fd);
	}

	/* Process file */
	if (!err)
	{
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
		if (err) what ="Cannot parse savefile error %d";
	}

	/* Okay */
	if (!err)
	{
		/* Give a conversion warning */
		if ((version_major != sf_major) ||
				(version_minor != sf_minor) ||
				(version_patch != sf_patch))
		{
			/* Message */
			printf("Converted a %d.%d.%d savefile.\n",
					sf_major, sf_minor, sf_patch);
		}

		/* The server state was loaded */
		server_state_loaded = TRUE;

		/* Success */
		return (TRUE);
	}

	/* Message */
	s_printf("Error (%s) reading a %d.%d.%d server savefile.", what, sf_major, sf_minor, sf_patch);

	return (FALSE);
}


/*
 * Save the server state to a "server" savefile.
 */
bool save_server_info(void)
{
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
	if (save_server_aux(safe))
	{
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

void wr_towns(){
	int i, j;
	wr_u16b(numtowns);
	for(i=0;i<numtowns;i++){
		wr_u16b(town[i].x);
		wr_u16b(town[i].y);
		wr_u16b(town[i].baselevel);
		wr_u16b(town[i].flags);
		wr_u16b(town[i].num_stores);

		/* Dump the stores */
		for (j = 0; j < town[i].num_stores; j++){
			wr_store(&town[i].townstore[j]);
		}
	}
}
