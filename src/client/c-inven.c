/* $Id$ */
#include "angband.h"


/* This is actually not very smart but done with a bad hack -
   Try to avoid swapping an item for another one which are actually identical,
   but search the inventory for an alternative item to use in that case.
   Example: Player has 3x teleport ring @x0 and 1x protection ring @x0,
   he certainly doesn't want to swap one teleport ring for another one.
   The problem is that item names aren't identical due to number/article and grammar,
   eg '3 rings' vs 'a ring'. The current hack just checks for ' of ' in the item name
   and only compares the rest of it, ignoring the first part, so it basically only
   works for identified flavoured items atm. Better than nothing maybe.
   (The best ever state would be of course if the client could check if the items are
   really totally identical and then even takes off the equipped item if the only
   items in inventory that are fittingly inscribed are actually identical ones ^^.)
   Note: SMART_SWAP needs the 'smart handling' code in cmd_swap() to work properly. */
#define SMART_SWAP

/* In item prompts, allow switching from normal inven/equip into subinventory? */
#ifdef ENABLE_SUBINVEN
 #define ITEM_PROMPT_ALLOWS_SWITCHING_TO_SUBINVEN
#endif



bool verified_item = FALSE;
bool abort_prompt = FALSE;

static int get_tag(int *cp, char tag, bool inven, bool equip, int mode);



s16b index_to_label(int i) {
	/* Indices for "inven" are easy */
	if (i < INVEN_WIELD) return(I2A(i));

	/* Indices for "equip" are offset */
	return(I2A(i - INVEN_WIELD));
}


bool item_tester_okay(object_type *o_ptr) {
	/* Hack for live_timeouts: If showing equip via cmd_equip(), always show all slots. */
	if (showing_equip) return(TRUE);

	/* Hack -- allow testing empty slots */
	if (item_tester_full) return(TRUE);

	/* Require an item */
	if (!o_ptr->tval) return(FALSE);

	/* Hack -- ignore "gold" */
	if (o_ptr->tval == TV_GOLD) return(FALSE);

	/* Check the tval */
	if (item_tester_tval) {
		if (!(item_tester_tval == o_ptr->tval)) return(FALSE);
	}

	/* Check the weight */
	if (item_tester_max_weight) {
		if (item_tester_max_weight < o_ptr->weight * o_ptr->number) return(FALSE);
	}

	/* Check the hook */
	if (item_tester_hook) {
		if (!(*item_tester_hook)(o_ptr)) return(FALSE);
	}

	/* Assume okay */
	return(TRUE);
}


static bool get_item_okay(int i) {
#ifdef ENABLE_SUBINVEN
	if (using_subinven != -1) {
		/* Illegal items */
		if ((i < 0) || (i >= using_subinven_size)) return(FALSE);

		/* Verify the item */
		if (!item_tester_okay(&subinventory[using_subinven][i])) return(FALSE);

		/* Assume okay */
		return(TRUE);
	} else if (i >= SUBINVEN_INVEN_MUL) {
		int s = i / SUBINVEN_INVEN_MUL - 1;

		i = i % SUBINVEN_INVEN_MUL;

		/* Illegal items */
		if ((i < 0) || (i >= inventory[s].bpval)) return(FALSE);

		/* Verify the item */
		if (!item_tester_okay(&subinventory[s][i])) return(FALSE);

		/* Assume okay */
		return(TRUE);
	}
#endif
	/* Illegal items */
	if ((i < 0) || (i >= INVEN_TOTAL)) return(FALSE);

	/* Verify the item */
	if (!item_tester_okay(&inventory[i])) return(FALSE);

	/* Assume okay */
	return(TRUE);
}

/* For c_get_item(): Capital letter leads to asking whether we really want to try that item. */
static bool verify(cptr prompt, int item) {
	char	o_name[ONAME_LEN];
	char	out_val[MSG_LEN];

	/* Describe */
#ifdef ENABLE_SUBINVEN
	if (using_subinven != -1)
		strcpy(o_name, subinventory_name[using_subinven][item]);
	else
#endif
	strcpy(o_name, inventory_name[item]);

	/* Prompt */
	(void)sprintf(out_val, "%s %s?", prompt, o_name);

	/* Query */
	return(get_check2(out_val, FALSE));
}


static s16b c_label_to_inven(int c) {
	int i;

	/* Convert */
	i = (islower(c) ? A2I(c) : -1);

	/* Verify the index */
	if ((i < 0) || (i > INVEN_PACK)) return(-1);

	/* Empty slots can never be chosen */
#ifdef ENABLE_SUBINVEN
	if (using_subinven != -1) {
		if (!subinventory[using_subinven][i].tval) return(-1);
	} else
#endif
	if (!inventory[i].tval) return(-1);

	/* Return the index */
	return(i);
}

static s16b c_label_to_equip(int c) {
	int i;

	/* Convert */
	i = (islower(c) ? A2I(c) : -1) + INVEN_WIELD;

	/* Verify the index */
	if ((i < INVEN_WIELD) || (i >= INVEN_TOTAL)) return(-1);

	/* Empty slots can never be chosen */
	if (!inventory[i].tval) return(-1);

	/* Return the index */
	return(i);
}

/* Helper function for get_tag().
  'i' can be inventory or subinventory index. */
static int get_tag_aux(int i, int *cp, char tag, int mode) {
	char *buf;
	cptr s;
	bool charged = (mode & CHECK_CHARGED) != 0, charged_ready = (mode & CHECK_CHARGED_READY) != 0;
	int mode_ready = (mode & ~CHECK_CHARGED) | CHECK_CHARGED_READY; /* Must prevent accidental recursion by removing CHECK_CHARGED, instead we want to check for readiness-to-use! */
	int mode_unmulti = (mode & ~CHECK_MULTI);
	int k, tval, sval, pval;
#ifdef SMART_SWAP
	int m, method;
	bool chk_multi = (mode & CHECK_MULTI) != 0;
	char *oname1ptr, *oname2ptr;
#endif
#ifdef ENABLE_SUBINVEN
	int si = -1;

	if (i >= SUBINVEN_INVEN_MUL) {
		si = i / SUBINVEN_INVEN_MUL - 1;
		i = i % SUBINVEN_INVEN_MUL;

		buf = subinventory_name[si][i];
		tval = subinventory[si][i].tval;
		sval = subinventory[si][i].sval;
		pval = subinventory[si][i].pval; //basically only for spellbooks

		/* Skip empty objects */
		if (!buf[0]) return(FALSE);

		if (!item_tester_okay(&subinventory[si][i])) return(FALSE);
	} else
#endif
	{
		buf = inventory_name[i];
		tval = inventory[i].tval;
		sval = inventory[i].sval;
		pval = inventory[i].pval; //basically only for spellbooks

		/* Skip empty objects */
		if (!buf[0]) return(FALSE);

		/* Skip items that don't fit (for mkey) */
		if (!item_tester_okay(&inventory[i])) return(FALSE);
	}

#ifdef SMART_SWAP
	/* Compare tval and sval directly, assuming they are available - true for non-flavour items */
	if (sval != 255 || tval == TV_BOOK) {
		oname1ptr = buf;
		method = 1; /* compare tval+sval (non-flavour items, or known flavour items (hahaa!)) */
	}
	/* Compare item names, assuming flavoured item*/
	else {
		oname1ptr = strstr(buf, " of ");
		if (!oname1ptr) method = 3; /* aka auto-accept anything -_- */
		else method = 2; /* compare rest of the item name (unknown flavoured items) */
	}
#endif

	/* Skip empty inscriptions (those which don't contain any @.. tag) */
	if (!(s = strchr(buf, '@'))) return(FALSE);

	/* Init with 'no valid item found' */
	*cp = -1;

	/* Process all tags */
	while (s) {
		/* Check the normal tags (w/o command key) and special tags */
		if (s[1] != tag && (s[1] != command_cmd || s[2] != tag)) {
			/* Tag failed us, find another tag aka next '@' */
			s = strchr(s + 1, '@');
			continue;
		}

		/* Found tagged item! */

		/* Charged items: Check if not ready to use */
		if (charged && (
		    strstr(buf, "(charging)") || strstr(buf, "(#)") || /* rods (and other devices, in theory) */
		    //strstr(buf, "(partially charging)") || strstr(buf, "(~)") ||
		    strstr(buf, "(0 charges)") || strstr(buf, "{empty}") /* wands, staves */
		    )) {
			k = -1;

			/* WIELD_DEVICES : Check equip first! */
			get_tag(&k, tag, FALSE, TRUE, mode_ready);
			/* unnecessary check, but whatever -- no, actually this should even be disabled if we want to use the all-in-one command with two different item types of same tag!*/
			//if (k != -1 && tval != inventory[k].tval) k = -1;

			/* Now check inven/subinven */
			if (k == -1) get_tag(&k, tag, TRUE, FALSE, mode_ready);
			/* unnecessary check, but whatever -- no, actually this should even be disabled if we want to use the all-in-one command with two different item types of same tag!*/
			//if (k != -1 && tval != (k >= SUBINVEN_INVEN_MUL ? subinventory[k / SUBINVEN_INVEN_MUL - 1][k % SUBINVEN_INVEN_MUL].tval : inventory[k].tval)) k = -1;

			/* Found a ready-to-use replacement magic device for our still-charging/empty one! */
			if (k != -1) {
#ifdef ENABLE_SUBINVEN
				if (k >= SUBINVEN_INVEN_MUL) {
					si = k / SUBINVEN_INVEN_MUL - 1;
					i = k % SUBINVEN_INVEN_MUL;
				} else {
					si = -1;
					i = k;
				}
#else
				i = k;
#endif
			}
		} else if (charged_ready) {
			if (strstr(buf, "(charging)") || strstr(buf, "(#)") || /* rods (and other devices, in theory) */
			    //strstr(buf, "(partially charging)") || strstr(buf, "(~)") ||
			    strstr(buf, "(0 charges)") || strstr(buf, "{empty}") /* wands, staves */
			    )
				return(FALSE);
#ifdef ENABLE_SUBINVEN
			if (si != -1) *cp = (si + 1) * SUBINVEN_INVEN_MUL + i;
			else
#endif
			*cp = i;
			return(TRUE);
		}
		/* Accept item, for now! (Still gotta pass smart swap perhaps, see next below) */

#ifdef SMART_SWAP
		/* Cannot smart-swap items inside subinventories or from equipment (only _to_ equipment) */
		if (!chk_multi || i > INVEN_PACK || si != -1) {
#endif
#ifdef ENABLE_SUBINVEN
			if (si != -1) *cp = (si + 1) * SUBINVEN_INVEN_MUL + i;
			else
#endif
			*cp = i;

			return(TRUE);
#ifdef SMART_SWAP
		}

		/* Check for tag-matching, same tval+sval item in the equipment.
		   If found, rather search the inventory for tag-matching non-same-tval+sval alternative.
		   If we cannot find such an inventory item, go with the original inventory item. */
		k = -1;
		for (m = INVEN_WIELD; m < INVEN_TOTAL; m++) {
			if (!inventory[m].tval) continue;

			switch (method) {
			case 1:
				/* Compare tval and sval directly, assuming they are available - true for non-flavour items.
				   Almost no need to check bpval or pval, because switching items in such a manner doesn't make much sense,
				   not even for polymorph rings, EXCEPT for spell books! */
				if (inventory[m].tval != tval || inventory[m].sval != sval || (inventory[m].tval == TV_BOOK && inventory[m].sval == SV_SPELLBOOK && inventory[m].pval != pval)
				    //only for books (as we might stack multiple of the same type), rings (as only these and weapons have two equip slots - and for weapons we might want this (eg 2x dagger vs 1x 2hander))!
				    || (inventory[m].tval != TV_RING && inventory[m].tval != TV_BOOK))
					continue;
				break;
			case 2:
				/* Compare item names, assuming flavoured item*/
				oname2ptr = strstr(inventory_name[m], " of ");

				/* paranoia or very long item name that got shortened, omitting the 'of', for some reason by object_desc() perhaps -
				   anyway, we accept the item for now, might need changing -_- */
				if (!oname2ptr) break;

				/* If name is equal -> 'Success' - same item exists in equipment as our original inventory item :/.
				    So, postpone our original item for now and continue searching the equipment for a different one instead. */
				if (strcmp(oname1ptr, oname2ptr)) continue;
				break;
			case 3: /* Badly processable item name - always accept -_- */
				break;
			}

			/* Final check: It must have a matching tag though, of course */
			if (get_tag_aux(m, &k, tag, mode_unmulti)) {
				break;
			}
		}

		/* Found no equipment alternative */
		if (k == -1) {
 #ifdef ENABLE_SUBINVEN
			if (si != -1) *cp = (si + 1) * SUBINVEN_INVEN_MUL + i;
			else
 #endif
			*cp = i;
			/* Success */
			return(TRUE);
		}

		/* Found valid alternative in equipment.
		   Remember our original item to use it if we don't find any further, different item in inventory,
		   which would take precedence over this equipment item.. */
 #ifdef ENABLE_SUBINVEN
		if (si != -1) *cp = (si + 1) * SUBINVEN_INVEN_MUL + i;
		else
 #endif
		*cp = i;
		/* Hack: We did find an item, but it must be postponed.
		   We express it by setting the item (*cp) correctly, but returning FALSE anyway. */
		return(FALSE);
#endif
	}
	return(FALSE);
}

/*
 * Find the "first" inventory object with the given "tag".
 *
 * A "tag" is a char "n" appearing as "@n" anywhere in the
 * inscription of an object.
 *
 * Also, the tag "@xn" will work as well, where "n" is a tag-char,
 * and "x" is the "current" command_cmd code.
 *
 * Addition: flags 'inven' and 'equip' tell it which items to check:
 * If one of them is false, the according part is ignored. - C. Blue
 * mode flags checked here: INVEN_FIRST, CHECK_MULTI, CHECK_CHARGED.
 *
 * NOTE: Unlike c_get_item() which differentiates between inven, subinven and equip,
 *       get_tag() currently only knows 'inven' and 'equip', and treats 'inven' as
 *       encompassing both, inventory and subinventory. So it needs to be called from
 *       c_get_item() with 'inven | subinven' for its 'inven' parameter in order to
 *       access tags in bags while 'inven' is actually FALSE in c_get_item() when it calls get_tag().
 */
static int get_tag(int *cp, char tag, bool inven, bool equip, int mode) {
	int i, j, si;
	int start, stop, step;
	bool inven_first = (mode & INVEN_FIRST) != 0;
#ifdef SMART_SWAP
	int i_found = -1, tval = -1, sval = -1;
#endif

	/* neither inventory nor equipment is allowed to be searched? */
	if (!inven && !equip) return(FALSE);

	/* Search tag in inventory before looking in equipment? (default is other way round).
	   (Note: The overflow slot, which is INVEN_WIELD - 1, equal to INVEN_PACK, will also be processed.) */
	if (inven_first) {
		start = inven ? 0 : INVEN_WIELD;
		stop = equip ? INVEN_TOTAL : INVEN_WIELD;
		step = 1;
	} else {
		start = equip ? INVEN_TOTAL : INVEN_WIELD;
		stop = inven ? 0 : INVEN_WIELD;
		step = -1;
	}

	/* Init with 'no valid item found' */
	*cp = -1;

	/* Check every object */
	for (j = start; j != stop; j += step) {
		/* translate back so order within each - inven & equip - is alphabetically again */
		if (inven_first)
			i = j;
		else /* Translate, so equip and inven are processed in normal order _each_ */
			i = INVEN_WIELD + (j > INVEN_WIELD ? INVEN_TOTAL : 0) - j;

		if (!inventory[i].tval) continue;

#ifdef SMART_SWAP
		if (i_found != -1) {
			/* Skip same-type inventory items to become our alternative replacement item for swapping.
			   TODO: Apply method 1/2/3 here, instead of just 1. */
			if (tval == TV_RING && //(only for books (as we might stack multiple of the same type),) rings (as only these and weapons have two equip slots - and for weapons we might want this (eg 2x dagger vs 1x 2hander))!
			    tval == inventory[i].tval && sval == inventory[i].sval)
				continue;
		}
#endif

		if (get_tag_aux(i, cp, tag, mode)) return(TRUE);
#ifdef SMART_SWAP
		/* Check for 'postponed' hack: */
		if (*cp != -1) {
			/* We did find a valid item but postpone it for later, as we search for another alternative item first */
			i_found = *cp;
			tval = inventory[*cp].tval;
			sval = inventory[*cp].sval;
			/* Continue searching for an item */
			*cp = -1;
			/* Only search the inventory this time though -- assume non inven_first aka reversed traversal direction >_> */
			if (stop > INVEN_PACK) stop = INVEN_PACK;
			if (j >= stop) j = stop - 1;
			/* Do not AGAIN replace the alternative item we might find */
			mode &= ~CHECK_MULTI;
			/* No need to scan for subinventories below, as we may not use them for smart-swapping */
			continue;
		}
#endif

#ifdef ENABLE_SUBINVEN
		/* Also scan all items inside sub-bags */
		if (inventory[i].tval == TV_SUBINVEN)
			for (si = 0; si < inventory[i].bpval; si++) {
				if (!subinventory[i][si].tval) break;
				if (get_tag_aux((i + 1) * SUBINVEN_INVEN_MUL + si, cp, tag, mode)) return(TRUE);
				/* (Note: Items in subinven cannot be smart-swapped, so there is no check here regarding that, unlike above for normal inventory.) */
			}
#endif
	}

#ifdef SMART_SWAP
	if (i_found != -1) {
		*cp = i_found;
		return(TRUE);
	}
#endif

	/* No such tag */
	return(FALSE);
}

/*
 * General function to find an item by its name
 *
 * This is modified code from ToME. - mikaelh
 */
cptr get_item_hook_find_obj_what;
bool get_item_hook_find_obj(int *item, int mode) {
	bool inven_first = (mode & INVEN_FIRST) != 0;
	int i, j, start = inven_first ? 0 : INVEN_TOTAL, stop = inven_first ? INVEN_TOTAL : 0, step = inven_first ? 1 : -1;
	char buf[ONAME_LEN];
	char buf1[ONAME_LEN], buf2[ONAME_LEN], *ptr; /* for manual strcasestr() */
#ifdef SMART_SWAP
	char buf3[ONAME_LEN];
	bool chk_multi = (mode & CHECK_MULTI) != 0;
	int i_found = -1, tval = -1, sval = -1, pval = -1, method;
#endif
	bool charged = (mode & CHECK_CHARGED) != 0;
#ifdef ENABLE_SUBINVEN
	bool subinven = (mode & USE_SUBINVEN);
#endif
	object_type *o_ptr;

	strcpy(buf, "");
	if (!get_string(get_item_hook_find_obj_what, buf, 79)) return(FALSE);

#ifdef ENABLE_SUBINVEN
	if (using_subinven != -1) {
		for (j = 0; j < using_subinven_size; j++) {
			i = j; /* Identity translation (too lazy to change copy-pasted code to remove this =p WOW!) */

			o_ptr = &subinventory[using_subinven][i];
			if (o_ptr->tval) break;
			if (!item_tester_okay(o_ptr)) continue;
 #if 0
			if (!my_strcasestr(subinventory_name[using_subinven][i], buf)) continue;
 #else
			strcpy(buf1, subinventory_name[using_subinven][i]);
			strcpy(buf2, buf);
			ptr = buf1;
			while (*ptr) {
				/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
				   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
				if (*ptr == '@') {
					ptr++;
					if (!*ptr) break;
				} else *ptr = tolower(*ptr);
				ptr++;
			}
			ptr = buf2;
			while (*ptr) {
				/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
				   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
				if (*ptr == '@') {
					ptr++;
					if (!*ptr) break;
				} else *ptr = tolower(*ptr);
				ptr++;
			}
			if (!strstr(buf1, buf2)) continue;
 #endif

			if (charged && (
			    strstr(buf1, "(charging)") || strstr(buf1, "(#)") || /* rods (and other devices, in theory) */
			    //(partially charging) || strstr(buf1, "(~)")
			    strstr(buf1, "(0 charges") || strstr(buf1, "{empty}") /* wands, staves */
			    )) {
				/* Especially added for non-stackable rods (Havoc): check for same rod, but not 'charging' */
				char *buf1p, *buf3p;
				int k;

				if (!(buf1p = strstr(buf1, " of "))) buf1p = buf1; //skip item's article/amount
				for (k = 0; k <= using_subinven_size; k++) {
					if (!subinventory[using_subinven][k].tval) continue;
					if (k == i) continue;

					strcpy(buf3, subinventory_name[using_subinven][k]);
					ptr = buf3;
					while (*ptr) {
						/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
						   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
						if (*ptr == '@') {
							ptr++;
							if (!*ptr) break;
						} else *ptr = tolower(*ptr);
						ptr++;
					}
					/* Skip fully charging stacks */
					if (strstr(buf3, "(charging)") || strstr(buf3, "(#)") || /* rods (and other devices, in theory) */
					    //(partially charging) || strstr(buf3, "(~)")
					    strstr(buf3, "(0 charges") || strstr(buf3, "{empty}")) /* wands, staves */
						continue;

					if (!(buf3p = strstr(buf3, " of "))) buf3p = buf3; //skip item's article/amount
					if (//subinventory[using_subinven][k].tval == subinventory[using_subinven][i].tval && /* unnecessary check, but whatever -- no, actually this should even be disabled if we want to use the all-in-one command with two different item types of same tag!*/
					    strstr(buf3, buf2)) {
						i = k;
						break;
					}
				}
			}
			*item = i;
			return(TRUE);
		}
		return(FALSE);
	} else if (subinven) {
		/* Scan all subinvens for item name match */
		int l;
		object_type *o_ptr;

		/* Exception: If !inven_first, we need to scan equip, then subinvens, then normal inven.
		   This is important for magic devices that can be equipped (WIELD_DEVICES). */
		if (!inven_first) {
			/* Scan the equipment now, before subinventories */
			for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
				if (!inventory[i].tval) continue;

				o_ptr = &inventory[i];
				if (!item_tester_okay(o_ptr)) continue;
#if 0
				if (!my_strcasestr(inventory_name[i], buf)) continue;
#else
				strcpy(buf1, inventory_name[i]);
				strcpy(buf2, buf);
				ptr = buf1;
				while (*ptr) {
					/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
					   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
					if (*ptr == '@') {
						ptr++;
						if (!*ptr) break;
					} else *ptr = tolower(*ptr);
					ptr++;
				}
				ptr = buf2;
				while (*ptr) {
					/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
					   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
					if (*ptr == '@') {
						ptr++;
						if (!*ptr) break;
					} else *ptr = tolower(*ptr);
					ptr++;
				}
//printf("comparing '%s','%s'\n", buf1, buf2);
				if (!strstr(buf1, buf2)) continue;
#endif

				if (charged && (
				    strstr(buf1, "(charging)") || strstr(buf1, "(#)") || /* rods (and other devices, in theory) */
				    //(partially charging) || strstr(buf1, "(~)")
				    strstr(buf1, "(0 charges") || strstr(buf1, "{empty}") /* wands, staves */
				    )) {
					/* Especially added for non-stackable rods (Havoc): check for same rod, but not 'charging' */
					char *buf1p, *buf3p;
					int k;

					if (!(buf1p = strstr(buf1, " of "))) buf1p = buf1; //skip item's article/amount
					for (k = 0; k < INVEN_PACK; k++) {
						if (!inventory[k].tval) continue;
						if (k == i) continue;

						if (inventory[k].tval == TV_SUBINVEN) {
							for (j = 0; j < inventory[k].bpval; j++) {
								if (!subinventory[k][j].tval) break;

								strcpy(buf3, subinventory_name[k][j]);
								ptr = buf3;
								while (*ptr) {
									/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
									   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
									if (*ptr == '@') {
										ptr++;
										if (!*ptr) break;
									} else *ptr = tolower(*ptr);
									ptr++;
								}
								/* Skip fully charging stacks */
								if (strstr(buf3, "(charging)") || strstr(buf3, "(#)") || /* rods (and other devices, in theory) */
								    //(partially charging) || strstr(buf3, "(~)")
								    strstr(buf3, "(0 charges") || strstr(buf3, "{empty}")) /* wands, staves */
									continue;

								if (!(buf3p = strstr(buf3, " of "))) buf3p = buf3; //skip item's article/amount
								if (//subinventory[k][j].tval == inventory[i].tval && /* unnecessary check, but whatever -- no, actually this should even be disabled if we want to use the all-in-one command with two different item types of same tag!*/
								    strstr(buf3, buf2)) {
									i = (k + 1) * SUBINVEN_INVEN_MUL + j;
									//use this item!
									*item = i;
									return(TRUE);
								}
							}
							/* Proceed to next inventory slot, we don't want to scan this slot any further as it was a subinven,
							   so only its contents were of interest, not the item itself. */
							continue;
						}

						strcpy(buf3, inventory_name[k]);
						ptr = buf3;
						while (*ptr) {
							/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
							   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
							if (*ptr == '@') {
								ptr++;
								if (!*ptr) break;
							} else *ptr = tolower(*ptr);
							ptr++;
						}
						/* Skip fully charging stacks */
						if (strstr(buf3, "(charging)") || strstr(buf3, "(#)") || /* rods (and other devices, in theory) */
						    //(partially charging) || strstr(buf3, "(~)")
						    strstr(buf3, "(0 charges") || strstr(buf3, "{empty}")) /* wands, staves */
							continue;

						if (!(buf3p = strstr(buf3, " of "))) buf3p = buf3; //skip item's article/amount
						if (//inventory[k].tval == inventory[i].tval && /* unnecessary check, but whatever -- no, actually this should even be disabled if we want to use the all-in-one command with two different item types of same tag!*/
						    strstr(buf3, buf2)) {
							i = k;
							break;
						}
					}
					/* While under 'charged' restriction, we didn't find an item in inventory matching our equipped one?
					   Continue scanning the next equipment item then. */
					if (k == INVEN_PACK) continue;
				}
				/* use this item */
				*item = i;
				return(TRUE);
			}

			/* Later on, scan only the normal inven, not the equipment again */
			inven_first = TRUE;
			start = 0;
			stop = INVEN_WIELD;
			step = 1;
		}

		for (l = 0; l < INVEN_PACK; l++) {
			if (!inventory[l].tval) continue;
			/* Assume that subinvens are always at the beginning of the inventory! */
			if (inventory[l].tval != TV_SUBINVEN) break;

			for (j = 0; j < inventory[l].bpval; j++) {
				i = j; /* Identity translation - laziness */

				o_ptr = &subinventory[l][i];
				if (!o_ptr->tval) break;
				if (!item_tester_okay(o_ptr)) continue;
 #if 0
				if (!my_strcasestr(subinventory_name[l][i], buf)) continue;
 #else
				strcpy(buf1, subinventory_name[l][i]);
				strcpy(buf2, buf);
				ptr = buf1;
				while (*ptr) {
					/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
					   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
					if (*ptr == '@') {
						ptr++;
						if (!*ptr) break;
					} else *ptr = tolower(*ptr);
					ptr++;
				}
				ptr = buf2;
				while (*ptr) {
					/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
					   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
					if (*ptr == '@') {
						ptr++;
						if (!*ptr) break;
					} else *ptr = tolower(*ptr);
					ptr++;
				}
				if (!strstr(buf1, buf2)) continue;
 #endif

				if (charged && (
				    strstr(buf1, "(charging)") || strstr(buf1, "(#)") || /* rods (and other devices, in theory) */
				    //(partially charging) || strstr(buf1, "(~)")
				    strstr(buf1, "(0 charges") || strstr(buf1, "{empty}") /* wands, staves */
				    )) {
					/* Especially added for non-stackable rods (Havoc): check for same rod, but not 'charging' */
					char *buf1p, *buf3p;
					int k, ic = -1;

					if (!(buf1p = strstr(buf1, " of "))) buf1p = buf1; //skip item's article/amount

					/* WIELD_DEVICES : Check equip first! */
					//if (stop != INVEN_WIELD) /* no need, as we just already processed equipment? */
					for (k = INVEN_WIELD; k < INVEN_TOTAL; k++) {
						if (!inventory[k].tval) continue;
						if (k == i) continue;

						strcpy(buf3, inventory_name[k]);
						ptr = buf3;
						while (*ptr) {
							/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
							   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
							if (*ptr == '@') {
								ptr++;
								if (!*ptr) break;
							} else *ptr = tolower(*ptr);
							ptr++;
						}
						/* Skip fully charging stacks */
						if (strstr(buf3, "(charging)") || strstr(buf3, "(#)") || /* rods (and other devices, in theory) */
						    //(partially charging) || strstr(buf3, "(~)")
						    strstr(buf3, "(0 charges") || strstr(buf3, "{empty}")) /* wands, staves */
							continue;

						if (!(buf3p = strstr(buf3, " of "))) buf3p = buf3; //skip item's article/amount
						if (//inventory[k].tval == subinventory[l][i].tval && /* unnecessary check, but whatever -- no, actually this should even be disabled if we want to use the all-in-one command with two different item types of same tag!*/
						    strstr(buf3, buf2)) {
							ic = k;
							break;
						}
					}

					/* Check the current subinventory */
					if (ic == -1)
					for (k = 0; k < inventory[l].bpval; k++) {
						if (!subinventory[l][k].tval) break;
						if (k == i) continue;

						strcpy(buf3, subinventory_name[l][k]);
						ptr = buf3;
						while (*ptr) {
							/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
							   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
							if (*ptr == '@') {
								ptr++;
								if (!*ptr) break;
							} else *ptr = tolower(*ptr);
							ptr++;
						}
						/* Skip fully charging stacks */
						if (strstr(buf3, "(charging)") || strstr(buf3, "(#)") || /* rods (and other devices, in theory) */
						    //(partially charging) || strstr(buf3, "(~)")
						    strstr(buf3, "(0 charges") || strstr(buf3, "{empty}")) /* wands, staves */
							continue;

						if (!(buf3p = strstr(buf3, " of "))) buf3p = buf3; //skip item's article/amount
						if (//subinventory[l][k].tval == subinventory[l][i].tval && /* unnecessary check, but whatever -- no, actually this should even be disabled if we want to use the all-in-one command with two different item types of same tag!*/
						    strstr(buf3, buf2)) {
							ic = k;
							break;
						}
					}

					/* Check normal inventory too */
					if (ic == -1)
					for (k = 0; k < INVEN_PACK; k++) {
						if (!subinventory[l][k].tval) break;
						if (k == i) continue;

						strcpy(buf3, subinventory_name[l][k]);
						ptr = buf3;
						while (*ptr) {
							/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
							   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
							if (*ptr == '@') {
								ptr++;
								if (!*ptr) break;
							} else *ptr = tolower(*ptr);
							ptr++;
						}
						/* Skip fully charging stacks */
						if (strstr(buf3, "(charging)") || strstr(buf3, "(#)") || /* rods (and other devices, in theory) */
						    //(partially charging) || strstr(buf3, "(~)")
						    strstr(buf3, "(0 charges") || strstr(buf3, "{empty}")) /* wands, staves */
							continue;

						if (!(buf3p = strstr(buf3, " of "))) buf3p = buf3; //skip item's article/amount
						if (//subinventory[l][k].tval == subinventory[l][i].tval && /* unnecessary check, but whatever -- no, actually this should even be disabled if we want to use the all-in-one command with two different item types of same tag!*/
						    strstr(buf3, buf2)) {
							ic = k;
							break;
						}
					}

					if (ic == -1) continue;
				}
				*item = i + (l + 1) * SUBINVEN_INVEN_MUL;
				return(TRUE);
			}
		}
		/* Fall through and scan inventory normally, after we didn't find anything in subinvens. */
	}
#endif
//c_msg_format("start %d, stop %d, step %d", start, stop, step);
	for (j = start; j != stop; j += step) {
		/* translate back so order within each - inven & equip - is alphabetically again */
		if (inven_first)
			i = j;
		else /* Translate, so equip and inven are processed in normal order _each_ */
			i = INVEN_WIELD + (j > INVEN_WIELD ? INVEN_TOTAL : 0) - j;

		o_ptr = &inventory[i];
		if (!o_ptr->tval) continue;
		if (!item_tester_okay(o_ptr)) continue;

#ifdef SMART_SWAP
		if (i_found != -1) {
			/* Skip same-type inventory items to become our alternative replacement item for swapping.
			   TODO: Apply method 1/2/3 here, instead of just 1. */
			if (tval == TV_RING && //(only for books (as we might stack multiple of the same type),) rings (as only these and weapons have two equip slots - and for weapons we might want this (eg 2x dagger vs 1x 2hander))!
			    tval == inventory[i].tval && sval == inventory[i].sval)
				continue;
		}
#endif

#if 0
		if (!my_strcasestr(inventory_name[i], buf)) continue;
#else
		strcpy(buf1, inventory_name[i]);
		strcpy(buf2, buf);
		ptr = buf1;
		while (*ptr) {
			/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
			   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
			if (*ptr == '@') {
				ptr++;
				if (!*ptr) break;
			} else *ptr = tolower(*ptr);
			ptr++;
		}
		ptr = buf2;
		while (*ptr) {
			/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
			   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
			if (*ptr == '@') {
				ptr++;
				if (!*ptr) break;
			} else *ptr = tolower(*ptr);
			ptr++;
		}
//c_msg_format("comparing '%s','%s'\n", buf1, buf2);
		if (!strstr(buf1, buf2)) continue;
#endif

#ifdef SMART_SWAP /* not really cool, with the ' of ' hack.. problem was eg 'ring' vs 'rings' */
		if (chk_multi && i < INVEN_PACK) {
			/* Check for same item in the equipment, if found, search inventory for a non-same alternative */
			char *buf1p, *buf3p;
			int k;

			tval = inventory[i].tval;
			sval = inventory[i].sval;
			pval = inventory[i].pval; //basically only for spellbooks

			/* Compare tval and sval directly, assuming they are available - true for non-flavour items */
			if (sval != 255 || tval == TV_BOOK) {
				buf1p = buf1;
				method = 1; /* compare tval+sval (non-flavour items, or known flavour items (hahaa!)) */
			}
			/* Compare item names, assuming flavoured item*/
			else {
				buf1p = strstr(buf1, " of ");
				if (!buf1p) method = 3; /* aka auto-accept anything -_- */
				else method = 2; /* compare rest of the item name (unknown flavoured items) */
			}

			for (k = INVEN_WIELD; k < INVEN_TOTAL; k++) {
				if (!inventory[k].tval) continue;

				strcpy(buf3, inventory_name[k]);
				ptr = buf3;
				while (*ptr) {
					/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
					   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
					if (*ptr == '@') {
						ptr++;
					if (!*ptr) break;
					} else *ptr = tolower(*ptr);
					ptr++;
				}
 #if 0 /* old, unclean (and 'buggy' for items of same tval+sval but with different pval, yet we certainly don't want to treat them as 'different') */
				/* Compare tval and sval directly, assuming they are available - true for non-flavour items */
				if (invenory[k].sval != 255 || inventory[k]tval == TV_BOOK)
					buf3p = buf3;
				/* Compare item names, assuming flavoured item*/
				else {
					buf3p = strstr(buf1, " of ");
					if (!buf3p) buf3p = buf3;
				}
				/* Actually we should only test for equipment slots that fulfill wield_slot() condition
				   for the inventory item, but since we don't have this function client-side we just test all.. */
				if (!strstr(buf3, buf2) && !strcmp(buf1p, buf3p)) continue;
 #else /* quick, rough fix, copy-pasted from new get_tag_aux(). Todo: clean this mess up in a similar fashion to get_tag()+get_tag_aux() */
				switch (method) {
				case 1:
					/* Compare tval and sval directly, assuming they are available - true for non-flavour items.
					   Almost no need to check bpval or pval, because switching items in such a manner doesn't make much sense,
					   not even for polymorph rings, EXCEPT for spell books! */
					if (inventory[k].tval != tval || inventory[k].sval != sval || (inventory[k].tval == TV_BOOK && inventory[k].sval == SV_SPELLBOOK && inventory[k].pval != pval)
					    //only for books (as we might stack multiple of the same type), rings (as only these and weapons have two equip slots - and for weapons we might want this (eg 2x dagger vs 1x 2hander))!
					    || (inventory[k].tval != TV_RING && inventory[k].tval != TV_BOOK))
						continue;
					break;
				case 2:
					/* Compare item names, assuming flavoured item*/
					buf3p = strstr(buf3, " of ");

					/* paranoia or very long item name that got shortened, omitting the 'of', for some reason by object_desc() perhaps -
					   anyway, we accept the item for now, might need changing -_- */
					if (!buf3p) break;

					/* If name is equal -> 'Success' - same item exists in equipment as our original inventory item :/.
					    So, postpone our original item for now and continue searching the equipment for a different one instead. */
					if (strcmp(buf1p, buf3p)) continue;
					break;
				case 3: /* Badly processable item name - always accept -_- */
					break;
				}

				/* Final check: It must have a matching tag though, of course */
				if (!strstr(buf3, buf2)) continue;
 #endif
				/* remember this item to use it if we don't find a different one.. */
				i_found = i;

				/* Only search the inventory this time though -- assume non inven_first aka reversed traversal direction >_> */
				if (stop > INVEN_PACK) stop = INVEN_PACK;
				if (j >= stop) j = stop - 1;
				/* Do not AGAIN replace the alternative item we might find */
				mode &= ~CHECK_MULTI;

				break;
			}
			if (k < INVEN_TOTAL && i_found != -1) continue;
		}
#endif
//c_msg_format("charged = %d, charged_ready = %d", charged, charged_ready);
//c_msg_format("<%s> charged = %d", buf1, charged);
		if (charged && (
		    strstr(buf1, "(charging)") || strstr(buf1, "(#)") || /* rods (and other devices, in theory) */
		    //(partially charging) || strstr(buf1, "(~)")
		    strstr(buf1, "(0 charges") || strstr(buf1, "{empty}") /* wands, staves */
		    )) {
			/* Especially added for non-stackable rods (Havoc): check for same rod, but not 'charging' */
			char *buf1p, *buf3p;
			int k;

			if (!(buf1p = strstr(buf1, " of "))) buf1p = buf1; //skip item's article/amount
			for (k = 0; k < INVEN_PACK; k++) {
				if (!inventory[k].tval) continue;
				if (k == i) continue;

				strcpy(buf3, inventory_name[k]);
				ptr = buf3;
				while (*ptr) {
					/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
					   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
					if (*ptr == '@') {
						ptr++;
						if (!*ptr) break;
					} else *ptr = tolower(*ptr);
					ptr++;
				}
				/* Skip fully charging stacks */
				if (strstr(buf3, "(charging)") || strstr(buf3, "(#)") || /* rods (and other devices, in theory) */
				    //(partially charging) || strstr(buf3, "(~)")
				    strstr(buf3, "(0 charges") || strstr(buf3, "{empty}")) /* wands, staves */
					continue;

				if (!(buf3p = strstr(buf3, " of "))) buf3p = buf3; //skip item's article/amount
				if (//inventory[k].tval == inventory[i].tval && /* unnecessary check, but whatever -- no, actually this should even be disabled if we want to use the all-in-one command with two different item types of same tag!*/
				    strstr(buf3, buf2)) {
					i = k;
					break;
				}
			}
		}
		*item = i;
		return(TRUE);
	}

#ifdef SMART_SWAP
	if (i_found != -1) {
		*item = i_found;
		return(TRUE);
	}
#endif
	return(FALSE);
}

bool (*get_item_extra_hook)(int *cp, int mode);
bool c_get_item(int *cp, cptr pmt, int mode) {
	//char n1, n2;
	char which = ' ';

	int k, i1, i2, e1, e2, ver;
	bool done, spammy = FALSE, alt;
	byte item;

	char tmp_val[160];
	char out_val[160];

	bool equip = FALSE;
	bool inven = FALSE;
	//bool floor = FALSE;
	bool extra = FALSE, limit = FALSE;
	bool special_req = FALSE;
	s16b newest = -1;
	bool equip_first = FALSE;
#ifdef ENABLE_SUBINVEN
	bool found_subinven = FALSE;
	bool subinven = FALSE; /* are items inside subinventory/subinventories (possible) subject to choose from? */
	bool use_subinven = FALSE; /* can we select subinven items to enter? */
	int sub_i = (using_subinven + 1) * SUBINVEN_INVEN_MUL, autoswitch_subinven = -1;
#endif
	bool safe_input = FALSE;


	/* The top line is icky */
	topline_icky = TRUE;

	/* Not done */
	done = FALSE;

	/* No item selected */
	item = FALSE;

	/* Default to "no item" */
	*cp = -1;

	/* Clear previous flag */
	verified_item = FALSE;

	if (mode & USE_EQUIP) equip = TRUE;
	if (mode & USE_INVEN) inven = TRUE;
	//if (mode & (USE_FLOOR) floor = TRUE;
	if (mode & USE_EXTRA) extra = TRUE;
	if (mode & SPECIAL_REQ) special_req = TRUE;

	//if (mode & NEWEST)
	newest = item_newest; /* experimental: always on if available */
	/* If we're within subinven operation, translate newest item to local item if it's in our subinven actually (for get_item_okay()) */
	if (using_subinven != -1) {
		if ((newest / SUBINVEN_INVEN_MUL) - 1 == using_subinven) newest %= SUBINVEN_INVEN_MUL;
		else newest = -1;
	}
	if (!get_item_okay(newest)) {
		newest = item_newest_2nd;
		/* If we're within subinven operation, translate newest item to local item if it's in our subinven actually (for get_item_okay()) */
		if (using_subinven != -1) {
			if ((newest / SUBINVEN_INVEN_MUL) - 1 == using_subinven) newest %= SUBINVEN_INVEN_MUL;
			else newest = -1;
		}
		if (!get_item_okay(newest)) newest = -1;
	}
	if (c_cfg.show_newest) redraw_newest();

	if (mode & USE_LIMIT) limit = TRUE;
	if (mode & EQUIP_FIRST) equip_first = TRUE;
#ifdef ENABLE_SUBINVEN
	/* Hack: Always enable subinven when we also have inven enabled, for now.
	   The idea is future client compatibility in case more custom bags are added, so items are already compatible. */
	if (inven && !(mode & EXCLUDE_SUBINVEN)) mode |= USE_SUBINVEN;

	if (mode & USE_SUBINVEN) {
		subinven = TRUE;
		use_subinven = TRUE;

		/* Hack: Since we cannot list normal inven + subinven at the same time, we NEED 'extra' access via item name.
		   If USE_EXTRA wasn't set though we must be careful to set get_item_extra_hook so it's not a null pointer! */
		if (!extra) {
			item_tester_hook = NULL;
			get_item_hook_find_obj_what = "Item name? ";
			get_item_extra_hook = get_item_hook_find_obj;
			extra = TRUE;
		}
	}
#endif

#ifdef ENABLE_SUBINVEN
	if (using_subinven != -1) {
		inven = equip = FALSE; /* If we are inside a specific subinventory already, disallow normal inventory */
		subinven = TRUE; /* ..and definitely allow subinven, of course. */
		use_subinven = FALSE; /* There are no choosable bags inside actual bags! */
	}
#endif

	/* Paranoia */
#ifdef ENABLE_SUBINVEN
	if (!subinven)
#endif
	if (!inven && !equip) {
		/* Forget the item_tester_tval restriction */
		item_tester_tval = 0;
		/* Forget the item_tester_max_weight restriction */
		item_tester_max_weight = 0;
		/* Forget the item_tester_hook restriction */
		item_tester_hook = 0;

		/* Stop macro execution if we're on safe_macros! */
		if (parse_macro && c_cfg.safe_macros) flush_now();

		return(FALSE);
	}

	/* Check for special '%' tag, based on current command_cmd:
	   Use first item found with new "@<commandkey>%" inscription right away without prompting for item choice */
	if (get_tag(&k, '%', inven | subinven, equip, mode)) {
		(*cp) = k;

		screen_line_icky = -1;
		screen_column_icky = -1;

		/* Fix the top line */
		topline_icky = FALSE;

		/* Flush any events */
		Flush_queue();

		/* Forget the item_tester_tval restriction */
		item_tester_tval = 0;
		/* Forget the item_tester_max_weight restriction */
		item_tester_max_weight = 0;
		/* Forget the item_tester_hook restriction */
		item_tester_hook = 0;

		/* Redraw inventory */
		p_ptr->window |= PW_INVEN | PW_EQUIP;
		window_stuff();

		/* Clear the prompt line */
		if (!item) clear_topline_forced(); //special case: ESCaped instead of specifying an item
		else clear_topline();

		/* Cease command macro exception */
		//inkey_get_item = FALSE;

		/* restore macro handling hack to default state */
		abort_prompt = FALSE;

		return(TRUE);
	}

	/* Command macros work as an exception here */
	//inkey_get_item = TRUE;

	/* Full inventory */
	i1 = 0;
#ifdef ENABLE_SUBINVEN
	if (using_subinven != -1) {
		i2 = using_subinven_size - 1;

		/* Restrict subinventory indices (for unstow this basically reduces max capacity to actually used capacity) */
		while ((i1 <= i2) && (!get_item_okay(i1))) i1++;
		while ((i1 <= i2) && (!get_item_okay(i2))) i2--;
	} else
	    /* Don't restrict items inside subinventories, as we don't have a function for this yet
	       (would need to pass subinven flag+index to get_item_okay() or something).
	       Just leave it to server-side checks for now. */
	{
#endif
		i2 = INVEN_PACK - 1;
		/* Forbid inventory */
		if (!inven) i2 = -1;

		/* Restrict inventory indices */
		while ((i1 <= i2) && (!get_item_okay(i1))) i1++;
		while ((i1 <= i2) && (!get_item_okay(i2))) i2--;
#ifdef ENABLE_SUBINVEN
	}
#endif

#ifdef ENABLE_SUBINVEN
	if (subinven) {
		int j;

		/* Also scan all subinventories for at least one valid item */
		for (k = 0; k < INVEN_PACK; k++) {
			if (!inventory[k].tval) continue;
			if (inventory[k].tval != TV_SUBINVEN) continue;
			/* Check all _specialized_ container types. Chests are not eligible. */
			if (inventory[k].sval >= SV_SI_CHEST_SMALL_WOODEN && inventory[k].sval <= SV_SI_CHEST_LARGE_STEEL) continue;

			for (j = 0; j < inventory[k].bpval; j++) {
				if (!subinventory[k][j].tval) break;
				if (!get_item_okay((k + 1) * SUBINVEN_INVEN_MUL + j)) continue;

				found_subinven = TRUE;
				autoswitch_subinven = k;
				break;
			}
		}
	}
#endif

	/* Full equipment */
	e1 = INVEN_WIELD;
	e2 = INVEN_TOTAL - 1;

	/* Restrict equipment indices */
	while ((e1 <= e2) && (!get_item_okay(e1))) e1++;
	while ((e1 <= e2) && (!get_item_okay(e2))) e2--;

	/* Handle the option */
	/* XXX remove it when you need to control it from outside of
	 * this function	- Jir - */
	command_see = c_cfg.always_show_lists;
	command_wrk = FALSE;

	if ((i1 > i2) && (e1 > e2)
#ifdef ENABLE_SUBINVEN
	    /* Nothing to display, but at least allow selecting a subinven item via @name */
	    && !found_subinven
#endif
	    ) {
		/* Cancel command_see */
		command_see = FALSE;

		/* Hack -- Nothing to choose */
		*cp = -2;
		/* more hack: Tell macro that it should skip any item-selection code
		   that might follow because there are no eligible items available.
		   Otherwise the macro might 'run wild' by causing unintended key
		   presses instead of picking the item. */
		if (parse_macro) macro_missing_item = extra ? 1 : 3;

		/* Actually output a warning to combat message window */
		topline_icky = FALSE;
		if (!(mode & NO_FAIL_MSG)) c_msg_print("You do not have an eligible item.");

		/* Stop macro execution if we're on safe_macros! */
		if (parse_macro && c_cfg.safe_macros) flush_now();
		/* Flush any events */
		Flush_queue(); //I thought this also cleared macro execution, making flush_now() superfluous, but sometimes (delay-dependant?) it didn't

		/* Hack -- Cancel "display" */
		command_see = FALSE;

		/* Forget the item_tester_tval restriction */
		item_tester_tval = 0;
		/* Forget the item_tester_max_weight restriction */
		item_tester_max_weight = 0;
		/* Forget the item_tester_hook restriction */
		item_tester_hook = 0;

		/* Redraw inventory */
		p_ptr->window |= PW_INVEN;
		window_stuff();

		return(item);
	}

	/* Analyze choices */
	else {
		/* Hack -- reset display width */
		if (!command_see) command_gap = 50;

		/* Hack -- Start on equipment if requested */
		if (command_see && command_wrk && equip) command_wrk = TRUE;
		/* Use inventory if allowed */
		else if (inven) command_wrk = FALSE;
		/* Use equipment if allowed */
		else if (equip) command_wrk = TRUE;
		/* Use subinventory if allowed */
		else if (subinven) command_wrk = FALSE;
	}

	/* Start with equipment? ('A'ctivate command) */
	if (equip_first) command_wrk = TRUE;

	/* Automatically switch to equipment or to bags (or to inventory/bags if we started out in equipment)
	   if no eligible item was found in our starter inventory screen */
	if (c_cfg.autoswitch_inven && using_subinven == -1) {
		if (command_wrk) { /* we start out in equipment */
			if (e1 > e2) { /* no eligible item in the equipment? */
				if (i1 > i2) { /* no eligible item in the inventory either? */
					if (found_subinven) { //found_subinven must be TRUE if i1>i2, or we should've returned further above already
						/* start out with subinventory */
						command_wrk = FALSE; /* don't start in the equipment anymore */
						//c_msg_format("SWITCHED->subinv(%d)", using_subinven);
					}
				} else {
					/* start out with inventory */
					command_wrk = FALSE; /* don't start in the equipment anymore */
					//c_msg_print("SWITCHED->inv");
					autoswitch_subinven = -1; /* don't start in a bag */
				}
			} else autoswitch_subinven = -1; /* don't start in a bag */
		} else { /* we start out in inventory (standard) */
			if (i1 > i2) { /* no eligible item in the inventory? */
				if (e1 > e2) { /* no eligible item in the equipment either? */
					if (found_subinven) { //found_subinven must be TRUE if i1>i2, or we should've returned further above already
						/* start out with subinventory */
						//c_msg_format("SWITCHED->subinv(%d)", using_subinven);
					}
				} else {
					/* start out in the equipment screen */
					command_wrk = TRUE;
					//c_msg_print("SWITCHED->eq");
					autoswitch_subinven = -1; /* don't start in a bag */
				}
			} else autoswitch_subinven = -1; /* don't start in a bag */
		}
	} else autoswitch_subinven = -1; /* don't start in a bag */

	/* Redraw inventory */
	p_ptr->window |= PW_INVEN;
	window_stuff();

	/* Hack -- start out in "display" mode */
	if (command_see) Term_save();

	/* If we can't find a specified item, there are two ways to proceed:
	   1) repeat item-request, this will result in any macro being continued
	    after an input to the pending, unfulfilled (item-)request has been made
	    manually. However, this scenario is more unlikely to be desired.
	   2) Clear the prompt and cancel the macro, so the remaining macro part is
	    discarded. This is usually wanted, because the usual scenario for this is
	    having run out of an important item (eg healing potions) and getting
	    'stuck' in the item-request prompt.
	    So a hack will be added to this hack: safe_macros should clear the prompt
	    and abort the macro. - C. Blue */
	/* safe_macros avoids getting stuck in an unfulfilled (item-)prompt */
	if (parse_macro && c_cfg.safe_macros) safe_input = TRUE;

	/* Too long description - shorten? */
	if (special_req + (newest != -1)
#if defined(ENABLE_SUBINVEN) && defined(ITEM_PROMPT_ALLOWS_SWITCHING_TO_SUBINVEN)
	    + found_subinven
#endif
	    + inven + equip
	    /* Some prompt texts are too long if we have +/- functionality enabled at the same time */
	    + (strlen(pmt) > 15)
	    >= 4)
		spammy = TRUE;

	/* Repeat while done */
	while (!done) {
		alt = FALSE;

		/* hack - cancel prompt if we're in a failed macro execution */
		if (safe_input && abort_prompt) {
			command_gap = 50;
			break;
		}

		if (!command_wrk) {
			/* Extract the legal requests */
			//n1 = I2A(i1);
			//n2 = I2A(i2);

			/* Redraw if needed */
#ifdef ENABLE_SUBINVEN
			if (command_see && using_subinven != -1) show_subinven(using_subinven);
			else
#endif
			if (command_see) show_inven();
		}

		/* Equipment screen */
		else {
			/* Extract the legal requests */
			//n1 = I2A(e1 - INVEN_WIELD);
			//n2 = I2A(e2 - INVEN_WIELD);

			/* Redraw if needed */
			if (command_see) show_equip();
		}

		/* Viewing inventory */
		if (!command_wrk) {
			/* Begin the prompt */
			//if (spammy) sprintf(out_val, "Inv"); else  --not (yet?) needed, instead we just cut the 'ESC' part for now
			sprintf(out_val, "Inven:");

			/* Some legal items */
			if (i1 <= i2) {
				/* Build the prompt */
				sprintf(tmp_val, " %c-%c,",
					index_to_label(i1), index_to_label(i2));

				/* Append */
				strcat(out_val, tmp_val);
			}

			/* Indicate ability to "view" */
			if (!command_see) {
				if (spammy) strcat(out_val, " * see,");
				else strcat(out_val, " * to see,");
			}

			/* Append */
			if (equip) {
				if (spammy) strcat(out_val, " / Equip,");
				else strcat(out_val, " / for Equip,");
			}
		}
		/* Viewing equipment */
		else {
			/* Begin the prompt */
			sprintf(out_val, "Equip:");

			/* Some legal items */
			if (e1 <= e2) {
				/* Build the prompt */
				sprintf(tmp_val, " %c-%c",
					index_to_label(e1), index_to_label(e2));

				/* Append */
				strcat(out_val, tmp_val);
			}

			/* Indicate the ability to "view" */
			if (!command_see) strcat(out_val, " * to see,");

			/* Append */
			if (inven) {
				if (spammy) strcat(out_val, " / Inven,");
				else strcat(out_val, " / for Inven,");
			}
		}

		/* Extra? */
		if (extra) {
			if (spammy) strcat(out_val, " @ name,");
			else strcat(out_val, " @ to name,");
		}

		/* Limit? (for SFLG1_LIMIT_SPELLS -- default: disabled) */
		if (limit) {
			if (spammy) strcat(out_val, " # limit,");
			else strcat(out_val, " # to limit,");
		}

#ifdef ITEM_PROMPT_ALLOWS_SWITCHING_TO_SUBINVEN
		if (use_subinven) {
			/* No need to show '! for bag' prompt if we don't have any eligible items inside any bags */
			if (found_subinven) {
				if (spammy) strcat(out_val, " ! bag,");
				else strcat(out_val, " ! for bag,");
			}
		}
#endif

		/* Special request toggle? */
		if (special_req) {
			if (spammy) strcat(out_val, " - switch,");
			else strcat(out_val, " - to switch,");
		}
		/* Re-use 'newest' item? */
		if (newest != -1) {
			if (spammy) strcat(out_val, " + newest,");
			else strcat(out_val, " + for newest,");
		}

		/* Finish the prompt */
		//if (spammy)
		if (strlen(out_val) + strlen(pmt) >= MAX_CHARS - 7)
			out_val[strlen(out_val) - 1] = 0; //erase the comma
		else strcat(out_val, " ESC");

		/* Build the prompt */
		sprintf(tmp_val, "(%s) %s", out_val, pmt);

		/* Show the prompt */
		prompt_topline(tmp_val);


#ifdef ENABLE_SUBINVEN
		if (autoswitch_subinven != -1) which = '!';
		else
#endif
		/* Get a key */
		which = inkey();

		/* Parse it */
		switch (which) {
		case ESCAPE:
			command_gap = 50;
			done = TRUE;
			break;

		case KTRL('T'):
			/* Take a screenshot */
			xhtml_screenshot("screenshot????", FALSE);
			break;

#ifdef ITEM_PROMPT_ALLOWS_SWITCHING_TO_SUBINVEN
		case '!':
			if (use_subinven) {
				int i;
				int old_tval = item_tester_tval;
				cptr old_obj_what = get_item_hook_find_obj_what;
				bool (*old_extra_hook)(int *cp, int mode) = get_item_extra_hook;
				int old_max_weight = item_tester_max_weight;
				bool (*old_tester_hook)(object_type *o_ptr) = item_tester_hook;

				item_tester_tval = TV_SUBINVEN;
				get_item_hook_find_obj_what = "Bag name? ";
				get_item_extra_hook = get_item_hook_find_obj;
				item_tester_max_weight = 0;
				item_tester_hook = 0;

				/* Fix the screen if necessary */
				if (command_see) Term_load();

				if (autoswitch_subinven != -1) {
					i = autoswitch_subinven;
					autoswitch_subinven = -1;
				} else
				if (!c_get_item(&i, "Use which bag? ", (USE_INVEN | EXCLUDE_SUBINVEN | NO_FAIL_MSG))) {
					if (i == -2) c_msg_print("You have no bags.");
					if (parse_macro && c_cfg.safe_macros) flush_now();//Term_flush();

					/* Return to our original inventory-browsing prompt */
					item_tester_tval = old_tval;
					get_item_hook_find_obj_what = old_obj_what;
					get_item_extra_hook = old_extra_hook;
					item_tester_max_weight = old_max_weight;
					item_tester_hook = old_tester_hook;

					break;
				}

				/* Select this bag (subinventory) */
				using_subinven = i;

				/* Continue looking for our original item type(s) in this newly selected subinventory */
				item_tester_tval = old_tval;
				get_item_hook_find_obj_what = old_obj_what;
				get_item_extra_hook = old_extra_hook;
				item_tester_max_weight = old_max_weight;
				item_tester_hook = old_tester_hook;

				/* Got valid item selected from subinventory */
				if (c_get_item(&i, pmt, (mode | EXCLUDE_SUBINVEN) & ~NO_FAIL_MSG)) {
					*cp = i;
					item = TRUE;
					done = TRUE;
				}

				/* Leave this bag and continue looking for our original item type(s) in our normal inven/equip again?
				   (The above c_get_item() will have NULL'ed item_tester_hook at least, so we have to restore it again.) */
				if (!done) {
					item_tester_tval = old_tval;
					get_item_hook_find_obj_what = old_obj_what;
					get_item_extra_hook = old_extra_hook;
					item_tester_max_weight = old_max_weight;
					item_tester_hook = old_tester_hook;
				}

				/* Leave subinventory again and return to our main inventory */
				using_subinven = -1;

				/* Fix the screen if necessary */
				//if (command_see) Term_save();
				break;
			} else if (c_cfg.item_error_beep) bell();
			else bell_silent();
			break;
#endif
		case '*':
		case '?':
		case ' ':
			/* Show/hide the list */
			if (!command_see) {
				Term_save();
				command_see = TRUE;
			} else {
				Term_load();
				command_see = FALSE;

				/* Flush any events */
				Flush_queue();
			}
			break;

		case '/':
			/* Verify legality */
			if (!inven || !equip) {
				if (c_cfg.item_error_beep) bell();
				else bell_silent();
				break;
			}

			/* Fix screen */
			if (command_see) {
				Term_load();
				Flush_queue();
				Term_save();
			}

			/* Switch inven/equip */
			command_wrk = !command_wrk;

			/* Need to redraw */
			break;

		case '0':
		case '1': case '2': case '3':
		case '4': case '5': case '6':
		case '7': case '8': case '9':
			/* XXX XXX Look up that tag */
			if (!get_tag(&k, which, inven | subinven, equip, mode)) {
				if (c_cfg.item_error_beep) bell();
				else bell_silent();
				break;
			}
#ifdef ENABLE_SUBINVEN
			if (using_subinven != -1) {
				/* get_tag() is using_subinven agnostic, so we have to convert k back to a direct subinven index */
				if (k >= SUBINVEN_INVEN_MUL) k = k % SUBINVEN_INVEN_MUL;

				if (k < using_subinven_size && !subinven) {
					if (c_cfg.item_error_beep) bell();
					else bell_silent();
					break;
				}
			} else if (k >= SUBINVEN_INVEN_MUL) {
				/* Hack -- verify item (in subinventory) */
				if (!inven && !subinven) {
					if (c_cfg.item_error_beep) bell();
					else bell_silent();
					break;
				}
			} else
#endif
			/* Hack -- verify item */
			if ((k < INVEN_WIELD) ? !inven : !equip) {
				if (c_cfg.item_error_beep) bell();
				else bell_silent();
				break;
			}

			/* Validate the item */
			if (!get_item_okay(k)) {
				if (c_cfg.item_error_beep) bell();
				else bell_silent();
				break;
			}

#if 0
			if (!get_item_allow(k)) {
				done = TRUE;
				break;
			}
#endif

			/* Use that item */
			(*cp) = k;
#ifdef ENABLE_SUBINVEN
			if (using_subinven != -1) (*cp) += sub_i;
#endif
			item = TRUE;
			done = TRUE;
			break;

		case '\n':
		case '\r':
			/* Choose "default" inventory item */
			if (!command_wrk) k = ((i1 == i2) ? i1 : -1);
			/* Choose "default" equipment item */
			else k = ((e1 == e2) ? e1 : -1);

			/* Validate the item */
			if (!get_item_okay(k)) {
				if (c_cfg.item_error_beep) bell();
				else bell_silent();
				break;
			}

#if 0
			/* Allow player to "refuse" certain actions */
			if (!get_item_allow(k)) {
				done = TRUE;
				break;
			}
#endif

			/* Accept that choice */
			(*cp) = k;
#ifdef ENABLE_SUBINVEN
			if (using_subinven != -1) (*cp) += sub_i;
#endif
			item = TRUE;
			done = TRUE;
			break;

		case '@':
		{
			int i;

			if (extra && get_item_extra_hook(&i, mode)) {
				(*cp) = i;
#ifdef ENABLE_SUBINVEN
				if (using_subinven != -1) (*cp) += sub_i;
#endif
				item = TRUE;
				done = TRUE;
			} else if (c_cfg.item_error_beep) bell();
			else bell_silent();
			break;
		}

		case '#':
			if (!limit) {
				if (c_cfg.item_error_beep) bell();
				else bell_silent();
				break;
			}
			hack_force_spell_level = c_get_quantity("Limit spell level (0 for unlimited)? ", 0, -1);
			if (hack_force_spell_level < 0) hack_force_spell_level = 0;
			if (hack_force_spell_level > 99) hack_force_spell_level = 99;
			limit = FALSE; //just for visuals: don't offer to re-enter level limit over and over since it's pointless
			break;

		case '-':
			if (!special_req) {
				if (c_cfg.item_error_beep) bell();
				else bell_silent();
				break;
			}
			command_gap = 50;
			done = TRUE;
			item = FALSE;
			*cp = -3;
			break;

		case '+':
			if (newest == -1) {
				if (c_cfg.item_error_beep) bell();
				else bell_silent();
				break;
			}
			/* Validate the item */
			if (!get_item_okay(newest)) { //redundant, we checked this at the start
				if (c_cfg.item_error_beep) bell();
				else bell_silent();
				break;
			}

			*cp = newest;
#ifdef ENABLE_SUBINVEN
			if (using_subinven != -1) (*cp) += sub_i;
#endif
			item = TRUE;
			done = TRUE;
			break;

		default:
			/* Extract "query" setting */
			ver = isupper(which);
			if (ver) which = tolower(which);

			/* Instead of query, switch to alternate function? */
			if (ver && (mode & CAPS_ALT)) {
				ver = 0;
				alt = TRUE;
			} else alt = FALSE;

			/* Convert letter to inventory index */
			if (!command_wrk) k = c_label_to_inven(which);
			/* Convert letter to equipment index */
			else k = c_label_to_equip(which);

			/* Validate the item */
			if (!get_item_okay(k)) {
				if (c_cfg.item_error_beep) bell(); //not necessarily an item selection error though, as we also just catch ANY invalid key here in 'default'
				else bell_silent();
				break;
			}

			/* Verify, abort if requested */
			if (ver && !verify("Try", k)) {
				done = TRUE;
				break;
			}

#if 0
			/* Allow player to "refuse" certain actions */
			if (!get_item_allow(k)) {
				done = TRUE;
				break;
			}
#endif

			/* Accept that choice */
			(*cp) = k;
#ifdef ENABLE_SUBINVEN
			if (using_subinven != -1) (*cp) += sub_i;
#endif
			item = TRUE;
			done = TRUE;

			/* Remember that we hit SHIFT+slot to override whole_ammo_stack */
			if (ver) verified_item = TRUE;
			break;
		}
	}

	screen_line_icky = -1;
	screen_column_icky = -1;

	/* Fix the screen if necessary */
	if (command_see) Term_load();

	/* Fix the top line */
	topline_icky = FALSE;

	/* Flush any events */
	Flush_queue();

	/* Hack -- Cancel "display" */
	command_see = FALSE;


	/* Forget the item_tester_tval restriction */
	item_tester_tval = 0;
	/* Forget the item_tester_max_weight restriction */
	item_tester_max_weight = 0;
	/* Forget the item_tester_hook restriction */
	item_tester_hook = 0;

	/* Redraw inventory */
	p_ptr->window |= PW_INVEN | PW_EQUIP;
	window_stuff();


	/* Clear the prompt line */
	if (!item) clear_topline_forced(); //special case: ESCaped instead of specifying an item
	else clear_topline();


	/* Cease command macro exception */
//	inkey_get_item = FALSE;

	/* restore macro handling hack to default state */
	abort_prompt = FALSE;

	/* Return TRUE if something was picked */
#ifdef ENABLE_SUBINVEN
	//if (using_subinven != -1) cp += sub_i;
#endif

	if (alt) *cp = -2 - *cp;
	return(item);
}

void redraw_newest(void) {
	p_ptr->window |= PW_INVEN | PW_EQUIP;
#ifdef ENABLE_SUBINVEN
	p_ptr->window |= PW_SUBINVEN;
#endif
	window_stuff();
}
