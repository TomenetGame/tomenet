/* File: main-x11.c */

/* Purpose: One (awful) way to run Angband under X11	-BEN- */


#include "angband.h"


#ifdef USE_X11

/* #define USE_GRAPHICS */

#include "../common/z-util.h"
#include "../common/z-virt.h"
#include "../common/z-form.h"


#ifndef __MAKEDEPEND__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/Xatom.h>
#if 0
#include <X11/extensions/XTest.h> /* for turn_off_numlock() */
#endif
#endif /* __MAKEDEPEND__ */


/*
 * OPTION: Allow the use of a "Mirror Window", if supported
 */
#define GRAPHIC_MIRROR

/*
 * OPTION: Allow the use of a "Recall Window", if supported
 */
#define GRAPHIC_RECALL

/*
 * OPTION: Allow the use of a "Choice Window", if supported
 */
#define GRAPHIC_CHOICE

/* More options: Enable windows 5..8 (term-4..term-7) - C. Blue */
#define GRAPHIC_TERM_4
#define GRAPHIC_TERM_5
#define GRAPHIC_TERM_6
#define GRAPHIC_TERM_7


/*
 * Notes on Colors:
 *
 *   1) On a monochrome (or "fake-monochrome") display, all colors
 *   will be "cast" to "fg," except for the bg color, which is,
 *   obviously, cast to "bg".  Thus, one can ignore this setting.
 *
 *   2) Because of the inner functioning of the color allocation
 *   routines, colors may be specified as (a) a typical color name,
 *   (b) a hexidecimal color specification (preceded by a pound sign),
 *   or (c) by strings such as "fg", "bg", "zg".
 *
 *   3) Due to the workings of the init routines, many colors
 *   may also be dealt with by their actual pixel values.  Note that
 *   the pixel with all bits set is "zg = (1<<metadpy->depth)-1", which
 *   is not necessarily either black or white.
 */


void resize_main_window_x11(int cols, int rows);


/**** Available Types ****/

/*
 * An X11 pixell specifier
 */
typedef unsigned long Pixell;

/*
 * The structures defined below
 */
typedef struct metadpy metadpy;
typedef struct infowin infowin;
typedef struct infoclr infoclr;
typedef struct infofnt infofnt;


/*
 * A structure summarizing a given Display.
 *
 *	- The Display itself
 *	- The default Screen for the display
 *	- The virtual root (usually just the root)
 *	- The default colormap (from a macro)
 *
 *	- The "name" of the display
 *
 *	- The socket to listen to for events
 *
 *	- The width of the display screen (from a macro)
 *	- The height of the display screen (from a macro)
 *	- The bit depth of the display screen (from a macro)
 *
 *	- The black Pixell (from a macro)
 *	- The white Pixell (from a macro)
 *
 *	- The background Pixell (default: black)
 *	- The foreground Pixell (default: white)
 *	- The maximal Pixell (Equals: ((2 ^ depth)-1), is usually ugly)
 *
 *	- Bit Flag: Force all colors to black and white (default: !color)
 *	- Bit Flag: Allow the use of color (default: depth > 1)
 *	- Bit Flag: We created 'dpy', and so should nuke it when done.
 */

struct metadpy
{
	Display	*dpy;
	Screen	*screen;
	Window	root;
	Colormap	cmap;

	char		*name;

	int		fd;

	uint		width;
	uint		height;
	uint		depth;

	Pixell	black;
	Pixell	white;

	Pixell	bg;
	Pixell	fg;
	Pixell	zg;

	uint		mono:1;
	uint		color:1;
	uint		nuke:1;
};



/*
 * A Structure summarizing Window Information.
 *
 * I assume that a window is at most 30000 pixels on a side.
 * I assume that the root windw is also at most 30000 square.
 *
 *	- The Window
 *	- The current Input Event Mask
 *
 *	- The location of the window
 *	- The width, height of the window
 *	- The border width of this window
 *
 *	- Byte: 1st Extra byte
 *
 *	- Bit Flag: This window is currently Mapped
 *	- Bit Flag: This window needs to be redrawn
 *	- Bit Flag: This window has been resized
 *
 *	- Bit Flag: We should nuke 'win' when done with it
 *
 *	- Bit Flag: 1st extra flag
 *	- Bit Flag: 2nd extra flag
 *	- Bit Flag: 3rd extra flag
 *	- Bit Flag: 4th extra flag
 */

struct infowin
{
	Window		win;
	long			mask;

	s16b			x, y;
	s16b			w, h;
	u16b			b;

	byte			byte1;

	uint			mapped:1;
	uint			redraw:1;
	uint			resize:1;

	uint			nuke:1;

	uint			flag1:1;
	uint			flag2:1;
	uint			flag3:1;
	uint			flag4:1;
};






/*
 * A Structure summarizing Operation+Color Information
 *
 *	- The actual GC corresponding to this info
 *
 *	- The Foreground Pixell Value
 *	- The Background Pixell Value
 *
 *	- Num (0-15): The operation code (As in Clear, Xor, etc)
 *	- Bit Flag: The GC is in stipple mode
 *	- Bit Flag: Destroy 'gc' at Nuke time.
 */

struct infoclr
{
	GC			gc;

	Pixell		fg;
	Pixell		bg;

	uint			code:4;
	uint			stip:1;
	uint			nuke:1;
};



/*
 * A Structure to Hold Font Information
 *
 *	- The 'XFontStruct*' (yields the 'Font')
 *
 *	- The font name
 *
 *	- The default character width
 *	- The default character height
 *	- The default character ascent
 *
 *	- Byte: Pixel offset used during fake mono
 *
 *	- Flag: Force monospacing via 'wid'
 *	- Flag: Nuke info when done
 */

struct infofnt
{
	XFontStruct	*info;

	cptr			name;

	s16b			wid;
	s16b			hgt;
	s16b			asc;

	byte			off;

	uint			mono:1;
	uint			nuke:1;
};





/* OPEN: x-metadpy.h */




/**** Available Macros ****/


/* Set current metadpy (Metadpy) to 'M' */
#define Metadpy_set(M) \
	Metadpy = M


/* Initialize 'M' using Display 'D' */
#define Metadpy_init_dpy(D) \
	Metadpy_init_2(D,cNULL)

/* Initialize 'M' using a Display named 'N' */
#define Metadpy_init_name(N) \
	Metadpy_init_2((Display*)(NULL),N)

/* Initialize 'M' using the standard Display */
#define Metadpy_init() \
	Metadpy_init_name("")


/* SHUT: x-metadpy.h */


/* OPEN: x-infowin.h */


/**** Available Macros ****/

/* Init an infowin by giving father as an (info_win*) (or NULL), and data */
#define Infowin_init_dad(D,X,Y,W,H,B,FG,BG) \
	Infowin_init_data(((D) ? ((D)->win) : (Window)(None)), \
	                  X,Y,W,H,B,FG,BG)


/* Init a top level infowin by pos,size,bord,Colors */
#define Infowin_init_top(X,Y,W,H,B,FG,BG) \
	Infowin_init_data(None,X,Y,W,H,B,FG,BG)


/* Request a new standard window by giving Dad infowin and X,Y,W,H */
#define Infowin_init_std(D,X,Y,W,H,B) \
	Infowin_init_dad(D,X,Y,W,H,B,Metadpy->fg,Metadpy->bg)


/* Set the current Infowin */
#define Infowin_set(I) \
	(Infowin = (I))



/* SHUT: x-infowin.h */


/* OPEN: x-infoclr.h */




/**** Available Macros  ****/

/* Set the current Infoclr */
#define Infoclr_set(C) \
	(Infoclr = (C))



/**** Available Macros (Requests) ****/

#define Infoclr_init_ppo(F,B,O,M) \
	Infoclr_init_data(F,B,O,M)

#define Infoclr_init_cco(F,B,O,M) \
	Infoclr_init_ppo(Infoclr_Pixell(F),Infoclr_Pixell(B),O,M)

#define Infoclr_init_ppn(F,B,O,M) \
	Infoclr_init_ppo(F,B,Infoclr_Opcode(O),M)

#define Infoclr_init_ccn(F,B,O,M) \
	Infoclr_init_cco(F,B,Infoclr_Opcode(O),M)


/* SHUT: x-infoclr.h */


/* OPEN: x-infofnt.h */



/**** Available Macros ****/

/* Set the current infofnt */
#define Infofnt_set(I) \
	(Infofnt = (I))


/* SHUT: x-infofnt.h */



/* OPEN: r-metadpy.h */

/* SHUT: r-metadpy.h */


/* OPEN: r-infowin.h */


/**** Available macros ****/

/* Errr: Expose Infowin */
#define Infowin_expose() \
	(!(Infowin->redraw = 1))

/* Errr: Unxpose Infowin */
#define Infowin_unexpose() \
	(Infowin->redraw = 0)


/* SHUT: r-infowin.h */


/* OPEN: r-infoclr.h */

/* SHUT: r-infoclr.h */


/* OPEN: r-infofnt.h */

/* SHUT: r-infofnt.h */




/* File: xtra-x11.c */


/*
 * The "default" values
 */
static metadpy metadpy_default;


/*
 * The "current" variables
 */
static metadpy *Metadpy = &metadpy_default;
static infowin *Infowin = (infowin*)(NULL);
static infoclr *Infoclr = (infoclr*)(NULL);
static infofnt *Infofnt = (infofnt*)(NULL);





/* OPEN: x-metadpy.c */


/*
 * Init the current metadpy, with various initialization stuff.
 *
 * Inputs:
 *	dpy:  The Display* to use (if NULL, create it)
 *	name: The name of the Display (if NULL, the current)
 *
 * Notes:
 *	If 'name' is NULL, but 'dpy' is set, extract name from dpy
 *	If 'dpy' is NULL, then Create the named Display
 *	If 'name' is NULL, and so is 'dpy', use current Display
 */
static errr Metadpy_init_2(Display *dpy, cptr name)
{
	metadpy *m = Metadpy;


	/*** Open the display if needed ***/

	/* If no Display given, attempt to Create one */
	if (!dpy)
	{
		/* Attempt to open the display */
		dpy = XOpenDisplay(name);

		/* Failure */
		if (!dpy)
		{
			/* No name given, extract DISPLAY */
			if (!name) name = getenv("DISPLAY");

			/* No DISPLAY extracted, use default */
			if (!name) name = "(default)";

			/* Error */
			return (-1);
		}

		/* We WILL have to Nuke it when done */
		m->nuke = 1;
	}

	/* Since the Display was given, use it */
	else
	{
		/* We will NOT have to Nuke it when done */
		m->nuke = 0;
	}


	/*** Save some information ***/

	/* Save the Display itself */
	m->dpy = dpy;

	/* Get the Screen and Virtual Root Window */
	m->screen = DefaultScreenOfDisplay(dpy);
	m->root = RootWindowOfScreen(m->screen);

	/* Get the default colormap */
	m->cmap = DefaultColormapOfScreen(m->screen);

	/* Extract the true name of the display */
	m->name = DisplayString(dpy);

	/* Extract the fd */
	m->fd = ConnectionNumber(Metadpy->dpy);

	/* Use the fd for select() - mikaelh */
	x11_socket = m->fd;

	/* Save the Size and Depth of the screen */
	m->width = WidthOfScreen(m->screen);
	m->height = HeightOfScreen(m->screen);
	m->depth = DefaultDepthOfScreen(m->screen);

	/* Save the Standard Colors */
	m->black = BlackPixelOfScreen(m->screen);
	m->white = WhitePixelOfScreen(m->screen);


	/*** Make some clever Guesses ***/

	/* Guess at the desired 'fg' and 'bg' Pixell's */
	m->bg = m->black;
	m->fg = m->white;

	/* Calculate the Maximum allowed Pixel value.  */
	m->zg = (1 << m->depth) - 1;

	/* Save various default Flag Settings */
	m->color = ((m->depth > 1) ? 1 : 0);
	m->mono = ((m->color) ? 0 : 1);


	/*** All done ***/

	/* Return "success" ***/
	return (0);
}

/*
 * General Flush/ Sync/ Discard routine
 */
static errr Metadpy_update(int flush, int sync, int discard)
{
	/* Flush if desired */
	if (flush) XFlush(Metadpy->dpy);

	/* Sync if desired, using 'discard' */
	if (sync) XSync(Metadpy->dpy, discard);

	/* Success */
	return (0);
}



/*
 * Make a simple beep
 */
static errr Metadpy_do_beep(void)
{
	/* Make a simple beep */
	XBell(Metadpy->dpy, 100);

	return (0);
}



/* SHUT: x-metadpy.c */


/* OPEN: x-metadpy.c */


/*
 * Set the name (in the title bar) of Infowin
 */
static errr Infowin_set_name(cptr name)
{
	Status st;
	XTextProperty tp;
	char buf[128];
	char *bp = buf;
	strcpy(buf, name);
	st = XStringListToTextProperty(&bp, 1, &tp); /* --- Valgrind actually says that this function has a memory leak! --- */
	if (st) XSetWMName(Metadpy->dpy, Infowin->win, &tp);
	return (0);
}

/*
 * Prepare a new 'infowin'.
 */
static errr Infowin_prepare(Window xid)
{
	infowin *iwin = Infowin;

	Window tmp_win;
	XWindowAttributes xwa;
	int x, y;
	unsigned int w, h, b, d;

	/* Assign stuff */
	iwin->win = xid;

	/* Check For Error XXX Extract some ACTUAL data from 'xid' */
	XGetGeometry(Metadpy->dpy, xid, &tmp_win, &x, &y, &w, &h, &b, &d);

	/* Apply the above info */
	iwin->x = x;
	iwin->y = y;
	iwin->w = w;
	iwin->h = h;
	iwin->b = b;

	/* Check Error XXX Extract some more ACTUAL data */
	XGetWindowAttributes(Metadpy->dpy, xid, &xwa);

	/* Apply the above info */
	iwin->mask = xwa.your_event_mask;
	iwin->mapped = ((xwa.map_state == IsUnmapped) ? 0 : 1);

	/* And assume that we are exposed */
	iwin->redraw = 1;

	/* Success */
	return (0);
}

/*
 * Init an infowin by giving some data.
 *
 * Inputs:
 *	dad: The Window that should own this Window (if any)
 *	x,y: The position of this Window
 *	w,h: The size of this Window
 *	b,d: The border width and pixel depth
 *
 * Notes:
 *	If 'dad == None' assume 'dad == root'
 */
static errr Infowin_init_data(Window dad, int x, int y, int w, int h,
                              int b, Pixell fg, Pixell bg)
{
	Window xid;


	/* Wipe it clean */
	WIPE(Infowin, infowin);


	/*** Error Check XXX ***/


	/*** Create the Window 'xid' from data ***/

	/* If no parent given, depend on root */
	if (dad == None) dad = Metadpy->root;

	/* Create the Window XXX Error Check */
	xid = XCreateSimpleWindow(Metadpy->dpy, dad, x, y, w, h, b, fg, bg);

	/* Start out selecting No events */
	XSelectInput(Metadpy->dpy, xid, 0L);


	/*** Prepare the new infowin ***/

	/* Mark it as nukable */
	Infowin->nuke = 1;

	/* Attempt to Initialize the infowin */
	return (Infowin_prepare (xid));
}


/* SHUT: x-infowin.c */


/* OPEN: x-infoclr.c */


/*
 * A NULL terminated pair list of legal "operation names"
 *
 * Pairs of values, first is texttual name, second is the string
 * holding the decimal value that the operation corresponds to.
 */
static cptr opcode_pairs[] =
{
	"cpy", "3",
	"xor", "6",
	"and", "1",
	"ior", "7",
	"nor", "8",
	"inv", "10",
	"clr", "0",
	"set", "15",

	"src", "3",
	"dst", "5",

	"dst & src", "1",
	"src & dst", "1",

	"dst | src", "7",
	"src | dst", "7",

	"dst ^ src", "6",
	"src ^ dst", "6",

	"+andReverse", "2",
	"+andInverted", "4",
	"+noop", "5",
	"+equiv", "9",
	"+orReverse", "11",
	"+copyInverted", "12",
	"+orInverted", "13",
	"+nand", "14",
	NULL
};


/*
 * Parse a word into an operation "code"
 *
 * Inputs:
 *	str: A string, hopefully representing an Operation
 *
 * Output:
 *	0-15: if 'str' is a valid Operation
 *	-1:   if 'str' could not be parsed
 */
static int Infoclr_Opcode(cptr str)
{
	register int i;

	/* Scan through all legal operation names */
	for (i = 0; opcode_pairs[i*2]; ++i)
	{
		/* Is this the right oprname? */
		if (streq(opcode_pairs[i*2], str))
		{
			/* Convert the second element in the pair into a Code */
			return (atoi(opcode_pairs[i*2+1]));
		}
	}

	/* The code was not found, return -1 */
	return (-1);
}



/*
 * Request a Pixell by name.  Note: uses 'Metadpy'.
 *
 * Inputs:
 *      name: The name of the color to try to load (see below)
 *
 * Output:
 *	The Pixell value that metched the given name
 *	'Metadpy->fg' if the name was unparseable
 *
 * Valid forms for 'name':
 *	'fg', 'bg', 'zg', '<name>' and '#<code>'
 */
static Pixell Infoclr_Pixell(cptr name)
{
	XColor scrn;


	/* Attempt to Parse the name */
	if (name && name[0])
	{
		/* The 'bg' color is available */
		if (streq(name, "bg")) return (Metadpy->bg);

		/* The 'fg' color is available */
		if (streq(name, "fg")) return (Metadpy->fg);

		/* The 'zg' color is available */
		if (streq(name, "zg")) return (Metadpy->zg);

		/* The 'white' color is available */
		if (streq(name, "white")) return (Metadpy->white);

		/* The 'black' color is available */
		if (streq(name, "black")) return (Metadpy->black);

		/* Attempt to parse 'name' into 'scrn' */
		if (!(XParseColor(Metadpy->dpy, Metadpy->cmap, name, &scrn)))
		{
			plog_fmt("Warning: Couldn't parse color '%s'\n", name);
		}

		/* Attempt to Allocate the Parsed color */
		if (!(XAllocColor (Metadpy->dpy, Metadpy->cmap, &scrn)))
		{
			plog_fmt("Warning: Couldn't allocate color '%s'\n", name);
		}

		/* The Pixel was Allocated correctly */
		else return (scrn.pixel);
	}

	/* Warn about the Default being Used */
	plog_fmt("Warning: Using 'fg' for unknown color '%s'\n", name);

	/* Default to the 'Foreground' color */
	return (Metadpy->fg);
}

/*
 * Initialize an infoclr with some data
 *
 * Inputs:
 *	fg:   The Pixell for the requested Foreground (see above)
 *	bg:   The Pixell for the requested Background (see above)
 *	op:   The Opcode for the requested Operation (see above)
 *	stip: The stipple mode
 */
static errr Infoclr_init_data(Pixell fg, Pixell bg, int op, int stip)
{
	infoclr *iclr = Infoclr;

	GC gc;
	XGCValues gcv;
	unsigned long gc_mask;



	/*** Simple error checking of opr and clr ***/

	/* Check the 'Pixells' for realism */
	if (bg > Metadpy->zg) return (-1);
	if (fg > Metadpy->zg) return (-1);

	/* Check the data for trueness */
	if ((op < 0) || (op > 15)) return (-1);


	/*** Create the requested 'GC' ***/

	/* Assign the proper GC function */
	gcv.function = op;

	/* Assign the proper GC background */
	gcv.background = bg;

	/* Assign the proper GC foreground */
	gcv.foreground = fg;

	/* Hack -- Handle XOR (xor is code 6) by hacking bg and fg */
	if (op == 6) gcv.background = 0;
	if (op == 6) gcv.foreground = (bg ^ fg);

	/* Assign the proper GC Fill Style */
	gcv.fill_style = (stip ? FillStippled : FillSolid);

	/* Turn off 'Give exposure events for pixmap copying' */
	gcv.graphics_exposures = False;

	/* Set up the GC mask */
	gc_mask = (GCFunction | GCBackground | GCForeground |
	           GCFillStyle | GCGraphicsExposures);

	/* Create the GC detailed above */
	gc = XCreateGC(Metadpy->dpy, Metadpy->root, gc_mask, &gcv);


	/*** Initialize ***/

	/* Wipe the iclr clean */
	WIPE(iclr, infoclr);

	/* Assign the GC */
	iclr->gc = gc;

	/* Nuke it when done */
	iclr->nuke = 1;

	/* Assign the parms */
	iclr->fg = fg;
	iclr->bg = bg;
	iclr->code = op;
	iclr->stip = stip ? 1 : 0;

	/* Success */
	return (0);
}

/*
 * Prepare a new 'infofnt'
 */
static errr Infofnt_prepare(XFontStruct *info)
{
	infofnt *ifnt = Infofnt;

	XCharStruct *cs;

	/* Assign the struct */
	ifnt->info = info;

	/* Jump into the max bouonds thing */
	cs = &(info->max_bounds);

	/* Extract default sizing info */
	ifnt->asc = info->ascent;
	ifnt->hgt = info->ascent + info->descent;
	ifnt->wid = cs->width;

#ifdef OBSOLETE_SIZING_METHOD
	/* Extract default sizing info */
	ifnt->asc = cs->ascent;
	ifnt->hgt = (cs->ascent + cs->descent);
	ifnt->wid = cs->width;
#endif

	/* Success */
	return (0);
}

/*
 * Init an infofnt by its Name
 *
 * Inputs:
 *	name: The name of the requested Font
 */
static errr Infofnt_init_data(cptr name)
{
	XFontStruct *info;


	/*** Load the info Fresh, using the name ***/

	/* If the name is not given, report an error */
	if (!name) return (-1);

	/* Attempt to load the font */
	info = XLoadQueryFont(Metadpy->dpy, name);

	/* The load failed, try to recover */
	if (!info) {
		fprintf(stderr, "Font not found: %s\n", name);
		return (-1);
	}


	/*** Init the font ***/

	/* Wipe the thing */
	WIPE(Infofnt, infofnt);

	/* Attempt to prepare it */
	if (Infofnt_prepare(info))
	{
		/* Free the font */
		XFreeFont(Metadpy->dpy, info);

		/* Fail */
		return (-1);
	}

	/* Save a copy of the font name */
	Infofnt->name = string_make(name);

	/* Mark it as nukable */
	Infofnt->nuke = 1;

	/* Success */
	return (0);
}


/* SHUT: x-infofnt.c */







/* OPEN: r-metadpy.c */

/* SHUT: r-metadpy.c */

/* OPEN: r-infowin.c */

/*
 * Modify the event mask of an Infowin
 */
static errr Infowin_set_mask (long mask)
{
	/* Save the new setting */
	Infowin->mask = mask;

	/* Execute the Mapping */
	XSelectInput(Metadpy->dpy, Infowin->win, Infowin->mask);

	/* Success */
	return (0);
}








/*
 * Request that Infowin be mapped
 */
static errr Infowin_map (void)
{
	/* Execute the Mapping */
	XMapWindow(Metadpy->dpy, Infowin->win);

	/* Success */
	return (0);
}

/*
 * Request that Infowin be raised
 */
static errr Infowin_raise(void)
{
	/* Raise towards visibility */
	XRaiseWindow(Metadpy->dpy, Infowin->win);

	/* Success */
	return (0);
}

/*
 * Resize an infowin
 */
static errr Infowin_resize(int w, int h)
{
	/* Execute the request */
	XResizeWindow(Metadpy->dpy, Infowin->win, w, h);

	/* Success */
	return (0);
}

/*
 * Visually clear Infowin
 */
static errr Infowin_wipe(void)
{
	/* Execute the request */
	XClearWindow(Metadpy->dpy, Infowin->win);

	/* Success */
	return (0);
}

/*
 * Standard Text
 */
static errr Infofnt_text_std(int x, int y, cptr str, int len)
{
	int i;


	/*** Do a brief info analysis ***/

	/* Do nothing if the string is null */
	if (!str || !*str) return (-1);

	/* Get the length of the string */
	if (len < 0) len = strlen (str);


	/*** Decide where to place the string, vertically ***/

	/* Ignore Vertical Justifications */
	y = (y * Infofnt->hgt) + Infofnt->asc;


	/*** Decide where to place the string, horizontally ***/

	/* Line up with x at left edge of column 'x' */
	x = (x * Infofnt->wid);


	/*** Actually draw 'str' onto the infowin ***/

	/* Be sure the correct font is ready */
	XSetFont(Metadpy->dpy, Infoclr->gc, Infofnt->info->fid);


	/*** Handle the fake mono we can enforce on fonts ***/

	/* Monotize the font */
	if (Infofnt->mono)
	{
		/* Do each character */
		for (i = 0; i < len; ++i)
		{
			/* Note that the Infoclr is set up to contain the Infofnt */
			XDrawImageString(Metadpy->dpy, Infowin->win, Infoclr->gc,
			                 x + i * Infofnt->wid + Infofnt->off, y, str + i, 1);
		}
	}

	/* Assume monoospaced font */
	else
	{
		/* Note that the Infoclr is set up to contain the Infofnt */
		XDrawImageString(Metadpy->dpy, Infowin->win, Infoclr->gc,
		                 x, y, str, len);
	}


	/* Success */
	return (0);
}






/*
 * Painting where text would be
 */
static errr Infofnt_text_non(int x, int y, cptr str, int len)
{
	int w, h;


	/*** Find the width ***/

	/* Negative length is a flag to count the characters in str */
	if (len < 0) len = strlen(str);

	/* The total width will be 'len' chars * standard width */
	w = len * Infofnt->wid;


	/*** Find the X dimensions ***/

	/* Line up with x at left edge of column 'x' */
	x = x * Infofnt->wid;


	/*** Find other dimensions ***/

	/* Simply do 'Infofnt->hgt' (a single row) high */
	h = Infofnt->hgt;

	/* Simply do "at top" in row 'y' */
	y = y * h;


	/*** Actually 'paint' the area ***/

	/* Just do a Fill Rectangle */
	XFillRectangle(Metadpy->dpy, Infowin->win, Infoclr->gc, x, y, w, h);

	/* Success */
	return (0);
}





/* SHUT: r-infofnt.c */




/* OPEN: main-x11.c */


#ifndef IsModifierKey

/*
 * Keysym macros, used on Keysyms to test for classes of symbols
 * These were stolen from one of the X11 header files
 */

#define IsKeypadKey(keysym) \
    (((unsigned)(keysym) >= XK_KP_Space) && ((unsigned)(keysym) <= XK_KP_Equal))

#define IsCursorKey(keysym) \
    (((unsigned)(keysym) >= XK_Home)     && ((unsigned)(keysym) <  XK_Select))

#define IsPFKey(keysym) \
    (((unsigned)(keysym) >= XK_KP_F1)     && ((unsigned)(keysym) <= XK_KP_F4))

#define IsFunctionKey(keysym) \
    (((unsigned)(keysym) >= XK_F1)       && ((unsigned)(keysym) <= XK_F35))

#define IsMiscFunctionKey(keysym) \
    (((unsigned)(keysym) >= XK_Select)   && ((unsigned)(keysym) <  XK_KP_Space))

#define IsModifierKey(keysym) \
    (((unsigned)(keysym) >= XK_Shift_L)  && ((unsigned)(keysym) <= XK_Hyper_R))

#endif


/*
 * Checks if the keysym is a special key or a normal key
 * Assume that XK_MISCELLANY keysyms are special
 */
#define IsSpecialKey(keysym) \
    ((unsigned)(keysym) >= 0xFF00)


/*
 * Hack -- cursor color
 */
static infoclr *xor;

/*
 * Color table
 */
#ifndef EXTENDED_BG_COLOURS
static infoclr *clr[16];
#else
static infoclr *clr[16 + 1];
#endif

/*
 * Forward declare
 */
typedef struct term_data term_data;

/*
 * A structure for each "term"
 */
struct term_data
{
	term t;

	infofnt *fnt;

	infowin *outer;
	infowin *inner;

#ifdef USE_GRAPHICS

	XImage *tiles;

#endif
};


/*
 * The main screen
 */
static term_data screen;

#ifdef GRAPHIC_MIRROR

/*
 * The (optional) "mirror" window
 */
static term_data mirror;

#endif

#ifdef GRAPHIC_RECALL

/*
 * The (optional) "recall" window
 */
static term_data recall;

#endif

#ifdef GRAPHIC_CHOICE

/*
 * The (optional) "choice" window
 */
static term_data choice;

#endif

#ifdef GRAPHIC_TERM_4
static term_data term_4;
#endif
#ifdef GRAPHIC_TERM_5
static term_data term_5;
#endif
#ifdef GRAPHIC_TERM_6
static term_data term_6;
#endif
#ifdef GRAPHIC_TERM_7
static term_data term_7;
#endif


/*
 * Set the size hints of Infowin
 */
static errr Infowin_set_size(int w, int h, int r_w, int r_h, bool fixed)
{
	XSizeHints *sh;

	/* Make Size Hints */
	sh = XAllocSizeHints();

	/* Oops */
	if (!sh) return (1);

	/* Fixed window size */
	if (fixed)
	{
		sh->flags = PMinSize | PMaxSize;
		sh->min_width = sh->max_width = w;
		sh->min_height = sh->max_height = h;
	}

	/* Variable window size */
	else
	{
		sh->flags = PMinSize;
		sh->min_width = r_w + 2;
		sh->min_height = r_h + 2;
	}

	/* Standard fields */
	sh->width = w;
	sh->height = h;
	sh->width_inc = r_w;
	sh->height_inc = r_h;
	sh->base_width = 2;
	sh->base_height = 2;

	/* Useful settings */
	sh->flags |= PSize | PResizeInc | PBaseSize;

	/* Use the size hints */
	XSetWMNormalHints(Metadpy->dpy, Infowin->win, sh);

	/* Success */
	XFree(sh);
	return 0;
}


/*
 * Set the name (in the title bar) of Infowin
 */
static errr Infowin_set_class_hint(cptr name)
{
	XClassHint *ch;

	char res_name[20];
	char res_class[20];

	ch = XAllocClassHint();
	if (ch == NULL) return (1);

	strcpy(res_name, name);
	res_name[0] = FORCELOWER(res_name[0]);
	ch->res_name = res_name;

	strcpy(res_class, "TomeNET");
	ch->res_class = res_class;

	XSetClassHint(Metadpy->dpy, Infowin->win, ch);

	XFree(ch);
	return (0);
}



/*
 * Process a keypress event
 */
static void react_keypress(XEvent *xev)
{
	int i, n, mc, ms, mo, mx;

	uint ks1;

	XKeyEvent *ev = (XKeyEvent*)(xev);

	KeySym ks;

	char buf[128];
	char msg[128];


	/* Check for "normal" keypresses */
	n = XLookupString(ev, buf, 125, &ks, NULL);

	/* Terminate */
	buf[n] = '\0';

	/* Hack -- convert into an unsigned int */
	ks1 = (uint)(ks);

	/* Extract four "modifier flags" */
	mc = (ev->state & ControlMask) ? TRUE : FALSE;
	ms = (ev->state & ShiftMask) ? TRUE : FALSE;
	mo = (ev->state & Mod1Mask) ? TRUE : FALSE;

	/* This is the NumLock state and usually it only causes problems - mikaelh */
//	mx = (ev->state & Mod2Mask) ? TRUE : FALSE;
	mx = FALSE;


	/* Hack -- Ignore "modifier keys" */
	if (IsModifierKey(ks)) return;


	/* Normal keys with no modifiers */
	if (n && !mo && !mx && !IsSpecialKey(ks))
	{
		/* Enqueue the normal key(s) */
		for (i = 0; buf[i]; i++) Term_keypress(buf[i]);

		/* All done */
		return;
	}


	/* Handle a few standard keys */
	switch (ks1)
	{
		case XK_Escape:
		Term_keypress(ESCAPE); return;

		case XK_Return:
		Term_keypress('\r'); return;

		case XK_Tab:
		Term_keypress('\t'); return;

		case XK_Delete:
		case XK_BackSpace:
		Term_keypress('\010'); return;
	}

	/* Hack -- Use the KeySym */
	if (ks)
	{
		sprintf(msg, "%c%s%s%s%s_%lX%c", 31,
		        mc ? "N" : "", ms ? "S" : "",
		        mo ? "O" : "", mx ? "M" : "",
		        (unsigned long)(ks), 13);
	}

	/* Hack -- Use the Keycode */
	else
	{
		sprintf(msg, "%c%s%s%s%sK_%X%c", 31,
		        mc ? "N" : "", ms ? "S" : "",
		        mo ? "O" : "", mx ? "M" : "",
		        ev->keycode, 13);
	}

	/* Enqueue the "fake" string */
	for (i = 0; msg[i]; i++) Term_keypress(msg[i]);


	/* Hack -- dump an "extra" string */
	if (n)
	{
		/* Start the "extra" string */
		Term_keypress(28);

		/* Enqueue the "real" string */
		for (i = 0; buf[i]; i++) Term_keypress(buf[i]);

		/* End the "extra" string */
		Term_keypress(28);
	}
}




/*
 * Process events
 */
static errr CheckEvent(bool wait) {
	term_data *old_td = (term_data*)(Term->data);

	XEvent xev_body, *xev = &xev_body;

	term_data *td = NULL;
	infowin *iwin = NULL;

	//int flag = 0;
	//int x, y, data;

	int t_idx = -1;


	/* Do not wait unless requested */
	if (!wait && !XPending(Metadpy->dpy)) return (1);

	/* Load the Event */
	XNextEvent(Metadpy->dpy, xev);


	/* Notice new keymaps */
	if (xev->type == MappingNotify) {
		XRefreshKeyboardMapping(&xev->xmapping);
		return 0;
	}


	/* Main screen, inner window */
	if (xev->xany.window == screen.inner->win) {
		td = &screen;
		iwin = td->inner;
		t_idx = 0;
	}
	/* Main screen, outer window */
	else if (xev->xany.window == screen.outer->win) {
		td = &screen;
		iwin = td->outer;
		t_idx = 0;
	}

//#ifdef GRAPHIC_MIRROR
	/* Mirror window, inner window */
	else if (term_mirror && xev->xany.window == mirror.inner->win) {
		td = &mirror;
		iwin = td->inner;
		t_idx = 1;
	}
	/* Mirror window, outer window */
	else if (term_mirror && xev->xany.window == mirror.outer->win)
	{
		td = &mirror;
		iwin = td->outer;
		t_idx = 1;
	}
//#endif

//#ifdef GRAPHIC_RECALL
	/* Recall window, inner window */
	else if (term_recall && xev->xany.window == recall.inner->win) {
		td = &recall;
		iwin = td->inner;
		t_idx = 2;
	}
	/* Recall Window, outer window */
	else if (term_recall && xev->xany.window == recall.outer->win) {
		td = &recall;
		iwin = td->outer;
		t_idx = 2;
	}
//#endif

//#ifdef GRAPHIC_CHOICE
	/* Choice window, inner window */
	else if (term_choice && xev->xany.window == choice.inner->win) {
		td = &choice;
		iwin = td->inner;
		t_idx = 3;
	}
	/* Choice Window, outer window */
	else if (term_choice && xev->xany.window == choice.outer->win) {
		td = &choice;
		iwin = td->outer;
		t_idx = 3;
	}
//#endif

//#ifdef GRAPHIC_TERM_4
	/* Choice window, inner window */
	else if (term_term_4 && xev->xany.window == term_4.inner->win) {
		td = &term_4;
		iwin = td->inner;
		t_idx = 4;
	}
	/* Choice Window, outer window */
	else if (term_term_4 && xev->xany.window == term_4.outer->win) {
		td = &term_4;
		iwin = td->outer;
		t_idx = 4;
	}
//#endif

//#ifdef GRAPHIC_TERM_5
	/* Choice window, inner window */
	else if (term_term_5 && xev->xany.window == term_5.inner->win) {
		td = &term_5;
		iwin = td->inner;
		t_idx = 5;
	}
	/* Choice Window, outer window */
	else if (term_term_5 && xev->xany.window == term_5.outer->win) {
		td = &term_5;
		iwin = td->outer;
		t_idx = 5;
	}
//#endif

//#ifdef GRAPHIC_TERM_6
	/* Choice window, inner window */
	else if (term_term_6 && xev->xany.window == term_6.inner->win) {
		td = &term_6;
		iwin = td->inner;
		t_idx = 6;
	}
	/* Choice Window, outer window */
	else if (term_term_6 && xev->xany.window == term_6.outer->win) {
		td = &term_6;
		iwin = td->outer;
		t_idx = 6;
	}
//#endif

//#ifdef GRAPHIC_TERM_7
	/* Choice window, inner window */
	else if (term_term_7 && xev->xany.window == term_7.inner->win) {
		td = &term_7;
		iwin = td->inner;
		t_idx = 7;
	}
	/* Choice Window, outer window */
	else if (term_term_7 && xev->xany.window == term_7.outer->win) {
		td = &term_7;
		iwin = td->outer;
		t_idx = 7;
	}
//#endif


	/* Unknown window */
	if (!td || !iwin) return (0);


	/* Hack -- activate the Term */
	Term_activate(&td->t);

	/* Hack -- activate the window */
	Infowin_set(iwin);


	/* Switch on the Type */
	switch (xev->type)
	{
		/* A Button Press Event */
		case ButtonPress:
		{
			/* Set flag, then fall through */
			//flag = 1;
		}

		/* A Button Release (or ButtonPress) Event */
		case ButtonRelease:
		{
			/* Which button is involved */
			/* if      (xev->xbutton.button == Button1) data = 1;
			else if (xev->xbutton.button == Button2) data = 2;
			else if (xev->xbutton.button == Button3) data = 3;
			else if (xev->xbutton.button == Button4) data = 4;
			else if (xev->xbutton.button == Button5) data = 5; */

			/* Where is the mouse */
			//x = xev->xbutton.x;
			//y = xev->xbutton.y;

			/* XXX Handle */

			break;
		}

		/* An Enter Event */
		case EnterNotify:
		{
			/* Note the Enter, Fall into 'Leave' */
			//flag = 1;
		}

		/* A Leave (or Enter) Event */
		case LeaveNotify:
		{
			/* Where is the mouse */
			//x = xev->xcrossing.x;
			//y = xev->xcrossing.y;

			/* XXX Handle */

			break;
		}

		/* A Motion Event */
		case MotionNotify:
		{
			/* Where is the mouse */
			//x = xev->xmotion.x;
			//y = xev->xmotion.y;

			/* XXX Handle */

			break;
		}

		/* A KeyRelease */
		case KeyRelease:
		{
			/* Nothing */
			break;
		}

		/* A KeyPress */
		case KeyPress:
		{
			/* Save the mouse location */
			//x = xev->xkey.x;
			//y = xev->xkey.y;

			/* Hack -- use "old" term */
			Term_activate(&old_td->t);

			/* Process the key */
			react_keypress(xev);

			break;
		}

		/* An Expose Event */
		case Expose:
		{
			/* Ignore "extra" exposes */
			if (xev->xexpose.count) break;

			/* Clear the window */
			Infowin_wipe();

			/* Redraw (if allowed) */
			if (iwin == td->inner) Term_redraw();

			break;
		}

		/* A Mapping Event */
		case MapNotify:
		{
			Infowin->mapped = 1;
			break;
		}

		/* An UnMap Event */
		case UnmapNotify:
		{
			/* Save the mapped-ness */
			Infowin->mapped = 0;
			break;
		}

		/* A Move AND/OR Resize Event */
		case ConfigureNotify:
		{
			int cols, rows, wid, hgt;
			//int old_w, old_h;
			/* For OSX: Prevent loop bug of outer-infowin-resizing forever, freezing/slowing down */
			int old_cols, old_rows;

			Infowin_set(td->outer);

			/* Save the Old information */
			//old_w = Infowin->w;
			//old_h = Infowin->h;

			/* Save the new Window Parms */
			Infowin->x = xev->xconfigure.x;
			Infowin->y = xev->xconfigure.y;
			Infowin->w = xev->xconfigure.width;
			Infowin->h = xev->xconfigure.height;

			/* Detemine "proper" number of rows/cols */
			cols = ((Infowin->w - 2) / td->fnt->wid);
			rows = ((Infowin->h - 2) / td->fnt->hgt);

			old_cols = cols;
			old_rows = rows;

			/* Hack -- do not allow bad resizing of main screen */
			if (td == &screen) cols = 80;
			if (td == &screen) {
				/* respect big_map option */
				if (rows <= 24 || (in_game && !(sflags1 & SFLG1_BIG_MAP))) rows = 24;
				else rows = 46;
			}

			/* Hack -- minimal size */
			if (cols < 1) cols = 1;
			if (rows < 1) rows = 1;

			/* Desired size of "outer" window */
			wid = cols * td->fnt->wid;
			hgt = rows * td->fnt->hgt;

			/* Resize the windows if any "change" is needed -- This should work now.
			   Added hack for OSX, where Infowin_resize() wouldn't resize except for
			   the very first time it was called, but not while mouse-resizing. :( - C. Blue */
			if (old_rows != rows || old_cols != cols /* for main window */
			    || td != &screen) { /* for sub windows */
				Infowin_set(td->inner);
				Infowin_resize(wid, hgt);

				/* Make the changes go live (triggers on next c_message_add() call) */
				Term_activate(&td->t);
				Term_resize(cols, rows);

				/* only change rows/cols of outer infowin if we really have to,
				   or it'll cause freeze/slowing loop on OSX. For some reason,
				   doing it in this exceptional case seems to work though. */
				if (td == &screen) { /* for main window only */
					Infowin_set(td->outer);
					Infowin_resize(wid + 2, hgt + 2);
				}

				/* basically obsolete, since these values are unused once they were
				   read and used in the very begining of client initialisation, but w/e */
				term_prefs[t_idx].columns = cols;
				term_prefs[t_idx].lines = rows;

				/* In case we resized Chat/Msg/Msg+Chat window,
				   refresh contents so they are displayed properly,
				   without having to wait for another incoming message to do it for us. */
				p_ptr->window |= PW_MESSAGE | PW_CHAT | PW_MSGNOCHAT;
			}

			/* hack: Switch big_map mode (and clear screen)  */
			if (td == &screen && in_game &&
			    ((rows == 24 && Client_setup.options[CO_BIGMAP]) ||
			    (rows == 46 && !Client_setup.options[CO_BIGMAP]))) {
				/* and for big_map.. */
				if (rows == 24) {
					/* turn off big_map */
					c_cfg.big_map = FALSE;
					Client_setup.options[CO_BIGMAP] = FALSE;
					screen_hgt = SCREEN_HGT;
				} else {
					/* turn on big_map */
					c_cfg.big_map = TRUE;
					Client_setup.options[CO_BIGMAP] = TRUE;
					screen_hgt = MAX_SCREEN_HGT;
				}
				Term_clear();
				Send_screen_dimensions();
				cmd_redraw();
			}

			break;
		}
	}


	/* Hack -- Activate the old term */
	Term_activate(&old_td->t);

	/* Hack -- Activate the proper "inner" window */
	Infowin_set(old_td->inner);


	/* XXX XXX Hack -- map/unmap as needed */


	/* Success */
	return (0);
}







/*
 * Handle "activation" of a term
 */
static errr Term_xtra_x11_level(int v)
{
	term_data *td = (term_data*)(Term->data);

	/* Handle "activate" */
	if (v)
	{
		/* Activate the "inner" window */
		Infowin_set(td->inner);

		/* Activate the "inner" font */
		Infofnt_set(td->fnt);
	}

	/* Success */
	return (0);
}


/*
 * Handle a "special request"
 */
static errr Term_xtra_x11(int n, int v)
{
	/* Handle a subset of the legal requests */
	switch (n)
	{
		/* Make a noise */
		case TERM_XTRA_NOISE: Metadpy_do_beep(); return (0);

		/* Flush the output XXX XXX XXX */
		case TERM_XTRA_FRESH: Metadpy_update(1, 0, 0); return (0);

		/* Process random events XXX XXX XXX */
		case TERM_XTRA_BORED: return (CheckEvent(0));

		/* Process Events XXX XXX XXX */
		case TERM_XTRA_EVENT: return (CheckEvent(v));

		/* Flush the events XXX XXX XXX */
		case TERM_XTRA_FLUSH: while (!CheckEvent(FALSE)); return (0);

		/* Handle change in the "level" */
		case TERM_XTRA_LEVEL: return (Term_xtra_x11_level(v));

		/* Clear the screen */
		case TERM_XTRA_CLEAR: Infowin_wipe(); return (0);

		/* Delay for some milliseconds */
		case TERM_XTRA_DELAY: usleep(1000 * v); return (0);
	}

	/* Unknown */
	return (1);
}



/*
 * Erase a number of characters
 */
static errr Term_wipe_x11(int x, int y, int n)
{
	/* Erase (use black) */
	Infoclr_set(clr[0]);

	/* Mega-Hack -- Erase some space */
	Infofnt_text_non(x, y, "", n);

	/* Success */
	return (0);
}



static int cursor_x = -1, cursor_y = -1;
/*
 * Draw the cursor (XXX by hiliting)
 */
static errr Term_curs_x11(int x, int y)
{
	/*
	 * Don't place the cursor in the same place multiple times to avoid
	 * blinking.
	 */
	if ((cursor_x != x) || (cursor_y != y)) {
		/* Draw the cursor */
		Infoclr_set(xor);

		/* Hilite the cursor character */
		Infofnt_text_non(x, y, " ", 1);

		cursor_x = x;
		cursor_y = y;
	}

	/* Success */
	return (0);
}


/*
 * Draw a number of characters (XXX Consider using "cpy" mode)
 */
static errr Term_text_x11(int x, int y, int n, byte a, cptr s)
{
	/* Draw the text in Xor */
#ifndef EXTENDED_BG_COLOURS
	Infoclr_set(clr[a & 0x0F]);
#else
	if (a == TERM2_BLUE) a = 0xF + 1;
	Infoclr_set(clr[a & 0x1F]);
#endif

	/* Draw the text */
	Infofnt_text_std(x, y, s, n);

	/* Drawing text seems to clear the cursor */
	if (cursor_y == y && x <= cursor_x && cursor_x <= x + n) {
		/* Cursor is gone */
		cursor_x = -1;
		cursor_y = -1;
	}

	/* Success */
	return (0);
}

#ifdef USE_GRAPHICS

/*
 * Draw some graphical characters.
 */
static errr Term_pict_x11(int x, int y, byte a, byte c)
{
	int i;

	term_data *td = (term_data*)(Term->data);

	y *= Infofnt->hgt;
	x *= Infofnt->wid;

	XPutImage(Metadpy->dpy, td->inner->win,
	          clr[15]->gc,
	          td->tiles,
		  (c&0x7F) * td->fnt->wid + 1,
		  (a&0x7F) * td->fnt->hgt + 1,
		  x, y,
		  td->fnt->wid, td->fnt->hgt);

	/* Success */
	return (0);
}

#endif /* USE_GRAPHICS */



/*
 * Initialize a term_data
 */
static errr term_data_init(int index, term_data *td, bool fixed, cptr name, cptr font)
{
	term *t = &td->t;

	int wid, hgt, num;
	int win_cols = 80, win_lines = 24; /* 80, 24 default */
	cptr n;
	int topx, topy; /* 0, 0 default */

	/* Use values from .tomenetrc;
	   Environment variables (see further below) may override those. */
	win_cols = term_prefs[index].columns;
	win_lines = term_prefs[index].lines;
	topx = term_prefs[index].x;
	topy = term_prefs[index].y;

	/* Prepare the standard font */
	MAKE(td->fnt, infofnt);
	Infofnt_set(td->fnt);
	if (Infofnt_init_data(font) == -1) {
		fprintf(stderr, "Failed to load a font!\n");
	}

	/* Hack -- extract key buffer size */
	num = (fixed ? 1024 : 16);

//	if (!strcmp(name, "Screen")) {
	if (!strcmp(name, ang_term_name[0])) {
#if 1
		n = getenv("TOMENET_X11_WID_SCREEN");
		if (n) win_cols = atoi(n);
		n = getenv("TOMENET_X11_HGT_SCREEN");
		if (n) win_lines = atoi(n);
#else /* must remain 80x24! (also, it's always visible) */
		win_cols = 80;
		win_lines = 24;
#endif
	}
	if (!strcmp(name, ang_term_name[1])) {
		n = getenv("TOMENET_X11_WID_MIRROR");
		if (n) win_cols = atoi(n);
		n = getenv("TOMENET_X11_HGT_MIRROR");
		if (n) win_lines = atoi(n);
	}
	if (!strcmp(name, ang_term_name[2])) {
		n = getenv("TOMENET_X11_WID_RECALL");
		if (n) win_cols = atoi(n);
		n = getenv("TOMENET_X11_HGT_RECALL");
		if (n) win_lines = atoi(n);
	}
	if (!strcmp(name, ang_term_name[3])) {
		n = getenv("TOMENET_X11_WID_CHOICE");
		if (n) win_cols = atoi(n);
		n = getenv("TOMENET_X11_HGT_CHOICE");
		if (n) win_lines = atoi(n);
	}
	if (!strcmp(name, ang_term_name[4])) {
		n = getenv("TOMENET_X11_WID_TERM_4");
		if (n) win_cols = atoi(n);
		n = getenv("TOMENET_X11_HGT_TERM_4");
		if (n) win_lines = atoi(n);
	}
	if (!strcmp(name, ang_term_name[5])) {
		n = getenv("TOMENET_X11_WID_TERM_5");
		if (n) win_cols = atoi(n);
		n = getenv("TOMENET_X11_HGT_TERM_5");
		if (n) win_lines = atoi(n);
	}
	if (!strcmp(name, ang_term_name[6])) {
		n = getenv("TOMENET_X11_WID_TERM_6");
		if (n) win_cols = atoi(n);
		n = getenv("TOMENET_X11_HGT_TERM_6");
		if (n) win_lines = atoi(n);
	}
	if (!strcmp(name, ang_term_name[7])) {
		n = getenv("TOMENET_X11_WID_TERM_7");
		if (n) win_cols = atoi(n);
		n = getenv("TOMENET_X11_HGT_TERM_7");
		if (n) win_lines = atoi(n);
	}

	/* Hack -- Assume full size windows */
	wid = win_cols * td->fnt->wid;
	hgt = win_lines * td->fnt->hgt;

	/* Create a top-window (border 5) */
	MAKE(td->outer, infowin);
	Infowin_set(td->outer);
	Infowin_init_top(topx, topy, wid + 2, hgt + 2, 1, Metadpy->fg, Metadpy->bg);
	Infowin_set_mask(StructureNotifyMask | KeyPressMask);
	if (!strcmp(name, ang_term_name[0])) {
		char version[MAX_CHARS];
		sprintf(version, "TomeNET %d.%d.%d%s",
		    VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, CLIENT_VERSION_TAG);
		Infowin_set_name(version);
	} else Infowin_set_name(name);
	Infowin_set_class_hint(name);
	Infowin_set_size(wid + 2, hgt + 2, td->fnt->wid, td->fnt->hgt, fixed);
	Infowin_map();

	/* Create a sub-window for playing field */
	MAKE(td->inner, infowin);
	Infowin_set(td->inner);
	Infowin_init_std(td->outer, 1, 1, wid, hgt, 0);
	Infowin_set_mask(ExposureMask);
	Infowin_map();

#ifdef USE_GRAPHICS
	/* No graphics yet */
	td->tiles = NULL;
#endif /* USE_GRAPHICS */

	/* Initialize the term (full size) */
	term_init(t, win_cols, win_lines, num);

	/* Use a "soft" cursor */
	t->soft_cursor = TRUE;

	/* Erase with "white space" */
	t->attr_blank = TERM_WHITE;
	t->char_blank = ' ';

	/* Hooks */
	t->xtra_hook = Term_xtra_x11;
	t->curs_hook = Term_curs_x11;
	t->wipe_hook = Term_wipe_x11;
	t->text_hook = Term_text_x11;

#ifdef USE_GRAPHICS

	/* Use graphics */
	if (use_graphics) {
		printf("Setup hook\n");
		/* Graphics hook */
		t->pict_hook = Term_pict_x11;

		/* Use graphics sometimes */
		t->higher_pict = TRUE;
	}

#endif /* USE_GRAPHICS */

	/* Save the data */
	t->data = td;

	/* Activate (important) */
	Term_activate(t);

	/* Success */
	return (0);
}

/*
 * Names of the 16 colors
 *   Black, White, Slate, Orange,    Red, Green, Blue, Umber
 *   D-Gray, L-Gray, Violet, Yellow, L-Red, L-Green, L-Blue, L-Umber
 *
 * Colors courtesy of: Torbj|rn Lindgren <tl@ae.chalmers.se>
 *
 * These colors may no longer be valid...
 */
static char color_name[16][8] =
{
#if 0
	"black",        /* BLACK */
	"white",        /* WHITE */
	"#d7d7d7",      /* GRAY */
	"#ff9200",      /* ORANGE */
	"#ff0000",      /* RED */
	"#00cd00",      /* GREEN */
	"#0000fe",      /* BLUE */
	"#c86400",      /* BROWN */
	"#a3a3a3",      /* DARKGRAY */
	"#ebebeb",      /* LIGHTGRAY */
	"#a500ff",      /* PURPLE */
	"#fffd00",      /* YELLOW */
	"#ff00bc",      /* PINK */
	"#00ff00",      /* LIGHTGREEN */
	"#00c8ff",      /* LIGHTBLUE */
	"#ffcc80",      /* LIGHTBROWN */
#else /* same as main-win.c, which is reference */
	"#000000",      /* BLACK */
	"#ffffff",      /* WHITE */
	"#9d9d9d",      /* GRAY */
	"#ff8d00",      /* ORANGE */
	"#b70000",      /* RED */
	"#009d44",      /* GREEN */
#ifndef READABILITY_BLUE
	"#0000ff",      /* BLUE */
#else
	"#0033ff",      /* BLUE */
#endif
	"#8d6600",      /* BROWN */
#ifndef DISTINCT_DARK
	"#747474",      /* DARKGRAY */
#else
	//"#585858",      /* DARKGRAY */
	"#666666",      /* DARKGRAY */
#endif
	"#d7d7d7",      /* LIGHTGRAY */
	"#af00ff",      /* PURPLE */
	"#ffff00",      /* YELLOW */
	"#ff3030",      /* PINK */
	"#00ff00",      /* LIGHTGREEN */
	"#00ffff",      /* LIGHTBLUE */
	"#c79d55",      /* LIGHTBROWN */
#endif
};
#ifdef EXTENDED_BG_COLOURS
static cptr color_ext_name[1][2] =
{
	{"#0000ff", "#444444", },
};
#endif
static void enable_common_colormap_x11() {
	int i;
	for (i = 0; i < 16; i++) {
		unsigned long c = client_color_map[i];
		sprintf(color_name[i], "#%06lx", c & 0xffffffL);
	}
}

#ifdef USE_GRAPHICS


typedef struct BITMAPFILEHEADER
{
	u16b bfAlign;    /* HATE this */
	u16b bfType;
	u32b bfSize;
	u16b bfReserved1;
	u16b bfReserved2;
	u32b bfOffBits;
} BITMAPFILEHEADER;

typedef struct BITMAPINFOHEADER
{
	u32b biSize;
	u32b biWidth;
	u32b biHeight;
	u16b biPlanes;
	u16b biBitCount;
	u32b biCompresion;
	u32b biSizeImage;
	u32b biXPelsPerMeter;
	u32b biYPelsPerMeter;
	u32b biClrUsed;
	u32b biClrImportand;
} BITMAPINFOHEADER;

typedef struct RGB
{
	unsigned char r,g,b;
	unsigned char filler;
} RGB;

static Pixell Infoclr_Pixell(cptr name);
/*
 * Read a BMP file. XXX XXX XXX
 *
 * Replaced ReadRaw & RemapColors.
 */
static XImage *ReadBMP(Display *disp, char Name[])
{
	FILE *f;

	BITMAPFILEHEADER fileheader;
	BITMAPINFOHEADER infoheader;

	XImage *Res = NULL;
	char *Data,cname[8];
	int ncol,depth,x,y;
	RGB clrg;
	Pixell clr_Pixells[256];

	f = fopen(Name, "r");

	if (f != NULL) {
		fread((vptr)((int)&fileheader + 2), sizeof(fileheader) - 2, 1, f);
		fread(&infoheader,sizeof(infoheader), 1, f);
		if ((fileheader.bfType != 19778) || (infoheader.biSize != 40)) {
				plog_fmt("Incorrect file format %s",Name);
				quit("Bad BMP format");};

		/* Compute number of colors recorded */
		ncol = (fileheader.bfOffBits - 54) / 4;

		for (x = 0; x < ncol; ++x) {
		   fread(&clrg, 4, 1, f);
		   sprintf(cname, "#%02x%02x%02x", clrg.r, clrg.g, clrg.b);
		   clr_Pixells[x] = Infoclr_Pixell(cname); }

		depth = DefaultDepth(disp, DefaultScreen(disp));

		x = 1;
		y = (depth-1) >> 2;
		while (y >>= 1) x <<= 1;

		Data = (char *)malloc(infoheader.biSizeImage*x);

		if (Data != NULL) {
			Res = XCreateImage(disp,
			                   DefaultVisual(disp, DefaultScreen(disp)),
			                   depth, ZPixmap, 0, Data,
			                   infoheader.biWidth, infoheader.biHeight, 8, 0);

			if (Res != NULL) {
			   for (y = 0 ; y < infoheader.biHeight; ++y) {
			       for (x = 0; x < infoheader.biWidth; ++x) {
				    XPutPixel(Res, x, infoheader.biHeight - y - 1, clr_Pixells[getc(f)]);
			       }
			   }
			} else {
				free(Data);
			}
		}

		fclose(f);
	}

	return Res;
}



/*
 * Resize an image. XXX XXX XXX
 *
 * Also appears in "main-xaw.c".
 */
static XImage *ResizeImage(Display *disp, XImage *Im,
                           int ix, int iy, int ox, int oy)
{
	int width1, height1, width2, height2;
	int x1, x2, y1, y2, Tx, Ty;
	int *px1, *px2, *dx1, *dx2;
	int *py1, *py2, *dy1, *dy2;

	XImage *Tmp;

	char *Data;


	width1 = Im->width;
	height1 = Im->height;

	width2 = ox * width1 / ix;
	height2 = oy * height1 / iy;
	printf("w1: %d h1:%d w2:%d h2:%d\n",width1, height1, width2, height2);

	Data = (char *)malloc(width2 * height2 * Im->bits_per_pixel / 8);

	Tmp = XCreateImage(disp,
	                   DefaultVisual(disp, DefaultScreen(disp)),
	                   Im->depth, ZPixmap, 0, Data, width2, height2,
	                   32, 0);

	if (ix > ox)
	{
		px1 = &x1;
		px2 = &x2;
		dx1 = &ix;
		dx2 = &ox;
	}
	else
	{
		px1 = &x2;
		px2 = &x1;
		dx1 = &ox;
		dx2 = &ix;
	}

	if (iy > oy)
	{
		py1 = &y1;
		py2 = &y2;
		dy1 = &iy;
		dy2 = &oy;
	}
	else
	{
		py1 = &y2;
		py2 = &y1;
		dy1 = &oy;
		dy2 = &iy;
	}

	Ty = *dy1;

	for (y1 = 0, y2 = 0; (y1 < height1) && (y2 < height2); )
	{
		Tx = *dx1;

		for (x1 = 0, x2 = 0; (x1 < width1) && (x2 < width2); )
		{
			XPutPixel(Tmp, x2, y2, XGetPixel(Im, x1, y1));

			(*px1)++;

			Tx -= *dx2;
			if (Tx < 0)
			{
				Tx += *dx1;
				(*px2)++;
			}
		}

		(*py1)++;

		Ty -= *dy2;
		if (Ty < 0)
		{
			Ty += *dy1;
			(*py2)++;
		}
	}

	return Tmp;
}


#endif /* USE_GRAPHICS */


static term_data* term_idx_to_term_data(int term_idx) {
	term_data *td = &screen;

	switch (term_idx) {
	case 0: td = &screen; break;
	case 1: td = &mirror; break;
	case 2: td = &recall; break;
	case 3: td = &choice; break;
	case 4: td = &term_4; break;
	case 5: td = &term_5; break;
	case 6: td = &term_6; break;
	case 7: td = &term_7; break;
	}

	return (td);
}


/*
 * Initialization function for an "X11" module to Angband
 */
errr init_x11(void) {
	int i;
	cptr fnt_name = NULL;
	cptr dpy_name = "";

#ifdef USE_GRAPHICS
	char filename[1024];
	char path[1024];
#endif

#ifdef USE_GRAPHICS
	init_file_paths(path);
	/* Try graphics */
	if (getenv("TOMENET_GRAPHICS")) {
		int gfd;
		/* Build the name of the "graf" file */
		path_build(filename, 1024, ANGBAND_DIR_XTRA, "graf/8x8.bmp");
//???		strcpy(filename, "/usr/local/tomenet/lib/xtra/graf/16x16.bmp");

		printf("Trying for graphics file: %s\n", filename);
		/* Use graphics if bitmap file exists */
		if ((gfd = open(filename, 0, O_RDONLY)) != -1)
		{
			printf("Got graphics file\n");
			close(gfd);
			/* Use graphics */
			use_graphics = TRUE;
		}
	}
#endif /* USE_GRAPHICS */


	/* Init the Metadpy if possible */
	if (Metadpy_init_name(dpy_name)) return (-1);


	/* set OS-specific resize_main_window() hook */
	resize_main_window = resize_main_window_x11;

	enable_common_colormap_x11();

	/* Prepare color "xor" (for cursor) */
	MAKE(xor, infoclr);
	Infoclr_set (xor);
	Infoclr_init_ccn ("fg", "bg", "xor", 0);

	/* Prepare the colors (including "black") */
	for (i = 0; i < 16; ++i) {
		cptr cname = color_name[0];
		MAKE(clr[i], infoclr);
		Infoclr_set (clr[i]);
		if (Metadpy->color) cname = color_name[i];
		else if (i) cname = color_name[1];
		Infoclr_init_ccn (cname, "bg", "cpy", 0);
	}
#ifdef EXTENDED_BG_COLOURS
	/* Prepare the colors (including "black") */
	for (i = 0; i < 1; ++i) {
		cptr cname = color_name[0], cname2 = color_name[0];
		MAKE(clr[16 + i], infoclr);
		Infoclr_set (clr[16 + i]);
		if (Metadpy->color) {
			cname = color_ext_name[i][0];
			cname2 = color_ext_name[i][1];
		}
		Infoclr_init_ccn (cname, cname2, "cpy", 0);
	}
#endif

{ /* Main window is always visible */
	/* Check environment for "screen" font */
	fnt_name = getenv("TOMENET_X11_FONT_SCREEN");
	/* Check environment for "base" font */
	if (!fnt_name) fnt_name = getenv("TOMENET_X11_FONT");
	/* Use loaded (from config file) or predefined default font */
	if (!fnt_name) fnt_name = term_prefs[0].font;
	/* paranoia; use the default */
	if (!fnt_name) fnt_name = DEFAULT_X11_FONT_SCREEN;
	/* Initialize the screen */
//	term_data_init(0, &screen, TRUE, "TomeNET", fnt_name);
//	term_data_init(0, &screen, TRUE, ang_term_name[0], fnt_name);
	/* allow resizing, for font changes */
	term_data_init(0, &screen, FALSE, ang_term_name[0], fnt_name);
	term_screen = Term;
	ang_term[0] = Term;
}
#ifdef USE_GRAPHICS
	/* Load graphics */
	if (use_graphics) {
		XImage *tiles_raw;

		/* Load the graphics XXX XXX XXX */
		tiles_raw = ReadBMP(Metadpy->dpy, filename);

		/* Resize tiles */
		screen.tiles = ResizeImage(Metadpy->dpy, tiles_raw, 16, 16,
		                        screen.fnt->wid, screen.fnt->hgt);
	}
#endif /* USE_GRAPHICS */


#ifdef GRAPHIC_MIRROR
if (term_prefs[1].visible) {
	/* Check environment for "mirror" font */
	fnt_name = getenv("TOMENET_X11_FONT_MIRROR");
	/* Check environment for "base" font */
	if (!fnt_name) fnt_name = getenv("TOMENET_X11_FONT");
	/* Use loaded (from config file) or predefined default font */
	if (!fnt_name) fnt_name = term_prefs[1].font;
	/* paranoia; use the default */
	if (!fnt_name) fnt_name = DEFAULT_X11_FONT_MIRROR;

	/* Initialize the mirror window */
	term_data_init(1, &mirror, FALSE, ang_term_name[1], fnt_name);
	term_mirror = Term;
	ang_term[1] = Term;

}
#endif

#ifdef GRAPHIC_RECALL
if (term_prefs[2].visible) {
	/* Check environment for "recall" font */
	fnt_name = getenv("TOMENET_X11_FONT_RECALL");
	/* Check environment for "base" font */
	if (!fnt_name) fnt_name = getenv("TOMENET_X11_FONT");
	/* Use loaded (from config file) or predefined default font */
	if (!fnt_name) fnt_name = term_prefs[2].font;
	/* paranoia; use the default */
	if (!fnt_name) fnt_name = DEFAULT_X11_FONT_RECALL;

	/* Initialize the recall window */
	term_data_init(2, &recall, FALSE, ang_term_name[2], fnt_name);
	term_recall = Term;
	ang_term[2] = Term;

}
#endif

#ifdef GRAPHIC_CHOICE
if (term_prefs[3].visible) {
	/* Check environment for "choice" font */
	fnt_name = getenv("TOMENET_X11_FONT_CHOICE");
	/* Check environment for "base" font */
	if (!fnt_name) fnt_name = getenv("TOMENET_X11_FONT");
	/* Use loaded (from config file) or predefined default font */
	if (!fnt_name) fnt_name = term_prefs[3].font;
	/* paranoia; use the default */
	if (!fnt_name) fnt_name = DEFAULT_X11_FONT_CHOICE;

	/* Initialize the choice window */
	term_data_init(3, &choice, FALSE, ang_term_name[3], fnt_name);
	term_choice = Term;
	ang_term[3] = Term;
}
#endif

#ifdef GRAPHIC_TERM_4
if (term_prefs[4].visible) {
	/* Check environment for "term4" font */
	fnt_name = getenv("TOMENET_X11_FONT_TERM_4");
	/* Check environment for "base" font */
	if (!fnt_name) fnt_name = getenv("TOMENET_X11_FONT");
	/* Use loaded (from config file) or predefined default font */
	if (!fnt_name) fnt_name = term_prefs[4].font;
	/* paranoia; use the default */
	if (!fnt_name) fnt_name = DEFAULT_X11_FONT_TERM_4;

	/* Initialize the term_4 window */
	term_data_init(4, &term_4, FALSE, ang_term_name[4], fnt_name);
	term_term_4 = Term;
	ang_term[4] = Term;

}
#endif

#ifdef GRAPHIC_TERM_5
if (term_prefs[5].visible) {
	/* Check environment for "term5" font */
	fnt_name = getenv("TOMENET_X11_FONT_TERM_5");
	/* Check environment for "base" font */
	if (!fnt_name) fnt_name = getenv("TOMENET_X11_FONT");
	/* Use loaded (from config file) or predefined default font */
	if (!fnt_name) fnt_name = term_prefs[5].font;
	/* paranoia; use the default */
	if (!fnt_name) fnt_name = DEFAULT_X11_FONT_TERM_5;

	/* Initialize the term_5 window */
	term_data_init(5, &term_5, FALSE, ang_term_name[5], fnt_name);
	term_term_5 = Term;
	ang_term[5] = Term;

}
#endif

#ifdef GRAPHIC_TERM_6
if (term_prefs[6].visible) {
	/* Check environment for "term6" font */
	fnt_name = getenv("TOMENET_X11_FONT_TERM_6");
	/* Check environment for "base" font */
	if (!fnt_name) fnt_name = getenv("TOMENET_X11_FONT");
	/* Use loaded (from config file) or predefined default font */
	if (!fnt_name) fnt_name = term_prefs[6].font;
	/* paranoia; use the default */
	if (!fnt_name) fnt_name = DEFAULT_X11_FONT_TERM_6;

	/* Initialize the term_6 window */
	term_data_init(6, &term_6, FALSE, ang_term_name[6], fnt_name);
	term_term_6 = Term;
	ang_term[6] = Term;

}
#endif

#ifdef GRAPHIC_TERM_7
if (term_prefs[7].visible) {
	/* Check environment for "term7" font */
	fnt_name = getenv("TOMENET_X11_FONT_TERM_7");
	/* Check environment for "base" font */
	if (!fnt_name) fnt_name = getenv("TOMENET_X11_FONT");
	/* Use loaded (from config file) or predefined default font */
	if (!fnt_name) fnt_name = term_prefs[7].font;
	/* paranoia; use the default */
	if (!fnt_name) fnt_name = DEFAULT_X11_FONT_TERM_7;

	/* Initialize the term_7 window */
	term_data_init(7, &term_7, FALSE, ang_term_name[7], fnt_name);
	term_term_7 = Term;
	ang_term[7] = Term;

}
#endif


	/* Activate the "Angband" window screen */
	Term_activate(&screen.t);

	/* Raise the "Angband" window */
	Infowin_set(screen.outer);
	Infowin_raise();


	/* restore window coordinates from .tomenetrc */
	for (i = 0; i <= 7; i++) { /* MAX_TERM_DATA should be defined for X11 too.. */
		if (!term_prefs[i].visible) continue;
		if (term_prefs[i].x != -32000 && term_prefs[i].y != -32000) {
			XMoveWindow(Metadpy->dpy,
			    term_idx_to_term_data(i)->outer->win,
			    term_prefs[i].x,
			    term_prefs[i].y);
		}
	}


	/* Success */
	return (0);
}


#if 0
/* Turn off num-lock if it's on */
void turn_off_numlock_X11(void) {
	Display* disp = XOpenDisplay(NULL);
	if (disp == NULL) return; /* Error */

	XTestFakeKeyEvent(disp, XKeysymToKeycode(disp, XK_Num_Lock), True, 0);
	XTestFakeKeyEvent(disp, XKeysymToKeycode(disp, XK_Num_Lock), False, 0);
	XFlush(disp);
	XCloseDisplay(disp);
}
#endif


#if 1 /* CHANGE_FONTS_X11 */
/* EXPERIMENTAL // allow user to change main window font live - C. Blue
   So far only uses 1 parm ('s') to switch between hardcoded choices:
   -1 - cycle
    0 - normal
    1 - big
    2 - bigger
    3 - huge */
/* only offer 2 cycling stages? */
#define REDUCED_FONT_CHOICE
static void term_force_font(int term_idx, char fnt_name[256]);
void change_font(int s) {
	/* use main window font for measuring */
	char tmp[128] = "";
	if (screen.fnt->name) strcpy(tmp, screen.fnt->name);
	else strcpy(tmp, DEFAULT_X11_FONT);

	/* cycle? */
	if (s == -1) {
#ifdef REDUCED_FONT_CHOICE
		if (strstr(tmp, "8x13")) s = 1;
		else if (strstr(tmp, "9x15")) s = 0;
#else
		if (strstr(tmp, "8x13") || strstr(tmp, "lucidasanstypewriter-8")) s = 1;
		else if (strstr(tmp, "lucidasanstypewriter-10")) s = 2;
#endif
		else if (strstr(tmp, "lucidasanstypewriter-12")) s = 3;
		else if (strstr(tmp, "lucidasanstypewriter-18")) s = 0;
	}

	/* Force the font */
	switch (s) {
	case 0:
		/* change main window font */
#ifdef REDUCED_FONT_CHOICE
		term_force_font(0, "8x13");
#else
		term_force_font(0, "lucidasanstypewriter-8");
#endif
		/* Change sub windows too */
		term_force_font(1, "8x13"); //msg
		term_force_font(2, "8x13"); //inv
		term_force_font(3, "5x8"); //char
		term_force_font(4, "6x10"); //chat
		term_force_font(5, "6x10"); //eq (5x8)
		term_force_font(6, "5x8");
		term_force_font(7, "5x8");
		break;
	case 1:
		/* change main window font */
#ifdef REDUCED_FONT_CHOICE
		term_force_font(0, "9x15");//was 10x14x
#else
		term_force_font(0, "lucidasanstypewriter-10");
#endif
		/* Change sub windows too */
		term_force_font(1, "9x15");
		term_force_font(2, "9x15");
		term_force_font(3, "6x10");
		term_force_font(4, "8x13");
		term_force_font(5, "6x10");
		term_force_font(6, "6x10");
		term_force_font(7, "6x10");
		break;
	case 2:
		/* change main window font */
		term_force_font(0, "lucidasanstypewriter-12");
		/* Change sub windows too */
		term_force_font(1, "9x15");
		term_force_font(2, "9x15");
		term_force_font(3, "8x13");
		term_force_font(4, "9x15");
		term_force_font(5, "8x13");
		term_force_font(6, "8x13");
		term_force_font(7, "8x13");
		break;
	case 3:
		/* change main window font */
		term_force_font(0, "lucidasanstypewriter-18");
		/* Change sub windows too */
		term_force_font(1, "lucidasanstypewriter-12");
		term_force_font(2, "lucidasanstypewriter-12");
		term_force_font(3, "9x15");
		term_force_font(4, "lucidasanstypewriter-12");
		term_force_font(5, "9x15");
		term_force_font(6, "9x15");
		term_force_font(7, "9x15");
		break;
	}
}
static void term_force_font(int term_idx, char fnt_name[256]) {
	int rows, cols, wid, hgt;
	term_data *td = term_idx_to_term_data(term_idx);

	/* non-visible window has no fnt-> .. */
	if (!term_prefs[term_idx].visible) return;


	/* special hack: this window was invisible, but we just toggled
	   it to become visible on next client start. - C. Blue */
	if (!td->fnt) return;


	/* Detemine "proper" number of rows/cols */
	cols = ((Infowin->w - 2) / td->fnt->wid);
	rows = ((Infowin->h - 2) / td->fnt->hgt);

#if 0
	/* Hack -- do not allow resize of main screen */
	if (td == &screen) cols = 80;
	if (td == &screen) rows = 24;
#endif

	/* Hack -- minimal size */
	if (cols < 1) cols = 1;
	if (rows < 1) rows = 1;

	/* font change */
	Infofnt_set(td->fnt);
	Infofnt_init_data(fnt_name);

	/* Desired size of "outer" window */
	wid = cols * td->fnt->wid;
	hgt = rows * td->fnt->hgt;
	Infowin_set(td->outer);
	/* Resize the windows if any "change" is needed */
	if ((Infowin->w != wid + 2) || (Infowin->h != hgt + 2)) {
		Infowin_set(td->outer);
		Infowin_resize(wid + 2, hgt + 2);
		Infowin_set(td->inner);
		Infowin_resize(wid, hgt);
	}
	XFlush(Metadpy->dpy);

	/* Reload custom font prefs on main screen font change */
	if (td == &screen) handle_process_font_file();
}
#endif

/* For saving window positions and sizes on quitting:
   Returns x,y,cols,rows,font name. */
void x11win_getinfo(int term_idx, int *x, int *y, int *c, int *r, char *fnt_name) {
	term_data *td = term_idx_to_term_data(term_idx);
	infowin *iwin;
	Window xid, tmp_win;
	unsigned int wu, hu, bu, du;
#if 0
	XWindowAttributes xwa;
#endif
	Atom property;
	Atom type_return;
	int format_return;
	unsigned long nitems_return;
	unsigned long bytes_after_return;
	unsigned char *data;
	int x_rel, y_rel;
	char got_frame_extents = FALSE;

	/* non-visible window has no window info .. */
	if (!term_prefs[term_idx].visible) {
		*x = *y = *c = *r = 0;
		return;
	}


	/* special hack: this window was invisible, but we just toggled
	   it to become visible on next client start. - C. Blue */
	if (!td->fnt) {
		*x = *y = *c = *r = 0;
		return;
	}


	strcpy(fnt_name, td->fnt->name);

	iwin = td->outer;
	Infowin_set(iwin);
	xid = iwin->win;

	/* Detemine number of rows/cols */
	*c = ((iwin->w - 2) / td->fnt->wid);
	*r = ((iwin->h - 2) / td->fnt->hgt);

	property = XInternAtom(Metadpy->dpy, "_NET_FRAME_EXTENTS", True);

	/* Try to use _NET_FRAME_EXTENTS to get the window borders */
	if (property != None && XGetWindowProperty(Metadpy->dpy, xid, property,
	    0, LONG_MAX, False, XA_CARDINAL, &type_return, &format_return,
	    &nitems_return, &bytes_after_return, &data) == Success) {
		if ((type_return == XA_CARDINAL) && (format_return == 32) && (nitems_return == 4) && (data)) {
			unsigned long *ldata = (unsigned long *)data;
			got_frame_extents = TRUE;
			/* _NET_FRAME_EXTENTS format is left, right, top, bottom */
			x_rel = ldata[0];
			y_rel = ldata[2];
//			printf("FRAME_EXTENTS: x_rel = %d, y_rel = %d\n", x_rel, y_rel);
		}

		if (data) XFree(data);
	}

#if 0
	/* Check Error XXX Extract some more ACTUAL data */
	XGetWindowAttributes(Metadpy->dpy, xid, &xwa);
//	x_rel = xwa.x;
//	y_rel = xwa.y;
//	printf("XGetWindowAttributes: x = %d, y = %d, width = %d, height = %d\n", (int)xwa.x, (int)xwa.y, (int)xwa.width, (int)xwa.height);
#endif

	if (!got_frame_extents) {
		/* Check For Error XXX Extract some ACTUAL data from 'xid' */
		if (XGetGeometry(Metadpy->dpy, xid, &tmp_win, &x_rel, &y_rel, &wu, &hu, &bu, &du)) {
//			*w = (int)wu;
//			*h = (int)hu;
//			printf("XGetGeometry: x = %d, y = %d\n", x_rel, y_rel);
		} else {
			x_rel = 0;
			y_rel = 0;
		}
	}

	XTranslateCoordinates(Metadpy->dpy, xid, Metadpy->root, 0, 0, x, y, &tmp_win);
//	printf("XTranslateCoordinates: x = %d, y = %d\n", *x, *y);

	/* correct window position to account for X11 border/title bar sizes */
	*x -= x_rel;
	*y -= y_rel;
}

void resize_main_window_x11(int cols, int rows) {
	int wid, hgt;
	term_data *td = term_idx_to_term_data(0);
	term *t = ang_term[0]; //&screen

	term_prefs[0].columns = cols; //screen_wid + (SCREEN_PAD_X);
	term_prefs[0].lines = rows; //screen_hgt + (SCREEN_PAD_Y);

	wid = cols * td->fnt->wid;
	hgt = rows * td->fnt->hgt;

	/* Resize the windows if any "change" is needed */
	Infowin_set(td->outer);
	if ((Infowin->w != wid + 2) || (Infowin->h != hgt + 2)) {
		Infowin_set(td->outer);
		Infowin_resize(wid + 2, hgt + 2);
		Infowin_set(td->inner);
		Infowin_resize(wid, hgt);

		Term_activate(t);
		Term_resize(cols, rows);
	}
}

bool ask_for_bigmap(void) {
	return ask_for_bigmap_generic();
}

const char* get_font_name(int term_idx) {
	term_data *td = term_idx_to_term_data(term_idx);
	if (td->fnt) return td->fnt->name;
	else return DEFAULT_X11_FONT;
}
void set_font_name(int term_idx, char* fnt) {
	term_force_font(term_idx, fnt);
}
void term_toggle_visibility(int term_idx) {
	term_prefs[term_idx].visible = !term_prefs[term_idx].visible;
	/* NOTE: toggling visible flag of a window during runtime is dangerous:
	   Two "special hack"s were added in term_force_font() and x11win_getinfo()
	   to accomodate for this and to continue to treat the window as invisible. */
}
bool term_get_visibility(int term_idx) {
	return term_prefs[term_idx].visible;
}

/* automatically store name+password to ini file if we're a new player? */
void store_crecedentials(void) {
	write_mangrc(TRUE);
}
void get_screen_font_name(char *buf) {
	/* fonts aren't available in command-line mode */
	if (!strcmp(ANGBAND_SYS, "gcu")) {
		buf[0] = 0;
		return;
	}

	if (screen.fnt->name) strcpy(buf, screen.fnt->name);
	else strcpy(buf, "");
}
#endif
