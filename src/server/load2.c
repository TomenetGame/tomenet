/* File: load2.c */

/* Purpose: support for loading savefiles -BEN- */

#define SERVER

#include "angband.h"


/*
 * This file is responsible for loading all "2.7.X" savefiles
 *
 * Note that 2.7.0 - 2.7.2 savefiles are obsolete and will not work.
 *
 * We attempt to prevent corrupt savefiles from inducing memory errors.
 *
 * Note that Angband 2.7.9 encodes "terrain features" in the savefile
 * using the old 2.7.8 method.  Angband 2.8.0 will use the same method
 * to read pre-2.8.0 savefiles, but will use a new method to save them,
 * which will only affect "save.c".
 *
 * Note that Angband 2.8.0 will use a VERY different savefile method,
 * which will use "blocks" of information which can be ignored or parsed,
 * and which will not use a silly "protection" scheme on the savefiles,
 * but which may still use some form of "checksums" to prevent the use
 * of "corrupt" savefiles, which might cause nasty weirdness.
 *
 * Note that this file should not use the random number generator, the
 * object flavors, the visual attr/char mappings, or anything else which
 * is initialized *after* or *during* the "load character" function.
 *
 * We should also make the "cheating" options official flags, and
 * move the "byte" options to a different part of the code, perhaps
 * with a few more (for variety).
 *
 * Implement simple "savefile extenders" using some form of "sized"
 * chunks of bytes, with a {size,type,data} format, so everyone can
 * know the size, interested people can know the type, and the actual
 * data is available to the parsing routines that acknowledge the type.
 *
 * Consider changing the "globe of invulnerability" code so that it
 * takes some form of "maximum damage to protect from" in addition to
 * the existing "number of turns to protect for", and where each hit
 * by a monster will reduce the shield by that amount.
 *
 * XXX XXX XXX
 */





/*
 * Local "savefile" pointer
 */
static FILE	*fff;

/*
 * Hack -- old "encryption" byte
 */
static byte	xor_byte;

/*
 * Hack -- simple "checksum" on the actual values
 */
static u32b	v_check = 0L;

/*
 * Hack -- simple "checksum" on the encoded bytes
 */
static u32b	x_check = 0L;



/*
 * This function determines if the version of the savefile
 * currently being read is older than version "x.y.z".
 */
static bool older_than(byte x, byte y, byte z)
{
	/* Much older, or much more recent */
	if (sf_major < x) return (TRUE);
	if (sf_major > x) return (FALSE);

	/* Distinctly older, or distinctly more recent */
	if (sf_minor < y) return (TRUE);
	if (sf_minor > y) return (FALSE);

	/* Barely older, or barely more recent */
	if (sf_patch < z) return (TRUE);
	if (sf_patch > z) return (FALSE);

	/* Identical versions */
	return (FALSE);
}


/*
 * Show information on the screen, one line at a time.
 * Start at line 2, and wrap, if needed, back to line 2.
 */
static void note(cptr msg)
{
#if 0
	static int y = 2;

	/* Draw the message */
	prt(msg, y, 0);

	/* Advance one line (wrap if needed) */
	if (++y >= 24) y = 2;

	/* Flush it */
	Term_fresh();
#endif
}


/*
 * Hack -- determine if an item is "wearable" (or a missile)
 */
static bool wearable_p(object_type *o_ptr)
{
	/* Valid "tval" codes */
	switch (o_ptr->tval)
	{
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
		case TV_BOW:
		case TV_DIGGING:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_HELM:
		case TV_CROWN:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
		case TV_LITE:
		case TV_AMULET:
		case TV_RING:
			{
				return (TRUE);
			}
	}

	/* Nope */
	return (FALSE);
}


/*
 * The following functions are used to load the basic building blocks
 * of savefiles.  They also maintain the "checksum" info for 2.7.0+
 */

static byte sf_get(void)
{
	byte c, v;

	/* Get a character, decode the value */
	c = getc(fff) & 0xFF;
	v = c ^ xor_byte;
	xor_byte = c;

	/* Maintain the checksum info */
	v_check += v;
	x_check += xor_byte;

	/* Return the value */
	return (v);
}

static void rd_byte(byte *ip)
{
	*ip = sf_get();
}

static void rd_u16b(u16b *ip)
{
	(*ip) = sf_get();
	(*ip) |= ((u16b)(sf_get()) << 8);
}

static void rd_s16b(s16b *ip)
{
	rd_u16b((u16b*)ip);
}

static void rd_u32b(u32b *ip)
{
	(*ip) = sf_get();
	(*ip) |= ((u32b)(sf_get()) << 8);
	(*ip) |= ((u32b)(sf_get()) << 16);
	(*ip) |= ((u32b)(sf_get()) << 24);
}

static void rd_s32b(s32b *ip)
{
	rd_u32b((u32b*)ip);
}


/*
 * Hack -- read a string
 */
static void rd_string(char *str, int max)
{
	int i;

	/* Read the string */
	for (i = 0; TRUE; i++)
	{
		byte tmp8u;

		/* Read a byte */
		rd_byte(&tmp8u);

		/* Collect string while legal */
		if (i < max) str[i] = tmp8u;

		/* End of string */
		if (!tmp8u) break;
	}

	/* Terminate */
	str[max-1] = '\0';
}


/*
 * Hack -- strip some bytes
 */
static void strip_bytes(int n)
{
	int i;
	byte tmp8u;

	/* Strip the bytes */
	for (i = 0; i < n; i++) rd_byte(&tmp8u);
}


/*
 * Owner Conversion -- pre-2.7.8 to 2.7.8
 * Shop is column, Owner is Row, see "tables.c"
 */
#if 0
static byte convert_owner[24] =
{
	1, 3, 1, 0, 2, 3, 2, 0,
	0, 1, 3, 1, 0, 1, 1, 0,
	3, 2, 0, 2, 1, 2, 3, 0
};
#endif


/*
 * Old inventory slot values (pre-2.7.3)
 */
#define OLD_INVEN_WIELD     22
#define OLD_INVEN_HEAD      23
#define OLD_INVEN_NECK      24
#define OLD_INVEN_BODY      25
#define OLD_INVEN_ARM       26
#define OLD_INVEN_HANDS     27
#define OLD_INVEN_RIGHT     28
#define OLD_INVEN_LEFT      29
#define OLD_INVEN_FEET      30
#define OLD_INVEN_OUTER     31
#define OLD_INVEN_LITE      32
#define OLD_INVEN_AUX       33

#if 0
/*
 * Analyze pre-2.7.3 inventory slots
 */
static s16b convert_slot(int old)
{
	/* Move slots */
	switch (old)
	{
		case OLD_INVEN_WIELD: return (INVEN_WIELD);
		case OLD_INVEN_HEAD: return (INVEN_HEAD);
		case OLD_INVEN_NECK: return (INVEN_NECK);
		case OLD_INVEN_BODY: return (INVEN_BODY);
		case OLD_INVEN_ARM: return (INVEN_ARM);
		case OLD_INVEN_HANDS: return (INVEN_HANDS);
		case OLD_INVEN_RIGHT: return (INVEN_RIGHT);
		case OLD_INVEN_LEFT: return (INVEN_LEFT);
		case OLD_INVEN_FEET: return (INVEN_FEET);
		case OLD_INVEN_OUTER: return (INVEN_OUTER);
		case OLD_INVEN_LITE: return (INVEN_LITE);

							 /* Hack -- "hold" old aux items */
		case OLD_INVEN_AUX: return (INVEN_WIELD - 1);
	}

	/* Default */
	return (old);
}




/*
 * Hack -- convert 2.7.X ego-item indexes into 2.7.9 ego-item indexes
 */

static byte convert_ego_item[128] =
{
	0				/* 0 */,
	EGO_RESISTANCE		/* 1 = EGO_RESIST (XXX) */,
	EGO_RESIST_ACID		/* 2 = EGO_RESIST_A (XXX) */,
	EGO_RESIST_FIRE		/* 3 = EGO_RESIST_F (XXX) */,
	EGO_RESIST_COLD		/* 4 = EGO_RESIST_C (XXX) */,
	EGO_RESIST_ELEC		/* 5 = EGO_RESIST_E (XXX) */,
	EGO_HA			/* 6 = EGO_HA */,
	EGO_DF			/* 7 = EGO_DF */,
	EGO_SLAY_ANIMAL		/* 8 = EGO_SLAY_ANIMAL */,
	EGO_SLAY_DRAGON		/* 9 = EGO_SLAY_DRAGON */,
	EGO_SLAY_EVIL		/* 10 = EGO_SLAY_EVIL (XXX) */,
	EGO_SLAY_UNDEAD		/* 11 = EGO_SLAY_UNDEAD (XXX) */,
	EGO_BRAND_FIRE		/* 12 = EGO_FT */,
	EGO_BRAND_COLD		/* 13 = EGO_FB */,
	EGO_FREE_ACTION		/* 14 = EGO_FREE_ACTION (XXX) */,
	EGO_SLAYING			/* 15 = EGO_SLAYING */,
	0				/* 16 */,
	0				/* 17 */,
	EGO_SLOW_DESCENT		/* 18 = EGO_SLOW_DESCENT */,
	EGO_SPEED			/* 19 = EGO_SPEED */,
	EGO_STEALTH			/* 20 = EGO_STEALTH (XXX) */,
	0				/* 21 */,
	0				/* 22 */,
	0				/* 23 */,
	EGO_INTELLIGENCE		/* 24 = EGO_INTELLIGENCE */,
	EGO_WISDOM			/* 25 = EGO_WISDOM */,
	EGO_INFRAVISION		/* 26 = EGO_INFRAVISION */,
	EGO_MIGHT			/* 27 = EGO_MIGHT */,
	EGO_LORDLINESS		/* 28 = EGO_LORDLINESS */,
	EGO_MAGI			/* 29 = EGO_MAGI (XXX) */,
	EGO_BEAUTY			/* 30 = EGO_BEAUTY */,
	EGO_SEEING			/* 31 = EGO_SEEING (XXX) */,
	EGO_REGENERATION		/* 32 = EGO_REGENERATION */,
	0				/* 33 */,
	0				/* 34 */,
	0				/* 35 */,
	0				/* 36 */,
	0				/* 37 */,
	EGO_PERMANENCE		/* 38 = EGO_ROBE_MAGI */,
	EGO_PROTECTION		/* 39 = EGO_PROTECTION */,
	0				/* 40 */,
	0				/* 41 */,
	0				/* 42 */,
	EGO_BRAND_FIRE		/* 43 = EGO_FIRE (XXX) */,
	EGO_HURT_EVIL		/* 44 = EGO_AMMO_EVIL */,
	EGO_HURT_DRAGON		/* 45 = EGO_AMMO_DRAGON */,
	0				/* 46 */,
	0				/* 47 */,
	0				/* 48 */,
	0				/* 49 */,
	EGO_FLAME			/* 50 = EGO_AMMO_FIRE */,
	0				/* 51 */,	/* oops */
	EGO_FROST			/* 52 = EGO_AMMO_SLAYING */,
	0				/* 53 */,
	0				/* 54 */,
	EGO_HURT_ANIMAL		/* 55 = EGO_AMMO_ANIMAL */,
	0				/* 56 */,
	0				/* 57 */,
	0				/* 58 */,
	0				/* 59 */,
	EGO_EXTRA_MIGHT		/* 60 = EGO_EXTRA_MIGHT */,
	EGO_EXTRA_SHOTS		/* 61 = EGO_EXTRA_SHOTS */,
	0				/* 62 */,
	0				/* 63 */,
	EGO_VELOCITY		/* 64 = EGO_VELOCITY */,
	EGO_ACCURACY		/* 65 = EGO_ACCURACY */,
	0				/* 66 */,
	EGO_SLAY_ORC		/* 67 = EGO_SLAY_ORC */,
	EGO_POWER			/* 68 = EGO_POWER */,
	0				/* 69 */,
	0				/* 70 */,
	EGO_WEST			/* 71 = EGO_WEST */,
	EGO_BLESS_BLADE		/* 72 = EGO_BLESS_BLADE */,
	EGO_SLAY_DEMON		/* 73 = EGO_SLAY_DEMON */,
	EGO_SLAY_TROLL		/* 74 = EGO_SLAY_TROLL */,
	0				/* 75 */,
	0				/* 76 */,
	EGO_WOUNDING		/* 77 = EGO_AMMO_WOUNDING */,
	0				/* 78 */,
	0				/* 79 */,
	0				/* 80 */,
	EGO_LITE			/* 81 = EGO_LITE */,
	EGO_AGILITY			/* 82 = EGO_AGILITY */,
	0				/* 83 */,
	0				/* 84 */,
	EGO_SLAY_GIANT		/* 85 = EGO_SLAY_GIANT */,
	EGO_TELEPATHY		/* 86 = EGO_TELEPATHY */,
	EGO_ELVENKIND		/* 87 = EGO_ELVENKIND (XXX) */,
	0				/* 88 */,
	0				/* 89 */,
	EGO_ATTACKS			/* 90 = EGO_ATTACKS */,
	EGO_AMAN			/* 91 = EGO_AMAN */,
	0				/* 92 */,
	0				/* 93 */,
	0				/* 94 */,
	0				/* 95 */,
	0				/* 96 */,
	0				/* 97 */,
	0				/* 98 */,
	0				/* 99 */,
	0				/* 100 */,
	0				/* 101 */,
	0				/* 102 */,
	0				/* 103 */,
	EGO_WEAKNESS		/* 104 = EGO_WEAKNESS */,
	EGO_STUPIDITY		/* 105 = EGO_STUPIDITY */,
	EGO_NAIVETY			/* 106 = EGO_DULLNESS */,
	EGO_SICKLINESS		/* 107 = EGO_SICKLINESS */,
	EGO_CLUMSINESS		/* 108 = EGO_CLUMSINESS */,
	EGO_UGLINESS		/* 109 = EGO_UGLINESS */,
	EGO_TELEPORTATION		/* 110 = EGO_TELEPORTATION */,
	0				/* 111 */,
	EGO_IRRITATION		/* 112 = EGO_IRRITATION */,
	EGO_VULNERABILITY		/* 113 = EGO_VULNERABILITY */,
	EGO_ENVELOPING		/* 114 = EGO_ENVELOPING */,
	0				/* 115 */,
	EGO_SLOWNESS		/* 116 = EGO_SLOWNESS */,
	EGO_NOISE			/* 117 = EGO_NOISE */,
	EGO_ANNOYANCE		/* 118 = EGO_GREAT_MASS */,
	0				/* 119 */,
	EGO_BACKBITING		/* 120 = EGO_BACKBITING */,
	0				/* 121 */,
	0				/* 122 */,
	0				/* 123 */,
	EGO_MORGUL			/* 124 = EGO_MORGUL */,
	0				/* 125 */,
	EGO_SHATTERED		/* 126 = EGO_SHATTERED */,
	EGO_BLASTED			/* 127 = EGO_BLASTED (XXX) */
};

#endif

/*
 * Read an item (2.7.0 or later)
 *
 * Note that savefiles from 2.7.0 and 2.7.1 are obsolete.
 *
 * Note that pre-2.7.9 savefiles (from Angband 2.5.1 onward anyway)
 * can be read using the code above.
 *
 * This function attempts to "repair" old savefiles, and to extract
 * the most up to date values for various object fields.
 *
 * Note that Angband 2.7.9 introduced a new method for object "flags"
 * in which the "flags" on an object are actually extracted when they
 * are needed from the object kind, artifact index, ego-item index,
 * and two special "xtra" fields which are used to encode any "extra"
 * power of certain ego-items.  This had the side effect that items
 * imported from pre-2.7.9 savefiles will lose any "extra" powers they
 * may have had, and also, all "uncursed" items will become "cursed"
 * again, including Calris, even if it is being worn at the time.  As
 * a complete hack, items which are inscribed with "uncursed" will be
 * "uncursed" when imported from pre-2.7.9 savefiles.
 */
/* For wilderness levels, dun_depth has been changed from 1 to 4 bytes. */
static void rd_item(object_type *o_ptr)
{
	byte old_dd;
	byte old_ds;

	u32b f1, f2, f3;

	u16b tmp16u;

	object_kind *k_ptr;

	char note[128];


	/* Hack -- wipe */
	WIPE(o_ptr, object_type);

	rd_s32b(&o_ptr->owner);
	rd_s16b(&o_ptr->level);

	/* Kind */
	rd_s16b(&o_ptr->k_idx);

	/* Location */
	rd_byte(&o_ptr->iy);
	rd_byte(&o_ptr->ix);

	if (older_than(0,5,5))
	{
		rd_byte((char *)&o_ptr->dun_depth);
	}
	else rd_s32b((s32b *)&o_ptr->dun_depth);

	/* Type/Subtype */
	rd_byte(&o_ptr->tval);
	rd_byte(&o_ptr->sval);

	/* Base pval */
	if (!older_than(0,7,0))
		rd_s32b(&o_ptr->bpval);
	else o_ptr->bpval = 0;

	/* Special pval */
	if (older_than(0,6,1))
	{
		rd_s16b(&o_ptr->pval);
	}
	else	rd_s32b(&o_ptr->pval);

	rd_byte(&o_ptr->discount);
	rd_byte(&o_ptr->number);
	rd_s16b(&o_ptr->weight);

	rd_byte(&o_ptr->name1);
	rd_byte(&o_ptr->name2);
	rd_s32b(&o_ptr->name3);
	rd_s16b(&o_ptr->timeout);

	rd_s16b(&o_ptr->to_h);
	rd_s16b(&o_ptr->to_d);
	rd_s16b(&o_ptr->to_a);

	rd_s16b(&o_ptr->ac);

	rd_byte(&old_dd);
	rd_byte(&old_ds);

	rd_byte(&o_ptr->ident);

	strip_bytes(1);
	/*rd_byte(&o_ptr->marked);*/

	/* Old flags */
	strip_bytes(12);

	/* Unused */
	rd_u16b(&tmp16u);

	/* Special powers */
	rd_byte(&o_ptr->xtra1);
	rd_byte(&o_ptr->xtra2);

	/* Inscription */
	rd_string(note, 128);

	/* Save the inscription */
	if (note[0]) o_ptr->note = quark_add(note);


	/* Mega-Hack -- handle "dungeon objects" later */
	if ((o_ptr->k_idx >= 445) && (o_ptr->k_idx <= 479)) return;


	/* Obtain the "kind" template */
	k_ptr = &k_info[o_ptr->k_idx];

	/* Obtain tval/sval from k_info */
	o_ptr->tval = k_ptr->tval;
	o_ptr->sval = k_ptr->sval;


	/* Hack -- notice "broken" items */
	if (k_ptr->cost <= 0) o_ptr->ident |= ID_BROKEN;


	/* Repair non "wearable" items */
	if (!wearable_p(o_ptr))
	{
		/* Acquire correct fields */
		o_ptr->to_h = k_ptr->to_h;
		o_ptr->to_d = k_ptr->to_d;
		o_ptr->to_a = k_ptr->to_a;

		/* Acquire correct fields */
		o_ptr->ac = k_ptr->ac;
		o_ptr->dd = k_ptr->dd;
		o_ptr->ds = k_ptr->ds;

		/* Acquire correct weight */
		o_ptr->weight = k_ptr->weight;

		/* Paranoia */
		o_ptr->name1 = o_ptr->name2 = 0;

		/* All done */
		return;
	}


	/* Extract the flags */
	object_flags(o_ptr, &f1, &f2, &f3);

	/* Paranoia */
	if (o_ptr->name2)
	{
		ego_item_type *e_ptr;

		/* Obtain the ego-item info */
		e_ptr = &e_info[o_ptr->name2];

		/* Verify that ego-item */
		if (!e_ptr->name) o_ptr->name2 = 0;
	}


	/* Acquire standard fields */
	o_ptr->ac = k_ptr->ac;
	o_ptr->dd = k_ptr->dd;
	o_ptr->ds = k_ptr->ds;

	/* Acquire standard weight */
	o_ptr->weight = k_ptr->weight;

	/* Hack -- extract the "broken" flag */
	if (!o_ptr->pval < 0) o_ptr->ident |= ID_BROKEN;


	/* Artifacts */
	if (o_ptr->name1)
	{
		artifact_type *a_ptr;

		/* Obtain the artifact info */
		/* Hack -- Randarts! */
		if (o_ptr->name1 == ART_RANDART)
		{
			a_ptr = randart_make(o_ptr);
		}
		else
		{
			a_ptr = &a_info[o_ptr->name1];
		}

		/* Acquire new artifact "pval" */
		o_ptr->pval = a_ptr->pval;

		/* Acquire new artifact fields */
		o_ptr->ac = a_ptr->ac;
		o_ptr->dd = a_ptr->dd;
		o_ptr->ds = a_ptr->ds;

		/* Acquire new artifact weight */
		o_ptr->weight = a_ptr->weight;

		/* Hack -- extract the "broken" flag */
		if (!a_ptr->cost) o_ptr->ident |= ID_BROKEN;
	}

	/* Ego items */
	if (o_ptr->name2)
	{
		ego_item_type *e_ptr;

		/* Obtain the ego-item info */
		e_ptr = &e_info[o_ptr->name2];

		/* Hack -- keep some old fields */
		if ((o_ptr->dd < old_dd) && (o_ptr->ds == old_ds))
		{
			/* Keep old boosted damage dice */
			o_ptr->dd = old_dd;
		}

		/* Hack -- extract the "broken" flag */
		if (!e_ptr->cost) o_ptr->ident |= ID_BROKEN;
	}
}



/*
 * Read a "monster" record
 */
static void rd_monster_race(monster_race *r_ptr)
{
	int i;

	rd_s16b(&r_ptr->name);
	rd_s16b(&r_ptr->text);
	rd_byte(&r_ptr->hdice);
	rd_byte(&r_ptr->hside);
	rd_s16b(&r_ptr->ac);
	rd_s16b(&r_ptr->sleep);
	rd_byte(&r_ptr->aaf);
	rd_byte(&r_ptr->speed);
	rd_s32b(&r_ptr->mexp);
	rd_s16b(&r_ptr->extra);
	rd_byte(&r_ptr->freq_inate);
	rd_byte(&r_ptr->freq_spell);
	rd_s32b(&r_ptr->flags1);
	rd_s32b(&r_ptr->flags2);
	rd_s32b(&r_ptr->flags3);
	rd_s32b(&r_ptr->flags4);
	rd_s32b(&r_ptr->flags5);
	rd_s32b(&r_ptr->flags6);
	rd_s16b(&r_ptr->level);
	rd_byte(&r_ptr->rarity);
	rd_byte(&r_ptr->d_attr);
	rd_byte(&r_ptr->d_char);
	rd_byte(&r_ptr->x_attr);
	rd_byte(&r_ptr->x_char);
	for (i = 0; i < 4; i++)
	{
		rd_byte(&r_ptr->blow[i].method);
		rd_byte(&r_ptr->blow[i].effect);
		rd_byte(&r_ptr->blow[i].d_dice);
		rd_byte(&r_ptr->blow[i].d_side);
	}
}


/*
 * Read a monster
 */

static void rd_monster(monster_type *m_ptr)
{
	byte tmp8u, i;

	/* Hack -- wipe */
	WIPE(m_ptr, monster_type);

	rd_byte(&m_ptr->special);

	/* Owner */
	rd_s32b(&m_ptr->owner);

	/* Read the monster race */
	rd_s16b(&m_ptr->r_idx);

	/* Read the other information */
	rd_byte(&m_ptr->fy);
	rd_byte(&m_ptr->fx);
	rd_u16b(&m_ptr->dun_depth);
	rd_s16b(&m_ptr->ac);
	rd_byte(&m_ptr->speed);
	rd_s32b(&m_ptr->exp);
	rd_s16b(&m_ptr->level);
	for (i = 0; i < 4; i++)
	{
		rd_byte(&(m_ptr->blow[i].method));
		rd_byte(&(m_ptr->blow[i].effect));
		rd_byte(&(m_ptr->blow[i].d_dice));
		rd_byte(&(m_ptr->blow[i].d_side));
	}
	rd_s32b(&m_ptr->hp);
	rd_s32b(&m_ptr->maxhp);
	rd_s16b(&m_ptr->csleep);
	rd_byte(&m_ptr->mspeed);
	rd_byte(&m_ptr->energy);
	rd_byte(&m_ptr->stunned);
	rd_byte(&m_ptr->confused);
	rd_byte(&m_ptr->monfear);
	if (!older_than(3, 2, 1)) rd_s16b(&m_ptr->mind);
	else m_ptr->mind = GOLEM_NONE;
	if (m_ptr->special)
	{
		MAKE(m_ptr->r_ptr, monster_race);
		rd_monster_race(m_ptr->r_ptr);
	}

	if (!older_than(3, 2, 2))
	{
		rd_u16b(&m_ptr->ego);
		rd_s32b(&m_ptr->name3);
	}
}


/*
 * Read the monster lore
 */
static void rd_lore(int r_idx)
{
	byte tmp8u;
	u16b tmp16u;

	monster_race *r_ptr = &r_info[r_idx];

	/* Count sights/deaths/kills */
	rd_s16b(&r_ptr->r_sights);
	rd_s16b(&r_ptr->r_deaths);
	rd_s16b(&r_ptr->r_pkills);
	rd_s16b(&r_ptr->r_tkills);

	/* Count wakes and ignores */
	rd_byte(&r_ptr->r_wake);
	rd_byte(&r_ptr->r_ignore);

	/* Load in the amount of time left until the (possobile) unique respawns */
	if (older_than(0,6,1))
	{
		rd_u16b(&tmp16u);
		r_ptr->respawn_timer = tmp16u;
	}
	else
	{
		rd_s32b(&r_ptr->respawn_timer);
	}

	/* Count drops */
	rd_byte(&r_ptr->r_drop_gold);
	rd_byte(&r_ptr->r_drop_item);

	/* Count spells */
	rd_byte(&r_ptr->r_cast_inate);
	rd_byte(&r_ptr->r_cast_spell);

	/* Count blows of each type */
	rd_byte(&r_ptr->r_blows[0]);
	rd_byte(&r_ptr->r_blows[1]);
	rd_byte(&r_ptr->r_blows[2]);
	rd_byte(&r_ptr->r_blows[3]);

	/* Memorize flags */
	rd_u32b(&r_ptr->r_flags1);
	rd_u32b(&r_ptr->r_flags2);
	rd_u32b(&r_ptr->r_flags3);
	rd_u32b(&r_ptr->r_flags4);
	rd_u32b(&r_ptr->r_flags5);
	rd_u32b(&r_ptr->r_flags6);


	/* Read the "Racial" monster limit per level */
	rd_byte(&r_ptr->max_num);

	/* Read the "killer" info */
	if (!older_than(0,4,1) && older_than(3, 0, 3))
		rd_s32b(&r_ptr->killer);

	/* Later (?) */
	rd_byte(&tmp8u);
	rd_byte(&tmp8u);
	rd_byte(&tmp8u);

	/* Repair the lore flags */
	r_ptr->r_flags1 &= r_ptr->flags1;
	r_ptr->r_flags2 &= r_ptr->flags2;
	r_ptr->r_flags3 &= r_ptr->flags3;
	r_ptr->r_flags4 &= r_ptr->flags4;
	r_ptr->r_flags5 &= r_ptr->flags5;
	r_ptr->r_flags6 &= r_ptr->flags6;
}




/*
 * Read a store
 */
static errr rd_store(int n)
{
	store_type *st_ptr = &store[n];

	int j;

	byte own, num;

	/* Read the basic info */
	rd_s32b(&st_ptr->store_open);
	rd_s16b(&st_ptr->insult_cur);
	rd_byte(&own);
	rd_byte(&num);
	rd_s16b(&st_ptr->good_buy);
	rd_s16b(&st_ptr->bad_buy);

	/* Extract the owner (see above) */
	st_ptr->owner = own;

	/* Read the items */
	for (j = 0; j < num; j++)
	{
		object_type forge;

		/* Read the item */
		rd_item(&forge);

		/* Acquire valid items */
		if (st_ptr->stock_num < STORE_INVEN_MAX)
		{
			/* Acquire the item */
			st_ptr->stock[st_ptr->stock_num++] = forge;
		}
	}

	/* Success */
	return (0);
}


/*
 * Read some party info

 FOUND THE BUIG!!!!!! the disapearing party bug. no more.
 I hope......
 -APD-
 */
static void rd_party(int n)
{
	party_type *party_ptr = &parties[n];

	/* Party name */
	rd_string(party_ptr->name, 80);

	/* Party owner's name */
	rd_string(party_ptr->owner, 20);

	/* Number of people and creation time */
	rd_s32b(&party_ptr->num);
	rd_s32b(&party_ptr->created);

	/* Hack -- repair dead parties 
	   I THINK this line was causing some problems....
	   lets find out
	   */
	if (!lookup_player_id(party_ptr->owner))
	{
		/*
		   Set to no people in party
		   party_ptr->num = 0;
		   */
	}
}

/*
 * Read some house info
 */
static void rd_house(int n)
{
	house_type *house_ptr = &houses[n];

#ifdef NEWHOUSES
	rd_byte(&house_ptr->x); 
	rd_byte(&house_ptr->y);
	rd_byte(&house_ptr->dx); 
	rd_byte(&house_ptr->dy);
	MAKE(house_ptr->dna, struct dna_type);
	rd_u32b(&house_ptr->dna->creator);
	rd_u32b(&house_ptr->dna->owner);
	rd_byte(&house_ptr->dna->owner_type);
	rd_byte(&house_ptr->dna->a_flags);
	rd_u16b(&house_ptr->dna->min_level);
	rd_u32b(&house_ptr->dna->price);
	rd_u16b(&house_ptr->flags);
	rd_u32b(&house_ptr->depth);
	if(cave[house_ptr->depth] && !(house_ptr->flags&HF_STOCK)){
		/* add dna to static levels */
		cave[house_ptr->depth][house_ptr->y+house_ptr->dy][house_ptr->x+house_ptr->dx].special.type=DNA_DOOR;
		cave[house_ptr->depth][house_ptr->y+house_ptr->dy][house_ptr->x+house_ptr->dx].special.ptr=house_ptr->dna;
	}
	if(house_ptr->flags&HF_RECT){
		rd_byte(&house_ptr->coords.rect.width);
		rd_byte(&house_ptr->coords.rect.height);
	}
	else{
		int i=-2;
		C_MAKE(house_ptr->coords.poly, MAXCOORD, byte);
		do{
			i+=2;
			rd_byte(&house_ptr->coords.poly[i]);
			rd_byte(&house_ptr->coords.poly[i+1]);
		}while(house_ptr->coords.poly[i] || house_ptr->coords.poly[i+1]);
		GROW(house_ptr->coords.poly, MAXCOORD, i+2, byte);
	}

#else
	/* coordinates of corners of house */
	rd_byte(&house_ptr->x_1); 
	rd_byte(&house_ptr->y_1);
	rd_byte(&house_ptr->x_2);
	rd_byte(&house_ptr->y_2);

	/* coordinates of the house door */
	rd_byte(&house_ptr->door_y);
	rd_byte(&house_ptr->door_x);

	/* Door Strength */
	rd_byte(&house_ptr->strength);

	/* Owned or not */
	rd_byte(&house_ptr->owned);

	rd_s32b(&house_ptr->depth);
	rd_s32b(&house_ptr->price);
#endif
}

static void rd_wild(int n)
{
	u32b tmp32u;
	wilderness_type *w_ptr = &wild_info[-n];

	/* future use */
	rd_u32b(&tmp32u);
	/* the flags */
	rd_u32b((u32b *) &w_ptr->flags);

	if (!older_than(3, 0, 4))
	{	
		/* the player(KING) owning the wild */
		rd_s32b(&w_ptr->own);
	}
}


/*
 * Read RNG state
 */
#if 0
static void rd_randomizer(void)
{
	int i;

	u16b tmp16u;

	/* Old version */
	if (older_than(2, 8, 0)) return;

	/* Tmp */
	rd_u16b(&tmp16u);

	/* Place */
	rd_u16b(&Rand_place);

	/* State */
	for (i = 0; i < RAND_DEG; i++)
	{
		rd_u32b(&Rand_state[i]);
	}

	/* Accept */
	Rand_quick = FALSE;
}
#endif



/*
 * Read options (ignore pre-2.8.0 options)
 *
 * Note that the normal options are now stored as a set of 256 bit flags,
 * plus a set of 256 bit masks to indicate which bit flags were defined
 * at the time the savefile was created.  This will allow new options
 * to be added, and old options to be removed, at any time, without
 * hurting old savefiles.
 *
 * The window options are stored in the same way, but note that each
 * window gets 32 options, and their order is fixed by certain defines.
 */
#if 0
static void rd_options(void)
{
	int i, n;

	byte b;

	u16b c;

	u32b flag[8];
	u32b mask[8];


	/*** Oops ***/

	/* Oops */
	strip_bytes(16);


	/*** Special info */

	/* Read "delay_factor" */
	rd_byte(&b);
	delay_factor = b;

	/* Read "hitpoint_warn" */
	rd_byte(&b);
	hitpoint_warn = b;


	/*** Cheating options ***/

	rd_u16b(&c);

	if (c & 0x0002) wizard = TRUE;

	cheat_peek = (c & 0x0100) ? TRUE : FALSE;
	cheat_hear = (c & 0x0200) ? TRUE : FALSE;
	cheat_room = (c & 0x0400) ? TRUE : FALSE;
	cheat_xtra = (c & 0x0800) ? TRUE : FALSE;
	cheat_know = (c & 0x1000) ? TRUE : FALSE;
	cheat_live = (c & 0x2000) ? TRUE : FALSE;


	/*** Normal Options ***/

	/* Read the option flags */
	for (n = 0; n < 8; n++) rd_u32b(&flag[n]);

	/* Read the option masks */
	for (n = 0; n < 8; n++) rd_u32b(&mask[n]);

	/* Analyze the options */
	for (n = 0; n < 8; n++)
	{
		/* Analyze the options */
		for (i = 0; i < 32; i++)
		{
			/* Process valid flags */
			if (mask[n] & (1L << i))
			{
				/* Process valid flags */
				if (option_mask[n] & (1L << i))
				{
					/* Set */
					if (flag[n] & (1L << i))
					{
						/* Set */
						option_flag[n] |= (1L << i);
					}

					/* Clear */
					else
					{
						/* Clear */
						option_flag[n] &= ~(1L << i);
					}
				}				
			}
		}
	}


	/*** Window Options ***/

	/* Read the window flags */
	for (n = 0; n < 8; n++) rd_u32b(&flag[n]);

	/* Read the window masks */
	for (n = 0; n < 8; n++) rd_u32b(&mask[n]);

	/* Analyze the options */
	for (n = 0; n < 8; n++)
	{
		/* Analyze the options */
		for (i = 0; i < 32; i++)
		{
			/* Process valid flags */
			if (mask[n] & (1L << i))
			{
				/* Process valid flags */
				if (window_mask[n] & (1L << i))
				{
					/* Set */
					if (flag[n] & (1L << i))
					{
						/* Set */
						window_flag[n] |= (1L << i);
					}

					/* Clear */
					else
					{
						/* Clear */
						window_flag[n] &= ~(1L << i);
					}
				}				
			}
		}
	}
}
#endif





/*
 * Hack -- strip the "ghost" info
 *
 * XXX XXX XXX This is such a nasty hack it hurts.
 */
#if 0
static void rd_ghost(void)
{
	char buf[64];

	/* Strip name */
	rd_string(buf, 64);

	/* Strip old data */
	strip_bytes(60);
}
#endif




/*
 * Read/Write the "extra" information
 */

static bool rd_extra(int Ind)
{
	player_type *p_ptr = Players[Ind];
	char pass[80];

	int i;

	byte tmp8u;
	s16b tmps16b;

	rd_string(p_ptr->name, 32);

	rd_string(pass, 80);

	if (strcmp(pass, p_ptr->pass))
		return TRUE;

	rd_string(p_ptr->died_from, 80);

	/* if new enough, load the died_from_list information */
	if (!older_than(0,5,5)) 
	{
		rd_string(p_ptr->died_from_list, 80);
		rd_s16b(&p_ptr->died_from_depth);
	}

	for (i = 0; i < 4; i++)
	{
		rd_string(p_ptr->history[i], 60);
	}

	/* Class/Race/Gender/Party */
	rd_byte(&p_ptr->prace);
	rd_byte(&p_ptr->pclass);
	rd_byte(&p_ptr->male);
	rd_byte(&p_ptr->party);
	if (!older_than(3, 1, 0))
	{
		rd_byte(&p_ptr->mode);
	}
	else
	{
		p_ptr->mode = MODE_NORMAL;
	}

	/* Special Race/Class info */
	rd_byte(&p_ptr->hitdie);
	if (!older_than(3, 0, 9))
	{
		rd_s16b(&p_ptr->expfact);
	}
	else
	{
		rd_byte(&p_ptr->expfact);
	}

	/* Age/Height/Weight */
	rd_s16b(&p_ptr->age);
	rd_s16b(&p_ptr->ht);
	rd_s16b(&p_ptr->wt);

	/* Read the stat info */
	for (i = 0; i < 6; i++) rd_s16b(&p_ptr->stat_max[i]);
	for (i = 0; i < 6; i++) rd_s16b(&p_ptr->stat_cur[i]);

	rd_s32b(&p_ptr->id);

#ifdef NEWHOUSES
	rd_u32b(&p_ptr->dna);
#endif

	/* If he was created in the pre-ID days, give him one */
	if (!p_ptr->id)
		p_ptr->id = newid();

	strip_bytes(20);	/* oops */

	rd_s32b(&p_ptr->au);

	rd_s32b(&p_ptr->max_exp);
	rd_s32b(&p_ptr->exp);
	rd_u16b(&p_ptr->exp_frac);

	rd_s16b(&p_ptr->lev);

	rd_s16b(&p_ptr->mhp);
	rd_s16b(&p_ptr->chp);
	rd_u16b(&p_ptr->chp_frac);

	rd_s16b(&p_ptr->msp);
	rd_s16b(&p_ptr->csp);
	rd_u16b(&p_ptr->csp_frac);

	rd_s16b(&p_ptr->max_plv);
	rd_s16b(&p_ptr->max_dlv);

	p_ptr->recall_depth = p_ptr->max_dlv;

	rd_s16b(&p_ptr->py);
	rd_s16b(&p_ptr->px);
	if (older_than(0,5,5))
		rd_byte((char *)&p_ptr->dun_depth);
	else rd_s32b((s32b *) &p_ptr->dun_depth);

	/* read the world coordinates if new enough */
	if (!older_than(0,5,5))
	{
		rd_s16b(&p_ptr->world_x);
		rd_s16b(&p_ptr->world_y);
	}

	if (older_than(0,5,5))
		strip_bytes(1);

	/* More info */

	rd_s16b(&p_ptr->ghost);
	rd_s16b(&p_ptr->sc);
	rd_s16b(&p_ptr->fruit_bat);

	/* Read the flags */
	rd_byte(&p_ptr->lives);
	/* hack */
	rd_byte(&tmp8u);
	rd_s16b(&p_ptr->blind);
	rd_s16b(&p_ptr->paralyzed);
	rd_s16b(&p_ptr->confused);
	rd_s16b(&p_ptr->food);
	strip_bytes(4);	/* Old "food_digested" / "protection" */
	rd_s16b(&p_ptr->energy);
	rd_s16b(&p_ptr->fast);
	rd_s16b(&p_ptr->slow);
	rd_s16b(&p_ptr->afraid);
	rd_s16b(&p_ptr->cut);
	rd_s16b(&p_ptr->stun);
	rd_s16b(&p_ptr->poisoned);
	rd_s16b(&p_ptr->image);
	rd_s16b(&p_ptr->protevil);
	rd_s16b(&p_ptr->invuln);
	rd_s16b(&p_ptr->hero);
	rd_s16b(&p_ptr->shero);
	rd_s16b(&p_ptr->shield);
	rd_s16b(&p_ptr->blessed);
	rd_s16b(&p_ptr->tim_invis);
	rd_s16b(&p_ptr->word_recall);
	rd_s16b(&p_ptr->see_infra);
	rd_s16b(&p_ptr->tim_infra);
	rd_s16b(&p_ptr->oppose_fire);
	rd_s16b(&p_ptr->oppose_cold);
	rd_s16b(&p_ptr->oppose_acid);
	rd_s16b(&p_ptr->oppose_elec);
	rd_s16b(&p_ptr->oppose_pois);
	if (!older_than(3, 1, 2))
	{
		rd_s16b(&p_ptr->prob_travel);
	}
	else
	{
		p_ptr->prob_travel = 0;
	}
	if (!older_than(3, 1, 3))
	{
		rd_s16b(&p_ptr->st_anchor);
		rd_s16b(&p_ptr->tim_esp);
		rd_s16b(&p_ptr->adrenaline);
		rd_s16b(&p_ptr->biofeedback);
	}
	else
	{
		p_ptr->st_anchor = 0;
		p_ptr->tim_esp = 0;
		p_ptr->adrenaline = 0;
		p_ptr->biofeedback = 0;
	}

	rd_byte(&p_ptr->confusing);
	rd_s16b(&p_ptr->tim_wraith);
	rd_byte(&p_ptr->wraith_in_wall);
	rd_byte(&p_ptr->searching);
	rd_byte(&p_ptr->maximize);
	rd_byte(&p_ptr->preserve);
	rd_byte(&p_ptr->stunning);

	if (!older_than(3, 0, 6))
	{
		rd_s16b(&p_ptr->body_monster);
		if (older_than(3, 1, 4)) p_ptr->body_monster = 0;
		rd_s16b(&p_ptr->auto_tunnel);
	}

	rd_s16b(&p_ptr->tim_meditation);

	if (!older_than(3, 0, 0))
	{
		rd_s16b(&p_ptr->tim_invisibility);
		rd_s16b(&p_ptr->tim_invis_power);

		rd_s16b(&p_ptr->furry);
	}

	if (!older_than(3, 0, 1))
	{
		rd_s16b(&p_ptr->tim_manashield);
	}

	rd_s16b(&p_ptr->tim_traps);

	if (!older_than(3, 0, 2))
	{
		rd_s16b(&p_ptr->tim_mimic);
		rd_s16b(&p_ptr->tim_mimic_what);
	}

	/* Read the unique list info */
	if (!older_than(3, 0, 3))
	{
		int i;
		u16b tmp16u;

		/* Monster Memory */
		rd_u16b(&tmp16u);

		/* Incompatible save files */
		if (tmp16u > MAX_R_IDX)
		{
			note(format("Too many (%u) monster races!", tmp16u));
			return (22);
		}

		if (older_than(3, 0, 5))
		{
			for (i = 0; i < tmp16u; i++) rd_byte(&p_ptr->r_killed[i]);
		}
		else
		{
			for (i = 0; i < tmp16u; i++) rd_s16b(&p_ptr->r_killed[i]);
		}
	}
	else
	{
		int i;
		for (i = 0; i < MAX_R_IDX; i++) p_ptr->r_killed[i] = FALSE;
	}

	/* Future use */
	for (i = 0; i < 44; i++) rd_byte(&tmp8u);

	/* Skip the flags */
	strip_bytes(12);


	/* Hack -- the two "special seeds" */
	/*rd_u32b(&seed_flavor);
	  rd_u32b(&seed_town);*/


	/* Special stuff */
	rd_u16b(&panic_save);
	rd_u16b(&p_ptr->total_winner);
	if (!older_than(3,0,4))
	{
		rd_s16b(&p_ptr->own1);
		rd_s16b(&p_ptr->own2);
	}
	if (!older_than(0,7,0))
		rd_u16b(&p_ptr->retire_timer);
	rd_u16b(&p_ptr->noscore);

	/* Read "death" */
	rd_byte(&tmp8u);
	p_ptr->death = tmp8u;

	/* Read "feeling" */
	/*rd_byte(&tmp8u);
	  feeling = tmp8u;*/

	/* Turn of last "feeling" */
	/*rd_s32b(&old_turn);*/

	/* Current turn */
	/*rd_s32b(&turn);*/

	/* Success */
	return FALSE;
}




/*
 * Read the player inventory
 *
 * Note that the inventory changed in Angband 2.7.4.  Two extra
 * pack slots were added and the equipment was rearranged.  Note
 * that these two features combine when parsing old save-files, in
 * which items from the old "aux" slot are "carried", perhaps into
 * one of the two new "inventory" slots.
 *
 * Note that the inventory is "re-sorted" later by "dungeon()".
 */
static errr rd_inventory(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int slot = 0;

	object_type forge;

	/* No weight */
	p_ptr->total_weight = 0;

	/* No items */
	p_ptr->inven_cnt = 0;
	p_ptr->equip_cnt = 0;

	/* Read until done */
	while (1)
	{
		u16b n;

		/* Get the next item index */
		rd_u16b(&n);

		/* Nope, we reached the end */
		if (n == 0xFFFF) break;

		/* Read the item */
		rd_item(&forge);

		/* Hack -- verify item */
		if (!forge.k_idx) return (53);

		/* Mega-Hack -- Handle artifacts that aren't yet "created" */
		if (artifact_p(&forge))
		{
			/* If this artifact isn't created, mark it as created */
			/* Only if this isn't a "death" restore */
			if (!a_info[forge.name1].cur_num && !p_ptr->death)
				a_info[forge.name1].cur_num = 1;
		}

		/* Wield equipment */
		if (n >= INVEN_WIELD)
		{
			/* Structure copy */
			p_ptr->inventory[n] = forge;

			/* Add the weight */
			p_ptr->total_weight += (forge.number * forge.weight);

			/* One more item */
			p_ptr->equip_cnt++;
		}

		/* Warning -- backpack is full */
		else if (p_ptr->inven_cnt == INVEN_PACK)
		{
			/* Oops */
			/*note("Too many items in the inventory!");*/

			/* Fail */
			return (54);
		}

		/* Carry inventory */
		else
		{
			/* Get a slot */
			n = slot++;

			/* Structure copy */
			p_ptr->inventory[n] = forge;

			/* Add the weight */
			p_ptr->total_weight += (forge.number * forge.weight);

			/* One more item */
			p_ptr->inven_cnt++;
		}
	}

	/* Success */
	return (0);
}

/*
 * Read hostility information
 *
 * Note that this function is responsible for deleting stale entries
 */
static errr rd_hostilities(int Ind)
{
	player_type *p_ptr = Players[Ind];
	hostile_type *h_ptr;
	int i;
	s32b id;
	u16b tmp16u;

	/* Read number */
	rd_u16b(&tmp16u);

	/* Read available ID's */
	for (i = 0; i < tmp16u; i++)
	{
		/* Read next ID */
		rd_s32b(&id);

		/* Check for stale player */
		if (id > 0 && !lookup_player_name(id)) continue;

		/* Check for stale party */
		if (id < 0 && !parties[0 - id].num) continue;

		/* Create node */
		MAKE(h_ptr, hostile_type);
		h_ptr->id = id;

		/* Add to chain */
		h_ptr->next = p_ptr->hostile;
		p_ptr->hostile = h_ptr;
	}

	/* Success */
	return (0);
}


#if 0

/*
 * Read the saved messages
 */
static void rd_messages(void)
{
	int i;
	char buf[128];

	s16b num;

	/* Hack -- old method used circular queue */
	rd_s16b(&num);

	/* Read the messages */
	for (i = 0; i < num; i++)
	{
		/* Read the message */
		rd_string(buf, 128);

		/* Save the message */
		message_add(buf);
	}
}



/*
 * New "cave grid" flags -- saved in savefile
 */
#define OLD_GRID_W_01	0x0001	/* Wall type (bit 1) */
#define OLD_GRID_W_02	0x0002	/* Wall type (bit 2) */
#define OLD_GRID_PERM	0x0004	/* Wall type is permanent */
#define OLD_GRID_QQQQ	0x0008	/* Unused */
#define OLD_GRID_MARK	0x0010	/* Grid is memorized */
#define OLD_GRID_GLOW	0x0020	/* Grid is illuminated */
#define OLD_GRID_ROOM	0x0040	/* Grid is part of a room */
#define OLD_GRID_ICKY	0x0080	/* Grid is anti-teleport */

/*
 * Masks for the new grid types
 */
#define OLD_GRID_WALL_MASK	0x0003	/* Wall type */

/*
 * Legal results of OLD_GRID_WALL_MASK
 */
#define OLD_GRID_WALL_NONE		0x0000	/* No wall */
#define OLD_GRID_WALL_MAGMA		0x0001	/* Magma vein */
#define OLD_GRID_WALL_QUARTZ	0x0002	/* Quartz vein */
#define OLD_GRID_WALL_GRANITE	0x0003	/* Granite wall */


#endif

/*
 * Read the run-length encoded dungeon
 *
 * Note that this only reads the dungeon features and flags, 
 * the objects and monsters will be read later.
 *
 */

static errr rd_dungeon(void)
{
	s32b depth;
	u16b max_y, max_x;

	int i, y, x;
	cave_type *c_ptr;

	unsigned char runlength, feature, flags;


	/*** Depth info ***/

	/* Level info */
	rd_s32b(&depth);
	rd_u16b(&max_y);
	rd_u16b(&max_x);

	/* players on this depth */
	rd_s16b(&players_on_depth[depth]);

	/* Hack -- only read in staircase information for non-wilderness
	 * levels
	 */

	if (depth >= 0)
	{
		rd_byte(&level_up_y[depth]);
		rd_byte(&level_up_x[depth]);
		rd_byte(&level_down_y[depth]);
		rd_byte(&level_down_x[depth]);
		rd_byte(&level_rand_y[depth]);
		rd_byte(&level_rand_x[depth]);
	}

	/* allocate the memory for the dungoen */
	alloc_dungeon_level(depth);


	/*** Run length decoding ***/

	/* Load the dungeon data */
	for (x = y = 0; y < max_y; )
	{
		/* Grab RLE info */
		rd_byte(&runlength);
		rd_byte(&feature);
		rd_byte(&flags);

		/* Apply the RLE info */
		for (i = 0; i < runlength; i++)
		{
			/* Access the cave */
			c_ptr = &cave[depth][y][x];

			/* set the feature */
			c_ptr->feat = feature;

			/* set flags */
			c_ptr->info = flags;

			/* incrament our position */
			x++;
			if (x >= max_x)
			{
				/* Wrap onto a new row */
				x = 0;
				y++;
			}
		}
	}

	/* Success */
	return (0);
}

/* Reads in a players memory of the level he is currently on, in run-length encoded
 * format.  Simmilar to the above function.
 */

static errr rd_cave_memory(int Ind)
{
	player_type *p_ptr = Players[Ind];
	u16b max_y, max_x;
	int i, y, x;

	unsigned char runlength, cur_flag;

	/* Memory dimensions */
	rd_u16b(&max_y);
	rd_u16b(&max_x);


	/*** Run length decoding ***/

	/* Load the memory data */
	for (x = y = 0; y < max_y; )
	{
		/* Grab RLE info */
		rd_byte(&runlength);
		rd_byte(&cur_flag);

		/* Apply the RLE info */
		for (i = 0; i < runlength; i++)
		{
			p_ptr->cave_flag[y][x] = cur_flag;

			/* incrament our position */
			x++;
			if (x >= max_x)
			{
				/* Wrap onto a new row */
				x = 0;
				y++;

				/* paranoia */
				/* if (y >= max_y) break; */
			}
		}
	}

	/* Success */
	return (0);
}

/*
 * Actually read the savefile
 *
 * Angband 2.8.0 will completely replace this code, see "save.c",
 * though this code will be kept to read pre-2.8.0 savefiles.
 */
static errr rd_savefile_new_aux(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int i;

	u16b tmp16u, y, x, ymax, xmax;
	u32b tmp32u;


#ifdef VERIFY_CHECKSUMS
	u32b n_x_check, n_v_check;
	u32b o_x_check, o_v_check;
#endif


	/* Mention the savefile version */
	/*note(format("Loading a %d.%d.%d savefile...",
	  sf_major, sf_minor, sf_patch));*/



	/* Strip the version bytes */
	strip_bytes(4);

	/* Hack -- decrypt */
	xor_byte = sf_extra;


	/* Clear the checksums */
	v_check = 0L;
	x_check = 0L;


	/* Operating system info */
	rd_u32b(&sf_xtra);

	/* Time of savefile creation */
	rd_u32b(&sf_when);

	/* Number of resurrections */
	rd_u16b(&sf_lives);

	/* Number of times played */
	rd_u16b(&sf_saves);


	/* Later use (always zero) */
	rd_u32b(&tmp32u);

	/* Later use (always zero) */
	rd_u32b(&tmp32u);

#if 0

	/* Read RNG state */
	rd_randomizer();
	if (arg_fiddle) note("Loaded Randomizer Info");


	/* Then the options */
	rd_options();
	if (arg_fiddle) note("Loaded Option Flags");


	/* Then the "messages" */
	rd_messages();
	if (arg_fiddle) note("Loaded Messages");


	/* Monster Memory */
	rd_u16b(&tmp16u);

	/* Incompatible save files */
	if (tmp16u > MAX_R_IDX)
	{
		note(format("Too many (%u) monster races!", tmp16u));
		return (21);
	}

	/* Read the available records */
	for (i = 0; i < tmp16u; i++)
	{
		monster_race *r_ptr;

		/* Read the lore */
		rd_lore(i);

		/* Access that monster */
		r_ptr = &r_info[i];
	}
	if (arg_fiddle) note("Loaded Monster Memory");
#endif


	/* Object Memory */
	rd_u16b(&tmp16u);

	/* Incompatible save files */
	if (tmp16u > MAX_K_IDX)
	{
		note(format("Too many (%u) object kinds!", tmp16u));
		return (22);
	}

	/* Read the object memory */
	for (i = 0; i < tmp16u; i++)
	{
		byte tmp8u;

		rd_byte(&tmp8u);

		Players[Ind]->obj_aware[i] = (tmp8u & 0x01) ? TRUE : FALSE;
		Players[Ind]->obj_tried[i] = (tmp8u & 0x02) ? TRUE : FALSE;
	}
	/*if (arg_fiddle) note("Loaded Object Memory");*/

#if 0

	/* Load the Quests */
	rd_u16b(&tmp16u);

	/* Incompatible save files */
	if (tmp16u > 4)
	{
		note(format("Too many (%u) quests!", tmp16u));
		return (23);
	}

	/* Load the Quests */
	for (i = 0; i < tmp16u; i++)
	{
		rd_byte(&tmp8u);
		q_list[i].level = tmp8u;
		rd_byte(&tmp8u);
		rd_byte(&tmp8u);
		rd_byte(&tmp8u);
	}
	if (arg_fiddle) note("Loaded Quests");


	/* Load the Artifacts */
	rd_u16b(&tmp16u);

	/* Incompatible save files */
	if (tmp16u > MAX_A_IDX)
	{
		note(format("Too many (%u) artifacts!", tmp16u));
		return (24);
	}

	/* Read the artifact flags */
	for (i = 0; i < tmp16u; i++)
	{
		rd_byte(&tmp8u);
		a_info[i].cur_num = tmp8u;
		rd_byte(&tmp8u);
		rd_byte(&tmp8u);
		rd_byte(&tmp8u);
	}
	if (arg_fiddle) note("Loaded Artifacts");
#endif


	/* Read the extra stuff */
	if (rd_extra(Ind))
		return 35;
	/*if (arg_fiddle) note("Loaded extra information");*/


	/* Read the player_hp array */
	rd_u16b(&tmp16u);

#if 0
	/* Incompatible save files */
	if (tmp16u > PY_MAX_LEVEL)
	{
		note(format("Too many (%u) hitpoint entries!", tmp16u));
		return (25);
	}
#endif

	/* Read the player_hp array */
	for (i = 0; i < tmp16u; i++)
	{
		rd_s16b(&p_ptr->player_hp[i]);
	}


	/* Important -- Initialize the race/class */
	p_ptr->rp_ptr = &race_info[p_ptr->prace];
	p_ptr->cp_ptr = &class_info[p_ptr->pclass];

	/* Important -- Choose the magic info */
	p_ptr->mp_ptr = &magic_info[p_ptr->pclass];


	/* Read spell info */
	rd_u32b(&p_ptr->spell_learned1);
	rd_u32b(&p_ptr->spell_learned2);
	rd_u32b(&p_ptr->spell_worked1);
	rd_u32b(&p_ptr->spell_worked2);
	rd_u32b(&p_ptr->spell_forgotten1);
	rd_u32b(&p_ptr->spell_forgotten2);

	/* Hack, rogue spells tottaly changed */
	if ((p_ptr->pclass == CLASS_ROGUE) && (older_than(3,0,0)))
	{
		p_ptr->spell_learned1 = 0;
		p_ptr->spell_learned2 = 0;
		p_ptr->spell_worked1 = 0;
		p_ptr->spell_worked2 = 0;
		p_ptr->spell_forgotten1 = 0;
		p_ptr->spell_forgotten2 = 0;
		p_ptr->update |= PU_SPELLS;
	}

	for (i = 0; i < 64; i++)
	{
		rd_byte(&p_ptr->spell_order[i]);
	}

	/* 
	   quick hack to fix messed up my spells...
	   added after the "chaos storm"	
	   -APD-


	   if (!(strcmp(p_ptr->name,"Tarik")))
	   {	
	   p_ptr->spell_learned1 = 0;
	   p_ptr->spell_learned2 = 0;
	   p_ptr->spell_worked1 = 0;
	   p_ptr->spell_worked2 = 0;
	   p_ptr->spell_forgotten1 = 0;
	   p_ptr->spell_forgotten2 = 0;
	   p_ptr->update |= PU_SPELLS;
	   }

*/	

	/* Read the inventory */
	if (rd_inventory(Ind))
	{
		/*note("Unable to read inventory");*/
		return (21);
	}

	/* Read hostility information if new enough */
	if (!older_than(0, 5, 3))
	{
		if (rd_hostilities(Ind))
		{
			return (22);
		}
	}

	/* read the dungeon memory if new enough */
	if (!older_than(0,5,7))
	{
		rd_cave_memory(Ind);
	}

	/* read the wilderness map if new enough */
	if (!older_than(0,5,5))
	{
		/* get the map size */
		rd_u32b(&tmp32u);

		/* if too many map entries */
		if (tmp32u > MAX_WILD)
		{
			return 23;
		}

		/* read in the map */
		for (i = 0; i < tmp32u; i++)
		{
			rd_byte(&p_ptr->wild_map[i]);
		}
	}


#if 0
	/* Read the stores */
	rd_u16b(&tmp16u);
	for (i = 0; i < tmp16u; i++)
	{
		if (rd_store(i)) return (22);
	}


	/* I'm not dead yet... */
	if (!death)
	{
		/* Dead players have no dungeon */
		note("Restoring Dungeon...");
		if (rd_dungeon())
		{
			note("Error reading dungeon data");
			return (34);
		}

		/* Read the ghost info */
		rd_ghost();
	}

#endif

#ifdef VERIFY_CHECKSUMS

	/* Save the checksum */
	n_v_check = v_check;

	/* Read the old checksum */
	rd_u32b(&o_v_check);

	/* Verify */
	if (o_v_check != n_v_check)
	{
		note("Invalid checksum");
		return (11);
	}


	/* Save the encoded checksum */
	n_x_check = x_check;

	/* Read the checksum */
	rd_u32b(&o_x_check);


	/* Verify */
	if (o_x_check != n_x_check)
	{
		note("Invalid encoded checksum");
		return (11);
	}

#endif


	/* Hack -- no ghosts */
	r_info[MAX_R_IDX-1].max_num = 0;

	/* Initialize a little more */
	p_ptr->ignore = NULL;
	p_ptr->afk = FALSE;

	/* Success */
	return (0);
}


/*
 * Actually read the savefile
 *
 * Angband 2.8.0 will completely replace this code, see "save.c",
 * though this code will be kept to read pre-2.8.0 savefiles.
 */
errr rd_savefile_new(int Ind)
{
	player_type *p_ptr = Players[Ind];

	errr err;

	/* The savefile is a binary file */
	fff = my_fopen(p_ptr->savefile, "rb");

	/* Paranoia */
	if (!fff) return (-1);

	/* Call the sub-function */
	err = rd_savefile_new_aux(Ind);

	/* Check for errors */
	if (ferror(fff)) err = -1;

	/* Close the file */
	my_fclose(fff);

	/* Result */
	return (err);
}

errr rd_server_savefile()
{
	int i;

	errr err = 0;

	char savefile[1024];

	byte tmp8u;
	u16b tmp16u;
	u32b tmp32u;
	s32b tmp32s;

	/* Savefile name */
	path_build(savefile, 1024, ANGBAND_DIR_SAVE, "server");

	/* The server savefile is a binary file */
	fff = my_fopen(savefile, "rb");

	/* Paranoia */
	if (!fff) return (-1);

	/* Strip the version bytes */
	strip_bytes(4);

	/* Hack -- decrypt */
	xor_byte = sf_extra;


	/* Clear the checksums */
	v_check = 0L;
	x_check = 0L;


	/* Operating system info */
	rd_u32b(&sf_xtra);

	/* Time of savefile creation */
	rd_u32b(&sf_when);

	/* Number of lives */
	rd_u16b(&sf_lives);

	/* Number of times played */
	rd_u16b(&sf_saves);


	/* Later use (always zero) */
	rd_u32b(&tmp32u);

	/* Later use (always zero) */
	rd_u32b(&tmp32u);

	/* Monster Memory */
	rd_u16b(&tmp16u);

	/* Incompatible save files */
	if (tmp16u > MAX_R_IDX)
	{
		note(format("Too many (%u) monster races!", tmp16u));
		return (21);
	}

	/* Read the available records */
	for (i = 0; i < tmp16u; i++)
	{
		monster_race *r_ptr;

		/* Read the lore */
		rd_lore(i);

		/* Access the monster race */
		r_ptr = &r_info[i];

		/* Special hack -- bring back all uniques if this is a pre-0.4.1 savefile */
		if (older_than(0,4,1))
		{
			/* Check for unique */
			if (r_ptr->flags1 & RF1_UNIQUE)
				r_ptr->max_num = 1;
		}
	}

	/* Load the Artifacts */
	rd_u16b(&tmp16u);

	/* Incompatible save files */
	if (tmp16u > MAX_A_IDX)
	{
		note(format("Too many (%u) artifacts!", tmp16u));
		return (24);
	}

	/* Read the artifact flags */
	for (i = 0; i < tmp16u; i++)
	{
		rd_byte(&tmp8u);
		a_info[i].cur_num = tmp8u;
		rd_byte(&tmp8u);
		rd_byte(&tmp8u);
		rd_byte(&tmp8u);
	}

	/* Read the stores */
	rd_u16b(&tmp16u);
	for (i = 0; i < tmp16u; i++)
	{
		if (rd_store(i)) return (22);
	}

	/* Read party info if savefile is new enough */
	if (older_than(0,3,0))
	{
		/* Set party zero's name to "Neutral" */
		strcpy(parties[0].name, "Neutral");
	}

	/* New enough to have party info */
	else
	{
		rd_u16b(&tmp16u);

		/* Incompatible save files */
		if (tmp16u > MAX_PARTIES)
		{
			note(format("Too many (%u) parties!", tmp16u));
			return (25);
		}

		/* Read the available records */
		for (i = 0; i < tmp16u; i++)
		{
			rd_party(i);
		}
	}

	/* XXX If new enough, read in the saved levels and monsters. */

	if (!older_than(0,5,7))
	{
		/* read the number of levels to be loaded */
		rd_u32b(&tmp32u);
		/* load the levels */
		for (i = 0; i < tmp32u; i++) rd_dungeon();

		/* get the number of monsters to be loaded */
		rd_u32b(&tmp32u);
		if (tmp32u > MAX_M_IDX)
		{
			note(format("Too many (%u) monsters!", tmp16u));
			return (29);
		}
		/* load the monsters */
		for (i = 0; i < tmp32u; i++)
		{
			rd_monster(&m_list[m_pop()]);
		}
	}

	/* Read object info if new enough */
	if (!older_than(0,4,0))
	{
		rd_u16b(&tmp16u);

		/* Incompatible save files */
		if (tmp16u > MAX_O_IDX)
		{
			note(format("Too many (%u) objects!", tmp16u));
			return (26);
		}

		/* Read the available records */
		for (i = 0; i < tmp16u; i++)
		{		
			rd_item(&o_list[i]);
		}

		/* Set the maximum object number */
		o_max = tmp16u;
	}

	/* Read house info if new enough */
	if (!older_than(0,4,0))
	{
		rd_u32b(&num_houses);

		while(house_alloc<num_houses){
			GROW(houses, house_alloc, house_alloc+512, house_type);
			house_alloc+=512;
		}

		/* Incompatible save files */
		if (num_houses > MAX_HOUSES)
		{
			note(format("Too many (%u) houses!", num_houses));
			return (27);
		}

		/* Read the available records */
		for (i = 0; i < num_houses; i++)
		{
			rd_house(i);
		}
		/* insert houses into wild space if needed */
		for (i=-MAX_WILD;i<0;i++){
			if(cave[i]){
				int j;
				for(j=0;j<num_houses;j++){
					if(houses[j].depth==i){
						int x,y;
						/* add the house dna */	
						x=houses[j].dx;
						y=houses[j].dy;
						cave[i][y][x].special.type=DNA_DOOR;
						cave[i][y][x].special.ptr=houses[j].dna;
					}
				}
			}
		}
	}

	/* Read wilderness info if new enough */
	if (!older_than(0,5,5))
	{
		/* read how many wilderness levels */
		rd_u32b(&tmp32u);

		if (tmp32u > MAX_WILD)
		{
			note("Too many wilderness levels");
			return 28;
		}

		for (i = 1; i < tmp32u; i++)
		{
			rd_wild(i);
		}	
	}

	/* Read the player name database if new enough */
	if (!older_than(0,4,1))
	{
		char name[80];

		rd_u32b(&tmp32u);

		/* Read the available records */
		for (i = 0; i < tmp32u; i++)
		{
			time_t laston;

			/* Read the ID */
			rd_s32b(&tmp32s);
			rd_s32b(&laston);

			/* Read the player name */
			rd_string(name, 80);

			/* Store the player name */
			add_player_name(name, tmp32s, laston);
		}
	}

	rd_u32b(&seed_flavor);
	rd_u32b(&seed_town);

	if (!older_than(0,4,1))
	{
		rd_s32b(&player_id);
	}
	else player_id = 1;

	rd_s32b(&turn);

	/* Hack -- no ghosts */
	r_info[MAX_R_IDX-1].max_num = 0;


	/* Check for errors */
	if (ferror(fff)) err = -1;

	/* Close the file */
	my_fclose(fff);

	/* Result */
	return (err);
}

