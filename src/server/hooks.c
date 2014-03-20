/* File: plots.c */

/* Purpose: plots & quests */

/*
 * Copyright (c) 2001 James E. Wilson, Robert A. Koeneke, DarkGod
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

/* added this for consistency in some (unrelated) header-inclusion,
   it IS a server file, isn't it? */
#define SERVER

#include "angband.h"

#include "lua.h"
#include "tolua.h"
extern lua_State* L;

bool check_hook(int h_idx);
void del_hook_name(int h_idx, cptr name);

/******** Hooks stuff *********/
FILE *hook_file;

#define MAX_ARGS        50

static hooks_chain *hooks_heads[MAX_HOOKS];

/* Wipe hooks and init them with quest hooks */
void wipe_hooks()
{
	int i;

	for (i = 0; i < MAX_HOOKS; i++)
	{
		hooks_heads[i] = NULL;
	}
}

void dump_hooks()
{
	int i;

	for (i = 0; i < MAX_HOOKS; i++)
	{
		hooks_chain *c = hooks_heads[i];

		/* Find it */
		while (c != NULL)
		{
			msg_admin("%s(%s)", c->name, "Lua");

			c = c->next;
		}
	}
}

/* Check a hook */
bool check_hook(int h_idx)
{
	hooks_chain *c = hooks_heads[h_idx];

	return (c != NULL);
}

/* Add a hook */
hooks_chain* add_hook(int h_idx, cptr script, cptr name)
{
	hooks_chain *new, *c = hooks_heads[h_idx];

	/* Find it */
	while ((c != NULL) && (strcmp(c->name, name)))
	{
		c = c->next;
	}

	/* If not already in the list, add it */
	if (c == NULL)
	{
		MAKE(new, hooks_chain);
		sprintf(new->name, "%s", name);
                sprintf(new->script, "%s", script);
		new->next = hooks_heads[h_idx];
		hooks_heads[h_idx] = new;
		return (new);
	}
	else return (c);
}

/* Remove a hook */
void del_hook_name(int h_idx, cptr name)
{
	hooks_chain *c = hooks_heads[h_idx], *p = NULL;

	/* Find it */
	while ((c != NULL) && (strcmp(c->name, name)))
	{
		p = c;
		c = c->next;
	}

	/* Remove it */
	if (c != NULL)
	{
		if (p == NULL)
		{
			hooks_heads[h_idx] = c->next;
			FREE(c, hooks_chain);
		}
		else
		{
			p->next = c->next;
			FREE(c, hooks_chain);
		}
	}
}

/* Actually process the hooks */
int process_hooks_restart = FALSE;
hook_return process_hooks_return[10];
static bool vprocess_hooks_return(int h_idx, char *ret, char *fmt, va_list *ap)
{
	hooks_chain *c = hooks_heads[h_idx];
	va_list real_ap;

	while (c != NULL)
	{
		{
			int i = 0, nb = 0, nbr = 1;
			int oldtop = lua_gettop(L), size;

			/* Push the function */
			lua_getglobal(L, c->script);

			/* Push and count the arguments */
			nb = 0;
			COPY(&real_ap, ap, va_list);
			while (fmt[i])
			{
				switch (fmt[i++])
				{
					case 'd':
					case 'l':
						tolua_pushnumber(L, va_arg(real_ap, s32b));
						nb++;
						break;
					case 's':
						tolua_pushstring(L, va_arg(real_ap, char*));
						nb++;
						break;
					case 'O':
						tolua_pushusertype(L, (void*)va_arg(real_ap, object_type*), tolua_tag(L, "object_type"));
						nb++;
						break;
					case '(':
					case ')':
					case ',':
						break;
				}
			}

			/* Count returns */
			nbr += strlen(ret);

			/* Call the function */
                        lua_call(L, nb, nbr);

                        /* Should be the same as nbr, but lets be carefull */
                        size = lua_gettop(L) - oldtop;

			/* get the extra returns if needed */
			for (i = 0; i < nbr - 1; i++)
			{
				if ((ret[i] == 'd') || (ret[i] == 'l'))
				{
                                        if (lua_isnumber(L, (-size) + 1 + i)) process_hooks_return[i].num = tolua_getnumber(L, (-size) + 1 + i, 0);
                                        else process_hooks_return[i].num = 0;
				}
				else if (ret[i] == 's')
				{
					if (lua_isstring(L, (-size) + 1 + i)) process_hooks_return[i].str = (char*)tolua_getstring(L, (-size) + 1 + i, 0);
					else process_hooks_return[i].str = NULL;
				}
				else process_hooks_return[i].num = 0;
			}
			/* Get the basic return(continue or stop the hook chain) */
			if (tolua_getnumber(L, -size, 0))
			{
				lua_settop(L, oldtop);
				return (TRUE);
			}
			if (process_hooks_restart)
			{
				c = hooks_heads[h_idx];
				process_hooks_restart = FALSE;
			}
			else
				c = c->next;
			lua_settop(L, oldtop);
		}
	}

	return FALSE;
}

#if 0 /* not used? - mikaelh */
static bool process_hooks_ret(int h_idx, char *ret, char *fmt, ...)
{
	va_list ap;
	bool r;

	va_start(ap, fmt);
	r = vprocess_hooks_return(h_idx, ret, fmt, &ap);
	va_end(ap);
	return (r);
}
#endif // 0

bool process_hooks(int h_idx, char *fmt, ...)
{
	va_list ap;
	bool ret;

	va_start(ap, fmt);
	ret = vprocess_hooks_return(h_idx, "", fmt, &ap);
	va_end(ap);
	return (ret);
}
