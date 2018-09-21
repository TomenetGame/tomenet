/* File: main-win.c */

/* Purpose: Support for Windows Angband */

/*
 * Written (2.7.8) by Skirmantas Kligys (kligys@scf.usc.edu)
 *
 * Based loosely on "main-mac.c" and "main-xxx.c" and suggestions
 * by Ben Harrison (benh@voicenet.com).
 *
 * Upgrade to Angband 2.7.9v6 by Ben Harrison (benh@voicenet.com),
 * Ross E Becker (beckerr@cis.ohio-state.edu), and Chris R. Martin
 * (crm7479@tam2000.tamu.edu).
 *
 * Note that the "Windows" version requires several extra files, which
 * must be placed in various places.  These files are distributed in a
 * special "ext-win.zip" archive, with instructions on where to place
 * the various files.  For example, we require that all font files,
 * bitmap files, and sound files, be placed into the "lib/xtra/font/",
 * "/lib/xtra/graf/", and "lib/xtra/sound/" directories, respectively.
 *
 * See "h-config.h" for the extraction of the "WINDOWS" flag based
 * on the "_Windows", "__WINDOWS__", "__WIN32__", "WIN32", "__WINNT__",
 * or "__NT__" flags.  If your compiler uses a different compiler flag,
 * add it to "h-config.h", or, simply place it in the "Makefile".
 *
 *
 * This file still needs some work, possible problems are indicated by
 * the string "XXX XXX XXX" in any comment.
 *
 * XXX XXX XXX
 * The "Term_xtra_win_clear()" function should probably do a low-level
 * clear of the current window, and redraw the borders and other things.
 *
 * XXX XXX XXX
 * The "use_graphics" option should affect ALL of the windows, not just
 * the main "screen" window.  The "FULL_GRAPHICS" option is supposed to
 * enable this behavior, but it will load one bitmap file for each of
 * the windows, which may take a lot of memory, especially since with
 * a little clever code, we could load each bitmap only when needed,
 * and only free it once nobody is using it any more.
 *
 * XXX XXX XXX
 * The user should be able to select the "bitmap" for a window independant
 * from the "font", and should be allowed to select any bitmap file which
 * is strictly smaller than the font file.  Currently, bitmap selection
 * always imitates the font selection unless the "Font" and "Graf" lines
 * are used in the "ANGBAND.INI" file.  This may require the addition
 * of a menu item for each window to select the bitmaps.
 *
 * XXX XXX XXX
 * The various "warning" messages assume the existence of the "screen.w"
 * window, I think, and only a few calls actually check for its existence,
 * this may be okay since "NULL" means "on top of all windows". (?)
 *
 * XXX XXX XXX
 * Special "Windows Help Files" can be placed into "lib/xtra/help/" for
 * use with the "winhelp.exe" program.  These files *may* be available
 * at the ftp site somewhere.
 *
 * XXX XXX XXX
 * The "prepare menus" command should "gray out" any menu command which
 * is not allowed at the current time.  This will simplify the actual
 * processing of menu commands, which can assume the command is legal.
 *
 * XXX XXX XXX
 * Remove the old "FontFile" entries from the "*.ini" file, and remove
 * the entire section about "sounds", after renaming a few sound files.
 */


#include "angband.h"

#ifdef WINDOWS

/* for SendInput() */
#include <winuser.h>

#define MNU_SUPPORT
/* but do we actually create a menu? */
//#define MNU_USE
/* #define USE_GRAPHICS */

#ifdef JP
#  ifndef USE_LOGFONT
#    define USE_LOGFONT
#  endif
#endif

#ifdef USE_LOGFONT
# if 0 /* too small */
#  define DEFAULT_FONTNAME "8X13"
# else /* fitting for full-hd */
#  define DEFAULT_FONTNAME "9X15"
# endif
#  undef  USE_GRAPHICS
#else
# if 0 /* too small */
#  define DEFAULT_FONTNAME "8X13.FON"
#  define DEFAULT_TILENAME "8X13.BMP"
# else /* fitting for full-hd */
#  define DEFAULT_FONTNAME "9X15.FON"
#  define DEFAULT_TILENAME "9X15.BMP"
# endif
#endif

#define OPTIMIZE_DRAWING


/*
 * Extract the "WIN32" flag from the compiler
 */
#if defined(__WIN32__) || defined(__WINNT__) || defined(__NT__)
# ifndef WIN32
#  define WIN32
# endif
#endif

/*
 * XXX XXX XXX Hack -- broken sound libraries
 */
#ifdef BEN_HACK
# undef USE_SOUND
#endif


/*
 * Menu constants -- see "ANGBAND.RC"
 */
 
#define IDM_FILE_NEW			101
#define IDM_FILE_OPEN			102
#define IDM_FILE_SAVE			103
#define IDM_FILE_EXIT			104
#define IDM_FILE_QUIT			105

#define IDM_TEXT_SCREEN			201
#define IDM_TEXT_MIRROR			202
#define IDM_TEXT_RECALL			203
#define IDM_TEXT_CHOICE			204
#define IDM_TEXT_TERM_4			205
#define IDM_TEXT_TERM_5			206
#define IDM_TEXT_TERM_6			207
#define IDM_TEXT_TERM_7			208

#define IDM_WINDOWS_SCREEN		211
#define IDM_WINDOWS_MIRROR		212
#define IDM_WINDOWS_RECALL		213
#define IDM_WINDOWS_CHOICE		214
#define IDM_WINDOWS_TERM_4		215
#define IDM_WINDOWS_TERM_5		216
#define IDM_WINDOWS_TERM_6		217
#define IDM_WINDOWS_TERM_7		218

#define IDM_OPTIONS_GRAPHICS	221
#define IDM_OPTIONS_SOUND		222
#define IDM_OPTIONS_UNUSED		231
#define IDM_OPTIONS_SAVER		232
#define IDM_OPTIONS_SAVEPREF	241

#define IDM_HELP_GENERAL		901
#define IDM_HELP_SPOILERS		902


/*
 * This may need to be removed for some compilers XXX XXX XXX
 */
#ifndef STRICT
#define STRICT
#endif

/*
 * exclude parts of WINDOWS.H that are not needed
 */
#define NOSOUND           /* Sound APIs and definitions */
#define NOCOMM            /* Comm driver APIs and definitions */
#define NOLOGERROR        /* LogError() and related definitions */
#define NOPROFILER        /* Profiler APIs */
#define NOLFILEIO         /* _l* file I/O routines */
#define NOOPENFILE        /* OpenFile and related definitions */
#define NORESOURCE        /* Resource management */
#define NOATOM            /* Atom management */
#define NOLANGUAGE        /* Character test routines */
#define NOLSTRING         /* lstr* string management routines */
#define NODBCS            /* Double-byte character set routines */
#define NOKEYBOARDINFO    /* Keyboard driver routines */
#define NOCOLOR           /* COLOR_* color values */
#define NODRAWTEXT        /* DrawText() and related definitions */
#define NOSCALABLEFONT    /* Truetype scalable font support */
#define NOMETAFILE        /* Metafile support */
#define NOSYSTEMPARAMSINFO /* SystemParametersInfo() and SPI_* definitions */
#define NODEFERWINDOWPOS  /* DeferWindowPos and related definitions */
#define NOKEYSTATES       /* MK_* message key state flags */
#define NOWH              /* SetWindowsHook and related WH_* definitions */
#define NOCLIPBOARD       /* Clipboard APIs and definitions */
#define NOICONS           /* IDI_* icon IDs */
#define NOMDI             /* MDI support */
#define NOCTLMGR          /* Control management and controls */
#define NOHELP            /* Help support */

/*
 * exclude parts of WINDOWS.H that are not needed (Win32)
 */
#define WIN32_LEAN_AND_MEAN
#define NONLS             /* All NLS defines and routines */
#define NOSERVICE         /* All Service Controller routines, SERVICE_ equates, etc. */
#define NOKANJI           /* Kanji support stuff. */
#define NOMCX             /* Modem Configuration Extensions */

/*
 * Include the "windows" support file
 */
#include <winsock.h>	/* includes windows.h */


/*
 * exclude parts of MMSYSTEM.H that are not needed
 */
#define MMNODRV          /* Installable driver support */
#define MMNOWAVE         /* Waveform support */
#define MMNOMIDI         /* MIDI support */
#define MMNOAUX          /* Auxiliary audio support */
#define MMNOTIMER        /* Timer support */
#define MMNOJOY          /* Joystick support */
#define MMNOMCI          /* MCI support */
#define MMNOMMIO         /* Multimedia file I/O support */
#define MMNOMMSYSTEM     /* General MMSYSTEM functions */

/*
 * Include the ??? files
 */
//#include <mmsystem.h>
#include <commdlg.h>

/*
 * Include the support for loading bitmaps
 */
#ifdef USE_GRAPHICS
# include "readdib.h"
#endif

/*
 * Cannot include "dos.h", so we define some things by hand.
 */
#ifdef WIN32
#define INVALID_FILE_NAME (DWORD)0xFFFFFFFF
#else /* WIN32 */
#define FA_LABEL    0x08        /* Volume label */
#define FA_DIREC    0x10        /* Directory */
unsigned _cdecl _dos_getfileattr(const char *, unsigned *);
#endif /* WIN32 */

/*
 * Silliness in WIN32 drawing routine
 */
#ifdef WIN32
# define MoveTo(H,X,Y) MoveToEx(H, X, Y, NULL)
#endif /* WIN32 */

/*
 * Silliness for Windows 95
 */
#ifndef WS_EX_TOOLWINDOW
# define WS_EX_TOOLWINDOW 0
#endif

/*
 * Foreground color bits (hard-coded by DOS)
 */
#define VID_BLACK	0x00
#define VID_BLUE	0x01
#define VID_GREEN	0x02
#define VID_CYAN	0x03
#define VID_RED		0x04
#define VID_MAGENTA	0x05
#define VID_YELLOW	0x06
#define VID_WHITE	0x07

/*
 * Bright text (hard-coded by DOS)
 */
#define VID_BRIGHT	0x08

/*
 * Background color bits (hard-coded by DOS)
 */
#define VUD_BLACK	0x00
#define VUD_BLUE	0x10
#define VUD_GREEN	0x20
#define VUD_CYAN	0x30
#define VUD_RED		0x40
#define VUD_MAGENTA	0x50
#define VUD_YELLOW	0x60
#define VUD_WHITE	0x70

/*
 * Blinking text (hard-coded by DOS)
 */
#define VUD_BRIGHT	0x80


/* Optimize palette-animation? [EXTENDED_COLOURS_PALANIM] */
#define PALANIM_OPTIMIZED /* KEEP SAME AS SERVER! */
#ifdef PALANIM_OPTIMIZED
 #define PALANIM_OPTIMIZED2 /* Will batch colour updates together */
#endif



/* Swap colour indices used for palette animation with their originals.
   For some reason this may be required on Windows:
   The colours 0-15 can be animated just fine and flicker-free.
   The colours 16-31 cause flickering on animation.
   So on Win we just do it the opposite way round and animate 0-15 and
   use 16-31 for static colours. - C. Blue */
//#define PALANIM_SWAP


void resize_main_window_win(int cols, int rows);


/*
 * Forward declare
 */
typedef struct _term_data term_data;

/*
 * Extra "term" data
 *
 * Note the use of "font_want" and "graf_want" for the names of the
 * font/graf files requested by the user, and the use of "font_file"
 * and "graf_file" for the currently active font/graf files.
 *
 * The "font_file" and "graf_file" are capitilized, and are of the
 * form "8X13.FON" and "8X13.BMP", while "font_want" and "graf_want"
 * can be in almost any form as long as it could be construed as
 * attempting to represent the name of a font or bitmap or file.
 */
struct _term_data
{
	term     t;

	cptr     s;

	HWND     w;

#ifdef USE_GRAPHICS
	DIBINIT infGraph;
#endif

	DWORD    dwStyle;
	DWORD    dwExStyle;

	uint     keys;

	uint     rows;
	uint     cols;

	uint     pos_x;
	uint     pos_y;
	uint     size_wid;
	uint     size_hgt;
	uint     client_wid;
	uint     client_hgt;
	uint     size_ow1;
	uint     size_oh1;
	uint     size_ow2;
	uint     size_oh2;

	byte     visible;

	byte     size_hack;

	cptr     font_want;
	cptr     graf_want;

	cptr     font_file;
	cptr     graf_file;

	HFONT    font_id;

	uint     font_wid;
	uint     font_hgt;

#ifdef USE_LOGFONT
	LOGFONT  lf;
#endif
};


/*
 * Maximum number of windows XXX XXX XXX XXX
 */
#define MAX_TERM_DATA 8

/*
 * An array of term_data's
 */
static term_data data[MAX_TERM_DATA];

/*
 * Mega-Hack -- global "window creation" pointer
 */
static term_data *td_ptr;

/*
 * Various boolean flags
 */
bool game_in_progress  = FALSE;  /* game in progress */
bool initialized       = FALSE;  /* note when "open"/"new" become valid */
bool paletted          = FALSE;  /* screen paletted, i.e. 256 colors */
bool colors16          = FALSE;  /* 16 colors screen, don't use RGB() */

/*
 * Saved instance handle
 */
static HINSTANCE hInstance;

/*
 * Yellow brush for the cursor
 */
static HBRUSH hbrYellow;

/*
 * An icon
 */
static HICON hIcon;

/*
 * A palette
 */
static HPALETTE hPal;

/*
 * An array of sound file names
 */
static cptr sound_file[SOUND_MAX];

/*
 * Full path to ANGBAND.INI
 */
static cptr ini_file = NULL;

/*
 * Name of application
 */
static cptr AppName  = "TOMENET";

/*
 * Name of sub-window type
 */
static cptr AngList  = "AngList";

/*
 * Directory names
 */
static cptr ANGBAND_DIR_XTRA_FONT;
#ifdef USE_GRAPHICS
static cptr ANGBAND_DIR_XTRA_GRAF;
#endif
#ifdef USE_SOUND
#ifndef USE_SOUND_2010
static cptr ANGBAND_DIR_XTRA_SOUND;
#endif
#endif

/*
 * The Angband color set:
 *   Black, White, Slate, Orange,    Red, Blue, Green, Umber
 *   D-Gray, L-Gray, Violet, Yellow, L-Red, L-Blue, L-Green, L-Umber
 *
 * Colors 8 to 15 are basically "enhanced" versions of Colors 0 to 7.
 * Note that on B/W machines, all non-zero colors can be white (on black).
 *
 * Note that all characters are assumed to be drawn on a black background.
 * This may require calling "Term_wipe()" before "Term_text()", etc.
 *
 * XXX XXX XXX See "main-ibm.c" for a method to allow color editing
 *
 * XXX XXX XXX The color codes below were taken from "main-ibm.c".
 */
#ifndef EXTENDED_COLOURS_PALANIM
 static COLORREF win_clr[16] = {
#else
 #ifdef PALANIM_OPTIMIZED2
  static COLORREF win_clr_buf[16 * 2];
 #endif
 static COLORREF win_clr[16 * 2] = {
#endif
	PALETTERGB(0x00, 0x00, 0x00),  /* 0 0 0  Dark */
	PALETTERGB(0xFF, 0xFF, 0xFF),  /* 4 4 4  White */
	PALETTERGB(0x9D, 0x9D, 0x9D),  /* 2 2 2  Slate */
	PALETTERGB(0xFF, 0x8D, 0x00),  /* 4 2 0  Orange */
	PALETTERGB(0xB7, 0x00, 0x00),  /* 3 0 0  Red (was 0xD7,0,0) - making shaman/istar more distinguishable */
	PALETTERGB(0x00, 0x9D, 0x44),  /* 0 2 1  Green */
#ifndef READABILITY_BLUE
	PALETTERGB(0x00, 0x00, 0xFF),  /* 0 0 4  Blue */
#else
	PALETTERGB(0x00, 0x33, 0xFF),  /* 0 0 4  Blue */
#endif
	PALETTERGB(0x8D, 0x66, 0x00),  /* 2 1 0  Umber */
#ifndef DISTINCT_DARK
	PALETTERGB(0x74, 0x74, 0x74),  /* 1 1 1  Lt. Dark */
#else
	//PALETTERGB(0x58, 0x58, 0x58),  /* 1 1 1  Lt. Dark */
	PALETTERGB(0x66, 0x66, 0x66),  /* 1 1 1  Lt. Dark */
#endif
	PALETTERGB(0xD7, 0xD7, 0xD7),  /* 3 3 3  Lt. Slate */
	PALETTERGB(0xAF, 0x00, 0xFF),  /* 4 0 4  Violet (was 2,0,2) */
	PALETTERGB(0xFF, 0xFF, 0x00),  /* 4 4 0  Yellow */
	PALETTERGB(0xFF, 0x30, 0x30),  /* 4 0 0  Lt. Red (was 0xFF,0,0) - see 'Red' - C. Blue */
	PALETTERGB(0x00, 0xFF, 0x00),  /* 0 4 0  Lt. Green */
	PALETTERGB(0x00, 0xFF, 0xFF),  /* 0 4 4  Lt. Blue */
	PALETTERGB(0xC7, 0x9D, 0x55)   /* 3 2 1  Lt. Umber */
#ifdef EXTENDED_COLOURS_PALANIM
	/* And clone the above 16 standard colours again here: */
	,
	PALETTERGB(0x00, 0x00, 0x00),  /* 0 0 0  Dark */
	PALETTERGB(0xFF, 0xFF, 0xFF),  /* 4 4 4  White */
	PALETTERGB(0x9D, 0x9D, 0x9D),  /* 2 2 2  Slate */
	PALETTERGB(0xFF, 0x8D, 0x00),  /* 4 2 0  Orange */
	PALETTERGB(0xB7, 0x00, 0x00),  /* 3 0 0  Red (was 0xD7,0,0) - making shaman/istar more distinguishable */
	PALETTERGB(0x00, 0x9D, 0x44),  /* 0 2 1  Green */
 #ifndef READABILITY_BLUE
	PALETTERGB(0x00, 0x00, 0xFF),  /* 0 0 4  Blue */
 #else
	PALETTERGB(0x00, 0x33, 0xFF),  /* 0 0 4  Blue */
 #endif
	PALETTERGB(0x8D, 0x66, 0x00),  /* 2 1 0  Umber */
 #ifndef DISTINCT_DARK
	PALETTERGB(0x74, 0x74, 0x74),  /* 1 1 1  Lt. Dark */
 #else
	//PALETTERGB(0x58, 0x58, 0x58),  /* 1 1 1  Lt. Dark */
	PALETTERGB(0x66, 0x66, 0x66),  /* 1 1 1  Lt. Dark */
 #endif
	PALETTERGB(0xD7, 0xD7, 0xD7),  /* 3 3 3  Lt. Slate */
	PALETTERGB(0xAF, 0x00, 0xFF),  /* 4 0 4  Violet (was 2,0,2) */
	PALETTERGB(0xFF, 0xFF, 0x00),  /* 4 4 0  Yellow */
	PALETTERGB(0xFF, 0x30, 0x30),  /* 4 0 0  Lt. Red (was 0xFF,0,0) - see 'Red' - C. Blue */
	PALETTERGB(0x00, 0xFF, 0x00),  /* 0 4 0  Lt. Green */
	PALETTERGB(0x00, 0xFF, 0xFF),  /* 0 4 4  Lt. Blue */
	PALETTERGB(0xC7, 0x9D, 0x55)   /* 3 2 1  Lt. Umber */
#endif
};
void enable_readability_blue_win(void) {
	client_color_map[6] = 0x0033ff;
}
static void enable_common_colormap_win(void) {
	int i;

	#define RED(i)   (client_color_map[i] >> 16 & 0xFF)
	#define GREEN(i) (client_color_map[i] >> 8 & 0xFF)
	#define BLUE(i)  (client_color_map[i] & 0xFF)

#ifndef EXTENDED_COLOURS_PALANIM
	for (i = 0; i < 16; i++) {
#else
	for (i = 0; i < 16 * 2; i++) {
#endif
		win_clr[i] = PALETTERGB(RED(i), GREEN(i), BLUE(i));
#ifdef PALANIM_OPTIMIZED2
		win_clr_buf[i] = win_clr[i];
#endif
	}
}


/*
 * Palette indices for 16 colors
 *
 * See "main-ibm.c" for original table information
 *
 * The entries below are taken from the "color bits" defined above.
 *
 * Note that many of the choices below suck, but so do crappy monitors.
 */
static BYTE win_pal[16] =
{
	VID_BLACK,			/* Dark */
//	VID_WHITE,			/* White */
VID_WHITE | VID_BRIGHT,
//	VID_CYAN,			/* Slate XXX */
VID_BLUE | VID_BRIGHT,
	VID_RED | VID_BRIGHT,	/* Orange XXX */
	VID_RED,			/* Red */
	VID_GREEN,			/* Green */
	VID_BLUE,			/* Blue */
	VID_YELLOW,			/* Umber XXX */
	VID_BLACK | VID_BRIGHT,	/* Light Dark */
//	VID_CYAN | VID_BRIGHT,	/* Light Slate XXX */
VID_WHITE,
	VID_MAGENTA,		/* Violet XXX */
	VID_YELLOW | VID_BRIGHT,	/* Yellow */
	VID_MAGENTA | VID_BRIGHT,	/* Light Red XXX */
	VID_GREEN | VID_BRIGHT,	/* Light Green */
	VID_BLUE | VID_BRIGHT,	/* Light Blue */
	VID_YELLOW			/* Light Umber XXX */
};


#ifdef OPTIMIZE_DRAWING
HDC oldDC = NULL;
byte old_attr = -1;
#endif



/*
 * Hack -- given a pathname, point at the filename
 */
static cptr extract_file_name(cptr s) {
	cptr p;

	/* Start at the end */
	p = s + strlen(s) - 1;

	/* Back up to divider */
	while ((p >= s) && (*p != ':') && (*p != '\\')) p--;

	/* Return file name */
	return (p+1);
}



/*
 * Check for existence of a file
 */
static bool check_file(cptr s) {
	char path[1024];

#ifdef WIN32

	DWORD attrib;

#else /* WIN32 */

	unsigned int attrib;

#endif /* WIN32 */

	/* Copy it */
	strcpy(path, s);

#ifdef WIN32

	/* Examine */
	attrib = GetFileAttributes(path);

	/* Require valid filename */
	if (attrib == INVALID_FILE_NAME) return (FALSE);

	/* Prohibit directory */
	if (attrib & FILE_ATTRIBUTE_DIRECTORY) return (FALSE);

#else /* WIN32 */

	/* Examine and verify */
	if (_dos_getfileattr(path, &attrib)) return (FALSE);

	/* Prohibit something */
	if (attrib & FA_LABEL) return (FALSE);

	/* Prohibit directory */
	if (attrib & FA_DIREC) return (FALSE);

#endif /* WIN32 */

	/* Success */
	return (TRUE);
}


/*
 * Check for existence of a directory
 */
bool check_dir(cptr s) {
	int i;
	char path[1024];

#ifdef WIN32

	DWORD attrib;

#else /* WIN32 */

	unsigned int attrib;

#endif /* WIN32 */

	/* Copy it */
	strcpy(path, s);

	/* Check length */
	i = strlen(path);

	/* Remove trailing backslash */
	if (i && (path[i-1] == '\\')) path[--i] = '\0';

#ifdef WIN32

	/* Examine */
	attrib = GetFileAttributes(path);

	/* Require valid filename */
	if (attrib == INVALID_FILE_NAME) return (FALSE);

	/* Require directory */
	if (!(attrib & FILE_ATTRIBUTE_DIRECTORY)) return (FALSE);

#else /* WIN32 */

	/* Examine and verify */
	if (_dos_getfileattr(path, &attrib)) return (FALSE);

	/* Prohibit something */
	if (attrib & FA_LABEL) return (FALSE);

	/* Require directory */
	if (!(attrib & FA_DIREC)) return (FALSE);

#endif /* WIN32 */

	/* Success */
	return (TRUE);
}


/*
 * Validate a file
 */
static void validate_file(cptr s) {
	/* Verify or fail */
	if (!check_file(s))
		quit_fmt("Cannot find required file:\n%s", s);
}


/*
 * Validate a directory
 */
static void validate_dir(cptr s) {
	/* Verify or fail */
	if (!check_dir(s))
		quit_fmt("Cannot find required directory:\n%s", s);
}


/*
 * Get the "size" for a window
 */
static void term_getsize(term_data *td) {
	RECT        rc;

	/* Paranoia */
	if (td->cols < 1) td->cols = 1;
	if (td->rows < 1) td->rows = 1;

	/* Paranoia */
	if (td->cols > (MAX_WINDOW_WID)) td->cols = (MAX_WINDOW_WID);
	if (td->rows > (MAX_WINDOW_HGT)) td->rows = (MAX_WINDOW_HGT);

	/* Window sizes */
	td->client_wid = td->cols * td->font_wid + td->size_ow1 + td->size_ow2;
	td->client_hgt = td->rows * td->font_hgt + td->size_oh1 + td->size_oh2;

	/* Fake window size */
	rc.left = rc.top = 0;
	rc.right = rc.left + td->client_wid;
	rc.bottom = rc.top + td->client_hgt;

	/* Adjust */
#ifdef MNU_USE
	if (td == &data[0]) AdjustWindowRectEx(&rc, td->dwStyle, TRUE, td->dwExStyle);
	else
#endif
	AdjustWindowRectEx(&rc, td->dwStyle, FALSE, td->dwExStyle);

	/* Total size */
	td->size_wid = rc.right - rc.left;
	td->size_hgt = rc.bottom - rc.top;

	/* See CreateWindowEx */
	if (!td->w) return;

	/* Extract actual location */
	GetWindowRect(td->w, &rc);

	/* Save the location */
	td->pos_x = rc.left;
	td->pos_y = rc.top;
}


/*
 * Write the "preference" data for single term
 */
static void save_prefs_aux(term_data *td, cptr sec_name) {
	char buf[32];

	/* Visible (Sub-windows) */
	if (td != &data[0]) {
		strcpy(buf, td->visible ? "1" : "0");
		WritePrivateProfileString(sec_name, "Visible", buf, ini_file);
	}

#ifdef USE_LOGFONT
	WritePrivateProfileString(sec_name, "Font", *(td->lf.lfFaceName) ? td->lf.lfFaceName : DEFAULT_FONTNAME, ini_file);
	wsprintf(buf, "%d", td->lf.lfWidth);
	WritePrivateProfileString(sec_name, "FontWid", buf, ini_file);
	wsprintf(buf, "%d", td->lf.lfHeight);
	WritePrivateProfileString(sec_name, "FontHgt", buf, ini_file);
	wsprintf(buf, "%d", td->lf.lfWeight);
	WritePrivateProfileString(sec_name, "FontWgt", buf, ini_file);
#else
	/* Desired font */
	if (td->font_file)
		WritePrivateProfileString(sec_name, "Font", td->font_file, ini_file);
#endif

	/* Desired graf */
	if (td->graf_file)
		WritePrivateProfileString(sec_name, "Graf", td->graf_file, ini_file);

	if (td != &data[0]) {
		/* Current size (x) */
		wsprintf(buf, "%d", td->cols);
		WritePrivateProfileString(sec_name, "Columns", buf, ini_file);

		/* Current size (y) */
		wsprintf(buf, "%d", td->rows);
		WritePrivateProfileString(sec_name, "Rows", buf, ini_file);
	} else {
                int hgt;
                if (c_cfg.big_map || data[0].rows > SCREEN_HGT + SCREEN_PAD_Y) {
                        /* only change height if we're currently running the default short height */
                        if (screen_hgt <= SCREEN_HGT)
                                hgt = SCREEN_PAD_Y + MAX_SCREEN_HGT;
                        /* also change it however if the main window was resized manually (ie by mouse dragging) */
                        else if (data[0].rows != SCREEN_PAD_Y + screen_hgt)
                                hgt = data[0].rows;
                        /* keep current, modified screen size */
                        else
	                        hgt = SCREEN_PAD_Y + screen_hgt;
    		} else hgt = SCREEN_PAD_Y + SCREEN_HGT;
    		if (hgt > MAX_WINDOW_HGT) hgt = MAX_WINDOW_HGT;

		/* Current size (y) - only change to double-screen if we're not already in some kind of enlarged screen */
		wsprintf(buf, "%d", hgt);
		WritePrivateProfileString(sec_name, "Rows", buf, ini_file);
	}

	/* Current position (x) */
	if (td->pos_x != -32000) { /* don't save windows in minimized state */
		wsprintf(buf, "%d", td->pos_x);
		WritePrivateProfileString(sec_name, "PositionX", buf, ini_file);
	}

	/* Current position (y) */
	if (td->pos_y != -32000) { /* don't save windows in minimized state */
		wsprintf(buf, "%d", td->pos_y);
		WritePrivateProfileString(sec_name, "PositionY", buf, ini_file);
	}
}


/*
 * Write the "preference" data to the .INI file
 *
 * We assume that the windows have all been initialized
 */
static void save_prefs(void) {
#if defined(USE_GRAPHICS) || defined(USE_SOUND)
	char       buf[32];
#endif

#ifdef USE_GRAPHICS
	strcpy(buf, use_graphics ? "1" : "0");
	WritePrivateProfileString("Base", "Graphics", buf, ini_file);
#endif
#ifdef USE_SOUND
	strcpy(buf, use_sound_org ? "1" : "0");
	WritePrivateProfileString("Base", "Sound", buf, ini_file);

 #ifdef USE_SOUND_2010
	WritePrivateProfileString("Base", "SoundpackFolder", cfg_soundpackfolder, ini_file);
	WritePrivateProfileString("Base", "MusicpackFolder", cfg_musicpackfolder, ini_file);
	strcpy(buf, cfg_audio_master ? "1" : "0");
	WritePrivateProfileString("Base", "AudioMaster", buf, ini_file);
	strcpy(buf, cfg_audio_music ? "1" : "0");
	WritePrivateProfileString("Base", "AudioMusic", buf, ini_file);
	strcpy(buf, cfg_audio_sound ? "1" : "0");
	WritePrivateProfileString("Base", "AudioSound", buf, ini_file);
	strcpy(buf, cfg_audio_weather ? "1" : "0");
	WritePrivateProfileString("Base", "AudioWeather", buf, ini_file);
	strcpy(buf, format("%d", cfg_audio_master_volume));
	WritePrivateProfileString("Base", "AudioVolumeMaster", buf, ini_file);
	strcpy(buf, format("%d", cfg_audio_music_volume));
	WritePrivateProfileString("Base", "AudioVolumeMusic", buf, ini_file);
	strcpy(buf, format("%d", cfg_audio_sound_volume));
	WritePrivateProfileString("Base", "AudioVolumeSound", buf, ini_file);
	strcpy(buf, format("%d", cfg_audio_weather_volume));
	WritePrivateProfileString("Base", "AudioVolumeWeather", buf, ini_file);
 #endif
#endif
	save_prefs_aux(&data[0], "Main window");
	/* XXX XXX XXX XXX */
	save_prefs_aux(&data[1], "Mirror window");
	save_prefs_aux(&data[2], "Recall window");
	save_prefs_aux(&data[3], "Choice window");
	save_prefs_aux(&data[4], "Term-4 window");
	save_prefs_aux(&data[5], "Term-5 window");
	save_prefs_aux(&data[6], "Term-6 window");
	save_prefs_aux(&data[7], "Term-7 window");
}


/*
 * Load preference for a single term
 */
static void load_prefs_aux(term_data *td, cptr sec_name) {
	char tmp[128];
	int i = 0;

	/* Visibility (Sub-window) */
	if (td != &data[0]) {
		/* Extract visibility */
		td->visible = (GetPrivateProfileInt(sec_name, "Visible", td->visible, ini_file) != 0);
	}

	/* Desired font, with default */
	GetPrivateProfileString(sec_name, "Font", DEFAULT_FONTNAME, tmp, 127, ini_file);
#ifdef USE_LOGFONT
	td->font_want = string_make(tmp);
	td->lf.lfWidth  = GetPrivateProfileInt(sec_name, "FontWid", 0, ini_file);
	td->lf.lfHeight = GetPrivateProfileInt(sec_name, "FontHgt", 15, ini_file);
	td->lf.lfWeight = GetPrivateProfileInt(sec_name, "FontWgt", 0, ini_file);
#else
	td->font_want = string_make(extract_file_name(tmp));
#endif

	/* Desired graf, with default */
#ifdef USE_GRAPHICS
	GetPrivateProfileString(sec_name, "Graf", DEFAULT_TILENAME, tmp, 127, ini_file);
	td->graf_want = string_make(extract_file_name(tmp));
#endif

	/* Window size */
	td->cols = GetPrivateProfileInt(sec_name, "Columns", td->cols, ini_file);
	td->rows = GetPrivateProfileInt(sec_name, "Rows", td->rows, ini_file);
	if (!td->cols) td->cols = 80;
	if (!td->rows) td->rows = 24;
	if (td == &data[0]) {
		if (td->cols != 80) td->cols = 80;
		screen_wid = td->cols - SCREEN_PAD_X;
		screen_hgt = td->rows - SCREEN_PAD_Y;
	}

	/* Window position */
	td->pos_x = GetPrivateProfileInt(sec_name, "PositionX", td->pos_x, ini_file);
	td->pos_y = GetPrivateProfileInt(sec_name, "PositionY", td->pos_y, ini_file);

	/* Window title */
	i = GetPrivateProfileInt(sec_name, "WindowNumber", i, ini_file);
	if (i != 0) { /* exempt main window */
		GetPrivateProfileString(sec_name, "WindowTitle", ang_term_name[i], tmp, 127, ini_file);
		strcpy(ang_term_name[i], string_make(tmp));
	}
}


/*
 * Hack -- load a "sound" preference by index and name
 */
#ifdef USE_SOUND
#ifndef USE_SOUND_2010
static void load_prefs_sound(int i) {
	char aux[128];
	char wav[128];
	char tmp[128];
	char buf[1024];

	/* Capitalize the sound name */
	strcpy(aux, sound_names[i]);
	aux[0] = FORCEUPPER(aux[0]);

	/* Default to standard name plus ".wav" */
	strcpy(wav, sound_names[i]);
	strcat(wav, ".wav");

	/* Look up the sound by its proper name, using default */
	GetPrivateProfileString("Sound", aux, wav, tmp, 127, ini_file);

	/* Access the sound */
	path_build(buf, 1024, ANGBAND_DIR_XTRA_SOUND, extract_file_name(tmp));

	/* Save the sound filename, if it exists */
	if (check_file(buf)) sound_file[i] = string_make(buf);
}
#endif
#endif

/*
 * Load the preferences from the .INI file
 */
static void load_prefs(void) {
	int i;

	/* Extract the "disable_numlock" flag */
	disable_numlock = (GetPrivateProfileInt("Base", "DisableNumlock", 1, ini_file) != 0);

	/* Extract the readability_blue flag */
	if (GetPrivateProfileInt("Base", "LighterDarkBlue", 1, ini_file) != 0)
		enable_readability_blue_win();

	/* Read the colormap */
	for (i = 0; i < 16; i++) {
		char key_name[12] = { '\0' };
		snprintf(key_name, sizeof(key_name), "Colormap_%d", i);

		char default_color[8] = { '\0' };
		unsigned long c = client_color_map[i];
		snprintf(default_color, sizeof(default_color), "#%06lx", c);

		char want_color[8] = { '\0' };
		GetPrivateProfileString("Base", key_name, default_color, want_color, sizeof(want_color), ini_file);

		client_color_map[i] = parse_color_code(want_color);
#ifdef EXTENDED_COLOURS_PALANIM
		/* Clone it */
		client_color_map[i + 16] = client_color_map[i];
#endif
	}
	enable_common_colormap_win();

#ifdef USE_GRAPHICS
	/* Extract the "use_graphics" flag */
	use_graphics = (GetPrivateProfileInt("Base", "Graphics", 0, ini_file) != 0);
#endif

#ifdef USE_SOUND
	/* Extract the "use_sound" flag */
	use_sound_org = use_sound = (GetPrivateProfileInt("Base", "Sound", 1, ini_file) != 0);
	if (!use_sound) quiet_mode = TRUE;
 #ifdef USE_SOUND_2010
	sound_hint = (GetPrivateProfileInt("Base", "HintSound", 1, ini_file) != 0);
	if (sound_hint) WritePrivateProfileString("Base", "HintSound", "0", ini_file);

	no_cache_audio = !(GetPrivateProfileInt("Base", "CacheAudio", 1, ini_file) != 0);
	cfg_audio_rate = GetPrivateProfileInt("Base", "SampleRate", 44100, ini_file);
	cfg_max_channels = GetPrivateProfileInt("Base", "MaxChannels", 32, ini_file);
	cfg_audio_buffer = GetPrivateProfileInt("Base", "AudioBuffer", 1024, ini_file);
	cfg_audio_master = (GetPrivateProfileInt("Base", "AudioMaster", 1, ini_file) != 0);
	GetPrivateProfileString("Base", "SoundpackFolder", "sound", cfg_soundpackfolder, 127, ini_file);
	GetPrivateProfileString("Base", "MusicpackFolder", "music", cfg_musicpackfolder, 127, ini_file);
	cfg_audio_music = (GetPrivateProfileInt("Base", "AudioMusic", 1, ini_file) != 0);
	cfg_audio_sound = (GetPrivateProfileInt("Base", "AudioSound", 1, ini_file) != 0);
	cfg_audio_weather = (GetPrivateProfileInt("Base", "AudioWeather", 1, ini_file) != 0);
	cfg_audio_master_volume = GetPrivateProfileInt("Base", "AudioVolumeMaster", 100, ini_file);
	cfg_audio_music_volume = GetPrivateProfileInt("Base", "AudioVolumeMusic", 100, ini_file);
	cfg_audio_sound_volume = GetPrivateProfileInt("Base", "AudioVolumeSound", 100, ini_file);
	cfg_audio_weather_volume = GetPrivateProfileInt("Base", "AudioVolumeWeather", 100, ini_file);
 #endif
#endif

	/* Load window prefs */
	load_prefs_aux(&data[0], "Main window");

	/* XXX XXX XXX XXX */

	load_prefs_aux(&data[1], "Mirror window");

	load_prefs_aux(&data[2], "Recall window");

	load_prefs_aux(&data[3], "Choice window");

	load_prefs_aux(&data[4], "Term-4 window");

	load_prefs_aux(&data[5], "Term-5 window");

	load_prefs_aux(&data[6], "Term-6 window");

	load_prefs_aux(&data[7], "Term-7 window");

	if (bigmap_hint) {
		bigmap_hint = (GetPrivateProfileInt("Base", "HintBigmap", 1, ini_file) != 0);
		if (!bigmap_hint) firstrun = FALSE;
	} else bigmap_hint = (GetPrivateProfileInt("Base", "HintBigmap", 1, ini_file) != 0);
	WritePrivateProfileString("Base", "HintBigmap", "0", ini_file);

#ifdef USE_SOUND
#ifndef USE_SOUND_2010
	/* Prepare the sounds */
	for (i = 1; i < SOUND_MAX; i++) load_prefs_sound(i);
#endif
#endif

	/* Metaserver address */
	GetPrivateProfileString("Online", "meta", "", meta_address, MAX_CHARS - 1, ini_file);

	/* Pull nick/pass */
	GetPrivateProfileString("Online", "nick", "", nick, 70, ini_file);
	GetPrivateProfileString("Online", "pass", "", pass, 19, ini_file); //default was 'passwd', but players should really be forced to make up different passwords
	cfg_game_port = GetPrivateProfileInt("Online", "port", 18348, ini_file);
	cfg_client_fps = GetPrivateProfileInt("Online", "fps", 100, ini_file);

	/* XXX Default real name */
	strcpy(real_name, "PLAYER");
}


/*
 * Create the new global palette based on the bitmap palette
 * (if any) from the given bitmap xxx, and the standard 16
 * entry palette derived from "win_clr[]" which is used for
 * the basic 16 Angband colors.
 *
 * This function is never called before all windows are ready.
 */
static void new_palette(void) {
	HPALETTE       hBmPal;
	HPALETTE       hNewPal;
	HDC            hdc;
	int            i, nEntries;
	int            pLogPalSize;
	int            lppeSize;
	LPLOGPALETTE   pLogPal;
	LPPALETTEENTRY lppe;


	/* Cannot handle palettes */
	if (!paletted) return;


	/* No palette */
	hBmPal = NULL;

	/* No bitmap */
	lppeSize = 0;
	lppe = NULL;
	nEntries = 0;

#ifdef USE_GRAPHICS

	/* Check windows */
	for (i = MAX_TERM_DATA - 1; i >= 0; i--) {
		if (data[i].graf_file) {
			/* Check the bitmap palette */
			hBmPal = data[i].infGraph.hPalette;
		}
	}

	/* Use the bitmap */
	if (hBmPal) {
		lppeSize = 256*sizeof(PALETTEENTRY);
		lppe = (LPPALETTEENTRY)mem_alloc(lppeSize);
		nEntries = GetPaletteEntries(hBmPal, 0, 255, lppe);
		if (nEntries == 0) quit("Corrupted bitmap palette");
		if (nEntries > 220) quit("Bitmap must have no more than 220 colors");
	}

#endif

	/* Size of palette */
#ifndef EXTENDED_COLOURS_PALANIM
	pLogPalSize = sizeof(LOGPALETTE) + (16+nEntries)*sizeof(PALETTEENTRY);
#else
	pLogPalSize = sizeof(LOGPALETTE) + (16+16+nEntries)*sizeof(PALETTEENTRY);
#endif
	/* Allocate palette */
	pLogPal = (LPLOGPALETTE)mem_alloc(pLogPalSize);

	/* Version */
	pLogPal->palVersion = 0x300;

	/* Make room for bitmap and normal data */
#ifndef EXTENDED_COLOURS_PALANIM
	pLogPal->palNumEntries = nEntries + 16;
#else
	pLogPal->palNumEntries = nEntries + 16 + 16;
#endif
	/* Save the bitmap data */
	for (i = 0; i < nEntries; i++)
		pLogPal->palPalEntry[i] = lppe[i];

	/* Save the normal data */
#ifndef EXTENDED_COLOURS_PALANIM
	for (i = 0; i < 16; i++) {
#else
	for (i = 0; i < 16 + 16; i++) {
#endif
		LPPALETTEENTRY p;

		/* Access the entry */
		p = &(pLogPal->palPalEntry[i+nEntries]);

		/* Save the colors */
		p->peRed = GetRValue(win_clr[i]);
		p->peGreen = GetGValue(win_clr[i]);
		p->peBlue = GetBValue(win_clr[i]);

		/* Save the flags */
		p->peFlags = PC_NOCOLLAPSE;
	}

	/* Free something */
	if (lppe) mem_free(lppe);

	/* Create a new palette, or fail */
	hNewPal = CreatePalette(pLogPal);
	if (!hNewPal) quit("Cannot create palette");

	/* Free the palette */
	mem_free(pLogPal);


	hdc = GetDC(data[0].w);
	SelectPalette(hdc, hNewPal, 0);
	i = RealizePalette(hdc);
	ReleaseDC(data[0].w, hdc);
	if (i == 0) quit("Cannot realize palette");

	/* Check windows */
	for (i = 1; i < MAX_TERM_DATA; i++) {
		hdc = GetDC(data[i].w);
		SelectPalette(hdc, hNewPal, 0);
		ReleaseDC(data[i].w, hdc);
	}

	/* Delete old palette */
	if (hPal) DeleteObject(hPal);

	/* Save new palette */
	hPal = hNewPal;
}
#ifdef PALANIM_SWAP
/* Special version of new_palette() for just animating colours 0-15. */
static void new_palette_ps(void) {
	HPALETTE       hBmPal;
	HPALETTE       hNewPal;
	HDC            hdc;
	int            i, nEntries;
	int            pLogPalSize;
	int            lppeSize;
	LPLOGPALETTE   pLogPal;
	LPPALETTEENTRY lppe;


	/* Cannot handle palettes */
	if (!paletted) return;


	/* No palette */
	hBmPal = NULL;

	/* No bitmap */
	lppeSize = 0;
	lppe = NULL;
	nEntries = 0;

 #ifdef USE_GRAPHICS

	/* Check windows */
	for (i = MAX_TERM_DATA - 1; i >= 0; i--) {
		if (data[i].graf_file) {
			/* Check the bitmap palette */
			hBmPal = data[i].infGraph.hPalette;
		}
	}

	/* Use the bitmap */
	if (hBmPal) {
		lppeSize = 256*sizeof(PALETTEENTRY);
		lppe = (LPPALETTEENTRY)mem_alloc(lppeSize);
		nEntries = GetPaletteEntries(hBmPal, 0, 255, lppe);
		if (nEntries == 0) quit("Corrupted bitmap palette");
		if (nEntries > 220) quit("Bitmap must have no more than 220 colors");
	}

 #endif

	/* Size of palette */
	pLogPalSize = sizeof(LOGPALETTE) + (16+nEntries)*sizeof(PALETTEENTRY);

	/* Allocate palette */
	pLogPal = (LPLOGPALETTE)mem_alloc(pLogPalSize);

	/* Version */
	pLogPal->palVersion = 0x300;

	/* Make room for bitmap and normal data */
	pLogPal->palNumEntries = nEntries + 16;

	/* Save the bitmap data */
	for (i = 0; i < nEntries; i++)
		pLogPal->palPalEntry[i] = lppe[i];

	/* Save the normal data */
	for (i = 0; i < 16; i++) {
		LPPALETTEENTRY p;

		/* Access the entry */
		p = &(pLogPal->palPalEntry[i+nEntries]);

		/* Save the colors */
		p->peRed = GetRValue(win_clr[i]);
		p->peGreen = GetGValue(win_clr[i]);
		p->peBlue = GetBValue(win_clr[i]);

		/* Save the flags */
		p->peFlags = PC_NOCOLLAPSE;
	}

	/* Free something */
	if (lppe) mem_free(lppe);

	/* Create a new palette, or fail */
	hNewPal = CreatePalette(pLogPal);
	if (!hNewPal) quit("Cannot create palette");

	/* Free the palette */
	mem_free(pLogPal);


	hdc = GetDC(data[0].w);
	SelectPalette(hdc, hNewPal, 0);
	i = RealizePalette(hdc);
	ReleaseDC(data[0].w, hdc);
	if (i == 0) quit("Cannot realize palette");

	/* Check windows */
	for (i = 1; i < MAX_TERM_DATA; i++) {
		hdc = GetDC(data[i].w);
		SelectPalette(hdc, hNewPal, 0);
		ReleaseDC(data[i].w, hdc);
	}

	/* Delete old palette */
	if (hPal) DeleteObject(hPal);

	/* Save new palette */
	hPal = hNewPal;
}
#endif

/*
 * Resize a window
 */
static void term_window_resize(term_data *td) {

	/* Require window */
	if (!td->w) return;

	/* Resize the window */
	SetWindowPos(td->w, 0, 0, 0,
	             td->size_wid, td->size_hgt,
	             SWP_NOMOVE | SWP_NOZORDER);

	/* Redraw later */
	InvalidateRect(td->w, NULL, TRUE);
}



/*
 * Force the use of a new "font file" for a term_data
 *
 * This function may be called before the "window" is ready
 *
 * This function returns zero only if everything succeeds.
 */
static errr term_force_font(term_data *td, cptr name) {
	int i;
	int wid, hgt;
	cptr s;
	char base[16];
	char base_font[16];
	char buf[1024];
	bool used;


#ifdef USE_LOGFONT
	td->font_id = CreateFontIndirect(&(td->lf));
	if (!td->font_id) return (1);
	wid = td->lf.lfWidth;
	hgt = td->lf.lfHeight;
#else
	/* Forget the old font (if needed) */
	if (td->font_id) DeleteObject(td->font_id);

	/* Forget old font */
	if (td->font_file) {
		used = FALSE;

		/* Scan windows */
		for (i = 0; i < MAX_TERM_DATA; i++) {
			/* Check "screen" */
			if ((td != &data[i]) &&
			    (data[i].font_file) &&
			    (streq(data[i].font_file, td->font_file)))
				used = TRUE;
		}

		/* Remove unused font resources */
		if (!used) RemoveFontResource(td->font_file);

		/* Free the old name */
		string_free(td->font_file);

		/* Forget it */
		td->font_file = NULL;
	}


	/* No name given */
	if (!name) return (1);

	/* Extract the base name (with suffix) */
	s = extract_file_name(name);

	/* Extract font width */
	wid = atoi(s);

	/* Default font height */
	hgt = 0;

	/* Copy, capitalize, remove suffix, extract width */
	for (i = 0; (i < 16 - 1) && s[i] && (s[i] != '.'); i++) {
		/* Capitalize */
		base[i] = FORCEUPPER(s[i]);

		/* Extract "hgt" when found */
		if (base[i] == 'X') hgt = atoi(s+i+1);
	}

	/* Terminate */
	base[i] = '\0';


	/* Build base_font */
	strcpy(base_font, base);
	strcat(base_font, ".FON");


	/* Access the font file */
	path_build(buf, 1024, ANGBAND_DIR_XTRA_FONT, base_font);


	/* Verify file */
	if (!check_file(buf)) return (1);


	/* Save new font name */
	td->font_file = string_make(buf);

	used = FALSE;

	/* Scan windows */
	for (i = 0; i < MAX_TERM_DATA; i++) {
		/* Check "screen" */
		if ((td != &data[i]) &&
		    (data[i].font_file) &&
		    (streq(data[i].font_file, td->font_file)))
			used = TRUE;
	}

	/* Only call AddFontResource once for each file or the font files
	 * will remain locked! - mikaelh */
	if (!used) {
		/* Load the new font or quit */
		if (!AddFontResource(buf))
			quit_fmt("Font file corrupted:\n%s", buf);
	}

	/* Notify other applications that a new font is available  XXX */
	PostMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);

	/* Create the font XXX XXX XXX Note use of "base" */
	td->font_id = CreateFont(hgt, wid, 0, 0, FW_DONTCARE, 0, 0, 0,
	                         ANSI_CHARSET, OUT_DEFAULT_PRECIS,
	                         CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
	                         FIXED_PITCH | FF_DONTCARE, base);
#endif

	/* Hack -- Unknown size */
	if (!wid || !hgt) {
		HDC         hdcDesktop;
		HFONT       hfOld;
		TEXTMETRIC  tm;

		/* all this trouble to get the cell size */
		hdcDesktop = GetDC(HWND_DESKTOP);
		hfOld = SelectObject(hdcDesktop, td->font_id);
		GetTextMetrics(hdcDesktop, &tm);
		SelectObject(hdcDesktop, hfOld);
		ReleaseDC(HWND_DESKTOP, hdcDesktop);

		/* Font size info */
		wid = tm.tmAveCharWidth;
		hgt = tm.tmHeight;
	}

	/* Save the size info */
	td->font_wid = wid;
	td->font_hgt = hgt;


	/* Analyze the font */
	term_getsize(td);

	/* Resize the window */
	term_window_resize(td);

	/* Reload custom font prefs on main screen font change */
	if (td == &data[0]) handle_process_font_file();

	/* Success */
	return (0);
}


#ifdef USE_GRAPHICS

/*
 * Force the use of a new "graf file" for a term_data
 *
 * This function is never called before the windows are ready
 *
 * This function returns zero only if everything succeeds.
 */
static errr term_force_graf(term_data *td, cptr name) {
	int i;
	int wid, hgt;
	cptr s;
	char base[16];
	char base_graf[16];
	char buf[1024];

	/* Forget old stuff */
	if (td->graf_file) {
		/* Free the old information */
		if (td->infGraph.hDIB) GlobalFree(td->infGraph.hDIB);
		if (td->infGraph.hPalette) DeleteObject(td->infGraph.hPalette);
		if (td->infGraph.hBitmap) DeleteObject(td->infGraph.hBitmap);

		/* Forget them */
		td->infGraph.hDIB = 0;
		td->infGraph.hPalette = 0;
		td->infGraph.hBitmap = 0;

		/* Forget old graf name */
		string_free(td->graf_file);

		/* Forget it */
		td->graf_file = NULL;
	}


	/* No name */
	if (!name) return (1);

	/* Extract the base name (with suffix) */
	s = extract_file_name(name);

	/* Extract font width */
	wid = atoi(s);

	/* Default font height */
	hgt = 0;

	/* Copy, capitalize, remove suffix, extract width */
	for (i = 0; (i < 16 - 1) && s[i] && (s[i] != '.'); i++) {
		/* Capitalize */
		base[i] = FORCEUPPER(s[i]);

		/* Extract "hgt" when found */
		if (base[i] == 'X') hgt = atoi(s+i+1);
	}

	/* Terminate */
	base[i] = '\0';

	/* Require actual sizes */
	if (!wid || !hgt) return (1);

	/* Build base_graf */
	strcpy(base_graf, base);
	strcat(base_graf, ".BMP");


	/* Access the graf file */
	path_build(buf, 1024, ANGBAND_DIR_XTRA_GRAF, base_graf);

	/* Verify file */
	if (!check_file(buf)) return (1);


	/* Save new graf name */
	td->graf_file = string_make(base_graf);

	/* Load the bitmap or quit */
	if (!ReadDIB(td->w, buf, &td->infGraph))
		quit_fmt("Bitmap corrupted:\n%s", buf);

	/* Save the new sizes */
	td->infGraph.CellWidth = wid;
	td->infGraph.CellHeight = hgt;

	/* Activate a palette */
	new_palette();

	/* Success */
	return (0);
}

#endif


/*
 * Allow the user to change the font (and graf) for this window.
 *
 * XXX XXX XXX This is only called for non-graphic windows
 */
static void term_change_font(term_data *td) {
#ifdef USE_LOGFONT
	CHOOSEFONT cf;

	memset(&cf, 0, sizeof(cf));
	cf.lStructSize = sizeof(cf);
	cf.Flags = CF_SCREENFONTS | CF_FIXEDPITCHONLY | CF_NOVERTFONTS | CF_INITTOLOGFONTSTRUCT;
	cf.lpLogFont = &(td->lf);

	if (ChooseFont(&cf)) {
		/* Force the font */
		term_force_font(td, NULL);

		/* Analyze the font */
		term_getsize(td);

		/* Resize the window */
		term_window_resize(td);
	}
#else
	OPENFILENAME ofn;

	char tmp[128] = "";

	/* Extract a default if possible */
	if (td->font_file) strcpy(tmp, td->font_file);

	/* Ask for a choice */
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = data[0].w;
	ofn.lpstrFilter = "Font Files (*.fon)\0*.fon\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = tmp;
	ofn.nMaxFile = 128;
	ofn.lpstrInitialDir = ANGBAND_DIR_XTRA_FONT;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	ofn.lpstrDefExt = "fon";

	/* Force choice if legal */
	if (GetOpenFileName(&ofn)) {
		/* Force the font */
		if (term_force_font(td, tmp)) {
			/* Oops */
			(void)term_force_font(td, DEFAULT_FONTNAME);
		}
	}
#endif
}


#ifdef USE_GRAPHICS

/*
 * Allow the user to change the graf (and font) for a window
 *
 * XXX XXX XXX This is only called for graphic windows, and
 * changes the font and the bitmap for the window, and is
 * only called if "use_graphics" is true.
 */
static void term_change_bitmap(term_data *td) {
	OPENFILENAME ofn;
	char tmp[128] = "";

	/* Extract a default if possible */
	if (td->graf_file) strcpy(tmp, td->graf_file);

	/* Ask for a choice */
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = data[0].w;
	ofn.lpstrFilter = "Bitmap Files (*.bmp)\0*.bmp\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = tmp;
	ofn.nMaxFile = 128;
	ofn.lpstrInitialDir = ANGBAND_DIR_XTRA_GRAF;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	ofn.lpstrDefExt = "bmp";

	/* Force choice if legal */
	if (GetOpenFileName(&ofn)) {
		/* XXX XXX XXX */

		/* Force the requested font and bitmap */
		if (term_force_font(td, tmp) ||
		    term_force_graf(td, tmp)) {
			/* Force the "standard" font */
			(void)term_force_font(td, DEFAULT_FONTNAME);

			/* Force the "standard" bitmap */
			(void)term_force_graf(td, DEFAULT_TILENAME);
		}
	}
}

#endif


/*
 * Hack -- redraw a term_data
 */
static void term_data_redraw(term_data *td) {
	/* Activate the term */
	Term_activate(&td->t);

	/* Redraw the contents */
	Term_redraw();

	/* Restore the term */
	Term_activate(term_screen);
}


/*
 * Hack -- redraw a term_data
 */
static void term_data_redraw_section(term_data *td, int x1, int y1, int x2, int y2) {
    /* Activate the term */
    Term_activate(&td->t);

    /* Redraw the area */
    Term_redraw_section(x1, y1, x2, y2);

    /* Restore the term */
    Term_activate(term_screen);
}


/*
 * This function either gets a new DC or uses an old one. Each screen update
 * must be followed by a TERM_XTRA_FRESH so that the DC gets released.
 *  - mikaelh
 */
static HDC myGetDC(HWND hWnd) {
#ifdef OPTIMIZE_DRAWING
	term_data *td = (term_data*)(Term->data);

	if (!oldDC) {
		oldDC = GetDC(hWnd);

		/* The background color and the font is always the same */
		SetBkColor(oldDC, RGB(0, 0, 0));
		SelectObject(oldDC, td->font_id);

		/* Foreground color not set */
		old_attr = -1;
	}
	return oldDC;
#else
	return GetDC(hWnd);
#endif
}




/*** Function hooks needed by "Term" ***/

/*
 * Interact with the User
 */
static errr Term_user_win(int n) {
	/* Success */
	return (0);
}


/*
 * React to global changes
 */
static errr Term_xtra_win_react(void) {
	int i;
/*	I added this USE_GRAPHICS because we lost color support as well. -GP */
#ifdef USE_GRAPHICS
	static int old_use_graphics = FALSE;


	/* XXX XXX XXX Check "color_table[]" */


	/* Simple color */
	if (colors16) {
		/* Save the default colors */
		for (i = 0; i < 16; i++) {
			/* Simply accept the desired colors */
			win_pal[i] = color_table[i][0];
		}
	}

	/* Complex color */
	else {
		COLORREF code;
		byte rv, gv, bv;
		bool change = FALSE;

		/* Save the default colors */
#ifndef EXTENDED_COLOURS_PALANIM
		for (i = 0; i < 16; i++) {
#else
		for (i = 0; i < 16 + 16; i++) {
#endif
			/* Extract desired values */
			rv = color_table[i][1];
			gv = color_table[i][2];
			bv = color_table[i][3];

			/* Extract a full color code */
			code = PALETTERGB(rv, gv, bv);

			/* Activate changes */
			if (win_clr[i] != code) {
				/* Note the change */
				change = TRUE;

				/* Apply the desired color */
				win_clr[i] = code;
			}
		}

		/* Activate the palette if needed */
		if (change) new_palette();
	}
#endif	/* no color support -gp */

#ifdef USE_GRAPHICS

	/* XXX XXX XXX Check "use_graphics" */
	if (use_graphics && !old_use_graphics) {
		/* Hack -- set the player picture */
//		r_info[0].x_attr = 0x87;
//		r_info[0].x_char = 0x80 | (10 * p_ptr->pclass + p_ptr->prace);
	}

	/* Remember */
	old_use_graphics = use_graphics;

#endif


	/* Clean up windows */
	for (i = 0; i < MAX_TERM_DATA; i++) {
		term *old = Term;
		term_data *td = &data[i];

		/* Skip non-changes XXX XXX XXX */
		if ((td->cols == td->t.wid) && (td->rows == td->t.hgt)) continue;

		/* Check this vs WM_SIZING in AngbandWndProc - Redundant/problem? */

		/* Activate */
		Term_activate(&td->t);

		/* Hack -- Resize the term */
		Term_resize(td->cols, td->rows);

		/* Redraw the contents */
		Term_redraw();

		/* Restore */
		Term_activate(old);
	}


	/* Success */
	return (0);
}


#ifdef OPTIMIZE_DRAWING
/* Declare before use */
static errr Term_xtra_win_fresh(int v);
#endif


/*
 * Process at least one event
 */
static errr Term_xtra_win_event(int v)
{
	MSG msg;

#ifdef OPTIMIZE_DRAWING
	/* Release DC before waiting */
	Term_xtra_win_fresh(v);
#endif

	/* Wait for an event */
	if (v)
	{
		/* Block */
		if (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	/* Check for an event */
	else
	{
		/* Check */
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	/* Success */
	return 0;
}


/*
 * Process all pending events
 */
static errr Term_xtra_win_flush(void)
{
	MSG msg;

	/* Process all pending events */
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	/* Success */
	return (0);
}


/*
 * Hack -- clear the screen
 *
 * XXX XXX XXX Make this more efficient
 */
static errr Term_xtra_win_clear(void)
{
	term_data *td = (term_data*)(Term->data);

	HDC  hdc;
	RECT rc;

	/* Rectangle to erase */
	rc.left   = td->size_ow1;
	rc.right  = rc.left + td->cols * td->font_wid;
	rc.top    = td->size_oh1;
	rc.bottom = rc.top + td->rows * td->font_hgt;

	/* Erase it */
	hdc = myGetDC(td->w);
#ifdef OPTIMIZE_DRAWING
	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
#else
	SetBkColor(hdc, RGB(0, 0, 0));
	SelectObject(hdc, td->font_id);
	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
	ReleaseDC(td->w, hdc);
#endif

	/* Success */
	return 0;
}


/*
 * Hack -- make a noise
 */
static errr Term_xtra_win_noise(void)
{
	MessageBeep(MB_ICONASTERISK);
	return (0);
}


/*
 * Hack -- make a sound
 */
static errr Term_xtra_win_sound(int v)
{
	/* Unknown sound */
	if ((v < 0) || (v >= SOUND_MAX)) return (1);

	/* Unknown sound */
	if (!sound_file[v]) return (1);

#ifdef USE_SOUND

#ifdef WIN32

	/* Play the sound, catch errors */
	return (PlaySound(sound_file[v], 0, SND_FILENAME | SND_ASYNC));

#else /* WIN32 */

	/* Play the sound, catch errors */
	return (sndPlaySound(sound_file[v], SND_ASYNC));

#endif /* WIN32 */

#endif /* USE_SOUND */

	/* Oops */
	return (1);
}


/*
 * Delay for "x" milliseconds
 */
static errr Term_xtra_win_delay(int v)
{

#ifdef WIN32

	/* Sleep */
	Sleep(v);

#else /* WIN32 */

	DWORD t;
	MSG   msg;

	/* Final count */
	t = GetTickCount() + v;

	/* Wait for it */
	while (GetTickCount() < t)
	{
		/* Handle messages */
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

#endif /* WIN32 */

	/* Success */
	return (0);
}


#ifdef OPTIMIZE_DRAWING
/*
 * This is where we free the DC we've been using.
 */
static errr Term_xtra_win_fresh(int v)
{
	term_data *td = (term_data*)(Term->data);

	if (oldDC) {
		ReleaseDC(td->w, oldDC);
		oldDC = NULL;
	}

	/* Success */
	return (0);
}
#endif


/*
 * Do a "special thing"
 */
static errr Term_xtra_win(int n, int v)
{
	/* Handle a subset of the legal requests */
	switch (n)
	{
		/* Make a bell sound */
		case TERM_XTRA_NOISE:
		return (Term_xtra_win_noise());

		/* Make a special sound */
		case TERM_XTRA_SOUND:
		return (Term_xtra_win_sound(v));

		/* Process random events */
		case TERM_XTRA_BORED:
		return (Term_xtra_win_event(0));

		/* Process an event */
		case TERM_XTRA_EVENT:
		return (Term_xtra_win_event(v));

		/* Flush all events */
		case TERM_XTRA_FLUSH:
		return (Term_xtra_win_flush());

		/* Clear the screen */
		case TERM_XTRA_CLEAR:
		return (Term_xtra_win_clear());

		/* React to global changes */
		case TERM_XTRA_REACT:
		return (Term_xtra_win_react());

		/* Delay for some milliseconds */
		case TERM_XTRA_DELAY:
		return (Term_xtra_win_delay(v));

#ifdef OPTIMIZE_DRAWING
		case TERM_XTRA_FRESH:
		return (Term_xtra_win_fresh(v));
#endif
	}

	/* Oops */
	return 1;
}



/*
 * Low level graphics (Assumes valid input).
 *
 * Erase a "block" of "n" characters starting at (x,y).
 */
static errr Term_wipe_win(int x, int y, int n)
{
	term_data *td = (term_data*)(Term->data);

	HDC  hdc;
	RECT rc;

	/* Rectangle to erase in client coords */
	rc.left   = x * td->font_wid + td->size_ow1;
	rc.right  = rc.left + n * td->font_wid;
	rc.top    = y * td->font_hgt + td->size_oh1;
	rc.bottom = rc.top + td->font_hgt;

	hdc = myGetDC(td->w);
#ifdef OPTIMIZE_DRAWING
	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
#else
	SetBkColor(hdc, RGB(0, 0, 0));
	SelectObject(hdc, td->font_id);
	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
	ReleaseDC(td->w, hdc);
#endif

	/* Success */
	return 0;
}


/*
 * Low level graphics (Assumes valid input).
 * Draw a "cursor" at (x,y), using a "yellow box".
 */
static errr Term_curs_win(int x, int y)
{
	term_data *td = (term_data*)(Term->data);

	RECT   rc;
	HDC    hdc;

	/* Frame the grid */
	rc.left   = x * td->font_wid + td->size_ow1;
	rc.right  = rc.left + td->font_wid;
	rc.top    = y * td->font_hgt + td->size_oh1;
	rc.bottom = rc.top + td->font_hgt;

	/* Cursor is done as a yellow "box" */
	hdc = myGetDC(data[0].w);
	FrameRect(hdc, &rc, hbrYellow);
#ifndef OPTIMIZE_DRAWING
	ReleaseDC(data[0].w, hdc);
#endif

	/* Success */
	return 0;
}


/*
 * Low level graphics.  Assumes valid input.
 * Draw a "special" attr/char at the given location.
 *
 * XXX XXX XXX We use the "Term_pict_win()" function for "graphic" data,
 * which are encoded by setting the "high-bits" of both the "attr" and
 * the "char" data.  We use the "attr" to represent the "row" of the main
 * bitmap, and the "char" to represent the "col" of the main bitmap.  The
 * use of this function is induced by the "higher_pict" flag.
 *
 * If we are called for anything but the "screen" window, or if the global
 * "use_graphics" flag is off, we simply "wipe" the given grid.
 */
static errr Term_pict_win(int x, int y, byte a, char c)
{

#ifdef USE_GRAPHICS

	term_data *td = (term_data*)(Term->data);
	HDC  hdc;
	HDC hdcSrc;
	HBITMAP hbmSrcOld;
	int row, col;
	int x1, y1, w1, h1;
	int x2, y2, w2, h2;

	/* Paranoia -- handle weird requests */
	if (!use_graphics)
	{
		/* First, erase the grid */
		return (Term_wipe_win(x, y, 1));
	}

#ifndef FULL_GRAPHICS
	/* Paranoia -- handle weird requests */
	if (td != &data[0])
	{
		/* First, erase the grid */
		return (Term_wipe_win(x, y, 1));
	}
#endif

	/* Extract picture info */
	row = (a & 0x7F);
	col = (c & 0x7F);

	/* Size of bitmap cell */
	w1 = td->infGraph.CellWidth;
	h1 = td->infGraph.CellHeight;

	/* Location of bitmap cell */
	x1 = col * w1;
	y1 = row * h1;

	/* Size of window cell */
	w2 = td->font_wid;
	h2 = td->font_hgt;

	/* Location of window cell */
	x2 = x * w2 + td->size_ow1;
	y2 = y * h2 + td->size_oh1;

	/* Info */
	hdc = myGetDC(td->w);

	/* Handle small bitmaps */
	if ((w1 < w2) || (h1 < h2))
	{
		RECT rc;

		/* Erasure rectangle */
		rc.left   = x2;
		rc.right  = x2 + w2;
		rc.top    = y2;
		rc.bottom = y2 + h2;

		/* Erase the rectangle */
#ifdef OPTIMIZE_DRAWING
		ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
#else
		SetBkColor(hdc, RGB(0, 0, 0));
		SelectObject(hdc, td->font_id);
		ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
#endif

		/* Center bitmaps */
		x2 += (w2 - w1) >> 1;
		y2 += (h2 - h1) >> 1;
	}

	/* More info */
	hdcSrc = CreateCompatibleDC(hdc);
	hbmSrcOld = SelectObject(hdcSrc, td->infGraph.hBitmap);

	/* Copy the picture from the bitmap to the window */
	BitBlt(hdc, x2, y2, w1, h1, hdcSrc, x1, y1, SRCCOPY);

	/* Release */
	SelectObject(hdcSrc, hbmSrcOld);
	DeleteDC(hdcSrc);

#ifndef OPTIMIZE_DRAWING
	/* Release */
	ReleaseDC(td->w, hdc);
#endif

#else

	/* Just erase this grid */
	return (Term_wipe_win(x, y, 1));

#endif

	/* Success */
	return 0;
}


/*
 * Low level graphics.  Assumes valid input.
 * Draw several ("n") chars, with an attr, at a given location.
 *
 * All "graphic" data is handled by "Term_pict_win()", above.
 *
 * XXX XXX XXX Note that this function assumes the font is monospaced.
 *
 * XXX XXX XXX One would think there is a more efficient method for
 * telling a window what color it should be using to draw with, but
 * perhaps simply changing it every time is not too inefficient.
 */
static errr Term_text_win(int x, int y, int n, byte a, const char *s) {
	term_data *td = (term_data*)(Term->data);
	RECT rc;
	HDC  hdc;


	/* Location */
	rc.left   = x * td->font_wid + td->size_ow1;
	rc.right  = rc.left + n * td->font_wid;
	rc.top    = y * td->font_hgt + td->size_oh1;
	rc.bottom = rc.top + td->font_hgt;

	/* Acquire DC */
	hdc = myGetDC(td->w);

#ifdef PALANIM_SWAP
	a = (a + 16) % 32;
#endif

#ifdef OPTIMIZE_DRAWING
	if (old_attr != a) {
		/* Foreground color */
		if (colors16)
			SetTextColor(hdc, PALETTEINDEX(win_pal[a & 0x0F]));
		else
 #ifndef EXTENDED_COLOURS_PALANIM
			SetTextColor(hdc, win_clr[a & 0x0F]);
 #else
			SetTextColor(hdc, win_clr[a & 0x1F]);
 #endif
		old_attr = a;
	}

	/* Dump the text */
	ExtTextOut(hdc, rc.left, rc.top, ETO_OPAQUE | ETO_CLIPPED, &rc,
	           s, n, NULL);
#else
	/* Background color */
	SetBkColor(hdc, RGB(0, 0, 0));

	/* Foreground color */
	if (colors16)
		SetTextColor(hdc, PALETTEINDEX(win_pal[a & 0x0F]));
	else
 #ifndef EXTENDED_COLOURS_PALANIM
		SetTextColor(hdc, win_clr[a & 0x0F]);
 #else
		SetTextColor(hdc, win_clr[a & 0x1F]);
 #endif

	/* Use the font */
	SelectObject(hdc, td->font_id);

	/* Dump the text */
	ExtTextOut(hdc, rc.left, rc.top, ETO_OPAQUE | ETO_CLIPPED, &rc,
	           s, n, NULL);

	/* Release DC */
	ReleaseDC(td->w, hdc);
#endif

	/* Success */
	return 0;
}


/*** Other routines ***/


/*
 * Create and initialize a "term_data" given a title
 */
static void term_data_link(term_data *td)
{
	term *t = &td->t;

	/* Initialize the term */
	term_init(t, td->cols, td->rows, td->keys);

	/* Use a "software" cursor */
	t->soft_cursor = TRUE;

#ifdef USE_GRAPHICS
	/* Use "Term_pict" for "graphic" data */
	t->higher_pict = TRUE;
#endif

	/* Total erases seem to make the screen flicker - mikaelh */
	t->no_total_erase_on_wipe = TRUE;

	/* Erase with "white space" */
	t->attr_blank = TERM_WHITE;
	t->char_blank = ' ';

	/* Prepare the template hooks */
	t->user_hook = Term_user_win;
	t->xtra_hook = Term_xtra_win;
	t->wipe_hook = Term_wipe_win;
	t->curs_hook = Term_curs_win;
	t->pict_hook = Term_pict_win;
	t->text_hook = Term_text_win;

	/* Remember where we came from */
	t->data = (vptr)(td);
}


/*
 * Create the windows
 *
 * First, instantiate the "default" values, then read the "ini_file"
 * to over-ride selected values, then create the windows, and fonts.
 *
 * XXX XXX XXX Need to work on the default window positions
 *
 * Must use SW_SHOW not SW_SHOWNA, since on 256 color display
 * must make active to realize the palette. (?)
 */
static void init_windows(void)
{
	int i;
	static char version[20];
	term_data *td;


	/* Main window */
	td = &data[0];
	WIPE(td, term_data);

	sprintf(version, "TomeNET %d.%d.%d%s", 
			VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, CLIENT_VERSION_TAG);
	td->s = version;
	td->keys = 1024;
	td->rows = 24;
	td->cols = 80;
	td->visible = TRUE;
	td->size_ow1 = 2;
	td->size_ow2 = 2;
	td->size_oh1 = 2;
	td->size_oh2 = 2;
	td->pos_x = 0;
	td->pos_y = 0;

	/* Sub-windows */
	for (i = 1; i < MAX_TERM_DATA; i++) {
		td = &data[i];
		WIPE(td, term_data);
		td->s = ang_term_name[i];
		td->keys = 16;
		td->rows = 24;
		td->cols = 80;
		td->visible = TRUE;
		td->size_ow1 = 1;
		td->size_ow2 = 1;
		td->size_oh1 = 1;
		td->size_oh2 = 1;
		td->pos_x = 0;
		td->pos_y = 0;
	}


	/* Load .INI preferences */
	load_prefs();


	/* Need these before term_getsize gets called */
	data[0].dwStyle = (WS_OVERLAPPED | WS_THICKFRAME | WS_SYSMENU |
	                   WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CAPTION |
	                   WS_VISIBLE);
	data[0].dwExStyle = 0;

	/* Windows */
	for (i = 1; i < MAX_TERM_DATA; i++)
	{
		data[i].dwStyle = (WS_OVERLAPPED | WS_THICKFRAME | WS_SYSMENU | WS_CAPTION);
		data[i].dwExStyle = (WS_EX_TOOLWINDOW);
	}


	/* Windows */
	for (i = 0; i < MAX_TERM_DATA; i++)
	{
#ifdef USE_LOGFONT
		td = &data[i];

		strncpy(td->lf.lfFaceName, td->font_want, LF_FACESIZE);
		td->lf.lfHeight = td->font_hgt;
		td->lf.lfWidth  = td->font_wid;
		td->lf.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
		term_force_font(td, NULL);
#else
		if (term_force_font(&data[i], data[i].font_want))
		{
			(void)term_force_font(&data[i], DEFAULT_FONTNAME);
		}
#endif
	}


	/* Screen window */
	td_ptr = &data[0];
	td_ptr->w = CreateWindowEx(td_ptr->dwExStyle, AppName,
	                           td_ptr->s, td_ptr->dwStyle,
	                           td_ptr->pos_x, td_ptr->pos_y,
	                           td_ptr->size_wid, td_ptr->size_hgt,
	                           HWND_DESKTOP, NULL, hInstance, NULL);
	if (!td_ptr->w) quit("Failed to create Angband window");
	td_ptr = NULL;
	term_data_link(&data[0]);
	ang_term[0] = &data[0].t;

	/* Windows */
	for (i = 1; i < MAX_TERM_DATA; i++)
	{
		td_ptr = &data[i];
		td_ptr->w = CreateWindowEx(td_ptr->dwExStyle, AngList,
		                           td_ptr->s, td_ptr->dwStyle,
		                           td_ptr->pos_x, td_ptr->pos_y,
		                           td_ptr->size_wid, td_ptr->size_hgt,
		                           HWND_DESKTOP, NULL, hInstance, NULL);
		if (!td_ptr->w) quit("Failed to create sub-window");
		if (td_ptr->visible)
		{
			td_ptr->size_hack = TRUE;
			ShowWindow(td_ptr->w, SW_SHOW);
			td_ptr->size_hack = FALSE;
		}
		td_ptr = NULL;
		term_data_link(&data[i]);
		ang_term[i] = &data[i].t;
	}


	/* Activate the screen window */
	SetActiveWindow(data[0].w);

	/* Bring main screen back to top */
	SetWindowPos(data[0].w, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);


#ifdef USE_GRAPHICS

	/* Handle "graphics" mode */
	if (use_graphics)
	{
		/* Force the "requested" bitmap XXX XXX XXX */
		if (term_force_graf(&data[0], data[0].graf_want))
		{
			/* XXX XXX XXX Force the "standard" font */
			(void)term_force_font(&data[0], DEFAULT_FONTNAME);

			/* XXX XXX XXX Force the "standard" bitmap */
			(void)term_force_graf(&data[0], DEFAULT_TILENAME);
		}

#ifdef FULL_GRAPHICS

		/* Windows */
		for (i = 1; i < MAX_TERM_DATA; i++)
		{
			/* Force the "requested" bitmap XXX XXX XXX */
			if (term_force_graf(&data[i], data[i].graf_want))
			{
				/* XXX XXX XXX Force the "standard" font */
				(void)term_force_font(&data[i], DEFAULT_FONTNAME);

				/* XXX XXX XXX Force the "standard" bitmap */
				(void)term_force_graf(&data[i], DEFAULT_TILENAME);
			}
		}

#endif /* FULL_GRAPHICS */

	}

#endif


	/* New palette XXX XXX XXX */
	new_palette();


	/* Create a "brush" for drawing the "cursor" */
	hbrYellow = CreateSolidBrush(win_clr[TERM_YELLOW]);


	/* Process pending messages */
	(void)Term_xtra_win_flush();
}


#if 0
/*
 * Hack -- disables new and open from file menu
 */
static void disable_start(void)
{
	HMENU hm = GetMenu(data[0].w);

	EnableMenuItem(hm, IDM_FILE_NEW, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	EnableMenuItem(hm, IDM_FILE_OPEN, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
}
#endif


/*
 * Prepare the menus
 *
 * XXX XXX XXX See the updated "main-mac.c" for a much nicer
 * system, basically, you explicitly disable any menu option
 * which the user should not be allowed to use, and then you
 * do not have to do any checking when processing the menu.
 */
static void setup_menus(void)
{
	int i;

	HMENU hm = GetMenu(data[0].w);
#ifdef MNU_SUPPORT
	/* Save player */
	EnableMenuItem(hm, IDM_FILE_SAVE,
                       MF_BYCOMMAND | MF_ENABLED | MF_GRAYED);
 

	/* Exit with save */
	EnableMenuItem(hm, IDM_FILE_EXIT,
                       MF_BYCOMMAND | MF_ENABLED);
 

	/* Window font options */
	for (i = 1; i < MAX_TERM_DATA; i++)
	{
		/* Window font */
		EnableMenuItem(hm, IDM_TEXT_SCREEN + i,
		               MF_BYCOMMAND | (data[i].visible ? MF_ENABLED : MF_DISABLED | MF_GRAYED));
	}

	/* Window options */
	for (i = 1; i < MAX_TERM_DATA; i++)
	{
		/* Window */
		CheckMenuItem(hm, IDM_WINDOWS_SCREEN + i,
		              MF_BYCOMMAND | (data[i].visible ? MF_CHECKED : MF_UNCHECKED));
	}
#endif
#ifdef USE_GRAPHICS
	/* Item "Graphics" */
	CheckMenuItem(hm, IDM_OPTIONS_GRAPHICS,
	              MF_BYCOMMAND | (use_graphics ? MF_CHECKED : MF_UNCHECKED));
#endif
#ifdef USE_SOUND
	/* Item "Sound" */
	CheckMenuItem(hm, IDM_OPTIONS_SOUND,
	              MF_BYCOMMAND | (use_sound ? MF_CHECKED : MF_UNCHECKED));
#endif

#ifdef BEN_HACK 
	/* Item "Colors 16" */
	EnableMenuItem(hm, IDM_OPTIONS_UNUSED,
	               MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
#endif

}


#if 0
/*
 * Check for double clicked (or dragged) savefile
 *
 * Apparently, Windows copies the entire filename into the first
 * piece of the "command line string".  Perhaps we should extract
 * the "basename" of that filename and append it to the "save" dir.
 */
static void check_for_save_file(LPSTR cmd_line)
{
	char *s, *p;

	/* First arg */
	s = cmd_line;

	/* Second arg */
	p = strchr(s, ' ');

	/* Tokenize, advance */
	if (p) *p++ = '\0';

	/* No args */
	if (!*s) return;

/* More stuff moved to server -GP
	// Extract filename
	strcat(savefile, s);

	// Validate the file
	validate_file(savefile);
*/
	/* Game in progress */
	game_in_progress = TRUE;

	/* No restarting */
	disable_start();

	/* Play game */
/*	play_game(FALSE);	-changed to network call -GP */
}
#endif


/*
 * Process a menu command
 */
static void process_menus(WORD wCmd) {
#ifdef	MNU_SUPPORT
	int i;
 #if 0
	OPENFILENAME ofn;
 #endif

	/* Analyze */
	switch (wCmd) {
		/* New game */
		case IDM_FILE_NEW:
 #if 0
			if (!initialized) {
				MessageBox(data[0].w, "You cannot do that yet...",
				           "Warning", MB_ICONEXCLAMATION | MB_OK);
			} else if (game_in_progress) {
				MessageBox(data[0].w,
				           "You can't start a new game while you're still playing!",
				           "Warning", MB_ICONEXCLAMATION | MB_OK);
			} else {
				game_in_progress = TRUE;
				disable_start();
				Term_flush();
				play_game(TRUE);
				quit(NULL);
			}
 #endif
			break;

		// Open game
		case IDM_FILE_OPEN:
 #if 0
			if (!initialized) {
				MessageBox(data[0].w, "You cannot do that yet...",
				           "Warning", MB_ICONEXCLAMATION | MB_OK);
			} else if (game_in_progress) {
				MessageBox(data[0].w,
				           "You can't open a new game while you're still playing!",
				           "Warning", MB_ICONEXCLAMATION | MB_OK);
			} else {
				memset(&ofn, 0, sizeof(ofn));
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = data[0].w;
				ofn.lpstrFilter = "Save Files (*.)\0*\0";
				ofn.nFilterIndex = 1;
				ofn.lpstrFile = savefile;
				ofn.nMaxFile = 1024;
				ofn.lpstrInitialDir = ANGBAND_DIR_USER;
				ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

				if (GetOpenFileName(&ofn)) {
					// Load 'savefile'
					validate_file(savefile);
					game_in_progress = TRUE;
					disable_start();
					Term_flush();
					play_game(FALSE);
					quit(NULL);
				}
			}
 #endif
			break;

		/* Save game */
		case IDM_FILE_SAVE:
 #if 0
			if (!game_in_progress) {
				MessageBox(data[0].w, "No game in progress.",
				           "Warning", MB_ICONEXCLAMATION | MB_OK);
			} else {
				// Save the game
				do_cmd_save_game();
			}
 #endif
			break;


		/* Save and Exit */
		case IDM_FILE_EXIT:
			quit(NULL);
			break;

		/* Quit (no save) */
		case IDM_FILE_QUIT:
			quit(NULL);
			break;

		case IDM_TEXT_SCREEN:
 #ifdef USE_GRAPHICS
			/* XXX XXX XXX */
			if (use_graphics) {
				term_change_bitmap(&data[0]);
				break;
			}
 #endif

			term_change_font(&data[0]);
			break;

		/* Window fonts */
		case IDM_TEXT_MIRROR:
		case IDM_TEXT_RECALL:
		case IDM_TEXT_CHOICE:
		case IDM_TEXT_TERM_4:
		case IDM_TEXT_TERM_5:
		case IDM_TEXT_TERM_6:
		case IDM_TEXT_TERM_7:
			i = wCmd - IDM_TEXT_SCREEN;

			if ((i < 0) || (i >= MAX_TERM_DATA)) break;

			term_change_font(&data[i]);

			break;

		/* Window visibility */
		case IDM_WINDOWS_MIRROR:
		case IDM_WINDOWS_RECALL:
		case IDM_WINDOWS_CHOICE:
		case IDM_WINDOWS_TERM_4:
		case IDM_WINDOWS_TERM_5:
		case IDM_WINDOWS_TERM_6:
		case IDM_WINDOWS_TERM_7:
			i = wCmd - IDM_WINDOWS_SCREEN;

			if ((i < 0) || (i >= MAX_TERM_DATA)) break;

			if (!data[i].visible) {
				data[i].visible = TRUE;
				ShowWindow(data[i].w, SW_SHOW);
				term_data_redraw(&data[i]);
			} else {
				data[i].visible = FALSE;
				ShowWindow(data[i].w, SW_HIDE);
			}

			break;

		/* Hack -- unused */
		case IDM_OPTIONS_UNUSED:
			/* XXX XXX XXX */
			break;

/*	Currently no graphics options available. -GP */
#ifdef USE_GRAPHICS
		case IDM_OPTIONS_GRAPHICS:
			/* XXX XXX XXX  */
			Term_activate(term_screen);

			/* Reset the visuals */
			//reset_visuals();

			/* Toggle "graphics" */
			use_graphics = !use_graphics;

			/* Access the "graphic" mappings */
			handle_process_font_file();

#endif		/* GP's USE_GRAPHICS */
#ifdef USE_GRAPHICS

			/* Use graphics */
			if (use_graphics) {
				/* Try to use the current font */
				if (term_force_graf(&data[0], data[0].font_file)) {
					/* XXX XXX XXX Force a "usable" font */
					(void)term_force_font(&data[0], DEFAULT_FONTNAME);

					/* XXX XXX XXX Force a "usable" graf */
					(void)term_force_graf(&data[0], DEFAULT_TILENAME);
				}

#ifdef FULL_GRAPHICS

				/* Windows */
				for (i = 1; i < MAX_TERM_DATA; i++) {
					/* Try to use the current font */
					if (term_force_graf(&data[i], data[i].font_file)) {
						/* XXX XXX XXX Force a "usable" font */
						(void)term_force_font(&data[i], DEFAULT_FONTNAME);

						/* XXX XXX XXX Force a "usable" graf */
						(void)term_force_graf(&data[i], DEFAULT_TILENAME);
					}
				}

#endif

			}

#endif

#ifdef USE_GRAPHICS	/* no support -GP */
			/* React to changes */
			Term_xtra_win_react();

			/* Hack -- Force redraw */
			Term_key_push(KTRL('R'));

			break;
#endif
		case IDM_OPTIONS_SOUND:
			use_sound = !use_sound;
			break;

		case IDM_OPTIONS_SAVEPREF:
			save_prefs();
			break;
	}
#endif	/* MNU_SUPPORT */
}


/*
 * Redraw a section of a window
 */
static void handle_wm_paint(HWND hWnd, term_data *td)
{
	int x1, y1, x2, y2;
	PAINTSTRUCT ps;

	BeginPaint(hWnd, &ps);

	if (td) {
		/* Get the area that should be updated (rounding up/down) */
		x1 = ps.rcPaint.left - td->size_ow1;
		if (x1 < 0) x1 = 0;
		x1 /= td->font_wid;

		y1 = ps.rcPaint.top - td->size_oh1;
		if (y1 < 0) y1 = 0;
		y1 /= td->font_hgt;

		x2 = ((ps.rcPaint.right - td->size_ow1) / td->font_wid) + 1;
		y2 = ((ps.rcPaint.bottom - td->size_oh1) / td->font_hgt) + 1;

		/* Redraw */
		term_data_redraw_section(td, x1, y1, x2, y2);
	}

	EndPaint(hWnd, &ps);
}


#ifdef BEN_HACK
LRESULT FAR PASCAL AngbandWndProc(HWND hWnd, UINT uMsg,
                                          WPARAM wParam, LPARAM lParam);
#endif

LRESULT FAR PASCAL AngbandWndProc(HWND hWnd, UINT uMsg,
                                          WPARAM wParam, LPARAM lParam)
{
	HDC             hdc;
	term_data      *td;
	MINMAXINFO FAR *lpmmi;
	RECT            rc;
	int             i;

	static int screen_term_cols = -1;//just some dummy vals
	static int screen_term_rows = -1;

	/* Acquire proper "term_data" info */
	td = (term_data *)GetWindowLongPtr(hWnd, 0);
	/* Handle message */
	switch (uMsg) {
		/* XXX XXX XXX */
		case WM_NCCREATE:
			SetWindowLongPtr(hWnd, 0, (LONG_PTR)(td_ptr));
			break;

		/* XXX XXX XXX */
		case WM_CREATE:
			return 0;

		case WM_GETMINMAXINFO:
			lpmmi = (MINMAXINFO FAR *)lParam;

			if (!td) return 1;  /* this message was sent before WM_NCCREATE */

			/* Minimum window size is 8x2 */
			rc.left = rc.top = 0;
			rc.right = rc.left + 8 * td->font_wid + td->size_ow1 + td->size_ow2;
			rc.bottom = rc.top + 2 * td->font_hgt + td->size_oh1 + td->size_oh2 + 1;

			/* Adjust */
#ifdef MNU_USE
			if (td == &data[0]) AdjustWindowRectEx(&rc, td->dwStyle, TRUE, td->dwExStyle);
			else
#endif
			AdjustWindowRectEx(&rc, td->dwStyle, FALSE, td->dwExStyle);

			/* Save minimum size */
			lpmmi->ptMinTrackSize.x = rc.right - rc.left;
			lpmmi->ptMinTrackSize.y = rc.bottom - rc.top;

			/* Maximum window size */
			rc.left = rc.top = 0;
			rc.right = rc.left + (MAX_WINDOW_WID) * td->font_wid + td->size_ow1 + td->size_ow2;
			rc.bottom = rc.top + (MAX_WINDOW_HGT) * td->font_hgt + td->size_oh1 + td->size_oh2;

			/* Paranoia */
			rc.right  += (td->font_wid - 1);
			rc.bottom += (td->font_hgt - 1);

			/* Adjust */
#ifdef MNU_USE
			if (td == &data[0]) AdjustWindowRectEx(&rc, td->dwStyle, TRUE, td->dwExStyle);
			else
#endif
			AdjustWindowRectEx(&rc, td->dwStyle, FALSE, td->dwExStyle);

			/* Save maximum size */
			lpmmi->ptMaxSize.x      = rc.right - rc.left;
			lpmmi->ptMaxSize.y      = rc.bottom - rc.top;

			/* Save maximum size */
			lpmmi->ptMaxTrackSize.x = rc.right - rc.left;
			lpmmi->ptMaxTrackSize.y = rc.bottom - rc.top;

			return 0;

		case WM_PAINT:
			handle_wm_paint(hWnd, td);
			return 0;

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			bool enhanced = FALSE;
			bool mc = FALSE;
			bool ms = FALSE;
			bool ma = FALSE;

			/* Extract the modifiers */
			if (GetKeyState(VK_CONTROL) & 0x8000) mc = TRUE;
			if (GetKeyState(VK_SHIFT)   & 0x8000) ms = TRUE;
			if (GetKeyState(VK_MENU)    & 0x8000) ma = TRUE;

			/* Check for non-normal keys */
			if ((wParam >= VK_PRIOR) && (wParam <= VK_DOWN)) enhanced = TRUE;
			if ((wParam >= VK_F1) && (wParam <= VK_F12)) enhanced = TRUE;
			if ((wParam == VK_INSERT) || (wParam == VK_DELETE)) enhanced = TRUE;

			/* XXX XXX XXX */
			if (enhanced) {
				/* Begin the macro trigger */
				Term_keypress(31);

				/* Send the modifiers */
				if (mc) Term_keypress('C');
				if (ms) Term_keypress('S');
				if (ma) Term_keypress('A');

				/* Extract "scan code" from bits 16..23 of lParam */
				i = LOBYTE(HIWORD(lParam));

				/* Introduce the scan code */
				Term_keypress('x');

				/* Encode the hexidecimal scan code */
				Term_keypress(hexsym[i/16]);
				Term_keypress(hexsym[i%16]);

				/* End the macro trigger */
				Term_keypress(13);

				return 0;
			}

			break;
		}

		case WM_CHAR:
			Term_keypress(wParam);
			return 0;

		case WM_INITMENU:
			setup_menus();
			return 0;

		case WM_CLOSE:
		case WM_QUIT:
			quit(NULL);
			return 0;

		case WM_COMMAND:
			process_menus(LOWORD(wParam));
			return 0;

		case WM_SIZE:
			if (!td) return 1;    /* this message was sent before WM_NCCREATE */
			if (!td->w) return 1; /* it was sent from inside CreateWindowEx */
			if (td->size_hack) return 1; /* was sent from WM_SIZE */

			switch (wParam) {
				case SIZE_MINIMIZED:
					for (i = 1; i < MAX_TERM_DATA; i++)
						if (data[i].visible) ShowWindow(data[i].w, SW_HIDE);

					return 0;

				case SIZE_MAXIMIZED:
					/* fall through XXX XXX XXX */

				case SIZE_RESTORED: {
					int rows, cols;
					int old_rows, old_cols;

					td->size_hack = TRUE;

					cols = (LOWORD(lParam) - td->size_ow1 - td->size_ow2) / td->font_wid;
					rows = (HIWORD(lParam) - td->size_oh1 - td->size_oh2) / td->font_hgt;

					old_cols = cols;
					old_rows = rows;

		                        /* Hack -- do not allow bad resizing of main screen */
		                        cols = 80;
		                        /* respect big_map option */
		                        if (rows <= 24 || (in_game && !(sflags1 & SFLG1_BIG_MAP))) rows = 24;
		                        else rows = 46;

					/* Remember for WM_EXITSIZEMOVE later */
					screen_term_cols = cols;
					screen_term_rows = rows;

					/* Windows */
					for (i = 1; i < MAX_TERM_DATA; i++)
						if (data[i].visible) ShowWindow(data[i].w, SW_SHOWNOACTIVATE);

					td->size_hack = FALSE;

					return 0;
				}
			}
			break;

		case WM_EXITSIZEMOVE:
		{
			term *old;

			/* Remember final size values from WM_SIZE -> SIZE_RESTORED */
			if ((screen_term_rows == 24 && Client_setup.options[CO_BIGMAP]) ||
			    (screen_term_rows == 46 && !Client_setup.options[CO_BIGMAP])) {
		                bool val = !Client_setup.options[CO_BIGMAP];

		                Client_setup.options[CO_BIGMAP] = val;
		                c_cfg.big_map = val;

                    		if (!val) screen_hgt = SCREEN_HGT;
                    		else screen_hgt = MAX_SCREEN_HGT;

	                        /* hack: need to redraw map or it may look cut off */
                                if (in_game) {
                                        if (screen_icky) Term_switch(0);
	                                Term_clear(); /* get rid of map tiles where now status bars go instead */
    		                        if (screen_icky) Term_switch(0);
	                                Send_screen_dimensions();
        		                cmd_redraw();
    		    		}
			}

			/* Set main window size */
			td->cols = screen_term_cols;
			td->rows = screen_term_rows;
			term_getsize(td);
			old = Term;
                        Term_activate(&td->t);
                        Term_resize(screen_term_cols, screen_term_rows);
			term_window_resize(td);//not required? used in resize_main_window though
			Term_activate(old);

			MoveWindow(hWnd, td->pos_x, td->pos_y, td->size_wid, td->size_hgt, TRUE);
			return 0;
		}

		case WM_PALETTECHANGED:
			/* ignore if palette change caused by itself */
			if ((HWND)wParam == hWnd) return FALSE;
			/* otherwise, fall through!!! */

		case WM_QUERYNEWPALETTE:
			if (!paletted) return FALSE;
			hdc = GetDC(hWnd);
			SelectPalette(hdc, hPal, FALSE);
			i = RealizePalette(hdc);
			/* if any palette entries changed, repaint the window. */
			if (i) InvalidateRect(hWnd, NULL, TRUE);
			ReleaseDC(hWnd, hdc);
			return FALSE;

		case WM_ACTIVATE:
			if (wParam && !HIWORD(lParam)) {
				for (i = 1; i < MAX_TERM_DATA; i++)
					SetWindowPos(data[i].w, hWnd, 0, 0, 0, 0,
					    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
				SetFocus(hWnd);
				return 0;
			}
			break;
	}


	/* Oops */
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


#ifdef BEN_HACK
LRESULT FAR PASCAL AngbandListProc(HWND hWnd, UINT uMsg,
                                           WPARAM wParam, LPARAM lParam);
#endif

LRESULT FAR PASCAL AngbandListProc(HWND hWnd, UINT uMsg,
                                           WPARAM wParam, LPARAM lParam)
{
	term_data      *td;
	MINMAXINFO FAR *lpmmi;
	RECT            rc;
	HDC             hdc;
	int             i;


	/* Acquire proper "term_data" info */
	td = (term_data *)GetWindowLongPtr(hWnd, 0);

	/* Process message */
//if (in_game) c_message_add(format("uMsg %d", uMsg));
	switch (uMsg) {
		/* XXX XXX XXX */
		case WM_NCCREATE:
			SetWindowLongPtr(hWnd, 0, (LONG_PTR)(td_ptr));
			break;

		/* XXX XXX XXX */
		case WM_CREATE:
			return 0;

		case WM_GETMINMAXINFO:
			if (!td) return 1;  /* this message was sent before WM_NCCREATE */

			lpmmi = (MINMAXINFO FAR *)lParam;

			/* Minimum size */
			rc.left = rc.top = 0;
			rc.right = rc.left + 8 * td->font_wid + td->size_ow1 + td->size_ow2;
			rc.bottom = rc.top + 2 * td->font_hgt + td->size_oh1 + td->size_oh2;

			/* Adjust */
#ifdef MNU_USE
			if (td == &data[0]) AdjustWindowRectEx(&rc, td->dwStyle, TRUE, td->dwExStyle);
			else
#endif
			AdjustWindowRectEx(&rc, td->dwStyle, FALSE, td->dwExStyle);

			/* Save the minimum size */
			lpmmi->ptMinTrackSize.x = rc.right - rc.left;
			lpmmi->ptMinTrackSize.y = rc.bottom - rc.top;

			/* Maximum window size */
			rc.left = rc.top = 0;
			rc.right = rc.left + (MAX_WINDOW_WID) * td->font_wid + td->size_ow1 + td->size_ow2;
			rc.bottom = rc.top + (MAX_WINDOW_HGT) * td->font_hgt + td->size_oh1 + td->size_oh2;

			/* Paranoia */
			rc.right += (td->font_wid - 1);
			rc.bottom += (td->font_hgt - 1);

			/* Adjust */
#ifdef MNU_USE
			if (td == &data[0]) AdjustWindowRectEx(&rc, td->dwStyle, TRUE, td->dwExStyle);
			else
#endif
			AdjustWindowRectEx(&rc, td->dwStyle, FALSE, td->dwExStyle);

			/* Save maximum size */
			lpmmi->ptMaxSize.x      = rc.right - rc.left;
			lpmmi->ptMaxSize.y      = rc.bottom - rc.top;

			/* Save the maximum size */
			lpmmi->ptMaxTrackSize.x = rc.right - rc.left;
			lpmmi->ptMaxTrackSize.y = rc.bottom - rc.top;

			return 0;

		case WM_SIZE:
			if (!td) return 1;    /* this message was sent before WM_NCCREATE */
			if (!td->w) return 1; /* it was sent from inside CreateWindowEx */
			if (td->size_hack) return 1; /* was sent from inside WM_SIZE */

			if (wParam == SIZE_RESTORED) {
				term *old;
				int cols, rows;

				td->size_hack = TRUE;

				cols = (LOWORD(lParam) - td->size_ow1 - td->size_ow2) / td->font_wid;
				rows = (HIWORD(lParam) - td->size_oh1 - td->size_oh2) / td->font_hgt;

				old = Term;
                                Term_activate(&td->t);
                                Term_resize(cols, rows);
				Term_activate(old);

            	        	/* In case we resized Chat/Msg/Msg+Chat window,
                		   refresh contents so they are displayed properly,
                    		   without having to wait for another incoming message to do it for us. */
	                        p_ptr->window |= PW_MESSAGE | PW_CHAT | PW_MSGNOCHAT;

				td->size_hack = FALSE;

				return 0;
			}
			break;

		case WM_PAINT:
			handle_wm_paint(hWnd, td);

			return 0;

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			bool enhanced = FALSE;
			bool mc = FALSE;
			bool ms = FALSE;
			bool ma = FALSE;

			/* Extract the modifiers */
			if (GetKeyState(VK_CONTROL) & 0x8000) mc = TRUE;
			if (GetKeyState(VK_SHIFT)   & 0x8000) ms = TRUE;
			if (GetKeyState(VK_MENU)    & 0x8000) ma = TRUE;

			/* Check for non-normal keys */
			if ((wParam >= VK_PRIOR) && (wParam <= VK_DOWN)) enhanced = TRUE;
			if ((wParam >= VK_F1) && (wParam <= VK_F12)) enhanced = TRUE;
			if ((wParam == VK_INSERT) || (wParam == VK_DELETE)) enhanced = TRUE;

			/* XXX XXX XXX */
			if (enhanced) {
				/* Begin the macro trigger */
				Term_keypress(31);

				/* Send the modifiers */
				if (mc) Term_keypress('C');
				if (ms) Term_keypress('S');
				if (ma) Term_keypress('A');

				/* Extract "scan code" from bits 16..23 of lParam */
				i = LOBYTE(HIWORD(lParam));

				/* Introduce the scan code */
				Term_keypress('x');

				/* Encode the hexidecimal scan code */
				Term_keypress(hexsym[i / 16]);
				Term_keypress(hexsym[i % 16]);

				/* End the macro trigger */
				Term_keypress(13);

				return 0;
			}

			break;
		}

		case WM_CHAR:
			Term_keypress(wParam);
			return 0;

		case WM_PALETTECHANGED:
			/* ignore if palette change caused by itself */
			if ((HWND)wParam == hWnd) return FALSE;
			/* otherwise, fall through!!! */

		case WM_QUERYNEWPALETTE:
			if (!paletted) return 0;
			hdc = GetDC(hWnd);
			SelectPalette(hdc, hPal, FALSE);
			i = RealizePalette(hdc);
			/* if any palette entries changed, repaint the window. */
			if (i) InvalidateRect(hWnd, NULL, TRUE);
			ReleaseDC(hWnd, hdc);
			return 0;

		case WM_NCLBUTTONDOWN:
			if (wParam == HTSYSMENU) {
				/* Hide sub-windows */
				if (td != &data[0]) {
					if (td->visible) {
						td->visible = FALSE;
						ShowWindow(td->w, SW_HIDE);
					}
				}
				return 0;
			}
			break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}





/*** Temporary Hooks ***/


/*
 * Error message -- See "z-util.c"
 */
static void hack_plog(cptr str)
{
	/* Give a warning */
	if (str) MessageBox(NULL, str, "Warning", MB_OK);
}


/*
 * Quit with error message -- See "z-util.c"
 */
static void hack_quit(cptr str)
{
	/* Give a warning */
	if (str) MessageBox(NULL, str, "Quitting", MB_OK | MB_ICONSTOP);

	/* Unregister the classes */
	UnregisterClass(AppName, hInstance);

	/* Destroy the icon */
	if (hIcon) DestroyIcon(hIcon);

	/* Cleanup WinSock */
	WSACleanup();

	/* Exit */
	if (str) exit(-1);
	exit(0);
}


/*
 * Fatal error (see "z-util.c")
 */
static void hack_core(cptr str)
{
	/* Give a warning */
	if (str) MessageBox(NULL, str, "Error", MB_OK | MB_ICONSTOP);

	/* Quit */
	quit(NULL);
}


/*** Various hooks ***/


/*
 * Error message -- See "z-util.c"
 */
static void hook_plog(cptr str)
{
	/* Warning */
	if (str) MessageBox(data[0].w, str, "Warning", MB_OK);
}


/*
 * Quit with error message -- See "z-util.c"
 *
 * A lot of this function should be handled by actually calling
 * the "term_nuke()" function hook for each window.  XXX XXX XXX
 */
static void hook_quit(cptr str) {
	RECT rc;
	int i, res = save_chat;

#if 0
#ifdef USE_SOUND_2010
	/* let the sound fade out, also helps the user to realize
	   he's been disconnected or something - C. Blue */
 #ifdef SOUND_SDL
	mixer_fadeall();
 #endif
#endif
#endif
	Net_cleanup();

	c_quit = 1;

	/* Give a warning */
	if (str && *str) MessageBox(data[0].w, str, "Error", MB_OK | MB_ICONSTOP);

	/* Copied from quit_hook in c-init.c - mikaelh */
	if (message_num() && (res || (res = get_3way("Save chat log/all messages?", TRUE)))) {
		FILE *fp;
		char buf[80], buf2[1024];
		time_t ct = time(NULL);
		struct tm* ctl = localtime(&ct);

		if (res == 1) strcpy(buf, "tomenet-chat_");
		else strcpy(buf, "tomenet-messages_");
		strcat(buf, format("%04d-%02d-%02d_%02d.%02d.%02d",
		    1900 + ctl->tm_year, ctl->tm_mon + 1, ctl->tm_mday,
		    ctl->tm_hour, ctl->tm_min, ctl->tm_sec));
		strcat(buf, ".txt");

		i = message_num();
		if (!save_chat) get_string("Filename:", buf, 79);
		path_build(buf2, 1024, ANGBAND_DIR_USER, buf);
		fp = my_fopen(buf2, "w");
		if (fp != (FILE*)NULL) {
			dump_messages_aux(fp, i, 2 - res, FALSE);
			fclose(fp);
		}
	}

#if 1
#ifdef USE_SOUND_2010
	/* let the sound fade out, also helps the user to realize
	   he's been disconnected or something - C. Blue */
 #ifdef SOUND_SDL
	mixer_fadeall();
 #endif
#endif
#endif

	/* Hack - Save the window positions before destroying them - mikaelh */
	/* Main window */
	GetWindowRect(data[0].w, &rc);
	data[0].pos_x = rc.left;
	data[0].pos_y = rc.top;
	/* Sub-Windows */
	for (i = MAX_TERM_DATA - 1; i >= 1; i--) {
		GetWindowRect(data[i].w, &rc);
		data[i].pos_x = rc.left;
		data[i].pos_y = rc.top;
	}

	/* Save the preferences.
	   Note: Moved this here before destroying terms, to save Font choice
	         which can be modified with new '=f' option - C. Blue */
	/* Note: This takes time with anti-virus on */
	save_prefs();

#ifdef RETRY_LOGIN
	/* don't kill the windows and all */
	if (rl_connection_state >= 2) return;
#endif

	/* Destroy the windows */
	/* Sub-Windows */
	for (i = MAX_TERM_DATA - 1; i >= 1; i--) {
		term_force_font(&data[i], NULL);
		if (data[i].font_want) string_free(data[i].font_want);
		if (data[i].graf_want) string_free(data[i].graf_want);
		if (data[i].w) DestroyWindow(data[i].w);
		data[i].w = 0;
	}
	/* Main window */
#ifdef USE_GRAPHICS
	term_force_graf(&data[0], NULL);
#endif
	term_force_font(&data[0], NULL);
	if (data[0].font_want) string_free(data[0].font_want);
	if (data[0].graf_want) string_free(data[0].graf_want);
	if (data[0].w) DestroyWindow(data[0].w);
	data[0].w = 0;

	/* Nuke each term */
	for (i = ANGBAND_TERM_MAX - 1; i >= 0; i--) {
		/* Unused */
		if (!ang_term[i]) continue;

		/* Nuke it */
		term_nuke(ang_term[i]);
	}

	/*** Free some other stuff ***/

	DeleteObject(hbrYellow);

	if (hPal) DeleteObject(hPal);

	UnregisterClass(AppName, hInstance);

	if (hIcon) DestroyIcon(hIcon);

	WSACleanup();

	if (str) exit(-1);
	exit(0);
}



/*** Initialize ***/


/*
 * Init some stuff
 */
static void init_stuff(void) {
	int   i;
	char path[1024];
	FILE *fp0;


	/* Hack -- access "ANGBAND.INI" */
	GetModuleFileName(hInstance, path, 512);
	strcpy(path + strlen(path) - 4, ".ini");

	/* If tomenet.ini file doesn't exist yet, check for tomenet.ini.default
	   and copy it over. This way a full client update won't kill our config. */
	fp0 = fopen(path, "r");
	if (!fp0) {
		char path2[1024];

		GetModuleFileName(hInstance, path2, 512);
		strcpy(path2 + strlen(path2) - 4, ".ini.default");

		if (check_file(path2)) {
			/* copy it */
#if 0
			char out_val[2048];
			sprintf(out_val, "copy %s %s", path2, path);
            		system(out_val);
#else
			FILE *fp, *fp2;
			char buf[1024];
			buf[0] = 0;
			fp = fopen(path, "w");
			fp2 = fopen(path2, "r");
			if (!fp) quit(format("error: can't open %s for writing", path));
			if (!fp2) quit(format("error: can't open %s for reading", path2));
			while (!feof(fp2)) {
				fgets(buf, 1024, fp2);
				if (!feof(fp2)) fputs(buf, fp);
			}
			fclose(fp2);
			fclose(fp);
#endif
		} else plog(format("Error: Neither %s nor %s exists", path, path2));
	}

	/* Save "ANGBAND.INI" */
	ini_file = string_make(path);

	/* Validate "ANGBAND.INI" */
	validate_file(ini_file);


	/* XXX XXX XXX */
	GetPrivateProfileString("Base", "LibPath", "lib", path, 1000, ini_file);

	/* Analyze the path */
	i = strlen(path);

	/* Require a path */
	if (!i) quit("LibPath shouldn't be empty in ANGBAND.INI");

	/* Nuke terminal backslash */
	if (i && (path[i-1] != '\\')) {
		path[i++] = '\\';
		path[i] = '\0';
	}

	/* Validate the path */
	validate_dir(path);


	/* Windows 7 and higher: Usually user data is moved to the system-
	   specified user folder, but this option will allow to prevent that
	   and instead keep using the folder where TomeNET is installed:
	   Note: This option must be read before init_file_paths() and init_sound(). */
	win_dontmoveuser = (GetPrivateProfileInt("Base", "DontMoveUser", 0, ini_file) != 0);


	/* Init the file paths */

	init_file_paths(path);
 
	/* Hack -- Validate the paths */
	validate_dir(ANGBAND_DIR_SCPT);
	validate_dir(ANGBAND_DIR_TEXT);
	validate_dir(ANGBAND_DIR_USER);
#if !defined(USE_LOGFONT) || defined(USE_GRAPHICS) || defined(USE_SOUND)
	validate_dir(ANGBAND_DIR_XTRA);	/* Sounds & Graphics */
#endif
	validate_dir(ANGBAND_DIR_GAME);

#ifndef USE_LOGFONT
	// Build the "font" path
	path_build(path, 1024, ANGBAND_DIR_XTRA, "font");

	// Allocate the path
	ANGBAND_DIR_XTRA_FONT = string_make(path);

	// Validate the "font" directory
	validate_dir(ANGBAND_DIR_XTRA_FONT);

	// Build the filename 
	path_build(path, 1024, ANGBAND_DIR_XTRA_FONT, DEFAULT_FONTNAME);

	// Hack -- Validate the basic font
	validate_file(path);
#endif


#ifdef USE_GRAPHICS

	/* Build the "graf" path */
	path_build(path, 1024, ANGBAND_DIR_XTRA, "graf");

	/* Allocate the path */
	ANGBAND_DIR_XTRA_GRAF = string_make(path);

	/* Validate the "graf" directory */
	validate_dir(ANGBAND_DIR_XTRA_GRAF);

	/* Build the filename */
	path_build(path, 1024, ANGBAND_DIR_XTRA_GRAF, DEFAULT_TILENAME);

	/* Hack -- Validate the basic graf */
	validate_file(path);

#endif


#ifdef USE_SOUND
#ifndef USE_SOUND_2010

	/* Build the "sound" path */
	path_build(path, 1024, ANGBAND_DIR_XTRA, "sound");

	/* Allocate the path */
	ANGBAND_DIR_XTRA_SOUND = string_make(path);

	/* Validate the "sound" directory */
	validate_dir(ANGBAND_DIR_XTRA_SOUND);

#endif
#endif

}


/*
 * Get a number from command line
 */
static int cmd_get_number(char *str, int *number)
{
	int i, tmp = 0;

	/* Skip any spaces */
	for (i = 0; str[i] == ' ' && str[i] != '\0'; i++);

	for (; str[i] != '\0'; i++) {
		/* Confirm number */
		if ('0' <= str[i] && str[i] <= '9') {
			tmp *= 10;
			tmp += str[i] - '0';
		}
		else break;
	}

	*number = tmp;

	/* Find the next space */
	for (; str[i] != ' ' && str[i] != '\0'; i++);

	return i;
}

/*
 * Get a string from command line.
 * dest is the destination buffer and n is the size of that buffer.
 */
static int cmd_get_string(char *str, char *dest, int n)
{
	int start, end, len;

	/* Skip any spaces */
	for (start = 0; str[start] == ' ' && str[start] != '\0'; start++);

	/* Check for a double quote */
	if (str[start] == '"') {
		start++;

		/* Find another double quote */
		for (end = start; str[end] != '"' && str[end] != '\0'; end++);
	} else {
		/* Find the next space */
		for (end = start + 1; str[end] != ' ' && str[end] != '\0'; end++);
	}

	len = end - start;

	/* Make sure it doesn't overflow */
	if (len > n - 1) len = n - 1;

	if (len > 0) strncpy(dest, &str[start], len);

	/* Always terminate */
	if (len >= 0)
		dest[len] = '\0';

	return end + 1;
}

/* Turn off the num-lock key by toggling it if it's currently on. */
static void turn_off_numlock(void) {

	/* nothing to do if user doesn't want to auto-kill numlock */
	if (!disable_numlock) return;

	KEYBDINPUT k;
	INPUT inp;
	int r;

	/* alreay off? Then nothing to do */
	if (!(GetKeyState(VK_NUMLOCK) & 0xFFFF)) return;

	/* Numlock key */
	k.wVk = VK_NUMLOCK;
	k.wScan = 0;
	k.time = 0;
	k.dwFlags = 0;
	k.dwExtraInfo = 0;

	/* Press it */
	inp.type = INPUT_KEYBOARD;
	inp.ki = k;
	r = SendInput(1, &inp, sizeof(INPUT));
	if (!r) {
		DWORD err = GetLastError();
		char *msg = (char*)malloc(sizeof(char) * 50);
		sprintf(msg, "SendInput error (down): %lu", err);
		plog(msg);
	}

	/* Release it */
	k.dwFlags = KEYEVENTF_KEYUP;
	inp.type = INPUT_KEYBOARD;
	inp.ki = k;
	r = SendInput(1, &inp, sizeof(INPUT));
	if (!r) {
		DWORD err = GetLastError();
		char *msg = (char*)malloc(sizeof(char) * 50);
		sprintf(msg, "SendInput error (up): %lu", err);
		plog(msg);
	}
}

int FAR PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrevInst,
                       LPSTR lpCmdLine, int nCmdShow) {
	WNDCLASS wc;
	HDC      hdc;
	MSG      msg;
	WSADATA  wsadata;
	TIMECAPS tc;
	UINT     wTimerRes;

	hInstance = hInst;  /* save in a global var */

	int i, n;
	bool done = FALSE;
	u32b seed;

	/* make version strings. */
	version_build();

	/* assume defaults */
	strcpy(cfg_soundpackfolder, "sound");
	strcpy(cfg_musicpackfolder, "music");

	/* set OS-specific resize_main_window() hook */
	resize_main_window = resize_main_window_win;

	if (hPrevInst == NULL) {
		wc.style         = CS_CLASSDC;
		wc.lpfnWndProc   = AngbandWndProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 4; /* one long pointer to term_data */
		wc.hInstance     = hInst;
		wc.hIcon         = hIcon = LoadIcon(hInst, AppName);
		wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = GetStockObject(BLACK_BRUSH);
		wc.lpszMenuName  = AppName;
		wc.lpszClassName = AppName;

		if (!RegisterClass(&wc)) exit(1);

		wc.lpfnWndProc   = AngbandListProc;
		wc.lpszMenuName  = NULL;
		wc.lpszClassName = AngList;

		if (!RegisterClass(&wc)) exit(2);

	}

	/* Set hooks */
	quit_aux = hack_quit;
	core_aux = hack_core;
	plog_aux = hack_plog;

	/* Prepare the filepaths */
	init_stuff();

	/* Initialize WinSock */
	WSAStartup(MAKEWORD(1, 1), &wsadata);

	/* Try to set timer resolution to 1ms - mikaelh */
	if (timeGetDevCaps(&tc, sizeof (tc)) == TIMERR_NOERROR) {
		wTimerRes = min(max(tc.wPeriodMin, 1), tc.wPeriodMax);
		timeBeginPeriod(wTimerRes);
	}

	/* Determine if display is 16/256/true color */
	hdc = GetDC(NULL);
	colors16 = (GetDeviceCaps(hdc, BITSPIXEL) == 4);
	paletted = ((GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) ? TRUE : FALSE);
	ReleaseDC(NULL, hdc);
#ifdef USE_GRAPHICS
	/* Initialize "color_table" */
 #ifndef EXTENDED_COLOURS_PALANIM
	for (i = 0; i < 16; i++) {
 #else
	for (i = 0; i < 16 + 16; i++) {
 #endif
		/* Save the "complex" codes */
		color_table[i][1] = GetRValue(win_clr[i]);
		color_table[i][2] = GetGValue(win_clr[i]);
		color_table[i][3] = GetBValue(win_clr[i]);

		/* Save the "simple" code */
		color_table[i][0] = win_pal[i];
	}
#endif

	/* Prepare the windows */
	init_windows();

	/* Activate hooks */
	plog_aux = hook_plog;
	quit_aux = hook_quit;
	core_aux = hook_quit;

	/* Set the system suffix */
	ANGBAND_SYS = "win";

	/* We are now initialized */
	initialized = TRUE;

#if 0 /* apparently this wasn't so good after all - mikaelh */
	/* Check if we loaded some account name & password from the .ini file - mikaelh */
	if (strlen(nick) && strcmp(nick, "PLAYER") && strlen(pass) && strcmp(pass, "passwd"))
		done = TRUE;
#endif

	/* Process the command line */
	for (i = 0, n = strlen(lpCmdLine); i < n; i++) {
		/* Check for an option */
		if (lpCmdLine[i] == '-') {
			i++;

			switch (lpCmdLine[i]) {
				case 'C': /* compatibility mode */
					server_protocol = 1;
					break;
				case 'F':
					i += cmd_get_number(&lpCmdLine[i + 1], (int*)&cfg_client_fps);
					break;
				case 'h':
					/* Attempt to print out some usage information */
#if 0 /* we don't have the console attached anymore */
					puts(longVersion);
					puts("Usage  : tomenet [options] [servername]");
					puts("Example: tomenet -lMorgoth MorgyPass -p18348 europe.tomenet.eu");
					puts("  -C                 Compatibility mode for very old servers");
					puts("  -F                 Client FPS");
					puts("  -l<nick> <passwd>  Login as");
					puts("  -N<name>           character Name");
					puts("  -R<name>           character Name, auto-reincarnate");
					puts("  -p<num>            change game Port number");
					puts("  -P<path>           set the lib directory Path");
					puts("  -q                 disable audio capabilities ('quiet mode')");
					puts("  -w                 disable client-side weather effects");
					puts("  -u                 disable client-side automatic lua updates");
					puts("  -k                 don't disable numlock on client startup");
					puts("  -m                 skip motd (message of the day) on login");
					puts("  -v                 save chat log on exit instead of asking");
#else
					plog(format("%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s",
					    longVersion,
					    "Usage  : tomenet [options] [servername]",
					    "Example: tomenet -lMorgoth MorgyPass",
					    "                 -p18348 europe.tomenet.eu",
					    "  -C                 Compatibility mode for very old servers",
					    "  -F                 Client FPS",
					    "  -k                 don't disable numlock on client startup",
					    "  -l<nick> <passwd>  Login as",
					    "  -m                 skip motd (message of the day) on login",
					    "  -N<name>           character name",
					    "  -R<name>           character name, auto-reincarnate",
					    "  -p<num>            change game Port number",
					    "  -P<path>           set the lib directory Path",
					    "  -q                 disable audio capabilities ('quiet mode')",
					    "  -u                 disable client-side automatic lua updates",
					    "  -v                 save chat log on exit, don't prompt",
					    "  -v                 save complete message log on exit, don't prompt",
					    "  -w                 disable client-side weather effects"));
#endif
					quit(NULL);
					break;
				case 'l': /* account name & password */
					i += cmd_get_string(&lpCmdLine[i + 1], nick, MAX_CHARS);
					i += cmd_get_string(&lpCmdLine[i + 1], pass, MAX_CHARS);
					done = TRUE;
					break;
				case 'R':
					auto_reincarnation = TRUE;
				case 'N': /* character name */
					i += cmd_get_string(&lpCmdLine[i + 1], cname, MAX_CHARS);
					break;
				case 'p': /* port */
					i += cmd_get_number(&lpCmdLine[i + 1], (int*)&cfg_game_port);
					break;
				case 'P': /* lib directory path */
					i += cmd_get_string(&lpCmdLine[i + 1], path, 1024);
					break;
				case 'q':
					quiet_mode = TRUE;
					break;
				case 'w':
					noweather_mode = TRUE;
					break;
				case 'u':
					no_lua_updates = TRUE;
					break;
				case 'k':
					disable_numlock = FALSE;
					break;
				case 'm':
					skip_motd = TRUE;
					break;
				case 'v':
					save_chat = 1;
					break;
				case 'V':
					save_chat = 2;
					break;
			}
		} else if (lpCmdLine[i] == ' ') {
			/* Ignore spaces */
		} else {
			/* Get a server name */
			i += cmd_get_string(&lpCmdLine[i], svname, MAX_CHARS);
		}
	}

	/* Bam! */
	turn_off_numlock();

	if (quiet_mode) use_sound = FALSE;

	/* Basic seed */
	seed = (time(NULL));

	/* Use the complex RNG */
	Rand_quick = FALSE;

	/* Seed the "complex" RNG */
	Rand_state_init(seed);

	/* Do network initialization, etc. */
	/* Add ability to specify cmdline to windows  -Zz*/
	if (strlen(svname) > 0)
		client_init(svname, done);
	else
		/* Initialize and query metaserver */
		client_init(NULL, done);

	/* Process messages forever */
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	Net_cleanup();

	/* Paranoia */
	quit(NULL);

	/* Paranoia */
	return (0);
}


/* EXPERIMENTAL // allow user to change main window font live - C. Blue
   So far only uses 1 parm ('s') to switch between hardcoded choices:
   -1 - cycle
    0 - normal
    1 - big
    2 - bigger
    3 - huge */
void change_font(int s) {
	/* use main window font for measuring */
	char tmp[128] = "";
	if (data[0].font_file) strcpy(tmp, data[0].font_file);
	else strcpy(tmp, DEFAULT_FONTNAME);

	/* cycle? */
	if (s == -1) {
		if (strstr(tmp, "8X13")) s = 1;
		else if (strstr(tmp, "10X14")) s = 2;
		else if (strstr(tmp, "12X18")) s = 3;
		else if (strstr(tmp, "16X24")) s = 0;
	}

	/* Force the font */
	switch (s) {
	case 0:
		/* change main window font */
		term_force_font(&data[0], "8X13.FON");
		/* Change sub windows too */
		term_force_font(&data[1], "8X13.FON"); //msg
		term_force_font(&data[2], "8X13.FON"); //inv
		term_force_font(&data[3], "5X8.FON"); //char
		term_force_font(&data[4], "6X10.FON"); //chat
		term_force_font(&data[5], "6X10.FON"); //eq (5x8)
		term_force_font(&data[6], "5X8.FON");
		term_force_font(&data[7], "5X8.FON");
		break;
	case 1:
		/* change main window font */
		term_force_font(&data[0], "10X14X.FON");//or 9X15 maybe
		/* Change sub windows too */
		term_force_font(&data[1], "8X13.FON");//was 9x15
		term_force_font(&data[2], "8X13.FON");//was 9x15
		term_force_font(&data[3], "6X10.FON");
		term_force_font(&data[4], "8X13.FON");
		term_force_font(&data[5], "6X10.FON");
		term_force_font(&data[6], "6X10.FON");
		term_force_font(&data[7], "6X10.FON");
		break;
	case 2:
		/* change main window font */
		term_force_font(&data[0], "12X18X.FON");
		/* Change sub windows too */
		term_force_font(&data[1], "9X15.FON");
		term_force_font(&data[2], "9X15.FON");
		term_force_font(&data[3], "8X13.FON");
		term_force_font(&data[4], "9X15.FON");
		term_force_font(&data[5], "8X13.FON");
		term_force_font(&data[6], "8X13.FON");
		term_force_font(&data[7], "8X13.FON");
		break;
	case 3:
		/* change main window font */
		term_force_font(&data[0], "16X24X.FON");
		/* Change sub windows too */
		term_force_font(&data[1], "12X18X.FON");
		term_force_font(&data[2], "12X18X.FON");
		term_force_font(&data[3], "9X15.FON");
		term_force_font(&data[4], "12X18X.FON");
		term_force_font(&data[5], "9X15.FON");
		term_force_font(&data[6], "9X15.FON");
		term_force_font(&data[7], "9X15.FON");
		break;
	}
}

void resize_main_window_win(int cols, int rows) {
	term_data *td = &data[0];
	term *old;
	term *t = &td->t;

	td->cols = cols;
	td->rows = rows;
	term_getsize(td);
	term_window_resize(td);
	old = Term;
	Term_activate(t);
	Term_resize(td->cols, td->rows);
	Term_activate(old);
}

bool ask_for_bigmap(void) {
#if 0
	if (MessageBox(NULL,
	    "Do you want to double the height of this window?\n"
	    "It is recommended to do this on desktops,\n"
	    "but it may not fit on small netbook screens.\n"
	    "You can change this later anytime in the game's options menu.",
	    "Enable 'big_map' option?",
	    MB_YESNO + MB_ICONQUESTION) == IDYES)
		return TRUE;
	return FALSE;
#else
	return ask_for_bigmap_generic();
#endif
}

const char* get_font_name(int term_idx) {
	if (data[term_idx].font_file) return(data[term_idx].font_file);
	else return DEFAULT_FONTNAME;
}
void set_font_name(int term_idx, char* fnt) {
	char fnt2[256], *fnt_ptr = fnt;
	while (strchr(fnt_ptr, '\\')) fnt_ptr = strchr(fnt_ptr, '\\') + 1;
	strcpy(fnt2, fnt_ptr);
	term_force_font(&data[term_idx], fnt2);
}
void term_toggle_visibility(int term_idx) {
	data[term_idx].visible = !data[term_idx].visible;
}
bool term_get_visibility(int term_idx) {
	return data[term_idx].visible;
}

/* automatically store name+password to ini file if we're a new player? */
void store_crecedentials(void) {
	char tmp[MAX_CHARS];

	strcpy(tmp, pass);
	my_memfrob(tmp, strlen(tmp));

	WritePrivateProfileString("Online", "nick", nick, ini_file);
	WritePrivateProfileString("Online", "pass", tmp, ini_file);

	memset(tmp, 0, MAX_CHARS);
}
void store_audiopackfolders(void) {
	WritePrivateProfileString("Base", "SoundpackFolder", cfg_soundpackfolder, ini_file);
	WritePrivateProfileString("Base", "MusicpackFolder", cfg_musicpackfolder, ini_file);
}
void get_screen_font_name(char *buf) {
	if (data[0].font_file) strcpy(buf, data[0].font_file);
	else strcpy(buf, "");
}

/* Palette animation - 2018 *testing* */
void animate_palette(void) {
	byte i;
	byte rv, gv, bv;
	COLORREF code;

	static bool init = FALSE;
	static unsigned char ac = 0x00; //animatio


	/* Initialise the palette once. For some reason colour_table[] is all zero'ed again at the beginning. */
	if (!init) {
#ifndef EXTENDED_COLOURS_PALANIM
		for (i = 0; i < 16; i++) {
#else
 #ifdef PALANIM_SWAP
		for (i = 0; i < 16; i++) {
 #else
		for (i = 0; i < 16 + 16; i++) {
 #endif
#endif
			/* Extract desired values */
			rv = color_table[i][1];
			gv = color_table[i][2];
			bv = color_table[i][3];

			/* Extract a full color code */
			code = PALETTERGB(rv, gv, bv);

			//c_message_add(format("currently: [%d] %d -> %d (%d,%d,%d)", i, win_clr[i], code, rv, gv, bv));

			/* Save the "complex" codes */
			color_table[i][1] = GetRValue(win_clr[i]);
			color_table[i][2] = GetGValue(win_clr[i]);
			color_table[i][3] = GetBValue(win_clr[i]);

			/* Save the "simple" code */
			color_table[i][0] = win_pal[i];
		}
		init = TRUE;
		return;
	}


	/* Animate! */
	ac = (ac + 0x10) % 0x100;

	color_table[1][1] = 0;
	color_table[1][2] = 0xFF - ac;
	color_table[1][3] = 0xFF - ac;
	color_table[9][1] = ac;
	color_table[9][2] = 0;
	color_table[9][3] = 0;
	color_table[29][1] = ac;
	color_table[29][2] = 0;
	color_table[29][3] = 0;


	/* Simple color (nope) */
	if (colors16) {
		c_message_add(format("animating %d (simple)", (ac >> 4) & 0x0F));

		/* Save the default colors */
		for (i = 0; i < 16; i++) {
			/* Simply accept the desired colors */
			win_pal[i] = color_table[i][0] = (ac >> 4) & 0x0F;
		}
	}
	/* Complex color (yes) */
	else {
		//c_message_add(format("animating %d (complex)", ac));

		/* Save the default colors */
#ifndef EXTENDED_COLOURS_PALANIM
		for (i = 0; i < 16; i++) {
#else
 #ifdef PALANIM_SWAP
		for (i = 0; i < 16; i++) {
 #else
		for (i = 0; i < 16 + 16; i++) {
 #endif
#endif
			/* Extract desired values */
			rv = color_table[i][1];
			gv = color_table[i][2];
			bv = color_table[i][3];

			/* Extract a full color code */
			code = PALETTERGB(rv, gv, bv);

			/* Activate changes */
			if (win_clr[i] != code) {
				/* Apply the desired color */
				win_clr[i] = code;
				//c_message_add(format("changed [%d] %d -> %d (%d,%d,%d)", i, win_clr[i], code, rv, gv, bv));
			}
		}

		/* Activate the palette */
		new_palette();
	}

	/* Refresh aka redraw windows with new colour */
	for (i = 0; i < MAX_TERM_DATA; i++) {
		term *old = Term;
		term_data *td = &data[i];

		/* Activate */
		Term_activate(&td->t);

		/* Redraw the contents */
		Term_redraw();

		/* Restore */
		Term_activate(old);
	}
}
void set_palette(byte c, byte r, byte g, byte b) {
	COLORREF code;
	term *term_old = Term;

	if (!c_cfg.palette_animation) return;

#ifdef PALANIM_OPTIMIZED
	/* Check for refresh market at the end of a palette data transmission */
	if (c == 127) {
 #ifdef PALANIM_OPTIMIZED2
		int i;
		/* Batch-apply all colour changes */
  #ifndef EXTENDED_COLOURS_PALANIM
		for (i = 0; i < 16; i++)
  #else
   #ifdef PALANIM_SWAP
		for (i = 0; i < 16; i++)
   #else
		for (i = 0; i < 16 + 16; i++)
   #endif
  #endif
			win_clr[i] = win_clr_buf[i];
 #endif

		/* Activate the palette */
 #ifdef PALANIM_SWAP
		new_palette_ps();
 #else
		new_palette();
 #endif

		/* Refresh aka redraw main window with new colour */
		term_data *td = &data[0];
		/* Activate */
		Term_activate(&td->t);
		/* Redraw the contents */
 #if 0 /* no flickering here when animating colours 0..15, even though set_palette() flickers. Maybe because of colours 16-31 for some reason? */
		Term_redraw();
 #else /* trying this instead, should be identical ie no flickering, as it's the minimal possible version simply */
		Term_xtra(TERM_XTRA_FRESH, 0); /* Flickering occasionally on Windows :( */
 #endif
		/* Restore */
		Term_activate(term_old);
		return;
	}
#else
	if (c == 127) return; //just discard refresh marker
#endif

#ifdef PALANIM_SWAP
	c = (c + 16) % 32;
#endif

	/* Need complex color mode for palette animation */
	if (colors16) {
		c_message_add("Palette animation failed! Disable it in = 3 'palette_animation'!");
		return;
	}

	color_table[c][1] = r;
	color_table[c][2] = g;
	color_table[c][3] = b;
	//c_message_add(format("palette %d -> %d, %d, %d", c, r, g, b));

	/* Extract a full color code */
	code = PALETTERGB(r, g, b);

	/* Activate changes if any */
#ifndef PALANIM_OPTIMIZED2
	if (win_clr[c] == code) return;
#else
	if (win_clr_buf[c] == code) return;
#endif

	/* Apply the desired color */
#ifndef PALANIM_OPTIMIZED2
	win_clr[c] = code;
#else
	win_clr_buf[c] = code;
#endif
	//c_message_add("palette changed");

#ifndef PALANIM_OPTIMIZED
	/* Activate the palette */
 #ifdef PALANIM_SWAP
	new_palette_ps();
 #else
	new_palette();
 #endif

	/* Refresh aka redraw main window with new colour */
	term_data *td = &data[0];
	/* Activate */
	Term_activate(&td->t);
	/* Redraw the contents */
	Term_xtra(TERM_XTRA_FRESH, 0); /* Flickering occasionally on Windows :( */
	/* Restore */
	Term_activate(term_old);
#endif
}
#endif /* _Windows */
