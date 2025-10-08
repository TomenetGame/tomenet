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
 * "/lib/xtra/graphics/", and "lib/xtra/sound/" directories, respectively.
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
 * The user should be able to select the "bitmap" for windows independent
 * from the "font", and should be allowed to select any bitmap file. When
 * selecting a bitmap, that is used in all windows. The usage mappings are
 * loaded from preferences file placed in user directory.
 *
 * XXX XXX XXX
 * The various "warning" messages assume the existence of the "term_main.w"
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

/* For GetSystemDefaultUILanguage() for auto-CS_IME-choice */
#include <winnls.h>

#define MNU_SUPPORT
/* but do we actually create a menu? */
//#define MNU_USE
/* #define USE_GRAPHICS */

/* Method not implemented! Don't use. */
//#define CBM_METHOD_DIB

//#ifdef JP
//#  ifndef USE_LOGFONT
//#    define USE_LOGFONT
//#  endif
//#endif

/* ExtTextOut() won't draw certain glyphs eg codes 16-31 if we don't use glyph indices instead,
   via this method (thanks for bringing this up @ The Scar): */
#define WIN_GLYPH_FIX

/* Uncomment this to be able to use Windows LOGFONT instead of the fonts in lib/xtra/fonts.
   NOTE: Moved to config.h as it must be available to all other client source files too now that it's switchable. */
//#define USE_LOGFONT // Kurzel - Some .FON files considered security vulnerability on Windows? Ew.

#define DEFAULT_FONTNAME "9X15.FON"
#define DEFAULT_TILENAME "16x24sv.bmp"
#ifdef USE_LOGFONT
 #define DEFAULT_LOGFONTNAME "9X15"
#endif

/* Note that there was an issue with optimized drawing, when Term_repaint() and an incoming message happen together,
   like on sunrise/sunset:
   The hdc handle from repainting wasn't closed and then used when the chat+msg window was redrawn due to the incoming
   message, resulting in all messages in it being pasted into the main window instead (with the (usually smaller) font
   grid size of the chat+msg window.
   To fix this, the function Term_xtra_win_fresh() is now public and used in Term_repaint() before returning, to reset
   the OldDC handle. (Another idea might be to have a distinct OldDC[] handle for each term window.) - C. Blue */
/* Unfortunatels there is still a bug: On sunrise/sunset if you type in chat the client could now crash.
   Disabling OPTIMIZE_DRAWING completely for now. */
//#define OPTIMIZE_DRAWING


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

#define IDM_TEXT_TERM_MAIN		201
#define IDM_TEXT_TERM_1			202
#define IDM_TEXT_TERM_2			203
#define IDM_TEXT_TERM_3			204
#define IDM_TEXT_TERM_4			205
#define IDM_TEXT_TERM_5			206
#define IDM_TEXT_TERM_6			207
#define IDM_TEXT_TERM_7			208
#define IDM_TEXT_TERM_8			209
#define IDM_TEXT_TERM_9			210

#define IDM_WINDOWS_TERM_MAIN		211
#define IDM_WINDOWS_TERM_1		212
#define IDM_WINDOWS_TERM_2		213
#define IDM_WINDOWS_TERM_3		214
#define IDM_WINDOWS_TERM_4		215
#define IDM_WINDOWS_TERM_5		216
#define IDM_WINDOWS_TERM_6		217
#define IDM_WINDOWS_TERM_7		218
#define IDM_WINDOWS_TERM_8		219
#define IDM_WINDOWS_TERM_9		220

#define IDM_OPTIONS_GRAPHICS		221
#define IDM_OPTIONS_SOUND		222
#define IDM_OPTIONS_UNUSED		231
#define IDM_OPTIONS_SAVER		232
#define IDM_OPTIONS_SAVEPREF		241

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
#include <windowsx.h>
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
 * For some reason this may be required on Windows:
 * The colours 0-15 can be animated just fine and flicker-free.
 * The colours 16-31 cause flickering on animation.
 * So on Win we just do it the opposite way round and animate 0-15 and
 * use 16-31 for static colours. - C. Blue */
//#define PALANIM_SWAP



void resize_main_window_win(int cols, int rows);


/* Cache for prepared graphical tiles.
   Usage observations:
    Size 256:    Cache fills maybe within first 10 floors of Barrow-Downs (2mask mode), so it's very comfortable.
                 However, it overflows instantly in just 1 sector of housing area around Bree, on admin who can see all objects.
    Size 256*3:  Cache manages to more or less capture a whole housing area sector fine. This seems a good minimum cache size.
    Size 256*4:  Default choice now, for reserves.

   Notes: On Windows (any version, up to at least 10), a 256*4 size would lead to exceeding the default GDI objects limit of 10000
          (as cache always gets initialized for all 10 windows (main window + 9 subterms),
          resulting in NULL pointer getting returned by some CreateBitmap() calls half way through initialization,
          eg within ResizeTilesWithMasks(). To workaround this,
          - either reduce the cache size (eg to 384, compare 'tested values' table further below)
          - or increase the limit at
            for x86 systems only: HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Windows\GDIProcessHandleQuota
            for x64 systems only: HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Windows NT\CurrentVersion\Windows\GDIProcessHandleQuota
            which have a range of 256...65536 (on Windows 2000 only up to 16384)
            The default value is 0x2710 (decimal 10000) and should be increased to something safe depending on the tile cache size.
            You have to reboot for these registry changes to take effect!

            Potential issues with old x86 systems:
            However, on x86 anything above 12,000 might rarely give an effective increase above this due to x86-system resource constraints.
            Also, GDI is allocated out of desktop heap, it's possible on x86 systems for huge allocation sizes (not handle counts)
            to need to increase desktop heap allocation in Session View space from 20MB all the way up to 48MB by modifying a registry key,
            but Session View space is limited on x86 and other regions might get less resources in turn.

          Tested working GDI handle values vs cache sizes (10000 GDI handles means it works out of the box w/o need to change the registry):
            10000: (256 * 1), (384)
            25000: (256 * 2)
*/
/* Alternate method to remedy the Windows-specific GDI object limit w/o modifying the Registry:
   We can use a different cache structure: One large cache bitmap per window, instead of many small bitmaps for each tile.
   The number of cache tiles per line is defined by TILE_CACHE_SINGLEBMP's value [32],
   so the TILE_CACHE_SIZE should always be multiple of this: */
#define TILE_CACHE_SINGLEBMP 32		/* <- Comment out this define if you want to use the default cache method that is limited by GDI handle limit */
#ifdef TILE_CACHE_SINGLEBMP
 #define TILE_CACHE_SIZE (256 * 4)
#else
 #define TILE_CACHE_SIZE (384) /* Without the single-bmp structure, we cannot use a bigger cache w/o modifying the registry as explained above */
 //#define TILE_CACHE_SIZE (256 * 2) /* Only use this (or even bigger values) if you have modified the registry as explained above */
#endif

/* Output cache state information in the message window? Spammy and only for debugging purpose. */
//#define TILE_CACHE_LOG

#if 1 /* Enable extra cache efficiency regarding palette animation? */
 #if 1 /* Choose a cache behaviour aspect regarding palette animation, the 1st is more efficient */
 /* Cache invalidates based on attr when new palette info is received? -- more efficient */
  #define TILE_CACHE_NEWPAL
 #else
 /* Cache uses foreground + background colour palette values too? */
  #define TILE_CACHE_FGBG
 #endif
#endif

#ifdef TILE_CACHE_SIZE
bool disable_tile_cache = FALSE;
struct tile_cache_entry {
 #ifndef TILE_CACHE_SINGLEBMP
    HBITMAP hbmTilePreparation;
 #endif
    char32_t c;
    byte a;
 #ifdef GRAPHICS_BG_MASK
  #ifndef TILE_CACHE_SINGLEBMP
    HBITMAP hbmTilePreparation2;
  #endif
    char32_t c_back;
    byte a_back;
 #endif
    bool is_valid;
 #ifdef TILE_CACHE_FGBG
    s32b fg, bg; /* Optional palette_animation handling */
 #endif
};
#endif


/*
 * Forward declare
 */
typedef struct _term_data term_data;

/*
 * Extra "term" data
 *
 * Note the use of "font_want" for the names of the font files
 * requested by the user, and the use of "font_file" for the
 * currently active font files.
 *
 * The "font_file" is capitalized, and is of the form "8X13.FON",
 * while "font_want" can be in almost any form as long as it could
 * be construed as attempting to represent the name of a font or file.
 */
struct _term_data {
	term     t;
	cptr     s;
	HWND     w;

#ifdef USE_GRAPHICS
	HDC hdcTiles, hdcBgMask, hdcFgMask;
	rawpict_tile tiles_rawpict[MAX_TILES_RAWPICT + 1];
	int rawpict_scale_wid_org, rawpict_scale_hgt_org, rawpict_scale_wid_use, rawpict_scale_hgt_use;
 #ifdef GRAPHICS_BG_MASK
	HDC hdcBg2Mask;
	HDC hdcTilePreparation2;
	HBITMAP hbmTilePreparation2;
 #endif
	HDC hdcTilePreparation;
	HBITMAP hbmTilePreparation;

 #ifdef TILE_CACHE_SIZE
	int cache_position;
	struct tile_cache_entry tile_cache[TILE_CACHE_SIZE];
  #ifdef TILE_CACHE_SINGLEBMP
	HBITMAP hbmCacheTilePreparation;
	HDC hdcCacheTilePreparation;
   #ifdef GRAPHICS_BG_MASK
	HBITMAP hbmCacheTilePreparation2;
	HDC hdcCacheTilePreparation2;
   #endif
  #endif
 #endif
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
	cptr     font_file;
	HFONT    font_id;

	uint     font_wid;
	uint     font_hgt;

#ifdef USE_LOGFONT
	LOGFONT  lf;
#endif
};

#ifdef USE_GRAPHICS
HBITMAP g_hbmTiles = NULL;
HBITMAP g_hbmBgMask = NULL;
HBITMAP g_hbmFgMask = NULL;
 #ifdef GRAPHICS_BG_MASK
HBITMAP g_hbmBg2Mask = NULL;
 #endif

HBITMAP g_hbmTiles_sub[MAX_SUBFONTS] = NULL;
HBITMAP g_hbmBgMask_sub[MAX_SUBFONTS] = NULL;
HBITMAP g_hbmFgMask_sub[MAX_SUBFONTS] = NULL;
 #ifdef GRAPHICS_BG_MASK
HBITMAP g_hbmBg2Mask_sub[MAX_SUBFONTS] = NULL;
 #endif

/* These variables are computed at image load (in 'init_windows'). */
int graphics_tile_wid, graphics_tile_hgt;
int graphics_image_tpr; /* Tiles per row. */



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
bool game_in_progress	= FALSE;  /* game in progress */
bool initialized	= FALSE;  /* note when "open"/"new" become valid */
bool paletted		= FALSE;  /* screen paletted, i.e. 256 colors */
bool colors16		= FALSE;  /* 16 colors screen, don't use RGB() */
bool convert_ini	= FALSE;  /* Convert a pre 4.9.3 .ini file to current version */

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
cptr ini_file = NULL;

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
//#ifndef USE_LOGFONT
static cptr ANGBAND_DIR_XTRA_FONT;
//#endif
#ifdef USE_GRAPHICS
static cptr ANGBAND_DIR_XTRA_GRAPHICS;
#endif
#if defined(USE_SOUND) && !defined(USE_SOUND_2010) /* It's instead defined in snd-sdl.c, different sound system */
static cptr ANGBAND_DIR_XTRA_SOUND;
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

/* These colors are overwritten with the generic, OS-independant client_color_map[] in enable_common_colormap_win()! */
#ifdef PALANIM_OPTIMIZED2
 #ifndef EXTENDED_BG_COLOURS
static COLORREF win_clr_buf[CLIENT_PALETTE_SIZE];
 #else
static COLORREF win_clr_buf_bg[CLIENT_PALETTE_SIZE + TERMX_AMT];
static COLORREF win_clr_buf[CLIENT_PALETTE_SIZE + TERMX_AMT];
 #endif
#endif
#ifndef EXTENDED_BG_COLOURS
static COLORREF win_clr[CLIENT_PALETTE_SIZE] = {
#else
static COLORREF win_clr_bg[CLIENT_PALETTE_SIZE + TERMX_AMT];
static COLORREF win_clr[CLIENT_PALETTE_SIZE + TERMX_AMT] = {
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
	PALETTERGB(0xCD, 0xCD, 0xCD),  /* 3 3 3  Lt. Slate */
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
	PALETTERGB(0xCD, 0xCD, 0xCD),  /* 3 3 3  Lt. Slate */
	PALETTERGB(0xAF, 0x00, 0xFF),  /* 4 0 4  Violet (was 2,0,2) */
	PALETTERGB(0xFF, 0xFF, 0x00),  /* 4 4 0  Yellow */
	PALETTERGB(0xFF, 0x30, 0x30),  /* 4 0 0  Lt. Red (was 0xFF,0,0) - see 'Red' - C. Blue */
	PALETTERGB(0x00, 0xFF, 0x00),  /* 0 4 0  Lt. Green */
	PALETTERGB(0x00, 0xFF, 0xFF),  /* 0 4 4  Lt. Blue */
	PALETTERGB(0xC7, 0x9D, 0x55)   /* 3 2 1  Lt. Umber */
#endif
};

/*
 * Palette indices for 16 colors
 *
 * See "main-ibm.c" for original table information
 *
 * The entries below are taken from the "color bits" defined above.
 *
 * Note that many of the choices below suck, but so do crappy monitors.
 */
static BYTE win_pal[BASE_PALETTE_SIZE] = {
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



/* Copied and repurposed from: http://winprog.org/tutorial/transparency.html
   NOTE: This function actually blackens (or whitens, if 'inverse') any recognized mask pixel in the _original_ image!
         So while the mask is created, it is sort of 'cut out' of the original image, leaving blackness (whiteness) behind. */
//todo: implement -> for '-1' values for the foreground colour, change crTransparent from a COLORREF value to actual R,G,B, so "-1" can be processed?
HBITMAP CreateBitmapMask(HBITMAP hbmColour, COLORREF crTransparent, int *error) {
	BITMAP bm;
	GetObject(hbmColour, sizeof(BITMAP), &bm);

 #if 1 /* For top speed, operate directly on the bitmap data in memory via pointers */
	const unsigned char MaskColor0 = 0, MaskColor1 = 255;
	int scanline, pixel;

	unsigned char *data, *data2;
	unsigned char *r, *g, *b, *r2, *g2, *b2;
	unsigned char rt = GetRValue(crTransparent), gt = GetGValue(crTransparent), bt = GetBValue(crTransparent); /* Translate everything back xD */
 #ifdef GRAPHICS_SHADED_ALPHA
	/* A bit messy in terms of redundancy: We only need these values here if crTransparent is actually the GFXMASK_FG_* colour... */
	bool is_not_GFXMASK_FG = (crTransparent != RGB(GFXMASK_FG_R, GFXMASK_FG_G, GFXMASK_FG_B));
	unsigned char *a2; /* Mask gains shades via alpha channel */
	unsigned char rt1 = GFXMASK_FG_R1, gt1 = GFXMASK_FG_G1, bt1 = GFXMASK_FG_B1;
	unsigned char rt2 = GFXMASK_FG_R2, gt2 = GFXMASK_FG_G2, bt2 = GFXMASK_FG_B2;
	unsigned char rt3 = GFXMASK_FG_R3, gt3 = GFXMASK_FG_G3, bt3 = GFXMASK_FG_B3;
 #endif
	/* Note that while hbmColour is actually in 32bbm memory, bytesPerPixel is actually 24bbp usually! (PixelFormat.Format24bppRgb/Format32bppArgb): */
	int bytesPerLine = bm.bmWidthBytes, bytesPerPixel = bytesPerLine / bm.bmWidth;

	/* Debug info */
	logprint(format("(bmWidthBytes = %ld, bmBitsPixel = %d, bmWidth = %ld, bmHeight = %ld)\n", bm.bmWidthBytes, bm.bmBitsPixel, bm.bmWidth, bm.bmHeight));

	/* Get bitmap pixel data in memory */
  #ifdef CBM_METHOD_DIB /* --NOT IMPLEMENTED-- Use this with LR_CREATEDIBSECTION flag in LoadImageA */
	data = (unsigned char*)bm.bmBits;
	data2 = (unsigned char*)bm2.bmBits;
	/* Debug info */
	logprint(format("(data = %ld, data2 = %ld)\n", (long)data, (long)data2));
  #else /* Otherwise, request the bitmap data now, via GetBitmapBits() or GetDIBits() */
	/* Create 32bpp mask bitmap. */
	HBITMAP hbmMask = CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 32, NULL);

	if (!hbmMask) {
		logprint(format("Graphics error: CreateBitmap() returned null.\n"));
		*error = 7;
		return(NULL);
	}

	if (!(data = malloc(bm.bmWidthBytes * bm.bmHeight))) {
		logprint(format("Graphics error: malloc() returned null (Image).\n"));
		*error = 5;
		return(NULL);
	}
	if (!GetBitmapBits(hbmColour, bm.bmWidthBytes * bm.bmHeight, data)) {
		logprint(format("Graphics error: GetBitmapBits() returned zero (Image).\n"));
		*error = 1;
		return(NULL);
	}

	if (!(data2 = malloc(bm.bmWidthBytes * bm.bmHeight))) {
		logprint(format("Graphics error: malloc() returned null (Mask).\n"));
		*error = 6;
		return(NULL);
	}
	if (!GetBitmapBits(hbmMask, bm.bmWidthBytes * bm.bmHeight, data2)) {
		logprint(format("Graphics error: GetBitmapBits() returned zero (Mask).\n"));
		*error = 2;
		return(NULL);
	}
  #endif

	/* Hack to transpose bg2 map into bg map when not in 2mask-mode, for compatibility wih 2mask-tilesets */
	if (use_graphics != UG_2MASK && crTransparent == RGB(GFXMASK_BG2_R, GFXMASK_BG2_G, GFXMASK_BG2_B)) {
		for (int y = 0; y < bm.bmHeight; y++) {
			scanline = y * bytesPerLine;
			for (int x = 0; x < bm.bmWidth; x++) {
				pixel = scanline + x * bytesPerPixel;
				b = &data[pixel];
				g = b + 1;
				r = g + 1;
				b2 = &data2[pixel];
				g2 = b2 + 1;
				r2 = g2 + 1;

				if (*r == rt && *g == gt && *b == bt) {
					/* Set mask pixel */
					//*b2 = *g2 = *r2 = MaskColor1;  -- we're currently creating the bg2-mask, but it's not needed as we'e in non-2mask-mode
					/* Transpose origin pixel from bg2 to bg, so the unused bg2 mask becomes part of the usable bg mask instead, which should look better */
					*b = GFXMASK_BG_B;
					*g = GFXMASK_BG_G;
					*r = GFXMASK_BG_R;
				}
			}
		}
	}
	/* Normal operation */
	else {
		for (int y = 0; y < bm.bmHeight; y++) {
			scanline = y * bytesPerLine;
			for (int x = 0; x < bm.bmWidth; x++) {
				pixel = scanline + x * bytesPerPixel;
				b = &data[pixel];
				g = b + 1;
				r = g + 1;
				b2 = &data2[pixel];
				g2 = b2 + 1;
				r2 = g2 + 1;

				if (*r == rt && *g == gt && *b == bt) {
					/* Set mask pixel */
					*b2 = *g2 = *r2 = MaskColor1;
  #ifdef GRAPHICS_SHADED_ALPHA
					/* Set alpha value aka shading strength -> normal, ie fully opaque */
					*a2 = 255;
  #endif
					/* Erase origin pixel */
					*b = *g = *r = MaskColor0;
				}
  #ifdef GRAPHICS_SHADED_ALPHA
				if (!is_not_GFXMASK_FG) continue;
				/* Translate shaded RGB-values of the foreground mask to alpha-channel info */
				if (*r == rt1 && *g == gt1 && *b == bt1) {
					/* Set mask pixel */
					*b2 = *g2 = *r2 = MaskColor1;
					/* Set alpha value aka shading strength */
					*a2 = 151;
					/* Erase origin pixel */
					*b = *g = *r = MaskColor0;
				}
				if (*r == rt2 && *g == gt2 && *b == bt2) {
					/* Set mask pixel */
					*b2 = *g2 = *r2 = MaskColor1;
					/* Set alpha value aka shading strength */
					*a2 = 79;
					/* Erase origin pixel */
					*b = *g = *r = MaskColor0;
				}
				if (*r == rt3 && *g == gt3 && *b == bt3) {
					/* Set mask pixel */
					*b2 = *g2 = *r2 = MaskColor1;
					/* Set alpha value aka shading strength */
					*a2 = 29;
					/* Erase origin pixel */
					*b = *g = *r = MaskColor0;
				}
  #endif
			}
		}
	}

  #ifndef CBM_METHOD_DIB
	/* Write data back via SetBitmapBits() or SetDIBits() */
	if (!SetBitmapBits(hbmColour, bm.bmWidthBytes * bm.bmHeight, data)) {
		logprint(format("Graphics error: SetBitmapBits() returned zero (Image).\n"));
		*error = 3;
		return(NULL);
	}
	free(data);
	if (!SetBitmapBits(hbmMask, bm.bmWidthBytes * bm.bmHeight, data2)) {
		logprint(format("Graphics error: SetBitmapBits() returned zero (Mask).\n"));
		*error = 4;
		return(NULL);
	}
	free(data2);
  #endif
 #else /* GetPixel()/SetPixel() is WAY too slow on some Windows systems: Client startup time is a minute+, while it is immediate on WINE on an ancient, slow laptop. Microsoft please. */
 /* --- NOTE: GRAPHICS_SHADED_ALPHA is not implemented here! --- */
	const COLORREF MaskColor0 = RGB(0, 0, 0), MaskColor1 = RGB(255, 255, 255);

	/* Create 32bpp mask bitmap. */
	HBITMAP hbmMask = CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 32, NULL);

	/* Get some HDCs that are compatible with the display driver of main window. */
	HDC hdcMem = CreateCompatibleDC(NULL);
	HDC hdcMem2 = CreateCompatibleDC(NULL);

	/* Select the image and mask bitmap handles to created memory DC and store default images. */
	HBITMAP hbmOldMem = SelectBitmap(hdcMem, hbmColour);
	HBITMAP hbmOldMem2 = SelectBitmap(hdcMem2, hbmMask);

	/* Set the background colour of the colour image to the colour you want to be transparent. */
	//COLORREF clrSaveBk = SetBkColor(hdcMem, crTransparent);

	/* Copy the bits from the colour image to the B+W mask. Everything with the background colour ends up white while everythig else ends up black. */
	//BitBlt(hdcMem2, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);

	/* Take our new mask and use it to turn the transparent colour in our original colour image to black so the transparency effect will work right. */
	//BitBlt(hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem2, 0, 0, SRCINVERT);

	/* Clean up. */
	//SetBkColor(hdcMem, clrSaveBk);

	/* The commented code above, which is in all tutorials on net doesn't work for mingw for unknown reasons. Let's use more classical and slower approach. */

	for (int y = 0; y < bm.bmHeight; y++) {
		for (int x = 0; x < bm.bmWidth; x++) {
			COLORREF p = GetPixel(hdcMem, x, y);

			if (p == crTransparent) {
				/* Set mask pixel. */
				SetPixel(hdcMem2, x, y, MaskColor1);
				/* Erase origin pixel */
				SetPixel(hdcMem, x, y, MaskColor0);
			}
		}
	}

	/* Clean up. */
	SelectBitmap(hdcMem, hbmOldMem);
	SelectBitmap(hdcMem2, hbmOldMem2);
	DeleteDC(hdcMem);
	DeleteDC(hdcMem2);
 #endif

	*error = 0;
	return(hbmMask);
}

/* Resize an bitmap with it's bg/fg masks.
 *
 * It's your responsibility to free returned HBITMAP, hbmBgMask_return and hbmFgMask_return after usage.
 * Function will not free resources if already allocated in hbmBgMask_return or hbmFgMask_return input variable.
 */
 #ifdef GRAPHICS_BG_MASK
static HBITMAP ResizeTilesWithMasks(HBITMAP hbm, int ix, int iy, int ox, int oy, HBITMAP hbmBgMask, HBITMAP hbmFgMask, HBITMAP hbmBg2Mask, HBITMAP *hbmBgMask_return, HBITMAP *hbmFgMask_return, HBITMAP *hbmBg2Mask_return, term_data *td) {
 #else
static HBITMAP ResizeTilesWithMasks(HBITMAP hbm, int ix, int iy, int ox, int oy, HBITMAP hbmBgMask, HBITMAP hbmFgMask, HBITMAP *hbmBgMask_return, HBITMAP *hbmFgMask_return, term_data *td) {
 #endif
	BITMAP bm;
	GetObject(hbm, sizeof(BITMAP), &bm);

	int width1, height1, width2, height2;
	width1 = bm.bmWidth;
	height1 = bm.bmHeight;

	width2 = ox * width1 / ix;
	height2 = oy * height1 / iy;

	HBITMAP hbmResTiles = CreateBitmap(width2, height2, 1, 32, NULL);
	HBITMAP hbmResBgMask = CreateBitmap(width2, height2, 1, 32, NULL);
	HBITMAP hbmResFgMask = CreateBitmap(width2, height2, 1, 32, NULL);

	HDC hdcMemTiles = CreateCompatibleDC(NULL);
	HDC hdcMemBgMask = CreateCompatibleDC(NULL);
	HDC hdcMemFgMask = CreateCompatibleDC(NULL);

	HDC hdcMemResTiles = CreateCompatibleDC(NULL);
	HDC hdcMemResBgMask = CreateCompatibleDC(NULL);
	HDC hdcMemResFgMask = CreateCompatibleDC(NULL);

 #ifdef GRAPHICS_BG_MASK
	HBITMAP hbmResBg2Mask = CreateBitmap(width2, height2, 1, 32, NULL);
	HDC hdcMemBg2Mask = CreateCompatibleDC(NULL);
	HDC hdcMemResBg2Mask = CreateCompatibleDC(NULL);
 #endif

	/* Select our tiles into the memory DC and store default bitmap to not leak GDI objects. */
	HBITMAP hbmOldMemTiles = SelectObject(hdcMemTiles, g_hbmTiles);
	HBITMAP hbmOldMemBgMask = SelectObject(hdcMemBgMask, g_hbmBgMask);
	HBITMAP hbmOldMemFgMask = SelectObject(hdcMemFgMask, g_hbmFgMask);

	HBITMAP hbmOldMemResTiles = SelectObject(hdcMemResTiles, hbmResTiles);
	HBITMAP hbmOldMemResBgMask = SelectObject(hdcMemResBgMask, hbmResBgMask);
	HBITMAP hbmOldMemResFgMask = SelectObject(hdcMemResFgMask, hbmResFgMask);

 #ifdef GRAPHICS_BG_MASK
	HBITMAP hbmOldMemBg2Mask = SelectObject(hdcMemBg2Mask, g_hbmBg2Mask);
	HBITMAP hbmOldMemResBg2Mask = SelectObject(hdcMemResBg2Mask, hbmResBg2Mask);
 #endif

 #if 1 /* use StretchBlt instead of SetPixel-loops */
	/* StretchBlt is much faster on native Windows - mikaelh */
	SetStretchBltMode(hdcMemResTiles, COLORONCOLOR);
	SetStretchBltMode(hdcMemResBgMask, COLORONCOLOR);
	SetStretchBltMode(hdcMemResFgMask, COLORONCOLOR);
	StretchBlt(hdcMemResTiles, 0, 0, width2, height2, hdcMemTiles, 0, 0, width1, height1, SRCCOPY);
	StretchBlt(hdcMemResBgMask, 0, 0, width2, height2, hdcMemBgMask, 0, 0, width1, height1, SRCCOPY);
	StretchBlt(hdcMemResFgMask, 0, 0, width2, height2, hdcMemFgMask, 0, 0, width1, height1, SRCCOPY);
  #ifdef GRAPHICS_BG_MASK
	SetStretchBltMode(hdcMemResBg2Mask, COLORONCOLOR);
	StretchBlt(hdcMemResBg2Mask, 0, 0, width2, height2, hdcMemBg2Mask, 0, 0, width1, height1, SRCCOPY);
  #endif
 #else

	/* I like more this classical and slower approach, as the commented StretchBlt code above. In my opinion, it makes nicer resized pictures.*/
	int x1, x2, y1, y2, Tx, Ty;
	int *px1, *px2, *dx1, *dx2;
	int *py1, *py2, *dy1, *dy2;


	if (ix >= ox) {
		px1 = &x1;
		px2 = &x2;
		dx1 = &ix;
		dx2 = &ox;
	} else {
		px1 = &x2;
		px2 = &x1;
		dx1 = &ox;
		dx2 = &ix;
	}

	if (iy >= oy) {
		py1 = &y1;
		py2 = &y2;
		dy1 = &iy;
		dy2 = &oy;
	} else {
		py1 = &y2;
		py2 = &y1;
		dy1 = &oy;
		dy2 = &iy;
	}

	Ty = *dy1;

	for (y1 = 0, y2 = 0; (y1 < height1) && (y2 < height2); ) {
		Tx = *dx1;

		for (x1 = 0, x2 = 0; (x1 < width1) && (x2 < width2); ) {

			SetPixel(hdcMemResTiles, x2, y2, GetPixel(hdcMemTiles, x1, y1));
			SetPixel(hdcMemResBgMask, x2, y2, GetPixel(hdcMemBgMask, x1, y1));
			SetPixel(hdcMemResFgMask, x2, y2, GetPixel(hdcMemFgMask, x1, y1));
  #ifdef GRAPHICS_BG_MASK
			SetPixel(hdcMemResBg2Mask, x2, y2, GetPixel(hdcMemBg2Mask, x1, y1));
  #endif

			(*px1)++;

			Tx -= *dx2;
			if (Tx <= 0) {
				Tx += *dx1;
				(*px2)++;
			}
		}

		(*py1)++;

		Ty -= *dy2;
		if (Ty <= 0) {
			Ty += *dy1;
			(*py2)++;
		}
	}
 #endif

	/* Restore the stored default bitmap into the memory DC. */
	SelectObject(hdcMemTiles, hbmOldMemTiles);
	SelectObject(hdcMemBgMask, hbmOldMemBgMask);
	SelectObject(hdcMemFgMask, hbmOldMemFgMask);

	SelectObject(hdcMemResTiles, hbmOldMemResTiles);
	SelectObject(hdcMemResBgMask, hbmOldMemResBgMask);
	SelectObject(hdcMemResFgMask, hbmOldMemResFgMask);

 #ifdef GRAPHICS_BG_MASK
	SelectObject(hdcMemBg2Mask, hbmOldMemBg2Mask);
	SelectObject(hdcMemResBg2Mask, hbmOldMemResBg2Mask);
 #endif

	/* Release the created memory DC. */
	DeleteDC(hdcMemTiles);
	DeleteDC(hdcMemBgMask);
	DeleteDC(hdcMemFgMask);
 #ifdef GRAPHICS_BG_MASK
	DeleteDC(hdcMemBg2Mask);
 #endif

	DeleteDC(hdcMemResTiles);
	DeleteDC(hdcMemResBgMask);
	DeleteDC(hdcMemResFgMask);
 #ifdef GRAPHICS_BG_MASK
	DeleteDC(hdcMemResBg2Mask);
 #endif

	(*hbmBgMask_return) = hbmResBgMask;
	(*hbmFgMask_return) = hbmResFgMask;
 #ifdef GRAPHICS_BG_MASK
	(*hbmBg2Mask_return) = hbmResBg2Mask;
 #endif

	/* Also rescale rawpict image dimensions/coordinates according to new dimensions */
	int i;

	td->rawpict_scale_wid_org = width1;
	td->rawpict_scale_hgt_org = height1;
	td->rawpict_scale_wid_use = width2;
	td->rawpict_scale_hgt_use = height2;

	for (i = 1; i <= MAX_TILES_RAWPICT; i++) {
		if (!tiles_rawpict_org[i].defined) continue;
		if (!width1 || !height1) continue;
		td->tiles_rawpict[i].defined = TRUE;
		td->tiles_rawpict[i].x = (tiles_rawpict_org[i].x * width2) / width1;
		td->tiles_rawpict[i].w = (tiles_rawpict_org[i].w * width2) / width1;
		td->tiles_rawpict[i].y = (tiles_rawpict_org[i].y * height2) / height1;
		td->tiles_rawpict[i].h = (tiles_rawpict_org[i].h * height2) / height1;
	}

	return(hbmResTiles);
}

/* Rescale rawpict image dimensions/coordinates according to new dimensions */
void tiles_rawpict_scale(void) {
	int t, i, width1, width2, height1, height2;
	term_data *td;

	for (t = 0; t < ANGBAND_TERM_MAX; t++) {
		td = &data[t];

		width1 = td->rawpict_scale_wid_org;
		height1 = td->rawpict_scale_hgt_org;
		width2 = td->rawpict_scale_wid_use;
		height2 = td->rawpict_scale_hgt_use;

		for (i = 1; i <= MAX_TILES_RAWPICT; i++) {
			if (!tiles_rawpict_org[i].defined) continue;
			if (!width1 || !height1) continue;
			td->tiles_rawpict[i].x = (tiles_rawpict_org[i].x * width2) / width1;
			td->tiles_rawpict[i].w = (tiles_rawpict_org[i].w * width2) / width1;
			td->tiles_rawpict[i].y = (tiles_rawpict_org[i].y * height2) / height1;
			td->tiles_rawpict[i].h = (tiles_rawpict_org[i].h * height2) / height1;
		}
	}
}

static void releaseCreatedGraphicsObjects(term_data *td) {
	if (td == NULL) {
		logprint("(debug) releaseCreatedGraphicsObjects : NULL\n");
		return;
	}

	if (td->hdcTilePreparation != NULL) {
		//associated bitmap gets auto-deleted?
		DeleteDC(td->hdcTilePreparation);
		td->hdcTilePreparation = NULL;
	}
	if (td->hdcTiles != NULL) {
		//associated bitmap gets auto-deleted?
		DeleteDC(td->hdcTiles);
		td->hdcTiles = NULL;
	}
	if (td->hdcBgMask != NULL) {
		//associated bitmap gets auto-deleted?
		DeleteDC(td->hdcBgMask);
		td->hdcBgMask = NULL;
	}
	if (td->hdcFgMask != NULL) {
		//associated bitmap gets auto-deleted?
		DeleteDC(td->hdcFgMask);
		td->hdcFgMask = NULL;
	}
 #ifdef GRAPHICS_BG_MASK
	if (td->hdcTilePreparation2 != NULL) {
		//associated bitmap gets auto-deleted?
		DeleteDC(td->hdcTilePreparation2);
		td->hdcTilePreparation2 = NULL;
	}
	if (td->hdcBg2Mask != NULL) {
		//associated bitmap gets auto-deleted?
		DeleteDC(td->hdcBg2Mask);
		td->hdcBg2Mask = NULL;
	}
 #endif

 #ifdef TILE_CACHE_SIZE
	if (!disable_tile_cache) {
  #ifdef TILE_CACHE_SINGLEBMP
		if (td->hbmCacheTilePreparation != NULL) {
			DeleteBitmap(td->hbmCacheTilePreparation);
			DeleteDC(td->hdcCacheTilePreparation);
			td->hdcCacheTilePreparation = NULL;
		}
		if (td->hbmCacheTilePreparation2 != NULL) {
			DeleteBitmap(td->hbmCacheTilePreparation2);
			DeleteDC(td->hdcCacheTilePreparation2);
			td->hdcCacheTilePreparation2 = NULL;
		}
  #endif
		for (int i = 0; i < TILE_CACHE_SIZE; i++) {
  #ifndef TILE_CACHE_SINGLEBMP
			if (td->tile_cache[i].hbmTilePreparation != NULL) {
				DeleteBitmap(td->tile_cache[i].hbmTilePreparation);
				td->tile_cache[i].hbmTilePreparation = NULL;
				td->tile_cache[i].c = 0xffffffff;
				td->tile_cache[i].a = 0xff;
			}
   #ifdef GRAPHICS_BG_MASK
			if (td->tile_cache[i].hbmTilePreparation2 != NULL) {
				DeleteBitmap(td->tile_cache[i].hbmTilePreparation2);
				td->tile_cache[i].hbmTilePreparation2 = NULL;
				td->tile_cache[i].c_back = 0xffffffff;
				td->tile_cache[i].a_back = 0xff;
			}
   #endif
  #else
			td->tile_cache[i].c = 0xffffffff;
			td->tile_cache[i].a = 0xff;
   #ifdef GRAPHICS_BG_MASK
			td->tile_cache[i].c_back = 0xffffffff;
			td->tile_cache[i].a_back = 0xff;
   #endif
  #endif
			td->tile_cache[i].is_valid = FALSE;
			/* Optional 'bg' and 'fg' need no intialization */
		}
	}
 #endif
}

/* Called on term_data_link() (initial term creation+initialization) and on term_font_force() (any font change): */
/* This can cause a crash due to too many leaked GDI objects if TILE_CACHE_SIZE is defined. Fixed now, see comments.
   However, this beckons the question whether the other gfx tileset code requires something similar perhaps. - C. Blue */
static void recreateGraphicsObjects(term_data *td) {
	int fwid, fhgt;

 #ifdef USE_LOGFONT
	if (use_logfont) {
		fwid = td->lf.lfWidth;
		fhgt = td->lf.lfHeight;
	} else
 #endif
	{
		fwid = td->font_wid;
		fhgt = td->font_hgt;
	}

	releaseCreatedGraphicsObjects(td);

	HBITMAP hbmTilePreparation = CreateBitmap(2 * fwid, fhgt, 1, 32, NULL);
	HBITMAP hbmBgMask, hbmFgMask;
 #ifdef GRAPHICS_BG_MASK
	HBITMAP hbmTilePreparation2 = CreateBitmap(2 * fwid, fhgt, 1, 32, NULL);
	HBITMAP hbmBg2Mask;
	HBITMAP hbmTiles = ResizeTilesWithMasks(g_hbmTiles, graphics_tile_wid, graphics_tile_hgt, fwid, fhgt, g_hbmBgMask, g_hbmFgMask, g_hbmBg2Mask, &hbmBgMask, &hbmFgMask, &hbmBg2Mask, td);
 #else
	HBITMAP hbmTiles = ResizeTilesWithMasks(g_hbmTiles, graphics_tile_wid, graphics_tile_hgt, fwid, fhgt, g_hbmBgMask, g_hbmFgMask, &hbmBgMask, &hbmFgMask, td);
 #endif

	if (hbmTiles == NULL || hbmBgMask == NULL || hbmFgMask == NULL || hbmTilePreparation == NULL
 #ifdef GRAPHICS_BG_MASK
	    || hbmBg2Mask == NULL || hbmTilePreparation2 == NULL
 #endif
	    ) {
		logprint(format("(debug) fwid %d, fhgt %d; ht %d, hbg %d, hfm %d, htp %d\n", fwid, fhgt, hbmTiles, hbmBgMask, hbmFgMask, hbmTilePreparation));
 #ifdef GRAPHICS_BG_MASK
		logprint(format("(debug) hbg2 %d, htp2 %d.\n", hbmBg2Mask, hbmTilePreparation2));
 #endif
		quit("Resizing tiles or masks failed.\n");
	}

	/* Get device content for current window. */
	HDC hdc = GetDC(td->w);

	/* Create a compatible device content in memory. */
	td->hdcTilePreparation = CreateCompatibleDC(hdc);
	td->hdcTiles = CreateCompatibleDC(hdc);
	td->hdcBgMask = CreateCompatibleDC(hdc);
	td->hdcFgMask = CreateCompatibleDC(hdc);
 #ifdef GRAPHICS_BG_MASK
	td->hdcTilePreparation2 = CreateCompatibleDC(hdc);
	td->hdcBg2Mask = CreateCompatibleDC(hdc);
 #endif

	/* Select our tiles into the memory DC and store default bitmap to not leak GDI objects. */
	HBITMAP hbmOldTilePreparation = SelectObject(td->hdcTilePreparation, hbmTilePreparation);
	HBITMAP hbmOldTiles = SelectObject(td->hdcTiles, hbmTiles);
	HBITMAP hbmOldBgMask = SelectObject(td->hdcBgMask, hbmBgMask);
	HBITMAP hbmOldFgMask = SelectObject(td->hdcFgMask, hbmFgMask);
 #ifdef GRAPHICS_BG_MASK
	HBITMAP hbmOldTilePreparation2 = SelectObject(td->hdcTilePreparation2, hbmTilePreparation2);
	HBITMAP hbmOldBg2Mask = SelectObject(td->hdcBg2Mask, hbmBg2Mask);
 #endif

	/* Delete the default HBITMAPs here, the above created tiles & masks should be deleted when the HDCs are released. */
	DeleteBitmap(hbmOldTilePreparation);
	DeleteBitmap(hbmOldTiles);
	DeleteBitmap(hbmOldBgMask);
	DeleteBitmap(hbmOldFgMask);
 #ifdef GRAPHICS_BG_MASK
	DeleteBitmap(hbmOldTilePreparation2);
	DeleteBitmap(hbmOldBg2Mask);
 #endif

 #ifdef TILE_CACHE_SIZE
	if (!disable_tile_cache) {
  #ifdef TILE_CACHE_SINGLEBMP
		td->hbmCacheTilePreparation = CreateBitmap(TILE_CACHE_SINGLEBMP * 2 * fwid, (TILE_CACHE_SIZE / TILE_CACHE_SINGLEBMP) * fhgt, 1, 32, NULL);
		td->hdcCacheTilePreparation = CreateCompatibleDC(hdc);
		HBITMAP hbmOldCacheTilePreparation = SelectObject(td->hdcCacheTilePreparation, td->hbmCacheTilePreparation);
		DeleteBitmap(hbmOldCacheTilePreparation);
   #ifdef GRAPHICS_BG_MASK
		td->hbmCacheTilePreparation2 = CreateBitmap(TILE_CACHE_SINGLEBMP * 2 * fwid, (TILE_CACHE_SIZE / TILE_CACHE_SINGLEBMP) * fhgt, 1, 32, NULL);
		td->hdcCacheTilePreparation2 = CreateCompatibleDC(hdc);
		HBITMAP hbmOldCacheTilePreparation2 = SelectObject(td->hdcCacheTilePreparation2, td->hbmCacheTilePreparation2);
		DeleteBitmap(hbmOldCacheTilePreparation2);
   #endif
  #else
		for (int i = 0; i < TILE_CACHE_SIZE; i++) {
			//td->tiles->depth=32
			td->tile_cache[i].hbmTilePreparation = CreateBitmap(2 * fwid, fhgt, 1, 32, NULL);
   #ifdef GRAPHICS_BG_MASK
			//td->tiles->depth=32
			td->tile_cache[i].hbmTilePreparation2 = CreateBitmap(2 * fwid, fhgt, 1, 32, NULL);
   #endif
		}
  #endif
	}
 #endif

	/* Release */
	ReleaseDC(td->w, hdc);

	if (td->hdcTiles == NULL || td->hdcBgMask == NULL || td->hdcFgMask == NULL || td->hdcTilePreparation == NULL
 #ifdef GRAPHICS_BG_MASK
	    || td->hdcBg2Mask == NULL || td->hdcTilePreparation2 == NULL
 #endif
	    )
		quit("Creating device content handles for tiles, tile preparation or masks failed.\n");
}
#endif /* USE_GRAPHICS */

void enable_readability_blue_win(void) {
	client_color_map[6] = 0x0033ff;
#ifdef EXTENDED_COLOURS_PALANIM
	client_color_map[BASE_PALETTE_SIZE + 6] = 0x0033ff;
#endif
}
static void enable_common_colormap_win(void) {
	int i;
#ifdef EXTENDED_BG_COLOURS
	int j;

	#define REDX(i)   (client_ext_color_map[i][0] >> 16 & 0xFF)
	#define GREENX(i) (client_ext_color_map[i][0] >> 8 & 0xFF)
	#define BLUEX(i)  (client_ext_color_map[i][0] & 0xFF)
	#define REDXBG(i)   (client_ext_color_map[i][1] >> 16 & 0xFF)
	#define GREENXBG(i) (client_ext_color_map[i][1] >> 8 & 0xFF)
	#define BLUEXBG(i)  (client_ext_color_map[i][1] & 0xFF)
#endif

	#define RED(i)   (client_color_map[i] >> 16 & 0xFF)
	#define GREEN(i) (client_color_map[i] >> 8 & 0xFF)
	#define BLUE(i)  (client_color_map[i] & 0xFF)

	for (i = 0; i < CLIENT_PALETTE_SIZE; i++) {
		win_clr[i] = PALETTERGB(RED(i), GREEN(i), BLUE(i));
#ifdef PALANIM_OPTIMIZED2
		win_clr_buf[i] = win_clr[i];
#endif
#ifdef EXTENDED_BG_COLOURS
		/* The standard colours (0-15) all have black background traditionally */
		win_clr_bg[i] = PALETTERGB(RED(0), GREEN(0), BLUE(0));
 #ifdef PALANIM_OPTIMIZED2
		win_clr_buf_bg[i] = win_clr_bg[i];
 #endif
#endif
	}

#ifdef EXTENDED_BG_COLOURS
	for (j = i; j < i + TERMX_AMT; j++) {
		win_clr[j] = PALETTERGB(REDX(j - i), GREENX(j - i), BLUEX(j - i));
		win_clr_bg[j] = PALETTERGB(REDXBG(j - i), GREENXBG(j - i), BLUEXBG(j - i));
 #ifdef PALANIM_OPTIMIZED2
		win_clr_buf[j] = win_clr[j];
		win_clr_buf_bg[j] = win_clr_bg[j];
 #endif
	}
#endif
}





/*
 * Hack -- given a pathname, point at the filename
 */
//#ifndef USE_LOGFONT
static cptr extract_file_name(cptr s) {
	cptr p;

	/* Start at the end */
	p = s + strlen(s) - 1;

	/* Back up to divider */
	while ((p >= s) && (*p != ':') && (*p != '\\')) p--;

	/* Return file name */
	return(p + 1);
}
//#endif


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
	if (attrib == INVALID_FILE_NAME) return(FALSE);

	/* Prohibit directory */
	if (attrib & FILE_ATTRIBUTE_DIRECTORY) return(FALSE);

#else /* WIN32 */

	/* Examine and verify */
	if (_dos_getfileattr(path, &attrib)) return(FALSE);

	/* Prohibit something */
	if (attrib & FA_LABEL) return(FALSE);

	/* Prohibit directory */
	if (attrib & FA_DIREC) return(FALSE);

#endif /* WIN32 */

	/* Success */
	return(TRUE);
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
	if (i && (path[i - 1] == '\\')) path[--i] = '\0';

#ifdef WIN32

	/* Examine */
	attrib = GetFileAttributes(path);

	/* Require valid filename */
	if (attrib == INVALID_FILE_NAME) return(FALSE);

	/* Require directory */
	if (!(attrib & FILE_ATTRIBUTE_DIRECTORY)) return(FALSE);

#else /* WIN32 */

	/* Examine and verify */
	if (_dos_getfileattr(path, &attrib)) return(FALSE);

	/* Prohibit something */
	if (attrib & FA_LABEL) return(FALSE);

	/* Require directory */
	if (!(attrib & FA_DIREC)) return(FALSE);

#endif /* WIN32 */

	/* Success */
	return(TRUE);
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
	int fwid, fhgt;

#ifdef USE_LOGFONT
	if (use_logfont) {
		fwid = td->lf.lfWidth;
		fhgt = td->lf.lfHeight;
	} else
#endif
	{
		fwid = td->font_wid;
		fhgt = td->font_hgt;
	}

	/* Paranoia */
	if (td->cols < 1) td->cols = 1;
	if (td->rows < 1) td->rows = 1;

	/* Paranoia */
	if (td->cols > (MAX_WINDOW_WID)) td->cols = (MAX_WINDOW_WID);
	if (td->rows > (MAX_WINDOW_HGT)) td->rows = (MAX_WINDOW_HGT);

	/* Window sizes */
	td->client_wid = td->cols * fwid + td->size_ow1 + td->size_ow2;
	td->client_hgt = td->rows * fhgt + td->size_oh1 + td->size_oh2;

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
static void save_prefs_aux(int term_idx, cptr sec_name) {
	term_data *td = &data[term_idx];
	char buf[32];

	wsprintf(buf, "%d", term_idx);
	WritePrivateProfileString(sec_name, "WindowNumber", buf, ini_file);

	if (td != &data[0]) {
		/* Visible (Sub-windows) */
		strcpy(buf, td->visible ? "1" : "0");
		WritePrivateProfileString(sec_name, "Visible", buf, ini_file);

		/* Sub-window window title */
		WritePrivateProfileString(sec_name, "WindowTitle", ang_term_name[term_idx], ini_file);
	}

#ifdef USE_LOGFONT
	if (use_logfont) {
		WritePrivateProfileString(sec_name, "Font", *(td->lf.lfFaceName) ? td->lf.lfFaceName : DEFAULT_LOGFONTNAME, ini_file);
		wsprintf(buf, "%d", td->lf.lfWidth);
		WritePrivateProfileString(sec_name, "FontWid", buf, ini_file);
		wsprintf(buf, "%d", td->lf.lfHeight);
		WritePrivateProfileString(sec_name, "FontHgt", buf, ini_file);
		wsprintf(buf, "%d", td->lf.lfWeight);
		WritePrivateProfileString(sec_name, "FontWgt", buf, ini_file);
		wsprintf(buf, "%d", (td->lf.lfQuality == ANTIALIASED_QUALITY) ? 1 : 0);
		WritePrivateProfileString(sec_name, "FontAA", buf, ini_file);
	} else
#endif
	{
		/* Desired font */
		if (td->font_file)
			WritePrivateProfileString(sec_name, "Font", td->font_file, ini_file);
	}

	if (td != &data[0]) {
		/* Current size (x) */
		wsprintf(buf, "%d", td->cols);
		WritePrivateProfileString(sec_name, "Columns", buf, ini_file);

		/* Current size (y) */
		wsprintf(buf, "%d", td->rows);
		WritePrivateProfileString(sec_name, "Rows", buf, ini_file);
	} else {
		int hgt;

#ifndef GLOBAL_BIG_MAP
		if (c_cfg.big_map || data[0].rows > SCREEN_HGT + SCREEN_PAD_Y) {
#else
		if (global_c_cfg_big_map || data[0].rows > SCREEN_HGT + SCREEN_PAD_Y) {
#endif
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
void save_prefs(void) {
#if defined(USE_GRAPHICS) || defined(USE_SOUND)
	char buf[32];
	int i;
#endif

	strcpy(buf, INI_disable_CS_IME ? "1" : "0");
	WritePrivateProfileString("Base", "ForceIMEOff", buf, ini_file);
	strcpy(buf, INI_enable_CS_IME ? "1" : "0");
	WritePrivateProfileString("Base", "ForceIMEOn", buf, ini_file);

#ifdef USE_LOGFONT
	strcpy(buf, use_logfont_ini ? "1" : "0");
	WritePrivateProfileString("Base", "LogFont", buf, ini_file);
#endif

#ifdef USE_GRAPHICS
	strcpy(buf, use_graphics_new == UG_2MASK ? "2" : (use_graphics_new ? "1" : "0"));
	WritePrivateProfileString("Base", "Graphics", buf, ini_file);
	WritePrivateProfileString("Base", "GraphicTiles", graphic_tiles, ini_file);
	for (i = 0; i < MAX_SUBFONTS; i++)
		WritePrivateProfileInt("Base", format("GraphicSubTiles%d", graphic_subtiles[i] ? 1 : 0), ini_file);
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
	save_prefs_aux(0, "Term-Main");
	save_prefs_aux(1, "Term-1");
	save_prefs_aux(2, "Term-2");
	save_prefs_aux(3, "Term-3");
	save_prefs_aux(4, "Term-4");
	save_prefs_aux(5, "Term-5");
	save_prefs_aux(6, "Term-6");
	save_prefs_aux(7, "Term-7");
	save_prefs_aux(8, "Term-8");
	save_prefs_aux(9, "Term-9");

	/* Delete deprecated entries now that they have been copy-converted. */
	if (convert_ini) {
		logprint("Writing back window-parameter conversions to .ini file.\n");
		WritePrivateProfileString("Main window", NULL, NULL, ini_file);
		WritePrivateProfileString("Mirror window", NULL, NULL, ini_file);
		WritePrivateProfileString("Recall window", NULL, NULL, ini_file);
		WritePrivateProfileString("Choice window", NULL, NULL, ini_file);
		WritePrivateProfileString("Term-4 window", NULL, NULL, ini_file);
		WritePrivateProfileString("Term-5 window", NULL, NULL, ini_file);
		WritePrivateProfileString("Term-6 window", NULL, NULL, ini_file);
		WritePrivateProfileString("Term-7 window", NULL, NULL, ini_file);
		WritePrivateProfileString("Term-8 window", NULL, NULL, ini_file);
		WritePrivateProfileString("Term-9 window", NULL, NULL, ini_file);
	}
}


/*
 * Load preference for a single term
 */
static void load_prefs_aux(term_data *td, cptr sec_name) {
	char tmp[128];
	int i = 0;

	/* Hack for auto-conversion of 4.9.2->4.9.3 config files: Test if main window exists.
	   If it doesn't that would be because it uses outdated names, so it must be pre 4.9.3 and we convert it. */
	if (!convert_ini && td == &data[0] && GetPrivateProfileInt(sec_name, "WindowNumber", -1, ini_file) == -1) {
		convert_ini = TRUE;
		plog("Auto-converting old .ini file.");
	}
	if (convert_ini) {
		/* Read deprecated equivalent of the name */
		if (td == &data[0]) sec_name = "Main window";
		else if (td == &data[1]) sec_name = "Mirror window";
		else if (td == &data[2]) sec_name = "Recall window";
		else if (td == &data[3]) sec_name = "Choice window";
		else if (td == &data[4]) sec_name = "Term-4 window";
		else if (td == &data[5]) sec_name = "Term-5 window";
		else if (td == &data[6]) sec_name = "Term-6 window";
		else if (td == &data[7]) sec_name = "Term-7 window";
		else if (td == &data[8]) sec_name = "Term-8 window";
		else if (td == &data[9]) sec_name = "Term-9 window";
	}

	/* Visibility (Sub-window) */
	if (td != &data[0]) {
		/* Extract visibility */
		td->visible = (GetPrivateProfileInt(sec_name, "Visible", td->visible, ini_file) != 0);
	}

#if 0 /* just do this in dedicated load_prefs_logfont() actually */
#ifdef USE_LOGFONT
	use_logfont = use_logfont_ini = (GetPrivateProfileInt(sec_name, "LogFont", FALSE, ini_file) != 0);
#endif
#endif

	/* Desired font, with default */
#ifdef USE_LOGFONT
	if (use_logfont) {
		GetPrivateProfileString(sec_name, "Font", DEFAULT_LOGFONTNAME, tmp, 127, ini_file);
		td->font_want = string_make(tmp);
		td->lf.lfWidth  = GetPrivateProfileInt(sec_name, "FontWid", 0, ini_file);
		td->lf.lfHeight = GetPrivateProfileInt(sec_name, "FontHgt", 15, ini_file);
		td->lf.lfWeight = GetPrivateProfileInt(sec_name, "FontWgt", 0, ini_file);
		td->lf.lfQuality = ((GetPrivateProfileInt(sec_name, "FontAA", 0, ini_file)) == 1) ? ANTIALIASED_QUALITY : DEFAULT_QUALITY;
	} else
#endif
	{
		GetPrivateProfileString(sec_name, "Font", DEFAULT_FONTNAME, tmp, 127, ini_file);
		td->font_want = string_make(extract_file_name(tmp));
	}

	/* Window size */
	td->cols = GetPrivateProfileInt(sec_name, "Columns", td->cols, ini_file);
	td->rows = GetPrivateProfileInt(sec_name, "Rows", td->rows, ini_file);
	if (!td->cols) td->cols = 80;
	if (!td->rows) td->rows = 24;
	if (td == &data[0]) {
		if (td->cols != 80) td->cols = 80;
		screen_wid = td->cols - SCREEN_PAD_X;
		screen_hgt = td->rows - SCREEN_PAD_Y;

#ifdef GLOBAL_BIG_MAP
		/* Only the rc/ini file determines big_map state now! It is not saved anywhere else! */
		if (screen_hgt <= SCREEN_HGT) global_c_cfg_big_map = FALSE;
		else global_c_cfg_big_map = TRUE;
		//resize_main_window(CL_WINDOW_WID, CL_WINDOW_HGT); -- visual system not yet initialized! ->segfault
#endif
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
#if defined(USE_SOUND) && !defined(USE_SOUND_2010)
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

/* Load just the IME preferences from the .INI file, as it must be done early before Windows are initialized - C. Blue */
static void load_prefs_IME(void) {
	disable_CS_IME = INI_disable_CS_IME = (GetPrivateProfileInt("Base", "ForceIMEOff", 0, ini_file) != 0);
	enable_CS_IME = INI_enable_CS_IME = (GetPrivateProfileInt("Base", "ForceIMEOn", 0, ini_file) != 0);
}
#ifdef TILE_CACHE_SIZE
static void load_prefs_TILE_CACHE(void) {
	if (GetPrivateProfileInt("Base", "DisableGfxCache", 0, ini_file) != 0) disable_tile_cache = TRUE;
}
#endif
#ifdef USE_LOGFONT
/* Load just the logfont preferences from the .INI file, as it must be done early before Windows are initialized */
static void load_prefs_logfont(void) {
	use_logfont = use_logfont_ini = (GetPrivateProfileInt("Base", "LogFont", 0, ini_file) != 0);
}
#endif
/*
 * Load the preferences from the .INI file
 */
static void load_prefs(void) {
	int i;

	/* Extract the "disable_numlock" flag */
	disable_numlock = (GetPrivateProfileInt("Base", "DisableNumlock", 1, ini_file) != 0);

	/* Read the colormap */
	for (i = 0; i < BASE_PALETTE_SIZE; i++) {
		char key_name[12] = { '\0' };
		char default_color[8] = { '\0' };
		unsigned long c = client_color_map[i];
		char want_color[8] = { '\0' };

		snprintf(key_name, sizeof(key_name), "Colormap_%d", i);
		snprintf(default_color, sizeof(default_color), "#%06lx", c);
		GetPrivateProfileString("Base", key_name, default_color, want_color, sizeof(want_color), ini_file);
		client_color_map[i] = parse_color_code(want_color);
#ifdef EXTENDED_COLOURS_PALANIM
		/* Clone it */
		client_color_map[i + BASE_PALETTE_SIZE] = client_color_map[i];
#endif
	}
	/* Extract the readability_blue flag */
	lighterdarkblue = (GetPrivateProfileInt("Base", "LighterDarkBlue", 1, ini_file) != 0);
	if (lighterdarkblue && client_color_map[6] == 0x0000ff) enable_readability_blue_win();
	/* Set the colour map */
	enable_common_colormap_win();

#ifdef USE_GRAPHICS
	/* Extract the "use_graphics" flag */
 #ifdef GRAPHICS_BG_MASK
	use_graphics_new = use_graphics = GetPrivateProfileInt("Base", "Graphics", 0, ini_file) % 3; //max UG_2MASK
 #else
	use_graphics_new = use_graphics = (GetPrivateProfileInt("Base", "Graphics", 0, ini_file) != 0);
 #endif
	GetPrivateProfileString("Base", "GraphicTiles", DEFAULT_TILENAME, graphic_tiles, 255, ini_file);
	/* Convert to lowercase. */
	for (int i =0; i < 256; i++)
		graphic_tiles[i] = tolower(graphic_tiles[i]);
	for (i = 0; i < MAX_SUBFONTS; i++)
		graphic_subtiles[i] = (GetPrivateProfileInt("Base", format("GraphicSubTiles%d", i), 1, ini_file) != 0);
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
	GetPrivateProfileString("Base", "SoundpackFolder", "sound", cfg_soundpackfolder, 1024, ini_file);
	GetPrivateProfileString("Base", "MusicpackFolder", "music", cfg_musicpackfolder, 1024, ini_file);
	cfg_audio_music = (GetPrivateProfileInt("Base", "AudioMusic", 1, ini_file) != 0);
	cfg_audio_sound = (GetPrivateProfileInt("Base", "AudioSound", 1, ini_file) != 0);
	cfg_audio_weather = (GetPrivateProfileInt("Base", "AudioWeather", 1, ini_file) != 0);
	cfg_audio_master_volume = GetPrivateProfileInt("Base", "AudioVolumeMaster", AUDIO_VOLUME_DEFAULT, ini_file);
	cfg_audio_music_volume = GetPrivateProfileInt("Base", "AudioVolumeMusic", AUDIO_VOLUME_DEFAULT, ini_file);
	cfg_audio_sound_volume = GetPrivateProfileInt("Base", "AudioVolumeSound", AUDIO_VOLUME_DEFAULT, ini_file);
	cfg_audio_weather_volume = GetPrivateProfileInt("Base", "AudioVolumeWeather", AUDIO_VOLUME_DEFAULT, ini_file);
 #endif
#endif

	/* Load window prefs */
	load_prefs_aux(&data[0], "Term-Main");
	load_prefs_aux(&data[1], "Term-1");
	load_prefs_aux(&data[2], "Term-2");
	load_prefs_aux(&data[3], "Term-3");
	load_prefs_aux(&data[4], "Term-4");
	load_prefs_aux(&data[5], "Term-5");
	load_prefs_aux(&data[6], "Term-6");
	load_prefs_aux(&data[7], "Term-7");
	load_prefs_aux(&data[8], "Term-8");
	load_prefs_aux(&data[9], "Term-9");
	//remember, for writing it back later. ---convert_ini = FALSE; /* In case the first load_prefs_aux(0) call started an automatic ini-file conversion, reset this state after all has been converted now. */

	bigmap_hint = (GetPrivateProfileInt("Base", "HintBigmap", 1, ini_file) != 0);
	if (!bigmap_hint) firstrun = FALSE;
	//WritePrivateProfileString("Base", "HintBigmap", "0", ini_file); --instead done in ask_for_bigmap_generic()

#if defined(USE_SOUND) && !defined(USE_SOUND_2010)
	/* Prepare the sounds */
	for (i = 1; i < SOUND_MAX; i++) load_prefs_sound(i);
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

	/* Size of palette */
	pLogPalSize = sizeof(LOGPALETTE) + (CLIENT_PALETTE_SIZE + nEntries) * sizeof(PALETTEENTRY);

	/* Allocate palette */
	pLogPal = (LPLOGPALETTE)mem_alloc(pLogPalSize);

	/* Version */
	pLogPal->palVersion = 0x300;

	/* Make room for bitmap and normal data */
	pLogPal->palNumEntries = nEntries + CLIENT_PALETTE_SIZE;

	/* Save the bitmap data */
	for (i = 0; i < nEntries; i++)
		pLogPal->palPalEntry[i] = lppe[i];

	/* Save the normal data */
	for (i = 0; i < CLIENT_PALETTE_SIZE; i++) {
		LPPALETTEENTRY p;

		/* Access the entry */
		p = &(pLogPal->palPalEntry[i + nEntries]);

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

	/* Size of palette */
	pLogPalSize = sizeof(LOGPALETTE) + (BASE_PALETTE_SIZE + nEntries) * sizeof(PALETTEENTRY);

	/* Allocate palette */
	pLogPal = (LPLOGPALETTE)mem_alloc(pLogPalSize);

	/* Version */
	pLogPal->palVersion = 0x300;

	/* Make room for bitmap and normal data */
	pLogPal->palNumEntries = nEntries + BASE_PALETTE_SIZE;

	/* Save the bitmap data */
	for (i = 0; i < nEntries; i++)
		pLogPal->palPalEntry[i] = lppe[i];

	/* Save the normal data */
	for (i = 0; i < BASE_PALETTE_SIZE; i++) {
		LPPALETTEENTRY p;

		/* Access the entry */
		p = &(pLogPal->palPalEntry[i + nEntries]);

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
 * Force the use of a new "font file" for a term_data.
 * This function may be called before the "window" is ready.
 * This function returns zero only if everything succeeds.
 */
static errr term_force_font(term_data *td, cptr name) {
	int wid, hgt, prev_font_wid, prev_font_hgt;

#ifdef USE_GRAPHICS
 #ifdef USE_LOGFONT
	if (use_logfont) {
		prev_font_wid = td->lf.lfWidth;
		prev_font_hgt = td->lf.lfHeight;
	} else
 #endif
	{
		prev_font_wid = td->font_wid;
		prev_font_hgt = td->font_hgt;
	}
#endif

#ifdef USE_LOGFONT
	if (use_logfont) {
		td->font_id = CreateFontIndirect(&(td->lf));
		if (!td->font_id) return(1);
		wid = td->lf.lfWidth;
		hgt = td->lf.lfHeight;
	} else
#endif
	{
		int i;
		cptr s;
		char base[16];
		char base_font[16];
		char buf[1024];
		bool used;

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
		if (!name) return(1);

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
			if (base[i] == 'X') hgt = atoi(s + i + 1);
		}

		/* Terminate */
		base[i] = '\0';

		/* Build base_font */
		strcpy(base_font, base);
		strcat(base_font, ".FON");

		/* Access the font file */
		path_build(buf, 1024, ANGBAND_DIR_XTRA_FONT, base_font);

		/* Verify file */
		if (!check_file(buf)) return(1);

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
				quit_fmt("Font file corrupted:\n%s\nTry running TomeNET in privileged/admin mode!", buf);
		}

		/* Notify other applications that a new font is available  XXX */
		PostMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);

		/* Create the font XXX XXX XXX Note use of "base" */
		td->font_id = CreateFont(hgt, wid, 0, 0, FW_DONTCARE, 0, 0, 0, ANSI_CHARSET,
		                         OUT_DEFAULT_PRECIS, //(OUT_DEFAULT_PRECIS,) OUT_OUTLINE_PRECIS, OUT_TT_PRECIS (slow), OUT_TT_ONLY_PRECIS
		                         CLIP_DEFAULT_PRECIS,
		                         /* Don't antialiase non-logfonts maybe, just go with default instead:
		                            (hgt >= 9 && wid >= 9 && hgt + wid >= 23) ? ANTIALIASED_QUALITY : DEFAULT_QUALITY, //(DEFAULT_QUALITY,) ANTIALIASED_QUALITY, CLEARTYPE_QUALITY (perhaps slow) */
		                         DEFAULT_QUALITY,
		                         FIXED_PITCH | FF_DONTCARE,
		                         base);
	}

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

#ifdef USE_GRAPHICS
 #ifdef USE_LOGFONT
	if (use_logfont) {
		if (use_graphics && g_hbmTiles != NULL && (prev_font_wid != td->lf.lfWidth || prev_font_hgt != td->lf.lfHeight))
			recreateGraphicsObjects(td);
	} else
 #endif
	{
		if (use_graphics && g_hbmTiles != NULL && (prev_font_wid != td->font_wid || prev_font_hgt != td->font_hgt))
			recreateGraphicsObjects(td);
	}
#endif

	/* Success */
	return(0);
}

/*
 * Allow the user to change the font for this window.
 *
 * XXX XXX XXX This is only called for non-graphic windows
 */
static void term_change_font(term_data *td) {
#ifdef USE_LOGFONT
	if (use_logfont) {
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
	} else
#endif
	{
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
	}
}

/*
 * Hack -- redraw a term_data
 */
static void term_data_redraw(term_data *td) {
	/* Activate the term */
	Term_activate(&td->t);

	/* Redraw the contents */
	Term_redraw();

	/* Restore the term */
	Term_activate(term_term_main);
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
	Term_activate(term_term_main);
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
 #ifndef CUSTOMIZE_COLOURS_FREELY
		SetBkColor(oldDC, RGB(0, 0, 0));
 #else
		SetBkColor(oldDC, RGB(color_table[0][1], color_table[0][2], color_table[0][3]));
		//SetBkColor(oldDC, RGB(GetRValue(win_clr[i]), GetGValue(win_clr[i]), GetBValue(win_clr[i]));
 #endif
		SelectObject(oldDC, td->font_id);

		/* Foreground color not set */
		old_attr = -1;
	}
	return(oldDC);
#else
	return(GetDC(hWnd));
#endif
}




/*** Function hooks needed by "Term" ***/

/*
 * Interact with the User
 */
static errr Term_user_win(int n) {
	/* Success */
	return(0);
}


/*
 * React to global changes
 */
static errr Term_xtra_win_react(void) {
	int i;
	term *old;
	term_data *td;

	/* Clean up windows */
	for (i = 0; i < MAX_TERM_DATA; i++) {
		/* Save */
		old = Term;
		td = &data[i];

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
	return(0);
}


/* For OPTIMIZE_DRAWING - declare before use */
errr Term_xtra_win_fresh(int v);


/*
 * Process at least one event
 */
static errr Term_xtra_win_event(int v) {
	MSG msg;

#ifdef OPTIMIZE_DRAWING
	/* Release DC before waiting */
	Term_xtra_win_fresh(v);
#endif

	/* Wait for an event */
	if (v) {
		/* Block */
		if (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	/* Check for an event */
	else {
		/* Check */
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	/* Success */
	return(0);
}


/*
 * Process all pending events
 */
static errr Term_xtra_win_flush(void) {
	MSG msg;

	/* Process all pending events */
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	/* Success */
	return(0);
}


/*
 * Hack -- clear the screen
 *
 * XXX XXX XXX Make this more efficient
 */
static errr Term_xtra_win_clear(void) {
	term_data *td = (term_data*)(Term->data);
	int fwid, fhgt;

#ifdef USE_LOGFONT
	if (use_logfont) {
		fwid = td->lf.lfWidth;
		fhgt = td->lf.lfHeight;
	} else
#endif
	{
		fwid = td->font_wid;
		fhgt = td->font_hgt;
	}

	HDC  hdc;
	RECT rc;


	/* Rectangle to erase */
	rc.left   = td->size_ow1;
	rc.right  = rc.left + td->cols * fwid;
	rc.top    = td->size_oh1;
	rc.bottom = rc.top + td->rows * fhgt;

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
	return(0);
}


/*
 * Hack -- make a noise
 */
static errr Term_xtra_win_noise(void) {
	MessageBeep(MB_ICONASTERISK);
	return(0);
}


/*
 * Hack -- make a sound
 */
static errr Term_xtra_win_sound(int v) {
	/* Unknown sound */
	if ((v < 0) || (v >= SOUND_MAX)) return(1);

	/* Unknown sound */
	if (!sound_file[v]) return(1);

#ifdef USE_SOUND

#ifdef WIN32

	/* Play the sound, catch errors */
	return(PlaySound(sound_file[v], 0, SND_FILENAME | SND_ASYNC));

#else /* WIN32 */

	/* Play the sound, catch errors */
	return(sndPlaySound(sound_file[v], SND_ASYNC));

#endif /* WIN32 */

#endif /* USE_SOUND */

	/* Oops */
	return(1);
}


/*
 * Delay for "x" milliseconds
 */
static errr Term_xtra_win_delay(int v) {

#ifdef WIN32

	/* Sleep */
	Sleep(v);

#else /* WIN32 */

	DWORD t;
	MSG   msg;

	/* Final count */
	t = GetTickCount() + v;

	/* Wait for it */
	while (GetTickCount() < t) {
		/* Handle messages */
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

#endif /* WIN32 */

	/* Success */
	return(0);
}


/*
 * For OPTIMIZE_DRAWING, made available for z-term.c to fix the visual glitch
 * if a message ("The sun has risen.") is received together with the palette changes,
 * probably resulting in the hdc from term 'chat+msg' being wrongly used to paint into the main screen.
 * This is where we free the DC we've been using.  -- note: 'v' is unused.
 */
errr Term_xtra_win_fresh(int v) {
#ifdef OPTIMIZE_DRAWING
	term_data *td = (term_data*)(Term->data);

	if (oldDC) {
		ReleaseDC(td->w, oldDC);
		oldDC = NULL;
	}

	/* Success */
#endif
	return(0);
}


/*
 * Do a "special thing"
 */
static errr Term_xtra_win(int n, int v) {
	/* Handle a subset of the legal requests */
	switch (n) {
		/* Make a bell sound */
		case TERM_XTRA_NOISE:
		return(Term_xtra_win_noise());

		/* Make a special sound */
		case TERM_XTRA_SOUND:
		return(Term_xtra_win_sound(v));

		/* Process random events */
		case TERM_XTRA_BORED:
		return(Term_xtra_win_event(0));

		/* Process an event */
		case TERM_XTRA_EVENT:
		return(Term_xtra_win_event(v));

		/* Flush all events */
		case TERM_XTRA_FLUSH:
		return(Term_xtra_win_flush());

		/* Clear the screen */
		case TERM_XTRA_CLEAR:
		return(Term_xtra_win_clear());

		/* React to global changes */
		case TERM_XTRA_REACT:
		return(Term_xtra_win_react());

		/* Delay for some milliseconds */
		case TERM_XTRA_DELAY:
		return(Term_xtra_win_delay(v));

#ifdef OPTIMIZE_DRAWING
		case TERM_XTRA_FRESH:
		return(Term_xtra_win_fresh(v));
#endif
	}

	/* Oops */
	return(1);
}



/*
 * Low level graphics (Assumes valid input).
 *
 * Erase a "block" of "n" characters starting at (x,y).
 */
static errr Term_wipe_win(int x, int y, int n) {
	term_data *td = (term_data*)(Term->data);
	int fwid, fhgt;

#ifdef USE_LOGFONT
	if (use_logfont) {
		fwid = td->lf.lfWidth;
		fhgt = td->lf.lfHeight;
	} else
#endif
	{
		fwid = td->font_wid;
		fhgt = td->font_hgt;
	}

	HDC  hdc;
	RECT rc;


	/* Rectangle to erase in client coords */
	rc.left   = x * fwid + td->size_ow1;
	rc.right  = rc.left + n * fwid;
	rc.top    = y * fhgt + td->size_oh1;
	rc.bottom = rc.top + fhgt;

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
	return(0);
}


/*
 * Low level graphics (Assumes valid input).
 * Draw a "cursor" at (x,y), using a "yellow box".
 */
static errr Term_curs_win(int x, int y) {
	term_data *td = (term_data*)(Term->data);
	int fwid, fhgt;

#ifdef USE_LOGFONT
	if (use_logfont) {
		fwid = td->lf.lfWidth;
		fhgt = td->lf.lfHeight;
	} else
#endif
	{
		fwid = td->font_wid;
		fhgt = td->font_hgt;
	}

	RECT   rc;
	HDC    hdc;


	/* Frame the grid */
	rc.left   = x * fwid + td->size_ow1;
	rc.right  = rc.left + fwid;
	rc.top    = y * fhgt + td->size_oh1;
	rc.bottom = rc.top + fhgt;

	/* Cursor is done as a yellow "box" */
	hdc = myGetDC(data[0].w);
	FrameRect(hdc, &rc, hbrYellow);
#ifndef OPTIMIZE_DRAWING
	ReleaseDC(data[0].w, hdc);
#endif

	/* Success */
	return(0);
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
static errr Term_pict_win(int x, int y, byte a, char32_t c) {
#ifdef USE_GRAPHICS
	term_data *td;
	int x1, y1;
	COLORREF bgColor, fgColor;
	HDC hdc;

	RECT rectBg;
	RECT rectFg;
	HBRUSH brushBg;
	HBRUSH brushFg;

	int fwid, fhgt;

 #ifdef TILE_CACHE_SIZE
	struct tile_cache_entry *entry;
	int i, hole = -1;
  #ifdef TILE_CACHE_SINGLEBMP
	int tc_idx, tc_x, tc_y;
  #endif
 #endif


	/* Catch use in chat instead of as feat attr, or we crash :-s
	   (term-idx 0 is the main window; screen-pad-left check: In case it is used in the status bar for some reason; screen-pad-top check: main screen top chat line) */
	if (Term && Term->data == &data[0] && x >= SCREEN_PAD_LEFT && x < SCREEN_PAD_LEFT + screen_wid && y >= SCREEN_PAD_TOP && y < SCREEN_PAD_TOP + screen_hgt) {
		flick_global_x = x;
		flick_global_y = y;
	} else flick_global_x = 0;

	a = term2attr(a);

	bgColor = RGB(0, 0, 0);
	fgColor = RGB(0, 0, 0);

 #ifdef PALANIM_SWAP
	if (a < CLIENT_PALETTE_SIZE) a = (a + BASE_PALETTE_SIZE) % CLIENT_PALETTE_SIZE;
 #endif

	/* Background/Foreground color */
 #ifndef EXTENDED_COLOURS_PALANIM
  #ifndef EXTENDED_BG_COLOURS
	fgColor = win_clr[a & 0x0F];
  #else
	fgColor = win_clr[a & 0x1F];
	//bgColor = PALETTEINDEX(win_clr_bg[a & 0x0F]); //wrong / undefined state, as we don't want to have palette indices 0..15 + 32..32+TERMX_AMT with a hole in between?
	bgColor = win_clr_bg[a & 0x1F]; //wrong / undefined state, as we don't want to have palette indices 0..15 + 32..32+TERMX_AMT with a hole in between?
  #endif
 #else
  #ifndef EXTENDED_BG_COLOURS
	fgColor = win_clr[a & 0x1F];
  #else
	fgColor = win_clr[a & 0x3F];
	//bgColor = PALETTEINDEX(win_clr_bg[a & 0x1F]); //verify correctness
	bgColor = win_clr_bg[a & 0x3F]; //verify correctness
  #endif
 #endif

	td = (term_data*)(Term->data);

 #ifdef USE_LOGFONT
	if (use_logfont) {
		fwid = td->lf.lfWidth;
		fhgt = td->lf.lfHeight;
	} else
 #endif
	{
		fwid = td->font_wid;
		fhgt = td->font_hgt;
	}

	/* Location of screen window cell */
	x = x * fwid + td->size_ow1;
	y = y * fhgt + td->size_oh1;

	/* Location of graphics tile in the tileset */
	x1 = ((c - MAX_FONT_CHAR - 1) % graphics_image_tpr) * fwid;
	y1 = ((c - MAX_FONT_CHAR - 1) / graphics_image_tpr) * fhgt;

	hdc = myGetDC(td->w);


 #ifdef TILE_CACHE_SIZE
	if (!disable_tile_cache) {
		entry = NULL;
		for (i = 0; i < TILE_CACHE_SIZE; i++) {
			entry = &td->tile_cache[i];
			if (!entry->is_valid) hole = i;
			else if (entry->c == c && entry->a == a
  #ifdef GRAPHICS_BG_MASK
			    && entry->c_back == 32 && entry->a_back == TERM_DARK
  #endif
  #ifdef TILE_CACHE_FGBG /* Instead of this, invalidate_graphics_cache_...() will specifically invalidate affected entries */
			    /* Extra: Verify that palette is identical - allows palette_animation to work w/o invalidating the whole cache each time: */
			    && fgColor == entry->fg && bgColor == entry->bg
  #endif
			    ) {
				/* Copy cached tile to window. */
  #ifdef TILE_CACHE_SINGLEBMP
				//SelectObject(td->hdcCacheTilePreparation, td->hbmCacheTilePreparation); //already selected?
				BitBlt(hdc, x, y, fwid, fhgt, td->hdcCacheTilePreparation, (i % TILE_CACHE_SINGLEBMP) * 2 * fwid, (i / TILE_CACHE_SINGLEBMP) * fhgt, SRCCOPY);
  #else
				SelectObject(td->hdcTilePreparation, entry->hbmTilePreparation);
				BitBlt(hdc, x, y, fwid, fhgt, td->hdcTilePreparation, 0, 0, SRCCOPY);
  #endif
  #ifndef OPTIMIZE_DRAWING
				ReleaseDC(td->w, hdc);
  #endif
				/* Success */
				return(0);
			}
		}

		// Replace invalid cache entries right away in-place, so we don't kick other still valid entries out via FIFO'ing
		if (hole != -1) {
  #ifdef TILE_CACHE_LOG
			c_msg_format("Tile cache pos (hole): %d / %d", hole, TILE_CACHE_SIZE);
  #endif
  #ifdef TILE_CACHE_SINGLEBMP
			tc_idx = hole;
  #endif
			entry = &td->tile_cache[hole];
		} else {
  #ifdef TILE_CACHE_LOG
			c_msg_format("Tile cache pos (FIFO): %d / %d", td->cache_position, TILE_CACHE_SIZE);
  #endif
			// Replace valid cache entries in FIFO order
  #ifdef TILE_CACHE_SINGLEBMP
			tc_idx = td->cache_position;
  #endif
			entry = &td->tile_cache[td->cache_position++];
			if (td->cache_position >= TILE_CACHE_SIZE) td->cache_position = 0;
		}

  #ifndef TILE_CACHE_SINGLEBMP
		SelectObject(td->hdcTilePreparation, entry->hbmTilePreparation);
  #else
		//SelectObject(td->hdcCacheTilePreparation, td->hbmCacheTilePreparation); //already selected?
  #endif

		entry->c = c;
		entry->a = a;
  #ifdef GRAPHICS_BG_MASK
   #if 0
		entry->c_back = 32;
   #else
		entry->c_back = Client_setup.f_char[FEAT_SOLID];;
   #endif
		entry->a_back = TERM_DARK;
  #endif
		entry->is_valid = TRUE;
  #ifdef TILE_CACHE_FGBG
		entry->fg = fgColor;
		entry->bg = bgColor;
  #endif
	} else SelectObject(td->hdcTilePreparation, td->hbmTilePreparation);
 #else /* (TILE_CACHE_SIZE) No caching: */
	SelectObject(td->hdcTilePreparation, td->hbmTilePreparation);
 #endif


 #if defined(TILE_CACHE_SIZE) && defined(TILE_CACHE_SINGLEBMP)
	if (!disable_tile_cache) {
		tc_x = (tc_idx % TILE_CACHE_SINGLEBMP) * 2 * fwid;
		tc_y = (tc_idx / TILE_CACHE_SINGLEBMP) * fhgt;

		/* Paint background rectangle .*/
		rectBg = (RECT){ tc_x, tc_y, tc_x + fwid, tc_y + fhgt };
		rectFg = (RECT){ tc_x + fwid, tc_y, tc_x + 2 * fwid, tc_y + fhgt };
		brushBg = CreateSolidBrush(bgColor);
		brushFg = CreateSolidBrush(fgColor);
		FillSolidRect(td->hdcCacheTilePreparation, &rectBg, brushBg);
		FillSolidRect(td->hdcCacheTilePreparation, &rectFg, brushFg);
		DeleteObject(brushBg);
		DeleteObject(brushFg);


		//BitBlt(hdc, 0, 0, 2*9, 15, td->hdcCacheTilePreparation, 0, 0, SRCCOPY);

		BitBlt(td->hdcCacheTilePreparation, tc_x + fwid, tc_y, fwid, fhgt, td->hdcFgMask, x1, y1, SRCAND);
		BitBlt(td->hdcCacheTilePreparation, tc_x + fwid, tc_y, fwid, fhgt, td->hdcTiles, x1, y1, SRCPAINT);

		//BitBlt(hdc, 0, 15, 2*9, 15, td->hdcCacheTilePreparation, 0, 0, SRCCOPY);

		BitBlt(td->hdcCacheTilePreparation, tc_x, tc_y, fwid, fhgt, td->hdcBgMask, x1, y1, SRCAND);
		BitBlt(td->hdcCacheTilePreparation, tc_x, tc_y, fwid, fhgt, td->hdcCacheTilePreparation, tc_x + fwid, tc_y, SRCPAINT);

		//BitBlt(hdc, 0, 15, 5*9, 15, td->hdcBgMask, 0, 0, SRCCOPY);
		//BitBlt(hdc, 0, 2*15, 5*9, 15, td->hdcFgMask, 0, 0, SRCCOPY);
		//
		/* Copy the picture from the tile preparation memory to the window */
  #ifdef GRAPHICS_SHADED_ALPHA
		BLENDFUNCTION bf;
		bf.BlendOp = AC_SRC_OVER; //forced (0)
		bf.BlendFlags = 0; //forced
		bf.SourceConstantAlpha = 255; //0..255
		bf.AlphaFormat = AC_SRC_ALPHA; //forced (1)
		AlphaBlend(hdc, x, y, fwid, fhgt, td->hdcTilePreparation, 0, 0, fwid, fhgt, bf);
  #else
		BitBlt(hdc, x, y, fwid, fhgt, td->hdcCacheTilePreparation, tc_x, tc_y, SRCCOPY);
  #endif
	} else
 #endif
	{
		/* Paint background rectangle .*/
		rectBg = (RECT){ 0, 0, fwid, fhgt };
		rectFg = (RECT){ fwid, 0, 2 * fwid, fhgt };
		brushBg = CreateSolidBrush(bgColor);
		brushFg = CreateSolidBrush(fgColor);
		FillRect(td->hdcTilePreparation, &rectBg, brushBg);
		FillRect(td->hdcTilePreparation, &rectFg, brushFg);
		DeleteObject(brushBg);
		DeleteObject(brushFg);


		//BitBlt(hdc, 0, 0, 2*9, 15, td->hdcTilePreparation, 0, 0, SRCCOPY);

		BitBlt(td->hdcTilePreparation, fwid, 0, fwid, fhgt, td->hdcFgMask, x1, y1, SRCAND);
		BitBlt(td->hdcTilePreparation, fwid, 0, fwid, fhgt, td->hdcTiles, x1, y1, SRCPAINT);

		//BitBlt(hdc, 0, 15, 2*9, 15, td->hdcTilePreparation, 0, 0, SRCCOPY);

		BitBlt(td->hdcTilePreparation, 0, 0, fwid, fhgt, td->hdcBgMask, x1, y1, SRCAND);
		BitBlt(td->hdcTilePreparation, 0, 0, fwid, fhgt, td->hdcTilePreparation, fwid, 0, SRCPAINT);

		//BitBlt(hdc, 0, 15, 5*9, 15, td->hdcBgMask, 0, 0, SRCCOPY);
		//BitBlt(hdc, 0, 2*15, 5*9, 15, td->hdcFgMask, 0, 0, SRCCOPY);
		//
		/* Copy the picture from the tile preparation memory to the window */
		BitBlt(hdc, x, y, fwid, fhgt, td->hdcTilePreparation, 0, 0, SRCCOPY);
	}


 #ifndef OPTIMIZE_DRAWING
	ReleaseDC(td->w, hdc);
 #endif

#else /* #ifdef USE_GRAPHICS */

	/* Just erase this grid */
	return(Term_wipe_win(x, y, 1));

#endif

	/* Success */
	return(0);
}

#ifdef GRAPHICS_BG_MASK
static errr Term_pict_win_2mask(int x, int y, byte a, char32_t c, byte a_back, char32_t c_back) {
 #if 0 /* use fallback hook until 2mask routines are complete? */
	return(Term_pict_win(x, y, a, c));
 #endif
 #ifdef USE_GRAPHICS
	term_data *td;
	int x1, y1, x1b, y1b;
	COLORREF bgColor, fgColor;
	HDC hdc;

	RECT rectBg;
	RECT rectFg;
	HBRUSH brushBg;
	HBRUSH brushFg;

	int fwid, fhgt;

  #ifdef TILE_CACHE_SIZE
	struct tile_cache_entry *entry;
	int i, hole = -1;
   #ifdef TILE_CACHE_SINGLEBMP
	int tc_idx, tc_x, tc_y;
   #endif
  #endif

	/* Avoid visual glitches while not in 2mask mode */
	if (use_graphics != UG_2MASK) {
		a_back = TERM_DARK;
  #if 0
		c_back = 32; //space! NOT zero!
  #else
		c_back = Client_setup.f_char[FEAT_SOLID]; // 'graphical space' for erasure
  #endif
	}

	/* SPACE = erase background, aka black background. This is for places where we have no bg-info, such as client-lore in knowledge menu. */
	if (c_back == 32) a_back = TERM_DARK;

	/* Catch use in chat instead of as feat attr, or we crash :-s
	   (term-idx 0 is the main window; screen-pad-left check: In case it is used in the status bar for some reason; screen-pad-top check: main screen top chat line) */
	if (Term && Term->data == &data[0] && x >= SCREEN_PAD_LEFT && x < SCREEN_PAD_LEFT + screen_wid && y >= SCREEN_PAD_TOP && y < SCREEN_PAD_TOP + screen_hgt) {
		flick_global_x = x;
		flick_global_y = y;
	} else flick_global_x = 0;

	a = term2attr(a);
	a_back = term2attr(a_back);

  #ifdef PALANIM_SWAP
	if (a < CLIENT_PALETTE_SIZE) a = (a + BASE_PALETTE_SIZE) % CLIENT_PALETTE_SIZE;
	if (a_back < CLIENT_PALETTE_SIZE) a_back = (a_back + BASE_PALETTE_SIZE) % CLIENT_PALETTE_SIZE;
  #endif

	td = (term_data*)(Term->data);

  #ifdef USE_LOGFONT
	if (use_logfont) {
		fwid = td->lf.lfWidth;
		fhgt = td->lf.lfHeight;
	} else
  #endif
	{
		fwid = td->font_wid;
		fhgt = td->font_hgt;
	}

	/* Location of window cell */
	x = x * fwid + td->size_ow1;
	y = y * fhgt + td->size_oh1;

	hdc = myGetDC(td->w);


	/* --- Foreground (main) graphical tile --- */

	/* Background/Foreground color */
	bgColor = RGB(0, 0, 0); //this 0-init not really needed?
	fgColor = RGB(0, 0, 0);

  #ifndef EXTENDED_COLOURS_PALANIM
   #ifndef EXTENDED_BG_COLOURS
	fgColor = win_clr[a & 0x0F];
   #else
	fgColor = win_clr[a & 0x1F];
	//bgColor = PALETTEINDEX(win_clr_bg[a & 0x0F]); //wrong / undefined state, as we don't want to have palette indices 0..15 + 32..32+TERMX_AMT with a hole in between?
	bgColor = win_clr_bg[a & 0x1F]; //wrong / undefined state, as we don't want to have palette indices 0..15 + 32..32+TERMX_AMT with a hole in between?
   #endif
  #else
   #ifndef EXTENDED_BG_COLOURS
	fgColor = win_clr[a & 0x1F];
   #else
	fgColor = win_clr[a & 0x3F];
	//bgColor = PALETTEINDEX(win_clr_bg[a & 0x1F]); //verify correctness
	bgColor = win_clr_bg[a & 0x3F]; //verify correctness
   #endif
  #endif


   #ifdef TILE_CACHE_SIZE
	if (!disable_tile_cache) {
		entry = NULL;
		for (i = 0; i < TILE_CACHE_SIZE; i++) {
			entry = &td->tile_cache[i];
			if (!entry->is_valid) hole = i;
			else if (entry->c == c && entry->a == a && entry->c_back == c_back && entry->a_back == a_back
    #ifdef TILE_CACHE_FGBG /* Instead of this, invalidate_graphics_cache_...() will specifically invalidate affected entries */
			    /* Extra: Verify that palette is identical - allows palette_animation to work w/o invalidating the whole cache each time: */
			    && fgColor == entry->fg && bgColor == entry->bg
    #endif
			    ) {
				/* Copy cached tile to window. */
  #ifdef TILE_CACHE_SINGLEBMP
				//SelectObject(td->hdcCacheTilePreparation2, td->hbmCacheTilePreparation2); //already selected?
				BitBlt(hdc, x, y, fwid, fhgt, td->hdcCacheTilePreparation2, (i % TILE_CACHE_SINGLEBMP) * 2 * fwid, (i / TILE_CACHE_SINGLEBMP) * fhgt, SRCCOPY);
  #else
				SelectObject(td->hdcTilePreparation2, entry->hbmTilePreparation2);
				BitBlt(hdc, x, y, fwid, fhgt, td->hdcTilePreparation2, 0, 0, SRCCOPY);
  #endif
  #ifndef OPTIMIZE_DRAWING
				ReleaseDC(td->w, hdc);
  #endif
				/* Success */
				return(0);
			}
		}

		// Replace invalid cache entries right away in-place, so we don't kick other still valid entries out via FIFO'ing
		if (hole != -1) {
    #ifdef TILE_CACHE_LOG
			c_msg_format("Tile cache pos (hole): %d / %d", hole, TILE_CACHE_SIZE);
    #endif
    #ifdef TILE_CACHE_SINGLEBMP
			tc_idx = hole;
    #endif
			entry = &td->tile_cache[hole];
		} else {
    #ifdef TILE_CACHE_LOG
			c_msg_format("Tile cache pos (FIFO): %d / %d", td->cache_position, TILE_CACHE_SIZE);
    #endif
			// Replace valid cache entries in FIFO order
    #ifdef TILE_CACHE_SINGLEBMP
			tc_idx = td->cache_position;
    #endif
			entry = &td->tile_cache[td->cache_position++];
			if (td->cache_position >= TILE_CACHE_SIZE) td->cache_position = 0;
		}

  #ifndef TILE_CACHE_SINGLEBMP
		SelectObject(td->hdcTilePreparation, entry->hbmTilePreparation); //in 2mask-mode actually not used, as only tilePreparation2 is interesting as it holds the final result
		SelectObject(td->hdcTilePreparation2, entry->hbmTilePreparation2);
  #else
		//already selected?
		//SelectObject(td->hdcCacheTilePreparation, td->hbmCacheTilePreparation);
		//SelectObject(td->hdcCacheTilePreparation2, td->hbmCacheTilePreparation2);
  #endif

		entry->c = c;
		entry->a = a;
    #ifdef GRAPHICS_BG_MASK
		entry->c_back = c_back;
		entry->a_back = a_back;
    #endif
		entry->is_valid = TRUE;
    #ifdef TILE_CACHE_FGBG
		entry->fg = fgColor;
		entry->bg = bgColor;
    #endif
	} else {
		SelectObject(td->hdcTilePreparation, td->hbmTilePreparation);
		SelectObject(td->hdcTilePreparation2, td->hbmTilePreparation2);
	}
   #else /* (TILE_CACHE_SIZE) No caching: */
	SelectObject(td->hdcTilePreparation, td->hbmTilePreparation);
	SelectObject(td->hdcTilePreparation2, td->hbmTilePreparation2);
   #endif


	/* Note about the graphics tiles image (stored in hdcTiles):
	   Mask generation has blackened (or whitened if 'inverse') any pixel in it that was actually recognized as eligible mask pixel!
	   For this reason, usage of OR (SRCPAINT) bitblt here is correct as it doesn't collide with the original image's mask-pixels (as these are now black). */

	x1 = ((c - MAX_FONT_CHAR - 1) % graphics_image_tpr) * fwid;
	y1 = ((c - MAX_FONT_CHAR - 1) / graphics_image_tpr) * fhgt;


  #if defined(TILE_CACHE_SIZE) && defined(TILE_CACHE_SINGLEBMP)
	if (!disable_tile_cache) {
		tc_x = (tc_idx % TILE_CACHE_SINGLEBMP) * 2 * fwid;
		tc_y = (tc_idx / TILE_CACHE_SINGLEBMP) * fhgt;

		/* Paint background rectangle .*/
		rectBg = (RECT){ tc_x, tc_y, tc_x + fwid, tc_y + fhgt }; //uhh, C99 ^^'
		rectFg = (RECT){ tc_x + fwid, tc_y, tc_x + 2 * fwid, tc_y + fhgt };
		brushBg = CreateSolidBrush(bgColor);
		brushFg = CreateSolidBrush(fgColor);
		FillRect(td->hdcCacheTilePreparation, &rectBg, brushBg);
		FillRect(td->hdcCacheTilePreparation, &rectFg, brushFg);
		DeleteObject(brushBg);
		DeleteObject(brushFg);


		BitBlt(td->hdcCacheTilePreparation, tc_x + fwid, tc_y, fwid, fhgt, td->hdcFgMask, x1, y1, SRCAND);
		BitBlt(td->hdcCacheTilePreparation, tc_x + fwid, tc_y, fwid, fhgt, td->hdcTiles, x1, y1, SRCPAINT);

		BitBlt(td->hdcCacheTilePreparation, tc_x, tc_y, fwid, fhgt, td->hdcBgMask, x1, y1, SRCAND);
		BitBlt(td->hdcCacheTilePreparation, tc_x, tc_y, fwid, fhgt, td->hdcCacheTilePreparation, tc_x + fwid, tc_y, SRCPAINT);


		/* --- Background (terrain) graphical tile --- */

		/* Background/Foreground color */
		bgColor = RGB(0, 0, 0); //this 0-init not really needed?
		fgColor = RGB(0, 0, 0);

   #ifndef EXTENDED_COLOURS_PALANIM
    #ifndef EXTENDED_BG_COLOURS
		fgColor = win_clr[a_back & 0x0F];
    #else
		fgColor = win_clr[a_back & 0x1F];
		//bgColor = PALETTEINDEX(win_clr_bg[a_back & 0x0F]); //wrong / undefined state, as we don't want to have palette indices 0..15 + 32..32+TERMX_AMT with a hole in between?
		bgColor = win_clr_bg[a_back & 0x1F]; //wrong / undefined state, as we don't want to have palette indices 0..15 + 32..32+TERMX_AMT with a hole in between?
    #endif
   #else
    #ifndef EXTENDED_BG_COLOURS
		fgColor = win_clr[a_back & 0x1F];
    #else
		fgColor = win_clr[a_back & 0x3F];
		//bgColor = PALETTEINDEX(win_clr_bg[a_back & 0x1F]); //verify correctness
		bgColor = win_clr_bg[a_back & 0x3F]; //verify correctness
    #endif
   #endif

		/* Note about the graphics tiles image (stored in hdcTiles):
		   Mask generation has blackened (or whitened if 'inverse') any pixel in it that was actually recognized as eligible mask pixel!
		   For this reason, usage of OR (SRCPAINT) bitblt here is correct as it doesn't collide with the original image's mask-pixels (as these are now black). */

		x1b = ((c_back - MAX_FONT_CHAR - 1) % graphics_image_tpr) * fwid;
		y1b = ((c_back - MAX_FONT_CHAR - 1) / graphics_image_tpr) * fhgt;

		/* Paint background rectangle .*/
		rectBg = (RECT){ tc_x, tc_y, tc_x + fwid, tc_y + fhgt }; //uhh, C99 ^^'
		rectFg = (RECT){ tc_x + fwid, tc_y, tc_x + 2 * fwid, tc_y + fhgt };
		brushBg = CreateSolidBrush(bgColor);
		brushFg = CreateSolidBrush(fgColor);
		FillRect(td->hdcCacheTilePreparation2, &rectBg, brushBg);
		FillRect(td->hdcCacheTilePreparation2, &rectFg, brushFg);
		DeleteObject(brushBg);
		DeleteObject(brushFg);


		if (c_back == 32) {
			/* hack: SPACE aka ASCII 32 means empty background ie fill in a_back colour */
		} else {
			BitBlt(td->hdcCacheTilePreparation2, tc_x + fwid, tc_y, fwid, fhgt, td->hdcFgMask, x1b, y1b, SRCAND);
			BitBlt(td->hdcCacheTilePreparation2, tc_x + fwid, tc_y, fwid, fhgt, td->hdcTiles, x1b, y1b, SRCPAINT);

			BitBlt(td->hdcCacheTilePreparation2, tc_x, tc_y, fwid, fhgt, td->hdcBgMask, x1b, y1b, SRCAND);
			BitBlt(td->hdcCacheTilePreparation2, tc_x, tc_y, fwid, fhgt, td->hdcCacheTilePreparation2, tc_x + fwid, tc_y, SRCPAINT);
		}


		/* --- Merge the foreground tile onto the background tile, using the bg2Mask from the foreground tile --- */

		BitBlt(td->hdcCacheTilePreparation2, tc_x, tc_y, fwid, fhgt, td->hdcBg2Mask, x1, y1, SRCAND);
		BitBlt(td->hdcCacheTilePreparation2, tc_x, tc_y, fwid, fhgt, td->hdcCacheTilePreparation, tc_x + fwid, tc_y, SRCPAINT);


		/* --- Copy the picture from the (bg) tile preparation memory to the window --- */
		BitBlt(hdc, x, y, fwid, fhgt, td->hdcCacheTilePreparation2, tc_x, tc_y, SRCCOPY);
	} else
  #endif
	{
		/* Paint background rectangle .*/
		rectBg = (RECT){ 0, 0, fwid, fhgt }; //uhh, C99 ^^'
		rectFg = (RECT){ fwid, 0, 2 * fwid, fhgt };
		brushBg = CreateSolidBrush(bgColor);
		brushFg = CreateSolidBrush(fgColor);
		FillRect(td->hdcTilePreparation, &rectBg, brushBg);
		FillRect(td->hdcTilePreparation, &rectFg, brushFg);
		DeleteObject(brushBg);
		DeleteObject(brushFg);


		BitBlt(td->hdcTilePreparation, fwid, 0, fwid, fhgt, td->hdcFgMask, x1, y1, SRCAND);
		BitBlt(td->hdcTilePreparation, fwid, 0, fwid, fhgt, td->hdcTiles, x1, y1, SRCPAINT);

		BitBlt(td->hdcTilePreparation, 0, 0, fwid, fhgt, td->hdcBgMask, x1, y1, SRCAND);
		BitBlt(td->hdcTilePreparation, 0, 0, fwid, fhgt, td->hdcTilePreparation, fwid, 0, SRCPAINT);


		/* --- Background (terrain) graphical tile --- */

		/* Background/Foreground color */
		bgColor = RGB(0, 0, 0); //this 0-init not really needed?
		fgColor = RGB(0, 0, 0);

   #ifndef EXTENDED_COLOURS_PALANIM
    #ifndef EXTENDED_BG_COLOURS
		fgColor = win_clr[a_back & 0x0F];
    #else
		fgColor = win_clr[a_back & 0x1F];
		//bgColor = PALETTEINDEX(win_clr_bg[a_back & 0x0F]); //wrong / undefined state, as we don't want to have palette indices 0..15 + 32..32+TERMX_AMT with a hole in between?
		bgColor = win_clr_bg[a_back & 0x1F]; //wrong / undefined state, as we don't want to have palette indices 0..15 + 32..32+TERMX_AMT with a hole in between?
    #endif
   #else
    #ifndef EXTENDED_BG_COLOURS
		fgColor = win_clr[a_back & 0x1F];
    #else
		fgColor = win_clr[a_back & 0x3F];
		//bgColor = PALETTEINDEX(win_clr_bg[a_back & 0x1F]); //verify correctness
		bgColor = win_clr_bg[a_back & 0x3F]; //verify correctness
    #endif
   #endif

		/* Note about the graphics tiles image (stored in hdcTiles):
		   Mask generation has blackened (or whitened if 'inverse') any pixel in it that was actually recognized as eligible mask pixel!
		   For this reason, usage of OR (SRCPAINT) bitblt here is correct as it doesn't collide with the original image's mask-pixels (as these are now black). */

		x1b = ((c_back - MAX_FONT_CHAR - 1) % graphics_image_tpr) * fwid;
		y1b = ((c_back - MAX_FONT_CHAR - 1) / graphics_image_tpr) * fhgt;

		/* Paint background rectangle .*/
		rectBg = (RECT){ 0, 0, fwid, fhgt }; //uhh, C99 ^^'
		rectFg = (RECT){ fwid, 0, 2 * fwid, fhgt };
		brushBg = CreateSolidBrush(bgColor);
		brushFg = CreateSolidBrush(fgColor);
		FillRect(td->hdcTilePreparation2, &rectBg, brushBg);
		FillRect(td->hdcTilePreparation2, &rectFg, brushFg);
		DeleteObject(brushBg);
		DeleteObject(brushFg);


		if (c_back == 32) {
			/* hack: SPACE aka ASCII 32 means empty background ie fill in a_back colour */
		} else {
			BitBlt(td->hdcTilePreparation2, fwid, 0, fwid, fhgt, td->hdcFgMask, x1b, y1b, SRCAND);
			BitBlt(td->hdcTilePreparation2, fwid, 0, fwid, fhgt, td->hdcTiles, x1b, y1b, SRCPAINT);

			BitBlt(td->hdcTilePreparation2, 0, 0, fwid, fhgt, td->hdcBgMask, x1b, y1b, SRCAND);
			BitBlt(td->hdcTilePreparation2, 0, 0, fwid, fhgt, td->hdcTilePreparation2, fwid, 0, SRCPAINT);
		}


		/* --- Merge the foreground tile onto the background tile, using the bg2Mask from the foreground tile --- */

		BitBlt(td->hdcTilePreparation2, 0, 0, fwid, fhgt, td->hdcBg2Mask, x1, y1, SRCAND);
		BitBlt(td->hdcTilePreparation2, 0, 0, fwid, fhgt, td->hdcTilePreparation, fwid, 0, SRCPAINT);


		/* --- Copy the picture from the (bg) tile preparation memory to the window --- */
		BitBlt(hdc, x, y, fwid, fhgt, td->hdcTilePreparation2, 0, 0, SRCCOPY);
	}

  #ifndef OPTIMIZE_DRAWING
	ReleaseDC(td->w, hdc);
  #endif

 #else /* #ifdef USE_GRAPHICS -> no graphics available */
	/* Just erase this grid */
	return(Term_wipe_win(x, y, 1));
 #endif

	/* Success */
	return(0);
}
#endif

/* NOTE: Currently this code has no effect as Windows clients don't have/use a tile cache/TILE_CACHE_SIZE,
   it is merely here to potentially mirror the cache-invalidation-code in main-x11.c for hypothetical future changes.
   c_idx: -1 = invalidate all; otherwise only tiles that use this foreground colour as 'c' are invalidated. */
#if defined(USE_GRAPHICS) && defined(TILE_CACHE_SIZE)
static void invalidate_graphics_cache_win(term_data *td, int c_idx) {
	int i;

	if (disable_tile_cache) return;

	if (c_idx == -1)
		for (i = 0; i < TILE_CACHE_SIZE; i++)
			td->tile_cache[i].is_valid = FALSE;
	else
		for (i = 0; i < TILE_CACHE_SIZE; i++)
			if (td->tile_cache[i].a == c_idx
 #ifdef GRAPHICS_BG_MASK
			    || td->tile_cache[i].a_back == c_idx
 #endif
			    )
				td->tile_cache[i].is_valid = FALSE;
}
#endif

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
	int fwid, fhgt;

	RECT rc;
	HDC  hdc;

#ifdef WIN_GLYPH_FIX
	WORD lpGlyphIndices[n];
#endif

#if 1
	/* For 2mask mode: Actually imprint screen buffer with "empty background" for this text printed grid, to possibly avoid glitches. */
 #ifdef USE_GRAPHICS
  #ifdef GRAPHICS_BG_MASK
	{
		byte *scr_aa_back = Term->scr_back->a[y];
		char32_t *scr_cc_back = Term->scr_back->c[y];

		byte *old_aa_back = Term->old_back->a[y];
		char32_t *old_cc_back = Term->old_back->c[y];

		old_aa_back[x] = scr_aa_back[x] = TERM_DARK;
   #if 0
		old_cc_back[x] = scr_cc_back[x] = 32;
   #else
		old_cc_back[x] = scr_cc_back[x] = Client_setup.f_char[FEAT_SOLID];
   #endif
	}
  #endif
 #endif
#endif

#ifdef USE_LOGFONT
	if (use_logfont) {
		fwid = td->lf.lfWidth;
		fhgt = td->lf.lfHeight;
	} else
#endif
	{
		fwid = td->font_wid;
		fhgt = td->font_hgt;
	}

	/* Location */
	rc.left   = x * fwid + td->size_ow1;
	rc.right  = rc.left + n * fwid;
	rc.top    = y * fhgt + td->size_oh1;
	rc.bottom = rc.top + fhgt;

	/* Acquire DC */
	hdc = myGetDC(td->w);

	/* Catch use in chat instead of as feat attr, or we crash :-s
	   (term-idx 0 is the main window; screen-pad-left check: In case it is used in the status bar for some reason; screen-pad-top check: main screen top chat line) */
	if (Term && Term->data == &data[0] && x >= SCREEN_PAD_LEFT && x < SCREEN_PAD_LEFT + screen_wid && y >= SCREEN_PAD_TOP && y < SCREEN_PAD_TOP + screen_hgt) {
		flick_global_x = x;
		flick_global_y = y;
	} else flick_global_x = 0;

	a = term2attr(a);

#ifdef PALANIM_SWAP
	if (a < CLIENT_PALETTE_SIZE) a = (a + BASE_PALETTE_SIZE) % CLIENT_PALETTE_SIZE;
#endif

#ifdef OPTIMIZE_DRAWING
	if (old_attr != a) {
		/* Foreground color */
		if (colors16) {
			SetTextColor(hdc, PALETTEINDEX(win_pal[a & 0x0F]));
 #ifdef EXTENDED_BG_COLOURS
			//SetBkColor(hdc, PALETTEINDEX(win_pal_bg[a & 0x0F]));  -- would need rgb-value specific manual mapping, not for now
 #endif
		} else {
 #ifndef EXTENDED_COLOURS_PALANIM
  #ifndef EXTENDED_BG_COLOURS
			SetTextColor(hdc, win_clr[a & 0x0F]);
  #else
			SetTextColor(hdc, win_clr[a & 0x1F]);
			SetBkColor(hdc, win_clr_bg[a & 0x1F]); //undefined case actually, we don't want to have a hole in the colour array (0..15 and then 32..32+x) -_-
  #endif
 #else
  #ifndef EXTENDED_BG_COLOURS
			SetTextColor(hdc, win_clr[a & 0x1F]);
  #else
			SetTextColor(hdc, win_clr[a & 0x3F]);
			SetBkColor(hdc, win_clr_bg[a & 0x3F]);
  #endif
 #endif
		}
		old_attr = a;
	}
	/* Dump the text */
 #ifndef WIN_GLYPH_FIX
	ExtTextOut(hdc, rc.left, rc.top, ETO_OPAQUE | ETO_CLIPPED, &rc, s, n, NULL);
 #else
	GetGlyphIndices(hdc, s, n, lpGlyphIndices, 0);
	ExtTextOut(hdc, rc.left, rc.top, ETO_OPAQUE | ETO_CLIPPED | ETO_GLYPH_INDEX, &rc, (LPCSTR)lpGlyphIndices, n, NULL);
 #endif

#else

	/* Background color */
 #ifndef EXTENDED_BG_COLOURS
	SetBkColor(hdc, RGB(0, 0, 0));
 #endif

	/* Foreground color */
	if (colors16) {
		SetTextColor(hdc, PALETTEINDEX(win_pal[a & 0x0F]));
 #ifdef EXTENDED_BG_COLOURS
		//SetBkColor(hdc, PALETTEINDEX(win_pal_bg[a & 0x0F]));  -- would need rgb-value specific manual mapping, not for now
 #endif
	} else {
 #ifndef EXTENDED_COLOURS_PALANIM
  #ifndef EXTENDED_BG_COLOURS
		SetTextColor(hdc, win_clr[a & 0x0F]);
  #else
		SetTextColor(hdc, win_clr[a & 0x1F]); //undefined case actually, we don't want to have a hole in the colour array (0..15 and then 32..32+x) -_-
		//SetBkColor(hdc, PALETTEINDEX(win_clr_bg[a & 0x0F])); //verify correctness
		SetBkColor(hdc, win_clr_bg[a & 0x1F]);
  #endif
 #else
  #ifndef EXTENDED_BG_COLOURS
		SetTextColor(hdc, win_clr[a & 0x1F]);
  #else
		SetTextColor(hdc, win_clr[a & 0x3F]);
		//SetBkColor(hdc, PALETTEINDEX(win_clr_bg[a & 0x1F])); //verify correctness
		SetBkColor(hdc, win_clr_bg[a & 0x3F]);
  #endif
 #endif
	}

	/* Use the font */
	SelectObject(hdc, td->font_id);

	/* Dump the text */
 #ifndef WIN_GLYPH_FIX
	ExtTextOut(hdc, rc.left, rc.top, ETO_OPAQUE | ETO_CLIPPED, &rc, s, n, NULL);
 #else
	GetGlyphIndices(hdc, s, n, lpGlyphIndices, 0);
	ExtTextOut(hdc, rc.left, rc.top, ETO_OPAQUE | ETO_CLIPPED | ETO_GLYPH_INDEX, &rc, (LPCSTR)lpGlyphIndices, n, NULL);
 #endif

	/* Release DC */
	ReleaseDC(td->w, hdc);

#endif

	/* Success */
	return(0);
}

/* Arbitrary sized image to screen printing for graphics mode,
   added for Casino's client-side large dice slots symbol images.
   An <a> 'attr' colour (as there is no 'tile preparation') and tile caching are not applicable (for now).
   These images are just indexed via 'I:' lines in graphical *.prf mapping file instead of <c> 'character' mapping,
   which have their own enumerator independant of any 'character' types.
   Also due to their arbitrary size these images do not use the font tile cache.
    - C. Blue */
static errr Term_rawpict_win(int x, int y, int c) {
#ifdef USE_GRAPHICS
	int fwid, fhgt;

	term_data *td;
	int x1, y1;
	HDC hdc;
#if 0
	COLORREF bgColor, fgColor;

	RECT rectBg;
	RECT rectFg;
	HBRUSH brushBg;
	HBRUSH brushFg;

 #ifdef TILE_CACHE_SIZE
	struct tile_cache_entry *entry;
	int i, hole = -1;
  #ifdef TILE_CACHE_SINGLEBMP
	int tc_idx, tc_x, tc_y;
  #endif
 #endif

	a = term2attr(a);

	bgColor = RGB(0, 0, 0);
	fgColor = RGB(0, 0, 0);

 #ifdef PALANIM_SWAP
	if (a < CLIENT_PALETTE_SIZE) a = (a + BASE_PALETTE_SIZE) % CLIENT_PALETTE_SIZE;
 #endif

	/* Background/Foreground color */
 #ifndef EXTENDED_COLOURS_PALANIM
  #ifndef EXTENDED_BG_COLOURS
	fgColor = win_clr[a & 0x0F];
  #else
	fgColor = win_clr[a & 0x1F];
	//bgColor = PALETTEINDEX(win_clr_bg[a & 0x0F]); //wrong / undefined state, as we don't want to have palette indices 0..15 + 32..32+TERMX_AMT with a hole in between?
	bgColor = win_clr_bg[a & 0x1F]; //wrong / undefined state, as we don't want to have palette indices 0..15 + 32..32+TERMX_AMT with a hole in between?
  #endif
 #else
  #ifndef EXTENDED_BG_COLOURS
	fgColor = win_clr[a & 0x1F];
  #else
	fgColor = win_clr[a & 0x3F];
	//bgColor = PALETTEINDEX(win_clr_bg[a & 0x1F]); //verify correctness
	bgColor = win_clr_bg[a & 0x3F]; //verify correctness
  #endif
 #endif
#endif

	td = (term_data*)(Term->data);

 #ifdef USE_LOGFONT
	if (use_logfont) {
		fwid = td->lf.lfWidth;
		fhgt = td->lf.lfHeight;
	} else
 #endif
	{
		fwid = td->font_wid;
		fhgt = td->font_hgt;
	}

	/* Location of screen window cell */
	x = x * fwid + td->size_ow1;
	y = y * fhgt + td->size_oh1;

	/* Location of graphics tile in the tileset */
	x1 = td->tiles_rawpict[c].x;
	y1 = td->tiles_rawpict[c].y;

	hdc = myGetDC(td->w);

#if 0
 #ifdef TILE_CACHE_SIZE
	if (!disable_tile_cache) {
		entry = NULL;
		for (i = 0; i < TILE_CACHE_SIZE; i++) {
			entry = &td->tile_cache[i];
			if (!entry->is_valid) hole = i;
			else if (entry->c == c && entry->a == a
  #ifdef GRAPHICS_BG_MASK
			    && entry->c_back == 32 && entry->a_back == TERM_DARK
  #endif
  #ifdef TILE_CACHE_FGBG /* Instead of this, invalidate_graphics_cache_...() will specifically invalidate affected entries */
			    /* Extra: Verify that palette is identical - allows palette_animation to work w/o invalidating the whole cache each time: */
			    && fgColor == entry->fg && bgColor == entry->bg
  #endif
			    ) {
				/* Copy cached tile to window. */
  #ifdef TILE_CACHE_SINGLEBMP
				//SelectObject(td->hdcCacheTilePreparation, td->hbmCacheTilePreparation); //already selected?
				BitBlt(hdc, x, y, fwid, fhgt, td->hdcCacheTilePreparation, (i % TILE_CACHE_SINGLEBMP) * 2 * fwid, (i / TILE_CACHE_SINGLEBMP) * fhgt, SRCCOPY);
  #else
				SelectObject(td->hdcTilePreparation, entry->hbmTilePreparation);
				BitBlt(hdc, x, y, fwid, fhgt, td->hdcTilePreparation, 0, 0, SRCCOPY);
  #endif
  #ifndef OPTIMIZE_DRAWING
				ReleaseDC(td->w, hdc);
  #endif
				/* Success */
				return(0);
			}
		}

		// Replace invalid cache entries right away in-place, so we don't kick other still valid entries out via FIFO'ing
		if (hole != -1) {
  #ifdef TILE_CACHE_LOG
			c_msg_format("Tile cache pos (hole): %d / %d", hole, TILE_CACHE_SIZE);
  #endif
  #ifdef TILE_CACHE_SINGLEBMP
			tc_idx = hole;
  #endif
			entry = &td->tile_cache[hole];
		} else {
  #ifdef TILE_CACHE_LOG
			c_msg_format("Tile cache pos (FIFO): %d / %d", td->cache_position, TILE_CACHE_SIZE);
  #endif
			// Replace valid cache entries in FIFO order
  #ifdef TILE_CACHE_SINGLEBMP
			tc_idx = td->cache_position;
  #endif
			entry = &td->tile_cache[td->cache_position++];
			if (td->cache_position >= TILE_CACHE_SIZE) td->cache_position = 0;
		}

  #ifndef TILE_CACHE_SINGLEBMP
		SelectObject(td->hdcTilePreparation, entry->hbmTilePreparation);
  #else
		//SelectObject(td->hdcCacheTilePreparation, td->hbmCacheTilePreparation); //already selected?
  #endif

		entry->c = c;
		entry->a = a;
  #ifdef GRAPHICS_BG_MASK
   #if 0
		entry->c_back = 32;
   #else
		entry->c_back = Client_setup.f_char[FEAT_SOLID];;
   #endif
		entry->a_back = TERM_DARK;
  #endif
		entry->is_valid = TRUE;
  #ifdef TILE_CACHE_FGBG
		entry->fg = fgColor;
		entry->bg = bgColor;
  #endif
	} else SelectObject(td->hdcTilePreparation, td->hbmTilePreparation);
 #else /* (TILE_CACHE_SIZE) No caching: */
	SelectObject(td->hdcTilePreparation, td->hbmTilePreparation);
 #endif

 #if defined(TILE_CACHE_SIZE) && defined(TILE_CACHE_SINGLEBMP)
	if (!disable_tile_cache) {
		tc_x = (tc_idx % TILE_CACHE_SINGLEBMP) * 2 * fwid;
		tc_y = (tc_idx / TILE_CACHE_SINGLEBMP) * fhgt;

		/* Paint background rectangle .*/
		rectBg = (RECT){ tc_x, tc_y, tc_x + fwid, tc_y + fhgt };
		rectFg = (RECT){ tc_x + fwid, tc_y, tc_x + 2 * fwid, tc_y + fhgt };
		brushBg = CreateSolidBrush(bgColor);
		brushFg = CreateSolidBrush(fgColor);
		FillRect(td->hdcCacheTilePreparation, &rectBg, brushBg);
		FillRect(td->hdcCacheTilePreparation, &rectFg, brushFg);
		DeleteObject(brushBg);
		DeleteObject(brushFg);


		//BitBlt(hdc, 0, 0, 2*9, 15, td->hdcCacheTilePreparation, 0, 0, SRCCOPY);

		BitBlt(td->hdcCacheTilePreparation, tc_x + fwid, tc_y, fwid, fhgt, td->hdcFgMask, x1, y1, SRCAND);
		BitBlt(td->hdcCacheTilePreparation, tc_x + fwid, tc_y, fwid, fhgt, td->hdcTiles, x1, y1, SRCPAINT);

		//BitBlt(hdc, 0, 15, 2*9, 15, td->hdcCacheTilePreparation, 0, 0, SRCCOPY);

		BitBlt(td->hdcCacheTilePreparation, tc_x, tc_y, fwid, fhgt, td->hdcBgMask, x1, y1, SRCAND);
		BitBlt(td->hdcCacheTilePreparation, tc_x, tc_y, fwid, fhgt, td->hdcCacheTilePreparation, tc_x + fwid, tc_y, SRCPAINT);

		//BitBlt(hdc, 0, 15, 5*9, 15, td->hdcBgMask, 0, 0, SRCCOPY);
		//BitBlt(hdc, 0, 2*15, 5*9, 15, td->hdcFgMask, 0, 0, SRCCOPY);
		//
		/* Copy the picture from the tile preparation memory to the window */
		BitBlt(hdc, x, y, fwid, fhgt, td->hdcCacheTilePreparation, tc_x, tc_y, SRCCOPY);
	} else
 #endif
	{
		/* Paint background rectangle .*/
		rectBg = (RECT){ 0, 0, fwid, fhgt };
		rectFg = (RECT){ fwid, 0, 2 * fwid, fhgt };
		brushBg = CreateSolidBrush(bgColor);
		brushFg = CreateSolidBrush(fgColor);
		FillRect(td->hdcTilePreparation, &rectBg, brushBg);
		FillRect(td->hdcTilePreparation, &rectFg, brushFg);
		DeleteObject(brushBg);
		DeleteObject(brushFg);


		//BitBlt(hdc, 0, 0, 2*9, 15, td->hdcTilePreparation, 0, 0, SRCCOPY);

		BitBlt(td->hdcTilePreparation, fwid, 0, fwid, fhgt, td->hdcFgMask, x1, y1, SRCAND);
		BitBlt(td->hdcTilePreparation, fwid, 0, fwid, fhgt, td->hdcTiles, x1, y1, SRCPAINT);

		//BitBlt(hdc, 0, 15, 2*9, 15, td->hdcTilePreparation, 0, 0, SRCCOPY);

		BitBlt(td->hdcTilePreparation, 0, 0, fwid, fhgt, td->hdcBgMask, x1, y1, SRCAND);
		BitBlt(td->hdcTilePreparation, 0, 0, fwid, fhgt, td->hdcTilePreparation, fwid, 0, SRCPAINT);

		//BitBlt(hdc, 0, 15, 5*9, 15, td->hdcBgMask, 0, 0, SRCCOPY);
		//BitBlt(hdc, 0, 2*15, 5*9, 15, td->hdcFgMask, 0, 0, SRCCOPY);
		//
		/* Copy the picture from the tile preparation memory to the window */
		BitBlt(hdc, x, y, fwid, fhgt, td->hdcTilePreparation, 0, 0, SRCCOPY);
	}
#endif
	/* Copy the picture from the graphics tiles map image to the window */
	BitBlt(hdc, x, y, td->tiles_rawpict[c].w, td->tiles_rawpict[c].h, td->hdcTiles, x1, y1, SRCCOPY);

 #ifndef OPTIMIZE_DRAWING
	ReleaseDC(td->w, hdc);
 #endif

#else /* #ifdef USE_GRAPHICS */

	/* Just erase this grid */
	//return(Term_wipe_win(x, y, 1));

#endif

	/* Success */
	return(0);
}


/*** Other routines ***/

/*
 * Create and initialize a "term_data" given a title
 */
static void term_data_link(int i, term_data *td) {
	term *t = &td->t;

	/* Initialize the term */
	term_init(t, td->cols, td->rows, td->keys);

	/* Use a "software" cursor */
	t->soft_cursor = TRUE;

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
	t->text_hook = Term_text_win;

	/* Remember where we came from */
	t->data = (vptr)(td);

#ifdef USE_GRAPHICS
	if (use_graphics) {
		recreateGraphicsObjects(td);

		/* Graphics hook */
 #ifdef GRAPHICS_BG_MASK
		if (use_graphics == UG_2MASK) t->pict_hook_2mask = Term_pict_win_2mask;
 #endif
		t->pict_hook = Term_pict_win;
		t->rawpict_hook = Term_rawpict_win;

		/* use "term_pict" for "graphic" data */
		t->higher_pict = true;
	}
#endif
}

#ifdef USE_GRAPHICS
/* Assumes 'use_graphics' is enabled, but may disable it if init fails. */
int init_graphics_win(void) {
	BITMAP bm;
	char filename[1024], subfilename[1024];
	HDC hdc = GetDC(NULL);

	DIR *dir;
	struct dirent *ent;
	char tmp_name[256], *csub, *csub_end;
	int i, j;


	logprint("Initializing graphics.\n");
	if (GetDeviceCaps(hdc, BITSPIXEL) < 24) {
		sprintf(use_graphics_errstr, "Using graphic tiles needs a device content with at least 24 bits per pixel.");
		logprint(format("%s\n", use_graphics_errstr));
 #ifndef GFXERR_FALLBACK
		quit("Graphics device error (W1)");
 #else
		use_graphics = 0;
		use_graphics_err = 1;
		goto gfx_skip;
 #endif
	}
	ReleaseDC(NULL, hdc);

	/* Load graphics file. Quit if file missing or load error. */

	/* Check for tiles string & extract tiles width & height. */
	if (2 != sscanf(graphic_tiles, "%dx%d", &graphics_tile_wid, &graphics_tile_hgt)) {
		sprintf(use_graphics_errstr, "Couldn't extract tile dimensions from: %s", graphic_tiles);
		logprint(format("%s\n", use_graphics_errstr));
 #ifndef GFXERR_FALLBACK
		quit("Graphics load error (W2)");
 #else
		use_graphics = 0;
		use_graphics_err = 2;
		goto gfx_skip;
 #endif
	}

	if (graphics_tile_wid <= 0 || graphics_tile_hgt <= 0) {
		sprintf(use_graphics_errstr, "Invalid tiles dimensions: %dx%d", graphics_tile_wid, graphics_tile_hgt);
		logprint(format("%s\n", use_graphics_errstr));
 #ifndef GFXERR_FALLBACK
		quit("Graphics load error (W3)");
 #else
		use_graphics = 0;
		use_graphics_err = 3;
		goto gfx_skip;
 #endif
	}

	/* Validate the "graphics" directory */
	validate_dir(ANGBAND_DIR_XTRA_GRAPHICS);

	/* Build the name of the graphics file. */
	path_build(filename, 1024, ANGBAND_DIR_XTRA_GRAPHICS, graphic_tiles);
	strcat(filename, ".bmp");

	/* Validate the bitmap filename */
	validate_file(filename);

	/* for graphic_subtiles[]: read all fitting files and accept those with indices that are enabled in our config file (.rc/.ini) */
	if (!(dir = opendir(ANGBAND_DIR_XTRA_GRAPHICS))) {
		c_msg_format("init_graphics_win: Couldn't open tilesets directory (%s).", ANGBAND_DIR_XTRA_GRAPHICS);
		return(FALSE);
	}
	while ((ent = readdir(dir))) {
		strcpy(tmp_name, ent->d_name);

		/* file must end on '.bmp' */
		j = strlen(tmp_name) - 4;
		if (j < 1) continue;
		if ((tolower(tmp_name[j++]) != '.' || tolower(tmp_name[j++] != 'b') || tolower(tmp_name[j++] != 'm') || tolower(tmp_name[j] != 'p'))) continue;

		/* check whether it's a valid subtile file */
		if (!(csub = strchr(tmp_name, '#'))) continue; //valid sub index marker
		*csub = 0;
		if (strcmp(tmp_name, graphic_tiles)) continue; //is same base filename as the selected graphical_tiles file
		if (!(csub_end = strchr(csub + 1, '_'))) continue; //valid sub index terminator
		*csub_end = 0;
		i = atoi(csub + 1);
		if (i < 0 || i >= MAX_SUBFONTS) continue; //valid sub index

		/* accept if enabled */
		graphic_subtiles_file[i][0] = 0;
		if (!graphic_subtiles[i]) continue;
		strcpy(graphic_subtiles_file[i], tmp_name);
	}
	closedir(dir);

	/* Load .bmp image into memory */
 #ifndef CBM_METHOD_DIB
	g_hbmTiles = LoadImageA(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
 #else
	g_hbmTiles = LoadImageA(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION); // see CreateBitmapMask() for LR_CREATEDIBSECTION flag explanation
 #endif
	/* Calculate tiles per row. */
	GetObject(g_hbmTiles, sizeof(BITMAP), &bm);

	/* Ensure the BMP isn't empty or too small */
	if (bm.bmWidth < graphics_tile_wid || bm.bmHeight < graphics_tile_hgt) {
		sprintf(use_graphics_errstr, "Invalid image dimensions (width x height): %ldx%ld", bm.bmWidth, bm.bmHeight);
		logprint(format("%s\n", use_graphics_errstr));
 #ifndef GFXERR_FALLBACK
		quit("Graphics load error (W4)");
 #else
		use_graphics = 0;
		use_graphics_err = 4;
		goto gfx_skip;
 #endif
	}

	graphics_image_tpr = bm.bmWidth / graphics_tile_wid;
	if (graphics_image_tpr <= 0) { /* Paranoia. */
		sprintf(use_graphics_errstr, "Invalid image tiles per row count: %d", graphics_image_tpr);
		logprint(format("%s\n", use_graphics_errstr));
 #ifndef GFXERR_FALLBACK
		quit("Graphics load error (W5)");
 #else
		use_graphics = 0;
		use_graphics_err = 5;
		goto gfx_skip;
 #endif
	}

		/* Create masks. */
 #ifdef GRAPHICS_BG_MASK
	if (use_graphics == UG_2MASK) {
		int err1, err2, err3;

		g_hbmBgMask = CreateBitmapMask(g_hbmTiles, RGB(GFXMASK_BG_R, GFXMASK_BG_G, GFXMASK_BG_B), &err1);
		g_hbmFgMask = CreateBitmapMask(g_hbmTiles, RGB(GFXMASK_FG_R, GFXMASK_FG_G, GFXMASK_FG_B), &err2);
		g_hbmBg2Mask = CreateBitmapMask(g_hbmTiles, RGB(GFXMASK_BG2_R, GFXMASK_BG2_G, GFXMASK_BG2_B), &err3);
		if (!g_hbmBgMask || !g_hbmFgMask || !g_hbmBg2Mask) {
			sprintf(use_graphics_errstr, "Mask creation failed (2) (%d,%d,%d)", err1, err2, err3);
			logprint(format("%s\n", use_graphics_errstr));
 #ifndef GFXERR_FALLBACK
			quit("Graphics load error (W6)");
 #else
			use_graphics = 0;
			use_graphics_err = 6;
			goto gfx_skip;
 #endif
		}
	} else
 #endif
	/* actually always process the bg2mask even if not running 2mask mode,
	   just to change BG2 colours in a 2mask-ready tileset to just black. This ensures tileset "backward compatibility". */
	{
		int err1, err2, err3;

		/* Note the order: First, we set the unused bg2mask (transparency mask), but it'll be to bg instead of zero,
		   via hack inside CreateBitmapMask that recognizes bg2 colour in non-2mask-mode */
		void *res = CreateBitmapMask(g_hbmTiles, RGB(GFXMASK_BG2_R, GFXMASK_BG2_G, GFXMASK_BG2_B), &err1);
		/* so it instead becomes part of the bgmask now. */
		g_hbmBgMask = CreateBitmapMask(g_hbmTiles, RGB(GFXMASK_BG_R, GFXMASK_BG_G, GFXMASK_BG_B), &err2);
		g_hbmFgMask = CreateBitmapMask(g_hbmTiles, RGB(GFXMASK_FG_R, GFXMASK_FG_G, GFXMASK_FG_B), &err3);
		if (!g_hbmBgMask || !g_hbmFgMask || !res) {
			sprintf(use_graphics_errstr, "Mask creation failed (1) (%d,%d,%d)", err1, err2, err3);
			logprint(format("%s\n", use_graphics_errstr));
 #ifndef GFXERR_FALLBACK
			quit("Graphics load error (W7)");
 #else
			use_graphics = 0;
			use_graphics_err = 7;
			goto gfx_skip;
 #endif
		}
	}

gfx_skip:
	if (!use_graphics) {
		logprint(format("Disabling graphics and falling back to normal text mode.\n"));
		/* Actually also show it as 'off' in =g menu, as in, "desired at config-file level" */
		use_graphics_new = FALSE;
	}

	logprint(format("Graphics initialization complete, use_graphics is %d.\n", use_graphics));
	return(use_graphics);
}
#endif

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
static void init_windows(void) {
	int i;
	static char version[20];
	term_data *td;

	logprint("Initializing windows.\n");

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

#ifdef USE_GRAPHICS
 #ifdef TILE_CACHE_SIZE
		//Note: Cache cannot be disabled from ini file at this point yet,
		//      as we load_prefs() further below this, but doesn't matter as it just sets to NULL anyway.
		if (!disable_tile_cache) {
  #ifdef TILE_CACHE_SINGLEBMP
			td->hbmCacheTilePreparation = NULL;
   #ifdef GRAPHICS_BG_MASK
			td->hbmCacheTilePreparation2 = NULL;
   #endif
  #endif
			for (int i = 0; i < TILE_CACHE_SIZE; i++) {
  #ifndef TILE_CACHE_SINGLEBMP
				td->tile_cache[i].hbmTilePreparation = NULL;
  #endif
				td->tile_cache[i].c = 0xffffffff;
				td->tile_cache[i].a = 0xff;
  #ifdef GRAPHICS_BG_MASK
   #ifndef TILE_CACHE_SINGLEBMP
				td->tile_cache[i].hbmTilePreparation2 = NULL;
   #endif
				td->tile_cache[i].c_back = 0xffffffff;
				td->tile_cache[i].a_back = 0xff;
  #endif
				td->tile_cache[i].is_valid = FALSE;
			}
		}
 #endif
#endif /* USE_GRAPHICS */
	}


#if 0 /* This has only been reported on X11 (XFCE4) so far, so 0'ed here for now */
	/* For position restoration: It randomly fails when we use devilspie to undecorate. This small delay
	   might give the window manager et al enough time to handle everything correctly, hopefully. - C. Blue */
	usleep(100000);
#endif

	logprint(format("Loading .ini file '%s'.\n", ini_file ? ini_file : "NULL"));
	/* Load .INI preferences - this will overwrite nick and pass, so we need to keep these if we got them from command-line */
	if (nick[0]) {
		char tmpnick[ACCNAME_LEN], tmppass[PASSWORD_LEN];

		strcpy(tmpnick, nick);
		strcpy(tmppass, pass);

		load_prefs();

		strcpy(nick, tmpnick);
		strcpy(pass, tmppass);
	} else load_prefs();

	/* For '-a', '-g' and '-G' command-line parameters: These override the settings in the prf we just loaded! */
	if (override_graphics != -1) use_graphics_new = use_graphics = override_graphics;

	/* Need these before term_getsize gets called */
	data[0].dwStyle = (WS_OVERLAPPED | WS_THICKFRAME | WS_SYSMENU |
	                   WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CAPTION |
	                   WS_VISIBLE);
	data[0].dwExStyle = 0;

	/* Windows */
	for (i = 1; i < MAX_TERM_DATA; i++) {
		data[i].dwStyle = (WS_OVERLAPPED | WS_THICKFRAME | WS_SYSMENU | WS_CAPTION);
		data[i].dwExStyle = (WS_EX_TOOLWINDOW);
	}

	/* Windows */
	logprint("Initializing termdata font info.\n");
	for (i = 0; i < MAX_TERM_DATA; i++) {
#ifdef USE_LOGFONT
		if (use_logfont) {
			td = &data[i];

			strncpy(td->lf.lfFaceName, td->font_want, LF_FACESIZE);
			/* On first startup w/ logfont, the .INI file doesn't have hgt/wid parms yet.
			   For some reason the hgt parm doesn't matter, but if wid is missing,
			   the main window will assume wid 0 or something similarly wrong and it will be WAY too small horizontally.
			   So here's some paranoia code, defaulting to 9x15 (consistent with DEFAULT_LOGFONTNAME): */
			if (!td->lf.lfWidth) td->lf.lfWidth = td->font_wid;
			else td->font_wid = td->lf.lfWidth;
			if (!td->lf.lfWidth) td->lf.lfWidth = td->font_wid = 9;
			if (!td->lf.lfHeight) td->lf.lfHeight = td->font_hgt;
			else td->font_hgt = td->lf.lfHeight;
			if (!td->lf.lfHeight) td->lf.lfHeight = td->font_hgt = 15;
			// pro-tip: win32 calls corrupting your .INI? flag it read-only!
			td->lf.lfOutPrecision = OUT_DEFAULT_PRECIS; //(OUT_DEFAULT_PRECIS,) OUT_OUTLINE_PRECIS, OUT_TT_PRECIS (slow), OUT_TT_ONLY_PRECIS
 #if 0 /* Now set in load_prefs_aux() instead, so it can be defined in the INI file */
			td->lf.lfQuality = (td->lf.lfHeight >= 9 && td->lf.lfWidth >= 9 && td->lf.lfHeight + td->lf.lfWidth >= 23) ? ANTIALIASED_QUALITY : DEFAULT_QUALITY; //(DEFAULT_QUALITY,) ANTIALIASED_QUALITY, CLEARTYPE_QUALITY (perhaps slow)
 #endif
			td->lf.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
			term_force_font(td, NULL);
		} else
#endif
		{
			if (term_force_font(&data[i], data[i].font_want))
				(void)term_force_font(&data[i], DEFAULT_FONTNAME);
		}
	}

#ifdef USE_GRAPHICS
	/* Handle "graphics" mode */
	if (use_graphics) (void)init_graphics_win();
#endif

	/* Screen window */
	td_ptr = &data[0];
	td_ptr->w = CreateWindowEx(td_ptr->dwExStyle, AppName,
	                           td_ptr->s, td_ptr->dwStyle,
	                           td_ptr->pos_x, td_ptr->pos_y,
	                           td_ptr->size_wid, td_ptr->size_hgt,
	                           HWND_DESKTOP, NULL, hInstance, NULL);
	if (!td_ptr->w) quit("Failed to create TomeNET window");
	td_ptr = NULL;
	term_data_link(0, &data[0]);
	ang_term[0] = &data[0].t;

	/* Windows */
	logprint("Initializing termdata windows.\n");
	for (i = 1; i < MAX_TERM_DATA; i++) {
		td_ptr = &data[i];
		td_ptr->w = CreateWindowEx(td_ptr->dwExStyle, AngList,
		                           td_ptr->s, td_ptr->dwStyle,
		                           td_ptr->pos_x, td_ptr->pos_y,
		                           td_ptr->size_wid, td_ptr->size_hgt,
		                           HWND_DESKTOP, NULL, hInstance, NULL);
		if (!td_ptr->w) quit("Failed to create sub-window");
		if (td_ptr->visible) {
			td_ptr->size_hack = TRUE;
			ShowWindow(td_ptr->w, SW_SHOW);
			td_ptr->size_hack = FALSE;
		}
		td_ptr = NULL;
		term_data_link(i, &data[i]);
		ang_term[i] = &data[i].t;
	}

	/* Activate the screen window */
	SetActiveWindow(data[0].w);

	/* Bring main screen back to top */
	SetWindowPos(data[0].w, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	/* New palette XXX XXX XXX */
	new_palette();

	/* Create a "brush" for drawing the "cursor" */
	hbrYellow = CreateSolidBrush(win_clr[TERM_YELLOW]);

#if defined(USE_GRAPHICS) && defined(TILE_CACHE_SIZE)
	if (use_graphics && !disable_tile_cache) {
		int i;
 #ifndef TILE_CACHE_SINGLEBMP
		int j;
 #endif
		int fwid, fhgt;
		HDC hdc;

		logprint("Initializing graphical tileset cache.\n");

		for (i = 1; i < MAX_TERM_DATA; i++) {
			td = &data[i];
			hdc = GetDC(td->w);

 #ifdef USE_LOGFONT
			if (use_logfont) {
				fwid = td->lf.lfWidth;
				fhgt = td->lf.lfHeight;
			} else
 #endif
			{
				fwid = td->font_wid;
				fhgt = td->font_hgt;
			}

			/* Note: If we want to cache even more graphics for faster drawing, we could initialize 16 copies of the graphics image with all possible mask colours already applied.
			   Memory cost could become "large" quickly though (eg 5MB bitmap -> 80MB). Not a real issue probably. */
 #ifdef TILE_CACHE_SINGLEBMP
			td->hbmCacheTilePreparation = CreateBitmap(TILE_CACHE_SINGLEBMP * 2 * fwid, (TILE_CACHE_SIZE / TILE_CACHE_SINGLEBMP) * fhgt, 1, 32, NULL);
  #ifdef GRAPHICS_BG_MASK
			td->hbmCacheTilePreparation2 = CreateBitmap(TILE_CACHE_SINGLEBMP * 2 * fwid, (TILE_CACHE_SIZE / TILE_CACHE_SINGLEBMP) * fhgt, 1, 32, NULL);
  #endif
 #else
			for (j = 0; j < TILE_CACHE_SIZE; j++) {
				//td->tiles->depth=32
				td->tile_cache[j].hbmTilePreparation = CreateBitmap(2 * fwid, fhgt, 1, 32, NULL);
  #ifdef GRAPHICS_BG_MASK
				//td->tiles->depth=32
				td->tile_cache[j].hbmTilePreparation2 = CreateBitmap(2 * fwid, fhgt, 1, 32, NULL);
  #endif
			}
 #endif

			ReleaseDC(td->w, hdc);
		}
	}
#endif
	logprint("Window initialization completed.\n");

	/* Process pending messages */
	(void)Term_xtra_win_flush();
}


#if 0
/*
 * Hack -- disables new and open from file menu
 */
static void disable_start(void) {
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
static void setup_menus(void) {
	int i;

	HMENU hm = GetMenu(data[0].w);
#ifdef MNU_SUPPORT
	/* Save player */
	EnableMenuItem(hm, IDM_FILE_SAVE, MF_BYCOMMAND | MF_ENABLED | MF_GRAYED);

	/* Exit with save */
	EnableMenuItem(hm, IDM_FILE_EXIT, MF_BYCOMMAND | MF_ENABLED);

	/* Window font options */
	for (i = 1; i < MAX_TERM_DATA; i++) {
		/* Window font */
		EnableMenuItem(hm, IDM_TEXT_TERM_MAIN + i,
		               MF_BYCOMMAND | (data[i].visible ? MF_ENABLED : MF_DISABLED | MF_GRAYED));
	}

	/* Window options */
	for (i = 1; i < MAX_TERM_DATA; i++) {
		/* Window */
		CheckMenuItem(hm, IDM_WINDOWS_TERM_MAIN + i,
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
static void check_for_save_file(LPSTR cmd_line) {
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
			if (!cl_initialized) {
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
			if (!cl_initialized) {
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

		case IDM_TEXT_TERM_MAIN:
 #ifdef USE_GRAPHICS
			/* XXX XXX XXX */
			if (use_graphics) {
				//term_change_bitmap(&data[0]);
				break;
			}
 #endif

			term_change_font(&data[0]);
			break;

		/* Window fonts */
		case IDM_TEXT_TERM_1:
		case IDM_TEXT_TERM_2:
		case IDM_TEXT_TERM_3:
		case IDM_TEXT_TERM_4:
		case IDM_TEXT_TERM_5:
		case IDM_TEXT_TERM_6:
		case IDM_TEXT_TERM_7:
		case IDM_TEXT_TERM_8:
		case IDM_TEXT_TERM_9:
			i = wCmd - IDM_TEXT_TERM_MAIN;

			if ((i < 0) || (i >= MAX_TERM_DATA)) break;

			term_change_font(&data[i]);

			break;

		/* Window visibility */
		case IDM_WINDOWS_TERM_1:
		case IDM_WINDOWS_TERM_2:
		case IDM_WINDOWS_TERM_3:
		case IDM_WINDOWS_TERM_4:
		case IDM_WINDOWS_TERM_5:
		case IDM_WINDOWS_TERM_6:
		case IDM_WINDOWS_TERM_7:
		case IDM_WINDOWS_TERM_8:
		case IDM_WINDOWS_TERM_9:
			i = wCmd - IDM_WINDOWS_TERM_MAIN;

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
			Term_activate(term_term_main);

			/* Reset the visuals */
			//reset_visuals();

			/* Toggle "graphics" */
			/* Hack: Never switch graphics settings, especially UG_2MASK, live,
			   as it will cause instant packet corruption due to missing server-client synchronisation.
			   So we just switch the savegame-affecting 'use_graphics_new' instead of actual 'use_graphics'. */
  #ifdef GRAPHICS_BG_MASK
			use_graphics_new = (use_graphics_new + 1) % 3;
  #else
			use_graphics_new = !use_graphics_new;
  #endif

			/* Access the "graphic" mappings */
			handle_process_font_file();

 #endif		/* GP's USE_GRAPHICS */

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
static void handle_wm_paint(HWND hWnd, term_data *td) {
	int x1, y1, x2, y2;
	PAINTSTRUCT ps;
	int fwid, fhgt;

#ifdef USE_LOGFONT
	if (use_logfont) {
		fwid = td->lf.lfWidth;
		fhgt = td->lf.lfHeight;
	} else
#endif
	{
		fwid = td->font_wid;
		fhgt = td->font_hgt;
	}

	BeginPaint(hWnd, &ps);

	if (td) {
		/* Get the area that should be updated (rounding up/down) */
		x1 = ps.rcPaint.left - td->size_ow1;
		if (x1 < 0) x1 = 0;
		x1 /= fwid;

		y1 = ps.rcPaint.top - td->size_oh1;
		if (y1 < 0) y1 = 0;
		y1 /= fhgt;

		x2 = ((ps.rcPaint.right - td->size_ow1) / fwid) + 1;
		y2 = ((ps.rcPaint.bottom - td->size_oh1) / fhgt) + 1;

		/* Redraw */
		term_data_redraw_section(td, x1, y1, x2, y2);
	}

	EndPaint(hWnd, &ps);
}


#ifdef BEN_HACK
LRESULT FAR PASCAL AngbandWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

LRESULT FAR PASCAL AngbandWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	HDC             hdc;
	term_data      *td;
	MINMAXINFO FAR *lpmmi;
	RECT            rc;
	int             i;

	static int screen_term_cols = -1;//just some dummy vals
	static int screen_term_rows = -1;

	/* Acquire proper "term_data" info */
	td = (term_data *)GetWindowLongPtr(hWnd, 0);
	int fwid, fhgt;

	/* Handle message */
	switch (uMsg) {
		/* XXX XXX XXX */
		case WM_NCCREATE:
			SetWindowLongPtr(hWnd, 0, (LONG_PTR)(td_ptr));
			break;

		/* XXX XXX XXX */
		case WM_CREATE:
			return(0);

		case WM_GETMINMAXINFO:
			lpmmi = (MINMAXINFO FAR *)lParam;

			if (!td) return(1);  /* this message was sent before WM_NCCREATE */

			/* Minimum window size is 8x2 */
#ifdef USE_LOGFONT
			if (use_logfont) {
				fwid = td->lf.lfWidth;
				fhgt = td->lf.lfHeight;
			} else
#endif
			{
				fwid = td->font_wid;
				fhgt = td->font_hgt;
			}
			rc.left = rc.top = 0;
			rc.right = rc.left + 8 * fwid + td->size_ow1 + td->size_ow2;
			rc.bottom = rc.top + 2 * fhgt + td->size_oh1 + td->size_oh2 + 1;

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
#ifdef USE_LOGFONT
			if (use_logfont) {
				fwid = td->lf.lfWidth;
				fhgt = td->lf.lfHeight;
			} else
#endif
			{
				fwid = td->font_wid;
				fhgt = td->font_hgt;
			}
			rc.left = rc.top = 0;
			rc.right = rc.left + (MAX_WINDOW_WID) * fwid + td->size_ow1 + td->size_ow2;
			rc.bottom = rc.top + (MAX_WINDOW_HGT) * fhgt + td->size_oh1 + td->size_oh2;

			/* Paranoia */
			rc.right  += (fwid - 1);
			rc.bottom += (fhgt - 1);

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

			return(0);

		case WM_PAINT:
			handle_wm_paint(hWnd, td);
			return(0);

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			bool enhanced = FALSE;
			bool ms = FALSE;
			bool mc = FALSE;
			bool ma = FALSE;

			/* Extract the modifiers */
			if (GetKeyState(VK_SHIFT)   & 0x8000) ms = TRUE;
			if (GetKeyState(VK_CONTROL) & 0x8000) mc = TRUE;
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

				return(0);
			}

			break;
		}

		case WM_CHAR:
#ifdef ENABLE_SHIFT_SPECIALKEYS
			/* Extract the modifiers */
			if (GetKeyState(VK_SHIFT)   & 0x8000) inkey_shift_special |= 0x1;
			if (GetKeyState(VK_CONTROL) & 0x8000) inkey_shift_special |= 0x2;
			if (GetKeyState(VK_MENU)    & 0x8000) inkey_shift_special |= 0x4;
#endif

			Term_keypress(wParam);
			return(0);

		case WM_INITMENU:
			setup_menus();
			return(0);

		case WM_CLOSE:
		case WM_QUIT:
			quit(NULL);
			return(0);

		case WM_COMMAND:
			process_menus(LOWORD(wParam));
			return(0);

		case WM_SIZE:
			if (!td) return(1);    /* this message was sent before WM_NCCREATE */
			if (!td->w) return(1); /* it was sent from inside CreateWindowEx */
			if (td->size_hack) return(1); /* was sent from WM_SIZE */

			switch (wParam) {
				case SIZE_MINIMIZED:
					for (i = 1; i < MAX_TERM_DATA; i++)
						if (data[i].visible) ShowWindow(data[i].w, SW_HIDE);

					return(0);

				case SIZE_MAXIMIZED:
					/* fall through XXX XXX XXX */

				case SIZE_RESTORED: {
					int rows, cols;
					int old_rows, old_cols;

					td->size_hack = TRUE;

#ifdef USE_LOGFONT
					if (use_logfont) {
						fwid = td->lf.lfWidth;
						fhgt = td->lf.lfHeight;
					} else
#endif
					{
						fwid = td->font_wid;
						fhgt = td->font_hgt;
					}
					cols = (LOWORD(lParam) - td->size_ow1 - td->size_ow2) / fwid;
					rows = (HIWORD(lParam) - td->size_oh1 - td->size_oh2) / fhgt;

					old_cols = cols;
					old_rows = rows;

#if 0 /* wrong place, must be done by WM_EXITSIZEMOVE */
					/* Only restrict main screen window to certain dimensions, other windows are free */
					if (td == &data[0]) {
						/* Hack -- do not allow bad resizing of main screen */
						cols = 80;
						/* respect big_map option */
						if (rows <= 24 || (in_game && !(sflags1 & SFLG1_BIG_MAP))) rows = 24;
						else rows = 46;
					}
#endif

					/* Remember for WM_EXITSIZEMOVE later */
					screen_term_cols = cols;
					screen_term_rows = rows;

					/* Windows */
					for (i = 1; i < MAX_TERM_DATA; i++)
						if (data[i].visible) ShowWindow(data[i].w, SW_SHOWNOACTIVATE);

					td->size_hack = FALSE;

					return(0);
				}
			}
			break;

		/* Note for WINE users: This even will NOT be sent if winecfg->graphics->"Allow window manager to decorate the windows" is enabled, so you must disable it! */
		case WM_EXITSIZEMOVE:
		{
			term *old;

			/* Only restrict main screen window to certain dimensions, other windows are free */
			if (td != &data[0]) break;

			/* main window has constant, fixed width */
			screen_term_cols = 80;

#ifndef GLOBAL_BIG_MAP
			/* Remember final size values from WM_SIZE -> SIZE_RESTORED */
			if ((screen_term_rows != 24 && !Client_setup.options[CO_BIGMAP]) ||
			    (screen_term_rows != 46 && Client_setup.options[CO_BIGMAP])) {
				bool val = !Client_setup.options[CO_BIGMAP];

				Client_setup.options[CO_BIGMAP] = val;
			    c_cfg.big_map = val;
				if (screen_term_rows < 24) {
					val = Client_setup.options[CO_BIGMAP] = FALSE;
				    c_cfg.big_map = FALSE;
				}
#else
			/* Remember final size values from WM_SIZE -> SIZE_RESTORED */
			if ((screen_term_rows != 24 && !global_c_cfg_big_map) ||
			    (screen_term_rows != 46 && global_c_cfg_big_map)) {
				bool val = !global_c_cfg_big_map;

				global_c_cfg_big_map = val;
				if (screen_term_rows < 24) val = global_c_cfg_big_map = FALSE;
#endif

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

			/* apply */
			screen_term_rows = screen_hgt + SCREEN_PAD_Y;

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
			return(0);
		}

		case WM_PALETTECHANGED:
			/* ignore if palette change caused by itself */
			if ((HWND)wParam == hWnd) return(FALSE);
			/* otherwise, fall through!!! */

		case WM_QUERYNEWPALETTE:
			if (!paletted) return(FALSE);
			hdc = GetDC(hWnd);
			SelectPalette(hdc, hPal, FALSE);
			i = RealizePalette(hdc);
			/* if any palette entries changed, repaint the window. */
			if (i) InvalidateRect(hWnd, NULL, TRUE);
			ReleaseDC(hWnd, hdc);
			return(FALSE);

		case WM_ACTIVATE:
			if (wParam && !HIWORD(lParam)) {
				for (i = 1; i < MAX_TERM_DATA; i++)
					SetWindowPos(data[i].w, hWnd, 0, 0, 0, 0,
					    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
				SetFocus(hWnd);
				return(0);
			}
			break;
	}


	/* Oops */
	return(DefWindowProc(hWnd, uMsg, wParam, lParam));
}


#ifdef BEN_HACK
LRESULT FAR PASCAL AngbandListProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

LRESULT FAR PASCAL AngbandListProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	term_data      *td;
	MINMAXINFO FAR *lpmmi;
	RECT            rc;
	HDC             hdc;
	int             i;
	int fwid, fhgt;

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
			return(0);

		case WM_GETMINMAXINFO:
			if (!td) return(1);  /* this message was sent before WM_NCCREATE */

			lpmmi = (MINMAXINFO FAR *)lParam;

			/* Minimum size */
#ifdef USE_LOGFONT
			if (use_logfont) {
				fwid = td->lf.lfWidth;
				fhgt = td->lf.lfHeight;
			} else
#endif
			{
				fwid = td->font_wid;
				fhgt = td->font_hgt;
			}
			rc.left = rc.top = 0;
			rc.right = rc.left + 8 * fwid + td->size_ow1 + td->size_ow2;
			rc.bottom = rc.top + 2 * fhgt + td->size_oh1 + td->size_oh2;

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
#ifdef USE_LOGFONT
			if (use_logfont) {
				fwid = td->lf.lfWidth;
				fhgt = td->lf.lfHeight;
			} else
#endif
			{
				fwid = td->font_wid;
				fhgt = td->font_hgt;
			}
			rc.left = rc.top = 0;
			rc.right = rc.left + (MAX_WINDOW_WID) * fwid + td->size_ow1 + td->size_ow2;
			rc.bottom = rc.top + (MAX_WINDOW_HGT) * fhgt + td->size_oh1 + td->size_oh2;

			/* Paranoia */
			rc.right += (fwid - 1);
			rc.bottom += (fhgt - 1);

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

			return(0);

		case WM_SIZE:
			if (!td) return(1);    /* this message was sent before WM_NCCREATE */
			if (!td->w) return(1); /* it was sent from inside CreateWindowEx */
			if (td->size_hack) return(1); /* was sent from inside WM_SIZE */

			if (wParam == SIZE_RESTORED) {
				term *old;
				int cols, rows;

				td->size_hack = TRUE;

#ifdef USE_LOGFONT
				if (use_logfont) {
					fwid = td->lf.lfWidth;
					fhgt = td->lf.lfHeight;
				} else
#endif
				{
					fwid = td->font_wid;
					fhgt = td->font_hgt;
				}
				cols = (LOWORD(lParam) - td->size_ow1 - td->size_ow2) / fwid;
				rows = (HIWORD(lParam) - td->size_oh1 - td->size_oh2) / fhgt;

				old = Term;
				Term_activate(&td->t);
				Term_resize(cols, rows);
				Term_activate(old);

				/* In case we resized Chat/Msg/Msg+Chat window,
				   refresh contents so they are displayed properly,
				   without having to wait for another incoming message to do it for us. */
				p_ptr->window |= PW_MESSAGE | PW_CHAT | PW_MSGNOCHAT;

				td->size_hack = FALSE;

				return(0);
			}
			break;

		case WM_PAINT:
			handle_wm_paint(hWnd, td);

			return(0);

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:	/* Note: This code is not called. Instead, AngbandWndProc() is called. */
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

				return(0);
			}

			break;
		}

		case WM_CHAR:		/* Note: This code is not called. Instead, AngbandWndProc() is called. */
			Term_keypress(wParam);
			return(0);

		case WM_PALETTECHANGED:
			/* ignore if palette change caused by itself */
			if ((HWND)wParam == hWnd) return(FALSE);
			/* otherwise, fall through!!! */

		case WM_QUERYNEWPALETTE:
			if (!paletted) return(0);
			hdc = GetDC(hWnd);
			SelectPalette(hdc, hPal, FALSE);
			i = RealizePalette(hdc);
			/* if any palette entries changed, repaint the window. */
			if (i) InvalidateRect(hWnd, NULL, TRUE);
			ReleaseDC(hWnd, hdc);
			return(0);

		case WM_NCLBUTTONDOWN:
			if (wParam == HTSYSMENU) {
				/* Hide sub-windows */
				if (td != &data[0]) {
					if (td->visible) {
						td->visible = FALSE;
						ShowWindow(td->w, SW_HIDE);
					}
				}
				return(0);
			}
			break;
	}

	return(DefWindowProc(hWnd, uMsg, wParam, lParam));
}





/*** Temporary Hooks ***/


/*
 * Error message -- See "z-util.c"
 */
static void hack_plog(cptr str) {
	/* Give a warning */
	if (str) MessageBox(NULL, str, "Warning", MB_OK);
}


/*
 * Quit with error message -- See "z-util.c"
 */
static void hack_quit(cptr str) {
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
static void hack_core(cptr str) {
	/* Give a warning */
	if (str) MessageBox(NULL, str, "Error", MB_OK | MB_ICONSTOP);

	/* Quit */
	quit(NULL);
}


/*** Various hooks ***/


/*
 * Error message -- See "z-util.c"
 */
static void hook_plog(cptr str) {
	/* Warning */
	if (str) MessageBox(data[0].w, str, "Warning", MB_OK);
}


void save_term_data_to_term_prefs(void) {
	RECT rc;
	int i;

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

	save_prefs();
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
	char buf[1024];

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

	if (save_chat != 3) {
		/* Copied from quit_hook in c-init.c - mikaelh */
		if (message_num() && (res || (res = get_3way("Save chat log/all messages?", TRUE)))) {
			FILE *fp;
			char buf[80], buf2[1024];
			time_t ct = time(NULL);
			struct tm* ctl = localtime(&ct);

			if (res == 1) strcpy(buf, "tomenet-chat_");
			else strcpy(buf, "tomenet-messages_");
			strcat(buf, format("%04d-%02d-%02d_%02d-%02d-%02d",
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

	/* Remember chat input history across logins */
	/* Only write history if we have at least one line though */
	if (hist_chat_end || hist_chat_looped) {
		FILE *fp;
		path_build(buf, 1024, ANGBAND_DIR_USER, format("chathist-%s.tmp", nick));
		fp = fopen(buf, "w");
		if (!hist_chat_looped) {
			for (i = 0; i < hist_chat_end; i++) {
				if (!message_history_chat[i][0]) continue;
				fprintf(fp, "%s\n", message_history_chat[i]);
			}
		} else {
			for (i = hist_chat_end; i < hist_chat_end + MSG_HISTORY_MAX; i++) {
				if (!message_history_chat[i % MSG_HISTORY_MAX][0]) continue;
				fprintf(fp, "%s\n", message_history_chat[i % MSG_HISTORY_MAX]);
			}
		}
		fclose(fp);
	}

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
#ifdef USE_GRAPHICS
		/* Release the created graphics objects. */
		releaseCreatedGraphicsObjects(&data[i]);
#endif
		if (data[i].font_want) string_free(data[i].font_want);
		if (data[i].w) DestroyWindow(data[i].w);
		data[i].w = 0;
	}
	/* Main window */
	term_force_font(&data[0], NULL);
#ifdef USE_GRAPHICS
		/* Release the created graphics objects. */
		releaseCreatedGraphicsObjects(&data[0]);
#endif
	if (data[0].font_want) string_free(data[0].font_want);
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
#ifdef USE_GRAPHICS
	DeleteObject(g_hbmTiles);
	DeleteObject(g_hbmBgMask);
	DeleteObject(g_hbmFgMask);
#endif

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
void init_stuff(void) {
	int i;
	char path[1024];
	FILE *fp0;

	/* Hack -- access "ANGBAND.INI" */
	GetModuleFileName(hInstance, path, 512);
	argv0 = strdup(path); //save for execv() call
	strcpy(path + strlen(path) - 4, ".ini");

	/* If tomenet.ini file doesn't exist yet, check for tomenet.ini.default
	   and copy it over. This way a full client update won't kill our config. */
	fp0 = fopen(path, "r");
	if (!fp0) {
		char path2[1024];

		logprint(format("No file '%s' found. Trying to use default template.\n", path));
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
				(void)fgets(buf, 1024, fp2);
				if (!feof(fp2)) fputs(buf, fp);
			}
			fclose(fp2);
			fclose(fp);
#endif
			logprint(format("Successfully generated %s.\n", path));
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
	if (i && (path[i - 1] != '\\')) {
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
//#if !defined(USE_LOGFONT) ||
#if defined(USE_GRAPHICS) || defined(USE_SOUND)
	validate_dir(ANGBAND_DIR_XTRA);	/* Sounds & Graphics */
#endif
	validate_dir(ANGBAND_DIR_GAME);

//#ifndef USE_LOGFONT
	// Build the "font" path
	path_build(path, 1024, ANGBAND_DIR_XTRA, "font");

	// Allocate the path
	ANGBAND_DIR_XTRA_FONT = string_make(path);

	// Validate the "font" directory
	validate_dir(ANGBAND_DIR_XTRA_FONT);

	/* If we don't use the windows internal font, ensure that our user-fontfile of choice does exist */
	if (!use_logfont) {
		// Build the filename
		path_build(path, 1024, ANGBAND_DIR_XTRA_FONT, DEFAULT_FONTNAME);

		// Hack -- Validate the basic font
		validate_file(path);
	}
//#endif


#ifdef USE_GRAPHICS
	/* Just build the paths, leave the validation when use_graphics is true */

	/* Build the "graphics" path */
	path_build(path, 1024, ANGBAND_DIR_XTRA, "graphics");

	/* Allocate the path */
	ANGBAND_DIR_XTRA_GRAPHICS = string_make(path);
#endif


#if defined(USE_SOUND) && !defined(USE_SOUND_2010) /* done in snd-sdl.c instead, different sound system */
	/* Build the "sound" path */
	path_build(path, 1024, ANGBAND_DIR_XTRA, "sound");

	/* Allocate the path */
	ANGBAND_DIR_XTRA_SOUND = string_make(path);

	/* Validate the "sound" directory */
	validate_dir(ANGBAND_DIR_XTRA_SOUND);
#endif
}


/*
 * Get a number from command line
 */
static int cmd_get_number(char *str, int *number) {
	int i, tmp = 0;

	/* Skip any spaces */
	for (i = 0; str[i] == ' ' && str[i] != '\0'; i++);

	for (; str[i] != ' ' && str[i] != '\0'; i++) {
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

	return(i);
}

/*
 * Get a string from command line.
 * dest is the destination buffer and n is the size of that buffer.
 */
static int cmd_get_string(char *str, char *dest, int n, bool quoted) {
	int start, end, len;

	/* Skip any spaces */
	for (start = 0; str[start] == ' ' && str[start] != '\0'; start++);

	/* Check for a double quote */
	if (str[start] == '"') {
		start++;
		/* Find another double quote */
		for (end = start; str[end] != '"' && str[end] != '\0'; end++);
	} else if (quoted) {
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

	return(end + 1);
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

int FAR PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
	WNDCLASS wc;
	HDC      hdc;
	MSG      msg;
	WSADATA  wsadata;
	TIMECAPS tc;
	UINT     wTimerRes;

	hInstance = hInst;  /* save in a global var */

	int i, n;
	bool done = FALSE, quoted = FALSE, just_h = FALSE;
	u32b seed;

	char sys_lang_str[5] = { 0 };


	/* Get system UI default language group (last 2 hex digits, the first 2 are actually dialect) to determine whether or not we want CS_IME - C. Blue
	   The reason why we don't want it if it isn't necessary is that it apparently inhibits some hot keys:
	   One player reported he cannot change system language between cyrillic and english anymore via hotkey while TomeNET is focused (only via mouse in that case),
	   another (western) player reported he cannot use ALT+TAB to tab out of TomeNET anymore (!). */
	strcpy(sys_lang_str, format("%04x", GetSystemDefaultUILanguage()));
	sys_lang = strtol(&sys_lang_str[2], NULL, 16);
	/* Automatically suggest enabling IME for CJK systems: */
	switch (sys_lang) {
	case 0x04: // Chinese
	case 0x11: // Japanese
	case 0x12: // Korean
		suggest_IME = TRUE;
		break;
	}

	/* Set the system suffix */
	ANGBAND_SYS = "win";

	/* Get temp path for version-building below */
	init_temp_path();
	/* make version strings. */
	version_build();

	/* Make a copy to use in colour blindness menu when we want to reset palette to default values.
	   This must happen before we read the config file, as it contains colour-(re)definitions. */
	for (i = 0; i < CLIENT_PALETTE_SIZE; i++) client_color_map_org[i] = client_color_map[i];

	/* assume defaults */
	strcpy(cfg_soundpackfolder, "sound");
	strcpy(cfg_musicpackfolder, "music");

	/* set OS-specific resize_main_window() hook */
	resize_main_window = resize_main_window_win;

	/* Prepare the filepaths */
	init_stuff();
	/* Load IME settings before commandline-args (to allow -I to modify it) and before window initialization (or it won't work) */
	load_prefs_IME();
#ifdef USE_LOGFONT
	/* Load logfont settings before commandline-args (to allow -L to modify it) and before window initialization (or it won't work) */
	load_prefs_logfont();
#endif
#ifdef TILE_CACHE_SIZE
	load_prefs_TILE_CACHE();
#endif

	/* Process the command line -- now before initializing the main window because of CS_IME setting via cmdline arg 'I'/'i':*/
	for (i = 0, n = strlen(lpCmdLine); i < n; i++) {
		/* Check for an option */
		if (lpCmdLine[i] == '-') {
			i++;

			switch (lpCmdLine[i]) {
			case 'C': /* compatibility mode */
				server_protocol = 1; break;
			case 'F':
				i += cmd_get_number(&lpCmdLine[i + 1], (int*)&cfg_client_fps);
				break;
			case 'h':
				/* Attempt to print out some usage information */
				if (initialized) /* We're called AFTER init_windows()? Then we'll appear in a graphical windows message window, with annoying formatting :p */
					/* Message box on Windows has a default limit of characters, we need to cut out the 4
					   commented-out lines or it won't fit. Adding empty lines however seems to be no problem. */
					plog(format("%s\nRunning on %s.\n\n%s\n%s\n%s\n\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s",
					    longVersion,
					    os_version,
					    //"Usage:    tomenet [options] [server]",
					    "Example: tomenet -lMorgoth MorgyPass",
					    "                              -p18348 europe.tomenet.eu",
					    /* "  -h              Display this help",
					    "  -C              Compatibility mode for OLD servers",
					    "  -F              Client FPS", */
					    "  -I              force IME off (for CJK languages)",
					    "  -i               force IME on (for CJK languages)",
#ifdef USE_LOGFONT
					    "  -L              Use LOGFONT (Windows-internal font)",
#endif
					    "  -l<name> <pwd>       Login crecedentials",
					    "  -N<name>       character name",
					    "  -R<name>       char name, auto-reincarnate",
					    "  -p<num>         change game Port number",
					    "  -P<path>        set the lib directory Path",
					    "  -k              don't disable numlock on startup",
					    "  -m             skip message of the day window",
					    "  -q              disable all audio ('quiet mode')",
					    /* "  -u              disable automatic lua updates", */
					    "  -w             disable client-side weather effects",
					    "  -v              save chat log on exit",
					    "  -V              save chat+message log on exit",
					    "  -x              don't save chat/message log on exit",
					    "  -a/-g/-G       switch to ASCII/gfx/dualgfx mode"));
				else /* We're called BEFORE init_windows()? Then we'll appear in the terminal window, with normal fixed-width formatting */
					plog(format("%s\nRunning on %s.\n\n%s\n%s\n\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s",
					    longVersion,
					    os_version,
					    "Usage:    tomenet [options] [server]",
					    "Example:  tomenet -lMorgoth MorgyPass -p18348 europe.tomenet.eu",
					    /* "  -h              Display this help",
					    "  -C              Compatibility mode for OLD servers",
					    "  -F              Client FPS", */
					    "  -I              force IME off (for CJK languages)",
					    "  -i              force IME on (for CJK languages)",
#ifdef USE_LOGFONT
					    "  -L              Use LOGFONT (Windows-internal font)",
#endif
					    "  -l<name> <pwd>  Login crecedentials",
					    "  -N<name>        character name",
					    "  -R<name>        char name, auto-reincarnate",
					    "  -p<num>         change game Port number",
					    "  -P<path>        set the lib directory Path",
					    "  -k              don't disable numlock on startup",
					    "  -m              skip message of the day window",
					    "  -q              disable all audio ('quiet mode')",
					    // "  -u              disable automatic lua updates",
					    "  -w              disable client-side weather effects",
					    "  -v              save chat log on exit",
					    "  -V              save chat+message log on exit",
					    "  -x              don't save chat/message log on exit",
					    "  -a/-g/-G        switch to ASCII/gfx/dualgfx mode"));
				if (initialized) quit(NULL);
				just_h = TRUE;
				break;
			case 'I': disable_CS_IME = TRUE; break;
			case 'i': enable_CS_IME = TRUE; break;
#ifdef USE_LOGFONT
			case 'L': use_logfont = TRUE; break;
#endif
			case 'l': /* account name & password */
				i += cmd_get_string(&lpCmdLine[i + 1], nick, MAX_CHARS, quoted);
				i += cmd_get_string(&lpCmdLine[i + 1], pass, MAX_CHARS, FALSE);
				done = TRUE;
				break;
			case 'R':
				auto_reincarnation = TRUE;
			case 'N': /* character name */
				i += cmd_get_string(&lpCmdLine[i + 1], cname, MAX_CHARS, quoted);
				break;
			case 'p': /* port */
				i += cmd_get_number(&lpCmdLine[i + 1], (int*)&cfg_game_port);
				break;
			case 'P': /* lib directory path */
				i += cmd_get_string(&lpCmdLine[i + 1], path, 1024, quoted);
				break;
			case 'q': quiet_mode = TRUE; break;
			case 'w': noweather_mode = TRUE; break;
			case 'u': no_lua_updates = TRUE; break;
			case 'k': disable_numlock = FALSE; break;
			case 'm': skip_motd = TRUE; break;
			case 'v': save_chat = 1; break;
			case 'V': save_chat = 2; break;
			case 'x': save_chat = 3; break;
			case 'a': override_graphics = UG_NONE; ask_for_graphics = FALSE; break; // ASCII
			case 'g': override_graphics = UG_NORMAL; ask_for_graphics = FALSE; break; // graphics
			case 'G': override_graphics = UG_2MASK; ask_for_graphics = FALSE; break; // dual-mask graphics
#ifdef TILE_CACHE_SIZE
			case 'T': disable_tile_cache = TRUE; break;
#endif
			}
			quoted = FALSE;
		} else if (lpCmdLine[i] == ' ') {
			/* Ignore spaces */
		} else if (lpCmdLine[i] == '"') {
			/* For handling via Wine: It automatically adds spaces when an argument contains an escape-sequence such as '\ ' for a space for example */
			quoted = !quoted;
		} else {
			/* Get a server name */
			i += cmd_get_string(&lpCmdLine[i], svname, MAX_CHARS, quoted);
			quoted = FALSE;
		}
	}

	if (disable_tile_cache) logprint("Graphics tiles cache disabled.\n");

	if (hPrevInst == NULL) {
/* Not required, just a paranoia note, it's pretty undocumented */
//#define CS_IME 0x00010000
		// Apply CS_IME perhaps: force-off has highest priority, followed by force-on, last is auto-suggest via detected system language
		wc.style         = CS_CLASSDC | (disable_CS_IME ? 0x0 : (enable_CS_IME ? CS_IME : (suggest_IME ? CS_IME : 0x0)));
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

	/* Show commandline options as message box too, not just in the terminal */
	if (just_h) {
		plog(format("%s\nRunning on %s.\n\n%s\n%s\n%s\n\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s",
		    longVersion,
		    os_version,
		    "Usage:    tomenet [options] [server]",
		    "Example: tomenet -lMorgoth MorgyPass",
		    "                              -p18348 europe.tomenet.eu",
		    /* "  -h              Display this help",
		    "  -C              Compatibility mode for OLD servers",
		    "  -F              Client FPS", */
		    "  -I              force IME off (for CJK languages)",
		    "  -i               force IME on (for CJK languages)",
 #ifdef USE_LOGFONT
		    "  -L              Use LOGFONT (Windows-internal font)",
 #endif
		    "  -l<name> <pwd>       Login crecedentials",
		    "  -N<name>       character name",
		    "  -R<name>       char name, auto-reincarnate",
		    "  -p<num>         change game Port number",
		    "  -P<path>        set the lib directory Path",
		    "  -k              don't disable numlock on startup",
		    "  -m             skip message of the day window",
		    "  -q              disable all audio ('quiet mode')",
		    /* "  -u              disable automatic lua updates", */
		    "  -w             disable client-side weather effects",
		    "  -v              save chat log on exit",
		    "  -V              save chat+message log on exit",
		    "  -x              don't save chat/message log on exit"));
		quit(NULL);
	}

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

	/* As this spawns an ugly shell window on Windows, do it here before we even init the windows */
	(void)check_guide_checksums(FALSE);

	/* Prepare the windows */
	init_windows();
#if 1
	/* Check after ini has been loaded, so we know the bigmap_hint state and can use it to conclude if this is a first-run or not. */
	if (!bigmap_hint || c_cfg.big_map) ask_for_graphics = FALSE;
#endif

	/* Activate hooks */
	plog_aux = hook_plog;
	quit_aux = hook_quit;
	core_aux = hook_quit;

	/* We are now initialized */
	initialized = TRUE;
	fullscreen_weather = FALSE;

#if 0 /* apparently this wasn't so good after all - mikaelh */
	/* Check if we loaded some account name & password from the .ini file - mikaelh */
	if (strlen(nick) && strcmp(nick, "PLAYER") && strlen(pass) && strcmp(pass, "passwd"))
		done = TRUE;
#endif

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
	return(0);
}


/* EXPERIMENTAL // allow user to change main window font live - C. Blue
 * So far only uses 1 parm ('s') to switch between hardcoded choices:
 * -1 - cycle
 *  0 - normal
 *  1 - big
 *  2 - bigger
 *  3 - huge */
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
		return(TRUE);
	return(FALSE);
#else
	return(ask_for_bigmap_generic());
#endif
}

const char* get_font_name(int term_idx) {
	if (data[term_idx].font_file) return(data[term_idx].font_file);
#ifdef USE_LOGFONT
	else if (use_logfont) {
		static char logfont_name[MAX_CHARS];

		sprintf(logfont_name, "<LOGFONT>%dx%d", win_get_logfont_w(term_idx), win_get_logfont_h(term_idx));
		return(logfont_name);
	}
#endif
	else //return(DEFAULT_FONTNAME);
		return("<no font>");
}
void set_font_name(int term_idx, char* fnt) {
	char fnt2[256], *fnt_ptr = fnt;

	while (strchr(fnt_ptr, '\\')) fnt_ptr = strchr(fnt_ptr, '\\') + 1;
	strcpy(fnt2, fnt_ptr);
	term_force_font(&data[term_idx], fnt2);
}
#ifdef USE_LOGFONT
int win_get_logfont_w(int term_idx) {
	return(data[term_idx].lf.lfWidth);
}
int win_get_logfont_h(int term_idx) {
	return(data[term_idx].lf.lfHeight);
}
void win_logfont_inc(int term_idx, bool wh) {
	term_data *td = &data[term_idx];

	if (wh) {
		/* enforce sane dimensions */
		if (td->lf.lfHeight >= 128) return;
		td->lf.lfHeight++;
	} else {
		/* enforce sane dimensions */
		if (td->lf.lfWidth >= 128) return;
		td->lf.lfWidth++;
	}
	term_force_font(td, NULL);
 #ifdef USE_GRAPHICS
	if (use_graphics && g_hbmTiles != NULL) recreateGraphicsObjects(td);
 #endif
}
void win_logfont_dec(int term_idx, bool wh) {
	term_data *td = &data[term_idx];

	if (wh) {
		/* enforce sane dimensions */
		if (td->lf.lfHeight <= 5) return;
		td->lf.lfHeight--;
	} else {
		/* enforce sane dimensions */
		if (td->lf.lfWidth <= 5) return;
		td->lf.lfWidth--;
	}
	term_force_font(td, NULL);
 #ifdef USE_GRAPHICS
	if (use_graphics && g_hbmTiles != NULL) recreateGraphicsObjects(td);
 #endif
}
void win_logfont_set(int term_idx, char *sizestr) {
	term_data *td = &data[term_idx];
	int w = 9, h = 15;
	char *c;

	w = atoi(sizestr);
	c = strchr(sizestr, 'x');
	if (!c) {
		c_msg_print("\377yPlease enter two numbers for width and height, separated by an 'x' in between.");
		return;
	}
	h = atoi(c + 1);

	/* enforce sane dimensions */
	if (w > 128 || h > 128 || w < 5 || h < 5) return;

	td->lf.lfHeight = h;
	td->lf.lfWidth = w;

	term_force_font(td, NULL);
 #ifdef USE_GRAPHICS
	if (use_graphics && g_hbmTiles != NULL) recreateGraphicsObjects(td);
 #endif
}
bool win_logfont_get_aa(int term_idx) {
	term_data *td = &data[term_idx];

	return(td->lf.lfQuality == ANTIALIASED_QUALITY);
}
/* We want to change the antialiasing level (none aka default or actually antialiased). Can look blurry with small fonts. */
void win_logfont_set_aa(int term_idx, bool antialiased) {
	term_data *td = &data[term_idx];

	td->lf.lfQuality = antialiased ? ANTIALIASED_QUALITY : DEFAULT_QUALITY;
	term_force_font(td, NULL);
}
#endif
void term_toggle_visibility(int term_idx) {
	data[term_idx].visible = !data[term_idx].visible;
}
bool term_get_visibility(int term_idx) {
	return(data[term_idx].visible);
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
void get_term_main_font_name(char *buf) {
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

	term *old;
	term_data *td;


	/* Initialise the palette once. For some reason colour_table[] is all zero'ed again at the beginning. */
	if (!init) {
#ifndef EXTENDED_COLOURS_PALANIM
		for (i = 0; i < BASE_PALETTE_SIZE; i++) {
#else
 #ifdef PALANIM_SWAP
		for (i = 0; i < BASE_PALETTE_SIZE; i++) {
 #else
		for (i = 0; i < CLIENT_PALETTE_SIZE; i++) {
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
			color_table[i][0] = win_pal[i % BASE_PALETTE_SIZE];
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
		for (i = 0; i < BASE_PALETTE_SIZE; i++) {
#else
 #ifdef PALANIM_SWAP
		for (i = 0; i < BASE_PALETTE_SIZE; i++) {
 #else
		for (i = 0; i < CLIENT_PALETTE_SIZE; i++) {
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
		old = Term;
		td = &data[i];

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

#ifdef PALANIM_OPTIMIZED
	/* Check for refresh marker at the end of a palette data transmission */
	if (c == 127 || c == 128) {
		term_data *td;
 #ifdef PALANIM_OPTIMIZED2
		int i;

		/* Batch-apply all colour changes */
  #ifndef EXTENDED_COLOURS_PALANIM
		for (i = 0; i < BASE_PALETTE_SIZE; i++)
  #else
   #ifdef PALANIM_SWAP
		for (i = 0; i < BASE_PALETTE_SIZE; i++)
   #else
		for (i = 0; i < CLIENT_PALETTE_SIZE; i++)
   #endif
  #endif
			win_clr[i] = win_clr_buf[i];
 #endif
 #ifdef EXTENDED_BG_COLOURS
			//todo:implement   win_clr[CLIENT_PALETTE_SIZE] = win_clr_buf[CLIENT_PALETTE_SIZE];
 #endif

		/* Activate the palette */
 #ifdef PALANIM_SWAP
		new_palette_ps();
 #else
		new_palette();
 #endif

		/* Refresh aka redraw main window with new colour */
		td = &data[0];
		/* Activate */
		Term_activate(&td->t);
		/* Invalidate cache to ensure redrawal doesn't get cancelled by tile-caching */
 #if defined(USE_GRAPHICS) && defined(TILE_CACHE_SIZE)
  #if !defined(TILE_CACHE_NEWPAL) && !defined(TILE_CACHE_FGBG)
		/* Last resort: Invalidate whole cache as we didn't keep track of which colours' palettes have been modified. */
		invalidate_graphics_cache_win(td, -1);
  #endif
 #endif
		/* Redraw the contents */
//WiP, not functional		if (screen_icky) Term_switch_fully(0);
		if (c_cfg.gfx_palanim_repaint || (c_cfg.gfx_hack_repaint && !gfx_palanim_repaint_hack))
			/* Alternative function for flicker-free redraw - C. Blue */
			//Term_repaint(SCREEN_PAD_LEFT, SCREEN_PAD_TOP, screen_wid, screen_hgt);
			/* Include the UI elements, which is required if we use ANIM_FULL_PALETTE_FLASH or ANIM_FULL_PALETTE_LIGHTNING  - C. Blue */
			Term_repaint(0, 0, CL_WINDOW_WID, CL_WINDOW_HGT);
		else {
			Term_redraw();
			gfx_palanim_repaint_hack = FALSE;
		}
//WiP, not functional:		if (screen_icky) Term_switch_fully(0);
		/* Restore */
		Term_activate(term_old);
		return;
	}
#else
	if (c == 127 || c == 128) return; //just discard refresh marker
#endif

#ifdef EXTENDED_BG_COLOURS
	/* For now don't allow palette animation of extended-bg colours */
	if (c >= CLIENT_PALETTE_SIZE) return;
#endif

#ifdef PALANIM_SWAP
	if (c < CLIENT_PALETTE_SIZE) c = (c + BASE_PALETTE_SIZE) % CLIENT_PALETTE_SIZE;
#endif

	/* Need complex color mode for palette animation */
	if (colors16) {
		c_msg_print("\377yPalette animation failed! Disable it in = 2 'palette_animation'!");
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
	td = &data[0];
	/* Activate */
	Term_activate(&td->t);

 #if defined(USE_GRAPHICS) && defined(TILE_CACHE_SIZE)
  #ifdef TILE_CACHE_NEWPAL
	/* Invalidate cache using this particular colour to ensure redrawal doesn't get cancelled by tile-caching */
	invalidate_graphics_cache_win(td, c);
  #elif !defined(TILE_CACHE_FGBG)
	/* Last resort: Invalidate whole cache as we didn't keep track of which colours' palettes have been modified. */
	invalidate_graphics_cache_win(td, -1);
  #endif
 #endif
	/* Redraw the contents */
	Term_xtra(TERM_XTRA_FRESH, 0); /* Flickering occasionally on Windows :( */
	/* Restore */
	Term_activate(term_old);
#else
 #if defined(USE_GRAPHICS) && defined(TILE_CACHE_SIZE) && defined(TILE_CACHE_NEWPAL)
	/* Invalidate cache using this particular colour to ensure redrawal doesn't get cancelled by tile-caching */
	invalidate_graphics_cache_win(&data[0], c);
 #endif
#endif
}
void get_palette(byte c, byte *r, byte *g, byte *b) {
	COLORREF cref = win_clr[c];

	*r = GetRValue(cref);
	*g = GetGValue(cref);
	*b = GetBValue(cref);
}
void refresh_palette(void) {
	int i;
	term *term_old = Term;

	set_palette(128, 0, 0, 0);

	/* Refresh aka redraw windows with new colour (term 0 is already done in set_palette(128) line above) */
#ifdef MAX_TERM_DATA
	for (i = 1; i < MAX_TERM_DATA; i++) {
#else
	for (i = 1; i < 8; i++) { /* MAX_TERM_DATA should be defined for X11 too.. */
#endif
		term_data *td = &data[i];

		if (!td->visible) continue;
		if (td->pos_x == -32000 || td->pos_y == -32000) continue;

		Term_activate(&td->t);
		Term_redraw();
		//Term_xtra(TERM_XTRA_FRESH, 0);
	}

	Term_activate(term_old);
}

void set_window_title_win(int term_idx, cptr title) {
	term_data *td;

	/* The 'term_idx_to_term_data()' returns '&term_main' if 'term_idx' is out of bounds and it is not desired to resize screen terminal window in that case, so validate before. */
	if (term_idx < 0 || term_idx >= ANGBAND_TERM_MAX) return;
	td = &data[term_idx];

	/* Trying to change title in this state causes a crash? */
	if (!td->visible) return;
	if (td->pos_x == -32000 || td->pos_y == -32000) return;

	SetWindowTextA(td->w, title);
}

#endif /* _Windows */
