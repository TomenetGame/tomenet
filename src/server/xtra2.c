/* File: effects.c */

/* Purpose: effects of various "objects" */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#define SERVER

#include "angband.h"


/*
 * Set "p_ptr->adrenaline", notice observable changes
 * Note the interaction with biofeedback
 */
bool set_adrenaline(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE, sudden = FALSE, crash = FALSE;

        int i;
	
	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->adrenaline)
		{
			msg_print(Ind, "Adrenaline surges through your veins!");
			if (p_ptr->biofeedback)
			{
				msg_print(Ind, "You lose control of your blood flow!");
				i = randint(randint(v));
				take_hit(Ind, damroll(2, i),"adrenaline poisoning");
				v = v - i + 1;
				p_ptr->biofeedback = 0;
			}
			
			notice = TRUE;
		}
		else
		{
			/* Sudden crash */
			if (!rand_int(500) && (p_ptr->adrenaline >= v))
			{
				msg_print(Ind, "Your adrenaline suddenly runs out!");
				v = 0;
				sudden = TRUE;
				if (!rand_int(2)) crash = TRUE;
			}
		}
		
		while (v > 30 + randint(p_ptr->lev * 5))
		{
			msg_print(Ind, "Your body can't handle that much adrenaline!");
			i = randint(randint(v));
			take_hit(Ind, damroll(3, i * 2),"adrenaline poisoning");
			v = v - i + 1;
		}		
	}

	/* Shut */
	else
	{
		if (p_ptr->adrenaline)
		{
			if (!rand_int(3))
			{
				crash = TRUE;
				msg_print(Ind, "Your adrenaline runs out, leaving you tired and weak.");
			}
			else
			{
				msg_print(Ind, "Your heart slows down to normal.");
			}	
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->adrenaline = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Notice */
	p_ptr->update |= (PU_BONUS | PU_HP);
	
	/* Disturb */
	if (disturb_state) disturb(Ind, 0, 0);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
	
}
/*
 * Set "p_ptr->biofeedback", notice observable changes
 * Note the interaction with adrenaline
 */
bool set_biofeedback(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;
	
	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->biofeedback)
		{
			msg_print(Ind, "Your pulse slows and your body prepares to resist damage.");
			if (p_ptr->adrenaline)
			{
				msg_print(Ind, "The adrenaline drains out of your veins.");
				p_ptr->adrenaline = 0;
				if (!rand_int(8))
				{
					msg_print(Ind, "You start to tremble as your blood sugar crashes.");
					set_slow(Ind, p_ptr->slow + rand_int(rand_int(16)));
					if (!rand_int(5)) set_paralyzed(Ind, p_ptr->paralyzed + 1);
					if (!rand_int(3)) set_stun(Ind, p_ptr->stun + rand_int(30));
				}
			}
			notice = TRUE;
		}		
	}

	/* Shut */
	else
	{
		msg_print(Ind, "Your veins return to normal.");
	}
				
	while (v > 35 + rand_int(rand_int(p_ptr->lev)))
	{
			msg_print(Ind, "You speed up your pulse to avoid fainting!");
			v -= 20;
	}

	/* Use the value */
	p_ptr->biofeedback = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Notice */
	p_ptr->update |= (PU_BONUS | PU_HP);
	
	/* Disturb */
	if (disturb_state) disturb(Ind, 0, 0);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
	
}


/*
 * Set "p_ptr->tim_esp", notice observable changes
 */
bool set_tim_esp(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->tim_esp)
		{
			msg_print(Ind, "Your mind expands !");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tim_esp)
		{
			msg_print(Ind, "Your mind retracts.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_esp = v;
	
	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS | PU_MONSTERS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->st_anchor", notice observable changes
 */
bool set_st_anchor(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->st_anchor)
		{
			msg_print(Ind, "The Space/Time Continuum seems to solidify !");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->st_anchor)
		{
			msg_print(Ind, "The Space/Time Continuum seems more flexible.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->st_anchor = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->prob_travel", notice observable changes
 */
bool set_prob_travel(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->prob_travel)
		{
			msg_print(Ind, "You feel instable !");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->prob_travel)
		{
			msg_print(Ind, "You feel more stable.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->prob_travel = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->bow_brand_xxx", notice observable changes
 */
bool set_bow_brand(int Ind, int v, int t, int p)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->bow_brand)
		{
		  switch (t)
		    {
		    case BOW_BRAND_ELEC:
                    case BOW_BRAND_BALL_ELEC:
		      msg_print(Ind, "\377oYour ammos sparkle with lightnings !");
		      break;
                    case BOW_BRAND_BALL_COLD:
		    case BOW_BRAND_COLD:
		      msg_print(Ind, "\377oYour ammos freeze !");
		      break;
                    case BOW_BRAND_BALL_FIRE:
		    case BOW_BRAND_FIRE:
		      msg_print(Ind, "\377oYour ammos burn !");
		      break;
                    case BOW_BRAND_BALL_ACID:
		    case BOW_BRAND_ACID:
		      msg_print(Ind, "\377oYour ammos look acidic !");
		      break;
		    case BOW_BRAND_POIS:
		      msg_print(Ind, "\377oYour ammos are covered with venom !");
		      break;
		    case BOW_BRAND_MANA:
		      msg_print(Ind, "\377oYour ammos glow with power !");
		      break;
		    case BOW_BRAND_CONF:
		      msg_print(Ind, "\377oYour ammos glow many colors !");
		      break;
		    case BOW_BRAND_SHARP:
		      msg_print(Ind, "\377oYour ammos sharpen !");
		      break;
                    case BOW_BRAND_BALL_SOUND:
                      msg_print(Ind, "\377oYour ammos vibrate !");
		      break;
		    }
		  notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->bow_brand)
		{
			msg_print(Ind, "\377oYour ammos seem normal again.");
			notice = TRUE;
			t = 0;
			p = 0;
		}
	}

	/* Use the value */
	p_ptr->bow_brand = v;
	p_ptr->bow_brand_t = t;
	p_ptr->bow_brand_d = p;
	

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS | PU_MONSTERS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->tim_mimic", notice observable changes
 */
bool set_mimic(int Ind, int v, int p)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->tim_mimic)
		{
			msg_print(Ind, "Your image changes !");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tim_mimic)
		{
			msg_print(Ind, "Your image is back to normality.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_mimic = v;
	
	/* Enforce good values */
	if (p < 0) p = 0;
	if (p >= MAX_CLASS) p = MAX_CLASS - 1;
	p_ptr->tim_mimic_what = p;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS | PU_MONSTERS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->tim_manashield", notice observable changes
 */
bool set_tim_manashield(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->tim_manashield)
		{
			msg_print(Ind, "You feel immortal !");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tim_manashield)
		{
			msg_print(Ind, "You feel less immortal.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_manashield = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->tim_traps", notice observable changes
 */
bool set_tim_traps(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->tim_traps)
		{
			msg_print(Ind, "You can avoid all the traps !");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tim_traps)
		{
			msg_print(Ind, "You should worry about traps again.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_traps = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->tim_invis", notice observable changes
 */
bool set_invis(int Ind, int v, int p)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->tim_invisibility)
		{
			msg_format_near(Ind, "%s fades in the shadows!", p_ptr->name);
			msg_print(Ind, "You fade in the shadow!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tim_invisibility)
		{
			msg_format_near(Ind, "The shadows enveloping %s disipate.", p_ptr->name);
			msg_print(Ind, "The shadows enveloping you disipate.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_invisibility = v;
	p_ptr->tim_invis_power = p;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS | PU_MONSTERS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->furry", notice observable changes
 */
bool set_furry(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->furry)
		{
			msg_print(Ind, "You grow a fury!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->furry)
		{
			msg_print(Ind, "The fury stops.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->furry = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->tim_meditation", notice observable changes
 */
bool set_tim_meditation(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->tim_meditation)
		{
			msg_format_near(Ind, "%s starts a calm meditation!", p_ptr->name);
			msg_print(Ind, "You start a calm meditation!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tim_meditation)
		{
			msg_format_near(Ind, "%s stops meditating.", p_ptr->name);
			msg_print(Ind, "You stop your meditation.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_meditation = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_HP | PU_MANA);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->tim_wraith", notice observable changes
 */
bool set_tim_wraith(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(&p_ptr->wpos))) return FALSE;
#endif

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->tim_wraith)
		{
			msg_format_near(Ind, "%s turns into a wraith!", p_ptr->name);
			msg_print(Ind, "You turn into a wraith!");
			notice = TRUE;
			
			p_ptr->wraith_in_wall = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tim_wraith)
		{
			msg_format_near(Ind, "%s loses %s wraith powers.", p_ptr->name, p_ptr->male ? "his":"her");
			msg_print(Ind, "You lose your wraith powers.");
			notice = TRUE;
			
			/* That will hopefully prevent game hinging when loading */
#ifdef NEW_DUNGEON
			if (cave_floor_bold(zcave, p_ptr->py, p_ptr->px)) p_ptr->wraith_in_wall = FALSE;
#else
			if (cave_floor_bold(p_ptr->dun_depth, p_ptr->py, p_ptr->px)) p_ptr->wraith_in_wall = FALSE;
#endif
		}
	}

	/* Use the value */
	p_ptr->tim_wraith = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->blind", notice observable changes
 *
 * Note the use of "PU_UN_LITE" and "PU_UN_VIEW", which is needed to
 * memorize any terrain features which suddenly become "visible".
 * Note that blindness is currently the only thing which can affect
 * "player_can_see_bold()".
 */
bool set_blind(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* the admin wizard can not be blinded */
	if (!strcmp(p_ptr->name, cfg_admin_wizard)) return 1;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->blind)
		{
			msg_format_near(Ind, "%s gropes around blindly!", p_ptr->name);
			msg_print(Ind, "You are blind!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->blind)
		{
			msg_format_near(Ind, "%s can see again.", p_ptr->name);
			msg_print(Ind, "You can see again.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->blind = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Forget stuff */
	p_ptr->update |= (PU_UN_VIEW | PU_UN_LITE);

	/* Update stuff */
	p_ptr->update |= (PU_VIEW | PU_LITE);

	/* Update the monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Redraw the "blind" */
	p_ptr->redraw |= (PR_BLIND);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->confused", notice observable changes
 */
bool set_confused(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->confused)
		{
			msg_format_near(Ind, "%s appears confused!", p_ptr->name);
			msg_print(Ind, "You are confused!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->confused)
		{
			msg_print(Ind, "You feel less confused now.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->confused = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Redraw the "confused" */
	p_ptr->redraw |= (PR_CONFUSED);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->poisoned", notice observable changes
 */
bool set_poisoned(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->poisoned)
		{
			msg_print(Ind, "You are poisoned!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->poisoned)
		{
			msg_print(Ind, "You are no longer poisoned.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->poisoned = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Redraw the "poisoned" */
	p_ptr->redraw |= (PR_POISONED);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->afraid", notice observable changes
 */
bool set_afraid(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->afraid)
		{
			msg_format_near(Ind, "%s cowers in fear!", p_ptr->name);
			msg_print(Ind, "You are terrified!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->afraid)
		{
			msg_format_near(Ind, "%s appears bolder now.", p_ptr->name);
			msg_print(Ind, "You feel bolder now.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->afraid = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Redraw the "afraid" */
	p_ptr->redraw |= (PR_AFRAID);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->paralyzed", notice observable changes
 */
bool set_paralyzed(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->paralyzed)
		{
			msg_format_near(Ind, "%s becomes rigid!", p_ptr->name);
			msg_print(Ind, "You are paralyzed!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->paralyzed)
		{
			msg_format_near(Ind, "%s can move again.", p_ptr->name);
			msg_print(Ind, "You can move again.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->paralyzed = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Redraw the state */
	p_ptr->redraw |= (PR_STATE);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->image", notice observable changes
 *
 * Note that we must redraw the map when hallucination changes.
 */
bool set_image(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->image)
		{
			msg_print(Ind, "You feel drugged!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->image)
		{
			msg_print(Ind, "You can see clearly again.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->image = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Update monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->fast", notice observable changes
 */
bool set_fast(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->fast)
		{
			msg_format_near(Ind, "%s begins moving faster!", p_ptr->name);
			msg_print(Ind, "You feel yourself moving faster!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->fast)
		{
			msg_format_near(Ind, "%s slows down.", p_ptr->name);
			msg_print(Ind, "You feel yourself slow down.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->fast = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->slow", notice observable changes
 */
bool set_slow(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->slow)
		{
			msg_format_near(Ind, "%s begins moving slower!", p_ptr->name);
			msg_print(Ind, "You feel yourself moving slower!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->slow)
		{
			msg_format_near(Ind, "%s speeds up.", p_ptr->name);
			msg_print(Ind, "You feel yourself speed up.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->slow = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->shield", notice observable changes
 */
bool set_shield(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->shield)
		{
			msg_print(Ind, "A mystic shield forms around your body!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->shield)
		{
			msg_print(Ind, "Your mystic shield crumbles away.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->shield = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}



/*
 * Set "p_ptr->blessed", notice observable changes
 */
bool set_blessed(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->blessed)
		{
			msg_print(Ind, "You feel righteous!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->blessed)
		{
			msg_print(Ind, "The prayer has expired.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->blessed = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->hero", notice observable changes
 */
bool set_hero(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->hero)
		{
			msg_print(Ind, "You feel like a hero!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->hero)
		{
			msg_print(Ind, "The heroism wears off.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->hero = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Recalculate hitpoints */
	p_ptr->update |= (PU_HP);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->shero", notice observable changes
 */
bool set_shero(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->shero)
		{
			msg_print(Ind, "You feel like a killing machine!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->shero)
		{
			msg_print(Ind, "You feel less Berserk.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->shero = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Recalculate hitpoints */
	p_ptr->update |= (PU_HP);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->protevil", notice observable changes
 */
bool set_protevil(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->protevil)
		{
			msg_print(Ind, "You feel safe from evil!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->protevil)
		{
			msg_print(Ind, "You no longer feel safe from evil.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->protevil = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->invuln", notice observable changes
 */
bool set_invuln(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->invuln)
		{
			msg_print(Ind, "You feel invulnerable!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->invuln)
		{	/* Keeps the 2 turn GOI from getting annoying. DEG */
			if ((p_ptr->pclass == CLASS_MAGE) && (p_ptr->lev > 44))
			{
				msg_print(Ind, "You feel vulnerable once more.");
			}
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->invuln = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->invuln", but not notice observable changes
 * It should be used to protect players from recall-instadeath.  - Jir -
 */
bool set_invuln_short(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	/* not cumulative */
	if (p_ptr->invuln) return (FALSE);

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Use the value */
	p_ptr->invuln = v;

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result (never noticeable) */
	return (FALSE);
}

/*
 * Set "p_ptr->tim_invis", notice observable changes
 */
bool set_tim_invis(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->tim_invis)
		{
			msg_print(Ind, "Your eyes feel very sensitive!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tim_invis)
		{
			msg_print(Ind, "Your eyes feel less sensitive.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_invis = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Update the monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->tim_infra", notice observable changes
 */
bool set_tim_infra(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->tim_infra)
		{
			msg_print(Ind, "Your eyes begin to tingle!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tim_infra)
		{
			msg_print(Ind, "Your eyes stop tingling.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_infra = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Update the monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->oppose_acid", notice observable changes
 */
bool set_oppose_acid(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->oppose_acid)
		{
			msg_print(Ind, "You feel resistant to acid!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->oppose_acid)
		{
			msg_print(Ind, "You feel less resistant to acid.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->oppose_acid = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->oppose_elec", notice observable changes
 */
bool set_oppose_elec(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->oppose_elec)
		{
			msg_print(Ind, "You feel resistant to electricity!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->oppose_elec)
		{
			msg_print(Ind, "You feel less resistant to electricity.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->oppose_elec = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->oppose_fire", notice observable changes
 */
bool set_oppose_fire(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->oppose_fire)
		{
			msg_print(Ind, "You feel resistant to fire!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->oppose_fire)
		{
			msg_print(Ind, "You feel less resistant to fire.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->oppose_fire = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->oppose_cold", notice observable changes
 */
bool set_oppose_cold(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->oppose_cold)
		{
			msg_print(Ind, "You feel resistant to cold!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->oppose_cold)
		{
			msg_print(Ind, "You feel less resistant to cold.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->oppose_cold = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->oppose_pois", notice observable changes
 */
bool set_oppose_pois(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!p_ptr->oppose_pois)
		{
			msg_print(Ind, "You feel resistant to poison!");
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->oppose_pois)
		{
			msg_print(Ind, "You feel less resistant to poison.");
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->oppose_pois = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->stun", notice observable changes
 *
 * Note the special code to only notice "range" changes.
 */
bool set_stun(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	int old_aux, new_aux;

	bool notice = FALSE;


	/* hack -- the admin wizard can not be stunned */
	if (!strcmp(p_ptr->name, cfg_admin_wizard)) return TRUE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Knocked out */
	if (p_ptr->stun > 100)
	{
		old_aux = 3;
	}

	/* Heavy stun */
	else if (p_ptr->stun > 50)
	{
		old_aux = 2;
	}

	/* Stun */
	else if (p_ptr->stun > 0)
	{
		old_aux = 1;
	}

	/* None */
	else
	{
		old_aux = 0;
	}

	/* Knocked out */
	if (v > 100)
	{
		new_aux = 3;
	}

	/* Heavy stun */
	else if (v > 50)
	{
		new_aux = 2;
	}

	/* Stun */
	else if (v > 0)
	{
		new_aux = 1;
	}

	/* None */
	else
	{
		new_aux = 0;
	}

	/* Increase cut */
	if (new_aux > old_aux)
	{
		/* Describe the state */
		switch (new_aux)
		{
			/* Stun */
			case 1:
			msg_format_near(Ind, "\377o%s appears stunned.", p_ptr->name);
			msg_print(Ind, "\377oYou have been stunned.");
			break;

			/* Heavy stun */
			case 2:
			msg_format_near(Ind, "\377o%s is very stunned.", p_ptr->name);
			msg_print(Ind, "\377oYou have been heavily stunned.");
			break;

			/* Knocked out */
			case 3:
			msg_format_near(Ind, "%s has been knocked out.", p_ptr->name);
			msg_print(Ind, "You have been knocked out.");
			break;
		}

		/* Notice */
		notice = TRUE;
	}

	/* Decrease cut */
	else if (new_aux < old_aux)
	{
		/* Describe the state */
		switch (new_aux)
		{
			/* None */
			case 0:
			msg_format_near(Ind, "\377o%s is no longer stunned.", p_ptr->name);
			msg_print(Ind, "\377oYou are no longer stunned.");
			if (p_ptr->disturb_state) disturb(Ind, 0, 0);
			break;
		}

		/* Notice */
		notice = TRUE;
	}

	/* Use the value */
	p_ptr->stun = v;

	/* No change */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Redraw the "stun" */
	p_ptr->redraw |= (PR_STUN);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->cut", notice observable changes
 *
 * Note the special code to only notice "range" changes.
 */
bool set_cut(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	int old_aux, new_aux;

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* a ghost never bleeds */
	if (v && p_ptr->ghost) v = 0;

	/* Mortal wound */
	if (p_ptr->cut > 1000)
	{
		old_aux = 7;
	}

	/* Deep gash */
	else if (p_ptr->cut > 200)
	{
		old_aux = 6;
	}

	/* Severe cut */
	else if (p_ptr->cut > 100)
	{
		old_aux = 5;
	}

	/* Nasty cut */
	else if (p_ptr->cut > 50)
	{
		old_aux = 4;
	}

	/* Bad cut */
	else if (p_ptr->cut > 25)
	{
		old_aux = 3;
	}

	/* Light cut */
	else if (p_ptr->cut > 10)
	{
		old_aux = 2;
	}

	/* Graze */
	else if (p_ptr->cut > 0)
	{
		old_aux = 1;
	}

	/* None */
	else
	{
		old_aux = 0;
	}

	/* Mortal wound */
	if (v > 1000)
	{
		new_aux = 7;
	}

	/* Deep gash */
	else if (v > 200)
	{
		new_aux = 6;
	}

	/* Severe cut */
	else if (v > 100)
	{
		new_aux = 5;
	}

	/* Nasty cut */
	else if (v > 50)
	{
		new_aux = 4;
	}

	/* Bad cut */
	else if (v > 25)
	{
		new_aux = 3;
	}

	/* Light cut */
	else if (v > 10)
	{
		new_aux = 2;
	}

	/* Graze */
	else if (v > 0)
	{
		new_aux = 1;
	}

	/* None */
	else
	{
		new_aux = 0;
	}

	/* Increase cut */
	if (new_aux > old_aux)
	{
		/* Describe the state */
		switch (new_aux)
		{
			/* Graze */
			case 1:
			msg_print(Ind, "You have been given a graze.");
			break;

			/* Light cut */
			case 2:
			msg_print(Ind, "You have been given a light cut.");
			break;

			/* Bad cut */
			case 3:
			msg_print(Ind, "You have been given a bad cut.");
			break;

			/* Nasty cut */
			case 4:
			msg_print(Ind, "You have been given a nasty cut.");
			break;

			/* Severe cut */
			case 5:
			msg_print(Ind, "You have been given a severe cut.");
			break;

			/* Deep gash */
			case 6:
			msg_print(Ind, "You have been given a deep gash.");
			break;

			/* Mortal wound */
			case 7:
			msg_print(Ind, "You have been given a mortal wound.");
			break;
		}

		/* Notice */
		notice = TRUE;
	}

	/* Decrease cut */
	else if (new_aux < old_aux)
	{
		/* Describe the state */
		switch (new_aux)
		{
			/* None */
			case 0:
			msg_print(Ind, "You are no longer bleeding.");
			if (p_ptr->disturb_state) disturb(Ind, 0, 0);
			break;
		}

		/* Notice */
		notice = TRUE;
	}

	/* Use the value */
	p_ptr->cut = v;

	/* No change */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Redraw the "cut" */
	p_ptr->redraw |= (PR_CUT);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->food", notice observable changes
 *
 * The "p_ptr->food" variable can get as large as 20000, allowing the
 * addition of the most "filling" item, Elvish Waybread, which adds
 * 7500 food units, without overflowing the 32767 maximum limit.
 *
 * Perhaps we should disturb the player with various messages,
 * especially messages about hunger status changes.  XXX XXX XXX
 *
 * Digestion of food is handled in "dungeon.c", in which, normally,
 * the player digests about 20 food units per 100 game turns, more
 * when "fast", more when "regenerating", less with "slow digestion",
 * but when the player is "gorged", he digests 100 food units per 10
 * game turns, or a full 1000 food units per 100 game turns.
 *
 * Note that the player's speed is reduced by 10 units while gorged,
 * so if the player eats a single food ration (5000 food units) when
 * full (15000 food units), he will be gorged for (5000/100)*10 = 500
 * game turns, or 500/(100/5) = 25 player turns (if nothing else is
 * affecting the player speed).
 */
bool set_food(int Ind, int v)
{
	player_type *p_ptr = Players[Ind];

	int old_aux, new_aux;

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 20000) ? 20000 : (v < 0) ? 0 : v;

	/* Fainting / Starving */
	if (p_ptr->food < PY_FOOD_FAINT)
	{
		old_aux = 0;
	}

	/* Weak */
	else if (p_ptr->food < PY_FOOD_WEAK)
	{
		old_aux = 1;
	}

	/* Hungry */
	else if (p_ptr->food < PY_FOOD_ALERT)
	{
		old_aux = 2;
	}

	/* Normal */
	else if (p_ptr->food < PY_FOOD_FULL)
	{
		old_aux = 3;
	}

	/* Full */
	else if (p_ptr->food < PY_FOOD_MAX)
	{
		old_aux = 4;
	}

	/* Gorged */
	else
	{
		old_aux = 5;
	}

	/* Fainting / Starving */
	if (v < PY_FOOD_FAINT)
	{
		new_aux = 0;
	}

	/* Weak */
	else if (v < PY_FOOD_WEAK)
	{
		new_aux = 1;
	}

	/* Hungry */
	else if (v < PY_FOOD_ALERT)
	{
		new_aux = 2;
	}

	/* Normal */
	else if (v < PY_FOOD_FULL)
	{
		new_aux = 3;
	}

	/* Full */
	else if (v < PY_FOOD_MAX)
	{
		new_aux = 4;
	}

	/* Gorged */
	else
	{
		new_aux = 5;
	}

	/* Food increase */
	if (new_aux > old_aux)
	{
		/* Describe the state */
		switch (new_aux)
		{
			/* Weak */
			case 1:
			msg_print(Ind, "You are still weak.");
			break;

			/* Hungry */
			case 2:
			msg_print(Ind, "You are still hungry.");
			break;

			/* Normal */
			case 3:
			msg_print(Ind, "You are no longer hungry.");
			break;

			/* Full */
			case 4:
			msg_print(Ind, "You are full!");
			break;

			/* Bloated */
			case 5:
			msg_print(Ind, "You have gorged yourself!");
			break;
		}

		/* Change */
		notice = TRUE;
	}

	/* Food decrease */
	else if (new_aux < old_aux)
	{
		/* Describe the state */
		switch (new_aux)
		{
			/* Fainting / Starving */
			case 0:
			msg_print(Ind, "You are getting faint from hunger!");
			/* Hack -- if the player is at full hit points, 
			 * destroy his conneciton (this will hopefully prevent
			 * people from starving while afk)
			 */
			if (p_ptr->chp == p_ptr->mhp)
			{
				/* Use the value */
				p_ptr->food = v;
				Destroy_connection(p_ptr->conn, "Starving to death!");
				return TRUE;
			}
			break;

			/* Weak */
			case 1:
			msg_print(Ind, "You are getting weak from hunger!");
			break;

			/* Hungry */
			case 2:
			msg_print(Ind, "You are getting hungry.");
			break;

			/* Normal */
			case 3:
			msg_print(Ind, "You are no longer full.");
			break;

			/* Full */
			case 4:
			msg_print(Ind, "You are no longer gorged.");
			break;
		}

		/* Change */
		notice = TRUE;
	}

	/* Use the value */
	p_ptr->food = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (p_ptr->disturb_state) disturb(Ind, 0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Redraw hunger */
	p_ptr->redraw |= (PR_HUNGER);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Result */
	return (TRUE);
}





/*
 * Advance experience levels and print experience
 */
void check_experience(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int		i;


	/* Note current level */
	i = p_ptr->lev;


	/* Hack -- lower limit */
	if (p_ptr->exp < 0) p_ptr->exp = 0;

	/* Hack -- lower limit */
	if (p_ptr->max_exp < 0) p_ptr->max_exp = 0;

	/* Hack -- upper limit */
	if (p_ptr->exp > PY_MAX_EXP) p_ptr->exp = PY_MAX_EXP;

	/* Hack -- upper limit */
	if (p_ptr->max_exp > PY_MAX_EXP) p_ptr->max_exp = PY_MAX_EXP;


	/* Hack -- maintain "max" experience */
	if (p_ptr->exp > p_ptr->max_exp) p_ptr->max_exp = p_ptr->exp;

	/* Redraw experience */
	p_ptr->redraw |= (PR_EXP);

	/* Handle stuff */
	handle_stuff(Ind);

	/* Lose levels while possible */
	while ((p_ptr->lev > 1) &&
               (p_ptr->exp < ((s64b)((s64b)player_exp[p_ptr->lev-2] * (s64b)p_ptr->expfact / 100L))))
	{
		/* Lose a level */
		p_ptr->lev--;

		/* Update some stuff */
		p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);

		/* Redraw some stuff */
		p_ptr->redraw |= (PR_LEV | PR_TITLE);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER | PW_SPELL);

		/* Handle stuff */
		handle_stuff(Ind);
	}

	/* Gain levels while possible */
        while ((p_ptr->lev < PY_MAX_LEVEL) &&
               (p_ptr->exp >= ((s64b)((s64b)player_exp[p_ptr->lev-1] *
                               (s64b)p_ptr->expfact / 100L))))
	{
		/* Gain a level */
		p_ptr->lev++;

		/* Save the highest level */
		if (p_ptr->lev > p_ptr->max_plv) p_ptr->max_plv = p_ptr->lev;

		/* Sound */
		sound(Ind, SOUND_LEVEL);

		/* Message */
		msg_format(Ind, "Welcome to level %d.", p_ptr->lev);

		/* Update some stuff */
		p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);

		/* Redraw some stuff */
		p_ptr->redraw |= (PR_LEV | PR_TITLE);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER | PW_SPELL);

		/* Handle stuff */
		handle_stuff(Ind);
	}
}


/*
 * Gain experience
 */
void gain_exp(int Ind, s32b amount)
{
	player_type *p_ptr = Players[Ind], *p_ptr2;
	int Ind2 = 0;

	/* You cant gain xp on your land */
	if (player_is_king(Ind)) return;
        if (p_ptr->ghost) return;

	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_PAIN))
	  {
	    Ind2 = find_player(p_ptr->esp_link);

	    if (!Ind2)
	      end_mind(Ind, TRUE);
	    else
	      {
		p_ptr2 = Players[Ind2];
	      }
	  }

	if (Ind2)
	  {
	    /* Gain some experience */
	    p_ptr->exp += amount / 2;

	    /* Slowly recover from experience drainage */
	    if (p_ptr->exp < p_ptr->max_exp)
	      {
		/* Gain max experience (10%) */
		p_ptr->max_exp += amount / 20;
	      }
	    
	    /* Check Experience */
	    check_experience(Ind);

	    /* Gain some experience */
            p_ptr2->exp += amount / 2;

	    /* Slowly recover from experience drainage */
	    if (p_ptr2->exp < p_ptr2->max_exp)
	      {
		/* Gain max experience (10%) */
		p_ptr2->max_exp += amount / 20;
	      }
	    
	    /* Check Experience */
	    check_experience(Ind2);
	  }
	else
	  {
	    /* Gain some experience */
	    p_ptr->exp += amount;

	    /* Slowly recover from experience drainage */
	    if (p_ptr->exp < p_ptr->max_exp)
	      {
		/* Gain max experience (10%) */
		p_ptr->max_exp += amount / 10;
	      }
	    
	    /* Check Experience */
	    check_experience(Ind);
	  }
}


/*
 * Lose experience
 */
void lose_exp(int Ind, s32b amount)
{
	player_type *p_ptr = Players[Ind];

	/* Never drop below zero experience */
	if (amount > p_ptr->exp) amount = p_ptr->exp;

	/* Lose some experience */
	p_ptr->exp -= amount;

	/* Check Experience */
	check_experience(Ind);
}




/*
 * Hack -- Return the "automatic coin type" of a monster race
 * Used to allocate proper treasure when "Creeping coins" die
 *
 * XXX XXX XXX Note the use of actual "monster names"
 */
static int get_coin_type(monster_race *r_ptr)
{
	cptr name = (r_name + r_ptr->name);

	/* Analyze "coin" monsters */
	if (r_ptr->d_char == '$')
	{
		/* Look for textual clues */
		if (strstr(name, " copper ")) return (2);
		if (strstr(name, " silver ")) return (5);
		if (strstr(name, " gold ")) return (10);
		if (strstr(name, " mithril ")) return (16);
		if (strstr(name, " adamantite ")) return (17);

		/* Look for textual clues */
		if (strstr(name, "Copper ")) return (2);
		if (strstr(name, "Silver ")) return (5);
		if (strstr(name, "Gold ")) return (10);
		if (strstr(name, "Mithril ")) return (16);
		if (strstr(name, "Adamantite ")) return (17);
	}

	/* Assume nothing */
	return (0);
}


/*
 * Handle the "death" of a monster.
 *
 * Disperse treasures centered at the monster location based on the
 * various flags contained in the monster flags fields.
 *
 * Check for "Quest" completion when a quest monster is killed.
 *
 * Note that only the player can induce "monster_death()" on Uniques.
 * Thus (for now) all Quest monsters should be Uniques.
 *
 * Note that in a few, very rare, circumstances, killing Morgoth
 * may result in the Iron Crown of Morgoth crushing the Lead-Filled
 * Mace "Grond", since the Iron Crown is more important.
 */
 
 
void monster_death(int Ind, int m_idx)
{
	player_type *p_ptr = Players[Ind];
	player_type *q_ptr = Players[Ind];

	int			i, j, y, x, ny, nx;

	int			dump_item = 0;
	int			dump_gold = 0;

	int			number = 0;
	int			total = 0;

	char buf[160];

	cave_type		*c_ptr;

	monster_type	*m_ptr = &m_list[m_idx];

        monster_race *r_ptr = race_inf(m_ptr);

	bool visible = (p_ptr->mon_vis[m_idx] || (r_ptr->flags1 & RF1_UNIQUE));

	bool good = (r_ptr->flags1 & RF1_DROP_GOOD) ? TRUE : FALSE;
	bool great = (r_ptr->flags1 & RF1_DROP_GREAT) ? TRUE : FALSE;

	bool do_gold = (!(r_ptr->flags1 & RF1_ONLY_ITEM));
	bool do_item = (!(r_ptr->flags1 & RF1_ONLY_GOLD));

	int force_coin = get_coin_type(r_ptr);
#ifdef NEW_DUNGEON
	struct worldpos *wpos;
	cave_type **zcave;
#else
	int Depth;
#endif


	/* Get the location */
	y = m_ptr->fy;
	x = m_ptr->fx;
#ifdef NEW_DUNGEON
	wpos=&m_ptr->wpos;
	if(!(zcave=getcave(wpos))) return;
#else
	Depth = m_ptr->dun_depth;
#endif

	/* Determine how much we can drop */
	if ((r_ptr->flags1 & RF1_DROP_60) && (rand_int(100) < 60)) number++;
	if ((r_ptr->flags1 & RF1_DROP_90) && (rand_int(100) < 90)) number++;
	if (r_ptr->flags1 & RF1_DROP_1D2) number += damroll(1, 2);
	if (r_ptr->flags1 & RF1_DROP_2D2) number += damroll(2, 2);
	if (r_ptr->flags1 & RF1_DROP_3D2) number += damroll(3, 2);
	if (r_ptr->flags1 & RF1_DROP_4D2) number += damroll(4, 2);

	/* Drop some objects */
	for (j = 0; j < number; j++)
	{
		/* Try 20 times per item, increasing range */
		for (i = 0; i < 20; ++i)
		{
			int d = (i + 14) / 15;

			/* Pick a "correct" location */
#ifdef NEW_DUNGEON
			scatter(wpos, &ny, &nx, y, x, d, 0);
			/* Must be "clean" floor grid */
			if (!cave_clean_bold(zcave, ny, nx)) continue;

			/* Access the grid */
			c_ptr = &zcave[ny][nx];
#else
			scatter(Depth, &ny, &nx, y, x, d, 0);
			/* Must be "clean" floor grid */
			if (!cave_clean_bold(Depth, ny, nx)) continue;

			/* Access the grid */
			c_ptr = &cave[Depth][ny][nx];
#endif


			/* Hack -- handle creeping coins */
			coin_type = force_coin;

			/* Average dungeon and monster levels */
#ifdef NEW_DUNGEON
			object_level = (getlevel(wpos) + r_ptr->level) / 2;
#else
			object_level = (Depth + r_ptr->level) / 2;
#endif

			/* Place Gold */
			if (do_gold && (!do_item || (rand_int(100) < 50)))
			{
#ifdef NEW_DUNGEON
				place_gold(wpos, ny, nx);
				if (player_can_see_bold(Ind, ny, nx)) dump_gold++;
#else
				place_gold(Depth, ny, nx);
				if (player_can_see_bold(Ind, ny, nx)) dump_gold++;
#endif
			}

			/* Place Object */
			else
			{
#ifdef NEW_DUNGEON
				place_object(wpos, ny, nx, good, great);
				if (player_can_see_bold(Ind, ny, nx)) dump_item++;
#else
				place_object(Depth, ny, nx, good, great);
				if (player_can_see_bold(Ind, ny, nx)) dump_item++;
#endif
			}

			/* Reset the object level */
#ifdef NEW_DUNGEON
			object_level = getlevel(wpos);
#else
			object_level = Depth;
#endif

			/* Reset "coin" type */
			coin_type = 0;

			/* Notice */
#ifdef NEW_DUNGEON
			note_spot_depth(wpos, ny, nx);

			/* Display */
			everyone_lite_spot(wpos, ny, nx);
#else
			note_spot_depth(Depth, ny, nx);

			/* Display */
			everyone_lite_spot(Depth, ny, nx);
#endif

			/* Under a player */
			if (c_ptr->m_idx < 0)
			{
				msg_print(0 - c_ptr->m_idx, "You feel something roll beneath your feet.");
			}

			break;
		}
	}


	/* Take note of any dropped treasure */
	if (visible && (dump_item || dump_gold))
	{
		/* Take notes on treasure */
		lore_treasure(m_idx, dump_item, dump_gold);
	}

	if (p_ptr->r_killed[m_ptr->r_idx] < 1000)
	  {
	    /* Remember */
	    p_ptr->r_killed[m_ptr->r_idx]++;
	  }

	/* Take note of the killer */
	if (r_ptr->flags1 & RF1_UNIQUE)
	{
	        int Ind2 = 0;
		player_type *p_ptr2;

		if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_PAIN))
		  {
		    Ind2 = find_player(p_ptr->esp_link);
		    
		    if (!Ind2)
		      end_mind(Ind, TRUE);
		    else
		      {
			p_ptr2 = Players[Ind2];

			/* Remember */
			p_ptr2->r_killed[m_ptr->r_idx]++;
		      }
		  }


		/* give credit to the killer by default */
		if (!Ind2)
		  {
                    sprintf(buf,"\377b%s was slain by %s.", r_name_get(m_ptr), p_ptr->name);
		  }
		else
		  {
                    sprintf(buf,"\377b%s was slain by fusion %s-%s.", r_name_get(m_ptr), p_ptr->name, p_ptr2->name);
		  }
		
		/* give credit to the party if there is a teammate on the 
		level, and the level is not 0 (the town)  */
		if (p_ptr->party)
		{
			for (i = 1; i <= NumPlayers; i++)
			{
#ifdef NEW_DUNGEON
				if ( (Players[i]->party == p_ptr->party) && (inarea(&Players[i]->wpos, &p_ptr->wpos)) && (i != Ind) && (p_ptr->wpos.wz) )
#else
				if ( (Players[i]->party == p_ptr->party) && (Players[i]->dun_depth == p_ptr->dun_depth) && (i != Ind) && (p_ptr->dun_depth) )
#endif
				{
                                        sprintf(buf, "\377b%s was slain by %s.", r_name_get(m_ptr),parties[p_ptr->party].name);
					break; 
				} 
			
			}
		
		} 
		         
  
    		/* Tell every player */
    		msg_broadcast(Ind, buf);
	}

	/* Mega-Hack -- drop "winner" treasures */
	if (r_ptr->flags1 & RF1_DROP_CHOSEN)
	{
		/* Hack -- an "object holder" */
		object_type prize;

 	        int num = 0;

		/* Nothing left, game over... */
		for (i = 1; i <= NumPlayers; i++)
		{
			q_ptr = Players[i];
			/* Make everyone in the game in the same party on the
			 * same level greater than or equal to level 40 total
			 * winners.
			 */
#ifdef NEW_DUNGEON
			if ((((p_ptr->party) && (q_ptr->party == p_ptr->party)) ||
			   (q_ptr == p_ptr) ) && q_ptr->lev >= 40 && inarea(&p_ptr->wpos,&q_ptr->wpos))
#else
			if ((((p_ptr->party) && (q_ptr->party == p_ptr->party)) ||
			   (q_ptr == p_ptr) ) && q_ptr->lev >= 40 && p_ptr->dun_depth == q_ptr->dun_depth)
#endif
			{
			  int Ind2 = 0;
			  player_type *p_ptr2;
			  
			  if (q_ptr->esp_link_type && q_ptr->esp_link && (q_ptr->esp_link_flags & LINKF_PAIN))
			    {
			      Ind2 = find_player(q_ptr->esp_link);
			      
			      if (!Ind2)
				end_mind(i, TRUE);
			      else
				{
				  p_ptr2 = Players[Ind2];

				  /* Total winner */
				  p_ptr2->total_winner = TRUE;

				  /* Redraw the "title" */
				  p_ptr2->redraw |= (PR_TITLE);

				  /* Congratulations */
				  msg_print(i, "*** CONGRATULATIONS ***");
				  msg_print(i, "You have won the game!");
				  msg_print(i, "You may retire (commit suicide) when you are ready.");

				  num++;

				  /* Set his retire_timer if neccecary */
				  if (cfg_retire_timer >= 0)
				    {
				      p_ptr2->retire_timer = cfg_retire_timer;
					  msg_format(i, "Otherwise you will retire after %s minutes of tenure.", cfg_retire_timer);
				    }
				}
			    }
				/* Total winner */
			        q_ptr->total_winner = TRUE;

				/* Redraw the "title" */
				q_ptr->redraw |= (PR_TITLE);

				/* Congratulations */
				msg_print(i, "*** CONGRATULATIONS ***");
				msg_print(i, "You have won the game!");
				msg_print(i, "You may retire (commit suicide) when you are ready.");

				num++;

				/* Set his retire_timer if neccecary */
				if (cfg_retire_timer >= 0)
				{
					q_ptr->retire_timer = cfg_retire_timer;
					msg_format(i, "Otherwise you will retire after %s minutes of tenure.", cfg_retire_timer);
				}
			}
		}	

		/* Mega-Hack -- Prepare to make "Grond" */
		invcopy(&prize, lookup_kind(TV_HAFTED, SV_GROND));

		/* Mega-Hack -- Mark this item as "Grond" */
		prize.name1 = ART_GROND;

		/* Mega-Hack -- Actually create "Grond" */
#ifdef NEW_DUNGEON
		apply_magic(wpos, &prize, -1, TRUE, TRUE, TRUE);
#else
		apply_magic(Depth, &prize, -1, TRUE, TRUE, TRUE);
#endif

		prize.number = num;

		/* Drop it in the dungeon */
#ifdef NEW_DUNGEON
		drop_near(&prize, -1, wpos, y, x);
#else
		drop_near(&prize, -1, Depth, y, x);
#endif


		/* Mega-Hack -- Prepare to make "Morgoth" */
		invcopy(&prize, lookup_kind(TV_CROWN, SV_MORGOTH));

		/* Mega-Hack -- Mark this item as "Morgoth" */
		prize.name1 = ART_MORGOTH;

		/* Mega-Hack -- Actually create "Morgoth" */
#ifdef NEW_DUNGEON
		apply_magic(wpos, &prize, -1, TRUE, TRUE, TRUE);
#else
		apply_magic(Depth, &prize, -1, TRUE, TRUE, TRUE);
#endif

		prize.number = num;

		/* Drop it in the dungeon */
#ifdef NEW_DUNGEON
		drop_near(&prize, -1, wpos, y, x);
#else
		drop_near(&prize, -1, Depth, y, x);
#endif

		/* Hack -- instantly retire any new winners if neccecary */
		if (cfg_retire_timer == 0)
		{
			for (i = 1; i <= NumPlayers; i++)
			{
				p_ptr = Players[i];
				if (p_ptr->total_winner)
					do_cmd_suicide(i);
			}
		}

                FREE(m_ptr->r_ptr, monster_race);
		return;
	}


	/* Only process "Quest Monsters" */
	if (!(r_ptr->flags1 & RF1_QUESTOR)) return;

	/* Hack -- Mark quests as complete */
	for (i = 0; i < MAX_Q_IDX; i++)
	{
		/* Hack -- note completed quests */
		if (q_list[i].level == r_ptr->level) q_list[i].level = 0;

		/* Count incomplete quests */
		if (q_list[i].level) total++;
	}


	/* Need some stairs */
	if (total)
	{
		/* Stagger around */
#ifdef NEW_DUNGEON
		while (!cave_valid_bold(zcave, y, x))
#else
		while (!cave_valid_bold(Depth, y, x))
#endif
		{
			int d = 1;

			/* Pick a location */
#ifdef NEW_DUNGEON
			scatter(wpos, &ny, &nx, y, x, d, 0);
#else
			scatter(Depth, &ny, &nx, y, x, d, 0);
#endif

			/* Stagger */
			y = ny; x = nx;
		}

		/* Delete any old object XXX XXX XXX */
#ifdef NEW_DUNGEON
		delete_object(wpos, y, x);
#else
		delete_object(Depth, y, x);
#endif

		/* Explain the stairway */
		msg_print(Ind, "A magical stairway appears...");

		/* Access the grid */
#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x];
#else
		c_ptr = &cave[Depth][y][x];
#endif

		/* Create stairs down */
		c_ptr->feat = FEAT_MORE;

#ifdef NEW_DUNGEON
		/* Note the spot */
		note_spot_depth(wpos, y, x);

		/* Draw the spot */
		everyone_lite_spot(wpos, y, x);
#else
		/* Note the spot */
		note_spot_depth(Depth, y, x);

		/* Draw the spot */
		everyone_lite_spot(Depth, y, x);
#endif

		/* Remember to update everything */
		p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MONSTERS);
	}

        FREE(m_ptr->r_ptr, monster_race);
}

static void kill_house_contents(house_type *h_ptr){
#ifdef NEW_DUNGEON
	struct worldpos *wpos=&h_ptr->wpos;
#else
	int depth=h_ptr->depth;
#endif
	if(h_ptr->flags&HF_RECT){
		int sy,sx,ey,ex,x,y;
		sy=h_ptr->y+1;
		sx=h_ptr->x+1;
		ey=h_ptr->y+h_ptr->coords.rect.height-1;
		ex=h_ptr->x+h_ptr->coords.rect.width-1;
		for(y=sy;y<ey;y++){
			for(x=sx;x<ex;x++){
#ifdef NEW_DUNGEON
				delete_object(wpos,y,x);	
#else
				delete_object(depth,y,x);	
#endif
			}
		}
	}
	else{
		fill_house(h_ptr, 1, NULL);
		/* Polygonal house */
	}
}

void kill_houses(int id, int type){
	int i;
	for(i=0;i<num_houses;i++){
		struct dna_type *dna=houses[i].dna;
		if(dna->owner==id && dna->owner_type==type){
			dna->owner=0L;
			dna->creator=0L;
			dna->a_flags=ACF_NONE;
			kill_house_contents(&houses[i]);
		}
	}
}

void kill_objs(int id){
	int i;
	object_type *o_ptr;
	for(i=0;i<o_max;i++){
		o_ptr=&o_list[i];
		if(!o_ptr->k_idx) continue;
		if(o_ptr->owner==id){
			o_ptr->owner=MAX_ID+1;
		}
	}
}

/*
 * Handle the death of a player and drop their stuff.
 */
 
 /* 
  HACKED to handle fruit bat 
  changed so players remain in the team when killed
  changed so when leader ghosts perish the team is disbanded
  -APD-
 */
 
void player_death(int Ind)
{
	player_type *p_ptr = Players[Ind];
	char buf[1024];
	int i;
	wilderness_type *wild;
	bool hell=TRUE;

	if(p_ptr->mode!=MODE_HELL){
		struct dungeon_type *dungeon;
		wild=&wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx];
		dungeon=(p_ptr->wpos.wz > 0 ? wild->tower : wild->dungeon);
		if(!p_ptr->wpos.wz || !(dungeon->flags & DUNGEON_HELL))
			hell=FALSE;
	}

	if (p_ptr->esp_link_type && p_ptr->esp_link && (p_ptr->esp_link_flags & LINKF_PAIN))
	  {
	    int Ind2 = find_player(p_ptr->esp_link);

	    if (!Ind2)
	      end_mind(Ind, TRUE);
	    else
	      {
		strcpy(Players[Ind2]->died_from, p_ptr->died_from);
		if (!Players[Ind2]->ghost)
		{	strcpy(Players[Ind2]->died_from_list, p_ptr->died_from);
#ifdef NEW_DUNGEON
			Players[Ind2]->died_from_depth = getlevel(&Players[Ind2]->wpos);
#else
			Players[Ind2]->died_from_depth = Players[Ind2]->dun_depth;
#endif
		}
                bypass_invuln = TRUE;
                take_hit(Ind2, 32000, p_ptr->died_from);
                bypass_invuln = FALSE;
	      }
	  }

	/* Get rid of him if he's a ghost */
	if (p_ptr->ghost || (hell && p_ptr->alive))
	{
		/* Tell players */
		sprintf(buf, "\377r%s's ghost was destroyed by %s.",
				p_ptr->name, p_ptr->died_from);

		msg_broadcast(Ind, buf);
		kill_houses(p_ptr->id, OT_PLAYER);
		kill_objs(p_ptr->id);
		p_ptr->death=TRUE;
		
		/* Remove him from his party */
		if (p_ptr->party)
		{
			/* He leaves */
			party_leave(Ind);
		}

                /* Ghosts dont static the lvl if under cfg_preserve_death_level ft. DEG */

#ifdef NEW_DUNGEON
                if (getlevel(&p_ptr->wpos) < cfg_preserve_death_level)
                {
                	new_players_on_depth(&p_ptr->wpos,-1,TRUE);
                }
#else
                if ((p_ptr->dun_depth) < cfg_preserve_death_level)
                {
                	players_on_depth[p_ptr->dun_depth]--;
                }
#endif

		/* Remove him from the player name database */
		delete_player_name(p_ptr->name);

		/* Put him on the high score list */
		add_high_score(Ind);

		/* Format string */
		sprintf(buf, "Killed by %s (%d points)", p_ptr->died_from, total_points(Ind));

		/* Get rid of him */
		Destroy_connection(p_ptr->conn, buf);

		/* Done */
		return;
	}

	/* Tell everyone he died */
	
	if (p_ptr->fruit_bat == -1)
		sprintf(buf, "%s was turned into a fruit bat by %s!", p_ptr->name, p_ptr->died_from);
	
	else if (p_ptr->alive)
		sprintf(buf, "\377r%s was killed by %s.", p_ptr->name, p_ptr->died_from);
	else if (!p_ptr->total_winner)
		sprintf(buf, "\377r%s committed suicide.", p_ptr->name);
	else
		sprintf(buf, "The unbeatable %s has retired to a warm, sunny climate.", p_ptr->name);
	/* Tell the players */
	/* handle the secret_dungeon_master option */
	if ((strcmp(p_ptr->name,cfg_dungeon_master)) || (!cfg_secret_dungeon_master)) {
		if(p_ptr->lev>1 || p_ptr->alive)
			msg_broadcast(Ind, buf);
	}

	/* Drop gold if player has any */
        if (p_ptr->alive && p_ptr->au)
	{
		/* Put the player's gold in the overflow slot */
		invcopy(&p_ptr->inventory[INVEN_PACK], 488);

		/* Drop no more than 32000 gold */
		if (p_ptr->au > 32000) p_ptr->au = 32000;
		if(p_ptr->lev >= 5){
			/* Set the amount */
			p_ptr->inventory[INVEN_PACK].pval = p_ptr->au;
		}

		/* No more gold */
		p_ptr->au = 0;
	}

	/* Polymorph back to player (moved)*/
	/* if (p_ptr->body_monster) do_mimic_change(Ind, 0); */

	/* Setup the sorter */
	ang_sort_comp = ang_sort_comp_value;
	ang_sort_swap = ang_sort_swap_value;

	/* Sort the player's inventory according to value */
	ang_sort(Ind, p_ptr->inventory, NULL, INVEN_TOTAL);

	/* Unown land */
	if (p_ptr->total_winner)
	{
#ifdef NEW_DUNGEON
/* fixme */
/*
		msg_broadcast(Ind, format("%d(%d) and %d(%d) are no more owned.", p_ptr->own1, p_ptr->own2, p_ptr->own1 * 50, p_ptr->own2 * 50));
		wild_info[p_ptr->own1].own = wild_info[p_ptr->own2].own = 0;
*/
#else
		msg_broadcast(Ind, format("%d(%d) and %d(%d) are no more owned.", p_ptr->own1, p_ptr->own2, p_ptr->own1 * 50, p_ptr->own2 * 50));
		wild_info[p_ptr->own1].own = wild_info[p_ptr->own2].own = 0;
#endif
	}	
	
	/* Starting with the most valuable, drop things one by one */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		/* Make sure we have an object */
		if (p_ptr->inventory[i].k_idx == 0)
			continue;

		/* If we committed suicide, only drop artifacts */
		if (!p_ptr->alive && !artifact_p(&p_ptr->inventory[i])) continue;

		/* hack -- total winners do not drop artifacts when they suicide */
//		if (!p_ptr->alive && p_ptr->total_winner && artifact_p(&p_ptr->inventory[i])) 

		/* Artifacts cannot be dropped after all */	
		if (cfg_anti_arts_horde && !p_ptr->alive && artifact_p(&p_ptr->inventory[i])) 
		{
			/* set the artifact as unfound */
			a_info[p_ptr->inventory[i].name1].cur_num = 0;
			
			/* Don't drop the artifact */
			continue;
		}

		if(p_ptr->lev >= cfg_newbies_cannot_drop){
			/* Drop this one */
#ifdef NEW_DUNGEON
			drop_near(&p_ptr->inventory[i], 0, &p_ptr->wpos, p_ptr->py, p_ptr->px);
#else
			drop_near(&p_ptr->inventory[i], 0, p_ptr->dun_depth, p_ptr->py, p_ptr->px);
#endif
		}

		/* No more item */
		p_ptr->inventory[i].k_idx = 0;
		p_ptr->inventory[i].tval = 0;
		p_ptr->inventory[i].ident = 0;
		inven_item_increase(Ind, i, -p_ptr->inventory[i].number);
	}
	if (p_ptr->fruit_bat != -1)

	/* Handle suicide */
	if (!p_ptr->alive)
	{
		/* Delete his houses */
		kill_houses(p_ptr->id, OT_PLAYER);
		kill_objs(p_ptr->id);

		/* Remove him from his party */
		if (p_ptr->party)
		{
			/* He leaves */
			party_leave(Ind);
		}
	
		/* Kill him */
		p_ptr->death = TRUE;

		/* One less player here */
#ifdef NEW_DUNGEON
		new_players_on_depth(&p_ptr->wpos,-1,TRUE);
#else
		players_on_depth[p_ptr->dun_depth]--;
#endif

		/* Remove him from the player name database */
		delete_player_name(p_ptr->name);

		/* Put him on the high score list */
		add_high_score(Ind);

		/* Get rid of him */
		Destroy_connection(p_ptr->conn, "Committed suicide");

		/* Done */
		return;
	}

	if (p_ptr->fruit_bat == -1) 
	{
		p_ptr->mhp = (p_ptr->player_hp[p_ptr->lev-1] / 4) + (((adj_con_mhp[p_ptr->stat_ind[A_CON]]) - 128) * p_ptr->lev);
		p_ptr->chp = p_ptr->mhp;
		p_ptr->chp_frac = 0;
	}
	else
	{

	/* Tell him */
	msg_format(Ind, "\377RYou have been killed by %s.", p_ptr->died_from);

	/* Polymorph back to player */
	if (p_ptr->body_monster) do_mimic_change(Ind, 0);

	/* Turn him into a ghost */
	p_ptr->ghost = 1;

	/* Teleport him */
	teleport_player(Ind, 200);

	/* Give him his hit points back */
	p_ptr->chp = p_ptr->mhp;
	p_ptr->chp_frac = 0;
	}
	
	
	
	/* Cure him from various maladies */
	if (p_ptr->image) (void)set_image(Ind, 0);
	if (p_ptr->blind) (void)set_blind(Ind, 0);
	if (p_ptr->paralyzed) (void)set_paralyzed(Ind, 0);
	if (p_ptr->confused) (void)set_confused(Ind, 0);
	if (p_ptr->poisoned) (void)set_poisoned(Ind, 0);
	if (p_ptr->stun) (void)set_stun(Ind, 0);
	if (p_ptr->cut) (void)set_cut(Ind, 0);
	if (p_ptr->fruit_bat != -1) (void)set_food(Ind, PY_FOOD_MAX - 1);
	else p_ptr->fruit_bat = 2;
	/* Remove the death flag */
	p_ptr->death = 0;

	/* Cancel any WOR spells */
	p_ptr->word_recall = 0;

	/* He is carrying nothing */
	p_ptr->inven_cnt = 0;

	/* Update bonus */
	p_ptr->update |= (PU_BONUS);

	/* Redraw */
	p_ptr->redraw |= (PR_HP | PR_GOLD | PR_BASIC);

	/* Notice */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Windows */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL);
}

/*
 * Resurrect a player
 */
 
 /* To prevent people from ressurecting too many times, I am modifying this to give
    everyone 1 "freebie", and then to have a p_ptr->level % chance of failing to 
    ressurect and have your ghost be destroyed.
    
    -APD-
    
    hmm, haven't gotten aroudn to doing this yet...
 */
 
void resurrect_player(int Ind)
{
	player_type *p_ptr = Players[Ind];

	/* Hack -- the dungeon master can not ressurect */
	if (!strcmp(p_ptr->name,cfg_dungeon_master)) return;

	/* Reset ghost flag */
	p_ptr->ghost = 0;
	
	

	/* Lose some experience */
	p_ptr->max_exp -= p_ptr->max_exp / 2;
	p_ptr->exp -= p_ptr->exp / 2;
	check_experience(Ind);

	/* Message */
	msg_print(Ind, "You feel life return to your body.");
#ifdef NEW_DUNGEON
	everyone_lite_spot(&p_ptr->wpos, p_ptr->py, p_ptr->px);
#else
	everyone_lite_spot(p_ptr->dun_depth, p_ptr->py, p_ptr->px);
#endif

	/* Redraw */
	p_ptr->redraw |= (PR_BASIC);

	/* Update */
	p_ptr->update |= (PU_BONUS);

	/* Window */
	p_ptr->window |= (PW_SPELL);
}



/*
 * Decreases monsters hit points, handling monster death.
 *
 * We return TRUE if the monster has been killed (and deleted).
 *
 * We announce monster death (using an optional "death message"
 * if given, and a otherwise a generic killed/destroyed message).
 *
 * Only "physical attacks" can induce the "You have slain" message.
 * Missile and Spell attacks will induce the "dies" message, or
 * various "specialized" messages.  Note that "You have destroyed"
 * and "is destroyed" are synonyms for "You have slain" and "dies".
 *
 * Hack -- unseen monsters yield "You have killed it." message.
 *
 * Added fear (DGK) and check whether to print fear messages -CWS
 *
 * Genericized name, sex, and capitilization -BEN-
 *
 * As always, the "ghost" processing is a total hack.
 *
 * Hack -- we "delay" fear messages by passing around a "fear" flag.
 *
 * XXX XXX XXX Consider decreasing monster experience over time, say,
 * by using "(m_exp * m_lev * (m_lev)) / (p_lev * (m_lev + n_killed))"
 * instead of simply "(m_exp * m_lev) / (p_lev)", to make the first
 * monster worth more than subsequent monsters.  This would also need
 * to induce changes in the monster recall code.
 */
bool mon_take_hit(int Ind, int m_idx, int dam, bool *fear, cptr note)
{
	player_type *p_ptr = Players[Ind];

	monster_type	*m_ptr = &m_list[m_idx];

        monster_race    *r_ptr = race_inf(m_ptr);

	s32b		new_exp, new_exp_frac;


	/* Redraw (later) if needed */
	update_health(m_idx);

	/* Wake it up */
	m_ptr->csleep = 0;

	/* Hurt it */
	m_ptr->hp -= dam;

	/* It is dead now */
	if (m_ptr->hp < 0)
	{
		char m_name[80];

		/* Extract monster name */
		monster_desc(Ind, m_name, m_idx, 0);

		/* Make a sound */
		sound(Ind, SOUND_KILL);

		/* Death by Missile/Spell attack */
		if (note)
		{
			msg_format_near(Ind, "\377y%^s%s", m_name, note);
			msg_format(Ind, "\377y%^s%s", m_name, note);
		}

		/* Death by physical attack -- invisible monster */
		else if (!p_ptr->mon_vis[m_idx])
		{
			msg_format_near(Ind, "\377y%s has killed %s.", p_ptr->name, m_name);
			msg_format(Ind, "\377yYou have killed %s.", m_name);
		}

		/* Death by Physical attack -- non-living monster */
		else if ((r_ptr->flags3 & RF3_DEMON) ||
		         (r_ptr->flags3 & RF3_UNDEAD) ||
		         (r_ptr->flags2 & RF2_STUPID) ||
		         (strchr("Evg", r_ptr->d_char)))
		{
			msg_format_near(Ind, "\377y%s has destroyed %s.", p_ptr->name, m_name);
			msg_format(Ind, "\377yYou have destroyed %s.", m_name);
		}

		/* Death by Physical attack -- living monster */
		else
		{
			msg_format_near(Ind, "\377y%s has slain %s.", p_ptr->name, m_name);
			msg_format(Ind, "\377yYou have slain %s.", m_name);
		}

		/* Check if it's cloned unique */
		if ((r_ptr->flags1 & RF1_UNIQUE) && p_ptr->r_killed[m_ptr->r_idx])
		{
			m_ptr->clone = TRUE;
		}

		/* Split experience if in a party */
		if (p_ptr->party == 0)
		{
			/* Give some experience */
			new_exp = ((long)r_ptr->mexp * r_ptr->level) / p_ptr->lev;
			new_exp_frac = ((((long)r_ptr->mexp * r_ptr->level) % p_ptr->lev)
					* 0x10000L / p_ptr->lev) + p_ptr->exp_frac;

			/* Keep track of experience */
			if (new_exp_frac >= 0x10000L)
			{
				new_exp++;
				p_ptr->exp_frac = new_exp_frac - 0x10000L;
			}
			else
			{
				p_ptr->exp_frac = new_exp_frac;
			}

			/* Gain experience */
			if(!m_ptr->clone)
				gain_exp(Ind, new_exp);
		}
		else
		{
			/* Give experience to that party */
			if (!player_is_king(Ind) && !m_ptr->clone) party_gain_exp(Ind, p_ptr->party, (long)r_ptr->mexp * r_ptr->level);
		}

		/* Generate treasure */
		if(!m_ptr->clone) monster_death(Ind, m_idx);

		/* When the player kills a Unique, it stays dead */
		/* No more, this is handled byt p_ptr->r_killed -- DG */
//		if (r_ptr->flags1 & RF1_UNIQUE) r_ptr->max_num = 0;
//		p_ptr->r_killed[m_ptr->r_idx] = TRUE;

		/* Recall even invisible uniques or winners */
		if (p_ptr->mon_vis[m_idx] || (r_ptr->flags1 & RF1_UNIQUE))
		{
			/* Count kills this life */
			if (r_ptr->r_pkills < MAX_SHORT) r_ptr->r_pkills++;

			/* Count kills in all lives */
			if (r_ptr->r_tkills < MAX_SHORT) r_ptr->r_tkills++;

			/* Hack -- Auto-recall */
			recent_track(m_ptr->r_idx);
		}

		/* Delete the monster */
		delete_monster_idx(m_idx);

		/* Not afraid */
		(*fear) = FALSE;

		/* Monster is dead */
		return (TRUE);
	}


#ifdef ALLOW_FEAR

	/* Mega-Hack -- Pain cancels fear */
	if (m_ptr->monfear && (dam > 0))
	{
		int tmp = randint(dam);

		/* Cure a little fear */
		if (tmp < m_ptr->monfear)
		{
			/* Reduce fear */
			m_ptr->monfear -= tmp;
		}

		/* Cure all the fear */
		else
		{
			/* Cure fear */
			m_ptr->monfear = 0;

			/* No more fear */
			(*fear) = FALSE;
		}
	}

	/* Sometimes a monster gets scared by damage */
	if (!m_ptr->monfear && !(r_ptr->flags3 & RF3_NO_FEAR))
	{
		int		percentage;

		/* Percentage of fully healthy */
		percentage = (100L * m_ptr->hp) / m_ptr->maxhp;

		/*
		 * Run (sometimes) if at 10% or less of max hit points,
		 * or (usually) when hit for half its current hit points
		 */
		if (((percentage <= 10) && (rand_int(10) < percentage)) ||
		    ((dam >= m_ptr->hp) && (rand_int(100) < 80)))
		{
			/* Hack -- note fear */
			(*fear) = TRUE;

			/* XXX XXX XXX Hack -- Add some timed fear */
			m_ptr->monfear = (randint(10) +
			                  (((dam >= m_ptr->hp) && (percentage > 7)) ?
			                   20 : ((11 - percentage) * 5)));
		}
	}

#endif

	/* Not dead yet */
	return (FALSE);
}

void monster_death_mon(int am_idx, int m_idx)
{
        monster_type *am_ptr = &m_list[am_idx];

	int			i, j, y, x, ny, nx;

	int			dump_item = 0;
	int			dump_gold = 0;

	int			number = 0;
	int			total = 0;

	char buf[160];

	cave_type		*c_ptr;

	monster_type	*m_ptr = &m_list[m_idx];

        monster_race *r_ptr = race_inf(m_ptr);

	bool good = (r_ptr->flags1 & RF1_DROP_GOOD) ? TRUE : FALSE;
	bool great = (r_ptr->flags1 & RF1_DROP_GREAT) ? TRUE : FALSE;

	bool do_gold = (!(r_ptr->flags1 & RF1_ONLY_ITEM));
	bool do_item = (!(r_ptr->flags1 & RF1_ONLY_GOLD));

	int force_coin = get_coin_type(r_ptr);
#ifdef NEW_DUNGEON
	struct worldpos *wpos;
	cave_type **zcave;
#else
	int Depth;
#endif

	/* Get the location */
	y = m_ptr->fy;
	x = m_ptr->fx;
#ifdef NEW_DUNGEON
	wpos=&m_ptr->wpos;
	if(!(zcave=getcave(wpos))) return;
#else
	Depth = m_ptr->dun_depth;
#endif

	/* Determine how much we can drop */
	if ((r_ptr->flags1 & RF1_DROP_60) && (rand_int(100) < 60)) number++;
	if ((r_ptr->flags1 & RF1_DROP_90) && (rand_int(100) < 90)) number++;
	if (r_ptr->flags1 & RF1_DROP_1D2) number += damroll(1, 2);
	if (r_ptr->flags1 & RF1_DROP_2D2) number += damroll(2, 2);
	if (r_ptr->flags1 & RF1_DROP_3D2) number += damroll(3, 2);
	if (r_ptr->flags1 & RF1_DROP_4D2) number += damroll(4, 2);

	/* Drop some objects */
	for (j = 0; j < number; j++)
	{
		/* Try 20 times per item, increasing range */
		for (i = 0; i < 20; ++i)
		{
			int d = (i + 14) / 15;

			/* Pick a "correct" location */
#ifdef NEW_DUNGEON
			scatter(wpos, &ny, &nx, y, x, d, 0);
#else
			scatter(Depth, &ny, &nx, y, x, d, 0);
#endif

			/* Must be "clean" floor grid */
#ifdef NEW_DUNGEON
			if (!cave_clean_bold(zcave, ny, nx)) continue;

			/* Access the grid */
			c_ptr = &zcave[ny][nx];
#else
			if (!cave_clean_bold(Depth, ny, nx)) continue;

			/* Access the grid */
			c_ptr = &cave[Depth][ny][nx];
#endif

			/* Hack -- handle creeping coins */
			coin_type = force_coin;

			/* Average dungeon and monster levels */
#ifdef NEW_DUNGEON
			object_level = (getlevel(wpos) + r_ptr->level) / 2;
#else
			object_level = (Depth + r_ptr->level) / 2;
#endif

			/* Place Gold */
			if (do_gold && (!do_item || (rand_int(100) < 50)))
			{
#ifdef NEW_DUNGEON
				place_gold(wpos, ny, nx);
#else
				place_gold(Depth, ny, nx);
#endif
			}

			/* Place Object */
			else
			{
#ifdef NEW_DUNGEON
				place_object(wpos, ny, nx, good, great);
#else
				place_object(Depth, ny, nx, good, great);
#endif
			}

			/* Reset the object level */
#ifdef NEW_DUNGEON
			object_level = getlevel(wpos);
#else
			object_level = Depth;
#endif

			/* Reset "coin" type */
			coin_type = 0;

#ifdef NEW_DUNGEON
			/* Notice */
			note_spot_depth(wpos, ny, nx);

			/* Display */
			everyone_lite_spot(wpos, ny, nx);
#else
			/* Notice */
			note_spot_depth(Depth, ny, nx);

			/* Display */
			everyone_lite_spot(Depth, ny, nx);
#endif

			/* Under a player */
			if (c_ptr->m_idx < 0)
			{
				msg_print(0 - c_ptr->m_idx, "You feel something roll beneath your feet.");
			}

			break;
		}
	}

        FREE(m_ptr->r_ptr, monster_race);
}

bool mon_take_hit_mon(int am_idx, int m_idx, int dam, bool *fear, cptr note)
{
        monster_type *am_ptr = &m_list[am_idx];

	monster_type	*m_ptr = &m_list[m_idx];

        monster_race    *r_ptr = race_inf(m_ptr);

	s32b		new_exp, new_exp_frac;


	/* Redraw (later) if needed */
	update_health(m_idx);

	/* Wake it up */
	m_ptr->csleep = 0;

	/* Hurt it */
	m_ptr->hp -= dam;

        /* Cannot kill uniques */
        if ((r_ptr->flags1 & RF1_UNIQUE) && (m_ptr->hp < 1)) m_ptr->hp = 1;

	/* It is dead now */
	if (m_ptr->hp < 0)
	{
                /* Give some experience */
                new_exp = ((long)r_ptr->mexp * r_ptr->level) / am_ptr->level;

                /* Gain experience */
                if(!m_ptr->clone)
                        monster_gain_exp(am_idx, new_exp, TRUE);

		/* Generate treasure */
                if (!m_ptr->clone) monster_death_mon(am_idx, m_idx);

		/* Delete the monster */
		delete_monster_idx(m_idx);

		/* Not afraid */
		(*fear) = FALSE;

		/* Monster is dead */
		return (TRUE);
	}


#ifdef ALLOW_FEAR

	/* Mega-Hack -- Pain cancels fear */
	if (m_ptr->monfear && (dam > 0))
	{
		int tmp = randint(dam);

		/* Cure a little fear */
		if (tmp < m_ptr->monfear)
		{
			/* Reduce fear */
			m_ptr->monfear -= tmp;
		}

		/* Cure all the fear */
		else
		{
			/* Cure fear */
			m_ptr->monfear = 0;

			/* No more fear */
			(*fear) = FALSE;
		}
	}

	/* Sometimes a monster gets scared by damage */
	if (!m_ptr->monfear && !(r_ptr->flags3 & RF3_NO_FEAR))
	{
		int		percentage;

		/* Percentage of fully healthy */
		percentage = (100L * m_ptr->hp) / m_ptr->maxhp;

		/*
		 * Run (sometimes) if at 10% or less of max hit points,
		 * or (usually) when hit for half its current hit points
		 */
		if (((percentage <= 10) && (rand_int(10) < percentage)) ||
		    ((dam >= m_ptr->hp) && (rand_int(100) < 80)))
		{
			/* Hack -- note fear */
			(*fear) = TRUE;

			/* XXX XXX XXX Hack -- Add some timed fear */
			m_ptr->monfear = (randint(10) +
			                  (((dam >= m_ptr->hp) && (percentage > 7)) ?
			                   20 : ((11 - percentage) * 5)));
		}
	}

#endif

	/* Not dead yet */
	return (FALSE);
}



/*
 * Calculates current boundaries
 * Called below and from "do_cmd_locate()".
 */
void panel_bounds(int Ind)
{
	player_type *p_ptr = Players[Ind];

	p_ptr->panel_row_min = p_ptr->panel_row * (SCREEN_HGT / 2);
	p_ptr->panel_row_max = p_ptr->panel_row_min + SCREEN_HGT - 1;
	p_ptr->panel_row_prt = p_ptr->panel_row_min - 1;
	p_ptr->panel_col_min = p_ptr->panel_col * (SCREEN_WID / 2);
	p_ptr->panel_col_max = p_ptr->panel_col_min + SCREEN_WID - 1;
	p_ptr->panel_col_prt = p_ptr->panel_col_min - 13;
}



/*
 * Given an row (y) and col (x), this routine detects when a move
 * off the screen has occurred and figures new borders. -RAK-
 *
 * "Update" forces a "full update" to take place.
 *
 * The map is reprinted if necessary, and "TRUE" is returned.
 */
void verify_panel(int Ind)
{
	player_type *p_ptr = Players[Ind];

	int y = p_ptr->py;
	int x = p_ptr->px;

	int prow = p_ptr->panel_row;
	int pcol = p_ptr->panel_col;

	/* Scroll screen when 2 grids from top/bottom edge */
	if ((y < p_ptr->panel_row_min + 2) || (y > p_ptr->panel_row_max - 2))
	{
		prow = ((y - SCREEN_HGT / 4) / (SCREEN_HGT / 2));
		if (prow > p_ptr->max_panel_rows) prow = p_ptr->max_panel_rows;
		else if (prow < 0) prow = 0;
	}

	/* Scroll screen when 4 grids from left/right edge */
	if ((x < p_ptr->panel_col_min + 4) || (x > p_ptr->panel_col_max - 4))
	{
		pcol = ((x - SCREEN_WID / 4) / (SCREEN_WID / 2));
		if (pcol > p_ptr->max_panel_cols) pcol = p_ptr->max_panel_cols;
		else if (pcol < 0) pcol = 0;
	}

	/* Check for "no change" */
	if ((prow == p_ptr->panel_row) && (pcol == p_ptr->panel_col)) return;

	/* Hack -- optional disturb on "panel change" */
	if (p_ptr->disturb_panel) disturb(Ind, 0, 0);

	/* Save the new panel info */
	p_ptr->panel_row = prow;
	p_ptr->panel_col = pcol;

	/* Recalculate the boundaries */
	panel_bounds(Ind);

	/* Update stuff */
	p_ptr->update |= (PU_MONSTERS);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);
}



/*
 * Monster health description
 */
cptr look_mon_desc(int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
        monster_race *r_ptr = race_inf(m_ptr);

	bool          living = TRUE;
	int           perc;


	/* Determine if the monster is "living" (vs "undead") */
	if (r_ptr->flags3 & RF3_UNDEAD) living = FALSE;
	if (r_ptr->flags3 & RF3_DEMON) living = FALSE;
	if (strchr("Egv", r_ptr->d_char)) living = FALSE;


	/* Healthy monsters */
	if (m_ptr->hp >= m_ptr->maxhp)
	{
		/* No damage */
		return (living ? "unhurt" : "undamaged");
	}


	/* Calculate a health "percentage" */
	perc = 100L * m_ptr->hp / m_ptr->maxhp;

	if (perc >= 60)
	{
		return (living ? "somewhat wounded" : "somewhat damaged");
	}

	if (perc >= 25)
	{
		return (living ? "wounded" : "damaged");
	}

	if (perc >= 10)
	{
		return (living ? "badly wounded" : "badly damaged");
	}

	return (living ? "almost dead" : "almost destroyed");
}



/*
 * Angband sorting algorithm -- quick sort in place
 *
 * Note that the details of the data we are sorting is hidden,
 * and we rely on the "ang_sort_comp()" and "ang_sort_swap()"
 * function hooks to interact with the data, which is given as
 * two pointers, and which may have any user-defined form.
 */
void ang_sort_aux(int Ind, vptr u, vptr v, int p, int q)
{
	int z, a, b;

	/* Done sort */
	if (p >= q) return;

	/* Pivot */
	z = p;

	/* Begin */
	a = p;
	b = q;

	/* Partition */
	while (TRUE)
	{
		/* Slide i2 */
		while (!(*ang_sort_comp)(Ind, u, v, b, z)) b--;

		/* Slide i1 */
		while (!(*ang_sort_comp)(Ind, u, v, z, a)) a++;

		/* Done partition */
		if (a >= b) break;

		/* Swap */
		(*ang_sort_swap)(Ind, u, v, a, b);

		/* Advance */
		a++, b--;
	}

	/* Recurse left side */
	ang_sort_aux(Ind, u, v, p, b);

	/* Recurse right side */
	ang_sort_aux(Ind, u, v, b+1, q);
}


/*
 * Angband sorting algorithm -- quick sort in place
 *
 * Note that the details of the data we are sorting is hidden,
 * and we rely on the "ang_sort_comp()" and "ang_sort_swap()"
 * function hooks to interact with the data, which is given as
 * two pointers, and which may have any user-defined form.
 */
void ang_sort(int Ind, vptr u, vptr v, int n)
{
	/* Sort the array */
	ang_sort_aux(Ind, u, v, 0, n-1);
}


/* returns our max times 100 divided by our current...*/
int player_wounded(s16b ind)
{
	player_type *p_ptr = Players[ind];
	
	return (p_ptr->mhp * 100) / p_ptr->chp;
}

/* this should probably be somewhere more logical, but I should probably be
sleeping right now.....
Selects the most wounded target.

Hmm, I am sure there are faster sort algorithms out there... oh well, I don't 
think it really matters... this one goes out to you Mr. Munroe.
-ADA-
*/

void wounded_player_target_sort(int Ind, vptr sx, vptr sy, vptr id, int n)
{
	int c,num;
	s16b swp;
	s16b * idx = (s16b *) id;
	byte * x = (byte *) sx;
	byte * y = (byte *) sy; 
	byte swpb;
	
	/* num equals our max index */
	num = n-1;
	
	while (num > 0)
	{
		for (c = 0; c < num; c++)
		{
			if (player_wounded(idx[c+1]) > player_wounded(idx[c]))
			{
				swp = idx[c];
				idx[c] = idx[c+1];
				idx[c+1] = swp;
				
				swpb = x[c];
				x[c] = x[c+1];
				x[c+1] = swpb;
				
				swpb = y[c];
				y[c] = y[c+1];
				y[c+1] = swpb;
			}
		}
	num--;
	}
}



/*
 * Sorting hook -- comp function -- by "distance to player"
 *
 * We use "u" and "v" to point to arrays of "x" and "y" positions,
 * and sort the arrays by double-distance to the player.
 */
bool ang_sort_comp_distance(int Ind, vptr u, vptr v, int a, int b)
{
	player_type *p_ptr = Players[Ind];

	byte *x = (byte*)(u);
	byte *y = (byte*)(v);

	int da, db, kx, ky;

	/* Absolute distance components */
	kx = x[a]; kx -= p_ptr->px; kx = ABS(kx);
	ky = y[a]; ky -= p_ptr->py; ky = ABS(ky);

	/* Approximate Double Distance to the first point */
	da = ((kx > ky) ? (kx + kx + ky) : (ky + ky + kx));

	/* Absolute distance components */
	kx = x[b]; kx -= p_ptr->px; kx = ABS(kx);
	ky = y[b]; ky -= p_ptr->py; ky = ABS(ky);

	/* Approximate Double Distance to the first point */
	db = ((kx > ky) ? (kx + kx + ky) : (ky + ky + kx));

	/* Compare the distances */
	return (da <= db);
}


/*
 * Sorting hook -- swap function -- by "distance to player"
 *
 * We use "u" and "v" to point to arrays of "x" and "y" positions,
 * and sort the arrays by distance to the player.
 */
void ang_sort_swap_distance(int Ind, vptr u, vptr v, int a, int b)
{
	byte *x = (byte*)(u);
	byte *y = (byte*)(v);

	byte temp;

	/* Swap "x" */
	temp = x[a];
	x[a] = x[b];
	x[b] = temp;

	/* Swap "y" */
	temp = y[a];
	y[a] = y[b];
	y[b] = temp;
}



/*
 * Compare the values of two objects.
 *
 * Pointer "v" should not point to anything (it isn't used, anyway).
 */
bool ang_sort_comp_value(int Ind, vptr u, vptr v, int a, int b)
{
	object_type *inven = (object_type *)u;
	s32b va, vb;

	if (inven[a].tval && inven[b].tval)
	{
		va = object_value(Ind, &inven[a]);
		vb = object_value(Ind, &inven[b]);

		return (va >= vb);
	}

	if (inven[a].tval)
		return FALSE;

	return TRUE;
}


void ang_sort_swap_value(int Ind, vptr u, vptr v, int a, int b)
{
	object_type *x = (object_type *)u;
	object_type temp;

	temp = x[a];
	x[a] = x[b];
	x[b] = temp;
}

	

/*** Targetting Code ***/


/*
 * Determine is a monster makes a reasonable target
 *
 * The concept of "targetting" was stolen from "Morgul" (?)
 *
 * The player can target any location, or any "target-able" monster.
 *
 * Currently, a monster is "target_able" if it is visible, and if
 * the player can hit it with a projection, and the player is not
 * hallucinating.  This allows use of "use closest target" macros.
 *
 * Future versions may restrict the ability to target "trappers"
 * and "mimics", but the semantics is a little bit weird.
 */
bool target_able(int Ind, int m_idx)
{
	player_type *p_ptr = Players[Ind], *q_ptr;

	monster_type *m_ptr;

	if(!p_ptr) return FALSE;

	/* Check for OK monster */
	if (m_idx > 0)
	{
		/* Acquire pointer */
		m_ptr = &m_list[m_idx];

		/* Monster must be visible */
		if (!p_ptr->mon_vis[m_idx]) return (FALSE);

                /* Monster must not be owned */
                if (p_ptr->id == m_ptr->owner) return (FALSE);

		/* Monster must be projectable */
#ifdef NEW_DUNGEON
		if (!projectable(&p_ptr->wpos, p_ptr->py, p_ptr->px, m_ptr->fy, m_ptr->fx)) return (FALSE);
#else
		if (!projectable(p_ptr->dun_depth, p_ptr->py, p_ptr->px, m_ptr->fy, m_ptr->fx)) return (FALSE);
#endif

		if(m_ptr->owner==p_ptr->id) return(FALSE);

		/* Hack -- no targeting hallucinations */
		if (p_ptr->image) return (FALSE);

		/* XXX XXX XXX Hack -- Never target trappers */
		/* if (CLEAR_ATTR && CLEAR_CHAR) return (FALSE); */

		/* Assume okay */
		return (TRUE);
	}

	/* Check for OK player */
	if (m_idx < 0)
	{
		/* Acquire pointer */
		q_ptr = Players[0 - m_idx];

		/* Paranoia check -- require a valid player */
		if ((0 - m_idx) >= MAX_PLAYERS) return (FALSE);
		if (!q_ptr) return (FALSE);

		/* Players must be on same depth */
#ifdef NEW_DUNGEON
		if (inarea(&p_ptr->wpos, &q_ptr->wpos)) return (FALSE);
#else
		if (p_ptr->dun_depth != q_ptr->dun_depth) return (FALSE);
#endif

		/* Player must be visible */
		if (!player_can_see_bold(Ind, q_ptr->py, q_ptr->px)) return (FALSE);

		/* Player must be projectable */
#ifdef NEW_DUNGEON
		if (!projectable(&p_ptr->wpos, p_ptr->py, p_ptr->px, q_ptr->py, q_ptr->px)) return (FALSE);
#else
		if (!projectable(p_ptr->dun_depth, p_ptr->py, p_ptr->px, q_ptr->py, q_ptr->px)) return (FALSE);
#endif

		/* Hack -- no targetting hallucinations */
		if (p_ptr->image) return (FALSE);

		/* Assume okay */
		return (TRUE);
	}

	/* Assume no target */
	return (FALSE);
}




/*
 * Update (if necessary) and verify (if possible) the target.
 *
 * We return TRUE if the target is "okay" and FALSE otherwise.
 */
bool target_okay(int Ind)
{
	player_type *p_ptr = Players[Ind];

	/* Accept stationary targets */
	if (p_ptr->target_who > MAX_M_IDX) return (TRUE);

	/* Check moving monsters */
	if (p_ptr->target_who > 0)
	{
		/* Accept reasonable targets */
		if (target_able(Ind, p_ptr->target_who))
		{
			monster_type *m_ptr = &m_list[p_ptr->target_who];

			/* Acquire monster location */
			p_ptr->target_row = m_ptr->fy;
			p_ptr->target_col = m_ptr->fx;

			/* Good target */
			return (TRUE);
		}
	}

	/* Check moving players */
	if (p_ptr->target_who < 0)
	{
		/* Accept reasonable targets */
		if (target_able(Ind, p_ptr->target_who))
		{
			player_type *q_ptr = Players[0 - p_ptr->target_who];

			/* Acquire player location */
			p_ptr->target_row = q_ptr->py;
			p_ptr->target_col = q_ptr->px;

			/* Good target */
			return (TRUE);
		}
	}

	/* Assume no target */
	return (FALSE);
}



/*
 * Hack -- help "select" a location (see below)
 */
s16b target_pick(int Ind, int y1, int x1, int dy, int dx)
{
	player_type *p_ptr = Players[Ind];

	int i, v;

	int x2, y2, x3, y3, x4, y4;

	int b_i = -1, b_v = 9999;


	/* Scan the locations */
	for (i = 0; i < p_ptr->target_n; i++)
	{
		/* Point 2 */
		x2 = p_ptr->target_x[i];
		y2 = p_ptr->target_y[i];

		/* Directed distance */
		x3 = (x2 - x1);
		y3 = (y2 - y1);

		/* Verify quadrant */
		if (dx && (x3 * dx <= 0)) continue;
		if (dy && (y3 * dy <= 0)) continue;

		/* Absolute distance */
		x4 = ABS(x3);
		y4 = ABS(y3);

		/* Verify quadrant */
		if (dy && !dx && (x4 > y4)) continue;
		if (dx && !dy && (y4 > x4)) continue;

		/* Approximate Double Distance */
		v = ((x4 > y4) ? (x4 + x4 + y4) : (y4 + y4 + x4));

		/* XXX XXX XXX Penalize location */

		/* Track best */
		if ((b_i >= 0) && (v >= b_v)) continue;

		/* Track best */
		b_i = i; b_v = v;
	}

	/* Result */
	return (b_i);
}


/*
 * Set a new target.  This code can be called from "get_aim_dir()"
 *
 * The target must be on the current panel.  Consider the use of
 * "panel_bounds()" to allow "off-panel" targets, perhaps by using
 * some form of "scrolling" the map around the cursor.  XXX XXX XXX
 *
 * That is, consider the possibility of "auto-scrolling" the screen
 * while the cursor moves around.  This may require changes in the
 * "update_mon()" code to allow "visibility" even if off panel.
 *
 * Hack -- targetting an "outer border grid" may be dangerous,
 * so this is not currently allowed.
 *
 * You can now use the direction keys to move among legal monsters,
 * just like the new "look" function allows the use of direction
 * keys to move amongst interesting locations.
 */
bool target_set(int Ind, int dir)
{
	player_type *p_ptr = Players[Ind], *q_ptr;
#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
#else
	int Depth = p_ptr->dun_depth;
#endif

	int		i, m, idx;

	int		y;
	int		x;

	bool	flag = TRUE;

	char	out_val[160];

	cave_type		*c_ptr;

	monster_type	*m_ptr;
	monster_race	*r_ptr;

#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(wpos))) return(FALSE);
#endif
	if (!dir)
	{
		x = p_ptr->px;
		y = p_ptr->py;

		/* Go ahead and turn off target mode */
		p_ptr->target_who = 0;

		/* Turn off health tracking */
		health_track(Ind, 0);


		/* Reset "target" array */
		p_ptr->target_n = 0;

		/* Collect "target-able" monsters */
		for (i = 1; i < m_max; i++)
		{
			monster_type *m_ptr = &m_list[i];

			/* Skip "dead" monsters */
			if (!m_ptr->r_idx) continue;

			/* Skip monsters not on this depth */
#ifdef NEW_DUNGEON
			if (!inarea(&m_ptr->wpos, wpos)) continue;
#else
			if (m_ptr->dun_depth != Depth) continue;
#endif

			/* Ignore "unreasonable" monsters */
			if (!target_able(Ind, i)) continue;

			/* Save this monster index */
			p_ptr->target_x[p_ptr->target_n] = m_ptr->fx;
			p_ptr->target_y[p_ptr->target_n] = m_ptr->fy;
			p_ptr->target_n++;
		}

		/* Collect "target-able" players */
		for (i = 1; i <= NumPlayers; i++)
		{
			/* Acquire pointer */
			q_ptr = Players[i];

			/* Don't target yourself */
			if (i == Ind) continue;

			/* Skip unconnected players */
			if (q_ptr->conn == NOT_CONNECTED) continue;

			/* Ignore players we aren't hostile to */
			if (!check_hostile(Ind, i)) continue;

			/* Ignore "unreasonable" players */
			if (!target_able(Ind, 0 - i)) continue;

			/* Save the player index */
			p_ptr->target_x[p_ptr->target_n] = q_ptr->px;
			p_ptr->target_y[p_ptr->target_n] = q_ptr->py;
			p_ptr->target_n++;
		}

		/* Set the sort hooks */
		ang_sort_comp = ang_sort_comp_distance;
		ang_sort_swap = ang_sort_swap_distance;

		/* Sort the positions */
		ang_sort(Ind, p_ptr->target_x, p_ptr->target_y, p_ptr->target_n);

		/* Collect indices */
		for (i = 0; i < p_ptr->target_n; i++)
		{
#ifdef NEW_DUNGEON
			cave_type *c_ptr = &zcave[p_ptr->target_y[i]][p_ptr->target_x[i]];
#else
			cave_type *c_ptr = &cave[Depth][p_ptr->target_y[i]][p_ptr->target_x[i]];
#endif

			p_ptr->target_idx[i] = c_ptr->m_idx;
		}
			
		/* Start near the player */
		m = 0;
	}
	else if (dir >= 128)
	{
		/* Initialize if needed */
		if (dir == 128)
		{
			p_ptr->target_col = p_ptr->px;
			p_ptr->target_row = p_ptr->py;
		}
		else
		{
			p_ptr->target_row += ddy[dir - 128];
			p_ptr->target_col += ddx[dir - 128];
		}

		/* Info */
		strcpy(out_val, "[<dir>, q] ");

		/* Tell the client */
		Send_target_info(Ind, p_ptr->target_col - p_ptr->panel_col_prt, p_ptr->target_row - p_ptr->panel_row_prt, out_val);

		/* Check for completion */
		if (dir == 128 + 5)
		{
			p_ptr->target_who = MAX_M_IDX + 1;
			return TRUE;
		}

		/* Done */
		return FALSE;
	}
	else
	{
		/* Start where we last left off */
		m = p_ptr->look_index;

		/* Reset the locations */
		for (i = 0; i < p_ptr->target_n; i++)
		{
			if (p_ptr->target_idx[i] > 0)
			{
				m_ptr = &m_list[p_ptr->target_idx[i]];

				p_ptr->target_y[i] = m_ptr->fy;
				p_ptr->target_x[i] = m_ptr->fx;
			}
			else if (p_ptr->target_idx[i] < 0)
			{
				q_ptr = Players[0 - p_ptr->target_idx[i]];

				p_ptr->target_y[i] = q_ptr->py;
				p_ptr->target_x[i] = q_ptr->px;
			}
		}

		/* Find a new monster */
		i = target_pick(Ind, p_ptr->target_y[m], p_ptr->target_x[m], ddy[dir], ddx[dir]);

		/* Use that monster */
		if (i > 0) m = i;
	}

	/* Target monsters */
	if (flag && p_ptr->target_n && p_ptr->target_idx[m] > 0)
	{
		y = p_ptr->target_y[m];
		x = p_ptr->target_x[m];
		idx = p_ptr->target_idx[m];

#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x];
#else
		c_ptr = &cave[Depth][y][x];
#endif

		m_ptr = &m_list[idx];
                r_ptr = race_inf(m_ptr);

		/* Hack -- Track that monster race */
		recent_track(m_ptr->r_idx);

		/* Hack -- Track that monster */
		health_track(Ind, idx);

		/* Hack -- handle stuff */
		handle_stuff(Ind);

		/* Describe, prompt for recall */
		sprintf(out_val,
                        "%s{%d} (%s) [<dir>, q, t] ",
                        r_name_get(m_ptr),
                        m_ptr->level,
			look_mon_desc(idx));

		/* Tell the client about it */
		Send_target_info(Ind, x - p_ptr->panel_col_prt, y - p_ptr->panel_row_prt, out_val);
	}
	else if (flag && p_ptr->target_n && p_ptr->target_idx[m] < 0)
	{
		y = p_ptr->target_y[m];
		x = p_ptr->target_x[m];
		idx = p_ptr->target_idx[m];

#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x];
#else
		c_ptr = &cave[Depth][y][x];
#endif

		q_ptr = Players[0 - idx];

		/* Hack -- Track that player */
		health_track(Ind, idx);

		/* Hack -- handle stuff */
		handle_stuff(Ind);

		/* Describe */
		sprintf(out_val, "%s [<dir>, q, t] ", q_ptr->name);

		/* Tell the client about it */
		Send_target_info(Ind, x - p_ptr->panel_col_prt, y - p_ptr->panel_row_prt, out_val);
	}

	/* Remember current index */
	p_ptr->look_index = m;

	/* Set target */
	if (dir == 5)
	{
		p_ptr->target_who = p_ptr->target_idx[m];
		p_ptr->target_col = p_ptr->target_x[m];
		p_ptr->target_row = p_ptr->target_y[m];

		/* Track */
		if (p_ptr->target_who) health_track(Ind, p_ptr->target_who);
	}

	/* Failure */
	if (!p_ptr->target_who) return (FALSE);

	/* Clear target info */
	p_ptr->target_n = 0;

	/* Success */
	return (TRUE);
}

/* targets the most wounded teammate. should be useful for stuff like
 * heal other and teleport macros. -ADA-
 *
 * Now this function can take 3rd arg which specifies which player to
 * set the target.
 * This part was written by Ascrep(DEG); thx for his courtesy!
 * */

bool target_set_friendly(int Ind, int dir, ...)
{
	va_list ap;
	player_type *p_ptr = Players[Ind], *q_ptr;

#ifdef NEW_DUNGEON
	struct worldpos *wpos=&p_ptr->wpos;
	cave_type **zcave;
#else
	int Depth = p_ptr->dun_depth;
#endif

	int		i, m, castplayer, idx;

	int		y;
	int		x;

	bool	flag = TRUE;

	char	out_val[160];

	cave_type		*c_ptr;

	monster_type	*m_ptr;
	monster_race	*r_ptr;
#ifdef NEW_DUNGEON
	if(!(zcave=getcave(wpos))) return(FALSE);
#endif

		va_start(ap,dir);
		castplayer = va_arg(ap,int);
		va_end(ap);
		
		x = p_ptr->px;
		y = p_ptr->py;

		/* Go ahead and turn off target mode */
		p_ptr->target_who = 0;

		/* Turn off health tracking */
		health_track(Ind, 0);


		/* Reset "target" array */
		p_ptr->target_n = 0;

		if (!((castplayer > 0) && (castplayer < 20)))
		{
		/* Collect "target-able" players */
		for (i = 1; i <= NumPlayers; i++)
		{
			/* Acquire pointer */
			q_ptr = Players[i];

			/* Don't target yourself */
			if (i == Ind) continue;

			/* Skip unconnected players */
			if (q_ptr->conn == NOT_CONNECTED) continue;

			/* Ignore players we aren't friends with */
			if (check_hostile(Ind, i)) continue;

			/* if we are in party, only help members */
			if (p_ptr->party && (!player_in_party(p_ptr->party, i))) continue;

			/* Ignore "unreasonable" players */
			if (!target_able(Ind, 0 - i)) continue;

			/* Save the player index */
			p_ptr->target_x[p_ptr->target_n] = q_ptr->px;
			p_ptr->target_y[p_ptr->target_n] = q_ptr->py;
			p_ptr->target_idx[p_ptr->target_n] = i;
			p_ptr->target_n++;
		}
		}
		else
		{
			/* Acquire pointer */
			q_ptr = Players[castplayer];

			/* Skip unconnected players */
			if (q_ptr->conn == NOT_CONNECTED) return;

			/* Ignore "unreasonable" players */
			if (!target_able(Ind, 0 - castplayer)) return;

			/* Save the player index */
			p_ptr->target_x[p_ptr->target_n] = q_ptr->px;
			p_ptr->target_y[p_ptr->target_n] = q_ptr->py;
			p_ptr->target_idx[p_ptr->target_n] = castplayer;
			p_ptr->target_n++;
		}
		
			
		/* Set the sort hooks */ 
		ang_sort_comp = ang_sort_comp_distance;
		ang_sort_swap = ang_sort_swap_distance;

		/* Sort the positions */
		wounded_player_target_sort(Ind, p_ptr->target_x, p_ptr->target_y, p_ptr->target_idx, p_ptr->target_n);

		m = 0;

	/* too lazy to handle dirs right now */
	
	/* handle player target.... */	
	if (p_ptr->target_n)
	{
		y = p_ptr->target_y[m];
		x = p_ptr->target_x[m];
		idx = p_ptr->target_idx[m];

#ifdef NEW_DUNGEON
		c_ptr = &zcave[y][x];
#else
		c_ptr = &cave[Depth][y][x];
#endif

		q_ptr = Players[idx];

		/* Hack -- Track that player */
		health_track(Ind, 0 - idx);

		/* Hack -- handle stuff */
		handle_stuff(Ind);

		/* Describe */
		sprintf(out_val, "%s targetted.", q_ptr->name);

		/* Tell the client about it */
		Send_target_info(Ind, x - p_ptr->panel_col_prt, y - p_ptr->panel_row_prt, out_val);
	}

	/* Remember current index */
	p_ptr->look_index = m;

	p_ptr->target_who = 0 - p_ptr->target_idx[m];
	p_ptr->target_col = p_ptr->target_x[m];
	p_ptr->target_row = p_ptr->target_y[m];	

	/* Failure */
	if (!p_ptr->target_who) return (FALSE);

	/* Clear target info */
	p_ptr->target_n = 0;

	/* Success */
	return (TRUE);
}



/*
 * Get an "aiming direction" from the user.
 *
 * The "dir" is loaded with 1,2,3,4,6,7,8,9 for "actual direction", and
 * "0" for "current target", and "-1" for "entry aborted".
 *
 * Note that "Force Target", if set, will pre-empt user interaction,
 * if there is a usable target already set.
 *
 * Note that confusion over-rides any (explicit?) user choice.
 *
 * We just ask the client to send us a direction, unless we are confused --KLJ--
 */
bool get_aim_dir(int Ind)
{
	int		dir;
	player_type *p_ptr = Players[Ind];


	/* Global direction */
	dir = command_dir;

	/* Hack -- auto-target if requested */
	if (p_ptr->use_old_target && target_okay(Ind)) 
	{
		dir = 5;
		
		/* XXX XXX Pretend we read this direction from the network */
		Handle_direction(Ind, dir);
		return (TRUE);
	}

	Send_direction(Ind);

	return (TRUE);
}


bool get_item(int Ind)
{
	Send_item_request(Ind);

	return (TRUE);
}


/*
 * Request a "movement" direction (1,2,3,4,6,7,8,9) from the user,
 * and place it into "command_dir", unless we already have one.
 *
 * This function should be used for all "repeatable" commands, such as
 * run, walk, open, close, bash, disarm, spike, tunnel, etc.
 *
 * This function tracks and uses the "global direction", and uses
 * that as the "desired direction", to which "confusion" is applied.
 */
#if 0
bool get_rep_dir(int *dp)
{
	int dir;


	/* Global direction */
	dir = command_dir;

	/* Get a direction */
	while (!dir)
	{
		char ch;

		/* Get a command (or Cancel) */
		if (!get_com("Direction (Escape to cancel)? ", &ch)) break;

		/* Look up the direction */
		dir = keymap_dirs[ch & 0x7F];

		/* Oops */
		if (!dir) bell();
	}

	/* Keep the given direction */
	*dp = dir;

	/* Aborted */
	if (!dir) return (FALSE);

	/* Save the direction */
	command_dir = dir;

	/* Apply "confusion" */
	if (p_ptr->confused)
	{
		/* Warn the user XXX XXX XXX */
		/* msg_print("You are confused."); */

		/* Standard confusion */
		if (rand_int(100) < 75)
		{
			/* Random direction */
			*dp = ddd[rand_int(8)];
		}
	}

	/* A "valid" direction was entered */
	return (TRUE);
}
#endif

#ifdef NEW_DUNGEON
/*
 * Allows to travel both vertical/horizontal using Recall;
 * probably wilderness(horizontal) travel will be made by other means
 * in the future.
 *
 * Also, player_type doesn't contain the max.depth for each dungeon...
 * Currently, this function uses getlevel() to determine the max.depth
 * for each dungeon, but this should be replaced by actual depthes
 * a player has been.	- Jir -
 */
void set_recall_depth(player_type * p_ptr, object_type * o_ptr)
{
//	int recall_depth = 0;
//	worldpos goal;
	
	unsigned char * inscription = (unsigned char *) quark_str(o_ptr->note);
	
	/* default to the players maximum depth */
	p_ptr->recall_pos.wx = p_ptr->wpos.wx;
	p_ptr->recall_pos.wy = p_ptr->wpos.wy;
	p_ptr->recall_pos.wz = (wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags &
			WILD_F_DOWN) ? 0 - p_ptr->max_dlv : p_ptr->max_dlv;
#if 0
	p_ptr->recall_pos.wz = (wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags &
			WILD_F_DOWN) ? 0 - p_ptr->max_dlv :
			((wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags & WILD_F_UP) ?
			 p_ptr->max_dlv : 0);

	goal.wx = p_ptr->wpos.wx;
	goal.wy = p_ptr->wpos.wy;
//	goal.wz = 0 - p_ptr->max_dlv;	// hack -- default to 'dungeon'
#endif	// 0

	/* check for a valid inscription */
	if (inscription == NULL) return;
	
	/* scan the inscription for @R */
	while (*inscription != '\0')
	{
		
		if (*inscription == '@')
		{
			inscription++;
			
			/* a valid @R has been located */
			if (*inscription == 'R')
			{			
				inscription++;
				/* @RW for World(Wilderness) travel */
				/* It would be also amusing to limit the distance.. */
				if ((*inscription == 'W') || (*inscription == 'X'))
				{
					unsigned char * next;
					inscription++;
					p_ptr->recall_pos.wx = atoi(inscription) % MAX_WILD_X;
					p_ptr->recall_pos.wz = 0;
					next = strchr(inscription,',');
					if (next)
					{
						if (++next) p_ptr->recall_pos.wy = atoi(next) % MAX_WILD_Y;
					}
				}
				else if (*inscription == 'Y')
				{
					inscription++;
					p_ptr->recall_pos.wy = atoi(inscription) % MAX_WILD_Y;
					p_ptr->recall_pos.wz = 0;
				}
#if 1
				/* @RT for inter-Town travels (not fully implemented yet) */
				else if (*inscription == 'T')
				{
					inscription++;
					p_ptr->recall_pos.wz = 0;
				}
#endif
				else
				{
					if (*inscription == 'Z') inscription++;

					/* convert the inscription into a level index */
					p_ptr->recall_pos.wz = atoi(inscription) / 50;
				}
			}
		}
		inscription++;
	}
#if 0	/* sanity checks are done when recalling */	
	/* do some bounds checking / sanity checks */
	if ((recall_depth > p_ptr->max_dlv) || (!recall_depth)) recall_depth = p_ptr->max_dlv;
	
	/* if a wilderness level, verify that the player has visited here before */
	if (recall_depth < 0)
	{
		/* if the player has not visited here, set the recall depth to the town */
		if (!(p_ptr->wild_map[-recall_depth/8] & (1 << -recall_depth%8))) 		
			recall_depth = 1;
	}
	
	p_ptr->recall_depth = recall_depth;
	wpcopy(&p_ptr->recall_pos, &goal);
#endif
}

#else
void set_recall_depth(player_type * p_ptr, object_type * o_ptr)
{
	int recall_depth = 0;
	
	unsigned char * inscription = (unsigned char *) quark_str(o_ptr->note);
	
	/* default to the players maximum depth */
	p_ptr->recall_depth = p_ptr->max_dlv;
	
	/* check for a valid inscription */
	if (inscription == NULL) return;
	
	/* scan the inscription for @R */
	while (*inscription != '\0')
	{
		
		if (*inscription == '@')
		{
			inscription++;
			
			/* a valid @R has been located */
			if (*inscription == 'R')
			{			
				inscription++;
				/* convert the inscription into a level index */
				recall_depth = atoi(inscription) / 50;
			}
		}
		inscription++;
	}
	
	/* do some bounds checking / sanity checks */
	if ((recall_depth > p_ptr->max_dlv) || (!recall_depth)) recall_depth = p_ptr->max_dlv;
	
	/* if a wilderness level, verify that the player has visited here before */
	if (recall_depth < 0)
	{
		/* if the player has not visited here, set the recall depth to the town */
		if (!(p_ptr->wild_map[-recall_depth/8] & (1 << -recall_depth%8))) 		
			recall_depth = 1;
	}
	
	p_ptr->recall_depth = recall_depth;
}
#endif NEW_DUNGEON

void telekinesis_aux(int Ind, int item)
{
  player_type *p_ptr = Players[Ind], *p2_ptr;
  object_type *q_ptr, *o_ptr = p_ptr->current_telekinesis;
//bool ok = FALSE;
  int Ind2;

//	unsigned char * inscription = (unsigned char *) quark_str(o_ptr->note);

	p_ptr->current_telekinesis = NULL;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		q_ptr = &p_ptr->inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
	  msg_print(Ind, "You must carry the object to teleport it.");
	  return;
	}

	Ind2 = get_player(Ind, o_ptr);

	if (!Ind2) return;

#if 0	
	/* check for a valid inscription */
	if (inscription == NULL)
	  {
	    msg_print(Ind, "Nobody to send to.");
	    return;
	  }
	
	/* scan the inscription for @P */
	while ((*inscription != '\0') && !ok)
	{
		
		if (*inscription == '@')
		{
			inscription++;
			
			/* a valid @P has been located */
			if (*inscription == 'P')
			{			
				inscription++;
				
//				Ind2 = find_player_name(inscription);
				Ind2 = name_lookup_loose(Ind, inscription, FALSE);
				if (Ind2) ok = TRUE;
			}
		}
		inscription++;
	}
	
        if (!ok)
	  {
//	    msg_print(Ind, "Player is not on.");
	    msg_print(Ind, "Could not find the recipient.");
	    return;
	  }
#endif

	p2_ptr = Players[Ind2];

	/* You cannot send artifact */
	if(cfg_anti_arts_horde && (artifact_p(q_ptr)) && (!q_ptr->name3))
	{
		msg_print(Ind, "You have an acute feeling of loss!");
		a_info[q_ptr->name1].cur_num = 0;
	}
	else
	{
		/* Actually teleport the object to the player inventory */
		inven_carry(Ind2, q_ptr);

		/* Combine the pack */
		p2_ptr->notice |= (PN_COMBINE);

		/* Window stuff */
		p2_ptr->window |= (PW_INVEN | PW_EQUIP);

		msg_format(Ind2, "You are hit by a powerfull magic wave from %s.", p_ptr->name);
	}

	/* Wipe it */
	inven_item_increase(Ind, item, -99);
	inven_item_describe(Ind, item);
	inven_item_optimize(Ind, item);

	/* Combine the pack */
	p_ptr->notice |= (PN_COMBINE);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP);

}

int get_player(int Ind, object_type *o_ptr)
{
        player_type *p_ptr = Players[Ind];
        bool ok = FALSE;
        int Ind2;

	unsigned char * inscription = (unsigned char *) quark_str(o_ptr->note);

       	/* check for a valid inscription */
	if (inscription == NULL)
	  {
//	    msg_print(Ind, "Nobody to use the power with.");
	    msg_print(Ind, "No target player specified.");
	    return 0;
	  }
	
	/* scan the inscription for @P */
	while ((*inscription != '\0') && !ok)
	{
		
		if (*inscription == '@')
		{
			inscription++;
			
			/* a valid @R has been located */
			if (*inscription == 'P')
			{			
				inscription++;
				
//				Ind2 = find_player_name(inscription);
				Ind2 = name_lookup_loose(Ind, inscription, FALSE);
				if (Ind2) ok = TRUE;
			}
		}
		inscription++;
	}
	
        if (!ok)
	  {
	    msg_print(Ind, "Couldn't find the target.");
	    return 0;
	  }

	if (Ind == Ind2)
	{
		msg_print(Ind, "You cannot do that on yourself.");
		return 0;
	}

	return Ind2;
}

void blood_bond(int Ind, object_type *o_ptr)
{
        player_type *p_ptr = Players[Ind], *p2_ptr;
//      bool ok = FALSE;
        int Ind2;

	Ind2 = get_player(Ind, o_ptr);
	if (!Ind2) return;

#if 0
	unsigned char * inscription = (unsigned char *) quark_str(o_ptr->note);

       	/* check for a valid inscription */
	if (inscription == NULL)
	  {
	    msg_print(Ind, "Nobody to blood bond with.");
	    return;
	  }
	
	/* scan the inscription for @P */
	while ((*inscription != '\0') && !ok)
	{
		
		if (*inscription == '@')
		{
			inscription++;
			
			/* a valid @R has been located */
			if (*inscription == 'P')
			{			
				inscription++;
				
				Ind2 = find_player_name(inscription);
				if (Ind2) ok = TRUE;
			}
		}
		inscription++;
	}
	
        if (!ok)
	  {
	    msg_print(Ind, "Player is not on.");
	    return;
	  }
#endif

	p2_ptr = Players[Ind2];

	p_ptr->blood_bond = p2_ptr->id;
	p2_ptr->blood_bond = p_ptr->id;

	msg_format(Ind, "You blood bond with %s.", p2_ptr->name);
	msg_format(Ind2, "%s blood bonds with you.", p_ptr->name);
	msg_broadcast(Ind, format("%s blood bonds with %s.", p_ptr->name, p2_ptr->name));
}

bool telekinesis(int Ind, object_type *o_ptr)
{
  player_type *p_ptr = Players[Ind];

  p_ptr->current_telekinesis = o_ptr;
  get_item(Ind);

  return TRUE;
}

/* this has finally earned its own function, to make it easy for restoration to do this also */
bool do_scroll_life(int Ind)
{
	int x,y;
	
	player_type * p_ptr = Players[Ind];
	cave_type * c_ptr;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	zcave=getcave(&p_ptr->wpos);
	if(!zcave) return(FALSE);
#endif
	
	for (y = -1; y <= 1; y++)
	{
		for (x = -1; x <= 1; x++)
	 	{
#ifdef NEW_DUNGEON
	   		c_ptr = &zcave[p_ptr->py+y][p_ptr->px+x];
	  		if ((c_ptr->m_idx < 0) && (cave_floor_bold(zcave, p_ptr->py+y, p_ptr->px+x)) && (!(c_ptr->info & CAVE_ICKY)))
#else
	   		c_ptr = &cave[p_ptr->dun_depth][p_ptr->py+y][p_ptr->px+x];
	  		if ((c_ptr->m_idx < 0) && (cave_floor_bold(p_ptr->dun_depth, p_ptr->py+y, p_ptr->px+x)) && (!(c_ptr->info & CAVE_ICKY)))
#endif
	
	   		{
   				if (Players[0 - c_ptr->m_idx]->ghost)
   				{
    					resurrect_player(0 - c_ptr->m_idx);
   			        	return TRUE;
      				}
  			} 
  		}
  	}  	
  	/* we did nore ressurect anyone */
  	return FALSE; 
  }


/* modified above function to instead restore XP... used in priest spell rememberence */
bool do_restoreXP_other(int Ind)
{
	int x,y;
	
	player_type * p_ptr = Players[Ind];
	cave_type * c_ptr;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(&p_ptr->wpos))) return(FALSE);
#endif
	
	for (y = -1; y <= 1; y++)
	{
		for (x = -1; x <= 1; x++)
	 	{
#ifdef NEW_DUNGEON
	   		c_ptr = &zcave[p_ptr->py+y][p_ptr->px+x];
#else
	   		c_ptr = &cave[p_ptr->dun_depth][p_ptr->py+y][p_ptr->px+x];
#endif
	
	  		if (c_ptr->m_idx < 0)
	   		{
   				if (Players[0 - c_ptr->m_idx]->exp < Players[0 - c_ptr->m_idx]->max_exp)
   				{
    					restore_level(0 - c_ptr->m_idx);
   			        	return TRUE;
      				}
  			} 
  		}
  	}  	
  	/* we did nore ressurect anyone */
  	return FALSE; 
  }
  

/* Hack -- since the framerate has been boosted by five times since version
 * 0.6.0 to make game movement more smooth, we return the old level speed
 * times five to keep the same movement rate.
 */

#ifdef NEW_DUNGEON
/* ok so its a hack - lets get it working first */
int level_speed(struct worldpos *wpos){
	if(!wpos->wz){
		return(level_speeds[0]*5);
	}
	else{
		return (level_speeds[getlevel(wpos)]*5);
	}
}
#else
int level_speed(int Ind)
{
	if ( Ind <= 0) return level_speeds[0]*5;
	else return level_speeds[Ind]*5;
}
#endif

void unstatic_level(struct worldpos *wpos){
	int i;

	for (i = 1; i <= NumPlayers; i++)
	{
		if (Players[i]->conn == NOT_CONNECTED) continue;
		if (Players[i]->st_anchor){
			Players[i]->st_anchor=0;
			msg_print(GetInd[Players[i]->id],"Your space/time anchor breaks\n");
		}
	}
	for (i = 1; i <= NumPlayers; i++){
		if (Players[i]->conn == NOT_CONNECTED) continue;
		if (inarea(&Players[i]->wpos, wpos)){
			teleport_player_level(i);
		}
	}
	new_players_on_depth(wpos,0,FALSE);
}

/* these Dungeon Master commands should probably be added somewhere else, but I am
 * hacking them together here to start.
 */

/* static or unstatic a level */
bool master_level(int Ind, char * parms)
{
	int i;
	/* get the player pointer */
	player_type *p_ptr = Players[Ind];
	
	if (strcmp(p_ptr->name, cfg_dungeon_master)) return FALSE;

	switch (parms[0])
	{
		/* unstatic the level */
		case 'u':
		{
#ifdef NEW_DUNGEON
			struct worldpos twpos;
			wpcopy(&twpos,&p_ptr->wpos);
			unstatic_level(&twpos);
#else
			int depth = p_ptr->dun_depth;
#endif
       			msg_print(Ind, "The level has been unstaticed.");
			break;
		}

		/* static the level */
		case 's':
		{
			/* Increase the number of players on the dungeon 
			 * masters level by one. */
#ifdef NEW_DUNGEON
			new_players_on_depth(&p_ptr->wpos,1,TRUE);
#else
			players_on_depth[p_ptr->dun_depth]++;
#endif
			msg_print(Ind, "The level has been staticed.");
			break;
		}
#ifdef NEW_DUNGEON
		/* add dungeon stairs here */
		case 'D':
		{
			cave_type **zcave;
			if(!parms[1] || !parms[2] || p_ptr->wpos.wz) return FALSE;
			if(istown(&p_ptr->wpos)){
				msg_print(Ind,"Even you may not create dungeons in the town!");
				return FALSE;
			}
			if(parms[3]=='t' && !(wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags&WILD_F_UP)){
				adddungeon(&p_ptr->wpos, parms[1], parms[2], DUNGEON_RANDOM|DUNGEON_NOMAP, TRUE);
				new_level_down_y(&p_ptr->wpos, p_ptr->py);
				new_level_down_x(&p_ptr->wpos, p_ptr->px);
				if((zcave=getcave(&p_ptr->wpos))){
					zcave[p_ptr->py][p_ptr->px].feat=FEAT_LESS;
				}
			}
			if(parms[3]=='d' && !(wild_info[p_ptr->wpos.wy][p_ptr->wpos.wx].flags&WILD_F_DOWN)){
				adddungeon(&p_ptr->wpos, parms[1], parms[2], DUNGEON_RANDOM, FALSE);
				new_level_up_y(&p_ptr->wpos, p_ptr->py);
				new_level_up_x(&p_ptr->wpos, p_ptr->px);
				if((zcave=getcave(&p_ptr->wpos))){
					zcave[p_ptr->py][p_ptr->px].feat=FEAT_MORE;
				}
			}
			break;
		}
		case 'T':
		{
			struct worldpos twpos;
			if(!parms[1] || p_ptr->wpos.wz) return FALSE;
			if(istown(&p_ptr->wpos)){
				msg_print(Ind, "There is already a town here!");
				return FALSE;
			}
			wpcopy(&twpos,&p_ptr->wpos);

			/* clean level first! */
			wipe_m_list(&p_ptr->wpos);
			wipe_o_list(&p_ptr->wpos);

			/* dont do this where there are houses! */
			for(i=0;i<num_houses;i++){
				if(inarea(&p_ptr->wpos, &houses[i].wpos)){
					houses[i].flags|=HF_DELETED;
				}
			}
			addtown(p_ptr->wpos.wy, p_ptr->wpos.wx, parms[1], 0);
			unstatic_level(&twpos);
			if(getcave(&twpos))
				dealloc_dungeon_level(&twpos);

			break;
		}
#endif
		/* default -- do nothing. */
		default: break;
	}
	return TRUE;
}

/* static or unstatic a level (from chat-line command) */
#ifdef NEW_DUNGEON
bool master_level_specific(int Ind, struct worldpos *wpos, char * parms)
#else
bool master_level_specific(int Ind, int depth, char * parms)
#endif
{
	int i;
	/* get the player pointer */
	player_type *p_ptr = Players[Ind];
	
//	if (strcmp(p_ptr->name, cfg_dungeon_master)) return FALSE;

	switch (parms[0])
	{
		/* unstatic the level */
		case 'u':
		{
			unstatic_level(wpos);
//       			msg_format(Ind, "The level (%d,%d) %dft has been unstaticed.", wpos->wx, wpos->wy, wpos->wz*50);
       			msg_format(Ind, "The level %s has been unstaticed.", wpos_format(wpos));
			break;
		}

		/* static the level */
		case 's':
		{
			/* Increase the number of players on the dungeon 
			 * masters level by one. */
#ifdef NEW_DUNGEON
			new_players_on_depth(&p_ptr->wpos,1,TRUE);
#else
			players_on_depth[p_ptr->dun_depth]++;
#endif
			msg_print(Ind, "The level has been staticed.");
			break;
		}
		/* default -- do nothing. */
		default: break;
	}
	return TRUE;
}
/* Build walls and such.  This should probably be improved, I am just hacking
 * it together right now for Halloween. -APD
 */
bool master_build(int Ind, char * parms)
{
	player_type * p_ptr = Players[Ind];
	cave_type * c_ptr;
	static unsigned char new_feat = FEAT_WALL_EXTRA;
#ifdef NEW_DUNGEON
	cave_type **zcave;
	if(!(zcave=getcave(&p_ptr->wpos))) return(FALSE);
#endif

	if (strcmp(p_ptr->name, cfg_dungeon_master) && (!player_is_king(Ind))) return FALSE;
	
	/* extract arguments, otherwise build a wall of type new_feat */
	if (parms)
	{
		/* Hack -- the first character specifies the type of wall */
		new_feat = parms[0];
		/* Hack -- toggle auto-build on/off */
		switch (parms[1])
		{
			case 'T': p_ptr->master_move_hook = master_build; break;
			case 'F': p_ptr->master_move_hook = NULL; break;
			default : break;
		}
	}

	/* paranoia -- make sure the player is on a valid level */
#ifndef NEW_DUNGEON
	if (!cave[p_ptr->dun_depth]) return FALSE;
	c_ptr = &cave[p_ptr->dun_depth][p_ptr->py][p_ptr->px];
#else
	c_ptr = &zcave[p_ptr->py][p_ptr->px];
#endif
	
	/* build a wall of type new_feat at the player's location */
	if(c_ptr->special.type){
		switch(c_ptr->special.type){
			case KEY_DOOR:
				KILL(c_ptr->special.ptr, struct key_type);
				break;
			case DNA_DOOR:	/* even DM must not kill houses like this */
			default:
				return FALSE;
		}
	}
	c_ptr->feat = new_feat;
	if(c_ptr->feat>=FEAT_HOME_HEAD && c_ptr->feat<=FEAT_HOME_TAIL){
		/* new special door creation (with keys) */
		struct key_type *key;
		object_type newkey;
		int id;
		MAKE(key, struct key_type);
		sscanf(&parms[2],"%d",&key->id);
		invcopy(&newkey, lookup_kind(TV_KEY, 1));
		newkey.pval=key->id;
#ifdef NEW_DUNGEON
		drop_near(&newkey, -1, &p_ptr->wpos, p_ptr->py, p_ptr->px);
#else
		drop_near(&newkey, -1, p_ptr->dun_depth, p_ptr->py, p_ptr->px);
#endif
		c_ptr->special.type=KEY_DOOR;
		c_ptr->special.ptr=key;
		p_ptr->master_move_hook=NULL;	/*buggers up if not*/
	}

	return TRUE;
}

static char master_specific_race_char = 'a';

bool master_summon_specific_aux(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* no uniques */
	if (r_ptr->flags1 &RF1_UNIQUE) return FALSE;

	/* if we look like what we are looking for */
	if (r_ptr->d_char == master_specific_race_char) return TRUE;
	return FALSE;
}

/* Auxillary function to master_summon, determine the exact type of monster
 * to summon from a more general description.
 */
u16b master_summon_aux_monster_type( char monster_type, char * monster_parms)
{
	int tmp;
	/* handle each category of monster types */
	switch (monster_type)
	{
		/* specific monster specified */
		case 's': 
		{
			/* allows specification by monster No. */
			tmp = atoi(monster_parms);
			if (tmp > 0) return tmp;

			/* if the name was specified, summon this exact race */
			if (strlen(monster_parms) > 1) return race_index(monster_parms);
			/* otherwise, summon a monster that looks like us */
			else
			{
				master_specific_race_char = monster_parms[0];
				get_mon_num_hook = master_summon_specific_aux;
				get_mon_num_prep();
				tmp = get_mon_num(rand_int(100) + 10);

				/* restore monster generator */
				get_mon_num_hook = NULL;
				get_mon_num_prep();

				/* return our monster */
				return tmp;
			}
		}
		/* orc specified */
		case 'o':  
		{
			/* if not random, assume specific orc specified */
			if (strcmp(monster_parms, "random")) return race_index(monster_parms);
			/* random orc */
			else switch(rand_int(6))
			{
				case 0: return race_index("Snaga");
				case 1: return race_index("Cave orc");
				case 2: return race_index("Hill orc");
				case 3: return race_index("Dark orc");
				case 4: return race_index("Half-orc");
				case 5: return race_index("Uruk");
			}
			break;
		}
		/* low undead specified */
		case 'u':  
		{
			/* if not random, assume specific high undead specified */
			if (strcmp(monster_parms, "random")) return race_index(monster_parms);
			/* random low undead */
			else switch(rand_int(11))
			{
				case 0: return race_index("Poltergeist");
				case 1: return race_index("Green glutton ghost");
				case 2: return race_index("Lost soul");
				case 3: return race_index("Skeleton kobold");
				case 4: return race_index("Skeleton orc");
				case 5: return race_index("Skeleton human");
				case 6: return race_index("Zombified orc");
				case 7: return race_index("Zombified human");
				case 8: return race_index("Mummified orc");
				case 9: return race_index("Moaning spirit");
				case 10: return race_index("Vampire bat");
			}
			break;
		}
		
		/* high undead specified */
		case 'U':  
		{
			/* if not random, assume specific high undead specified */
			if (strcmp(monster_parms, "random")) return race_index(monster_parms);
			/* random low undead */
			else switch(rand_int(13))
			{
				case 0: return race_index("Vampire");
				case 1: return race_index("Giant skeleton troll");
				case 2: return race_index("Lich");
				case 3: return race_index("Master vampire");
				case 4: return race_index("Dread");
				case 5: return race_index("Nether wraith");
				case 6: return race_index("Night mare");
				case 7: return race_index("Vampire lord");
				case 8: return race_index("Archpriest");
				case 9: return race_index("Undead beholder");
				case 10: return race_index("Dreadmaster");
				case 11: return race_index("Nightwing");
				case 12: return race_index("Nightcrawler");
			}
			break;
		}

		/* specific depth specified */
		case 'd':
		{
			return get_mon_num(monster_parms[0]);
		}

		default : break;
	}

	/* failure */
	return 0;

}

/* Temporary debugging hack, to test the new excellents.
 */
bool master_acquire(int Ind, char * parms)
{
	player_type * p_ptr = Players[Ind];
	
	if (strcmp(p_ptr->name, cfg_dungeon_master)) return FALSE;
#ifdef NEW_DUNGEON
	acquirement(&p_ptr->wpos, p_ptr->py, p_ptr->px, 1, TRUE);
#else
	acquirement(p_ptr->dun_depth, p_ptr->py, p_ptr->px, 1, TRUE);
#endif
	return TRUE;
}

/* Monster summoning options. More documentation on this later. */
bool master_summon(int Ind, char * parms)
{
	int c;
	player_type * p_ptr = Players[Ind];
	cave_type * c_ptr;

	static char monster_type = 0;  /* What type of monster we are -- specific, random orc, etc */
	static char monster_parms[80];
	static char summon_type = 0; /* what kind to summon -- x right here, group at random location, etc */
	static char summon_parms = 0; /* arguments to previous byte */
	static u16b r_idx = 0; /* which monser to actually summon, from previous variables */
	unsigned char size = 0;  /* how many monsters to actually summon */

	if (strcmp(p_ptr->name, cfg_dungeon_master) && (!player_is_king(Ind))) return FALSE;
	
	/* extract arguments.  If none are found, summon previous type. */
	if (parms)
	{
		/* the first character specifies the type of monster */
		summon_type = parms[0];
		summon_parms = parms[1];
		monster_type = parms[2];
		/* Hack -- since monster_parms is a string, throw it on the end */
		strcpy(monster_parms, &parms[3]);
	}
	
	switch (summon_type)
	{
		/* summon x here */
		case 'x':
		{
			/* for each monster we are summoning */
			for (c = 0; c < summon_parms; c++)
			{
				/* hack -- monster_type '0' specifies mass genocide */
				if (monster_type == '0')
				{
					mass_genocide(Ind);
					break;
				}

				/* figure out who to summon */
				r_idx = master_summon_aux_monster_type(monster_type, monster_parms);

				/* summon the monster, if we have a valid one */
				if (r_idx)
#ifdef NEW_DUNGEON
					summon_specific_race(&p_ptr->wpos, p_ptr->py, p_ptr->px, r_idx, 1);
#else
					summon_specific_race(p_ptr->dun_depth, p_ptr->py, p_ptr->px, r_idx, 1);
#endif
			}
			break;
		}

		/* summon x at random locations */
		case 'X':
		{
			for (c = 0; c < summon_parms; c++)
			{
				/* figure out who to summon */
				r_idx = master_summon_aux_monster_type(monster_type, monster_parms);
				/* summon the monster at a random location */
				if (r_idx)
#ifdef NEW_DUNGEON
					summon_specific_race_somewhere(&p_ptr->wpos,r_idx, 1);
#else
					summon_specific_race_somewhere(p_ptr->dun_depth,r_idx, 1);
#endif
			}
			break;
		}

		/* summon group of random size here */
		case 'g':
		{
			/* figure out how many to summon */
			size = rand_int(rand_int(50)) + 2;
			/* figure out who to summon */
			r_idx = master_summon_aux_monster_type(monster_type, monster_parms);
			/* summon the group here */
#ifdef NEW_DUNGEON
			summon_specific_race(&p_ptr->wpos, p_ptr->py, p_ptr->px, r_idx, size);
#else
			summon_specific_race(p_ptr->dun_depth, p_ptr->py, p_ptr->px, r_idx, size);
#endif
			break;
		}
		/* summon group of random size at random location */
		case 'G':
		{
			/* figure out how many to summon */
			size = rand_int(rand_int(50)) + 2;
			/* figure out who to summon */
			r_idx = master_summon_aux_monster_type(monster_type, monster_parms);
			/* someone the group at a random location */
#ifdef NEW_DUNGEON
			summon_specific_race_somewhere(&p_ptr->wpos, r_idx, size);
#else
			summon_specific_race_somewhere(p_ptr->dun_depth, r_idx, size);
#endif
			break;
		}
		/* summon mode on (use with discretion... lets not be TOO mean ;-) )*/
		case 'T':
		{	
			summon_type = 'x';
			summon_parms = 1;
			
			p_ptr->master_move_hook = master_summon;
			break;
		}

		/* summon mode off */
		case 'F':
		{
			p_ptr->master_move_hook = NULL;
			break;
		}

		/* Wipe monsters from level (DM only) */
		case 'Q':
		{
#ifdef NEW_DUNGEON
#else
			if(!strcmp(p_ptr->name, cfg_dungeon_master))
				wipe_m_list((int)summon_parms);
#endif
			break;
		}
	}

	return TRUE;
}

void player_edit(char *name){
	
}

bool master_player(int Ind, char *parms){
	player_type *p_ptr=Players[Ind];
	player_type *q_ptr;
	int Ind2=0;
	int i;

	if(strcmp(p_ptr->name, cfg_dungeon_master)){
		msg_print(Ind,"You need to be the dungeon master to use this command.");
		return FALSE;
	}
	switch(parms[0]){
		case 'E':	/* offline editor */
			for(i=1;i<=NumPlayers;i++){
				if(!strcmp(Players[i]->name,&parms[1])){
					msg_format(Ind,"%s is currently playing",&parms[1]);
					return(FALSE);
				}
			}
			player_edit(&parms[1]);

			break;
		case 'A':	/* acquirement */
#if 0
			for(i=1;i<=NumPlayers;i++){
				if(!strcmp(Players[i]->name,&parms[1])){
					Ind2=i;
					break;
				}
			}
#endif
			Ind2 = name_lookup_loose(Ind, &parms[1], FALSE);
			if(Ind2)
			{
				player_type *p_ptr2 = Players[Ind2];
#ifdef NEW_DUNGEON
				acquirement(&p_ptr2->wpos, p_ptr2->py, p_ptr2->px, 1, TRUE);
#else
				acquirement(p_ptr2->dun_depth, p_ptr2->py, p_ptr2->px, 1, TRUE);
#endif
				msg_format(Ind, "%s is granted an item.", p_ptr2->name);
				msg_format(Ind2, "You feel a divine favor!");
				return(FALSE);
			}
//			msg_print(Ind, "That player is not in the game.");
			break;
		case 'k':	/* admin wrath */
#if 0
			for(i=1;i<=NumPlayers;i++){
				if(!strcmp(Players[i]->name,&parms[1])){
					Ind2=i;
					break;
				}
			}
#endif
			Ind2 = name_lookup_loose(Ind, &parms[1], FALSE);
			if(Ind2){
				q_ptr=Players[Ind2];
				msg_print(Ind2, "You are hit by a bolt from the blue!");
				strcpy(q_ptr->died_from,"divine wrath");
				//q_ptr->alive=FALSE;
				player_death(Ind2);
				return(TRUE);
			}
//			msg_print(Ind, "That player is not in the game.");

			break;
		case 'S':	/* Static a regular */
			stat_player(&parms[1], TRUE);
			break;
			
		case 'U':	/* Unstatic him */
			stat_player(&parms[1], FALSE);
			break;
		case 't':	/* DM telekinesis */
			/* I needed this before - it is useful */
			/* Unfortunately the current telekinesis */
			/* is not compatible with it, and I do not */
			/* want to combine it while there is a */
			/* potential bug. */
			break;
		case 'B':
			/* This could be fun - be wise dungeon master */
			msg_broadcast(Ind,&parms[1]);
	}
	return(FALSE);
}

vault_type *get_vault(char *name)
{
	int i;
	
	for(i=0; i<MAX_V_IDX; i++)
	{
		if(strstr(v_name + v_info[i].name, name))
			return &v_info[i];
	}

	return NULL;
}

/* Generate something */
bool master_generate(int Ind, char * parms)
{
	/* get the player pointer */
	player_type *p_ptr = Players[Ind];
	
	if (strcmp(p_ptr->name, cfg_dungeon_master)) return FALSE;

	switch (parms[0])
	{
		/* generate a vault */
		case 'v':
		{
			vault_type *v_ptr = NULL;
			
			switch(parms[1])
			{
				case '#':
					v_ptr = &v_info[parms[2] + 127];
					break;
				case 'n':
					v_ptr = get_vault(&parms[2]);
			}
			
			if(!v_ptr || !v_ptr->wid) return FALSE;

#ifdef NEW_DUNGEON
			build_vault(&p_ptr->wpos, p_ptr->py, p_ptr->px, v_ptr->hgt, v_ptr->wid, v_text + v_ptr->text);
#else
			build_vault(p_ptr->dun_depth, p_ptr->py, p_ptr->px, v_ptr->hgt, v_ptr->wid, v_text + v_ptr->text);
#endif

			break;
		}
	}
	return TRUE;
}

