/* File: z-term.h */

#ifndef INCLUDED_Z_TERM_H
#define INCLUDED_Z_TERM_H

#include "../common/h-basic.h"



/*
 * A term_win is a "window" for a Term
 *
 *	- Cursor Useless/Visible codes
 *	- Cursor Location (see "Useless")
 *
 *	- Array[h] -- Access to the attribute array
 *	- Array[h] -- Access to the character array
 *
 *	- Array[h*w] -- Attribute array
 *	- Array[h*w] -- Character array
 *
 * Note that the attr/char pair at (x,y) is a[y][x]/c[y][x]
 * and that the row of attr/chars at (0,y) is a[y]/c[y]
 */

typedef struct term_win term_win;

struct term_win
{
	bool cu, cv;
	int cx, cy;

	byte **a;
	char32_t **c;

	byte *va;
	char32_t *vc;
};


/*
 * A key queue stores key presses.
 */

typedef struct key_queue key_queue;

struct key_queue
{
	char *queue;

	s32b head;
	s32b tail;
	s32b length;
	s32b size;
};


/*
 * An actual "term" structure
 *
 *	- Extra info (used by application)
 *
 *	- Extra data (used by implementation)
 *
 *
 *	- Flag "active_flag"
 *	  This "term" is "active"
 *
 *	- Flag "mapped_flag"
 *	  This "term" is "mapped"
 *
 *	- Flag "total_erase"
 *	  This "term" should be fully erased
 *
 *	- Flag "icky_corner"
 *	  This "term" has an "icky" corner grid
 *
 *	- Flag "soft_cursor"
 *	  This "term" uses a "software" cursor
 *
 *	- Flag "always_pict"
 *	  Use the "Term_pict()" routine for all text
 *
 *	- Flag "higher_pict"
 *	  Use the "Term_pict()" routine for special text
 *
 *	- Flag "always_text"
 *	  Use the "Term_text()" routine for invisible text
 *
 *	- Flag "never_bored"
 *	  Never call the "TERM_XTRA_BORED" action
 *
 *	- Flag "never_frosh"
 *	  Never call the "TERM_XTRA_FROSH" action
 *
 *	- Flag "no_total_erase_on_wipe"
 *	  Don't set total_erase to TRUE when Term_wipe() is called
 *
 *
 *	- Value "attr_blank"
 *	  Use this "attr" value for "blank" grids
 *
 *	- Value "char_blank"
 *	  Use this "char" value for "blank" grids
 *
 *
 *	- Ignore this pointer
 *
 *	- Keypress Queue -- various data
 *
 *	- Keypress Queue -- pending keys
 *
 *
 *	- Window Width (max 255)
 *	- Window Height (max 255)
 *
 *	- Minimum modified row
 *	- Maximum modified row
 *
 *	- Minimum modified column (per row)
 *	- Maximum modified column (per row)
 *
 *
 *	- Displayed screen image
 *	- Requested screen image
 *
 *	- Temporary screen image
 *	- Memorized screen image
 *
 *
 *	- Hook for init-ing the term
 *	- Hook for nuke-ing the term
 *
 *	- Hook for user actions
 *
 *	- Hook for extra actions
 *
 *	- Hook for placing the cursor
 *
 *	- Hook for drawing some blank spaces
 *
 *	- Hook for drawing a special character
 *
 *	- Hook for drawing a string of characters
 */

typedef struct term term;

struct term
{
	vptr user;

	vptr data;

	bool active_flag;
	bool mapped_flag;
	bool total_erase;
	bool icky_corner;
	bool soft_cursor;
	bool always_pict;
	bool higher_pict;
	bool always_text;
	bool never_bored;
	bool never_frosh;
	bool no_total_erase_on_wipe;

	byte attr_blank;
	char32_t char_blank;

	key_queue *keys;
	key_queue *keys_old;
	s32b key_size_orig;

	int wid;
	int hgt;

	int y1;
	int y2;

	int *x1;
	int *x2;

	term_win *old;
	term_win *scr;

//	term_win *tmp;
	term_win *mem[4];

	void (*init_hook)(term *t);
	void (*nuke_hook)(term *t);

	errr (*user_hook)(int n);
	errr (*xtra_hook)(int n, int v);
	errr (*curs_hook)(int x, int y);
	errr (*wipe_hook)(int x, int y, int n);
	errr (*pict_hook)(int x, int y, byte a, char32_t c);
	errr (*text_hook)(int x, int y, int n, byte a, cptr s);
};







/**** Available Constants ****/


/*
 * Definitions for the "actions" of "Term_xtra()"
 *
 * These values may be used as the first parameter of "Term_xtra()",
 * with the second parameter depending on the "action" itself.  Many
 * of the actions shown below are optional on at least one platform.
 *
 * The "TERM_XTRA_EVENT" action uses "v" to "wait" for an event
 * The "TERM_XTRA_SHAPE" action uses "v" to "show" the cursor
 * The "TERM_XTRA_FROSH" action uses "v" for the index of the row
 * The "TERM_XTRA_SOUND" action uses "v" for the index of a sound
 * The "TERM_XTRA_ALIVE" action uses "v" to "activate" (or "close")
 * The "TERM_XTRA_LEVEL" action uses "v" to "resume" (or "suspend")
 * The "TERM_XTRA_DELAY" action uses "v" as a "millisecond" value
 *
 * The other actions do not need a "v" code, so "zero" is used.
 */
#define TERM_XTRA_EVENT	1	/* Process some pending events */
#define TERM_XTRA_FLUSH 2	/* Flush all pending events */
#define TERM_XTRA_CLEAR 3	/* Clear the entire window */
#define TERM_XTRA_SHAPE 4	/* Set cursor shape (optional) */
#define TERM_XTRA_FROSH 5	/* Flush one row (optional) */
#define TERM_XTRA_FRESH 6	/* Flush all rows (optional) */
#define TERM_XTRA_NOISE 7	/* Make a noise (optional) */
#define TERM_XTRA_SOUND 8	/* Make a sound (optional) */
#define TERM_XTRA_BORED 9	/* Handle stuff when bored (optional) */
#define TERM_XTRA_REACT 10	/* React to global changes (optional) */
#define TERM_XTRA_ALIVE 11	/* Change the "hard" level (optional) */
#define TERM_XTRA_LEVEL 12	/* Change the "soft" level (optional) */
#define TERM_XTRA_DELAY 13	/* Delay some milliseconds (optional) */

#define DEFAULT_TERM_WID 80
#define DEFAULT_TERM_HGT 24

/**** Available Variables ****/

extern term *Term;


/**** Available Functions ****/

extern byte term2attr(byte ta);

extern errr Term_user(int n);
extern errr Term_xtra(int n, int v);

extern errr Term_fresh(void);
extern errr Term_set_cursor(int v);
extern errr Term_gotoxy(int x, int y);
extern errr Term_draw(int x, int y, byte a, char32_t c);
extern errr Term_addch(byte a, char c);
extern errr Term_addstr(int n, byte a, cptr s);
extern errr Term_putch(int x, int y, byte a, char c);
extern errr Term_putstr(int x, int y, int n, byte a, char *s);
extern errr Term_erase(int x, int y, int n);
extern errr Term_clear(void);
extern errr Term_redraw(void);
extern errr Term_redraw_section(int x1, int y1, int x2, int y2);

extern errr Term_get_cursor(int *v);
extern errr Term_get_size(int *w, int *h);
extern errr Term_locate(int *x, int *y);
extern errr Term_what(int x, int y, byte *a, char32_t *c);

extern errr Term_flush(void);
extern errr Term_keypress_aux(key_queue *keys, int k);
extern errr Term_keypress(int k);
extern errr Term_key_push_aux(key_queue *keys, int k);
extern errr Term_key_push(int k);
extern errr Term_key_push_buf_aux(key_queue *keys, cptr buf, int len);
extern errr Term_key_push_buf(cptr buf, int len);
extern errr Term_inkey(char *ch, bool wait, bool take);

extern errr Term_save(void);
extern errr Term_load(void);
extern errr Term_restore(void);
extern errr Term_switch(int screen);

extern errr Term_resize(int w, int h);

extern errr Term_activate(term *t);

extern errr term_nuke(term *t);
extern errr term_init(term *t, int w, int h, int k);

extern byte flick_colour(byte attr);
extern void flicker(void);

extern void validate_term_screen_dimensions(int *cols, int *rows);
extern void validate_term_dimensions(int term_idx, int *cols, int *rows);
#endif
