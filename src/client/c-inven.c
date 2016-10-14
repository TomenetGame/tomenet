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

s16b index_to_label(int i) {
	/* Indices for "inven" are easy */
	if (i < INVEN_WIELD) return (I2A(i));

	/* Indices for "equip" are offset */
	return (I2A(i - INVEN_WIELD));
}


bool item_tester_okay(object_type *o_ptr) {
	/* Hack -- allow testing empty slots */
	if (item_tester_full) return (TRUE);

	/* Require an item */
	if (!o_ptr->tval) return (FALSE);

	/* Hack -- ignore "gold" */
	if (o_ptr->tval == TV_GOLD) return (FALSE);

	/* Check the tval */
	if (item_tester_tval) {
		if (!(item_tester_tval == o_ptr->tval)) return (FALSE);
	}

	/* Check the weight */
	if (item_tester_max_weight) {
		if (item_tester_max_weight < o_ptr->weight * o_ptr->number) return (FALSE);
	}

	/* Check the hook */
	if (item_tester_hook) {
		if (!(*item_tester_hook)(o_ptr)) return (FALSE);
	}

	/* Assume okay */
	return (TRUE);
}


static bool get_item_okay(int i) {
	/* Illegal items */
	if ((i < 0) || (i >= INVEN_TOTAL)) return (FALSE);

	/* Verify the item */
	if (!item_tester_okay(&inventory[i])) return (FALSE);

	/* Assume okay */
	return (TRUE);
}


static bool verify(cptr prompt, int item) {
	char	o_name[ONAME_LEN];
	char	out_val[MSG_LEN];


	/* Describe */
	strcpy(o_name, inventory_name[item]);

	/* Prompt */
	(void)sprintf(out_val, "%s %s?", prompt, o_name);

	/* Query */
	return (get_check2(out_val, FALSE));
}


static s16b c_label_to_inven(int c) {
	int i;

	/* Convert */
	i = (islower(c) ? A2I(c) : -1);

	/* Verify the index */
	if ((i < 0) || (i > INVEN_PACK)) return (-1);

	/* Empty slots can never be chosen */
	if (!inventory[i].tval) return (-1);

	/* Return the index */
	return (i);
}

static s16b c_label_to_equip(int c) {
	int i;

	/* Convert */
	i = (islower(c) ? A2I(c) : -1) + INVEN_WIELD;

	/* Verify the index */
	if ((i < INVEN_WIELD) || (i >= INVEN_TOTAL)) return (-1);

	/* Empty slots can never be chosen */
	if (!inventory[i].tval) return (-1);

	/* Return the index */
	return (i);
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
 */
static int get_tag(int *cp, char tag, bool inven, bool equip, int mode) {
	int i, j;
	int start, stop, step;
	cptr s;
	bool inven_first = mode & INVEN_FIRST;
#ifdef SMART_SWAP
	int i_found = -1;
	char buf1[ONAME_LEN], buf3[ONAME_LEN];
	bool multi = mode & CHECK_MULTI;
	bool postpone;
#endif
	bool charged = (mode & CHECK_CHARGED) != 0;

	/* neither inventory nor equipment is allowed to be searched? */
	if (!inven && !equip) return (FALSE);

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

	/* Check every object */
#if 0
	//for (i = (inven ? 0 : INVEN_WIELD); i < (equip ? INVEN_TOTAL : INVEN_PACK); ++i)
	/* Equipment before inventory, since inventory
	   items are usually more restricted. - C. Blue */
	for (j = (equip ? INVEN_TOTAL : INVEN_PACK) - 1; j >= (inven ? 0 : INVEN_WIELD); --j)
#else
	for (j = start; j != stop; j += step)
#endif
	{
		char *buf;
		char *buf2;

		if (!inven_first) {
			/* Translate, so equip and inven are processed in normal order _each_ */
			i = INVEN_PACK + (j >= INVEN_WIELD ? INVEN_TOTAL : 0) - 1 - j;
		} else {
			i = j;
		}

		buf = inventory_name[i];

		/* Skip empty objects */
		if (!buf[0]) continue;

		/* Skip items that don't fit (for mkey) */
		if (!item_tester_okay(&inventory[i])) continue;

		/* Skip empty inscriptions (those which don't contain any @.. tag) */
		if (!(buf2 = strchr(buf, '@'))) continue;

		/* Find a '@' */
		s = strchr(buf2, '@');

		/* Process all tags */
		while (s) {
			/* Check the normal tags */
			if (s[1] == tag) {
				if (charged && (
				    strstr(buf, "charging)") || strstr(buf, "(#)") || strstr(buf, "(~)") || /* rods (and other devices, in theory) */
				    strstr(buf, "(0 charges") || strstr(buf, "{empty}") /* wands, staves */
				    )) {
					/* Check for same item in the equipment, if found, search inventory for a non-same alternative */
					char *buf1p, *buf3p, *s3;
					int k;

					strcpy(buf1, buf);
					buf1p = buf1;
					while (*buf1p) {
						/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
						   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
						if (*buf1p == '@') buf1p ++;
						else *buf1p = tolower(*buf1p);
						buf1p++;
					}

					if (!(buf1p = strstr(buf1, " of "))) buf1p = buf1; //skip item's article/amount
					for (k = 0; k <= INVEN_PACK; k++) {
						if (k == i) continue;
						strcpy(buf3, inventory_name[k]);
						buf3p = buf3;
						while (*buf3p) {
							/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
							   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
							if (*buf3p == '@') buf3p ++;
							else *buf3p = tolower(*buf3p);
							buf3p++;
						}
						if (strstr(buf3, "charging)") || strstr(buf3, "(#)") || strstr(buf3, "(~)") || /* rods (and other devices, in theory) */
						    strstr(buf3, "(0 charges") || strstr(buf3, "{empty}")) /* wands, staves */
							continue;

						if (!(buf3p = strstr(buf3, " of "))) buf3p = buf3; //skip item's article/amount
						s3 = strchr(buf3, '@');
						if (inventory[k].tval == inventory[i].tval && /* unnecessary check, but whatever */
						    s3 && s3[1] == tag) {
							i = k;
							break;
						}
					}
				}
#ifdef SMART_SWAP /* not really cool, with the ' of ' hack.. problem was eg 'ring' vs 'rings' */
				postpone = FALSE;
				if (multi && i <= INVEN_PACK) {
					/* Check for same item in the equipment, if found, search inventory for a non-same alternative */
					char *buf1p, *buf3p, *s3;
					int k;

					strcpy(buf1, buf);
					buf1p = buf1;
					while (*buf1p) {
						/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
						   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
						if (*buf1p == '@') buf1p ++;
						else *buf1p = tolower(*buf1p);
						buf1p++;
					}

					if (!(buf1p = strstr(buf1, " of "))) buf1p = buf1; //skip item's article/amount
					for (k = INVEN_WIELD; k <= INVEN_TOTAL; k++) {
						strcpy(buf3, inventory_name[k]);
						buf3p = buf3;
						while (*buf3p) {
							/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
							   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
							if (*buf3p == '@') buf3p ++;
							else *buf3p = tolower(*buf3p);
							buf3p++;
						}

						if (!(buf3p = strstr(buf3, " of "))) buf3p = buf3; //skip item's article/amount
						/* Actually we should only test for equipment slots that fulfill wield_slot() condition
						   for the inventory item, but since we don't have this function client-side we just test all.. */
						s3 = strchr(buf3, '@');
						if (s3 && s3[1] == tag &&
						    !strcmp(buf1p, buf3p)) {
							/* remember this item to use it if we don't find a different one.. */
							i_found = i;
							break;
						}
					}
					if (k <= INVEN_TOTAL && i_found != -1) postpone = TRUE;
				}
			    if (!postpone) {
#endif
				/* Save the actual inventory ID */
				*cp = i;

				/* Success */
				return (TRUE);
#ifdef SMART_SWAP
			    }
#endif
			}

			/* Check the special tags */
			if ((s[1] == command_cmd) && (s[2] == tag)) {
				if (charged && (
				    strstr(buf, "charging)") || strstr(buf, "(#)") || strstr(buf, "(~)") || /* rods (and other devices, in theory) */
				    strstr(buf, "(0 charges") || strstr(buf, "{empty}") /* wands, staves */
				    )) {
					/* Check for same item in the equipment, if found, search inventory for a non-same alternative */
					char *buf1p, *buf3p, *s3;
					int k;

					strcpy(buf1, buf);
					buf1p = buf1;
					while (*buf1p) {
						/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
						   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
						if (*buf1p == '@') buf1p ++;
						else *buf1p = tolower(*buf1p);
						buf1p++;
					}

					if (!(buf1p = strstr(buf1, " of "))) buf1p = buf1; //skip item's article/amount
					for (k = 0; k <= INVEN_PACK; k++) {
						if (k == i) continue;
						strcpy(buf3, inventory_name[k]);
						buf3p = buf3;
						while (*buf3p) {
							/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
							   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
							if (*buf3p == '@') buf3p ++;
							else *buf3p = tolower(*buf3p);
							buf3p++;
						}

						if (!(buf3p = strstr(buf3, " of "))) buf3p = buf3; //skip item's article/amount
						s3 = strchr(buf3, '@');
						if (inventory[k].tval == inventory[i].tval && /* unnecessary check, but whatever */
						    s3 && s3[1] == command_cmd && s3[2] == tag) {
							i = k;
							break;
						}
					}
				}
#ifdef SMART_SWAP /* not really cool, with the ' of ' hack.. problem was eg 'ring' vs 'rings' */
				postpone = FALSE;
				if (multi && i <= INVEN_PACK) {
					/* Check for same item in the equipment, if found, search inventory for a non-same alternative */
					char *buf1p, *buf3p, *s3;
					int k;

					strcpy(buf1, buf);
					buf1p = buf1;
					while (*buf1p) {
						/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
						   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
						if (*buf1p == '@') buf1p ++;
						else *buf1p = tolower(*buf1p);
						buf1p++;
					}

					if (!(buf1p = strstr(buf1, " of "))) buf1p = buf1; //skip item's article/amount
					for (k = INVEN_WIELD; k <= INVEN_TOTAL; k++) {
						strcpy(buf3, inventory_name[k]);
						buf3p = buf3;
						while (*buf3p) {
							/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
							   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
							if (*buf3p == '@') buf3p ++;
							else *buf3p = tolower(*buf3p);
							buf3p++;
						}

						if (!(buf3p = strstr(buf3, " of "))) buf3p = buf3; //skip item's article/amount
						/* Actually we should only test for equipment slots that fulfill wield_slot() condition
						   for the inventory item, but since we don't have this function client-side we just test all.. */
						s3 = strchr(buf3, '@');
						if (s3 && s3[1] == command_cmd && s3[2] == tag &&
						    !strcmp(buf1p, buf3p)) {
							/* remember this item to use it if we don't find a different one.. */
							i_found = i;
							break;
						}
					}
					if (k <= INVEN_TOTAL && i_found != -1) postpone = TRUE;
				}
			    if (!postpone) {
#endif
				/* Save the actual inventory ID */
				*cp = i;

				/* Success */
				return (TRUE);
#ifdef SMART_SWAP
			    }
#endif
			}

			/* Find another '@' */
			s = strchr(s + 1, '@');
		}
	}

#ifdef SMART_SWAP
	if (i_found != -1) {
		*cp = i_found;
		return TRUE;
	}
#endif
	/* No such tag */
	return (FALSE);
}

/*
 * General function to find an item by its name
 *
 * This is modified code from ToME. - mikaelh
 */
cptr get_item_hook_find_obj_what;
bool get_item_hook_find_obj(int *item, int mode) {
	int i, j;
	char buf[ONAME_LEN];
	char buf1[ONAME_LEN], buf2[ONAME_LEN], *ptr; /* for manual strcasestr() */
	bool inven_first = mode & INVEN_FIRST;
#ifdef SMART_SWAP
	char buf3[ONAME_LEN];
	bool multi = mode & CHECK_MULTI;
	int i_found = -1;
#endif
	bool charged = (mode & CHECK_CHARGED) != 0;

	strcpy(buf, "");
	if (!get_string(get_item_hook_find_obj_what, buf, 79))
		return FALSE;

	for (j = inven_first ? 0 : INVEN_TOTAL - 1;
	    inven_first ? (j < INVEN_TOTAL) : (j >= 0);
	    inven_first ? j++ : j--) {
		/* translate back so order within each - inven & equip - is alphabetically again */
		if (!inven_first) {
			if (j < INVEN_WIELD) i = INVEN_PACK - j;
			else i = INVEN_WIELD + INVEN_TOTAL - 1 - j;
		} else i = j;

		object_type *o_ptr = &inventory[i];

		if (!item_tester_okay(o_ptr)) continue;

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
			if (multi && i <= INVEN_PACK) {
				/* Check for same item in the equipment, if found, search inventory for a non-same alternative */
				char *buf1p, *buf3p;
				int k;

				if (!(buf1p = strstr(buf1, " of "))) buf1p = buf1; //skip item's article/amount
				for (k = INVEN_WIELD; k <= INVEN_TOTAL; k++) {
					strcpy(buf3, inventory_name[k]);
					ptr = buf3;
					while (*ptr) {
						/* hack: if search string is actually an inscription (we just test if it starts on '@' char),
						   do not lower-case the following character! (Because for example @a0 is a different command than @A0) */
						if (*ptr == '@') ptr++;
						else *ptr = tolower(*ptr);
						ptr++;
					}

					if (!(buf3p = strstr(buf3, " of "))) buf3p = buf3; //skip item's article/amount
					/* Actually we should only test for equipment slots that fulfill wield_slot() condition
					   for the inventory item, but since we don't have this function client-side we just test all.. */
					if (strstr(buf3, buf2) && !strcmp(buf1p, buf3p)) {
						/* remember this item to use it if we don't find a different one.. */
						i_found = i;
						break;
					}
				}
				if (k <= INVEN_TOTAL && i_found != -1) continue;
			}
#endif
			if (charged && (
			    strstr(buf1, "charging)") || strstr(buf1, "(#)") || strstr(buf1, "(~)") || /* rods (and other devices, in theory) */
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
					if (strstr(buf3, "charging)") || strstr(buf3, "(#)") || strstr(buf3, "(~)") || /* rods (and other devices, in theory) */
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
			return TRUE;
		}
	}
#ifdef SMART_SWAP
	if (i_found != -1) {
		*item = i_found;
		return TRUE;
	}
#endif
	return FALSE;
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

	if (mode & (USE_EQUIP)) equip = TRUE;
	if (mode & (USE_INVEN)) inven = TRUE;
	//if (mode & (USE_FLOOR)) floor = TRUE;
	if (mode & (USE_EXTRA)) extra = TRUE;
	if (mode & (SPECIAL_REQ)) special_req = TRUE;
	//if (mode & (NEWEST))
	newest = (item_newest != -1); /* experimental: always on if available */
	if (mode & (USE_LIMIT)) limit = TRUE;

	/* Too long description - shorten? */
	if (special_req && newest) spammy = TRUE;

	/* Paranoia */
	if (!inven && !equip) {
		/* Forget the item_tester_tval restriction */
		item_tester_tval = 0;
		/* Forget the item_tester_max_weight restriction */
		item_tester_max_weight = 0;
		/* Forget the item_tester_hook restriction */
		item_tester_hook = 0;

		return (FALSE);
	}

	/* Command macros work as an exception here */
	//inkey_get_item = TRUE;

	/* Full inventory */
	i1 = 0;
	i2 = INVEN_PACK - 1;

	/* Forbid inventory */
	if (!inven) i2 = -1;

	/* Restrict inventory indices */
	while ((i1 <= i2) && (!get_item_okay(i1))) i1++;
	while ((i1 <= i2) && (!get_item_okay(i2))) i2--;


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

	if ((i1 > i2) && (e1 > e2)) {
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

		/* Flush any events */
		Flush_queue();//this will cancel macro execution already, so an additional 'c_cfg.safe_macros' check isn't needed here

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

		return item;
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
	}

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
			xhtml_screenshot("screenshot????");
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
				bell();
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
				bell();
				break;
			}

			/* Hack -- verify item */
			if ((k < INVEN_WIELD) ? !inven : !equip) {
				bell();
				break;
			}

			/* Validate the item */
			if (!get_item_okay(k)) {
				bell();
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
				bell();
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
			item = TRUE;
			done = TRUE;
			break;

		case '@':
		{
			int i;

			if (extra && get_item_extra_hook(&i, mode)) {
				(*cp) = i;
				item = TRUE;
				done = TRUE;
			} else bell();
			break;
		}

		case '#':
			if (!limit) {
				bell();
				break;
			}
			hack_force_spell_level = c_get_quantity("Limit spell level (0 for unlimited)? ", -1);
			if (hack_force_spell_level < 0) hack_force_spell_level = 0;
			if (hack_force_spell_level > 99) hack_force_spell_level = 99;
			limit = FALSE; //just for visuals: don't offer to re-enter level limit over and over since it's pointless
			break;

		case '-':
			if (!special_req) {
				bell();
				break;
			}
			command_gap = 50;
			done = TRUE;
			item = FALSE;
			*cp = -3;
			break;

		case '+':
			if (!newest) {
				bell();
				break;
			}
			which = 'a' + item_newest;
			/* fall through to process 'which' */

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
				bell();
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
			item = TRUE;
			done = TRUE;

			/* Remember that we hit SHIFT+slot to override whole_ammo_stack */
			if (ver) verified_item = TRUE;
			break;
		}
	}


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
	return (item);
}
