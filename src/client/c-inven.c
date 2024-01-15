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
	} else if (i >= 100) {
		int s = i / 100 - 1;

		i = i % 100;

		/* Illegal items */
		if ((i < 0) || (i >= inventory[s].pval)) return(FALSE);

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

	if (i >= 100) {
		si = i / 100 - 1;
		i = i % 100;

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
			/* unnecessary check, but whatever */
			if (k != -1 && tval != inventory[k].tval) k = -1;

			/* Now check inven/subinven */
			if (k == -1) get_tag(&k, tag, TRUE, FALSE, mode_ready);
			/* unnecessary check, but whatever */
			if (k != -1 && tval != (k >= 100 ? subinventory[k / 100 - 1][k % 100].tval : inventory[k].tval)) k = -1;

			/* Found a ready-to-use replacement magic device for our still-charging/empty one! */
			if (k != -1) {
#ifdef ENABLE_SUBINVEN
				if (k >= 100) {
					si = k / 100 - 1;
					i = k % 100;
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
			if (si != -1) *cp = (si + 1) * 100 + i;
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
			if (si != -1) *cp = (si + 1) * 100 + i;
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
				if (inventory[m].tval != tval || inventory[m].sval != sval || (inventory[m].tval == TV_BOOK && inventory[m].sval == SV_SPELLBOOK && inventory[m].pval != pval)) continue;
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
			if (si != -1) *cp = (si + 1) * 100 + i;
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
		if (si != -1) *cp = (si + 1) * 100 + i;
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
 */
static int get_tag(int *cp, char tag, bool inven, bool equip, int mode) {
	int i, j, si;
	int start, stop, step;
	bool inven_first = (mode & INVEN_FIRST) != 0;
#ifdef SMART_SWAP
	int i_found = -1, tval, sval;
#endif

	/* neither inventory nor equipment is allowed to be searched? */
	if (!inven && !equip) return(FALSE);

	/* search tag in inventory before looking in equipment? (default is other way round) */
	if (inven_first) {
		start = inven ? 0 : INVEN_WIELD;
		stop = equip ? INVEN_TOTAL : INVEN_PACK;
		step = 1;
	} else {
		start = (equip ? INVEN_TOTAL : INVEN_PACK) - 1;
		stop = (inven ? 0 : INVEN_WIELD) - 1;
		step = -1;
	}

	/* Init with 'no valid item found' */
	*cp = -1;

	/* Check every object */
	for (j = start; j != stop; j += step) {
		if (!inven_first) {
			/* Translate, so equip and inven are processed in normal order _each_ */
			i = INVEN_PACK + (j >= INVEN_WIELD ? INVEN_TOTAL : 0) - 1 - j;
		} else {
			i = j;
		}

#ifdef SMART_SWAP
		if (i_found != -1) {
			/* Skip same-type inventory items to become our alternative replacement item for swapping.
			   TODO: Apply method 1/2/3 here, instead of just 1. */
			if (tval == inventory[i].tval && sval == inventory[i].sval) continue;
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
			for (si = 0; si < inventory[i].pval; si++) {
				if (get_tag_aux((i + 1) * 100 + si, cp, tag, mode)) return(TRUE);
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
	int i, j, stop;
	char buf[ONAME_LEN];
	char buf1[ONAME_LEN], buf2[ONAME_LEN], *ptr; /* for manual strcasestr() */
	bool inven_first = (mode & INVEN_FIRST) != 0;
#ifdef SMART_SWAP
	char buf3[ONAME_LEN];
	bool chk_multi = (mode & CHECK_MULTI) != 0;
	int i_found = -1, tval, sval, pval, method;
#endif
	bool charged = (mode & CHECK_CHARGED) != 0;
#ifdef ENABLE_SUBINVEN
	bool subinven = (mode & USE_SUBINVEN);
#endif

	strcpy(buf, "");
	if (!get_string(get_item_hook_find_obj_what, buf, 79)) return(FALSE);

#ifdef ENABLE_SUBINVEN
    if (using_subinven != -1) {
	for (j = 0; j < using_subinven_size; j++) {
		i = j;

		object_type *o_ptr = &subinventory[using_subinven][i];

		if (!item_tester_okay(o_ptr)) continue;

 #if 0
		if (my_strcasestr(subinventory_name[using_subinven][i], buf)) {
 #else
		strcpy(buf1, subinventory_name[using_subinven][i]);
		strcpy(buf2, buf);
		ptr = buf1;
		while (*ptr) {
			/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
			   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
			if (*ptr == '@') ptr ++;
			else *ptr = tolower(*ptr);
			ptr++;
		}
		ptr = buf2;
		while (*ptr) {
			/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
			   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
			if (*ptr == '@') ptr += 2;
			*ptr = tolower(*ptr);
			ptr++;
		}
		if (strstr(buf1, buf2)) {
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
					if (k == i) continue;
					strcpy(buf3, subinventory_name[using_subinven][k]);
					ptr = buf3;
					while (*ptr) {
						/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
						   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
						if (*ptr == '@') ptr++;
						else *ptr = tolower(*ptr);
						ptr++;
					}
					/* Skip fully charging stacks */
					if (strstr(buf3, "(charging)") || strstr(buf3, "(#)") || /* rods (and other devices, in theory) */
					    //(partially charging) || strstr(buf3, "(~)")
					    strstr(buf3, "(0 charges") || strstr(buf3, "{empty}")) /* wands, staves */
						continue;

					if (!(buf3p = strstr(buf3, " of "))) buf3p = buf3; //skip item's article/amount
					if (subinventory[using_subinven][k].tval == subinventory[using_subinven][i].tval && /* unnecessary check, but whatever */
					    strstr(buf3, buf2)) {
						i = k;
						break;
					}
				}
			}
			*item = i;
			return(TRUE);
		}
	}
	return(FALSE);
    } else if (subinven) {
	/* Scan all subinvens for item name match */
	int l;

	for (l = 0; l < INVEN_PACK; l++) {
		/* Assume that subinvens are always at the beginning of the inventory! */
		if (inventory[l].tval != TV_SUBINVEN) break;

		for (j = 0; j < inventory[l].pval; j++) {
			i = j;

			object_type *o_ptr = &subinventory[l][i];

			if (!item_tester_okay(o_ptr)) continue;

 #if 0
			if (my_strcasestr(subinventory_name[l][i], buf)) {
 #else
			strcpy(buf1, subinventory_name[l][i]);
			strcpy(buf2, buf);
			ptr = buf1;
			while (*ptr) {
				/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
				   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
				if (*ptr == '@') ptr ++;
				else *ptr = tolower(*ptr);
				ptr++;
			}
			ptr = buf2;
			while (*ptr) {
				/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
				   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
				if (*ptr == '@') ptr += 2;
				*ptr = tolower(*ptr);
				ptr++;
			}
			if (strstr(buf1, buf2)) {
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
					for (k = 0; k < inventory[l].pval; k++) {
						if (k == i) continue;
						strcpy(buf3, subinventory_name[l][k]);
						ptr = buf3;
						while (*ptr) {
							/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
							   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
							if (*ptr == '@') ptr++;
							else *ptr = tolower(*ptr);
							ptr++;
						}
						/* Skip fully charging stacks */
						if (strstr(buf3, "(charging)") || strstr(buf3, "(#)") || /* rods (and other devices, in theory) */
						    //(partially charging) || strstr(buf3, "(~)")
						    strstr(buf3, "(0 charges") || strstr(buf3, "{empty}")) /* wands, staves */
							continue;

						if (!(buf3p = strstr(buf3, " of "))) buf3p = buf3; //skip item's article/amount
						if (subinventory[l][k].tval == subinventory[l][i].tval && /* unnecessary check, but whatever */
						    strstr(buf3, buf2)) {
							i = k;
							break;
						}
					}
				}
				*item = i + (l + 1) * 100;
				return(TRUE);
			}
		}
	}
	/* Fall through and scan inventory normally, after we didn't find anything in subinvens. */
    }
#endif
	stop = INVEN_TOTAL;
	for (j = inven_first ? 0 : stop - 1;
	    inven_first ? (j < stop) : (j >= 0);
	    inven_first ? j++ : j--) {
		/* translate back so order within each - inven & equip - is alphabetically again */
		if (!inven_first) {
			if (j < INVEN_WIELD) i = INVEN_PACK - j;
			else i = INVEN_WIELD + INVEN_TOTAL - 1 - j;
		} else i = j;

		object_type *o_ptr = &inventory[i];

		if (!item_tester_okay(o_ptr)) continue;

#ifdef SMART_SWAP
		if (i_found != -1) {
			/* Skip same-type inventory items to become our alternative replacement item for swapping.
			   TODO: Apply method 1/2/3 here, instead of just 1. */
			if (tval == inventory[i].tval && sval == inventory[i].sval) continue;
		}
#endif

#if 0
		if (my_strcasestr(inventory_name[i], buf)) {
#else
		strcpy(buf1, inventory_name[i]);
		strcpy(buf2, buf);
		ptr = buf1;
		while (*ptr) {
			/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
			   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
			if (*ptr == '@') ptr ++;
			else *ptr = tolower(*ptr);
			ptr++;
		}
		ptr = buf2;
		while (*ptr) {
			/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
			   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
			if (*ptr == '@') ptr += 2;
			*ptr = tolower(*ptr);
			ptr++;
		}
//printf("comparing '%s','%s'\n", buf1, buf2);
		if (strstr(buf1, buf2)) {
#endif
#ifdef SMART_SWAP /* not really cool, with the ' of ' hack.. problem was eg 'ring' vs 'rings' */
			if (chk_multi && i <= INVEN_PACK) {
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
					strcpy(buf3, inventory_name[k]);
					ptr = buf3;
					while (*ptr) {
						/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
						   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
						if (*ptr == '@') ptr++;
						else *ptr = tolower(*ptr);
						ptr++;
					}
 #if 0 /* old, unclean (and 'buggy' for items of same tval+sval but with different pval, yet we certainly don't want to treat them as 'different') */
					/* Actually we should only test for equipment slots that fulfill wield_slot() condition
					   for the inventory item, but since we don't have this function client-side we just test all.. */
					if (strstr(buf3, buf2) && !strcmp(buf1p, buf3p)) {
 #else /* quick, rough fix, copy-pasted from new get_tag_aux(). Todo: clean this mess up in a similar fashion to get_tag()+get_tag_aux() */
					if (!inventory[k].tval) continue;

					switch (method) {
					case 1:
						/* Compare tval and sval directly, assuming they are available - true for non-flavour items.
						   Almost no need to check bpval or pval, because switching items in such a manner doesn't make much sense,
						   not even for polymorph rings, EXCEPT for spell books! */
						if (inventory[k].tval != tval || inventory[k].sval != sval || (inventory[k].tval == TV_BOOK && inventory[k].sval == SV_SPELLBOOK && inventory[k].pval != pval)) continue;
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
					if (strstr(buf3, buf2)) {
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
				}
				if (k < INVEN_TOTAL && i_found != -1) continue;
			}
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
				for (k = 0; k <= INVEN_PACK; k++) {
					if (k == i) continue;
					strcpy(buf3, inventory_name[k]);
					ptr = buf3;
					while (*ptr) {
						/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
						   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
						if (*ptr == '@') ptr++;
						else *ptr = tolower(*ptr);
						ptr++;
					}
					/* Skip fully charging stacks */
					if (strstr(buf3, "(charging)") || strstr(buf3, "(#)") || /* rods (and other devices, in theory) */
					    //(partially charging) || strstr(buf3, "(~)")
					    strstr(buf3, "(0 charges") || strstr(buf3, "{empty}")) /* wands, staves */
						continue;

					if (!(buf3p = strstr(buf3, " of "))) buf3p = buf3; //skip item's article/amount
					if (inventory[k].tval == inventory[i].tval && /* unnecessary check, but whatever */
					    strstr(buf3, buf2)) {
						i = k;
						break;
					}
				}
			}
			*item = i;
			return(TRUE);
		}
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
	bool done, spammy = FALSE;
	byte item;

	char tmp_val[160];
	char out_val[160];

	bool equip = FALSE;
	bool inven = FALSE;
	//bool floor = FALSE;
	bool extra = FALSE, limit = FALSE;
	bool special_req = FALSE;
	bool newest = FALSE;
	bool equip_first = FALSE;
#ifdef ENABLE_SUBINVEN
	bool subinven = FALSE, found_subinven = FALSE;
	int sub_i = (using_subinven + 1) * 100;
#endif
	bool safe_input = FALSE;
	int use_without_asking = -1; /* Use first item found with new "@<commandkey>%" inscription right away without prompting for item choice */


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
	newest = (item_newest != -1); /* experimental: always on if available */
	if (mode & USE_LIMIT) limit = TRUE;
	if (mode & EQUIP_FIRST) equip_first = TRUE;
#ifdef ENABLE_SUBINVEN
	/* Hack: Always enable subinven when we also have inven enabled, for now.
	   The idea is future client compatibility in case more custom bags are added, so items are already compatible. */
	if (inven) mode |= USE_SUBINVEN;

	if (mode & USE_SUBINVEN) subinven = extra = TRUE; /* Since we cannot list normal inven + subinven at the same time, we NEED 'extra' access via item name */
#endif

	/* Too long description - shorten? */
	if (special_req && newest) spammy = TRUE;

#ifdef ENABLE_SUBINVEN
	if (using_subinven != -1) {
		inven = equip = FALSE; /* If we are inside a specific subinventory already, disallow normal inventory */
		subinven = TRUE; /* ..and definitely allow subinven, of course. */
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
		int j, s;

		/* Also scan all subinventories for at least one valid item */
		for (k = 0; k < INVEN_PACK; k++) {
			if (inventory[k].tval != TV_SUBINVEN) continue;
			/* Check all _specialized_ container types. Chests are not eligible. */
			if (inventory[k].sval >= SV_SI_CHEST_SMALL_WOODEN && inventory[k].sval <= SV_SI_CHEST_LARGE_STEEL) continue;

			for (j = 0; j < inventory[k].pval; j++) {
				if (!get_item_okay((k + 1) * 100 + j)) continue;
				found_subinven = TRUE;
 #if 0
				break;
 #else
				if (use_without_asking == -1 && get_tag(&s, '%', TRUE, FALSE, mode)) {
					use_without_asking = s;//(s + 1) * 100 + j;
					break;
				}
 #endif
			}
			if (use_without_asking != -1) break;
		}

		/* Exit prematurely via @x% inscription match in subinventory? */
		if (use_without_asking != -1) {
			(*cp) = use_without_asking;

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

	/* Repeat while done */
	while (!done) {
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
			if (!command_see) strcat(out_val, " * to see,");

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

		/* Limit? */
		if (limit) {
			if (spammy) strcat(out_val, " # limit,");
			else strcat(out_val, " # to limit,");
		}

		if (spammy) strcat(out_val, " - switch, + new,");
		/* Special request toggle? */
		else if (special_req) strcat(out_val, " - to switch,");
		/* Re-use 'newest' item? */
		else if (newest) strcat(out_val, " + for newest,");

		/* Finish the prompt */
		strcat(out_val, " ESC");

		/* Build the prompt */
		sprintf(tmp_val, "(%s) %s", out_val, pmt);

		/* Show the prompt */
		prompt_topline(tmp_val);


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
			if (!get_tag(&k, which, inven, equip, mode)) {
				if (c_cfg.item_error_beep) bell();
				else bell_silent();
				break;
			}
#ifdef ENABLE_SUBINVEN
			if (using_subinven != -1) {
				/* get_tag() is using_subinven agnostic, so we have to convert k back to a direct subinven index */
				if (k >= 100) k = k % 100;

				if ((k < using_subinven_size) ? !inven : !equip) {
					if (c_cfg.item_error_beep) bell();
					else bell_silent();
					break;
				}
			} else if (k >= 100) {
				/* Hack -- verify item (in subinventory) */
				if (!inven) {
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
			hack_force_spell_level = c_get_quantity("Limit spell level (0 for unlimited)? ", -1);
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
			if (!newest) {
				if (c_cfg.item_error_beep) bell();
				else bell_silent();
				break;
			}
			which = 'a' + item_newest;
			/* fall through to process 'which' */
			__attribute__ ((fallthrough));

		default:
			/* Extract "query" setting */
			ver = isupper(which);
			if (ver) which = tolower(which);

			/* Convert letter to inventory index */
			if (!command_wrk) k = c_label_to_inven(which);
			/* Convert letter to equipment index */
			else k = c_label_to_equip(which);

			/* Validate the item */
			if (!get_item_okay(k)) {
				if (c_cfg.item_error_beep) bell();
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
	return(item);
}
