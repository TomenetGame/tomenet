#ifdef USE_SDL3
/* main-sdl3.c
 *
 * SDL3 bindings implementation for the TomeNET game project.
 * Handles window creation, event handling, keyboard input, and graphics/text output using SDL3.
 *
 * Note: Requires SDL3, SDL3_ttf and SDL3_net libraries.
 * Screenshot saving uses SDL3_image for PNG when available; otherwise BMP is used.
 */

#include "angband.h"
#include "graphics_common.h"

#include "../common/z-util.h"
#include "../common/z-virt.h"
#include "../common/z-form.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_net/SDL_net.h>
#ifdef SDL3_IMAGE
 #include <SDL3_image/SDL_image.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <sys/types.h>
#ifdef REGEX_SEARCH
 #include <regex.h>
#endif
#include <dirent.h>

/**** Available Types ****/

/*
 * Notes on Colors:
 *
 *   1) On a monochrome (or "fake-monochrome") display, all colors
 *   will be "cast" to "fg," except for the bg color, which is,
 *   obviously, cast to "bg".
 *
 *   2) Because of the inner functioning of the color allocation
 *   routines, colors may be specified as (a) a typical color name,
 *   (b) a hexadecimal color specification (preceded by a pound sign),
 *   or (c) by strings such as "fg", "bg".
 */

/* Pixel is just shorthand for SDL_Color. */
typedef SDL_Color Pixel;

/* The structures defined below */
typedef struct infowin infowin;
typedef struct infoclr infoclr;
typedef struct infofnt infofnt;

/*
 * A Structure summarizing Window Information.
 *
 *	- The window.
 *	- The drawing surface for the window.
 *
 *	- The window identifier.
 *	- Just used for padding.
 *
 *	- The color of border and background.
 *
 *	- The location of the window.
 *	- The width, height of the window with borders. If window is not maximized, the width/height should always be width/height of drawing box plus two times border width/heiht.
 *	- The width, height of the drawing box of the window. It is calculated as cols/rows of terminal times font width/height.
 *	- The border width of this window (vertical and horizontal).
 *
 *	- Bit Flag: We should nuke 'window' when done with it
 *	- Bit Flag: This window is currently shown
 *	- Bit Flag: 1st extra flag
 *	- Bit Flag: 2nd extra flag
 *
 *	- Bit Flag: 3rd extra flag
 *	- Bit Flag: 4th extra flag
 *	- Bit Flag: 5th extra flag
 *	- Bit Flag: 6th extra flag
 */
struct infowin {
	SDL_Window		*window;
	SDL_Surface	*surface;

	uint32_t	windowID;

	Pixel		b_color, bg_color;

	int16_t		x, y;
	uint16_t		wb, hb;
	uint16_t		wd, hd;
	uint16_t		bw, bh;

	uint32_t		nuke:1;
	uint32_t		shown:1;
};

/*
 * A Structure summarizing Operation+Color Information
 *
 *	- The Foreground Pixel Value
 *	- The Background Pixel Value
 *
 */
struct infoclr {
	Pixel		fg;
	Pixel		bg;
};

/*
 * Used font types.
 */
typedef enum {
	FONT_TYPE_TTF,
	FONT_TYPE_PCF
} FontType;

/*
 * A Structure to Hold Font Information
 *
 *	- The 'TTF_Font*' or 'PCF_Font*' (points to the font structure)
 *	- The font's name (name of the PCF or TTF font, the TTF name ends with ".ttf" and is followed by (a space character and ) a number, representing the size)
 *	- The font type (`FONT_TYPE_TTF`, or `FONT_TYPE_PCF`)
 *
 *	- The default character width
 *	- The default character height
 *
 *	- Flag: Force monospacing via 'wid'
 *	- Flag: Nuke font when done
 */

struct infofnt {
	void			*font;
	cptr			name;
	FontType	type;


	float		scale;
	s16b			wid;
	s16b			hgt;

	uint			emulateMonospace:1;
	uint			nuke:1;
};

/**** Available Macros ****/

#if 0
 #define SDL3_TILESET_CACHE_DEBUG
#endif

#define SDL3_DEFAULT_BORDER_WIDTH 1

/* Pixel helper macros */
#define Pixel_equal(c1, c2) memcmp(&(c1), &(c2), sizeof(Pixel)) == 0
#define Pixel_quadruplet(color) color.r, color.g, color.b, color.a
#define Pixel_triplet(color) color.r, color.g, color.b

/* Set the current Infowin */
#define Infowin_set(I) \
	(Infowin = (I))

/* Set the current Infoclr */
#define Infoclr_set(C) \
	(Infoclr = (C))

#define Infoclr_init_parse(F, B) \
	Infoclr_init_data(Infoclr_Pixel(F), Infoclr_Pixel(B))

/* Set the current infofnt */
#define Infofnt_set(I) \
	(Infofnt = (I))

/*
 * Default color values.
 * Colors can be redefined during init.
 */
Pixel color_default_bg = (Pixel){0x00, 0x00, 0x00, 0xFF};
Pixel color_default_fg = (Pixel){0xFF, 0xFF, 0xFF, 0xFF};
Pixel color_default_b  = (Pixel){0xFF, 0xFF, 0xFF, 0xFF};
Pixel cursor_color = (Pixel){0xFF, 0xFF, 0xFF, 0x80};

/*
 * The "current" variables
 */
static infowin *Infowin = (infowin*)(NULL);
static infoclr *Infoclr = (infoclr*)(NULL);
static infofnt *Infofnt = (infofnt*)(NULL);


/*
 * Set the name (in the title bar) of Infowin
 */
static errr Infowin_set_name(cptr name) {
	if (!Infowin || !Infowin->window) return(1);
	SDL_SetWindowTitle(Infowin->window, name ? name : "");

	return(0);
}

/*
 * Request that Infowin be raised
 */
static errr Infowin_raise(void) {
	/* Raise towards visibility */
	SDL_RaiseWindow(Infowin->window);

	/* Success */
	return(0);
}

/*
 * Move an infowin.
 */
static errr Infowin_move(int x, int y) {
	/* Execute the request */
	if (!SDL_SetWindowPosition(Infowin->window, x, y)) return(1);

	/* Store new client coordinates. */
	Infowin->x = x;
	Infowin->y = y;

	/* Success */
	return(0);
}

/*
 * Resize an infowin
 */
static errr Infowin_resize(int w, int h) {
	/* Execute the request */
	if (!SDL_SetWindowSize(Infowin->window, w, h)) {
		fprintf(stderr, "Error: Cant resize window #%d to %dx%d due to: %s\n", Infowin->windowID, w, h, SDL_GetError());
		return(1);
	}

	/* Store new client dimensions. */
	Infowin->wb = w;
	Infowin->hb = h;

	/* Success */
	return(0);
}

/* Draw borders, clipping all rectangles to the actual client area. */
static void draw_borders(infowin *win, Pixel color) {
	if (!win || !win->surface || win->wb <= 0 || win->hb <= 0) return;

	uint32_t bc;

#define SDL3_FILL_BORDERS
#ifdef SDL3_FILL_BORDERS
	int top_h = win->bh < win->hb ? win->bh : win->hb;
	int bottom_y = win->bh + win->hd;
	int left_h = win->hb - win->bh > win->hd ? win->hd : win->hb - win->bh;
	int right_x = win->bw + win->wd;

	bc = SDL_MapRGBA(SDL_GetPixelFormatDetails(win->surface->format), NULL, 25, 25, 25, 0);
	if (top_h > 0) SDL_FillSurfaceRect(win->surface, &(SDL_Rect){0, 0, win->wb, top_h}, bc);
	if (bottom_y < win->hb) SDL_FillSurfaceRect(win->surface, &(SDL_Rect){0, bottom_y, win->wb, win->hb - bottom_y}, bc);
	if (win->bw > 0 && left_h > 0) SDL_FillSurfaceRect(win->surface, &(SDL_Rect){0, win->bh, win->bw, left_h}, bc);
	if (right_x < win->wb && left_h > 0) SDL_FillSurfaceRect(win->surface, &(SDL_Rect){right_x, win->bh, win->wb - right_x, left_h}, bc);
#endif

	bc = SDL_MapRGBA(SDL_GetPixelFormatDetails(win->surface->format), NULL, Pixel_quadruplet(color));
	if (win->bh > 0) {
		SDL_FillSurfaceRect(win->surface, &(SDL_Rect){0, 0, win->wb, SDL3_DEFAULT_BORDER_WIDTH}, bc);
		SDL_FillSurfaceRect(win->surface, &(SDL_Rect){0, win->hb - SDL3_DEFAULT_BORDER_WIDTH, win->wb, SDL3_DEFAULT_BORDER_WIDTH}, bc);
	}
	if (win->bw > 0) {
		SDL_FillSurfaceRect(win->surface, &(SDL_Rect){0, 0, SDL3_DEFAULT_BORDER_WIDTH, win->hb}, bc);
		SDL_FillSurfaceRect(win->surface, &(SDL_Rect){win->wb - SDL3_DEFAULT_BORDER_WIDTH, 0, SDL3_DEFAULT_BORDER_WIDTH, win->hb}, bc);
	}
}

/*
 * Visually clear Infowin
 */
static errr Infowin_wipe(void) {
	if (!Infowin || !Infowin->surface) return(1);

	/* Wipe surface. */
	SDL_FillSurfaceRect(Infowin->surface, NULL, SDL_MapRGBA(SDL_GetPixelFormatDetails(Infowin->surface->format), NULL, Pixel_quadruplet(Infowin->bg_color)));
	/* Draw borders. */
	draw_borders(Infowin, Infowin->b_color);
	/* Success */
	return(0);
}

/*
 * Init an infowin by giving some data.
 *
 * Inputs:
 *	x,y: The position of this Window
 *	w,h: The size of this Window
 *	b:   The border width
 *	b_color, bg_color: Border and background colors
 *
 */
static errr Infowin_init(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t b, Pixel b_color, Pixel bg_color) {
	if (2*b >= w || 2*b >= h) {
		fprintf(stderr, "Error initializing window: border %d too big for window size %dx%d\n", b, w, h);
		return(1);
	}

	Uint32 flags;
	SDL_Window *window;
	SDL_Surface *surface;

	/* Wipe it clean */
	WIPE(Infowin, infowin);

	/*** Create the SDL_Window* 'window' from data ***/

	/* Create the Window, but keep it hidden until positioned. */
	flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
	if (!sdl3_window_decorations) flags |= SDL_WINDOW_BORDERLESS;
	window = SDL_CreateWindow("", w, h, flags);

	if (window == NULL) {
		fprintf(stderr, "Error creating window: %s\n", SDL_GetError());
		return(2);
	}

	surface = SDL_GetWindowSurface(window);
	if (!surface) {
		fprintf(stderr, "Error creating surface for window: %s\n", SDL_GetError());
		SDL_DestroyWindow(window);
		return(3);
	}

	/* Initialize Infowin variables. */
	Infowin->windowID = SDL_GetWindowID(window);
	Infowin->window = window;
	Infowin->surface = surface;

	Infowin->x = x;
	Infowin->y = y;
	Infowin->wb = w;
	Infowin->hb = h;
	Infowin->wd = w-(2*b);
	Infowin->hd = h-(2*b);
	Infowin->bw = b;
	Infowin->bh = b;
	Infowin->b_color = b_color;
	Infowin->bg_color = bg_color;

	Infowin->nuke = true;
	Infowin->shown = !(SDL_GetWindowFlags(window) & SDL_WINDOW_HIDDEN);

	/* Draw borders on surface, fill with background color and update the created window. */
	SDL_FillSurfaceRect(surface, &(SDL_Rect){0, 0, w, h}, SDL_MapRGBA(SDL_GetPixelFormatDetails(surface->format), NULL, Pixel_quadruplet(b_color)));
	SDL_FillSurfaceRect(surface, &(SDL_Rect){b, b, w-(2*b), h-(2*b)}, SDL_MapRGBA(SDL_GetPixelFormatDetails(surface->format), NULL, Pixel_quadruplet(bg_color)));
	SDL_UpdateWindowSurface(window);

	/* Move he window to position and make the window visible. */
	Infowin_move(x, y);
	SDL_ShowWindow(window);

	return(0);
}

static void Infowin_readjust_frame_position() {
	int top = 0, left = 0;
	SDL_Rect bounds;
	SDL_DisplayID display;
	bool adjusted = false;

	/* First synchronize, so the window has decorations drawn, when asking for their dimensions. */
	SDL_SyncWindow(Infowin->window);

	/* Get the top and left decorations dimensions and adjust. */
	/* Note: Window needs to be shown to get the decorations dimensions. */
	if (!SDL_GetWindowBordersSize(Infowin->window, &top, &left, NULL, NULL)) {
		/* If decorations sizes can't be accessed, leave it be, do not try to correct the coordinates later. Just print a warning. When this becomes a real issue, it'll be handled. */
		fprintf(stderr, "Warning: Couldn't get window decorations sizes for window #%d: %s\n", Infowin->windowID, SDL_GetError());
		return;
	} 

	display = SDL_GetDisplayForWindow(Infowin->window);
	if (!display || !SDL_GetDisplayBounds(display, &bounds)) {
		fprintf(stderr, "Warning: Couldn't get display bounds for window #%d: %s\n", Infowin->windowID, SDL_GetError());
		return;
	}

	if (Infowin->x >= bounds.x && Infowin->x - left < bounds.x) {
		Infowin->x = bounds.x + left;
		adjusted = true;
	}
	if (Infowin->y >= bounds.y && Infowin->y - top < bounds.y) {
		Infowin->y = bounds.y + top;
		adjusted = true;
	}
	if (adjusted) Infowin_move(Infowin->x, Infowin->y);
}


/*
 * Request a Pixel by name.
 *
 * Inputs:
 *      name: The name of the color to try to load (see below)
 *
 * Output:
 *	The Pixel value that matched the given name
 *	'color_default_fg' if the name was unparseable
 *
 * Valid forms for 'name':
 *	'fg', 'bg', '<color-name>' and '#<hex-code>'
 */
static Pixel Infoclr_Pixel(cptr name) {

	/* Attempt to Parse the name */
	if (name && name[0]) {
		/* The 'bg' color is available */
		if (streq(name, "bg")) return color_default_bg;

		/* The 'fg' color is available */
		if (streq(name, "fg")) return color_default_fg;

		/* The 'white' color is available */
		if (streq(name, "white")) return (Pixel){0xFF, 0xFF, 0xFF, 0xFF};

		/* The 'black' color is available */
		if (streq(name, "black")) return (Pixel){0x00, 0x00, 0x00, 0xFF};

		/* 16 Basic Terminal Colors */
		if (streq(name, "red")) return (Pixel){0xAA, 0x00, 0x00, 0xFF};
		if (streq(name, "green")) return (Pixel){0x00, 0xAA, 0x00, 0xFF};
		if (streq(name, "yellow")) return (Pixel){0xAA, 0x55, 0x00, 0xFF};
		if (streq(name, "blue")) return (Pixel){0x00, 0x00, 0xAA, 0xFF};
		if (streq(name, "magenta")) return (Pixel){0xAA, 0x00, 0xAA, 0xFF};
		if (streq(name, "cyan")) return (Pixel){0x00, 0xAA, 0xAA, 0xFF};
		if (streq(name, "gray")) return (Pixel){0xAA, 0xAA, 0xAA, 0xFF};
		if (streq(name, "dark_red")) return (Pixel){0x55, 0x00, 0x00, 0xFF};
		if (streq(name, "dark_green")) return (Pixel){0x00, 0x55, 0x00, 0xFF};
		if (streq(name, "dark_yellow")) return (Pixel){0x55, 0x55, 0x00, 0xFF};
		if (streq(name, "dark_blue")) return (Pixel){0x00, 0x00, 0x55, 0xFF};
		if (streq(name, "dark_magenta")) return (Pixel){0x55, 0x00, 0x55, 0xFF};
		if (streq(name, "dark_cyan")) return (Pixel){0x00, 0x55, 0x55, 0xFF};
		if (streq(name, "dark_gray")) return (Pixel){0x55, 0x55, 0x55, 0xFF};
		if (streq(name, "light_gray")) return (Pixel){0xD3, 0xD3, 0xD3, 0xFF};

		/* If the format is '#......', parse the color */
		if (name[0] == '#' && strlen(name) == 7) {
			uint32_t color;
			if (sscanf(name + 1, "%06x", &color) == 1) {
				return 	(Pixel){
					.r = (color >> 16) & 0xFF,
					.g = (color >> 8) & 0xFF,
					.b = color & 0xFF,
					.a = 0xFF
				};
			}
		}

		plog_fmt("Warning: Couldn't parse color '%s'\n", name);
	}

	/* Warn about the Default being Used */
	plog_fmt("Warning: Using 'fg' for unknown color '%s'\n", name);

	/* Default to the 'Foreground' color */
	return color_default_fg;
}

/*
 * Initialize an infoclr with some data
 *
 * Inputs:
 *	fg:   The Pixel for the requested Foreground (see above)
 *	bg:   The Pixel for the requested Background (see above)
 */
static errr Infoclr_init_data(Pixel fg, Pixel bg) {
	infoclr *iclr = Infoclr;

	/*** Initialize ***/

	/* Wipe the iclr clean */
	WIPE(iclr, infoclr);

	/* Assign the parms */
	iclr->fg = fg;
	iclr->bg = bg;

	/* Success */
	return(0);
}


static cptr ANGBAND_DIR_XTRA_FONT;
static cptr ANGBAND_USER_DIR_XTRA_FONT;

/*
 * Init an infofnt by its Name and Size. Assumes font is a true type font.
 *
 * Inputs:
 *	name: The name of the requested Font followed by a space and a number, which defines the size of font to load.
 */
static errr Infofnt_init_ttf(cptr name, float scale) {
	/*** Load the info Fresh, using the name ***/

	/* If the name is not given, report an error */
	if (!name) return(-1);

	TTF_Font *font;
	char font_name[256];
	int font_size;
	char buf[1024];

	/* Parse the name to extract font name and size */
	if (sscanf(name, "%255s %d", font_name, &font_size) != 2) {
		/* If the format is incorrect, return an error */
		fprintf(stderr, "Font \"%s\" does not match pattern \"<font_name> <font_size>\"", name);
		return(-1);
	}

	path_build(buf, 1024, ANGBAND_USER_DIR_XTRA_FONT, font_name);
	if (!my_fexists(buf) && !sdl3_paths_same(ANGBAND_USER_DIR_XTRA_FONT, ANGBAND_DIR_XTRA_FONT)) {
		path_build(buf, 1024, ANGBAND_DIR_XTRA_FONT, font_name);
	}

	/* Attempt to load the font with the given size */
	font = TTF_OpenFont(buf, font_size * scale);

	/* The load failed, try to recover */
	if (!font) {
		fprintf(stderr, "Loading font \"%s\" error: %s\n", buf, SDL_GetError());
		return(-1);
	}


	/*** Init the font ***/

	/* Wipe the thing */
	WIPE(Infofnt, infofnt);

	/* Assign the struct */
	Infofnt->type = FONT_TYPE_TTF;
	Infofnt->font = font;
	Infofnt->scale = scale;

	/* Extract default sizing info */
	Infofnt->hgt = TTF_GetFontHeight(font);
	/* Assume monospaced font */
	if (TTF_FontIsFixedWidth(font) == 1) {
		Infofnt->emulateMonospace = false;
		/* Check all printable basic ASCII characters (32 to 126) for consistent width and height */
		char str[3] = "  ";
		SDL_Surface *surface;
		for (int ch = 32; ch <= 126; ++ch) {
			str[0] = (char)ch;
			surface = TTF_RenderText_Solid(font, str, 0, (Pixel){255, 255, 255});
			if (surface == NULL) {
				fprintf(stderr, "Failed to render character %d!\n", ch);
				/* Free the font */
				TTF_CloseFont(font);
				/* Fail */
				return(-1);
			}

			int str_w = surface->w;
			SDL_DestroySurface(surface);

			if (ch == 32) {
				/* Assign width and height based on the first character */
				Infofnt->wid = str_w/2;
			} else {
				/* Verify that all characters have the same width and height */
				if (str_w != 2*Infofnt->wid) {
					fprintf(stderr, "Dimensions mismatch for string \"%s\". Width is %d, expected %d.\n", str, str_w, 2*Infofnt->wid);
					/* Fail */
					Infofnt->emulateMonospace=true;
					break;
				}
			}
		}
	} else {
		if (Infofnt->emulateMonospace != true) {
			fprintf(stderr, "Font reports it is not monospace, turning on monospace emulation.\n");
			Infofnt->emulateMonospace=true;
		}
	}

	/* Save a copy of the font name */
	Infofnt->name = string_make(name);

	/* Mark it as nukable */
	Infofnt->nuke = 1;

	/* Success */
	return(0);
}

typedef struct PCF_Font PCF_Font;

/*
 * A structure holding a monospaced PCF font.
 *
 * - A SDL3 surface with all the glyphs. The glyphs are stored in one row.
 * - Number of glyphs in the font.
 * - The glyph index that corresponds to each encoding value (a value of 0xffff means no glyph for that encoding).
 * - The width and height of a glyph.
 */
struct PCF_Font {
	SDL_Surface *bitmap;
	uint32_t    nGlyphs;
	int16_t     *glyphIndexes;
	uint16_t    glyphWidth, glyphHeight;
};

static void PCF_CloseFont(PCF_Font *font);
static PCF_Font* PCF_OpenFont(const char *name, float scale);
SDL_Surface* PCF_RenderText(PCF_Font* font, const char* str, Pixel bg_color, Pixel fg_color);

/*
 * Init an infofnt by its Name. Assumes font is a monospaced .pcf bitmap font.
 *
 * Inputs:
 *	name: The name of the requested Font without extension.
 */
static errr Infofnt_init_pcf(cptr name, float scale) {
	/*** Load the info Fresh, using the name ***/

	/* If the name is not given, report an error */
	if (!name) return(-1);

	PCF_Font *font;
	char font_name[256];
	size_t len = strlen(name);
	char buf[1024];

	/* Append .pcf extension if missing */
	if ((len >= 4) && strcasecmp(name + len - 4, ".pcf") == 0) {
		strncpy(font_name, name, sizeof(font_name));
	} else {
		/* Add .pcf extension. */
		snprintf(font_name, sizeof(font_name), "%s.pcf", name);
	}
	font_name[sizeof(font_name) - 1] = '\0';

	path_build(buf, 1024, ANGBAND_USER_DIR_XTRA_FONT, font_name);
	if (!my_fexists(buf) && !sdl3_paths_same(ANGBAND_USER_DIR_XTRA_FONT, ANGBAND_DIR_XTRA_FONT)) {
		path_build(buf, 1024, ANGBAND_DIR_XTRA_FONT, font_name);
	}

	/* Attempt to load the font with the given size. */
	font = PCF_OpenFont(buf, scale);

	/* The load failed, try to recover */
	if (!font) {
		fprintf(stderr, "Loading font \"%s\" error\n", buf);
		return(-1);
	}

	/*** Init the font ***/

	/* Wipe the thing */
	WIPE(Infofnt, infofnt);

	/* Assign the struct */
	Infofnt->type = FONT_TYPE_PCF;
	Infofnt->font = font;
	Infofnt->scale = scale;

	/* Extract default sizing info */
	Infofnt->wid = font->glyphWidth;
	Infofnt->hgt = font->glyphHeight;
	Infofnt->emulateMonospace = false;

	/* Save a copy of the font name */
	Infofnt->name = string_make(name);

	/* Mark it as nukable */
	Infofnt->nuke = 1;

	/* Success */
	return(0);
}

/*
 * Detect if the filename is acceptable for a PCF font:
 *  - returns true for names without any extension
 *  - returns true for ".pcf"  (case-insensitive)
 *  - returns false for any other extension
 */
bool is_pcf_font(const char *name) {
	if (!name) return false;

	/* Trim trailing whitespace. */
	size_t len = strlen(name);
	while (len > 0 && isspace((unsigned char)name[len - 1])) --len;

	/* Check for empty string after trimming. */
	if (len == 0) return false;

	/* Mark end of string. One past the last real char. */
	const char *end = name + len;

	/* Find the last '.' that is *after* the last path separator. */
	const char *dot = NULL;
	for (const char *p = end; p-- > name; ) {
		/* Reached a path separator.  */
		if (*p == '/' || *p == '\\') break;

		/* Found a potential extension. */
		if (*p == '.') {
			dot = p;
			break;
		}
	}

	/* No '.' means no extension. */
	if (!dot) return true;

	/* Compare the extension including the dot. */
	if ((end - dot) == 4 && strncasecmp(dot, ".pcf", 4) == 0) return true;

	/* Other extension. */
	return false;
}

/*
 * Detect if `name` refers to a TTF font.
 * - If the filename ends with ".ttf" (case-insensitive), returns
 *   true; otherwise returns false.
 * - If `out_name` != NULL and `out_name_len` > 0, copies the
 *   *sanitized* basename (the part up to and including ".ttf",
 *   without trailing spaces or size suffix) into that buffer.
 * - If `out_size` != NULL, stores the parsed size; stores -1 when
 *   no numeric suffix is present.
 *
 * No dynamic allocation happens inside this function, so the caller
 * never has to `free()` anything.
 */
bool is_ttf_font(const char *name, char *out_name, size_t out_name_len, int8_t *out_size) {
	const char *ext;
	const char *p;
	int8_t size = -1;
	char tmp[256];
	size_t len;

	/* Initialise optional outputs. */
	if (out_name && out_name_len) *out_name = '\0';
	if (out_size) *out_size = -1;

	if (!name || !*name) return false;

	/* Copy and truncate to local buffer. */
	len = strlen(name);
	if (len >= sizeof(tmp)) len = sizeof(tmp) - 1;
	memcpy(tmp, name, len);
	tmp[len] = '\0';

	/* Trim trailing whitespace. */
	while (len && isspace((unsigned char)tmp[len - 1])) tmp[--len] = '\0';

	/* Locate extension. */
	ext = strrchr(tmp, '.');
	if (!ext || strncasecmp(ext, ".ttf", 4) != 0)
		return false;

	/* Parse optional numeric size after ".ttf". */
	/* Point just past ".ttf". */
	p = ext + 4;
	while (isspace((unsigned char)*p)) p++;
	if (*p) {
		/* There is something after the spaces. */
		char *endptr;
		long val = strtol(p, &endptr, 10);
		/* Not a number. */
		if (endptr == p) return false;
		while (isspace((unsigned char)*endptr)) endptr++;
		/* Junk after number. */
		if (*endptr) return false;
		size = (int8_t)val;
	}

	/* Copy sanitized basename into caller-supplied buffer. */
	if (out_name && out_name_len) {
		/* Include ".ttf". */
		size_t base_len = (ext - tmp) + 4;
		if (base_len >= out_name_len) base_len = out_name_len - 1;
		memcpy(out_name, tmp, base_len);
		out_name[base_len] = '\0';
	}

	if (out_size) *out_size = size;

	return true;
}

/*
 * Init an infofnt by its Name
 *
 * Inputs:
 *	name: The name of the requested Font
 */
static errr Infofnt_init(cptr name, float scale) {
	/* TrueType fonts are assumed unless the file name denotes a PCF font. */
	if (is_pcf_font(name)) {
		return Infofnt_init_pcf(name, scale);
	}
	return Infofnt_init_ttf(name, scale);

}

/* Free an initialized font structure. */
static void Infofnt_destroy(infofnt *fnt) {
	if (!fnt) return;

	if (fnt->font) {
		if (fnt->type == FONT_TYPE_TTF) TTF_CloseFont((TTF_Font*)fnt->font);
		if (fnt->type == FONT_TYPE_PCF) PCF_CloseFont((PCF_Font*)fnt->font);
	}
	if (fnt->name) string_free(fnt->name);
	FREE(fnt, infofnt);
}

/*
 * Standard Text
 */
static errr Infofnt_text_std(int x, int y, cptr str, int len) {
	if (!str || !*str) return(-1);


	if (len < 0) len = strlen (str);

	if (len == 0) return(0);

	if (Infofnt->emulateMonospace && len > 1) {
		for (int i = 0; i < len; ++i) {
			if (Infofnt_text_std(x + i, y, str + i, 1)) {
				fprintf(stderr, "Failed to display the letter '%c' onto the position x, y = %d, %d.\n", str[i], x+i, y);
			}
		}
		return(0);
	}

	x = Infowin->bw + (x * Infofnt->wid);
	y = Infowin->bh + (y * Infofnt->hgt);

	/* Create surface and text texture */
	Pixel fgColor = {Pixel_quadruplet(Infoclr->fg)};
	Pixel bgColor = {Pixel_quadruplet(Infoclr->bg)};

	SDL_Surface *surface = NULL;
	if (Infofnt->type == FONT_TYPE_TTF) {
		surface = TTF_RenderText_LCD((TTF_Font*)Infofnt->font, str, 0, fgColor, bgColor);
	} else if (Infofnt->type == FONT_TYPE_PCF) {
		surface = PCF_RenderText((PCF_Font*)Infofnt->font, str, fgColor, bgColor);
	}
	if (!surface) {
		fprintf(stderr, "Failed to create surface for text: %s\n", str);
		return(1);
	}


	SDL_Rect srcRect = {0, 0, Infofnt->wid*len, Infofnt->hgt};
	SDL_Rect dstRect = {x, y, Infofnt->wid*len, Infofnt->hgt};
	SDL_BlitSurface(surface, &srcRect, Infowin->surface, &dstRect);

	SDL_DestroySurface(surface);

	/* Success */
	return(0);
}
/*
 * Painting where text would be
 */
static errr Infofnt_text_non(int x, int y, cptr str, int len) {

	/* Negative length is a flag to count the characters in str. */
	if (len <= 0) len = strlen(str);

	int w = len * Infofnt->wid;
	int h = Infofnt->hgt;

	x = Infowin->bw + (x * Infofnt->wid);
	y = Infowin->bh + (y * h);


	/*** Find other dimensions ***/

	/* Simply do 'Infofnt->hgt' (a single row) high */


	/*** Actually 'paint' the area ***/
	/* Just do a Fill Rectangle */
	SDL_FillSurfaceRect(Infowin->surface, &(SDL_Rect){x, y, w, h}, SDL_MapRGBA(SDL_GetPixelFormatDetails(Infowin->surface->format), NULL, Pixel_quadruplet(Infoclr->fg)));

	/* Success */
	return(0);
}

/*
 * Color table
 */
#ifndef EXTENDED_BG_COLOURS
static infoclr *clr[CLIENT_PALETTE_SIZE];
#else
static infoclr *clr[CLIENT_PALETTE_SIZE + TERMX_AMT];
#endif

/*
 * Forward declare
 */
typedef struct term_data term_data;
static int term_data_to_term_idx(term_data *td);
void resize_main_window_sdl3(int cols, int rows);
static void resize_term_with_window(int term_idx, int cols, int rows);
static term_data* term_idx_to_term_data(int term_idx);

#ifdef USE_GRAPHICS
bool disable_tileset_caching = false;
#endif
#if defined(USE_GRAPHICS) && defined(TILE_CACHE_SIZE)
bool disable_tile_cache = false;

/*
 * The structure holds a single entry for tile cache.
 *
 * - Array of colors used to build the tile. Size of the array is `ncolors`.
 * - A SDL3 surface holding the cached tile.
 * - Tile index.
 * - Subset identifier for tiles coming from partial tilesets.
 * - Size of the `colors` array.
 */
struct tile_cache_entry {
	uint32_t *colors;
	SDL_Surface *tile;
	char32_t index;
	uint16_t ncolors;
	int16_t subset;
};
#endif
bool ignore_scaling = false;

/* A structure for each "term".
 *
 * - The `term` structure for the terminal.

 * - Font data for the terminal.
 * - Graphical window data.
 * - Timer used for terminal window resizing.
 *
 * Properties used when compiled with graphical tiles support:
 * - Loaded tiles image, resized and split into multiple layers. Number of layers is 'nlayers'.
 *   Each layer, except the first, contains only pixels, that belongs to it's mask color. All other pixels are fully transparent black color. The first layer contains also all other non-background mask color pixels.
 *   Each layer has blend mode set to SDL_BLENDMODE_NONE and color key set to it's foreground mask color. This way, if you blit/blend the layer tile over a colored surface, you'll get a layer surface with desired color and transparent background. These layers blended over a tile colored with background color will create full tile with colors changed to desired values.
 * - Used for preparing a tile layer during tile rendering. The size is always same as 'fnt' font size.
 * - Number of layers in 'tiles_layers'. When using graphics and everything is properly initialized, this is always 'graphics_image_mpt - 1' (number of foreground mask colors).
 *
 * Properties used when compiled with graphical tiles support and tile cache
 * - Array of cached tiles.
 * - Position of the writing head for the array (at which index will be the next tile cached).
 */
struct term_data {
	term t;

	infofnt *fnt;
	infowin *win;
	float display_scale;
	uint32_t resize_timer;

#ifdef USE_GRAPHICS
	SDL_Surface **tiles_layers;
	SDL_Surface **tiles_layers_sub[MAX_SUBFONTS];
	SDL_Surface *tilePreparation;
	uint8_t nlayers;
	SDL_Surface *tiles_surface;
	SDL_Surface *tiles_surface_sub[MAX_SUBFONTS];
	rawpict_tile tiles_rawpict[MAX_TILES_RAWPICT + 1];
	rawpict_tile tiles_rawpict_sub[MAX_SUBFONTS][MAX_TILES_RAWPICT + 1];
	int rawpict_scale_wid_org, rawpict_scale_hgt_org, rawpict_scale_wid_use, rawpict_scale_hgt_use;
	int rawpict_scale_wid_org_sub[MAX_SUBFONTS], rawpict_scale_hgt_org_sub[MAX_SUBFONTS], rawpict_scale_wid_use_sub[MAX_SUBFONTS], rawpict_scale_hgt_use_sub[MAX_SUBFONTS];

 #ifdef TILE_CACHE_SIZE
	struct tile_cache_entry tile_cache[TILE_CACHE_SIZE];
	int cache_position;
 #endif

#endif
};

#define OUTLINE_ALPHA 0xDD

/* The main screen. */
static term_data term_main;
/* The optional windows. */
static term_data term_1;
static term_data term_2;
static term_data term_3;

/*
 * Other (optional) windows.
 */
static term_data term_4;
static term_data term_5;
static term_data term_6;
static term_data term_7;
static term_data term_8;
static term_data term_9;

static term_data *sdl3_terms_term_data[ANGBAND_TERM_MAX] = {
	&term_main, &term_1, &term_2, &term_3, &term_4,
	&term_5, &term_6, &term_7, &term_8, &term_9
};
static char *sdl3_terms_font_env[ANGBAND_TERM_MAX] = {
	"TOMENET_SDL3_FONT_TERM_MAIN", "TOMENET_SDL3_FONT_TERM_1", "TOMENET_SDL3_FONT_TERM_2", "TOMENET_SDL3_FONT_TERM_3", "TOMENET_SDL3_FONT_TERM_4",
	"TOMENET_SDL3_FONT_TERM_5", "TOMENET_SDL3_FONT_TERM_6", "TOMENET_SDL3_FONT_TERM_7", "TOMENET_SDL3_FONT_TERM_8", "TOMENET_SDL3_FONT_TERM_9"
};
static char *sdl3_terms_font_default[ANGBAND_TERM_MAX] = {
	SDL3_DEFAULT_FONT_TERM_MAIN, SDL3_DEFAULT_FONT_TERM_1, SDL3_DEFAULT_FONT_TERM_2, SDL3_DEFAULT_FONT_TERM_3, SDL3_DEFAULT_FONT_TERM_4,
	SDL3_DEFAULT_FONT_TERM_5, SDL3_DEFAULT_FONT_TERM_6, SDL3_DEFAULT_FONT_TERM_7, SDL3_DEFAULT_FONT_TERM_8, SDL3_DEFAULT_FONT_TERM_9
};
static int sdl3_terms_ttf_size_default[ANGBAND_TERM_MAX] = {
	SDL3_DEFAULT_TTF_FONT_SIZE_TERM_MAIN, SDL3_DEFAULT_TTF_FONT_SIZE_TERM_1, SDL3_DEFAULT_TTF_FONT_SIZE_TERM_2, SDL3_DEFAULT_TTF_FONT_SIZE_TERM_3, SDL3_DEFAULT_TTF_FONT_SIZE_TERM_4,
	SDL3_DEFAULT_TTF_FONT_SIZE_TERM_5, SDL3_DEFAULT_TTF_FONT_SIZE_TERM_6, SDL3_DEFAULT_TTF_FONT_SIZE_TERM_7, SDL3_DEFAULT_TTF_FONT_SIZE_TERM_8, SDL3_DEFAULT_TTF_FONT_SIZE_TERM_9
};
const char *sdl3_terms_wid_env[ANGBAND_TERM_MAX] = {
	"TOMENET_SDL3_WID_TERM_MAIN", "TOMENET_SDL3_WID_TERM_1", "TOMENET_SDL3_WID_TERM_2", "TOMENET_SDL3_WID_TERM_3", "TOMENET_SDL3_WID_TERM_4",
	"TOMENET_SDL3_WID_TERM_5", "TOMENET_SDL3_WID_TERM_6", "TOMENET_SDL3_WID_TERM_7", "TOMENET_SDL3_WID_TERM_8", "TOMENET_SDL3_WID_TERM_9"
};
const char *sdl3_terms_hgt_env[ANGBAND_TERM_MAX] = {
	"TOMENET_SDL3_HGT_TERM_MAIN", "TOMENET_SDL3_HGT_TERM_1", "TOMENET_SDL3_HGT_TERM_2", "TOMENET_SDL3_HGT_TERM_3", "TOMENET_SDL3_HGT_TERM_4",
	"TOMENET_SDL3_HGT_TERM_5", "TOMENET_SDL3_HGT_TERM_6", "TOMENET_SDL3_HGT_TERM_7", "TOMENET_SDL3_HGT_TERM_8", "TOMENET_SDL3_HGT_TERM_9"
};

/*
 * Keycode macros, used on Keycode to test for classes of symbols.
 */
#define IsModifierKey(scancode) \
	((scancode == SDL_SCANCODE_LSHIFT) || (scancode == SDL_SCANCODE_RSHIFT) || \
	(scancode == SDL_SCANCODE_LCTRL) || (scancode == SDL_SCANCODE_RCTRL) || \
	(scancode == SDL_SCANCODE_LALT) || (scancode == SDL_SCANCODE_RALT) || \
	(scancode == SDL_SCANCODE_LGUI)   || (scancode == SDL_SCANCODE_RGUI)   || \
	(scancode == SDL_SCANCODE_NUMLOCKCLEAR) || (scancode == SDL_SCANCODE_CAPSLOCK) || \
	(scancode == SDL_SCANCODE_SCROLLLOCK))

/*
 * Checks if the keysym is a special key or a normal key.
 * Special keys in SDL3 start from SDLK_WORLD_0 keycode.
 */
#define IsSpecialKey(keycode) \
	(((keycode) >= SDLK_F1 && (keycode) <= SDLK_F12) || \
	 (keycode) == SDLK_ESCAPE || (keycode) == SDLK_RETURN || \
	 (keycode) == SDLK_TAB || (keycode) == SDLK_BACKSPACE || \
	 (keycode) == SDLK_DELETE || (keycode) == SDLK_HOME || \
	 (keycode) == SDLK_END || (keycode) == SDLK_PAGEUP || \
	 (keycode) == SDLK_PAGEDOWN || (keycode) == SDLK_INSERT || \
	 (keycode) == SDLK_PRINTSCREEN || (keycode) == SDLK_PAUSE)

#ifdef SDL3_STICKY_KEYS
bool ctrl_forced = false;
bool shift_forced = false;
bool alt_forced = false;
#endif
static int Term_win_update(int flush, int sync, int discard);
static void term_force_font(int term_idx, cptr fnt_name);

static int keypad_keysym(SDL_Scancode scancode, bool numlock) {
	switch (scancode) {
		case SDL_SCANCODE_KP_0: return(numlock ? 0xFFB0 : 0xFF9E);
		case SDL_SCANCODE_KP_1: return(numlock ? 0xFFB1 : 0xFF9C);
		case SDL_SCANCODE_KP_2: return(numlock ? 0xFFB2 : 0xFF99);
		case SDL_SCANCODE_KP_3: return(numlock ? 0xFFB3 : 0xFF9B);
		case SDL_SCANCODE_KP_4: return(numlock ? 0xFFB4 : 0xFF96);
		case SDL_SCANCODE_KP_5: return(numlock ? 0xFFB5 : 0xFF9D);
		case SDL_SCANCODE_KP_6: return(numlock ? 0xFFB6 : 0xFF98);
		case SDL_SCANCODE_KP_7: return(numlock ? 0xFFB7 : 0xFF95);
		case SDL_SCANCODE_KP_8: return(numlock ? 0xFFB8 : 0xFF97);
		case SDL_SCANCODE_KP_9: return(numlock ? 0xFFB9 : 0xFF9A);
		case SDL_SCANCODE_KP_PERIOD: return(numlock ? 0xFFAE : 0xFF9F);
		case SDL_SCANCODE_KP_DIVIDE: return(0xFFAF);
		case SDL_SCANCODE_KP_MULTIPLY: return(0xFFAA);
		case SDL_SCANCODE_KP_MINUS: return(0xFFAD);
		case SDL_SCANCODE_KP_PLUS: return(0xFFAB);
		case SDL_SCANCODE_KP_ENTER: return(0xFF8D);
		default: return(-1);
	}
}
static int non_printable_keysym(SDL_Keycode keycode) {
	switch (keycode) {
		case SDLK_F1: return(0xFFBE);
		case SDLK_F2: return(0xFFBF);
		case SDLK_F3: return(0xFFC0);
		case SDLK_F4: return(0xFFC1);
		case SDLK_F5: return(0xFFC2);
		case SDLK_F6: return(0xFFC3);
		case SDLK_F7: return(0xFFC4);
		case SDLK_F8: return(0xFFC5);
		case SDLK_F9: return(0xFFC6);
		case SDLK_F10: return(0xFFC7);
		case SDLK_F11: return(0xFFC8);
		case SDLK_F12: return(0xFFC9);
		case SDLK_PRINTSCREEN: return(0xFF61);
		case SDLK_SCROLLLOCK: return(0xFF14);
		case SDLK_PAUSE: return(0xFF13);
		case SDLK_UP: return(0xFF52);
		case SDLK_DOWN: return(0xFF54);
		case SDLK_LEFT: return(0xFF51);
		case SDLK_RIGHT: return(0xFF53);
		case SDLK_INSERT: return(0xFF63);
		case SDLK_PAGEUP: return(0xFF55);
		case SDLK_PAGEDOWN: return(0xFF56);
		case SDLK_HOME: return(0xFF50);
		case SDLK_END: return(0xFF57);
		case SDLK_ESCAPE: return(ESCAPE);
		case SDLK_RETURN: return('\r');
		case SDLK_TAB: return('\t');
		case SDLK_LEFT_TAB: return('\t');
		case SDLK_DELETE: return('\177'); /* DEL (127, 0x7F) */
		case SDLK_BACKSPACE: return('\010');
		case SDLK_SPACE: return(' ');
		default: return(-1);
	}
}

static bool is_special_keysym(SDL_Keycode keycode) {
	switch (keycode) {
		case SDLK_ESCAPE:
		case SDLK_RETURN:
		case SDLK_TAB:
		case SDLK_LEFT_TAB:
		case SDLK_DELETE:
		case SDLK_BACKSPACE:
		case SDLK_SPACE:
			return true;
	}
	return false;
}

#ifdef ALLOW_NAVI_KEYS_IN_PROMPT
static void navikey_msg(char *msg, byte msglen, bool mc, bool ms, bool ma, bool sc, uint32_t sym) {
	snprintf(msg, msglen, "%s%s%s%s%s_%X%s",
			(const char [])NAVI_KEY_SEQ_START,
			mc ? (const char [])NAVI_KEY_SEQ_CTRL : "",
			ms ? (const char [])NAVI_KEY_SEQ_SHIFT : "",
			ma ? (const char [])NAVI_KEY_SEQ_ALT : "",
			sc ? "K" : "",
			sym,
			(const char [])NAVI_KEY_SEQ_TERM);
}
static void navikey_symbol(char *msg, byte msglen, bool mc, bool ms, bool ma, uint32_t symbol) {
	navikey_msg(msg, msglen, mc, ms, ma, false, symbol);
}
static void navikey_scancode(char *msg, byte msglen, bool mc, bool ms, bool ma, SDL_Scancode code) {
	navikey_msg(msg, msglen, mc, ms, ma, true, (uint32_t)code);
}

/* Return the default character represented by a keypad key. */
static char keypad_fallback_key(SDL_Scancode scancode) {
	switch (scancode) {
		case SDL_SCANCODE_KP_1: return('1');
		case SDL_SCANCODE_KP_2: return('2');
		case SDL_SCANCODE_KP_3: return('3');
		case SDL_SCANCODE_KP_4: return('4');
		case SDL_SCANCODE_KP_5: return('5');
		case SDL_SCANCODE_KP_6: return('6');
		case SDL_SCANCODE_KP_7: return('7');
		case SDL_SCANCODE_KP_8: return('8');
		case SDL_SCANCODE_KP_9: return('9');
		case SDL_SCANCODE_KP_DIVIDE: return('/');
		case SDL_SCANCODE_KP_MULTIPLY: return('*');
		case SDL_SCANCODE_KP_MINUS: return('-');
		case SDL_SCANCODE_KP_PLUS: return('+');
		default: return(0);
	}
}

/*
 * Append an X11-style default action to a keypad trigger.  The delimiters
 * make inkey() discard this action when a keypad macro matches and execute it
 * without macro expansion when no keypad macro matches.  Thus macros on the
 * ordinary number row remain independent from keypad movement.
 */
static void keypad_fallback(SDL_Scancode scancode, bool mc, bool ms, bool ma) {
	char fallback = keypad_fallback_key(scancode);
	bool direction = fallback >= '1' && fallback <= '9';

	if (!fallback) return;

	Term_keypress(28);
	if (direction && ms && !mc && !ma) {
		Term_keypress(ESCAPE);
		Term_keypress(ESCAPE);
		Term_keypress('\\');
		Term_keypress('.');
	} else if (direction && mc && !ms && !ma) {
		Term_keypress(ESCAPE);
		Term_keypress(ESCAPE);
		Term_keypress('\\');
		Term_keypress('+');
	}
	Term_keypress(fallback);
	Term_keypress(28);
}
#endif

static bool has_control(SDL_Keycode k) {
    return (k >= '@' && k < '\177') || k == ' ' || (k >= '2' && k <= '8') || (k == '/');
}
static SDL_Keycode to_control(SDL_Keycode k) {
    if ((k >= '@' && k < '\177') || k == ' ') return k & 0x1F;
    if (k == '2') return '\000';
    if (k >= '3' && k <= '7') return k - ('3' - '\033');
    if (k == '8') return '\177';
    if (k == '/') return '_' & 0x1F;
    return '\0';
}

/*
 * Process a keypress event
 */
static void react_keypress(SDL_Event *event) {
	SDL_Scancode scancode = event->key.scancode;
	SDL_Keycode keycode;
	SDL_Keymod mod;
	int sym;
	char msg[128] = "";
	bool mc, ms, ma, mn, force_not_special;

	if (scancode == SDL_SCANCODE_UNKNOWN) {
		fprintf(stderr, "Key with unknown scancode pressed.\n");
		return;
	}

	/* Ignore "modifier keys" */
	if (IsModifierKey(scancode)) {
#ifdef SDL3_STICKY_KEYS
		if (scancode == SDL_SCANCODE_LCTRL || scancode == SDL_SCANCODE_RCTRL) { ctrl_forced = true; }
		if (scancode == SDL_SCANCODE_LSHIFT || scancode == SDL_SCANCODE_RSHIFT) { shift_forced = true; }
		if (scancode == SDL_SCANCODE_LALT || scancode == SDL_SCANCODE_RALT) { alt_forced = true; }
#endif
		return;
	}

	/* Extract four modifier flags. */
	/* Note: AltGr is handled as ordinary Alt key. */
	/* Note: Handle num-lock as always off. */
	mod = SDL_GetModState();
	ms = (mod & SDL_KMOD_SHIFT) ? true : false;
	mc = (mod & SDL_KMOD_CTRL) ? true : false;
	ma = (mod & SDL_KMOD_ALT) ? true : false;
	mn = false; /* (mod & SDL_KMOD_NUM) ? true : false; */

#ifdef SDL3_STICKY_KEYS
	if (ctrl_forced) {
		mc = true;
		ctrl_forced = false;
	}
	if (shift_forced) {
		ms = true;
		shift_forced = false;
	}
	if (alt_forced) {
		ma = true;
		alt_forced = false;
	}
	mod = (ms ? SDL_KMOD_LSHIFT : 0) | (mc ? SDL_KMOD_LCTRL : 0) | (ma ? SDL_KMOD_LALT : 0) | (mn ? SDL_KMOD_NUM : 0);
#endif

#ifdef ENABLE_SHIFT_SPECIALKEYS
	/* As we're shortcutting the whole key-evaluation with the various 'return' calls here, set the shiftkey flags manually */
	if (ms) inkey_shift_special |= 0x1;
	if (mc) inkey_shift_special |= 0x2;
	if (ma) inkey_shift_special |= 0x4;
#endif

	keycode = SDL_GetKeyFromScancode(scancode, mod, false);

	if (keycode == SDLK_UNKNOWN) {
#ifdef ALLOW_NAVI_KEYS_IN_PROMPT
		/* Unknown keycode but known scancode: send a special key sequence. */
		navikey_scancode(msg, 128, mc, ms, ma, scancode);
		for (int i = 0; msg[i]; i++) Term_keypress(msg[i]);
#else
		fprintf(stderr, "Unknown keycode for pressed key with scancode: %d\n", scancode);
#endif
		return;
	}

	/* Handle non-printable and special characters first. */
	sym = non_printable_keysym(keycode);
	if (sym < 0) sym = keypad_keysym(scancode, mn);
	if (sym > 0) {
#ifndef ALLOW_NAVI_KEYS_IN_PROMPT
		/* No navigational keys support, just handle special keys (enter, space, ...). */
		if (is_special_keysym(keycode)) Term_keypress(sym);
#else
		/* Hack for <tab> with modifiers. */
		force_not_special = false;
		if (sym == '\t' && (mc || ms || ma)) {
			sym = 0xFE20;
			force_not_special = true;
		};

		if (!force_not_special && is_special_keysym(keycode)) {
			if (mc || ms || ma) {
				navikey_symbol(msg, 128, mc, ms, ma, (uint32_t)sym);
				for (int i = 0; msg[i]; i++) Term_keypress(msg[i]);
				Term_keypress(28);
			}
			/* Hack: Just for space convert when ctrl pressed. */
			if (mc && sym == ' ') sym &= 0x1F;
			Term_keypress(sym);
			if (mc || ms || ma) {
				Term_keypress(28);
			}
			return;
		}

		navikey_symbol(msg, 128, mc, ms, ma, (uint32_t)sym);
		for (int i = 0; msg[i]; i++) Term_keypress(msg[i]);
		keypad_fallback(scancode, mc, ms, ma);
#endif
		return;
	}

	if (mc || ma) {
		if (!ma && !ms && keycode >= 'a' && keycode <= 'z') {
			keycode = to_control(keycode);
			Term_keypress(keycode);
			return;
		}
#ifndef ALLOW_NAVI_KEYS_IN_PROMPT
		/* No navigational keys support. */
		Term_keypress((mc && !ma && has_control(keycode)) ? to_control(keycode) : keycode);
#else
		/* Hack: For more compatibility with x11/windows clients, at least convert lowercase letters to uppercase if shift is pressed. */
		if (ms && !mc && keycode >= 'a' && keycode <= 'z') {
			keycode = keycode - 'a' + 'A';
		}

		navikey_symbol(msg, 128, mc, ms, ma, (uint32_t)keycode);
		for (int i = 0; msg[i]; i++) Term_keypress(msg[i]);

		if (mc && has_control(keycode)) {
			keycode = to_control(keycode);
		}

		Term_keypress(28);
		Term_keypress(keycode);
		Term_keypress(28);
#endif
		return;
	}

	Term_keypress(keycode);

	return;
}

static void available_tiles(term_data *td, int *cols, int *rows) {
	if (td == NULL || td->win == NULL || td->fnt == NULL || cols == NULL || rows == NULL) return;

 (*cols) = (td->win->wb - (2 * SDL3_DEFAULT_BORDER_WIDTH)) / td->fnt->wid;
 (*rows) = (td->win->hb - (2 * SDL3_DEFAULT_BORDER_WIDTH)) / td->fnt->hgt;
}
/*
 * Handles all terminal windows resize timers.
 */
static void resize_window_timers_handle(void) {
	uint32_t now = 0;
	int t_idx;

	for (t_idx = 0; t_idx < ANGBAND_TERM_MAX; t_idx++) {
		term_data *td = term_idx_to_term_data(t_idx);

		/* If resize_timer is nonzero, it represents the tick count after which we should resize. */
		if (td->resize_timer != 0) {
			/* Get the current SDL ticks once per loop. */
			if (now == 0) now = SDL_GetTicks();

			/* Check if the time has passed to resize. */
			if (now >= td->resize_timer) {
				td->resize_timer = 0;

				int cols, rows;
				available_tiles(td, &cols, &rows);
				/* Calculate new columns/rows based on window border and font dimensions. */
				resize_term_with_window(t_idx, cols, rows);

				/* In case we resized Chat/Msg/Msg+Chat window,
				   refresh contents so they are displayed properly,
				   without having to wait for another incoming message to do it for us. */
				p_ptr->window |= PW_MESSAGE | PW_CHAT | PW_MSGNOCHAT;
			}
		}
	}
}

/*
 * Process events
 */
static int CheckEvent(bool wait) {
	term_data *old_td = (term_data*)(Term->data);

	term_data *td = NULL;
	Uint32 window_id = 0;

	/* Handle all window resize timers. */
	resize_window_timers_handle();

	SDL_Event event;

	if (!wait) {
		if (!SDL_PollEvent(&event)) return (1);
	} else {
		if (!SDL_WaitEvent(&event)) return(1);
	}

	if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
		quit(NULL);
		return(0);
	}

	if (event.type == SDL_EVENT_KEY_DOWN) window_id = event.key.windowID;
	else if (event.type == SDL_EVENT_TEXT_INPUT) window_id = event.text.windowID;
	else if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST) window_id = event.window.windowID;

	/* Determine which window was the event for. */
	for (int i = 0; i < ANGBAND_TERM_MAX; i++) {
		if (sdl3_terms_term_data[i]->win && sdl3_terms_term_data[i]->win->window && sdl3_terms_term_data[i]->win->windowID == window_id) {
			td = sdl3_terms_term_data[i];
			break;
		}
	}

	/* Unknown window */
	if (!td || !td->win) return(0);

	/* Hack -- activate the Term */
	Term_activate(&td->t);

	/* Hack -- activate the window */
	Infowin_set(td->win);

	switch (event.type) {
		case SDL_EVENT_KEY_DOWN:

			/* Hack -- use "old" term */
			Term_activate(&old_td->t);

			/* Process the key */
			react_keypress(&event);

			break;

		case SDL_EVENT_WINDOW_RESIZED:
			{
				/* Handle window resize. */
				int new_wid = event.window.data1;
				int new_hgt = event.window.data2;

				/* Check if window is resized. */
				bool resized = Infowin->wb != new_wid || Infowin->hb != new_hgt;

				Infowin->wb = new_wid;
				Infowin->hb = new_hgt;
				///* Quick compute drawing space using default border width. */
				//Infowin->wd = new_wid > 2*SDL3_DEFAULT_BORDER_WIDTH ? new_wid - 2*SDL3_DEFAULT_BORDER_WIDTH : 0;
				//Infowin->hd = new_hgt > 2*SDL3_DEFAULT_BORDER_WIDTH ? new_hgt - 2*SDL3_DEFAULT_BORDER_WIDTH : 0;

				/* SDL invalidates the window surface as soon as the window is resized. Refresh it now so drawing during the resize debounce never uses the stale surface. */
				Infowin->surface = SDL_GetWindowSurface(Infowin->window);

				if (resized) {
					Infowin_wipe();
					Term_win_update(1, 0, 0);
					Term_redraw();

					/* Window resize timer start. */
					td->resize_timer = SDL_GetTicks() + 500; /* Add 1/2 second (500 ms). */
				}
				break;
			}

		case SDL_EVENT_WINDOW_MOVED:
			{
				/* The coordinates in the event are for client window. */
				Infowin->x = event.window.data1;
				Infowin->y = event.window.data2;
				break;
			}

		case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
			{
				if (ignore_scaling) break;

				float scale = SDL_GetWindowDisplayScale(Infowin->window);
				if (scale <= 0.0f) scale = 1.0f;

				if (scale != td->display_scale && td->fnt) {
					td->display_scale = scale;
					term_force_font(td->t.idx, td->fnt->name);
				}
				break;
			}
	}

	/* Hack -- Activate the old term */
	Term_activate(&old_td->t);

	/* Hack -- Activate the proper "inner" window */
	Infowin_set(old_td->win);

	/* Success */
	return(0);
}

#ifdef USE_GRAPHICS

/* Frees all graphics structures in provided term_data and sets them to zero values. */
static void free_graphics(term_data *td) {
	int i;
	/* Preserve layer count for sub-tileset cleanup. */
	int layers = td->nlayers;

	if (layers > 0 && td->tiles_layers != NULL) {
		for (i = 0; i < layers; i++) {
			if (td->tiles_layers[i] != NULL) SDL_DestroySurface(td->tiles_layers[i]);
		}

		C_FREE(td->tiles_layers, layers, SDL_Surface*);
	}
	td->tiles_layers = NULL;
	td->nlayers = 0;
	if (td->tilePreparation) {
		SDL_DestroySurface(td->tilePreparation);
		td->tilePreparation = NULL;
	}
	if (td->tiles_surface) {
		SDL_DestroySurface(td->tiles_surface);
		td->tiles_surface = NULL;
	}
	td->rawpict_scale_wid_org = td->rawpict_scale_hgt_org = 0;
	td->rawpict_scale_wid_use = td->rawpict_scale_hgt_use = 0;
	C_WIPE(td->tiles_rawpict, MAX_TILES_RAWPICT + 1, rawpict_tile);

	/* Sub-tilesets */
	for (int s = 0; s < MAX_SUBFONTS; s++) {
		if (td->tiles_layers_sub[s]) {
			for (i = 0; i < layers; i++) {
				if (td->tiles_layers_sub[s][i]) SDL_DestroySurface(td->tiles_layers_sub[s][i]);
			}
			C_FREE(td->tiles_layers_sub[s], layers, SDL_Surface*);
		}
		td->tiles_layers_sub[s] = NULL;
		if (td->tiles_surface_sub[s]) {
			SDL_DestroySurface(td->tiles_surface_sub[s]);
			td->tiles_surface_sub[s] = NULL;
		}
		td->rawpict_scale_wid_org_sub[s] = td->rawpict_scale_hgt_org_sub[s] = 0;
		td->rawpict_scale_wid_use_sub[s] = td->rawpict_scale_hgt_use_sub[s] = 0;
		C_WIPE(td->tiles_rawpict_sub[s], MAX_TILES_RAWPICT + 1, rawpict_tile);
	}

 #ifdef TILE_CACHE_SIZE
	td->cache_position = 0;
	for (int i = 0; i < TILE_CACHE_SIZE; i++) {
		if (td->tile_cache[i].tile) {
			SDL_DestroySurface(td->tile_cache[i].tile);
			td->tile_cache[i].tile = NULL;
		}
		td->tile_cache[i].index = 0;
		td->tile_cache[i].subset = -1;
		if (td->tile_cache[i].ncolors > 0) {
			C_FREE(td->tile_cache[i].colors, td->tile_cache[i].ncolors, uint32_t);
			td->tile_cache[i].ncolors = 0;
		}
	}
 #endif
}
#endif

/* Closes all SDL3 windows and frees all allocated data structures for input parameter. */
static errr term_data_nuke(term_data *td) {
	if (td == NULL) return(0);

#ifdef USE_GRAPHICS
	/* Free graphics structures. */
	if (use_graphics) {
		/* Free graphic tiles & masks. */
		free_graphics(td);
	}
#endif

	/* Unmap & free inner window. */
	if (td->win && td->win->nuke) {
		if (Infowin == td->win) Infowin_set(NULL);
		if (td->win->window) {
			SDL_DestroyWindow(td->win->window);
			td->win->surface = NULL;
			td->win->windowID = -1;
		}
		FREE(td->win, infowin);
	}

	/* Reset timers just to be sure. */
	td->resize_timer = 0;

	/* Free font. */
	if (td->fnt && td->fnt->nuke) {
		if (Infofnt == td->fnt) Infofnt_set(NULL);
		if (td->fnt->font) {
			if (td->fnt->type == FONT_TYPE_TTF) TTF_CloseFont((TTF_Font*)td->fnt->font);
			if (td->fnt->type == FONT_TYPE_PCF) PCF_CloseFont((PCF_Font*)td->fnt->font);
		}
		if (td->fnt->name) string_free(td->fnt->name);
		FREE(td->fnt, infofnt);
	}

	return(0);
}

/* Saves terminal window position, dimensions and font for term_idx to term_prefs.
 * Note: The term_prefs visibility is not handled here. */
static void term_data_to_term_prefs(int term_idx) {
	if (term_idx < 0 || term_idx >= ANGBAND_TERM_MAX) return;
	term_data *td = term_idx_to_term_data(term_idx);

	/* Update position. */
	term_prefs[term_idx].x = td->win->x;
	term_prefs[term_idx].y = td->win->y;

	term_prefs[term_idx].columns = td->t.wid;
	term_prefs[term_idx].lines = td->t.hgt;

	/* Update font. */
	if (strcmp(term_prefs[term_idx].font, td->fnt->name) != 0) {
		strncpy(term_prefs[term_idx].font, td->fnt->name, sizeof(term_prefs[term_idx].font));
		term_prefs[term_idx].font[sizeof(term_prefs[term_idx].font) - 1] = '\0';
	}
}

/* For saving all window layout while client is still running (from within = menu) */
void all_term_data_to_term_prefs(void) {
	int n;

	for (n = 0; n < ANGBAND_TERM_MAX; n++) {
		if (!term_get_visibility(n)) continue;
		term_data_to_term_prefs(n);
	}
}

/*
 * Handle destruction of a term.
 * Here we should properly destroy all windows and resources for terminal.
 * But after this the whole client ends (should not recover), so just use it for filling terminal preferences, which will be saved after all terminals are nuked.
 */
static void Term_nuke_sdl3(term *t) {
	term_data *td = (term_data*)(t->data);
	int term_idx;

	/* special hack: this window was invisible, but we just toggled it to become visible on next client start. */
	if (!td->fnt) return;

	term_idx = term_data_to_term_idx(td);
	if (term_idx < 0) {
		fprintf(stderr, "Error getting terminal index from term_data\n");
		return;
	}

	term_data_to_term_prefs(term_idx);

	term_data_nuke(td);
	t->data = NULL;
}

/*
 * Handle "activation" of a term
 */
static errr Term_xtra_sdl3_level(int v) {
	term_data *td = (term_data*)(Term->data);

	/* Handle "activate" */
	if (v) {
		/* Activate the "inner" window */
		Infowin_set(td->win);

		/* Activate the "inner" font */
		Infofnt_set(td->fnt);
	}

	/* Success */
	return(0);
}

/*
 * General Flush/ Sync/ Discard routine
 */
static int Term_win_update(int flush, int sync, int discard) {
	/* Flush if desired */
	if (flush) {
		/* Render prepared texture into the window renderer respecting borders. */
		term_data *td = (term_data*)(Term->data);

#ifdef SDL3_STICKY_KEYS
		if (td == &term_main) {
			uint8_t r = td->win->b_color.r;
			uint8_t g = td->win->b_color.g;
			uint8_t b = td->win->b_color.b;
			uint8_t a = td->win->b_color.a;
			if (ctrl_forced) r ^= 0xFF;
			if (shift_forced) g ^= 0xFF;
			if (alt_forced) b ^= 0xFF;
			draw_borders(td->win, (Pixel){r, g, b, a});
		}
#endif

		if (td->win->surface) SDL_UpdateWindowSurface(td->win->window);
	}

	/* Sync if desired, using 'discard' */
	if (sync) {
		/* Call SDL_PumpEvents to update the input state, which can be analogous to sync operations */
		SDL_PumpEvents();

		if (discard) {
			/* If we want to discard events, we can clear the event queue */
			SDL_FlushEvent(SDL_EVENT_FIRST);
		}
	}

	/* Success */
	return 0;
}

/*
 * Handle a "special request"
 */
static errr Term_xtra_sdl3(int n, int v) {
	/* Handle a subset of the legal requests */
	switch (n) {
		/* Make a noise */
		case TERM_XTRA_NOISE:
			/* This is called after failing making a sound using a sdl library or sdl sound is not built in. */
			/* Fallback to a simple ASCII bell */
			fputc('\a', stdout);
			fflush(stdout);
			return(0);

		/* Flush the output XXX XXX XXX */
		case TERM_XTRA_FRESH: Term_win_update(1, 0, 0); return(0);

		/* Process random events XXX XXX XXX */
		case TERM_XTRA_BORED: return(CheckEvent(0));

		/* Process Events XXX XXX XXX */
		case TERM_XTRA_EVENT: return(CheckEvent(v));

		/* Flush the events XXX XXX XXX */
		case TERM_XTRA_FLUSH: while (!CheckEvent(false)); return(0);

		/* Handle change in the "level" */
		case TERM_XTRA_LEVEL: return(Term_xtra_sdl3_level(v));

		/* Clear the screen */
		case TERM_XTRA_CLEAR: Infowin_wipe(); return(0);

		/* Delay for some milliseconds */
		case TERM_XTRA_DELAY: SDL_Delay(v); return(0);
	}

	/* Unknown */
	return(1);
}

/*
 * Erase a number of characters
 */
static errr Term_wipe_sdl3(int x, int y, int n) {
	/* Erase (use black) */
	Infoclr_set(clr[0]);

	/* Mega-Hack -- Erase some space */
	Infofnt_text_non(x, y, "", n);

	/* Success */
	return(0);
}

static int cursor_x = -1, cursor_y = -1;
/*
 * Draw the cursor (XXX by hiliting)
 */
static errr Term_curs_sdl3(int x, int y) {
	SDL_Surface *cursor;

	/*
	 * Don't place the cursor in the same place multiple times to avoid
	 * blinking.
	 */
	if ((cursor_x != x) || (cursor_y != y)) {
		/* Draw translucent cursor rectangle */
		int w = Infofnt->wid;
		int h = Infofnt->hgt;
		int px = Infowin->bw + x * w;
		int py = Infowin->bh + y * h;

		cursor = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
		if (cursor) {
			SDL_FillSurfaceRect(cursor, NULL, SDL_MapRGBA(SDL_GetPixelFormatDetails(cursor->format), NULL, Pixel_quadruplet(cursor_color)));
			SDL_SetSurfaceBlendMode(cursor, SDL_BLENDMODE_BLEND);
			SDL_BlitSurface(cursor, NULL, Infowin->surface, &(SDL_Rect){px, py, w, h});
			SDL_DestroySurface(cursor);
		}

		cursor_x = x;
		cursor_y = y;
	}

	/* Success */
	return(0);
}

/*
 * Draw a number of characters (XXX Consider using "cpy" mode)
 */
static errr Term_text_sdl3(int x, int y, int n, byte a, cptr s) {
#if 1 /* For 2mask mode: Actually imprint screen buffer with "empty background" for this text printed grid, to possibly avoid glitches */
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
	/* Catch use in chat instead of as feat attr, or we crash :-s
	   (term-idx 0 is the main window; screen-pad-left check: In case it is used in the status bar for some reason; screen-pad-top checks: main screen top chat line or status line) */
	if (Term && Term->data == &term_main&& x >= SCREEN_PAD_LEFT && x < SCREEN_PAD_LEFT + SCREEN_WID && y >= SCREEN_PAD_TOP && y < SCREEN_PAD_TOP + SCREEN_HGT) {
		flick_global_x = x;
		flick_global_y = y;
	} else flick_global_x = 0;

	a = term2attr(a);

	/* Draw the text in Xor */
#ifndef EXTENDED_COLOURS_PALANIM
 #ifndef EXTENDED_BG_COLOURS
	Infoclr_set(clr[a & 0x0F]);
 #else
	Infoclr_set(clr[a & 0x1F]); /* undefined case actually, we don't want to have a hole in the colour array (0..15 and then 32..32+x) -_- */
 #endif
#else
 #ifndef EXTENDED_BG_COLOURS
	Infoclr_set(clr[a & 0x1F]);
 #else
	Infoclr_set(clr[a & 0x3F]);
 #endif
#endif

	/* Draw the text */
	if (n < 0) n = strlen(s);
	if (n > 0) {
		const unsigned char solid_ch = (unsigned char)FONT_MAP_SOLID_X11;
		const unsigned char *us = (const unsigned char *)s;
		const unsigned char *first = memchr(us, solid_ch, (size_t)n);
		if (first == NULL) {
			Infofnt_text_std(x, y, s, n);
		} else {
			/* Render solid wall glyphs as filled blocks. */
			int i = (int)(first - us);
			if (i > 0) Infofnt_text_std(x, y, s, i);
			while (i < n) {
				bool solid = us[i] == solid_ch;
				int run = 1;
				while (i + run < n && (us[i + run] == solid_ch) == solid) run++;
				if (solid) Infofnt_text_non(x + i, y, "", run);
				else Infofnt_text_std(x + i, y, s + i, run);
				i += run;
			}
		}
	}

	/* Drawing text seems to clear the cursor */
	if (cursor_y == y && x <= cursor_x && cursor_x <= x + n) {
		/* Cursor is gone */
		cursor_x = -1;
		cursor_y = -1;
	}

	/* Success */
	return(0);
}

#ifdef USE_GRAPHICS

/* Directory with graphics tiles files (should be lib/xtra/graphics). */
static cptr ANGBAND_DIR_XTRA_GRAPHICS = NULL;
static cptr ANGBAND_USER_DIR_XTRA_GRAPHICS = NULL;

/* Loaded tiles image. */
static SDL_Surface *graphics_image = NULL;
static SDL_Surface *graphics_image_sub[MAX_SUBFONTS] = {NULL};

/* These variables are computed at image load (in 'init_sdl3'). */
static uint16_t graphics_tile_wid, graphics_tile_hgt;
/* Tiles per row. */
static int16_t graphics_image_tpr;
static int16_t graphics_image_tpr_sub[MAX_SUBFONTS] = {0};
/* Masks per tile. */
static uint8_t graphics_image_mpt;
/* Tiles per coordinate. */
static uint8_t graphics_image_tpc;
/* Mask colors loaded from prefs (0 means undefined). */
uint32_t graphics_image_masks_colors_prefs[GRAPHICS_MAX_MPT];
uint32_t graphics_image_masks_colors_sub_prefs[MAX_SUBFONTS][GRAPHICS_MAX_MPT];
/* Array of mask colors in RGBA format used in layer separation. The order is background, foreground, outline mask color. */
static uint32_t graphics_image_masks_colors[GRAPHICS_MAX_MPT];
static uint32_t graphics_image_masks_colors_sub[MAX_SUBFONTS][GRAPHICS_MAX_MPT];
/* Reinitialize graphic tiles for all windows after sdl3_graphics_pref_file_processed(). */
static bool graphics_reinitialize;
/* If there is any mask color set in prefs. */
static bool graphics_image_prefs_has_masks = false;
/* If an outline color is set in the pref file. */
static bool graphics_image_prefs_has_outline = false;
/* If there is any mask color set in prefs for subtileset. */
static bool graphics_image_sub_prefs_has_masks[MAX_SUBFONTS] = {false};
/* If an outline color is set in the pref file for subtileset. */
static bool graphics_image_sub_prefs_has_outline[MAX_SUBFONTS] = {false};

/* Tileset caching definitions and variables.
 * Note: Keep writes enabled during the provisional initialization before graphics preferences are processed. Caching that work deliberately speeds up the first startup; finalized rawpict/mask variants get distinct context keys.
 * Note: Bump SDL3_TILESET_CACHE_VERSION after code changes in tileset caching that make previously data incompatible. */
 #define SDL3_TILESET_CACHE_VERSION 1
static cptr ANGBAND_USER_DIR_XTRA_GRAPHICS_CACHE = NULL;
static uint32_t graphics_image_hash;
static uint32_t graphics_image_sub_hash[MAX_SUBFONTS];

 #if defined(GRAPHICS_BG_MASK)

bool sdl3_apply_graphics_image_force_outline(int value) {
	if (value < -1) value = -1;
	if (value > SDL3_FORCE_OUTLINE_MAX_RADIUS) value = SDL3_FORCE_OUTLINE_MAX_RADIUS;

	if (sdl3_graphics_image_force_outline == value) return(true);

	sdl3_graphics_image_force_outline = value;

	if (use_graphics == UG_NONE) return(true);
	if (graphics_image == NULL) return(true);

	graphics_reinitialize = true;
	return(sdl3_graphics_pref_file_processed());
}

 #endif

bool sdl3_apply_graphics_resize_type(int value) {
	int old_gfx_resize_type = gfx_resize_type;

	if (value < 0) value = 0;
	if (value >= INTERPOLATION_TYPES_COUNT) value = INTERPOLATION_LINEAR;
	if (gfx_resize_type == value) return(true);

	gfx_resize_type = value;

	if (use_graphics == UG_NONE) return(true);
	if (graphics_image == NULL) return(true);

	graphics_reinitialize = true;
	if (!sdl3_graphics_pref_file_processed()) {
		gfx_resize_type = old_gfx_resize_type;
		graphics_reinitialize = true;
		if (!sdl3_graphics_pref_file_processed()) {
			quit_fmt("Failed to reinitialize graphics after failed resize type change");
		}
		return(false);
	}

	return(true);
}

static errr term_data_enable_graphics(int index, term_data *td);
static errr term_data_init_graphic_tiles(term_data *td);
static errr Term_pict_sdl3_2mask(int x, int y, byte a, char32_t c, byte a_back, char32_t c_back);
static errr Term_pict_sdl3(int x, int y, byte a, char32_t c);
static errr Term_rawpict_sdl3(int x, int y, int c);
static uint32_t graphics_default_mask(uint8_t n);
static void sdl3_ensure_graphics_mask_colors(void);

/* Gets called after graphics image pref file is loaded. */
bool sdl3_graphics_pref_file_processed() {
	bool success = true;

	if (use_graphics) {
		sdl3_ensure_graphics_mask_colors();
	}
	if (use_graphics && graphics_reinitialize) {
		if (!graphics_image_prefs_has_masks) {
			fprintf(stderr, "Warning: Masks colors are not set in pref file. Using default values%s.\n", graphics_image_mpt == 3 ? " and compatibility mode" : "");
		}

		/* Initialize graphics for each initialized term. */
		for (int i = 0; i < ANGBAND_TERM_MAX; i++) {
			if (!term_get_visibility(i)) continue;

			if (term_data_enable_graphics(i, term_idx_to_term_data(i))) {
				logprint(format("Couldn't prepare images for terminal %d after graphics pref file processed, disabling graphics.\n", i));
				success = false;
			}
		}

		graphics_reinitialize = false;
	}

	return(success);
}

 #ifdef TILE_CACHE_SIZE
/*
 * Search for a tile with provided index and colors.
 * Returns a surface for that tile if found; otherwise returns NULL.
 * Assumes the colors array size is graphics_image_mpt.
 * The first input color (background) is ignored in cache comparisons.
 */
static SDL_Surface* graphics_tile_cache_search(term_data *td, char32_t index, uint32_t colors[], int subset) {
	struct tile_cache_entry *entry;
	int i;

	for (i = 0; i < TILE_CACHE_SIZE; i++) {
		entry = &td->tile_cache[i];
		if (entry->tile == NULL || entry->colors == NULL || entry->ncolors == 0) continue;
		if (entry->index == index && entry->subset == subset && memcmp(entry->colors, &colors[1], sizeof(uint32_t) * entry->ncolors) == 0) {
			return(entry->tile);
		}
	}
	return(NULL);
}

/*
 * Store the index, subset, and colors in a new entry and return a blank surface
 * to draw the cached tile into.
 * Assumes the colors array size is graphics_image_mpt.
 * Cache storage is limited (TILE_CACHE_SIZE) and circular (FIFO replacement).
 * The first color (background) is ignored for storage/comparison.
 */
static SDL_Surface* graphics_tile_cache_new(term_data *td, char32_t index, uint32_t colors[], int subset) {
	struct tile_cache_entry *entry;

	/* Replace valid cache entries in FIFO order. */
	entry = &td->tile_cache[td->cache_position++];
	if (td->cache_position >= TILE_CACHE_SIZE) td->cache_position = 0;

	/* Fill entry values and clear cached surface. */
	entry->index = index;
	entry->subset = subset;
	if (entry->colors && entry->ncolors > 0) {
		memcpy(entry->colors, &colors[1], sizeof(uint32_t) * entry->ncolors);
	}
	if (entry->tile) {
		SDL_FillSurfaceRect(entry->tile, NULL, 0x00000000);
	}

	return(entry->tile);
}
 #endif

static void draw_colored_layers_to_surface(SDL_Rect dst_rect, SDL_Surface *dst, uint8_t n, SDL_Rect src_rect, SDL_Surface *layers[], Pixel colors[], SDL_Surface *preparation) {
	for (uint32_t i = 0; i < n; i++) {
		if (colors[i].a == 0) continue;

		/* Draw mask of i-th layer in desired color to preparation surface. */
		SDL_FillSurfaceRect(preparation, NULL, SDL_MapRGBA(SDL_GetPixelFormatDetails(preparation->format), NULL, Pixel_quadruplet(colors[i])));
		SDL_BlitSurface(layers[i], &src_rect, preparation, NULL);

		/* Draw preparation surface to destination. */
		SDL_BlitSurface(preparation, NULL, dst, &dst_rect);
	}
}
static void term_data_draw_graphics_tile(term_data *td, int x, int y, char32_t index, Pixel colors[], int subset) {
	SDL_Rect dst_rect;
	SDL_Surface *dst_surface;

	if (subset < 0 || subset >= MAX_SUBFONTS || !td->tiles_layers_sub[subset]) subset = -1;
 #ifdef TILE_CACHE_SIZE
	dst_rect = (SDL_Rect){0, 0, td->fnt->wid, td->fnt->hgt};
	dst_surface = graphics_tile_cache_search(td, index, colors, subset);
	if ( dst_surface != NULL) {
		SDL_BlitSurface(dst_surface, NULL, td->win->surface, &(SDL_Rect){x, y, td->fnt->wid, td->fnt->hgt});
		return;
	}
	dst_surface = graphics_tile_cache_new(td, index, colors, subset);
 #else
	dst_rect = (SDL_Rect){x, y, td->fnt->wid, td->fnt->hgt};
	dst_surface = td->win->surface;
 #endif

	SDL_Surface **layers = (subset >= 0 ? td->tiles_layers_sub[subset] : td->tiles_layers);
	int tpr = (subset >= 0 && graphics_image_tpr_sub[subset] > 0 ? graphics_image_tpr_sub[subset] : graphics_image_tpr);
	uint32_t srcX = (index % tpr) * td->fnt->wid;
	uint32_t srcY = (index / tpr) * td->fnt->hgt;

	draw_colored_layers_to_surface(dst_rect, dst_surface, td->nlayers, (SDL_Rect){srcX, srcY, td->fnt->wid, td->fnt->hgt}, layers, &colors[1], td->tilePreparation);
 #ifdef TILE_CACHE_SIZE
	SDL_BlitSurface(dst_surface, NULL, td->win->surface, &(SDL_Rect){x, y, td->fnt->wid, td->fnt->hgt});
 #endif
}
/*
 * Draw a tile built from indexes/colors onto terminal surface coordinates.
 * Assumes indexes length == graphics_image_tpc and colors length ==
 * graphics_image_tpc * graphics_image_mpt (and td->nlayers == graphics_image_mpt - 1).
 * Assumes colors use td->win->surface->format (via SDL_MapRGBA).
 * Only index 0 background color is used; background colors for other indexes are ignored.
 * Tiles with index 0xFFFFFFFF are skipped.
 * The subsets array, if provided, must have the same length as indexes.
 */
static errr term_data_draw_graphics_tiles(term_data *td, int x, int y, char32_t *indexes, Pixel colors[], int *subsets) {
	/* Draw a rectangle filled with background color of the bottom tile. */
	SDL_FillSurfaceRect(td->win->surface, &(SDL_Rect){x, y, td->fnt->wid, td->fnt->hgt}, SDL_MapRGBA(SDL_GetPixelFormatDetails(td->win->surface->format), NULL, Pixel_quadruplet(colors[0])));

	for (uint32_t i = 0; i < graphics_image_tpc; i++) {
		if (indexes[i] == 0xFFFFFFFF) continue;
		term_data_draw_graphics_tile(td, x, y, indexes[i], &colors[i * graphics_image_mpt], subsets ? subsets[i] : -1);
	}
	return(0);
}

static bool Term_pict_sdl3_show_error = true;

/*
 * Draw some graphical characters.
 */
static errr Term_pict_sdl3(int x, int y, byte a, char32_t c) {

	if (graphics_image_mpt != 2 || graphics_image_tpc != 1) {
		if (Term_pict_sdl3_show_error) fprintf(stderr, "ERROR: Term_pict_sdl3: masks per tile %d or tiles per coord %d is wrong, should be %d, %d\n", graphics_image_mpt, graphics_image_tpc, 2, 1);
		Term_pict_sdl3_show_error = false;
		return(Infofnt_text_std(x, y, " ", 1));
	}

	term_data *td;

	/* Catch use in chat instead of as feat attr or we crash. */
	/* Screen pad left check: In case it is used in the status bar for some reason. */
	/* Screen pad top check: Main screen top chat line or status line. */
	if (Term && Term->data == &term_main && x >= SCREEN_PAD_LEFT && x < SCREEN_PAD_LEFT + SCREEN_WID && y >= SCREEN_PAD_TOP && y < SCREEN_PAD_TOP + SCREEN_HGT) {
		flick_global_x = x;
		flick_global_y = y;
	} else flick_global_x = 0;

	a = term2attr(a);

 #ifndef EXTENDED_COLOURS_PALANIM
  #ifndef EXTENDED_BG_COLOURS
	Infoclr_set(clr[a & 0x0F]);
  #else
	/* Undefined case actually, we don't want to have a hole in the colour array (0..15 and then 32..32+x). */
	Infoclr_set(clr[a & 0x1F]);
  #endif
 #else
  #ifndef EXTENDED_BG_COLOURS
	Infoclr_set(clr[a & 0x1F]);
  #else
	Infoclr_set(clr[a & 0x3F]);
  #endif
 #endif

	if (Pixel_equal(Infoclr->fg, Infoclr->bg)) {
		/* Foreground color is the same as background color. If this was text, the tile would be rendered as solid block of color. */
		/* But an image tile could contain some other color pixels and could result in no solid color tile. That's why paint a solid block as intended. */
		return(Infofnt_text_std(x, y, " ", 1));
	}

	td = (term_data*)(Term->data);
	x *= Infofnt->wid;
	y *= Infofnt->hgt;

	term_data_draw_graphics_tiles(td, td->win->bw + x, td->win->bh + y, (char32_t[]){c - MAX_FONT_CHAR - 1}, (Pixel[]){Infoclr->bg, Infoclr->fg}, (int[]){ ((c >= 0 && c < MAX_GFX_TILES) ? c_subtileset[c] : -1) });
	return(0);
}

 #ifdef GRAPHICS_BG_MASK
static bool Term_pict_sdl3_2mask_show_error = true;

static errr Term_pict_sdl3_2mask(int x, int y, byte a, char32_t c, byte a_back, char32_t c_back) {

	if (graphics_image_mpt != 3 || graphics_image_tpc != 2) {
		if (Term_pict_sdl3_2mask_show_error) fprintf(stderr, "ERROR: Term_pict_sdl3_2mask: masks per tile %d or tiles per coordinate %d is wrong, should be %d, %d\n", graphics_image_mpt, graphics_image_tpc, 3, 2);
		Term_pict_sdl3_2mask_show_error = false;
		return(Term_pict_sdl3(x, y, a, c));
	}

	term_data *td;
	char32_t indexes[GRAPHICS_MAX_TPC] = {0};
	Pixel colors[GRAPHICS_MAX_MPT * GRAPHICS_MAX_TPC] = {0};
	int subsets[GRAPHICS_MAX_TPC] = {-1};

	/* Catch use in chat instead of as feat attr, or we crash. :-s */
	/* (term-idx 0 is the main window; screen-pad-left check: In case it is used in the status bar for some reason; screen-pad-top checks: main screen top chat line or status line). */
	if (Term && Term->data == &term_main && x >= SCREEN_PAD_LEFT && x < SCREEN_PAD_LEFT + SCREEN_WID && y >= SCREEN_PAD_TOP && y < SCREEN_PAD_TOP + SCREEN_HGT) {
		flick_global_x = x;
		flick_global_y = y;
	} else flick_global_x = 0;

	/* Avoid visual glitches while not in 2mask mode */
	if (use_graphics != UG_2MASK) {
		a_back = TERM_DARK;
   #if 0
		c_back = 32; /* space! NOT zero! */
   #else
		c_back = Client_setup.f_char[FEAT_SOLID]; /* 'graphical space' for erasure */
   #endif
	}

	/* SPACE - Erase background, aka black background. This is for places where we have no bg-info, such as client-lore in knowledge menu. */
	if (c_back == 32) a_back = TERM_DARK;

	a_back = term2attr(a_back);
  #ifndef EXTENDED_COLOURS_PALANIM
   #ifndef EXTENDED_BG_COLOURS
	Infoclr_set(clr[a_back & 0x0F]);
   #else
	/* Undefined case actually, we don't want to have a hole in the colour array (0..15 and then 32..32+x). */
	Infoclr_set(clr[a_back & 0x1F]);
   #endif
  #else
   #ifndef EXTENDED_BG_COLOURS
	Infoclr_set(clr[a_back & 0x1F]);
   #else
	Infoclr_set(clr[a_back & 0x3F]);
   #endif
  #endif
	if (c_back == 32 || c_back == 0) {
		/* Background tile is an empty space and should not be painted. */
		indexes[0] = 0xFFFFFFFF;
	} else {
		indexes[0] = c_back - MAX_FONT_CHAR - 1;
	}
	/* Background color. */
	colors[0] = Infoclr->bg;
	/* Foreground color. */
	colors[1] = Infoclr->fg;
	/* Outline color. Set to background color. */
	colors[2] = Infoclr->bg;

	if (c_back >= 0 && c_back < MAX_GFX_TILES) subsets[0] = c_subtileset[c_back];

	a = term2attr(a);
  #ifndef EXTENDED_COLOURS_PALANIM
   #ifndef EXTENDED_BG_COLOURS
	Infoclr_set(clr[a & 0x0F]);
   #else
	/* Undefined case actually, we don't want to have a hole in the colour array (0..15 and then 32..32+x). */
	Infoclr_set(clr[a & 0x1F]);
   #endif
  #else
   #ifndef EXTENDED_BG_COLOURS
	Infoclr_set(clr[a & 0x1F]);
   #else
	Infoclr_set(clr[a & 0x3F]);
   #endif
  #endif
	indexes[1] = c - MAX_FONT_CHAR - 1;
	/* Background color. For all tiles except bottom tile background color is ignored. Set to fully transparent so we don't overpaint terrain. */
	colors[graphics_image_mpt + 0] = (Pixel){0x00, 0x00, 0x00, 0x00};
	/* Foreground color. */
	colors[graphics_image_mpt + 1] = Infoclr->fg;
	/* Outline color. Use semi-transparent black. */
	colors[graphics_image_mpt + 2] = (Pixel){0, 0, 0, OUTLINE_ALPHA};

	if (c >= 0 && c < MAX_GFX_TILES) subsets[1] = c_subtileset[c];

	if (Pixel_equal(Infoclr->fg, Infoclr->bg)) {
		/* Foreground color is the same as background color. If this was text, the tile would be rendered as solid block of color. */
		/* But an image tile could contain some other color pixels and could result in no solid color tile. That's why paint a solid block as intended. */
		return(Infofnt_text_std(x, y, " ", 1));
	}

	td = (term_data*)(Term->data);
	x *= Infofnt->wid;
	y *= Infofnt->hgt;

	return term_data_draw_graphics_tiles(td, td->win->bw + x, td->win->bh + y, indexes, colors, subsets);
}
 #endif /* GRAPHICS_BG_MASK */

static errr Term_rawpict_sdl3(int x, int y, int c) {
	term_data *td = (term_data*)(Term->data);

	if (!td) return(0);
	if (c < 0 || c > MAX_TILES_RAWPICT) return(0);

	int subset = tiles_rawpict_subtileset[c];
	rawpict_tile trp;
	SDL_Surface *surface = NULL;

	if (subset >= 0 && subset < MAX_SUBFONTS && td->tiles_surface_sub[subset]) {
		trp = td->tiles_rawpict_sub[subset][c];
		surface = td->tiles_surface_sub[subset];
	} else if (td->tiles_surface) {
		trp = td->tiles_rawpict[c];
		surface = td->tiles_surface;
	}

	if (!surface) return(0);

	if (!trp.defined) {
		return(Infofnt_text_std(x, y, " ", 1));
	}
	if (trp.w <= 0 || trp.h <= 0) return(0);

	int px = td->win->bw + x * Infofnt->wid;
	int py = td->win->bh + y * Infofnt->hgt;

	SDL_Rect src = { trp.x, trp.y, trp.w, trp.h };
	SDL_Rect dst = { px, py, src.w, src.h };

	SDL_BlitSurface(surface, &src, td->win->surface, &dst);

	return(0);
}

static Pixel sdl3_tileset_preview_mask_color(int type, int index, int count) {
	if (index < 0) index = 0;
	if (index >= count) index = count - 1;

	int denom = (count > 1) ? (count - 1) : 1;
	uint8_t col = 255 - (index * (255 / denom));

	Pixel color = {col, col, col, 255};
	switch (type) {
		case 0: return (Pixel){0, col, 0, 255};
		case 1: return (Pixel){0, 0, col, 255};
		case 2: return (Pixel){col, 0, 0, 255};
	}

	return(color);
}

static term_data *sdl3_tileset_preview_term(void) {
	term_data *td = term_idx_to_term_data(0);

	if (!td || !td->win || !td->win->surface || !td->fnt) return(NULL);
	return(td);
}

bool sdl3_tileset_preview_ready(void) {
	term_data *td = sdl3_tileset_preview_term();

	if (!td) return(false);
	if (!use_graphics) return(false);
	if (td->tiles_layers == NULL || td->nlayers <= 0) return(false);

	return(true);
}

void sdl3_tileset_preview_fill_cell(int col, int row, int count, uint8_t background_value) {
	term_data *td = sdl3_tileset_preview_term();

	if (!td) return;
	if (count <= 0) return;

	SDL_Rect rect = {td->win->bw + col * td->fnt->wid, td->win->bh + row * td->fnt->hgt, count * td->fnt->wid, td->fnt->hgt};
	SDL_FillSurfaceRect(td->win->surface, &rect, SDL_MapRGBA(SDL_GetPixelFormatDetails(td->win->surface->format), NULL, background_value, background_value, background_value, 255));
}

void sdl3_tileset_preview_draw_tile(int col, int row, int type, char32_t tile_char, char32_t background_char) {
	term_data *td = sdl3_tileset_preview_term();

	sdl3_tileset_preview_fill_cell(col, row, 4 + (2 * GRAPHICS_MAX_MPT), 0);
	sdl3_tileset_preview_fill_cell(col, row + 1, 4 + (2 * GRAPHICS_MAX_MPT), 0);

	if (!td || !sdl3_tileset_preview_ready()) {
		return;
	}

	if (tile_char <= MAX_FONT_CHAR) {
		return;
	}
	int tile_index = tile_char - MAX_FONT_CHAR - 1;
	int tile_subset = -1;
	if (tile_char >= 0 && tile_char < MAX_GFX_TILES) {
		tile_subset = c_subtileset[tile_char];
	}

	int mask_count = td->nlayers;
	if (mask_count <= 0) {
		return;
	}


	Pixel colors[GRAPHICS_MAX_TPC * GRAPHICS_MAX_MPT];
	WIPE(colors, colors);
	/* Set and foreground color depending on type. */
	for (int i = 0; i < mask_count; i++) {
		colors[1 + i] = sdl3_tileset_preview_mask_color(type, i, mask_count);
	}

	for (int i = 0; i < 2; i++) {
		/* Draw the requested tile on black/white background, with outline of the same color. */
		colors[0] = (Pixel){255*(i%2), 255*(i%2), 255*(i%2), 255};
		/* The last mask is outline. Make it background color. */
		if (mask_count > 1) colors[1 + mask_count - 1] = colors[0];
		int px = td->win->bw + (col + i) * td->fnt->wid;
		int py = td->win->bh + row * td->fnt->hgt;
		SDL_FillSurfaceRect(td->win->surface, &(SDL_Rect){px, py, td->fnt->wid, td->fnt->hgt}, SDL_MapRGBA(SDL_GetPixelFormatDetails(td->win->surface->format), NULL, Pixel_quadruplet(colors[0])));
		term_data_draw_graphics_tile(td, px, py, tile_index, colors, tile_subset);
	}

	uint32_t srcX = (tile_index % graphics_image_tpr) * td->fnt->wid;
	uint32_t srcY = (tile_index / graphics_image_tpr) * td->fnt->hgt;
	uint32_t dstY = td->win->bh + row * td->fnt->hgt;
	Uint32 fill = SDL_MapRGBA(SDL_GetPixelFormatDetails(td->win->surface->format), NULL, 0, 0, 0, 255);
	for (int l = 0; l < td->nlayers; l++) {
		uint32_t dstX = td->win->bw + (col + 4 + (2*l)) * td->fnt->wid;
		SDL_Rect src_rect = {srcX, srcY, td->fnt->wid, td->fnt->hgt};
		SDL_Rect dst_rect = {dstX, dstY, td->fnt->wid, td->fnt->hgt};
		SDL_FillSurfaceRect(td->win->surface, &dst_rect, fill);
		draw_colored_layers_to_surface(dst_rect, td->win->surface, 1, src_rect, &td->tiles_layers[l], &((Pixel){255, 0, 255, 255}), td->tilePreparation);
	}

	if (background_char <= MAX_FONT_CHAR || background_char == tile_char) return;
	if (graphics_image_tpc <= 1) return;
	int background_subset = -1;
	if (background_char >= 0 && background_char < MAX_GFX_TILES) {
		background_subset = c_subtileset[background_char];
	}

	WIPE(colors, colors);
	char32_t indexes[GRAPHICS_MAX_TPC];
	int subtiles[GRAPHICS_MAX_TPC];
	for (int i = 0; i < GRAPHICS_MAX_TPC; i++) {
		indexes[i] = 0xFFFFFFFF;
		subtiles[i] = -1;
	}

	indexes[0] = background_char - MAX_FONT_CHAR - 1;
	indexes[1] = tile_index;
	subtiles[0] = background_subset;
	subtiles[1] = tile_subset;

	for (int i = 0; i < mask_count; i++) {
		/* Bottom tile colors. */
		colors[1 + i] = sdl3_tileset_preview_mask_color(0, i, mask_count);
		/* Top tile colors. */
		colors[graphics_image_mpt + 1 + i] = sdl3_tileset_preview_mask_color(type, i, mask_count);
	}
	/* Background color for top tile. */
	colors[graphics_image_mpt] = (Pixel){0, 0, 0, 0};

	for (int i = 0; i < 2; i++) {
		/* Draw the requested tile on black/white background, with outline of the same color. */
		colors[0] = (Pixel){255*((i+1)%2), 255*((i+1)%2), 255*((i+1)%2), 255};
		/* The next masks are outline. Make it background color. */
		if (mask_count > 1) {
			/* Outline color for bottom tile (background color of bottom tile). */
			colors[1 + mask_count - 1] = colors[0];
			/* Outline color for top tile (semi-transparent black). */
			colors[graphics_image_mpt + mask_count] = (Pixel){0, 0, 0, OUTLINE_ALPHA};
		}

		int px = td->win->bw + (col + i) * td->fnt->wid;
		int py = td->win->bh + (row + 1) * td->fnt->hgt;
		term_data_draw_graphics_tiles(td, px, py, indexes, colors, subtiles);
	}
}

void tiles_rawpict_scale(void) {
	for (int t = 0; t < ANGBAND_TERM_MAX; t++) {
		term_data *td = term_idx_to_term_data(t);
		int width1 = td->rawpict_scale_wid_org;
		int height1 = td->rawpict_scale_hgt_org;
		int width2 = td->rawpict_scale_wid_use;
		int height2 = td->rawpict_scale_hgt_use;

		if (width1 && height1) {
			for (int i = 0; i <= MAX_TILES_RAWPICT; i++) {
				if (!tiles_rawpict_org[i].defined) {
					td->tiles_rawpict[i].defined = false;
					continue;
				}
				td->tiles_rawpict[i].defined = true;
				td->tiles_rawpict[i].x = (tiles_rawpict_org[i].x * width2) / width1;
				td->tiles_rawpict[i].y = (tiles_rawpict_org[i].y * height2) / height1;
				td->tiles_rawpict[i].w = (tiles_rawpict_org[i].w * width2) / width1;
				td->tiles_rawpict[i].h = (tiles_rawpict_org[i].h * height2) / height1;
			}
		}

		for (int sub = 0; sub < MAX_SUBFONTS; sub++) {
			if (!td->tiles_surface_sub[sub]) continue;

			width1 = td->rawpict_scale_wid_org_sub[sub];
			height1 = td->rawpict_scale_hgt_org_sub[sub];
			width2 = td->rawpict_scale_wid_use_sub[sub];
			height2 = td->rawpict_scale_hgt_use_sub[sub];

			if (!width1 || !height1) continue;

			for (int i = 0; i <= MAX_TILES_RAWPICT; i++) {
				if (!tiles_rawpict_org_sub[sub][i].defined) {
					td->tiles_rawpict_sub[sub][i].defined = false;
					continue;
				}
				td->tiles_rawpict_sub[sub][i].defined = true;
				td->tiles_rawpict_sub[sub][i].x = (tiles_rawpict_org_sub[sub][i].x * width2) / width1;
				td->tiles_rawpict_sub[sub][i].y = (tiles_rawpict_org_sub[sub][i].y * height2) / height1;
				td->tiles_rawpict_sub[sub][i].w = (tiles_rawpict_org_sub[sub][i].w * width2) / width1;
				td->tiles_rawpict_sub[sub][i].h = (tiles_rawpict_org_sub[sub][i].h * height2) / height1;
			}
		}
	}
}

static color_rgb sdl3_surface_get_pixel_rgb(SDL_Surface *surface, const uint32_t *pixels, int pitch, int x, int y) {
	Uint8 red, green, blue, alpha;
	color_rgb color;

	SDL_GetRGBA(pixels[y * pitch + x], SDL_GetPixelFormatDetails(surface->format), NULL, &red, &green, &blue, &alpha);
	color.red = red;
	color.green = green;
	color.blue = blue;

	(void)alpha;
	return(color);
}

static bool sdl3_surface_pixel_is_mask(SDL_Surface *surface, const uint32_t *pixels, int pitch, int x, int y, const uint32_t mask_colors[]) {
	color_rgb sample = sdl3_surface_get_pixel_rgb(surface, pixels, pitch, x, y);

	for (uint8_t i = 0; i < graphics_image_mpt; i++) {
		if (mask_colors[i] == 0) continue;
		if (sample.red == ((mask_colors[i] >> 24) & 0xFF) &&
		    sample.green == ((mask_colors[i] >> 16) & 0xFF) &&
		    sample.blue == ((mask_colors[i] >> 8) & 0xFF)) {
			return(true);
		}
	}

	return(false);
}

/*
 * Check if pixel color `c` matches any entry in the `mask_colors` array.
 * Returns true on match, false otherwise. If `mask_colors` is NULL, returns false immediately.
 */
static bool is_mask_color(Pixel c, const uint32_t mask_colors[]) {
	if (mask_colors == NULL) return(false);

	for (uint8_t i = 0; i < graphics_image_mpt; i++) {
		if (mask_colors[i] == 0) continue;
		if (c.r == ((mask_colors[i] >> 24) & 0xFF) &&
		    c.g == ((mask_colors[i] >> 16) & 0xFF) &&
		    c.b == ((mask_colors[i] >> 8) & 0xFF)) {
			return(true);
		}
	}
	return(false);
}

/*
 * Clamp `value` to the range [min_value, max_value].
 */
static int cofineInteger(int value, int min_value, int max_value) {
	if (value < min_value) return min_value;
	if (value > max_value) return max_value;
	return value;
}

static int surface_bytes_per_pixel(SDL_Surface *surface) {
	const SDL_PixelFormatDetails *details = SDL_GetPixelFormatDetails(surface->format);
	return(details ? details->bytes_per_pixel : 4);
}

static Uint32 surface_alpha_mask(SDL_Surface *surface) {
	const SDL_PixelFormatDetails *details = SDL_GetPixelFormatDetails(surface->format);
	return(details ? details->Amask : 0xFF000000);
}

/*
 * Scales source surface containing tiles using nearest neighbour interpolation.
 * Assumes source surface width/height are divisible by source tile width/height.
 * The returned surface keeps the source surface format.
 */
SDL_Surface *ScaleSurfaceNearest(SDL_Surface *src, uint16_t src_tile_wid, uint16_t src_tile_hgt, uint16_t dst_tile_wid, uint16_t dst_tile_hgt) {
	if (!src || !src->format) return NULL;

	uint16_t new_wid = (src->w / src_tile_wid) * dst_tile_wid;
	uint16_t new_hgt = (src->h / src_tile_hgt) * dst_tile_hgt;
	SDL_Surface *dst;
	int src_pitch, dst_pitch;
	int dst_x, dst_y, src_x, src_y, tile_x, tile_y;

	/* Create resulting surface. */
	dst = SDL_CreateSurface(new_wid, new_hgt, SDL_PIXELFORMAT_RGBA32);
	if (!dst) {
		fprintf(stderr, "Error creating scaled surface: %s\n", SDL_GetError());
		return NULL;
	}
	src_pitch = src->pitch / surface_bytes_per_pixel(src);
	dst_pitch = dst->pitch / surface_bytes_per_pixel(dst);

	SDL_LockSurface(src);
	SDL_LockSurface(dst);

	/* Fill scaled pixels. */
	for (dst_y = 0; dst_y < new_hgt; ++dst_y) {
		tile_y = dst_y / dst_tile_hgt;
		src_y = cofineInteger((int)round((dst_y * src->h) / (float)new_hgt), tile_y * src_tile_hgt, ((tile_y + 1) * src_tile_hgt) - 1);
		for (dst_x = 0; dst_x < new_wid; ++dst_x) {
			tile_x = dst_x / dst_tile_wid;
			src_x = cofineInteger((int)round((dst_x * src->w) / (float)new_wid), tile_x * src_tile_wid, ((tile_x + 1) * src_tile_wid) - 1);

			((uint32_t*)dst->pixels)[dst_y * dst_pitch + dst_x] = ((uint32_t*)src->pixels)[src_y * src_pitch + src_x];
		}
	}

	/* Unlock surfaces. */
	SDL_UnlockSurface(dst);
	SDL_UnlockSurface(src);

	return dst;
}

/* Feed an integer to SDL_crc32 in a stable byte order. */
static uint32_t SDL_crc32_u32(uint32_t hash, uint32_t value) {
	const uint8_t bytes[4] = {
		(uint8_t)(value & 0xFF),
		(uint8_t)((value >> 8) & 0xFF),
		(uint8_t)((value >> 16) & 0xFF),
		(uint8_t)((value >> 24) & 0xFF)
	};

	return(SDL_crc32(hash, bytes, sizeof(bytes)));
}

/*
 * Compute a stable fingerprint of the normalized RGBA32 source image.
 * It is part of the cache filename so replacing a tileset without renaming it cannot reuse stale scaled data.
 */
static uint32_t SDL_surface_hash(SDL_Surface *surface) {
	if (!surface || surface->format != SDL_PIXELFORMAT_RGBA32) return(0);

	const uint8_t *row;
	uint32_t hash = 0;
	int bytes_per_row;
	bool locked = false;

	if (SDL_MUSTLOCK(surface)) {
		if (!SDL_LockSurface(surface)) return(0);
		locked = true;
	}

	hash = SDL_crc32_u32(hash, surface->w);
	hash = SDL_crc32_u32(hash, surface->h);
	bytes_per_row = surface->w * surface_bytes_per_pixel(surface);
	for (int y = 0; y < surface->h; y++) {
		row = (const uint8_t*)surface->pixels + y * surface->pitch;
		hash = SDL_crc32(hash, row, bytes_per_row);
	}

	if (locked) SDL_UnlockSurface(surface);
	return(hash);
}

/* Hash the rawpict definitions that affect filtered sampling boundaries. */
static uint32_t rawpict_hash(const rawpict_tile rawpict_tiles[]) {
	if (!rawpict_tiles) return(0);

	uint32_t hash = 0;
	for (int i = 0; i <= MAX_TILES_RAWPICT; i++) {
		if (!rawpict_tiles[i].defined) continue;
		hash = SDL_crc32_u32(hash, i);
		hash = SDL_crc32_u32(hash, rawpict_tiles[i].x);
		hash = SDL_crc32_u32(hash, rawpict_tiles[i].y);
		hash = SDL_crc32_u32(hash, rawpict_tiles[i].w);
		hash = SDL_crc32_u32(hash, rawpict_tiles[i].h);
	}
	return(hash);
}

/* Hash the mask colors omitted while producing filtered base-layer pixels. */
static uint32_t mask_colors_hash(const uint32_t mask_colors[]) {
	if (!mask_colors) return(0);

	uint32_t hash = SDL_crc32_u32(0, graphics_image_mpt);
	for (uint8_t i = 0; i < graphics_image_mpt; i++) {
		hash = SDL_crc32_u32(hash, mask_colors[i]);
	}
	return(hash);
}

/*
 * Load a scaled sheet variant from the shared SDL3 tileset disk cache.
 * The `context_hash` distinguishes results affected by rawpict boundaries or mask colors in addition to the source image and destination tile dimensions.
 */
static SDL_Surface *tileset_cache_load(SDL_Surface *src, uint16_t src_tile_wid, uint16_t src_tile_hgt, uint16_t dst_tile_wid, uint16_t dst_tile_hgt, uint32_t source_hash, int subset, const char *variant, uint32_t context_hash, char cache_path[], int cache_path_size) {
	cache_path[0] = '\0';

	if (disable_tileset_caching || !source_hash) return(NULL);

	SDL_Surface *cached = NULL;
	SDL_Surface *converted;
	char cache_name[512];
	int expected_w, expected_h;

	expected_w = (src->w / src_tile_wid) * dst_tile_wid;
	expected_h = (src->h / src_tile_hgt) * dst_tile_hgt;
	strnfmt(cache_name, sizeof(cache_name),
			"%s%s.to-%dx%d.%s.sdl3-v%d.%08x.%08x.bmp",
			graphic_tiles,
			subset < 0 ? "" : format(".subset-%d", subset),
			dst_tile_wid, dst_tile_hgt,
			variant,
			SDL3_TILESET_CACHE_VERSION,
			(unsigned int)source_hash, (unsigned int)context_hash
			);
	path_build(cache_path, cache_path_size, ANGBAND_USER_DIR_XTRA_GRAPHICS_CACHE, cache_name);

	if (!my_fexists(cache_path)) return(NULL);

	cached = SDL_LoadBMP(cache_path);
	if (cached && (cached->w != expected_w || cached->h != expected_h)) {
		logprint(format("Cached image %s has unexpected dimensions: Got %dx%d, expexted %dx%d\n", cache_path, cached->w, cached->h, expected_w, expected_h));
		SDL_DestroySurface(cached);
		cached = NULL;
	}
	if (cached && cached->format != SDL_PIXELFORMAT_RGBA32) {
		converted = SDL_ConvertSurface(cached, SDL_PIXELFORMAT_RGBA32);
		SDL_DestroySurface(cached);
		cached = converted;
	}
	if (!cached) {
		logprint(format("SDL3 tileset cache invalid: %s\n", cache_path));
		return(NULL);
	}

 #ifdef SDL3_TILESET_CACHE_DEBUG
	logprint(format("SDL3 tileset cache hit: %s\n", cache_path));
 #endif
	return(cached);
}

/* Save the surface to .bmp file cache. */
static void tileset_cache_save(SDL_Surface *surface, const char *cache_path) {
	if (!surface || !cache_path || !cache_path[0]) return;

	char temp_name[64];
	char temp_path[1024];
	char error[256];

	/*
	 * Write to a unique file beside the final cache entry and rename it only after
	 * a complete save, so interrupted or concurrent writers cannot publish a
	 * partial BMP and publication stays atomic.
	 */
	strnfmt(temp_name, sizeof(temp_name), "tileset-%08x%08x.bmp", SDL_rand_bits(), SDL_rand_bits());
	path_build(temp_path, sizeof(temp_path), ANGBAND_USER_DIR_XTRA_GRAPHICS_CACHE, temp_name);
	if (!SDL_SaveBMP(surface, temp_path)) {
		strncpy(error, SDL_GetError(), sizeof(error));
		error[sizeof(error) - 1] = '\0';
		if (my_fexists(temp_path)) SDL_RemovePath(temp_path);
		logprint(format("Couldn't save SDL3 tileset cache %s: %s\n", cache_path, error));
		return;
	}

	if (!SDL_RenamePath(temp_path, cache_path)) {
		strncpy(error, SDL_GetError(), sizeof(error));
		error[sizeof(error) - 1] = '\0';
		SDL_RemovePath(temp_path);
		logprint(format("Couldn't publish SDL3 tileset cache %s: %s\n", cache_path, error));
		return;
	}

 #ifdef SDL3_TILESET_CACHE_DEBUG
	logprint(format("SDL3 tileset cache created: %s\n", cache_path));
 #endif
}
/*
 * Load the nearest-neighbour scaled sheet from disk, or create and cache it.
 */
static SDL_Surface *ScaleSurfaceNearestCached(SDL_Surface *src, uint16_t src_tile_wid, uint16_t src_tile_hgt, uint16_t dst_tile_wid, uint16_t dst_tile_hgt, int subset, uint32_t source_hash) {
 #ifdef SDL3_TILESET_CACHE_DEBUG
	Uint64 start_time = SDL_GetPerformanceCounter();
 #endif
	SDL_Surface *scaled;

	if (disable_tileset_caching || !source_hash) {
		scaled = ScaleSurfaceNearest(src, src_tile_wid, src_tile_hgt, dst_tile_wid, dst_tile_hgt);
 #ifdef SDL3_TILESET_CACHE_DEBUG
		logprint(format("ScaleSurfaceNearestCached: %dx%d->%dx%d,%d: disabled cache execution time: %f ms\n", src_tile_wid, src_tile_hgt, dst_tile_wid, dst_tile_hgt, subset, (double)(SDL_GetPerformanceCounter() - start_time) / (double)SDL_GetPerformanceFrequency() * 1000.0));
 #endif
		return scaled;
	}

	char cache_path[1024];

	scaled = tileset_cache_load(src, src_tile_wid, src_tile_hgt, dst_tile_wid, dst_tile_hgt, source_hash, subset, "nearest", 0, cache_path, sizeof(cache_path));
	if (scaled) {
 #ifdef SDL3_TILESET_CACHE_DEBUG
		logprint(format("ScaleSurfaceNearestCached: %dx%d->%dx%d,%d: cache hit execution time: %f ms\n", src_tile_wid, src_tile_hgt, dst_tile_wid, dst_tile_hgt, subset, (double)(SDL_GetPerformanceCounter() - start_time) / (double)SDL_GetPerformanceFrequency() * 1000.0));
 #endif
		return(scaled);
	}

	scaled = ScaleSurfaceNearest(src, src_tile_wid, src_tile_hgt, dst_tile_wid, dst_tile_hgt);
	tileset_cache_save(scaled, cache_path);
 #ifdef SDL3_TILESET_CACHE_DEBUG
	logprint(format("ScaleSurfaceNearestCached: %dx%d->%dx%d,%d: cache miss execution time: %f ms\n", src_tile_wid, src_tile_hgt, dst_tile_wid, dst_tile_hgt, subset, (double)(SDL_GetPerformanceCounter() - start_time) / (double)SDL_GetPerformanceFrequency() * 1000.0));
 #endif
	return(scaled);
}

/*
 * Separate defined tiles from `rawpict_tiles` into two arrays: `rawpict_defined` (original tiles) and `rawpict_scaled` (tiles with coordinates scaled to the destination tile size).
 * Returns the count of defined tiles.
 */
static int prepare_defined_and_scaled_rawpict_tiles(rawpict_tile *rawpict_tiles, int src_tile_wid, int src_tile_hgt, int dst_tile_wid, int dst_tile_hgt, rawpict_tile **rawpict_defined, rawpict_tile **rawpict_scaled) {
	if (rawpict_tiles == NULL) return 0;

	int count, i, rawpict_index;
	rawpict_tile *defined = NULL, *scaled = NULL;

	/* First pass: count defined tiles so we know how large to allocate the output arrays. */
	count = 0;
	for (i = 0; i <= MAX_TILES_RAWPICT; i++) {
		if (!rawpict_tiles[i].defined) continue;
		count++;
	}

	/* Nothing to process if no tiles are defined. */
	if (count == 0) return 0;

	/* Allocate output arrays for original and scaled tile data. */
	C_MAKE(defined, count, rawpict_tile);
	C_MAKE(scaled, count, rawpict_tile);

	/* Second pass: copy defined tiles and scale their coordinates to the destination size. */
	rawpict_index = 0;
	for (i = 0; i <= MAX_TILES_RAWPICT; i++) {
		if (!rawpict_tiles[i].defined) continue;

		defined[rawpict_index] = rawpict_tiles[i];
		scaled[rawpict_index].defined = true;
		scaled[rawpict_index].x = (rawpict_tiles[i].x * dst_tile_wid) / src_tile_wid;
		scaled[rawpict_index].y = (rawpict_tiles[i].y * dst_tile_hgt) / src_tile_hgt;
		scaled[rawpict_index].w = (rawpict_tiles[i].w * dst_tile_wid) / src_tile_wid;
		scaled[rawpict_index].h = (rawpict_tiles[i].h * dst_tile_hgt) / src_tile_hgt;

		rawpict_index++;
	}

	/* Return the allocated arrays via output pointers. */
	*rawpict_defined = defined;
	*rawpict_scaled = scaled;
	return count;
}

/*
 * Searches the scaled rawpict tiles for one covering the given pixel and returns that tile's original (unscaled) coordinates.
 * Falls back to the full surface bounds when no tile matches.
 */
static rectangle rawpict_bounds(int src_w, int src_h, int dst_x, int dst_y, int rawpict_defined_count, rawpict_tile *rawpict_defined, rawpict_tile *rawpict_scaled) {
	int i, rawpict_found = -1;

	for (i = 0; i < rawpict_defined_count; i++) {
		if ((dst_x >= rawpict_scaled[i].x) && (dst_x < (rawpict_scaled[i].x + rawpict_scaled[i].w)) && (dst_y >= rawpict_scaled[i].y) && (dst_y < (rawpict_scaled[i].y + rawpict_scaled[i].h))) {
			rawpict_found = i;
			break;
		}
	}

	if (rawpict_found != -1) return (rectangle){(coordinates){rawpict_defined[rawpict_found].x, rawpict_defined[rawpict_found].y}, (coordinates){rawpict_defined[rawpict_found].x + rawpict_defined[rawpict_found].w - 1, rawpict_defined[rawpict_found].y + rawpict_defined[rawpict_found].h - 1},};
	return (rectangle){(coordinates){0, 0}, (coordinates){src_w - 1, src_h - 1}};
}

/*
 * Scales source surface containing tiles using (bi)linear interpolation.
 * Assumes source surface width/height are divisible by source tile width/height.
 * The returned surface keeps the source surface format.
 */
static SDL_Surface *ScaleSurfaceLinear(SDL_Surface *src, uint16_t src_tile_wid, uint16_t src_tile_hgt, uint16_t dst_tile_wid, uint16_t dst_tile_hgt, rawpict_tile *rawpict_tiles, const uint32_t mask_colors[]) {
	if (!src || !src->format) return NULL;

	uint16_t new_wid = (src->w / src_tile_wid) * dst_tile_wid;
	uint16_t new_hgt = (src->h / src_tile_hgt) * dst_tile_hgt;
	SDL_Surface *dst = NULL;
	int src_pitch, dst_pitch;
	int rawpict_defined_count;
	rawpict_tile *rawpict_defined = NULL, *rawpict_scaled = NULL;
	int i;
	uint32_t *src_pixels = NULL, *dst_pixels = NULL;
	int dst_y, tile_y, y0, y1;
	int dst_x, tile_x, x0, x1;
	float src_y, fy, src_x, fx;
	rectangle bounds;
	coordinates p[4] = {0};
	float w[4] = {0};
	float r, g, b, a;
	Pixel c;

	dst = SDL_CreateSurface(new_wid, new_hgt, SDL_PIXELFORMAT_RGBA32);
	if (!dst) {
		fprintf(stderr, "Error creating linear scaled surface: %s\n", SDL_GetError());
		return NULL;
	}
	src_pitch = src->pitch / surface_bytes_per_pixel(src);
	dst_pitch = dst->pitch / surface_bytes_per_pixel(dst);

	/* Process raw tiles so there is no need to traverse whole rawpict_tiles array when scaling, just the enabled ones. Also scale for speedup. */
	rawpict_defined_count = prepare_defined_and_scaled_rawpict_tiles(rawpict_tiles, src_tile_wid, src_tile_hgt, dst_tile_wid, dst_tile_hgt, &rawpict_defined, &rawpict_scaled);

	SDL_LockSurface(src);
	SDL_LockSurface(dst);

	src_pixels = (uint32_t *)src->pixels;
	dst_pixels = (uint32_t *)dst->pixels;

	/* Scale pixel by pixel. */
	for (dst_y = 0; dst_y < new_hgt; dst_y++) {
		tile_y = dst_y / dst_tile_hgt;
		src_y = (dst_y * src->h) / (float)new_hgt;
		y0 = (int)floor(src_y);
		y1 = y0 + 1;
		fy = src_y - floor(src_y);

		for (dst_x = 0; dst_x < new_wid; dst_x++) {
			tile_x = dst_x / dst_tile_wid;
			src_x = (dst_x * src->w) / (float)new_wid;
			x0 = (int)floor(src_x);
			x1 = x0 + 1;
			fx = src_x - floor(src_x);

			/* Figure out the boundaries for current pixel. */
			bounds = rawpict_tiles == NULL ?
				(rectangle){(coordinates){tile_x * src_tile_wid, tile_y * src_tile_hgt}, (coordinates){((tile_x + 1) * src_tile_wid) - 1, ((tile_y + 1) * src_tile_hgt) - 1}} :
				rawpict_bounds(src->w, src->h, dst_x, dst_y, rawpict_defined_count, rawpict_defined, rawpict_scaled);

			/* Compute scaled pixel color using linear scaling. */
			p[0] = (coordinates){x0, y0}; p[1] = (coordinates){x1, y0}; p[2] = (coordinates){x0, y1}; p[3] = (coordinates){x1, y1};
			w[0] = (1.0 - fx) * (1.0 - fy); w[1] = fx * (1.0 - fy); w[2] = (1.0 - fx) * fy; w[3] = fx * fy;
			r = 0.0; g = 0.0; b = 0.0; a = 0.0;
			for (i = 0; i < 4; i++) {
				if (w[i] == 0.0) continue;

				/* Confine source coordinates to bounds. */
				p[i] = confineCoordinatesToRectangle(p[i].x, p[i].y, bounds);

				/* Get source pixel colors. */
				SDL_GetRGBA(src_pixels[p[i].y * src_pitch + p[i].x], SDL_GetPixelFormatDetails(src->format), NULL, &c.r, &c.g, &c.b, &c.a);

				/* If mask_colors are defined and src pixel is one of mask_colors, skip it (skipping treats the color as transparent black). */
				if (is_mask_color(c, mask_colors)) {
					continue;
				}

				/* Update destination pixel colors according to weights and source color. */
				r += w[i] * c.r; g += w[i] * c.g; b += w[i] * c.b; a += w[i] * c.a;
			}

			/* Write the computed scaled pixel color. */
			dst_pixels[dst_y * dst_pitch + dst_x] = SDL_MapRGBA(
					SDL_GetPixelFormatDetails(dst->format), NULL,
					(uint8_t)round(fmin(fmax(r, 0.0), 255.0)),
					(uint8_t)round(fmin(fmax(g, 0.0), 255.0)),
					(uint8_t)round(fmin(fmax(b, 0.0), 255.0)),
					(uint8_t)round(fmin(fmax(a, 0.0), 255.0))
					);
		}
	}

	SDL_UnlockSurface(dst);
	SDL_UnlockSurface(src);

	if (rawpict_defined != NULL) C_FREE(rawpict_defined, rawpict_defined_count, rawpict_tile);
	if (rawpict_scaled != NULL) C_FREE(rawpict_scaled, rawpict_defined_count, rawpict_tile);

	return dst;
}

/*
 * Scales source surface containing tiles using Lanczos resampling.
 * Assumes source surface width/height are divisible by source tile width/height.
 * The returned surface keeps the source surface format.
 */
static SDL_Surface *ScaleSurfaceLanczos(SDL_Surface *src, uint16_t src_tile_wid, uint16_t src_tile_hgt, uint16_t dst_tile_wid, uint16_t dst_tile_hgt, rawpict_tile *rawpict_tiles, const uint32_t mask_colors[]) {
	if (!src || !src->format) return NULL;

	uint16_t new_wid = (src->w / src_tile_wid) * dst_tile_wid;
	uint16_t new_hgt = (src->h / src_tile_hgt) * dst_tile_hgt;
	SDL_Surface *dst = NULL;
	int src_pitch, dst_pitch;
	int rawpict_defined_count;
	rawpict_tile *rawpict_defined = NULL, *rawpict_scaled = NULL;
	uint32_t *src_pixels = NULL, *dst_pixels = NULL;
	int dst_y, tile_y, y0, y1;
	int dst_x, tile_x, x0, x1;
	float src_y, src_x;
	rectangle bounds;
	float r, g, b, a, w;
	int sy, sx, cy, cx;
	float wy, wx, wxy;
	Pixel c;

	dst = SDL_CreateSurface(new_wid, new_hgt, SDL_PIXELFORMAT_RGBA32);
	if (!dst) {
		fprintf(stderr, "Error creating lanczos scaled surface: %s\n", SDL_GetError());
		return NULL;
	}
	src_pitch = src->pitch / surface_bytes_per_pixel(src);
	dst_pitch = dst->pitch / surface_bytes_per_pixel(dst);

	/* Process raw tiles so there is no need to traverse whole rawpict_tiles array when scaling, just the enabled ones. Also scale for speedup. */
	rawpict_defined_count = prepare_defined_and_scaled_rawpict_tiles(rawpict_tiles, src_tile_wid, src_tile_hgt, dst_tile_wid, dst_tile_hgt, &rawpict_defined, &rawpict_scaled);

	SDL_LockSurface(src);
	SDL_LockSurface(dst);

	src_pixels = (uint32_t *)src->pixels;
	dst_pixels = (uint32_t *)dst->pixels;

	for (dst_y = 0; dst_y < new_hgt; dst_y++) {
		tile_y = dst_y / dst_tile_hgt;
		src_y = (dst_y * src->h) / (float)new_hgt;
		y0 = (int)floor(src_y) - LANCZOS_A + 1;
		y1 = (int)floor(src_y) + LANCZOS_A;

		for (dst_x = 0; dst_x < new_wid; dst_x++) {
			tile_x = dst_x / dst_tile_wid;
			src_x = (dst_x * src->w) / (float)new_wid;
			x0 = (int)floor(src_x) - LANCZOS_A + 1;
			x1 = (int)floor(src_x) + LANCZOS_A;

			/* Figure out the boundaries for current pixel. */
			bounds = rawpict_tiles == NULL ?
				(rectangle){(coordinates){tile_x * src_tile_wid, tile_y * src_tile_hgt}, (coordinates){((tile_x + 1) * src_tile_wid) - 1, ((tile_y + 1) * src_tile_hgt) - 1}} :
				rawpict_bounds(src->w, src->h, dst_x, dst_y, rawpict_defined_count, rawpict_defined, rawpict_scaled);

			r = 0.0; g = 0.0; b = 0.0; a = 0.0; w = 0.0;
			for (sy = y0; sy <= y1; sy++) {
				cy = cofineInteger(sy, bounds.top_left.y, bounds.bottom_right.y);
				wy = lanczosKernel(src_y - (float)sy, LANCZOS_A);

				if (wy == 0.0) continue;

				for (sx = x0; sx <= x1; sx++) {
					cx = cofineInteger(sx, bounds.top_left.x, bounds.bottom_right.x);
					wx = lanczosKernel(src_x - (float)sx, LANCZOS_A);
					wxy = wx * wy;

					if (wxy == 0.0) continue;

					/* Get source pixel colors. */
					SDL_GetRGBA(src_pixels[cy * src_pitch + cx], SDL_GetPixelFormatDetails(src->format), NULL, &c.r, &c.g, &c.b, &c.a);

					/* If mask_colors are defined and src pixel is one of mask_colors, set colors to transparent black. */
					if (is_mask_color(c, mask_colors)) {
						c.r = 0.0; c.g = 0.0;  c.b = 0.0; c.a = 0.0;
					}

					r += wxy * c.r;
					g += wxy * c.g;
					b += wxy * c.b;
					a += wxy * c.a;
					w += wxy;
				}
			}

			if (w != 0.0) {
				r /= w;
				g /= w;
				b /= w;
				a /= w;
			}

			dst_pixels[dst_y * dst_pitch + dst_x] = SDL_MapRGBA(
					SDL_GetPixelFormatDetails(dst->format), NULL,
					(uint8_t)lround(fmin(fmax(r, 0.0), 255.0)),
					(uint8_t)lround(fmin(fmax(g, 0.0), 255.0)),
					(uint8_t)lround(fmin(fmax(b, 0.0), 255.0)),
					(uint8_t)lround(fmin(fmax(a, 0.0), 255.0))
					);
		}
	}

	SDL_UnlockSurface(dst);
	SDL_UnlockSurface(src);

	if (rawpict_defined != NULL) C_FREE(rawpict_defined, rawpict_defined_count, rawpict_tile);
	if (rawpict_scaled != NULL) C_FREE(rawpict_scaled, rawpict_defined_count, rawpict_tile);

	return dst;
}

/* Create array of filtered scaling functions to use without conditions. */
typedef __typeof__(&ScaleSurfaceLinear) sdl3_scale_surface_func;
static const sdl3_scale_surface_func scale_surface_functions[INTERPOLATION_TYPES_COUNT] = {
	[INTERPOLATION_LINEAR] = ScaleSurfaceLinear,
	[INTERPOLATION_LANCZOS] = ScaleSurfaceLanczos
};

/*
 * Load a filtered surface from disk, or run the selected scaler and cache it.
 * Rawpict-boundary and mask-omitting calls are separate cache variants because
 * their output differs even with the same source image and tile dimensions.
 */
static SDL_Surface *ScaleSurfaceFilteredCached(SDL_Surface *src, uint16_t src_tile_wid, uint16_t src_tile_hgt, uint16_t dst_tile_wid, uint16_t dst_tile_hgt, rawpict_tile *rawpict_tiles, const uint32_t mask_colors[], int subset, uint32_t source_hash) {
 #ifdef SDL3_TILESET_CACHE_DEBUG
	Uint64 start_time = SDL_GetPerformanceCounter();
 #endif
	if (scale_surface_functions[gfx_resize_type] == NULL) {
			logprint(format("Unknown filtered scaling type: %d\n", gfx_resize_type));
			return(NULL);
	}
	SDL_Surface *scaled;

	if (disable_tileset_caching || !source_hash) {
		scaled = scale_surface_functions[gfx_resize_type](src, src_tile_wid, src_tile_hgt, dst_tile_wid, dst_tile_hgt, rawpict_tiles, mask_colors);
 #ifdef SDL3_TILESET_CACHE_DEBUG
		logprint(format("ScaleSurfaceFilteredCached: %dx%d->%dx%d,%s,%s,%d: disabled cache execution time: %f ms\n", src_tile_wid, src_tile_hgt, dst_tile_wid, dst_tile_hgt, rawpict_tiles?"rawpict":"", mask_colors?"nomask":"", subset, (double)(SDL_GetPerformanceCounter() - start_time) / (double)SDL_GetPerformanceFrequency() * 1000.0));
 #endif
		return scaled;
	}

	uint32_t context_hash = 0;
	char cache_path[1024];
	const char *variant = format("%s%s%s",
			gfx_resize_type == INTERPOLATION_LINEAR ? "linear" : "lanczos",
			rawpict_tiles ? "-rawpict" : "",
			mask_colors ? "-nomask" : ""
			);

	if (rawpict_tiles) context_hash = SDL_crc32_u32(context_hash, rawpict_hash(rawpict_tiles));
	if (mask_colors) context_hash = SDL_crc32_u32(context_hash, mask_colors_hash(mask_colors));

	scaled = tileset_cache_load(src, src_tile_wid, src_tile_hgt, dst_tile_wid, dst_tile_hgt, source_hash, subset, variant, context_hash, cache_path, sizeof(cache_path));
	if (scaled) {
 #ifdef SDL3_TILESET_CACHE_DEBUG
		logprint(format("ScaleSurfaceFilteredCached: %dx%d->%dx%d,%s,%s,%d: cache hit execution time: %f ms\n", src_tile_wid, src_tile_hgt, dst_tile_wid, dst_tile_hgt, rawpict_tiles?"rawpict":"", mask_colors?"nomask":"", subset, (double)(SDL_GetPerformanceCounter() - start_time) / (double)SDL_GetPerformanceFrequency() * 1000.0));
#endif
		return(scaled);
	}

	scaled = scale_surface_functions[gfx_resize_type](src, src_tile_wid, src_tile_hgt, dst_tile_wid, dst_tile_hgt, rawpict_tiles, mask_colors);
	tileset_cache_save(scaled, cache_path);
 #ifdef SDL3_TILESET_CACHE_DEBUG
	logprint(format("ScaleSurfaceFilteredCached: %dx%d->%dx%d,%s,%s,%d: cache miss execution time: %f ms\n", src_tile_wid, src_tile_hgt, dst_tile_wid, dst_tile_hgt, rawpict_tiles?"rawpict":"", mask_colors?"nomask":"", subset, (double)(SDL_GetPerformanceCounter() - start_time) / (double)SDL_GetPerformanceFrequency() * 1000.0));
 #endif
	return(scaled);
}

/* Return RGBA color for default mask index. */
static uint32_t graphics_default_mask(uint8_t n) {
	if (graphics_image_mpt == 2) {
		switch (n) {
			case 0: return (GFXMASK_BG_R << 24) | (GFXMASK_BG_G << 16) | (GFXMASK_BG_B << 8) | 0xFF;
			case 1: return (GFXMASK_FG_R << 24) | (GFXMASK_FG_G << 16) | (GFXMASK_FG_B << 8) | 0xFF;
		}
	}
	else if (graphics_image_mpt == 3) {
		switch (n) {
			/* Background (BG) and outline (BG2) mask colors are switched here for compatibility reasons. */
			/* Well behaved images have their masks colors defined in .prefs file in proper order (bg, fg, bg2). */
			case 0: return (GFXMASK_BG2_R << 24) | (GFXMASK_BG2_G << 16) | (GFXMASK_BG2_B << 8) | 0xFF;
			case 1: return (GFXMASK_FG_R << 24) | (GFXMASK_FG_G << 16) | (GFXMASK_FG_B << 8) | 0xFF;
			case 2: return (GFXMASK_BG_R << 24) | (GFXMASK_BG_G << 16) | (GFXMASK_BG_B << 8) | 0xFF;
		}
	}
	fprintf(stderr, "Warning: Color for mask no %d, when there are %d masks per tile is undefined!\n", n, graphics_image_mpt);
	return(0);
};

/* Sync runtime mask colors from pref values and defaults. */
static void sdl3_ensure_graphics_mask_colors(void) {
	bool has_masks = false;

	if (!graphics_image_mpt) return;

	graphics_image_prefs_has_masks = false;
	graphics_image_prefs_has_outline = false;
	for (int s = 0; s < MAX_SUBFONTS; s++) {
		graphics_image_sub_prefs_has_masks[s] = false;
		graphics_image_sub_prefs_has_outline[s] = false;
	}

	/* Check whether primary mask colors are defined in prefs. */
	for (uint8_t i = 0; i < graphics_image_mpt; i++) {
		if (graphics_image_masks_colors_prefs[i] != 0) {
			has_masks = true;
			break;
		}
	}

	graphics_image_prefs_has_masks = has_masks;
	if (has_masks && graphics_image_mpt >= 3) {
		graphics_image_prefs_has_outline = (graphics_image_masks_colors_prefs[2] != 0);
	}

	/* Use default colors only when prefs define none. */
	if (!has_masks) {
		for (uint8_t i = 0; i < graphics_image_mpt; i++) {
			graphics_image_masks_colors[i] = graphics_default_mask(i);
		}
	} else {
		for (uint8_t i = 0; i < graphics_image_mpt; i++) {
			graphics_image_masks_colors[i] = graphics_image_masks_colors_prefs[i];
		}
	}

	for (int s = 0; s < MAX_SUBFONTS; s++) {
		bool sub_has_masks = false;

		/* Detect whether subfont overrides are defined in prefs. */
		for (uint8_t i = 0; i < graphics_image_mpt; i++) {
			if (graphics_image_masks_colors_sub_prefs[s][i] != 0) {
				sub_has_masks = true;
				break;
			}
		}

		graphics_image_sub_prefs_has_masks[s] = sub_has_masks;
		if (!sub_has_masks) {
			/* Inherit defaults for subfonts with no overrides. */
			for (uint8_t i = 0; i < graphics_image_mpt; i++) {
				graphics_image_masks_colors_sub[s][i] = graphics_image_masks_colors[i];
			}
		} else {
			for (uint8_t i = 0; i < graphics_image_mpt; i++) {
				graphics_image_masks_colors_sub[s][i] = graphics_image_masks_colors_sub_prefs[s][i];
			}
			if (graphics_image_mpt >= 3) {
				graphics_image_sub_prefs_has_outline[s] = (graphics_image_masks_colors_sub_prefs[s][2] != 0);
			}
		}
	}
}

/* Allocate and initialize empty layer surfaces for a scaled tileset. */
static errr init_tile_layers(SDL_Surface *scaled_image, uint8_t nlayers, SDL_Surface ***layers_out, const uint32_t mask_colors[]) {
	if (!scaled_image || nlayers == 0 || !layers_out || !mask_colors) return(1);

	C_MAKE(*layers_out, nlayers, SDL_Surface*);
	for (int i = 0; i < nlayers; i++) {
		(*layers_out)[i] = SDL_CreateSurface(scaled_image->w, scaled_image->h, SDL_PIXELFORMAT_RGBA32);
		if (!(*layers_out)[i]) {
			for (int j = 0; j < i; j++) SDL_DestroySurface((*layers_out)[j]);
			C_FREE(*layers_out, nlayers, SDL_Surface*);
			*layers_out = NULL;
			return(1);
		}

		/* The transparent color has to be different from mask color (color key is only RGB, alpha is not considered). */
		SDL_FillSurfaceRect((*layers_out)[i], NULL, SDL_MapRGBA(SDL_GetPixelFormatDetails((*layers_out)[i]->format), NULL, ((mask_colors[i+1] >> 24) & 0xFF) ^ 0xFF, ((mask_colors[i+1] >> 16) & 0XFF) ^ 0XFF, ((mask_colors[i+1] >> 8) & 0xFF) ^ 0xFF, 0x00));
		SDL_SetSurfaceColorKey((*layers_out)[i], true, SDL_MapRGB(SDL_GetPixelFormatDetails((*layers_out)[i]->format), NULL, ((mask_colors[i+1] >> 24) & 0xFF), ((mask_colors[i+1] >> 16) & 0xFF), ((mask_colors[i+1] >> 8) & 0xFF)));
		SDL_SetSurfaceBlendMode((*layers_out)[i], SDL_BLENDMODE_NONE);
	}

	return(0);
}

/* Copy background layer and split mask colors into individual layers. */
static void split_mask_colors(SDL_Surface *scaled_image, uint8_t nlayers, SDL_Surface **layers, const uint32_t mask_colors[]) {
	if (!scaled_image || !layers || !mask_colors) return;
	uint32_t *src_pixels, *dst_pixels, src_pos;
	int src_pitch, dst_pitch;
	int x, y, l;

	/* Create first layer by making background color fully transparent black. */
	SDL_SetSurfaceColorKey(scaled_image, true, SDL_MapRGB(SDL_GetPixelFormatDetails(scaled_image->format), NULL, ((mask_colors[0] >> 24) & 0xFF), ((mask_colors[0] >> 16) & 0xFF), ((mask_colors[0] >> 8) & 0xFF)));
	SDL_BlitSurface(scaled_image, NULL, layers[0], NULL);

	/* Separate the mask colours into each layer. */
	if (nlayers > 1) {
		src_pixels = (uint32_t*)layers[0]->pixels;
		src_pitch = layers[0]->pitch / surface_bytes_per_pixel(layers[0]);

		for (y = 0; y < layers[0]->h; y++) {
			for (x = 0; x < layers[0]->w; x++) {
				src_pos = (y * src_pitch) + x;
				for (l = 1; l < nlayers; l++) {
					/* Skip layers with zeroed colors and alpha. */
					if (mask_colors[l + 1] == 0) continue;

					if (src_pixels[src_pos] == SDL_MapRGBA(SDL_GetPixelFormatDetails(layers[0]->format), NULL, ((mask_colors[l + 1] >> 24) & 0xFF), ((mask_colors[l + 1] >> 16) & 0xFF), ((mask_colors[l + 1] >> 8) & 0xFF), 0xFF)) {
						dst_pixels = (uint32_t*)layers[l]->pixels;
					dst_pitch = layers[l]->pitch / surface_bytes_per_pixel(layers[l]);

						dst_pixels[y * dst_pitch + x] = src_pixels[src_pos];
						src_pixels[src_pos] = SDL_MapRGBA(SDL_GetPixelFormatDetails(layers[0]->format), NULL, ((mask_colors[0] >> 24) & 0xFF) ^ 0xFF, ((mask_colors[0] >> 16) & 0xFF) ^ 0xFF, ((mask_colors[0] >> 8) & 0xFF) ^ 0xFF, 0);
					}
				}
			}
		}
	}
}

/* Replace opaque pixels in the base layer with pixels from the filtered tileset. */
static errr copy_filtered_base_layer_pixels(SDL_Surface *layer, SDL_Surface *color_image, const uint32_t mask_colors[]) {
	if (!layer || !color_image) return 1;

	if (layer->format != SDL_PIXELFORMAT_RGBA32 || color_image->format != SDL_PIXELFORMAT_RGBA32) return 2;
	if (layer->h != color_image->h || layer->w != color_image->w) return 3;
	if (SDL_MUSTLOCK(layer)) if (!SDL_LockSurface(layer)) return 4;
	if (SDL_MUSTLOCK(color_image)) if (!SDL_LockSurface(color_image)) {
		if (SDL_MUSTLOCK(layer)) SDL_UnlockSurface(layer);
		return 5;
	}

	uint32_t *layer_row;
	int x, y;

	for (y = 0; y < layer->h; y++) {
		layer_row = (uint32_t *)((uint8_t*)layer->pixels + y * layer->pitch);
		for (x = 0; x < layer->w; x++) {
			/* Don't copy transparent and masked pixels. */
			if ((layer_row[x] & surface_alpha_mask(layer)) == 0) continue;
			if (mask_colors != NULL && sdl3_surface_pixel_is_mask(layer, layer_row, 0, x, 0, mask_colors)) continue;

			layer_row[x] = ((uint32_t *)((uint8_t*)color_image->pixels + y * color_image->pitch))[x];
		}
	}

	if (SDL_MUSTLOCK(layer)) SDL_UnlockSurface(layer);
	if (SDL_MUSTLOCK(color_image)) SDL_UnlockSurface(color_image);
	return 0;
}

 #ifdef GRAPHICS_BG_MASK
/* Generate forced outline into the last layer (nlayers-1) for a scaled tileset. */
static void generate_outline(SDL_Surface *scaled_image, SDL_Surface **layers, uint8_t nlayers, int tile_w, int tile_h, const uint32_t mask_colors[]) {
	if (!scaled_image || !layers || !mask_colors) return;
	if (nlayers <= 1) return;
	if (sdl3_graphics_image_force_outline < 0) return;

	int ol = nlayers - 1;
	SDL_FillSurfaceRect(layers[ol], NULL, SDL_MapRGBA(SDL_GetPixelFormatDetails(layers[ol]->format), NULL, ((mask_colors[ol + 1] >> 24) & 0xFF) ^ 0xFF, ((mask_colors[ol + 1] >> 16) & 0XFF) ^ 0XFF, ((mask_colors[ol + 1] >> 8) & 0xFF) ^ 0xFF, 0x00));

	if (sdl3_graphics_image_force_outline == 0) return;

	int tiles_per_row = tile_w > 0 ? scaled_image->w / tile_w : 0;
	int tiles_per_col = tile_h > 0 ? scaled_image->h / tile_h : 0;
	int outline_radius_x = sdl3_graphics_image_force_outline;
	int outline_radius_y = sdl3_graphics_image_force_outline;
	uint32_t bg_mask_color, ol_mask_color;
	uint32_t *src_pixels, *dst_pixels;
	int src_pitch, dst_pitch;
	int x, y, row, col;
	int start, end, cnt, left, right, top, bottom, bound;
	uint32_t src_pos, pass_pos;
	bool found_bg;
	uint8_t *hpass = NULL;
	uint8_t *vpass = NULL;

	C_MAKE(hpass, scaled_image->w * scaled_image->h, uint8_t);
	C_MAKE(vpass, scaled_image->w * scaled_image->h, uint8_t);

	if (graphics_tile_wid > 0) {
		outline_radius_x = (sdl3_graphics_image_force_outline * tile_w + graphics_tile_wid / 2) / graphics_tile_wid;
	}
	if (graphics_tile_hgt > 0) {
		outline_radius_y = (sdl3_graphics_image_force_outline * tile_h + graphics_tile_hgt / 2) / graphics_tile_hgt;
	}

	if (outline_radius_x < 1) outline_radius_x = 1;
	if (outline_radius_y < 1) outline_radius_y = 1;

	bg_mask_color = SDL_MapRGBA(SDL_GetPixelFormatDetails(scaled_image->format), NULL, ((mask_colors[0] >> 24) & 0xFF), ((mask_colors[0] >> 16) & 0xFF), ((mask_colors[0] >> 8) & 0xFF), 0xFF);
	ol_mask_color = SDL_MapRGBA(SDL_GetPixelFormatDetails(scaled_image->format), NULL, ((mask_colors[ol + 1] >> 24) & 0xFF), ((mask_colors[ol + 1] >> 16) & 0xFF), ((mask_colors[ol + 1] >> 8) & 0xFF), 0xFF);
	src_pitch = scaled_image->pitch / surface_bytes_per_pixel(scaled_image);
	dst_pitch = layers[ol]->pitch / surface_bytes_per_pixel(layers[ol]);

	/* Horizontal dilation with radius r using sliding-window, limited to individual tiles to not bleed outline to adjacent tiles. */
	src_pixels = (uint32_t*)scaled_image->pixels;
	for (y = 0; y < scaled_image->h; y++) {
		for (col = 0; col < tiles_per_row; col++) {
			start = col * tile_w;
			end = start + tile_w;
			cnt = 0;
			left = start;
			right = start - 1;
			for (x = start; x < end; x++) {
				src_pos = (y * src_pitch);
				pass_pos = (y * scaled_image->w);
				/* Expand right to min(end-1, x+r). */
				bound = end - 1 < x + outline_radius_x ? end - 1 : x + outline_radius_x;
				while (right < bound) {
					right++;
					found_bg = false;
					if (src_pixels[src_pos + right] == bg_mask_color) found_bg = true;
					if (mask_colors[ol + 1] != 0 && src_pixels[src_pos + right] == ol_mask_color) found_bg = true;
					if (!found_bg) {
						cnt++;
					}
				}
				/* Shrink left to max(start, x-r). */
				bound = start > x - outline_radius_x ? start : x - outline_radius_x;
				while (left < bound) {
					found_bg = false;
					if (src_pixels[src_pos + left] == bg_mask_color) found_bg = true;
					if (mask_colors[ol + 1] != 0 && src_pixels[src_pos + left] == ol_mask_color) found_bg = true;
					if (!found_bg) {
						cnt--;
					}
					left++;
				}

				hpass[pass_pos + x] = (cnt > 0) ? 1 : 0;
			}
		}
	}

	/* Vertical dilation with radius r using sliding-window, limited to individual tile to not bleed outline to adjacent tiles. */
	for (x = 0; x < scaled_image->w; x++) {
		for (row = 0; row < tiles_per_col; row++) {
			start = row * tile_h;
			end = start + tile_h;
			cnt = 0;
			top = start;
			bottom = start - 1;
			for (y = start; y < end; y++) {
				/* Expand bottom to min(end-1, y+r). */
				int bound = end - 1 < y + outline_radius_y ? end - 1 : y + outline_radius_y;
				while (bottom < bound) {
					bottom++;
					cnt += hpass[bottom * scaled_image->w + x];
				}
				/* Shrink top to max(start, y-r). */
				bound = start > y - outline_radius_y ? start : y - outline_radius_y;
				while (top < bound) {
					cnt -= hpass[top * scaled_image->w + x];
					top++;
				}
				vpass[y * scaled_image->w + x] = (cnt > 0) ? 1 : 0;
			}
		}
	}

	/* Outline = dilated (vpass) minus original. */
	for (y = 0; y < scaled_image->h; y++) {
		for (x = 0; x < scaled_image->w; x++) {
			pass_pos = y * scaled_image->w + x;
			src_pos = y * src_pitch + x;
			found_bg = false;
			if (src_pixels[src_pos] == bg_mask_color) found_bg = true;
			if (mask_colors[ol + 1] != 0 && src_pixels[src_pos] == ol_mask_color) found_bg = true;
			if(vpass[pass_pos] == 1 && found_bg) {
				dst_pixels = (uint32_t*)layers[ol]->pixels;
				dst_pixels[y * dst_pitch + x] = SDL_MapRGBA(SDL_GetPixelFormatDetails(layers[ol]->format), NULL, ((mask_colors[ol + 1] >> 24) & 0xFF), ((mask_colors[ol + 1] >> 16) & 0xFF), ((mask_colors[ol + 1] >> 8) & 0xFF), 0xFF);
			}
		}
	}

	C_FREE(hpass, scaled_image->w * scaled_image->h, uint8_t);
	C_FREE(vpass, scaled_image->w * scaled_image->h, uint8_t);
}
 #endif

/* Prepare scaled rawpict surfaces and coordinate maps. Returns 0 on success. */
static errr init_scaled_rawpict(SDL_Surface *scaled_image, SDL_Surface *orig_image, SDL_Surface **out_surface, int *wid_org, int *hgt_org, int *wid_use, int *hgt_use, rawpict_tile *rawpict_org, rawpict_tile *rawpict_dst) {
	if (!scaled_image || !out_surface || !wid_org || !hgt_org || !wid_use || !hgt_use || !rawpict_org || !rawpict_dst) return(1);

	/* Keep a copy of the scaled sheet for rawpict drawing. */
	*out_surface = SDL_CreateSurface(scaled_image->w, scaled_image->h, SDL_PIXELFORMAT_RGBA32);
	if (!*out_surface) {
		fprintf(stderr, "Error creating RGB surface: %s\n", SDL_GetError());
		return(2);
	}

	if (SDL_SetSurfaceBlendMode(*out_surface, SDL_BLENDMODE_BLEND) == false) {
		fprintf(stderr, "Error setting blend mode: %s\n", SDL_GetError());
		SDL_DestroySurface(*out_surface);
		*out_surface = NULL;
		return(3);
	}
	if (SDL_BlitSurface(scaled_image, NULL, *out_surface, NULL) == false) {
		fprintf(stderr, "Error copying surfaces: %s\n", SDL_GetError());
		SDL_DestroySurface(*out_surface);
		*out_surface = NULL;
		return(4);
	}

	int width1 = (orig_image) ? orig_image->w : 0;
	int height1 = (orig_image) ? orig_image->h : 0;
	int width2 = scaled_image->w;
	int height2 = scaled_image->h;

	*wid_org = width1;
	*hgt_org = height1;
	*wid_use = width2;
	*hgt_use = height2;

	for (int i = 0; i <= MAX_TILES_RAWPICT; i++) {
		if (!rawpict_org[i].defined || !width1 || !height1) {
			rawpict_dst[i].defined = false;
			continue;
		}

		rawpict_dst[i].defined = true;
		rawpict_dst[i].x = (rawpict_org[i].x * width2) / width1;
		rawpict_dst[i].y = (rawpict_org[i].y * height2) / height1;
		rawpict_dst[i].w = (rawpict_org[i].w * width2) / width1;
		rawpict_dst[i].h = (rawpict_org[i].h * height2) / height1;
	}

	return(0);
}

/* When there is only one layer, that means outline is not used.
 * But if the image contains an outline mask, make it transparent.
 */
static void clear_outline_mask_single_layer(uint8_t nlayers, SDL_Surface **layers, bool has_outline, const uint32_t mask_colors[]) {
	if (nlayers != 1) return;
	if (!has_outline) return;
	if (!layers || !layers[0] || !mask_colors) return;

	uint32_t* src_pixels = (uint32_t*)layers[0]->pixels;
	int src_pitch = layers[0]->pitch / surface_bytes_per_pixel(layers[0]);
	int x, y;
	uint32_t pos;

	for (y = 0; y < layers[0]->h; y++) {
		for (x = 0; x < layers[0]->w; x++) {
			pos = (y * src_pitch) + x;
			if (src_pixels[pos] == SDL_MapRGBA(SDL_GetPixelFormatDetails(layers[0]->format), NULL, ((mask_colors[2] >> 24) & 0xFF), ((mask_colors[2] >> 16) & 0xFF), ((mask_colors[2] >> 8) & 0xFF), 0xFF)) {
				src_pixels[pos] = SDL_MapRGBA(SDL_GetPixelFormatDetails(layers[0]->format), NULL, ((mask_colors[0] >> 24) & 0xFF) ^ 0xFF, ((mask_colors[0] >> 16) & 0xFF) ^ 0xFF, ((mask_colors[0] >> 8) & 0xFF) ^ 0xFF, 0);
			}
		}
	}
}

enum term_data_init_graphics_tileset_err {
	TDGTS_ERR_NONE = 0,
	TDGTS_ERR_NOIMAGE,
	TDGTS_ERR_SCALE,
	TDGTS_ERR_LAYERS,
	TDGTS_ERR_TILEPREP,
	TDGTS_ERR_RAWPICT,
	TDGTS_ERR_RESIZETYPE,
};

/* Initialize the scaled layers and rawpict data for a single tileset sheet. */
static errr term_data_init_graphics_tileset(term_data *td, SDL_Surface *src_image, SDL_Surface ***layers_out, SDL_Surface **tiles_surface_out, int *wid_org, int *hgt_org, int *wid_use, int *hgt_use, rawpict_tile *rawpict_org, rawpict_tile *rawpict_dst, const uint32_t mask_colors[], bool has_outline, int subset, uint32_t source_hash) {
	if (!td || !td->fnt || !src_image || !layers_out || !tiles_surface_out || !wid_org || !hgt_org || !wid_use || !hgt_use || !rawpict_org || !rawpict_dst || !mask_colors) return(TDGTS_ERR_NOIMAGE);

	/* Resize the loaded graphics image, so tile size match font size. */
	/* Note: Always scale the mask source to INTERPOLATION_NEAR.
	 * Layer separation and forced outline generation rely on exact single-color masks,
	 * so filtered scaling cannot be used there until SDL3 graphics gain a proper
	 * semi-transparent / coverage-mask pipeline.
	 */
	bool resize_needed = graphics_tile_wid != td->fnt->wid || graphics_tile_hgt != td->fnt->hgt;
	SDL_Surface *scaled_image = resize_needed ? ScaleSurfaceNearestCached(src_image, graphics_tile_wid, graphics_tile_hgt, td->fnt->wid, td->fnt->hgt, subset, source_hash) : SDL_DuplicateSurface(src_image);
	SDL_Surface *scaled_rawpict = scaled_image;

	if (!scaled_image) return(TDGTS_ERR_SCALE);

	if (resize_needed && gfx_resize_type != INTERPOLATION_NEAR) {
		/* Filtered scaling currently applies only to non-mask pixel content copied back
		 * into layer[0] / rawpict. Mask layers and generated outline intentionally keep
		 * nearest-neighbour edges for exact single-color masking.
		 */
		if (gfx_resize_type != INTERPOLATION_LINEAR && gfx_resize_type != INTERPOLATION_LANCZOS) {
			SDL_DestroySurface(scaled_image);
			return(TDGTS_ERR_RESIZETYPE);
		}
		scaled_rawpict = ScaleSurfaceFilteredCached(src_image, graphics_tile_wid, graphics_tile_hgt, td->fnt->wid, td->fnt->hgt, rawpict_org, NULL, subset, source_hash);
		if (!scaled_rawpict) {
			SDL_DestroySurface(scaled_image);
			return(TDGTS_ERR_SCALE);
		}
	}

	/* Keep a copy of the scaled sheet for rawpict drawing. */
	if (init_scaled_rawpict(scaled_rawpict, src_image, tiles_surface_out, wid_org, hgt_org, wid_use, hgt_use, rawpict_org, rawpict_dst)) {
		if (scaled_rawpict != scaled_image) SDL_DestroySurface(scaled_rawpict);
		SDL_DestroySurface(scaled_image);
		return(TDGTS_ERR_RAWPICT);
	}

	/* Initialize surfaces for all layers. */
	if (init_tile_layers(scaled_image, td->nlayers, layers_out, mask_colors)) {
		if (scaled_rawpict != scaled_image) SDL_DestroySurface(scaled_rawpict);
		SDL_DestroySurface(scaled_image);
		return(TDGTS_ERR_LAYERS);
	}
	/* Split tiles by masks from the nearest-neighbour sheet. */
	split_mask_colors(scaled_image, td->nlayers, *layers_out, mask_colors);

 #ifdef GRAPHICS_BG_MASK
	/* If needed, (re)generate outline for tiles. */
	generate_outline(scaled_image, *layers_out, td->nlayers, td->fnt->wid, td->fnt->hgt, mask_colors);
 #endif
	/* Make outline mask transparent when only one layer is available. */
	clear_outline_mask_single_layer(td->nlayers, *layers_out, has_outline, mask_colors);

	/* If resize type is other than INTERPOLATION_NEAR, update layer[0] pixels with the right filter. */
	if (scaled_rawpict != scaled_image) {
		SDL_Surface *scaled_nomask = ScaleSurfaceFilteredCached(src_image, graphics_tile_wid, graphics_tile_hgt, td->fnt->wid, td->fnt->hgt, NULL, mask_colors, subset, source_hash);
		if (!scaled_nomask) {
			for (int i = 0; i < td->nlayers; i++) {
				if ((*layers_out)[i]) SDL_DestroySurface((*layers_out)[i]);
			}
			C_FREE(*layers_out, td->nlayers, SDL_Surface*);
			*layers_out = NULL;
			SDL_DestroySurface(*tiles_surface_out);
			*tiles_surface_out = NULL;
			*wid_org = *hgt_org = *wid_use = *hgt_use = 0;
			C_WIPE(rawpict_dst, MAX_TILES_RAWPICT + 1, rawpict_tile);
			SDL_DestroySurface(scaled_rawpict);
			SDL_DestroySurface(scaled_image);
			return(TDGTS_ERR_SCALE);
		}
		copy_filtered_base_layer_pixels((*layers_out)[0], scaled_nomask, mask_colors);
		SDL_DestroySurface(scaled_nomask);
	}

	/* The scaled images are not needed anymore. */
	if (scaled_rawpict != scaled_image) SDL_DestroySurface(scaled_rawpict);
	SDL_DestroySurface(scaled_image);

	return(TDGTS_ERR_NONE);
}

/* Initializes graphics stuff and prepare surfaces for a terminal's term_data from graphics_image.
 * Frees all graphics resources allocated before and then makes the initialization.
 *
 * Resize tiles and separate into layers by mask colors.
 * First layer is the image with all other mask colors, except foreground color, made fully transparent black.
 * Other layers are just the other mask color pixels on transparent background.
 * The background mask color does not need to be extracted to a layer.
 */
static errr term_data_init_graphic_tiles(term_data *td) {
	/* Free old tiles. */
	free_graphics(td);

	if (!graphics_image) {
		fprintf(stderr, "No graphics image loaded for SDL3 term.\n");
		return(TDGTS_ERR_NOIMAGE);
	}

	/* Initialize surfaces for all layers. */
	td->nlayers = graphics_image_mpt - 1;

	errr err = term_data_init_graphics_tileset(td, graphics_image, &td->tiles_layers, &td->tiles_surface, &td->rawpict_scale_wid_org, &td->rawpict_scale_hgt_org, &td->rawpict_scale_wid_use, &td->rawpict_scale_hgt_use, tiles_rawpict_org, td->tiles_rawpict, graphics_image_masks_colors, graphics_image_prefs_has_outline, -1, graphics_image_hash);
	if (err != TDGTS_ERR_NONE) {
		if (err == TDGTS_ERR_NOIMAGE) fprintf(stderr, "Failed input checks for SDL3 graphics initialization.\n");
		if (err == TDGTS_ERR_SCALE) fprintf(stderr, "Failed to scale graphics surface for SDL3 term.\n");
		if (err == TDGTS_ERR_LAYERS) fprintf(stderr, "Failed to initialize graphics layers for SDL3 term.\n");
		if (err == TDGTS_ERR_RAWPICT) fprintf(stderr, "Failed to initialize SDL3 term rawpict surface.\n");
		if (err == TDGTS_ERR_RESIZETYPE) fprintf(stderr, "Failed to scale due unknown resize type.\n");
		free_graphics(td);
		return(err);
	}

	/* Prepare (partial) sub-tilesets */
	for (int s = 0; s < MAX_SUBFONTS; s++) {
		if (!graphic_subtiles[s]) continue;
		if (!graphics_image_sub[s]) {
			fprintf(stderr, "Warning: Graphics sub-tileset %d has no loaded image; disabling.\n", s);
			graphic_subtiles[s] = false;
			continue;
		}

		err = term_data_init_graphics_tileset(td, graphics_image_sub[s], &td->tiles_layers_sub[s], &td->tiles_surface_sub[s], &td->rawpict_scale_wid_org_sub[s], &td->rawpict_scale_hgt_org_sub[s], &td->rawpict_scale_wid_use_sub[s], &td->rawpict_scale_hgt_use_sub[s], tiles_rawpict_org_sub[s], td->tiles_rawpict_sub[s], graphics_image_masks_colors_sub[s], graphics_image_sub_prefs_has_outline[s], s, graphics_image_sub_hash[s]);
		if (err != TDGTS_ERR_NONE) {
			if (err == TDGTS_ERR_SCALE) fprintf(stderr, "Warning: Failed to scale graphics sub-tileset %d; disabling.\n", s);
			if (err == TDGTS_ERR_LAYERS) fprintf(stderr, "Warning: Failed to initialize graphics layers for SDL3 sub-tileset %d; disabling.\n", s);
			if (err == TDGTS_ERR_RAWPICT) fprintf(stderr, "Warning: Failed to initialize rawpict surface for SDL3 sub-tileset %d; disabling.\n", s);
			if (err == TDGTS_ERR_RESIZETYPE) fprintf(stderr, "Warning: Failed to scale due unknown resize type for SDL3 sub-tileset %d; disabling.\n", s);
			graphic_subtiles[s] = false;
			continue;
		}
	}

	/* Initialize preparation surface. */
	td->tilePreparation = SDL_CreateSurface(td->fnt->wid, td->fnt->hgt, SDL_PIXELFORMAT_RGBA32);
	if (!td->tilePreparation) {
		fprintf(stderr, "Failed to initialize SDL3 term tile preparation surface.\n");
		free_graphics(td);
		return(TDGTS_ERR_TILEPREP);
	}
	SDL_SetSurfaceBlendMode(td->tilePreparation, SDL_BLENDMODE_BLEND);

	/* Note: If we want to cache even more graphics for faster drawing, we could initialize 16 copies of the graphics image with all possible mask colors already applied. */
	/* Memory cost could become "large" quickly though (eg 5MB bitmap -> 80MB). Not a real issue probably. */
 #ifdef TILE_CACHE_SIZE
	for (int i = 0; i < TILE_CACHE_SIZE; i++) {
		td->tile_cache[i].tile = SDL_CreateSurface(td->fnt->wid, td->fnt->hgt, SDL_PIXELFORMAT_RGBA32);
		SDL_SetSurfaceBlendMode(td->tile_cache[i].tile, SDL_BLENDMODE_BLEND);
		td->tile_cache[i].ncolors = graphics_image_mpt - 1;
		C_MAKE(td->tile_cache[i].colors, td->tile_cache[i].ncolors, uint32_t);
		td->tile_cache[i].subset = -1;
		td->tile_cache[i].index = 0;
	}
 #endif
	return(TDGTS_ERR_NONE);
}

/* Initialize graphics and install graphics hooks for a term. */
static errr term_data_enable_graphics(int index, term_data *td) {
	term *t = &td->t;

	sdl3_ensure_graphics_mask_colors();

	if (term_data_init_graphic_tiles(td) || td->tiles_layers == NULL || td->tilePreparation == NULL) {
		/* Disable graphics & hooks. */
 #ifdef GRAPHICS_BG_MASK
		t->pict_hook_2mask = NULL;
 #endif
		t->pict_hook = NULL;
		t->rawpict_hook = NULL;
		t->higher_pict = false;
		return(1);
	}

	/* Graphics hook */
 #ifdef GRAPHICS_BG_MASK
	if (use_graphics == UG_2MASK) {
		t->pict_hook_2mask = Term_pict_sdl3_2mask;
	}
 #endif
	t->pict_hook = Term_pict_sdl3;
	t->rawpict_hook = Term_rawpict_sdl3;

	/* Use graphics sometimes */
	t->higher_pict = true;

	return(0);
}

#endif /* USE_GRAPHICS */

/* Get display scaling for position. */
static float display_scale_for_position(int x, int y) {
	SDL_DisplayID display_id;
	float scale;

	display_id = SDL_GetDisplayForPoint(&((SDL_Point){x, y}));
	if (!display_id) {
		fprintf(stderr, "Warning: Can't get scaling for coordinates {%d, %d}, using primary display scale\n", x, y);
		display_id = SDL_GetPrimaryDisplay();
	}

	scale = SDL_GetDisplayContentScale(display_id);

	if (scale <= 0.0f) return(1.0f);
	return(scale);
}

/*
 * Initialize a term_data
 */
static errr term_data_init(int index, term_data *td, bool fixed, cptr name, cptr font) {
	/* Use values from .tomenetrc;
	   Environment variables (see further below) may override those. */
	int win_cols = term_prefs[index].columns;
	int win_lines = term_prefs[index].lines;
	int topx = term_prefs[index].x;
	int topy = term_prefs[index].y;

	/* Determine display scaling. */
	td->display_scale = ignore_scaling ? 1.0f : display_scale_for_position(topx, topy);

	/* Prepare the standard font */
	MAKE(td->fnt, infofnt);
	infofnt *old_infofnt = Infofnt;
	Infofnt_set(td->fnt);
	if (Infofnt_init(font, td->display_scale) == -1) {
		/* Initialization failed, log and try to use the default font. */
		fprintf(stderr, "Failed to load the \"%s\" font for terminal %d\n", font, index);
		if (in_game) {
			/* If in game, inform the user. */
			Infofnt_set(old_infofnt);
			plog_fmt("Failed to load the \"%s\" font! Falling back to default font.\n", font);
			Infofnt_set(td->fnt);
		}
		if (Infofnt_init(sdl3_terms_font_default[index], td->display_scale) == -1) {
			/* Initialization of the default font failed too. Log, free allocated memory and return with error. */
			fprintf(stderr, "Failed to load the default \"%s\" font for terminal %d\n", sdl3_terms_font_default[index], index);
			Infofnt_set(old_infofnt);
			if (in_game) {
				/* If in game, inform the user. */
				plog_fmt("Failed to load the default \"%s\" font too! Try to change font manually.\n", sdl3_terms_font_default[index]);
			}
			FREE(td->fnt, infofnt);
			return(1);
		}
	}

#ifdef USE_GRAPHICS
	td->tiles_layers = NULL;
	td->tiles_surface = NULL;
	for (int s = 0; s < MAX_SUBFONTS; s++) {
		td->tiles_layers_sub[s] = NULL;
		td->tiles_surface_sub[s] = NULL;
		td->rawpict_scale_wid_org_sub[s] = td->rawpict_scale_hgt_org_sub[s] = 0;
		td->rawpict_scale_wid_use_sub[s] = td->rawpict_scale_hgt_use_sub[s] = 0;
		C_WIPE(td->tiles_rawpict_sub[s], MAX_TILES_RAWPICT + 1, rawpict_tile);
	}
#endif

	/* Extract widths and heights from env variables. */
	cptr n;
	for (int i = 0; i < ANGBAND_TERM_MAX; ++i) {
		if (!strcmp(name, ang_term_name[i])) {
			n = getenv(sdl3_terms_wid_env[i]);
			if (n) win_cols = atoi(n);
			n = getenv(sdl3_terms_hgt_env[i]);
			if (n) win_lines = atoi(n);
			break;
		}
	}

	/* Hack -- extract key buffer size */
	int num = (fixed ? 1024 : 16);

	/* Reset timers just to be sure. */
	td->resize_timer = 0;

	/* Calculate window and drawing field sizes. */
	int wid_draw = win_cols * td->fnt->wid;
	int hgt_draw = win_lines * td->fnt->hgt;
	int wid_border = wid_draw + (2 * SDL3_DEFAULT_BORDER_WIDTH);
	int hgt_border = hgt_draw + (2 * SDL3_DEFAULT_BORDER_WIDTH);

	infowin *old_infowin = Infowin;
	MAKE(td->win, infowin);
	Infowin_set(td->win);

	if (Infowin_init(topx, topy, wid_border, hgt_border, SDL3_DEFAULT_BORDER_WIDTH, color_default_b, color_default_bg)) {
		fprintf(stderr, "Error: Failed to initialize SDL3 terminal #%d window.\n", index);
		Infowin_set(old_infowin);
		FREE(td->win, infowin);
		Infofnt_set(old_infofnt);
		Infofnt_destroy(td->fnt);
		td->fnt = NULL;
		return(1);
	}

	/* HACK: Readjust window position so the top-left corner of window frame is not off screen if necessary. */
	if (sdl3_window_decorations) {
		Infowin_readjust_frame_position();
	}

	if (!strcmp(name, ang_term_name[0])) {
		char version[MAX_CHARS];
		sprintf(version, "TomeNET %d.%d.%d%s",
		    VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, CLIENT_VERSION_TAG);
		Infowin_set_name(version);
	} else Infowin_set_name(name);

#ifdef USE_GRAPHICS
	/* No graphics yet */
	td->nlayers = 0;
	td->tiles_layers = NULL;
	td->tilePreparation = NULL;
	td->tiles_surface = NULL;
	td->rawpict_scale_wid_org = td->rawpict_scale_hgt_org = 0;
	td->rawpict_scale_wid_use = td->rawpict_scale_hgt_use = 0;
	C_WIPE(td->tiles_rawpict, MAX_TILES_RAWPICT + 1, rawpict_tile);
 #ifdef TILE_CACHE_SIZE
	for (int i = 0; i < TILE_CACHE_SIZE; i++) {
		td->tile_cache[i].tile = NULL;
		td->tile_cache[i].index = 0;
		td->tile_cache[i].subset = -1;
		td->tile_cache[i].ncolors = 0;
		td->tile_cache[i].colors = NULL;
	}
 #endif
#endif /* USE_GRAPHICS */

	term *t = &td->t;

	/* Initialize the term (full size) */
	term_init(t, win_cols, win_lines, num);

	/* Set index for terminal. */
	t->idx = index;

	/* Use a "soft" cursor */
	t->soft_cursor = true;

	/* Erase with "white space" */
	t->attr_blank = TERM_WHITE;
	t->char_blank = ' ';

	/* Hooks */
	t->xtra_hook = Term_xtra_sdl3;
	t->curs_hook = Term_curs_sdl3;
	t->wipe_hook = Term_wipe_sdl3;
	t->text_hook = Term_text_sdl3;
	t->nuke_hook = Term_nuke_sdl3;

	/* Save the data */
	t->data = td;

	/* Activate (important) */
	Term_activate(t);

	/* Success */
	return(0);
}

/*
 * Names of the 16 colors
 *   Black, White, Slate, Orange,    Red, Green, Blue, Umber
 *   D-Gray, L-Gray, Violet, Yellow, L-Red, L-Green, L-Blue, L-Umber
 *
 * Colors courtesy of: Torbj|rn Lindgren <tl@ae.chalmers.se>
 *
 * These colors are overwritten with the generic, OS-independent client_color_map[] in enable_common_colormap_sdl3()!
 */
static char color_name[CLIENT_PALETTE_SIZE][8] = {
	"#000000",      /*  0 - BLACK */
	"#ffffff",      /*  1 - WHITE */
	"#9d9d9d",      /*  2 - GRAY */
	"#ff8d00",      /*  3 - ORANGE */
	"#b70000",      /*  4 - RED */
	"#009d44",      /*  5 - GREEN */
#ifndef READABILITY_BLUE
	"#0000ff",      /*  6 - BLUE */
#else
	"#0033ff",      /*  6 - BLUE */
#endif
	"#8d6600",      /*  7 - BROWN (UMBER) */
#ifndef DISTINCT_DARK
	"#747474",      /*  8 - DARKGRAY */
#else
	//"#585858",      /*  8 - DARKGRAY */
	"#666666",      /*  8 - DARKGRAY */
#endif
	"#cdcdcd",      /*  9 - LIGHTGRAY */
	"#af00ff",      /* 10 - PURPLE */
	"#ffff00",      /* 11 - YELLOW */
	"#ff3030",      /* 12 - PINK */
	"#00ff00",      /* 13 - LIGHTGREEN */
	"#00ffff",      /* 14 - LIGHTBLUE */
	"#c79d55",      /* 15 - LIGHTBROWN */
#ifdef EXTENDED_COLOURS_PALANIM
	/* And clone the above 16 standard colors again here: */
	"#000000",      /* 16 - BLACK */
	"#ffffff",      /* 17 - WHITE */
	"#9d9d9d",      /* 18 - GRAY */
	"#ff8d00",      /* 19 - ORANGE */
	"#b70000",      /* 20 - RED */
	"#009d44",      /* 21 - GREEN */
 #ifndef READABILITY_BLUE
	"#0000ff",      /* 22 - BLUE */
 #else
	"#0033ff",      /* 22 - BLUE */
 #endif
	"#8d6600",      /* 23 - BROWN */
 #ifndef DISTINCT_DARK
	"#747474",      /* 24 - DARKGRAY */
 #else
	//"#585858",      /* 24 - DARKGRAY */
	"#666666",      /* 24 - DARKGRAY */
 #endif
	"#cdcdcd",      /* 25 - LIGHTGRAY */
	"#af00ff",      /* 26 - PURPLE */
	"#ffff00",      /* 27 - YELLOW */
	"#ff3030",      /* 28 - PINK */
	"#00ff00",      /* 29 - LIGHTGREEN */
	"#00ffff",      /* 30 - LIGHTBLUE */
	"#c79d55",      /* 31 - LIGHTBROWN */
#endif
};
#ifdef EXTENDED_BG_COLOURS
 /* Format: (fg, bg) */
 static char color_ext_name[TERMX_AMT][2][8] = {
	//{"#0000ff", "#444444", },
	//{"#ffffff", "#0000ff", },
	//{"#666666", "#0000ff", },
	{"#aaaaaa", "#112288", },	/* TERMX_BLUE */
	{"#aaaaaa", "#007700", },	/* TERMX_GREEN */
	{"#aaaaaa", "#770000", },	/* TERMX_RED */
	{"#aaaaaa", "#AAAA00", },	/* TERMX_YELLOW */
	{"#aaaaaa", "#555555", },	/* TERMX_GREY */
	{"#aaaaaa", "#BBBBBB", },	/* TERMX_WHITE */
	{"#aaaaaa", "#333388", },	/* TERMX_PURPLE */
};
#endif

static void enable_common_colormap_sdl3() {
	int i;
	unsigned long c;
#ifdef EXTENDED_BG_COLOURS
	unsigned long b;
#endif

	for (i = 0; i < CLIENT_PALETTE_SIZE; i++) {
		c = client_color_map[i];

		sprintf(color_name[i], "#%06lx", c & 0xFFFFFFL);
	}

#ifdef EXTENDED_BG_COLOURS
	for (i = 0; i < TERMX_AMT; i++) {
		c = client_ext_color_map[i][0];
		b = client_ext_color_map[i][1];

		sprintf(color_ext_name[i][0], "#%06lx", c & 0xFFFFFFL);
		sprintf(color_ext_name[i][1], "#%06lx", b & 0xFFFFFFL);
	}
#endif
}

void enable_readability_blue_sdl3(void) {
	/* New colour code */
	client_color_map[6] = 0x0033FF;
#ifdef EXTENDED_COLOURS_PALANIM
	client_color_map[BASE_PALETTE_SIZE + 6] = 0x0033FF;
#endif
}

static term_data* term_idx_to_term_data(int term_idx) {
	term_data *td = &term_main;

	switch (term_idx) {
	case 0: td = &term_main; break;
	case 1: td = &term_1; break;
	case 2: td = &term_2; break;
	case 3: td = &term_3; break;
	case 4: td = &term_4; break;
	case 5: td = &term_5; break;
	case 6: td = &term_6; break;
	case 7: td = &term_7; break;
	case 8: td = &term_8; break;
	case 9: td = &term_9; break;
	}

	return(td);
}

static int term_data_to_term_idx(term_data *td) {
	if (td == &term_main) return(0);
	if (td == &term_1) return(1);
	if (td == &term_2) return(2);
	if (td == &term_3) return(3);
	if (td == &term_4) return(4);
	if (td == &term_5) return(5);
	if (td == &term_6) return(6);
	if (td == &term_7) return(7);
	if (td == &term_8) return(8);
	if (td == &term_9) return(9);
	return(-1);
}

/* Normalize a font string in-place:
 *   - TTF: add/default/clamp size and rewrite as "<file> <size>".
 *   - non-TTF: trim trailing whitespace.
 * `font` points to a writable buffer of `font_len` bytes.
 * `term_idx` selects the fallback size from sdl3_terms_ttf_size_default[].
 * */
static void sanitize_font_format(char *font, size_t font_len, int term_idx) {
	if (font == NULL || font_len == 0) return;

	char   font_base[256];
	int8_t size = 0;
	size_t len;

	if (is_ttf_font(font, font_base, sizeof(font_base), &size)) {
		/* Fix missing or out-of-range size. */
		if (size < 0) size = sdl3_terms_ttf_size_default[term_idx];

		if (size < SDL3_MIN_TTF_FONT_SIZE) size = SDL3_MIN_TTF_FONT_SIZE;
		else if (size > SDL3_MAX_TTF_FONT_SIZE) size = SDL3_MAX_TTF_FONT_SIZE;

		/* Rebuild the canonical "<file> <size>" string. */
		snprintf(font, font_len, "%s %d", font_base, size);
	} else {
		/* Trim trailing spaces for non-TTF descriptions. */
		len = strlen(font);
		while (len && isspace((unsigned char)font[len - 1])) font[--len] = '\0';
	}
}

/*
 * Initialization of i-th SDL3 terminal window.
 */
static errr sdl3_term_init(int term_id) {
	cptr fnt_name;
	char sanitized_fnt_name[256];
	errr err;

	if (term_id < 0 || term_id >= ANGBAND_TERM_MAX) {
		fprintf(stderr, "Terminal index %d out of bounds\n", term_id);
		return(1);
	}

	if (ang_term[term_id]) {
		fprintf(stderr, "Terminal window with index %d is already initialized\n", term_id);
		/* Success. */
		return(0);
	}

	/* Check environment for SDL3 terminal font. */
	fnt_name = getenv(sdl3_terms_font_env[term_id]);
	/* Check environment for "base" font. */
	if (!fnt_name) fnt_name = getenv("TOMENET_SDL3_FONT");
	/* Use loaded (from config file) or predefined default font. */
	if (!fnt_name && strlen(term_prefs[term_id].font)) fnt_name = term_prefs[term_id].font;
	/* Paranoia, use the default. */
	if (!fnt_name) fnt_name = sdl3_terms_font_default[term_id];

	strncpy(sanitized_fnt_name, fnt_name, sizeof(sanitized_fnt_name));
	sanitized_fnt_name[sizeof(sanitized_fnt_name)-1] = '\0';
	sanitize_font_format(sanitized_fnt_name, sizeof(sanitized_fnt_name), term_id);

	/* Initialize the terminal window, allow resizing, for font changes. */
	err = term_data_init(term_id, sdl3_terms_term_data[term_id], false, ang_term_name[term_id], sanitized_fnt_name);
	/* Store created terminal with SDL3 term data to ang_term array, even if term_data_init failed, but only if there is one. */
	if (Term && term_data_to_term_idx(Term->data) == term_id) ang_term[term_id] = Term;

	if (err) {
		fprintf(stderr, "Error initializing term_data for SDL3 terminal with index %d\n", term_id);
		if (ang_term[term_id]) {
			term_nuke(ang_term[term_id]);
			ang_term[term_id] = NULL;
		}
		return(err);
	}

	/* Success. */
	return(0);
}

#ifdef USE_GRAPHICS
errr init_graphics_sdl3(void) {
	char filename[1024];
	char filename_user[1024];
	char filename_game[1024];
	char path[1024];

	/* Load graphics file. Quit if file missing or load error. */

	/* Check for tiles string & extract tiles width & height. */
	if (2 != sscanf(graphic_tiles, "%hux%hu", &graphics_tile_wid, &graphics_tile_hgt)) {
		snprintf(use_graphics_errstr, sizeof(use_graphics_errstr), "Couldn't extract tile dimensions from: %s", graphic_tiles);
		return(101);
	}

	if (graphics_tile_wid <= 0 || graphics_tile_hgt <= 0) {
		snprintf(use_graphics_errstr, sizeof(use_graphics_errstr), "Invalid tiles dimensions: %dx%d", graphics_tile_wid, graphics_tile_hgt);
		return(102);
	}

	for (int i = 0; i < MAX_GFX_TILES; i++) c_subtileset[i] = -1;

	if (ANGBAND_DIR_XTRA_GRAPHICS == NULL) {
		/* Build & allocate the graphics path. */
		path_build(path, 1024, ANGBAND_DIR_XTRA, "graphics");
		ANGBAND_DIR_XTRA_GRAPHICS = string_make(path);
	}
	if (ANGBAND_USER_DIR_XTRA_GRAPHICS == NULL) {
		/* Build & allocate the user graphics path. */
		path_build(path, sizeof(path), ANGBAND_USER_DIR_XTRA, "graphics");
		ANGBAND_USER_DIR_XTRA_GRAPHICS = string_make(path);
	}
	if (!disable_tileset_caching) {
		if (!check_dir2(ANGBAND_USER_DIR_XTRA_GRAPHICS)) {
			if (MKDIR(ANGBAND_USER_DIR_XTRA_GRAPHICS) != 0 && errno != EEXIST) {
				logprint(format("Couldn't create SDL3 user graphics directory %s: %s\n", ANGBAND_USER_DIR_XTRA_GRAPHICS, strerror(errno)));
			}
		}
		if (ANGBAND_USER_DIR_XTRA_GRAPHICS_CACHE == NULL) {
			path_build(path, sizeof(path), ANGBAND_USER_DIR_XTRA_GRAPHICS, "cache");
			ANGBAND_USER_DIR_XTRA_GRAPHICS_CACHE = string_make(path);
		}
		if (check_dir2(ANGBAND_USER_DIR_XTRA_GRAPHICS) && !check_dir2(ANGBAND_USER_DIR_XTRA_GRAPHICS_CACHE)) {
			if (MKDIR(ANGBAND_USER_DIR_XTRA_GRAPHICS_CACHE) != 0 && errno != EEXIST) {
				logprint(format("Couldn't create SDL3 tileset cache directory %s: %s\n", ANGBAND_USER_DIR_XTRA_GRAPHICS_CACHE, strerror(errno)));
			}
		}
		if (!check_dir2(ANGBAND_USER_DIR_XTRA_GRAPHICS_CACHE)) {
			logprint("Disabling graphical tileset caching is for this session");
			disable_tileset_caching = true;
		}
	}

	/* Build the name of the graphics file, prefer user storage. */
	path_build(filename_user, 1024, ANGBAND_USER_DIR_XTRA_GRAPHICS, graphic_tiles);
	{
		size_t len = strlen(filename_user);
		if (len + 4 >= sizeof(filename_user)) {
			snprintf(use_graphics_errstr, sizeof(use_graphics_errstr), "Graphics user path too long: %s", filename_user);
			return(101);
		}
		memcpy(filename_user + len, ".bmp", 5);
	}
	path_build(filename_game, 1024, ANGBAND_DIR_XTRA_GRAPHICS, graphic_tiles);
	{
		size_t len = strlen(filename_game);
		if (len + 4 >= sizeof(filename_game)) {
			snprintf(use_graphics_errstr, sizeof(use_graphics_errstr), "Graphics game path too long: %s", filename_game);
			return(101);
		}
		memcpy(filename_game + len, ".bmp", 5);
	}
	if (my_fexists(filename_user) || sdl3_paths_same(ANGBAND_USER_DIR_XTRA_GRAPHICS, ANGBAND_DIR_XTRA_GRAPHICS)) strcpy(filename, filename_user);
	else strcpy(filename, filename_game);

	/* Reset subtile filenames before discovery. */
	for (int i = 0; i < MAX_SUBFONTS; i++) {
		if (!graphic_subtiles[i]) continue;
		graphic_subtiles_file[i][0] = 0;
	}

	/* Discover (partial) subtile files matching selected tileset */
	const char *graphics_dirs[] = {ANGBAND_USER_DIR_XTRA_GRAPHICS, ANGBAND_DIR_XTRA_GRAPHICS};
	int graphics_dir_count = sdl3_paths_same(graphics_dirs[0], graphics_dirs[1]) ? 1 : 2;
	for (int pass = 0; pass < graphics_dir_count; pass++) {
		DIR *dir = opendir(graphics_dirs[pass]);
		if (dir) {
			struct dirent *ent;
			while ((ent = readdir(dir))) {
				char tmp_name[256], *csub, *csub_end;
				int len;

				len = strlen(ent->d_name);
				if (len < 5) continue; /* need at least ".bmp" */
				if (strcmp(ent->d_name + len - 4, ".bmp")) continue;

				strcpy(tmp_name, ent->d_name);
				tmp_name[len - 4] = '\0'; /* strip extension */

				if (!(csub = strchr(tmp_name, '#'))) continue; /* valid sub index marker */
				*csub = 0;
				if (strcmp(tmp_name, graphic_tiles)) continue; /* base filename must match main tileset */
				if (!(csub_end = strchr(csub + 1, '_'))) continue; /* valid sub index terminator */
				*csub_end = 0;

				int idx = atoi(csub + 1);
				if (idx < 0 || idx >= MAX_SUBFONTS) continue;

				/* Accept if enabled, avoid overriding higher priority directory. */
				if (!graphic_subtiles[idx]) continue;
				if (graphic_subtiles_file[idx][0]) continue;
				strncpy(graphic_subtiles_file[idx], ent->d_name, sizeof(graphic_subtiles_file[idx]));
				graphic_subtiles_file[idx][sizeof(graphic_subtiles_file[idx]) - 1] = '\0';
			}
			closedir(dir);
		}
	}

	/* Load .bmp image. */
	graphics_image = SDL_LoadBMP(filename);
	if (graphics_image == NULL) {
		char err_user[256];
		bool tried_fallback = false;

		strncpy(err_user, SDL_GetError(), sizeof(err_user));
		err_user[sizeof(err_user) - 1] = 0;

		/* If user override failed to load, fall back to bundled graphics. */
		if (!strcmp(filename, filename_user) && my_fexists(filename_game)) {
			tried_fallback = true;
			graphics_image = SDL_LoadBMP(filename_game);
			if (graphics_image) strcpy(filename, filename_game);
		}

		if (graphics_image == NULL) {
			if (tried_fallback) {
				snprintf(use_graphics_errstr, sizeof(use_graphics_errstr), "Graphics file \"%s\" SDL_LoadBMP error: %s (fallback \"%s\" error: %s)\n",
				    filename_user, err_user, filename_game, SDL_GetError());
			} else {
				snprintf(use_graphics_errstr, sizeof(use_graphics_errstr), "Graphics file \"%s\" SDL_LoadBMP error: %s\n", filename, err_user);
			}
			return(103);
		}
	}

	/* Check, if format is RGBA32 and convert if not. */
	if (graphics_image->format != SDL_PIXELFORMAT_RGBA32) {
		SDL_Surface* convertedSurface = SDL_ConvertSurface(graphics_image, SDL_PIXELFORMAT_RGBA32);
		if (!convertedSurface) {
			snprintf(use_graphics_errstr, sizeof(use_graphics_errstr), "Error converting loaded image into RGBA32 format: %s\n", SDL_GetError());
			SDL_DestroySurface(graphics_image);
			return(104);
		}
		SDL_DestroySurface(graphics_image);
		graphics_image = convertedSurface;
	}

	/* Ensure the BMP isn't empty or too small */
	if (graphics_image->w < graphics_tile_wid || graphics_image->h < graphics_tile_hgt) {
		snprintf(use_graphics_errstr, sizeof(use_graphics_errstr), "Invalid image dimensions (width x height): %dx%d", graphics_image->w, graphics_image->h);
		SDL_DestroySurface(graphics_image);
		graphics_image = NULL;
		return(105);
	}
	graphics_image_hash = disable_tileset_caching ? 0 : SDL_surface_hash(graphics_image);

	/* Calculate tiles per row. */
	graphics_image_tpr = graphics_image->w / graphics_tile_wid;
	if (graphics_image_tpr <= 0) { /* Paranoia. */
		snprintf(use_graphics_errstr, sizeof(use_graphics_errstr), "Invalid image tiles per row count: %d", graphics_image_tpr);
		SDL_DestroySurface(graphics_image);
		graphics_image = NULL;
		return(106);
	}

	/* Prepare masks colors array and other mask related variables. */
	graphics_image_mpt = 2;
	graphics_image_tpc = 1;
 #ifdef GRAPHICS_BG_MASK
	if (use_graphics == UG_2MASK) {
		graphics_image_mpt = 3;
		graphics_image_tpc = 2;
	}
 #endif

	/* Load (partial) subtile images */
	for (int i = 0; i < MAX_SUBFONTS; i++) {
		if (!graphic_subtiles[i]) continue; /* subset is disabled */
		if (!graphic_subtiles_file[i][0]) continue; /* file not present */

		/* Build file paths for user and bundled subtiles. */
		path_build(filename_user, 1024, ANGBAND_USER_DIR_XTRA_GRAPHICS, graphic_subtiles_file[i]);
		path_build(filename_game, 1024, ANGBAND_DIR_XTRA_GRAPHICS, graphic_subtiles_file[i]);

		/* Resolve available files for this subset, preferring user override with bundled fallback. */
		const bool same_graphics_dir = sdl3_paths_same(ANGBAND_USER_DIR_XTRA_GRAPHICS, ANGBAND_DIR_XTRA_GRAPHICS);
		const bool have_user = my_fexists(filename_user);
		const bool have_game = same_graphics_dir ? have_user : my_fexists(filename_game);
		const char *loadfile = have_user ? filename_user : filename_game;

		SDL_Surface *img = SDL_LoadBMP(loadfile);
		if (!img) {
			char err_user[256];

			strncpy(err_user, SDL_GetError(), sizeof(err_user));
			err_user[sizeof(err_user) - 1] = 0;

			/* If user override failed to load, fall back to bundled graphics. */
			if (have_user && have_game) {
				loadfile = filename_game;
				img = SDL_LoadBMP(loadfile);
			}

			if (!img) {
				if (have_user && have_game) {
					snprintf(use_graphics_errstr, sizeof(use_graphics_errstr), "Graphics subfile \"%s\" SDL_LoadBMP error: %s (fallback \"%s\" error: %s)\n",
					    filename_user, err_user, filename_game, SDL_GetError());
				} else {
					snprintf(use_graphics_errstr, sizeof(use_graphics_errstr), "Graphics subfile \"%s\" SDL_LoadBMP error: %s\n", loadfile, err_user);
				}
				logprint(format("%s\n", use_graphics_errstr));
				graphic_subtiles[i] = false;
				continue;
			}
		}

		/* Normalize format so blitting logic can assume RGBA32 everywhere. */
		if (img->format != SDL_PIXELFORMAT_RGBA32) {
			SDL_Surface* convertedSurface = SDL_ConvertSurface(img, SDL_PIXELFORMAT_RGBA32);
			if (!convertedSurface) {
				snprintf(use_graphics_errstr, sizeof(use_graphics_errstr), "Error converting subimage into RGBA32 format: %s\n", SDL_GetError());
				SDL_DestroySurface(img);
				logprint(format("%s\n", use_graphics_errstr));
				graphic_subtiles[i] = false;
				continue;
			}
			SDL_DestroySurface(img);
			img = convertedSurface;
		}

		/* Bail out on malformed files to avoid divide-by-zero and layout bugs. */
		if (img->w < graphics_tile_wid || img->h < graphics_tile_hgt) {
			snprintf(use_graphics_errstr, sizeof(use_graphics_errstr), "Invalid subimage dimensions (width x height): %dx%d", img->w, img->h);
			logprint(format("%s\n", use_graphics_errstr));
			SDL_DestroySurface(img);
			graphic_subtiles[i] = false;
			continue;
		}

		graphics_image_tpr_sub[i] = img->w / graphics_tile_wid;
		if (graphics_image_tpr_sub[i] <= 0) {
			snprintf(use_graphics_errstr, sizeof(use_graphics_errstr), "Invalid subimage tiles per row count: %d", graphics_image_tpr_sub[i]);
			logprint(format("%s\n", use_graphics_errstr));
			SDL_DestroySurface(img);
			graphic_subtiles[i] = false;
			continue;
		}

		/* Keep a successfully loaded subset around for rendering. */
		graphics_image_sub_hash[i] = disable_tileset_caching ? 0 : SDL_surface_hash(img);
		graphics_image_sub[i] = img;
	}

	/* Flag to recreate term surfaces with the newly loaded graphics. */
	graphics_reinitialize = true;

	return(0);
}

static void nuke_graphics_sdl3(void) {
		for (int i = 0; i < ANGBAND_TERM_MAX; i++) {
			term_data *td = term_idx_to_term_data(i);
			if (!td) continue;
 #ifdef GRAPHICS_BG_MASK
			td->t.pict_hook_2mask = NULL;
 #endif
			td->t.pict_hook = NULL;
			td->t.rawpict_hook = NULL;
			td->t.higher_pict = false;
			free_graphics(td);
		}
		if (graphics_image) {
			SDL_DestroySurface(graphics_image);
			graphics_image = NULL;
		}
		for (int s = 0; s < MAX_SUBFONTS; s++) {
			if (graphics_image_sub[s]) {
				SDL_DestroySurface(graphics_image_sub[s]);
				graphics_image_sub[s] = NULL;
			}
			graphics_image_tpr_sub[s] = 0;
		}
		graphics_image_tpr = 0;
		graphics_image_mpt = 0;
		graphics_image_tpc = 0;
		graphics_image_hash = 0;
		WIPE(graphics_image_sub_hash, graphics_image_sub_hash);
		graphics_reinitialize = false;
}

static void reset_graphics_masks_sdl3(void) {
	WIPE(graphics_image_masks_colors_prefs, graphics_image_masks_colors_prefs);
	WIPE(graphics_image_masks_colors_sub_prefs, graphics_image_masks_colors_sub_prefs);
	WIPE(graphics_image_masks_colors, graphics_image_masks_colors);
	WIPE(graphics_image_masks_colors_sub, graphics_image_masks_colors_sub);
	WIPE(graphics_image_sub_prefs_has_masks, graphics_image_sub_prefs_has_masks);
	WIPE(graphics_image_sub_prefs_has_outline, graphics_image_sub_prefs_has_outline);
	graphics_image_prefs_has_masks = false;
	graphics_image_prefs_has_outline = false;
}

bool sdl3_set_graphics_mode(byte mode) {
	if (mode == use_graphics) return(true);

	byte previous = use_graphics;
	use_graphics = mode;

	if (previous != UG_NONE) nuke_graphics_sdl3();

	if (use_graphics == UG_NONE) return(true);

	reset_graphics_masks_sdl3();

	use_graphics_err = init_graphics_sdl3();
	if (use_graphics_err != 0) {
		use_graphics = UG_NONE;
		return(false);
	}

	return(true);
}

bool sdl3_reload_graphics_tileset(void) {
	if (use_graphics == UG_NONE) return(true);

	nuke_graphics_sdl3();

	reset_graphics_masks_sdl3();

	use_graphics_err = init_graphics_sdl3();
	if (use_graphics_err != 0) {
		return(false);
	}

	return(true);
}
#endif
/*
 * Initialization function for an "SDL3" module to Angband
 */
errr init_sdl3(void) {
	SDL_DisplayID display_id;
	const SDL_DisplayMode *dm;
	const SDL_PixelFormatDetails *pixel_format;
	bool dpy_color = false;
	int i;

#ifdef SDL_PLATFORM_LINUX
	/*
	 * Prefer X11 because Wayland does not allow applications to control the absolute placement of top-level windows.
	 * Users can override this with SDL_VIDEO_DRIVER.
	 */
	SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11,wayland");
#endif

	if (SDL_Init(SDL_INIT_VIDEO) == false) {
		fprintf(stderr, "ERROR: init_sdl3: Video SDL_Init error: %s\n", SDL_GetError());
		return(-1);
	}
	/* Ensure SDL shuts down on exit */
	atexit(SDL_Quit);

	if (TTF_Init() == false) {
		fprintf(stderr, "ERROR: init_sdl3: TTF_Init error: %s\n", SDL_GetError());
		SDL_Quit();
		return(-1);
	}

	/* Initialize SDL_net */
	if (!NET_Init()) {
		fprintf(stderr, "ERROR: init_sdl3: NET_Init error: %s\n", SDL_GetError());
		TTF_Quit();
		SDL_Quit();
		return 1;
	}
	/* Close net gracefully at exit. */
	atexit(NET_Quit);

	display_id = SDL_GetPrimaryDisplay();
	dm = SDL_GetCurrentDisplayMode(display_id);
	if (!dm) {
		fprintf(stderr, "ERROR: init_sdl3: SDL_GetCurrentDisplayMode failed: %s", SDL_GetError());
		NET_Quit();
		TTF_Quit();
		SDL_Quit();
		return(-1);
	}

	pixel_format = SDL_GetPixelFormatDetails(dm->format);
	if (pixel_format) {
		dpy_color = ((pixel_format->bits_per_pixel > 1) ? 1 : 0);
	}

#ifdef CUSTOMIZE_COLOURS_FREELY
	/* Actually use fg-colour of index #0 (usually black) as the bg colour. */
	color_default_bg = (Pixel){
		.r = (client_color_map[0] >> 16 ) & 0xFF,
		.g = (client_color_map[0] >> 8 ) & 0xFF,
		.b = client_color_map[0] & 0xFF,
		.a = 0xFF
	};
#endif

	/* set OS-specific resize_main_window() hook */
	resize_main_window = resize_main_window_sdl3;

	enable_common_colormap_sdl3();

	for (i = 0; i < CLIENT_PALETTE_SIZE; ++i) {
		cptr cname = color_name[0];

		MAKE(clr[i], infoclr);
		Infoclr_set(clr[i]);
		if (dpy_color) cname = color_name[i];
		else if (i) cname = color_name[1];
		Infoclr_init_parse(cname, "bg");
	}

#ifdef EXTENDED_BG_COLOURS
	/* Prepare the extended background-using colors */
	for (i = 0; i < TERMX_AMT; ++i) {
		cptr cname = color_name[0], cname2 = color_name[0];

		MAKE(clr[CLIENT_PALETTE_SIZE + i], infoclr);
		Infoclr_set(clr[CLIENT_PALETTE_SIZE + i]);
		if (dpy_color) {
			cname = color_ext_name[i][0];
			cname2 = color_ext_name[i][1];
		}
		Infoclr_init_parse(cname, cname2);
	}
#endif



	/* Initialize paths, to get access to lib directories. */
	init_stuff();

	char path[1024];
	/* Build the "game font" path. */
	path_build(path, 1024, ANGBAND_DIR_XTRA, "font");
	ANGBAND_DIR_XTRA_FONT = string_make(path);

	/* Build the "user font" path. */
	path_build(path, 1024, ANGBAND_USER_DIR_XTRA, "font");
	ANGBAND_USER_DIR_XTRA_FONT = string_make(path);

#ifdef USE_GRAPHICS
	if (use_graphics) {
		use_graphics_err = init_graphics_sdl3();
		if (use_graphics_err != 0) {
 #ifndef GFXERR_FALLBACK
			quit_fmt("Graphics load error (%d): %s\n", use_graphics_err, use_graphics_errstr);
 #else
			fprintf(stderr, "Graphics load error (%d): %s\n", use_graphics_err, use_graphics_errstr);
			printf("Disabling graphics and falling back to normal text mode.\n");
			use_graphics = 0;
			/* Actually also show it as 'off' in =g menu, as in, "desired at config-file level" */
			use_graphics_new = false;
 #endif
		}
	}
#endif /* USE_GRAPHICS */

	/* Initialize each term */
	/* When run through WINE, the main window does not rise above all others. Initialize it last. */
	for (i = 1; i < ANGBAND_TERM_MAX; i++) {
		/* Visibility depends on configuration. */
		if (term_prefs[i].visible) {
			if (sdl3_term_init(i) != 0) {
				fprintf(stderr, "Error initializing SDL3 terminal window with index %d\n", i);
			}
		}
	}
	if (sdl3_term_init(0) != 0) {
		fprintf(stderr, "Error initializing SDL3 main terminal window\n");
		/* Can't run without main screen. */
		return(1);
	}

	/* Activate the "Angband" main window screen. */
	Term_activate(&term_main.t);

	/* Raise the "Angband" main window. */
	/* Sync before raise, otherwise the rise fails sometimes. */
	Infowin_set(term_main.win);
	SDL_SyncWindow(Infowin->window);
	Infowin_raise();

#ifdef USE_GRAPHICS
	if (use_graphics) {
		/* Some preferences that affect graphic tile initialization are not loaded yet. Initialization is therefore done in `sdl3_graphics_pref_file_processed()`, which runs after relevant preferences are loaded. */

		/* Give a message so user doesn't think the client froze while initializing graphics and preparing tiles. */
		Term_putstr(1, 12, -1, TERM_WHITE, "\377o                 Please wait while initializing graphics...");
		Term_fresh();
		Term_flush();

	}
#endif

	/* Success */
	return(0);
}

/* EXPERIMENTAL: allow user to change main window font live - C. Blue
 * So far only uses 1 parm ('s') to switch between hardcoded choices:
 * -1 - cycle
 *  0 - normal
 *  1 - big
 *  2 - bigger
 *  3 - huge */
void change_font(int s) {
	static const char *pcf_fonts[4][ANGBAND_TERM_MAX] = {
		{"8x13", "8x13", "8x13", "5x8", "6x10", "6x10", "5x8", "5x8", "5x8", "5x8"},
		{"9x15", "9x15", "9x15", "6x10", "8x13", "8x13", "6x10", "6x10", "6x10", "6x10"},
		{"10x20", "10x20", "10x20", "8x13", "9x15", "9x15", "8x13", "8x13", "8x13", "8x13"},
		{"16x22", "16x22", "16x22", "9x15", "10x20", "10x20", "9x15", "9x15", "9x15", "9x15"}
	};
	char font_name[128] = "";
	char font_base[128];
	int cycle_type;
	term *old_term;
	term_data *td;

	/* use main window font for measuring */
	if (term_main.fnt->name) {
		strcpy(font_name, term_main.fnt->name);
		cycle_type = term_main.fnt->type;
	} else {
		strcpy(font_name, SDL3_DEFAULT_FONT);
		if (is_pcf_font(font_name)) {
				cycle_type = FONT_TYPE_PCF;
		} else {
				cycle_type = FONT_TYPE_TTF;
		}
	}

	/* cycle? */
	if (s == -1) {
		if (cycle_type == FONT_TYPE_PCF) {
			if (strstr(font_name, pcf_fonts[0][0])) s = 1;
			else if (strstr(font_name, pcf_fonts[1][0])) s = 2;
			else if (strstr(font_name, pcf_fonts[2][0])) s = 3;
			else if (strstr(font_name, pcf_fonts[3][0])) s = 0;
		} else {
			if (strstr(font_name, " 10")) s = 1;
			else if (strstr(font_name, " 12")) s = 2;
			else if (strstr(font_name, " 14")) s = 3;
			else if (strstr(font_name, " 16")) s = 0;
		}
	}

	if (s < 0 || s > 3) return;

	/* Save current active term. */
	old_term = Term;

	/* Force the new font or size. */
	if (cycle_type == FONT_TYPE_PCF) { /* PCF */
		for (int i = 0; i < ANGBAND_TERM_MAX; i++) {
			term_force_font(i, pcf_fonts[s][i]);

			/* If terminal is visible, repaint to avoid stale graphics after the font swap. */
			if (!term_get_visibility(i)) continue;
			if (term_prefs[i].x == -32000 || term_prefs[i].y == -32000) continue;

			td = term_idx_to_term_data(i);
			if (!td || !td->fnt) continue;

			Term_activate(&td->t);
			Term_redraw();
		}
	} else { /* TTF */
		/* Just extract font name without size. */
		is_ttf_font(font_name, font_base, sizeof(font_base), NULL);

		for (int i = 0; i < ANGBAND_TERM_MAX; i++) {
			/* Compute new size and apply. */
			int size = sdl3_terms_ttf_size_default[i] + s*2;
			if (size > SDL3_MAX_TTF_FONT_SIZE) size = SDL3_MAX_TTF_FONT_SIZE;
			snprintf(font_name, sizeof(font_name), "%s %d", font_base, size);
			term_force_font(i, font_name);

			/* If terminal is visible, repaint to avoid stale graphics after the font swap. */
			if (!term_get_visibility(i)) continue;
			if (term_prefs[i].x == -32000 || term_prefs[i].y == -32000) continue;

			td = term_idx_to_term_data(i);
			if (!td || !td->fnt) continue;

			Term_activate(&td->t);
			Term_redraw();
		}
	}

	/* Activate saved term. */
	Term_activate(old_term);

}

static void term_force_font(int term_idx, cptr fnt_name) {
	term_data *td = term_idx_to_term_data(term_idx);

	/* non-visible window has no fnt-> .. */
	if (!term_get_visibility(term_idx)) return;

	/* special hack: this window was invisible, but we just toggled
	   it to become visible on next client start. - C. Blue */
	if (!td->fnt) return;

	/* Save current font size. */
	int old_fnt_wid = td->fnt->wid;
	int old_fnt_hgt = td->fnt->hgt;

	/* Create and initialize font. */
	infofnt *new_font;
	MAKE(new_font, infofnt);
	infofnt *old_infofnt = Infofnt;
	Infofnt_set(new_font);
	if (Infofnt_init(fnt_name, td->display_scale)) {
		/* Failed to initialize. */
		fprintf(stderr, "Error forcing the \"%s\" font on terminal %d\n", fnt_name, term_idx);
		Infofnt_set(old_infofnt);
		if(in_game) {
			plog_fmt("Failed to load the \"%s\" font!", fnt_name);
		}
		FREE(new_font, infofnt);
		return;
	}

	/* New font was successfully initialized, free the old one and use the new one. */
	if (td->fnt->name) string_free(td->fnt->name);
	if (td->fnt->font) {
		if (td->fnt->type == FONT_TYPE_TTF) TTF_CloseFont((TTF_Font*)td->fnt->font);
		if (td->fnt->type == FONT_TYPE_PCF) PCF_CloseFont((PCF_Font*)td->fnt->font);
	}
	FREE(td->fnt, infofnt);
	td->fnt = new_font;

	/* Resize the windows if any "change" is needed */
	if ((old_fnt_wid != td->fnt->wid) || (old_fnt_hgt != td->fnt->hgt)) {

		resize_term_with_window(term_idx, td->t.wid, td->t.hgt);

#ifdef USE_GRAPHICS
		/* Resize graphic tiles if needed too. */
		if (use_graphics) {
			/* Recreate tile surfaces and restore graphics hooks. */
			if (term_data_enable_graphics(term_idx, td)) {
				logprint(format("Couldn't prepare images after font resize in terminal %d, disabling graphics.\n", term_idx));
			}
		}
#endif
	}
	SDL_UpdateWindowSurface(td->win->window);

	/* Reload custom font prefs on main screen font change */
	if (td == &term_main) handle_process_font_file();
}

/* Resizes the graphics terminal window described by 'td' to dimensions given in 'cols', 'rows' inputs.
 * Stops the resize timer and validates input.
 * Resizes the SDL3 window if current dimensions don't match the validated dimensions.
 * The terminal stored in 'td->t' is resized to desired size if needed.
 * When the window is the main window, update the screen globals, handle bigmap and notify server if in game.
 */
void resize_term_with_window(int term_idx, int cols, int rows) {
	/* The 'term_idx_to_term_data()' returns '&term_main' if 'term_idx' is out of bounds and it is not desired to resize term_main terminal window in that case, so validate before. */
	if (term_idx < 0 || term_idx >= ANGBAND_TERM_MAX) {
		fprintf(stderr, "ERROR: Can't resize terminal #%d, wrong terminal number\n", term_idx);
		return;
	}
	term_data *td = term_idx_to_term_data(term_idx);

	/* Clear timer. */
	td->resize_timer = 0;

	int rows_want = rows;
	/* Validate input dimensions. */
	/* Our 'term_data' indexes in 'term_idx' are the same as 'ang_term' indexes so it's safe to use 'validate_term_dimensions()'. */
	validate_term_dimensions(term_idx, &cols, &rows);
	/* Are we actually enlarging the window? */
	if (td == &term_main && rows_want > td->t.hgt && rows < rows_want && screen_hgt == SCREEN_HGT) {
		/* Don't enlarge when usabe display height is too small for big screen. */
		SDL_DisplayID display = SDL_GetDisplayForWindow(td->win->window);
		SDL_Rect bounds;
		if (display && SDL_GetDisplayBounds(display, &bounds)) {
			if (td->fnt->hgt * (MAX_SCREEN_HGT + SCREEN_PAD_Y) <= bounds.h) {
				/* Usable display height should be enough, try to enlarge the window to big screen. */
				rows = MAX_SCREEN_HGT + SCREEN_PAD_Y;
			}
		} else {
			fprintf(stderr, "Warning: Couldn't get display bounds for window #%d: %s\n", td->win->windowID, SDL_GetError());
		}
	}

	/* Calculate dimensions in pixels. */
	uint16_t wid_draw = cols * td->fnt->wid;
	uint16_t hgt_draw = rows * td->fnt->hgt;
	uint16_t wid_border = wid_draw + (2 * SDL3_DEFAULT_BORDER_WIDTH);
	uint16_t hgt_border = hgt_draw + (2 * SDL3_DEFAULT_BORDER_WIDTH);

	/* Save current Infowin and activated Term. */
	infowin *iwin = Infowin;
	term *t = Term;

	/* Activate Infowin and Term from term data belonging to term_idx. */
	Infowin_set(td->win);
	Term_activate(&td->t);

	/* Correct the size of the window if new calculated dimensions differ. */
	if (Infowin->wb != wid_border || Infowin->hb != hgt_border) {
		/* If window is maximized and the dimensions are not as calculated, after we try to correct the size, window manager will again maximize the window again. That will result in a resize loop. To disrupt the loop, don't correct the size of window, if window is maximized. */
		uint32_t window_flags = SDL_GetWindowFlags(td->win->window);
		if ((window_flags & SDL_WINDOW_MAXIMIZED) == false) {
			/* Resize may be rejected or the compositor could change its requested size. If yes, don't fight back and accept the size. */
			if (Infowin_resize(wid_border, hgt_border)) {
				fprintf(stderr, "Error correcting window: %s\n", SDL_GetError());
			} else {
				// On error, abort everything, restore previous state, resize wasn't rejected, event will be triggered, maybe we can recover there. */
				if (!SDL_SyncWindow(td->win->window)) {
					fprintf(stderr, "Warning: Timed out synchronizing window #%d: %s\n", td->win->windowID, SDL_GetError());
					Infowin_set(iwin);
					Term_activate(t);
					return;
				}
				/* Window was resized, surface needs to be regenerated too. */
				Infowin->surface = SDL_GetWindowSurface(Infowin->window);
				if (!Infowin->surface) {
					fprintf(stderr, "Error: Can't get resized window #%d surface: %s\n", Infowin->windowID, SDL_GetError());
					Infowin_set(iwin);
					Term_activate(t);
					return;
				}
				int w, h;
				if (!SDL_GetWindowSize(td->win->window, &w, &h)) {
					fprintf(stderr, "Error: Can't get window #%d dimensions: %s\n", td->win->windowID, SDL_GetError());
					Infowin_set(iwin);
					Term_activate(t);
					return;
				}
				Infowin->wb = w;
				Infowin->hb = h;
			}
		}

		/* We wanted to rezize, but te window is maximized or resize was rejected or was accepted, but compositor could change the dimensions. */
		/* Either way recalculate available cols/rows, window drawing sizes and border sizes. */
		available_tiles(td, &cols, &rows);
		validate_term_dimensions(term_idx, &cols, &rows);

		wid_draw = cols * td->fnt->wid;
		hgt_draw = rows * td->fnt->hgt;
		wid_border = Infowin->wb;
		hgt_border = Infowin->hb;

		/* Calculated widow sizes with borders may exceed current window sizes. Readjust if necessary. */
		if (wid_draw > wid_border) wid_draw = wid_border;
		if (hgt_draw > hgt_border) hgt_draw = hgt_border;
	}

	/* Calculate drawing box ad border dimensions. */
	Infowin->wd = wid_draw;
	Infowin->hd = hgt_draw;
	Infowin->wb = wid_border;
	Infowin->hb = hgt_border;
	Infowin->bw = (Infowin->wb - Infowin->wd + 1)/2;
	Infowin->bh = (Infowin->hb - Infowin->hd + 1)/2;


	/* Make the changes go live (triggers on next c_message_add() call). No need to check if dimensions differ, Term_resize handles it. */
	Term_resize(cols, rows);

	/* Main screen is special. Update the screen_wid/hgt globals if needed and notify server about it if in game. */
	if (td == &term_main) {

		int new_screen_cols = cols - SCREEN_PAD_X;
		int new_screen_rows = rows - SCREEN_PAD_Y;

		/* If in game, avoid bottom line of garbage left from big_screen when shrinking to normal screen. */
		if (in_game && new_screen_rows < screen_hgt) clear_from(SCREEN_HGT + SCREEN_PAD_Y - 1);

		if (screen_wid != new_screen_cols || screen_hgt != new_screen_rows) {
			screen_wid = new_screen_cols;
			screen_hgt = new_screen_rows;

			if (in_game) {
				/* Switch big_map mode. */
#ifndef GLOBAL_BIG_MAP
				if (Client_setup.options[CO_BIGMAP] && rows == DEFAULT_TERM_HGT) {
					c_cfg.big_map = false;
					Client_setup.options[CO_BIGMAP] = false;
				} else if (!Client_setup.options[CO_BIGMAP] && rows != DEFAULT_TERM_HGT) {
					c_cfg.big_map = true;
					Client_setup.options[CO_BIGMAP] = true;
				}
#else
				if (global_c_cfg_big_map && rows == DEFAULT_TERM_HGT) {
					global_c_cfg_big_map = false;
				} else if (!global_c_cfg_big_map && rows != DEFAULT_TERM_HGT) {
					global_c_cfg_big_map = true;
				}
#endif
				/* Notify server and ask for a redraw. */
				Send_screen_dimensions();
			}
		}
	}

	Infowin_wipe();
	Term_win_update(1, 0, 0);
	Term_redraw();

	/* Ask server for a redraw if in game. */
	if (in_game) {
		if (td != &term_main) {
			/* Mark all windows for content refresh. */
			p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER | PW_MSGNOCHAT | PW_MESSAGE | PW_CHAT | PW_CLONEMAP | PW_SUBINVEN); /* PW_LAGOMETER is called automatically, no need. */
		}
		cmd_redraw();
	}

	/* Restore saved Infowin and Tterm. */
	Infowin_set(iwin);
	Term_activate(t);
}

/* Resizes main terminal window to dimensions in input. */
/* Used for OS-specific resize_main_window() hook. */
void resize_main_window_sdl3(int cols, int rows) {
	resize_term_with_window(0, cols, rows);
}

bool ask_for_bigmap(void) {
	return(ask_for_bigmap_generic());
}

const char* get_font_name(int term_idx) {
	term_data *td = term_idx_to_term_data(term_idx);

	if (td->fnt) return(td->fnt->name);
	if (strlen(term_prefs[term_idx].font)) return(term_prefs[term_idx].font);
	return(sdl3_terms_font_default[term_idx]);
}

void set_font_name(int term_idx, char* fnt) {
	term_data *td;
	char sanitized_fnt[256];

	if (term_idx < 0 || term_idx >= ANGBAND_TERM_MAX) {
		fprintf(stderr, "Terminal index %d is out of bounds for set_font_name\n", term_idx);
		return;
	}

	strncpy(sanitized_fnt, fnt, sizeof(sanitized_fnt));
	sanitized_fnt[sizeof(sanitized_fnt)-1] = '\0';
	sanitize_font_format(sanitized_fnt, sizeof(sanitized_fnt), term_idx);

	if (!term_get_visibility(term_idx)) {
		/* Terminal is not visible, Do nothing, just change the font name in preferences. */
		if (strcmp(term_prefs[term_idx].font, sanitized_fnt) != 0) {
			strncpy(term_prefs[term_idx].font, sanitized_fnt, sizeof(term_prefs[term_idx].font));
			term_prefs[term_idx].font[sizeof(term_prefs[term_idx].font) - 1] = '\0';
		}
		return;
	}

	term_force_font(term_idx, sanitized_fnt);

	/* Redraw the terminal for which the font was forced. */
	td = term_idx_to_term_data(term_idx);
	if (&td->t != Term) {
		/* Terminal for which the font was forced is not activated. Activate, redraw and activate the terminal before. */
		term *old_term = Term;
		Term_activate(&td->t);
		Term_redraw();
		Term_activate(old_term);
	} else {
		/* Terminal for which the font was forced is currently activated. Just redraw. */
		Term_redraw();
	}
}

void term_toggle_visibility(int term_idx) {
	if (term_idx == 0) {
		fprintf(stderr, "Warning: Toggling visibility for main terminal window is not allowed\n");
		return;
	}

	if (term_get_visibility(term_idx)) {
		/* Window is visible. Save it, close it and free its resources. */

		/* Save window position, dimension and font to term_prefs, cause at quitting the nuke_hook won't be called for closed windows. */
		term_data_to_term_prefs(term_idx);
		term_prefs[term_idx].visible = false;

		/* Destroy window and free resources. */
		term_nuke(ang_term[term_idx]);
		ang_term[term_idx] = NULL;
		return;
	}
	/* Window is not visible. Create it and draw content. */

	/* Create and initialize terminal window. */
	errr err = sdl3_term_init(term_idx);
#ifdef USE_GRAPHICS
	if (!err && use_graphics) {
		if (term_data_enable_graphics(term_idx, term_idx_to_term_data(term_idx))) {
			logprint(format("Couldn't prepare images for terminal %d after toggling visibility, disabling graphics.\n", term_idx));
		}
	}
#endif
	/* After initializing the new window is active. Switch to main window. */
	Term_activate(&term_main.t);

	if (err) {
		fprintf(stderr, "Error initializing toggled X11 terminal window with index %d\n", term_idx);
		return;
	}
	/* Window was successfully created. */
	term_prefs[term_idx].visible = true;

	/* Mark all windows for content refresh. */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER | PW_MSGNOCHAT | PW_MESSAGE | PW_CHAT | PW_CLONEMAP | PW_SUBINVEN); /* PW_LAGOMETER is called automatically, no need. */
}

/* Returns true if terminal window specified by term_idx is currently visible. */
bool term_get_visibility(int term_idx) {
	if (term_idx < 0 || term_idx >= ANGBAND_TERM_MAX) return(false);

	/* Only windows initialized in ang_term array are currently visible. */
	return((bool)ang_term[term_idx]);
}

void apply_window_decorations(void) {
	bool bordered = sdl3_window_decorations ? true : false;
	for (int i = 0; i < ANGBAND_TERM_MAX; i++) {
		if (!term_get_visibility(i)) continue;
		term_data *td = term_idx_to_term_data(i);
		SDL_SetWindowBordered(td->win->window, bordered);
	}
}

void get_term_main_font_name(char *buf) {
	if (!buf) return;

	/* fonts aren't available in command-line mode */
	if (!strcmp(ANGBAND_SYS, "gcu")) {
		buf[0] = 0;
		return;
	}

	snprintf(buf, 1024, "%s", (term_main.fnt && term_main.fnt->name) ? term_main.fnt->name : "");
}

/* automatically store name+password to ini file if we're a new player? */
void store_crecedentials(void) {
	write_mangrc(true, true, false);
}

/* Palette animation - 2018 *testing* */
void animate_palette(void) {
	byte i;
	byte rv, gv, bv;
	unsigned long code;
	char cn[8], tmp[3];

	static bool init = false;
	static unsigned char ac = 0x00; /* animation */
	term_data *old_td;


	/* Initialise the palette once. For some reason colour_table[] is all zero'ed again at the beginning. */
	tmp[2] = 0;
	if (!init) {
		for (i = 0; i < BASE_PALETTE_SIZE; i++) {
			/* Extract desired values */
			rv = color_table[i][1];
			gv = color_table[i][2];
			bv = color_table[i][3];

			/* Extract a full color code */
			code = (rv << 16) | (gv << 8) | bv;
			sprintf(cn, "#%06lx", code);

			c_message_add(format("currently: [%d] %s -> %d (%d,%d,%d)", i, color_name[i], cn, rv, gv, bv));

			/* Save the "complex" codes */
			tmp[0] = color_name[i][1];
			tmp[1] = color_name[i][2];
			rv = strtol(tmp, NULL, 16);
			color_table[i][1] = rv;
			tmp[0] = color_name[i][3];
			tmp[1] = color_name[i][4];
			gv = strtol(tmp, NULL, 16);
			color_table[i][2] = gv;
			tmp[0] = color_name[i][5];
			tmp[1] = color_name[i][6];
			bv = strtol(tmp, NULL, 16);
			color_table[i][3] = bv;

			c_message_add(format("init to: %s = %d,%d,%d", color_name[i], rv, gv, bv));

			/* Save the "simple" code */
			color_table[i][0] = '#';
		}
		init = true;
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


	/* Save the default colors */
	for (i = 0; i < BASE_PALETTE_SIZE; i++) {
		/* Extract desired values */
		rv = color_table[i][1];
		gv = color_table[i][2];
		bv = color_table[i][3];

		/* Extract a full color code */
		/* code = PALETTERGB(rv, gv, bv); */
		code = (rv << 16) | (gv << 8) | bv;
		sprintf(cn, "#%06lx", code);

		/* Activate changes */
		if (strcmp(color_name[i], cn)) {
			/* Apply the desired color */
			sprintf(color_name[i], "#%06lx", code & 0xFFFFFFL);
			c_message_add(format("changed [%d] %d -> %d (%d,%d,%d)", i, color_name[i], code, rv, gv, bv));
		}
	}

	/* Activate the palette */
	for (i = 0; i < BASE_PALETTE_SIZE; ++i) {
		cptr cname = color_name[0];

		Infoclr_set (clr[i]);
#if 0 /* no colors on this display? */
		if (Metadpy->color) cname = color_name[i];
		else if (i) cname = color_name[1];
#else
		cname = color_name[i];
#endif
		Infoclr_init_parse(cname, "bg");
	}

	old_td = (term_data*)(Term->data);
	/* Refresh aka redraw windows with new colour */
	for (i = 0; i < ANGBAND_TERM_MAX; i++) {

		if (!term_get_visibility(i)) continue;
		if (term_prefs[i].x == -32000 || term_prefs[i].y == -32000) continue;

		Term_activate(&term_idx_to_term_data(i)->t);
		Term_xtra(TERM_XTRA_FRESH, 0);
	}
	Term_activate(&old_td->t);
}

#define PALANIM_OPTIMIZED /* KEEP SAME AS SERVER! */
/* Accept a palette entry index (NOT a TERM_ colour) and sets its R/G/B values from 0..255. - C. Blue */
void set_palette(byte c, byte r, byte g, byte b) {
	unsigned long code;
	char cn[8];
	cptr cname = color_name[0]; /* , bcname = "bg"; <- todo, for cleaner code */
	term_data *old_td = (term_data*)(Term->data);

#ifdef PALANIM_OPTIMIZED
	/* Check for refresh marker at the end of a palette data transmission */
	if (c == 127 || c == 128) {
		/* Refresh aka redraw the main window with new colour */
		if (!term_get_visibility(0)) return;
		if (term_prefs[0].x == -32000 || term_prefs[0].y == -32000) return;
		Term_activate(&term_idx_to_term_data(0)->t);
		/* Invalidate cache to ensure redrawal doesn't get cancelled by tile-caching */
		if (c_cfg.gfx_palanim_repaint || (c_cfg.gfx_hack_repaint && !gfx_palanim_repaint_hack))
			/* Alternative function for flicker-free redraw - C. Blue */
			/* Include the UI elements, which is required if we use ANIM_FULL_PALETTE_FLASH or ANIM_FULL_PALETTE_LIGHTNING  - C. Blue */
			Term_repaint(0, 0, CL_WINDOW_WID, CL_WINDOW_HGT);
		else {
			Term_redraw();
			gfx_palanim_repaint_hack = false;
		}
		Term_activate(&old_td->t);
		return;
	}
#else
	if (c == 127 || c == 128) return; /* just discard refresh marker */
#endif

	color_table[c][1] = r;
	color_table[c][2] = g;
	color_table[c][3] = b;

	/* Extract a full color code */
	code = (r << 16) | (g << 8) | b;
	sprintf(cn, "#%06lx", code);

	/* Activate changes */
#ifndef EXTENDED_BG_COLOURS
	if (strcmp(color_name[c], cn))
		/* Apply the desired color */
		strcpy(color_name[c], cn);
#else
	/* Testing: for extended-bg colors, currently animate the background part. */
	if (c >= CLIENT_PALETTE_SIZE) { /* TERMX_.. */
		if (strcmp(color_ext_name[c - CLIENT_PALETTE_SIZE][1], cn))
			/* Apply the desired color */
			strcpy(color_ext_name[c - CLIENT_PALETTE_SIZE][1], cn);
	} else {
		/* Normal colour: Just set the foreground part */
		if (strcmp(color_name[c], cn))
			/* Apply the desired color */
			strcpy(color_name[c], cn);
	}
#endif

	/* Activate the palette */
	Infoclr_set(clr[c]);

#ifndef EXTENDED_BG_COLOURS
	/* Foreground colour */
	cname = color_name[c];
#else
	/* For extended colors actually use background colour instead, as this interests us most atm */
	if (c >= CLIENT_PALETTE_SIZE) /* TERMX_.. */
		cname = color_ext_name[c - CLIENT_PALETTE_SIZE][1];
	/* Foreground colour */
	else cname = color_name[c];
#endif

#ifdef EXTENDED_BG_COLOURS
	/* Just for testing for now.. */
	if (c >= CLIENT_PALETTE_SIZE) { /* TERMX_.. */
		/* Actually animate the 'bg' colour instead of the 'fg' colour (testing purpose) */
		Infoclr_init_parse(color_ext_name[c - CLIENT_PALETTE_SIZE][0], cname);
	} else
#endif
	Infoclr_init_parse(cname, "bg");

#ifndef PALANIM_OPTIMIZED
	/* Refresh aka redraw the main window with new colour */
	if (!term_get_visibility(0)) return;
	if (term_prefs[0].x == -32000 || term_prefs[0].y == -32000) return;
	Term_activate(&term_idx_to_term_data(0)->t);
	Term_xtra(TERM_XTRA_FRESH, 0);
	Term_activate(&old_td->t);
#endif
}

/* Gets R/G/B values from 0..255 for a specific terminal palette entry (not for a TERM_ colour). */
void get_palette(byte c, byte *r, byte *g, byte *b) {
	*r = clr[c]->fg.r;
	*g = clr[c]->fg.g;
	*b = clr[c]->fg.b;
}

/* Redraw all term windows with current palette values. */
void refresh_palette(void) {
	int i;
	term_data *old_td = (term_data*)(Term->data);

	set_palette(128, 0, 0, 0);

	/* Refresh aka redraw windows with new colour (term 0 is already done in set_palette(128) line above) */
	for (i = 1; i < ANGBAND_TERM_MAX; i++) {
		if (!term_get_visibility(i)) continue;
		if (term_prefs[i].x == -32000 || term_prefs[i].y == -32000) continue;

		Term_activate(&term_idx_to_term_data(i)->t);
		Term_redraw();
		/* Term_xtra(TERM_XTRA_FRESH, 0); */
	}

	Term_activate(&old_td->t);
}

void set_window_title_sdl3(int term_idx, cptr title) {
	term_data *td;

	/* The 'term_idx_to_term_data()' returns '&term_main' if 'term_idx' is out of bounds and it is not desired to resize term_main terminal window in that case, so validate before. */
	if (term_idx < 0 || term_idx >= ANGBAND_TERM_MAX) return;

	/* Trying to change title in this state causes a crash */
	if (!term_get_visibility(term_idx)) return;
	if (term_prefs[term_idx].x == -32000 || term_prefs[term_idx].y == -32000) return;

	td = term_idx_to_term_data(term_idx);

	/* Save current Infowin. */
	infowin *iwin = Infowin;

	Infowin_set(td->win);
	Infowin_set_name(ang_term_name[term_idx]);

	/* Restore saved Infowin. */
	Infowin_set(iwin);
}

errr sdl3_win_term_main_screenshot(cptr name) {
	if (!term_main.win || !term_main.win->surface) return 1;

	/* Ensure latest content is displayed */
	SDL_UpdateWindowSurface(term_main.win->window);

	SDL_Surface *surface = term_main.win->surface;
	SDL_Surface *conv = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
	if (!conv) return 1;

#ifdef SDL3_IMAGE
	if (!IMG_SavePNG(conv, name)) {
		SDL_Log("IMG_SavePNG failed: %s", SDL_GetError());
#else
	if (!SDL_SaveBMP(conv, name)) {
		SDL_Log("SDL_SaveBMP failed: %s", SDL_GetError());
#endif
		SDL_DestroySurface(conv);
		return 1;
	}

	SDL_DestroySurface(conv);
	return 0;
}

/* PCF definitions and handling functions. */

#define PCF_HEADER              0x70636601  /* 'p' 'c' 'f' + 0x01 in LSBFirst */

#define PCF_PROPERTIES          (1<<0)
#define PCF_ACCELERATORS        (1<<1)
#define PCF_METRICS             (1<<2)
#define PCF_BITMAPS             (1<<3)
#define PCF_INK_METRICS         (1<<4)
#define PCF_BDF_ENCODINGS       (1<<5)
#define PCF_SWIDTHS             (1<<6)
#define PCF_GLYPH_NAMES         (1<<7)
#define PCF_BDF_ACCELERATORS    (1<<8)

#define PCF_DEFAULT_FORMAT      0x00000000
#define PCF_INKBOUNDS           0x00000200
#define PCF_ACCEL_W_INKBOUNDS   0x00000100
#define PCF_COMPRESSED_METRICS  0x00000100

#define PCF_GLYPH_PAD_MASK      (3<<0)            /* See the bitmap table for explanation */
#define PCF_BYTE_MASK           (1<<2)            /* If set then Most Sig Byte First */
#define PCF_BIT_MASK            (1<<3)            /* If set then Most Sig Bit First */
#define PCF_SCAN_UNIT_MASK      (3<<4)            /* See the bitmap table for explanation */

#define PCF_LSBFirst            0
#define PCF_MSBFirst            1
#define PCF_BYTE_ORDER(f)       (((f) & PCF_BYTE_MASK)?PCF_MSBFirst:PCF_LSBFirst)
#define PCF_GLYPHPADOPTIONS     4
#define PCF_GLYPH_PAD_INDEX(f)  ((f) & PCF_GLYPH_PAD_MASK)

#define PCF_FORMAT_MASK         0xFFFFFF00
#define PCF_FORMAT_MATCH(a,b)   (((a)&PCF_FORMAT_MASK) == ((b)&PCF_FORMAT_MASK))

typedef struct PCFTable PCFTable;
struct PCFTable {
	uint32_t type;
	uint32_t format;
	uint32_t size;
	uint32_t offset;
};

static uint32_t pcf_readU32LE(FILE *f) {
	uint8_t buf[4];
	if (fread(buf,1,4,f)!=4) return 0;
	return (uint32_t)(buf[0]) | ((uint32_t)(buf[1])<<8) | ((uint32_t)(buf[2])<<16) | ((uint32_t)(buf[3])<<24);
}

static uint16_t pcf_readU16LE(FILE *f) {
	uint8_t buf[2];
	if (fread(buf,1,2,f)!=2) return 0;
	return (uint16_t)(buf[0]) | ((uint16_t)(buf[1])<<8);
}

static uint8_t pcf_readU8(FILE *f) {
	uint8_t val;
	if (fread(&val,1,1,f)!=1) return 0;
	return val;
}

static uint32_t pcf_readU32BE(FILE *f) {
	uint8_t buf[4];
	if (fread(buf,1,4,f)!=4) return 0;
	return ((uint32_t)(buf[0])<<24) | ((uint32_t)(buf[1])<<16) |
		((uint32_t)(buf[2])<<8) |  (uint32_t)(buf[3]);
}

static uint16_t pcf_readU16BE(FILE *f) {
	uint8_t buf[2];
	if (fread(buf,1,2,f)!=2) return 0;
	return (uint16_t)((buf[0]<<8) | buf[1]);
}

static uint32_t pcf_readU32(FILE *f, uint32_t format) {
	if (PCF_BYTE_ORDER(format) == PCF_MSBFirst) return pcf_readU32BE(f); /* Big endian. */
	return pcf_readU32LE(f); /* Little endian. */
}

static uint16_t pcf_readU16(FILE *f, uint32_t format) {
	if (PCF_BYTE_ORDER(format) == PCF_MSBFirst) return pcf_readU16BE(f);
	return pcf_readU16LE(f);
}

static int16_t pcf_readS16(FILE *f, uint32_t format) {
	return (int16_t)pcf_readU16(f, format);
}

static uint32_t pcf_readLSB32(FILE *f) {
	return pcf_readU32LE(f);
}

/* Load a monospace bitmap PCF font. */
/* The returned PCF_Font needs to be freed using PCF_CloseFont() function after finished using. */
/* Ref: fontforge.org/docs/techref/pcf-format.html */
static PCF_Font* PCF_OpenFont(const char *name, float scale) {
	FILE *f;
	uint32_t header;

	PCFTable* tables;
	uint32_t ntables;

	PCFTable* metricsTab = NULL;
	uint32_t metricsFormat;
	bool metricsCompressed;
	int16_t charWidth, ascent, descent;

	PCFTable* encodingsTab = NULL;
	uint32_t encodingsFormat;
	int16_t min_char_or_byte2, max_char_or_byte2;
	int16_t min_byte1, max_byte1, default_char;
	int32_t nGlyphIndexes;

	PCFTable* bitmapTab = NULL;
	uint32_t bitmapFormat;
	uint32_t nbitmaps;
	uint32_t *glyphOffsets;
	uint32_t bitmapSizes[PCF_GLYPHPADOPTIONS];
	uint32_t bitmapSize;
	uint8_t *bitmapData;

	PCF_Font *font;
	uint32_t totalWidth;
	uint32_t totalHeight;
	bool bitmapLSBitFirst;
	uint32_t scanUnit;
	uint32_t rowPadding;
	uint32_t bytesPerLine;

	uint32_t i, g, x, y, bitIndex, b, color;
	void *row;
	int fontBitmapPitch;

	/* Open the file. */
	f = my_fopen(name, "rb");
	if(!f) {
		fprintf(stderr, "Error: Unable to open file %s.\n", name);
		return NULL;
	}

	/* Check PCF header. */
	header = pcf_readLSB32(f);
	if (header != PCF_HEADER) {
		fclose(f);
		fprintf(stderr, "Error: Not a valid PCF file (header mismatch).\n");
		return NULL;
	}

	/* PCF tables reading and checks. */

	/* Read number of tables stored in the file. */
	ntables = pcf_readLSB32(f);
	if (ntables == 0) {
		fprintf(stderr, "Error: Invalid PCF file (ntables=0).\n");
		fclose(f);
		return NULL;
	}

	/* Allocate memory for data on tables. */
	C_MAKE(tables, ntables, PCFTable);
	if (!tables) {
		fprintf(stderr, "Error: Memory allocation failed.\n");
		fclose(f);
		return NULL;
	}

	/* Read tables data until all wanted tables found. */
	for (i = 0; i < ntables; i++) {
		tables[i].type   = pcf_readLSB32(f);
		tables[i].format = pcf_readLSB32(f);
		tables[i].size   = pcf_readLSB32(f);
		tables[i].offset = pcf_readLSB32(f);

		if (metricsTab == NULL && tables[i].type == PCF_METRICS) {
			metricsTab = &tables[i];
		}
		if (encodingsTab == NULL && tables[i].type == PCF_BDF_ENCODINGS) {
			encodingsTab = &tables[i];
		}
		if (bitmapTab == NULL && tables[i].type == PCF_BITMAPS) {
			bitmapTab = &tables[i];
		}
		if (metricsTab && encodingsTab && bitmapTab) {
			break;
		}
	}

	/* Check if all necessary tables were found. */
	if (!metricsTab) {
		fprintf(stderr, "Error: PCF_METRICS table not found.\n");
	}
	if (!encodingsTab) {
		fprintf(stderr, "Error: PCF_BDF_ENCODINGS table not found.\n");
	}
	if(!bitmapTab) {
		fprintf(stderr, "Error: PCF_BITMAPS table not found.\n");
	}
	if (!metricsTab || !encodingsTab || !bitmapTab) {
		FREE(tables, PCFTable);
		fclose(f);
		return NULL;
	}

	/* Allocate memory for the font. */
	MAKE(font, PCF_Font);

	/* Read the font metrics and check if font is monospace. */

	/* Move the read head at the beginning of the metrics data. */
	if (fseek(f, metricsTab->offset, SEEK_SET) != 0) {
		fprintf(stderr, "Error: fseek to met table failed.\n");
		PCF_CloseFont(font);
		FREE(tables, PCFTable);
		fclose(f);
		return NULL;
	}

	/* Read metrics format in LSB order and determine compression for metrics data. */
	metricsFormat = pcf_readLSB32(f);
	metricsCompressed  = metricsFormat & PCF_COMPRESSED_METRICS;

	/* Get the number of glyphs in this font. */
	if (metricsCompressed) {
		font->nGlyphs = (uint32_t)pcf_readU16(f, metricsFormat);
	} else {
		font->nGlyphs = pcf_readU32(f, metricsFormat);
	}

	/* Check for no glyphs. */
	if (font->nGlyphs == 0) {
		fprintf(stderr, "Error: No glyphs found.\n");
		PCF_CloseFont(font);
		FREE(tables, PCFTable);
		fclose(f);
		return NULL;
	}

	/* For a monospace font, we take the first metric as the global dimension and check if the other glyphs are the same.. */
	for (i = 0; i < font->nGlyphs; i++ ) {
		if (metricsCompressed) {
			(void)pcf_readU8(f); /* leftBearing */
			(void)pcf_readU8(f); /* rightBearing */
			charWidth = (int16_t)(pcf_readU8(f) - 128); /* charWidth */
			ascent = (int16_t)(pcf_readU8(f) - 128); /* ascent */
			descent = (int16_t)(pcf_readU8(f) - 128); /* descent */
		} else {
			pcf_readS16(f, metricsFormat); /* leftBearing */
			pcf_readS16(f, metricsFormat); /* rightBearing */
			charWidth = pcf_readS16(f, metricsFormat); /* charWidth */
			ascent = pcf_readS16(f, metricsFormat); /* ascent */
			descent = pcf_readS16(f, metricsFormat); /* descent */
			pcf_readU16(f, metricsFormat); /* character_attributes */
		}

		if (i == 0) {
			font->glyphWidth = charWidth;
			font->glyphHeight = ascent + descent;
		} else {
			if (font->glyphWidth != charWidth || font->glyphHeight != (ascent + descent)) {
				fprintf(stderr, "Error: Font is not a monospace font.\n");
				PCF_CloseFont(font);
				FREE(tables, PCFTable);
				fclose(f);
				return NULL;
			}
		}
	}

	/* Check for monospace font dimensions. */
	if (font->glyphWidth == 0 || font->glyphHeight == 0) {
		fprintf(stderr, "Error: Invalid glyph metrics (width=%u, height=%u).\n", font->glyphWidth, font->glyphHeight);
		PCF_CloseFont(font);
		FREE(tables, PCFTable);
		fclose(f);
		return NULL;
	}

	/* Read and check the encodings data. */

	/* Move to encodings data position start in the file. */
	if (fseek(f, encodingsTab->offset, SEEK_SET) != 0) {
		PCF_CloseFont(font);
		FREE(tables, PCFTable);
		fclose(f);
		fprintf(stderr, "Error: fseek failed.\n");
		return NULL;
	}

	/* Read and check the encodings format. */
	encodingsFormat = pcf_readLSB32(f);
	if (!PCF_FORMAT_MATCH(encodingsFormat, PCF_DEFAULT_FORMAT)) {
		fprintf(stderr, "Error: Invalid encodings format (%x).\n", encodingsFormat);
		PCF_CloseFont(font);
		FREE(tables, PCFTable);
		fclose(f);
		return NULL;
	}

	/* Read encoding parameters. */
	min_char_or_byte2 = pcf_readS16(f, encodingsFormat);
	max_char_or_byte2 = pcf_readS16(f, encodingsFormat);
	min_byte1 = pcf_readS16(f, encodingsFormat);
	max_byte1 = pcf_readS16(f, encodingsFormat);
	default_char = pcf_readS16(f, encodingsFormat);

	if (min_byte1 != 0 || max_byte1 != 0) {
		/* For single byte encodings min_byte1 == max_byte1 == 0, and encoded values are between [min_char_or_byte2,max_char_or_byte2]. The glyph index corresponding to an encoding is glyphindex[encoding-min_char_or_byte2]. */
		fprintf(stderr, "Error: Only single byte encodings are supported.\n");
		PCF_CloseFont(font);
		FREE(tables, PCFTable);
		fclose(f);
		return NULL;
	}

	nGlyphIndexes = (max_char_or_byte2 - min_char_or_byte2 + 1) * (max_byte1 - min_byte1 + 1);

	/* Allocate memory for all the glyph indexes */
	C_MAKE(font->glyphIndexes, nGlyphIndexes, int16_t);
	if (!font->glyphIndexes) {
		fprintf(stderr, "Error: Memory allocation failed for glyphIndexes.\n");
		PCF_CloseFont(font);
		FREE(tables, PCFTable);
		fclose(f);
		return NULL;
	}

	/* Read the glyph indexes for all the encodings and use 'default_char' if there is no glyph index for encoding. */
	for (i = 0; i < nGlyphIndexes; i++) {
		font->glyphIndexes[i] = pcf_readS16(f, encodingsFormat);
		if (font->glyphIndexes[i] == (int16_t)0xFFFF) font->glyphIndexes[i] = default_char;
	}

	/* Read all bitmap storage and pixel data and do all validity checks. */

	/* Move to the bitmap data start position in the file. */
	if (fseek(f, bitmapTab->offset, SEEK_SET) != 0) {
		fprintf(stderr, "Error: fseek failed.\n");
		PCF_CloseFont(font);
		FREE(tables, PCFTable);
		fclose(f);
		return NULL;
	}

	/* Read and check the bitmap format. */
	bitmapFormat = pcf_readLSB32(f);
	if (!PCF_FORMAT_MATCH(bitmapFormat, PCF_DEFAULT_FORMAT)) {
		fprintf(stderr, "Error: Invalid bitmap format (%x).\n", bitmapFormat);
		PCF_CloseFont(font);
		FREE(tables, PCFTable);
		fclose(f);
		return NULL;
	}

	/* Read the number of bitmaps stored in the data. */
	nbitmaps = pcf_readU32(f, bitmapFormat);

	/* Check if number of stored bitmaps corresponds with the number of glyphs. */
	if (nbitmaps != font->nGlyphs) {
		fprintf(stderr, "Error: Number of bitmaps (%u) does not equal number of glyphs (%u).\n", nbitmaps, font->nGlyphs);
		PCF_CloseFont(font);
		FREE(tables, PCFTable);
		fclose(f);
		return NULL;
	}

	/* Allocate array for glyph offsets in the bitmap data. */
	C_MAKE(glyphOffsets, nbitmaps, uint32_t);
	if (!glyphOffsets) {
		fprintf(stderr, "Error: Memory allocation failed for glyphOffsets.\n");
		PCF_CloseFont(font);
		FREE(tables, PCFTable);
		fclose(f);
		return NULL;
	}

	/* Read all the glyph offsets for bitmap into a array. */
	for (i = 0; i < nbitmaps; i++) {
		glyphOffsets[i] = pcf_readU32(f, bitmapFormat);
	}

	/* Read all the bitmap sizes. */
	for (i = 0; i < PCF_GLYPHPADOPTIONS; i++) {
		bitmapSizes[i] = pcf_readU32(f, bitmapFormat);
	}

	/* Determine the size of the bitmap pixel data. */
	bitmapSize = bitmapSizes[PCF_GLYPH_PAD_INDEX(bitmapFormat)];

	/* Allocate memory for bitmap pixel data. */
	C_MAKE(bitmapData, bitmapSize, uint8_t); /* This is only possible because sizeof(uint8_t) is 1. */
	if (!bitmapData) {
		fprintf(stderr, "Error: Memory allocation failed for bitmap data.\n");
		C_FREE(glyphOffsets, nbitmaps, uint32_t);
		PCF_CloseFont(font);
		FREE(tables, PCFTable);
		fclose(f);
		return NULL;
	}

	/* Read the bitmap pixel data. */
	if (fread(bitmapData, 1, bitmapSize, f) != bitmapSize) {
		fprintf(stderr, "Error: Failed to read bitmap data.\n");
		C_FREE(bitmapData, bitmapSize, uint8_t);
		C_FREE(glyphOffsets, nbitmaps, uint32_t);
		PCF_CloseFont(font);
		FREE(tables, PCFTable);
		fclose(f);
		return NULL;
	}

	/* There is no need to read from file. */
	fclose(f);

	/* Write all the bitmap pixel data into an SDL surface. All the glyphs in one line. */

	/* Compute total width and height of the surface. */
	totalWidth = font->glyphWidth * font->nGlyphs;
	totalHeight = font->glyphHeight;

	/* Create the surface with desired dimensions. */
	font->bitmap = SDL_CreateSurface(totalWidth, totalHeight, SDL_PIXELFORMAT_RGBA32);
	if(!font->bitmap) {
		fprintf(stderr, "Error: SDL_CreateSurface failed: %s\n", SDL_GetError());
		C_FREE(bitmapData, bitmapSize, uint8_t);
		C_FREE(glyphOffsets, nbitmaps, uint32_t);
		PCF_CloseFont(font);
		FREE(tables, PCFTable);
		return NULL;
	}

	/* Compute all the parameters needed to convert bitmap pixel data into the surface. */
	bitmapLSBitFirst = (bitmapFormat & PCF_BIT_MASK);
	scanUnit = 1 << ((bitmapFormat & PCF_SCAN_UNIT_MASK) >> 4);
	if (scanUnit != 1 && scanUnit != 2 && scanUnit != 4) {
		fprintf(stderr, "Error: Invalid scanUnit %d. Only 1, 2 and 4 are allowed.\n", scanUnit);
		C_FREE(bitmapData, bitmapSize, uint8_t);
		C_FREE(glyphOffsets, nbitmaps, uint32_t);
		PCF_CloseFont(font);
		FREE(tables, PCFTable);
		return NULL;
	}
	rowPadding = 1 << (bitmapFormat & PCF_GLYPH_PAD_MASK);
	bytesPerLine = (font->glyphWidth + 7) / 8;
	if ((bytesPerLine % rowPadding) != 0) {
		bytesPerLine = bytesPerLine + (rowPadding - (bytesPerLine % rowPadding));
	}

	/* Render each glyph into the surface. */
	fontBitmapPitch = font->bitmap->pitch / surface_bytes_per_pixel(font->bitmap);
	for (g = 0; g < font->nGlyphs; g++) {
		for (y = 0; y < font->glyphHeight; y++) {
			/* Compute the starting position of row "y" in glyph "g" in the bitmap pixel data. */
			row = bitmapData + glyphOffsets[g] + y * bytesPerLine;
			for (x = 0; x < font->glyphWidth; x++) {
				/* Get portion of data, where the pixel on position [x, y] for glyph "g" is encoded. */
				switch (scanUnit) {
					case 1: b = ((uint8_t *)row)[x / 8]; break;
					case 2: b = ((uint16_t *)row)[x / 16]; break;
					case 4: b = ((uint32_t *)row)[x / 32]; break;
				}

				/* Compute the bit index based on bit order for the pixel. */
				bitIndex = x % 8;
				if (bitmapLSBitFirst) bitIndex = 7 - bitIndex;

				/* Determine the pixel color. */
				color = (b & (1 << bitIndex)) ? 0xFFFFFFFF : 0x00000000;

				/* Write the pixel into the SDL surface. */
				((uint32_t*)font->bitmap->pixels)[y * fontBitmapPitch + g * font->glyphWidth + x] = color;
			}
		}
	}

	/* Free all the unneeded variables previously allocated. */
	C_FREE(bitmapData, bitmapSize, uint8_t);
	C_FREE(glyphOffsets, nbitmaps, uint32_t);
	FREE(tables, PCFTable);

	/* Keep the bitmap font crisp while scaling it to the display's expected content size. */
	if (scale != 1.0f) {
		int scaled_glyph_width = SDL_lroundf(font->glyphWidth * scale);
		int scaled_glyph_height = SDL_lroundf(font->glyphHeight * scale);
		SDL_Surface *scaled_bitmap;

		if (scaled_glyph_width < 1) scaled_glyph_width = 1;
		if (scaled_glyph_height < 1) scaled_glyph_height = 1;
		//TODO Scale using gfx_resize_type (and cache) as in tiles?
		scaled_bitmap = SDL_ScaleSurface(font->bitmap, scaled_glyph_width * font->nGlyphs, scaled_glyph_height, SDL_SCALEMODE_NEAREST);
		if (!scaled_bitmap) {
			fprintf(stderr, "Error: Failed to scale PCF font for display: %s\n", SDL_GetError());
			PCF_CloseFont(font);
			return NULL;
		}

		SDL_DestroySurface(font->bitmap);
		font->bitmap = scaled_bitmap;
		font->glyphWidth = scaled_glyph_width;
		font->glyphHeight = scaled_glyph_height;
	}

	/* The bitmap contains letters in solid white on black transparent background. Set blend mode and color key now for PCF_RenderText(). */
	SDL_SetSurfaceBlendMode(font->bitmap, SDL_BLENDMODE_NONE);
	SDL_SetSurfaceColorKey(font->bitmap, true, SDL_MapRGBA(SDL_GetPixelFormatDetails(font->bitmap->format), NULL, 255, 255, 255, 0));

	return font;
}

/* Free all the font resources from memory. */
static void PCF_CloseFont(PCF_Font *font) {
	if (font == NULL) return;
	if (font->glyphIndexes != NULL) {
		free(font->glyphIndexes);
		font->glyphIndexes = NULL;
	}
	if (font->bitmap !=NULL) {
		SDL_DestroySurface(font->bitmap);
		font->bitmap = NULL;
	}
	font->nGlyphs = 0;
	font->glyphWidth = 0;
	font->glyphHeight = 0;
	FREE(font, PCF_Font);
}

/* Returns a 32b RGBA surface, with text in 'str' rendered in one line using 'fg_color' as text color and 'bg_color' as background color. */
/* NULL will be returned, if some error occurs. */
SDL_Surface* PCF_RenderText(struct PCF_Font* font, const char* str, Pixel fg_color, Pixel bg_color) {
	SDL_Surface *surface, *preparation;
	SDL_Rect src_rect, dest_rect;
	int text_length, width, height;
	int i, glyph_index;

	if (!font || !str || !font->bitmap || !font->glyphIndexes) {
		return NULL;
	}

	/* Prepare needed variables. */
	text_length = strlen(str);
	width = text_length * font->glyphWidth;
	height = font->glyphHeight;

	/* Create surface which will be returned. */
	surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA32);
	if (!surface) {
		fprintf(stderr, "PCF_RenderText: Error creating resulting surface: %s\n", SDL_GetError());
		return NULL;
	}
	/* Fill the returning surface with background color. */
	SDL_FillSurfaceRect(surface, NULL, SDL_MapRGBA(SDL_GetPixelFormatDetails(surface->format), NULL, bg_color.r, bg_color.g, bg_color.b, bg_color.a));

	/* If background and foreground colors are the same, there is no need for font rendering. */
	if (fg_color.r == bg_color.r && fg_color.g == bg_color.g && fg_color.b == bg_color.b) {
			return surface;
	}

	/* Create a surface for font preparation. */
	preparation = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA32);
	if (!preparation) {
		fprintf(stderr, "PCF_RenderText: Error creating preparation surface: %s\n", SDL_GetError());
		SDL_DestroySurface(surface);
		return NULL;
	}
	/* Fill the preparation surface with foreground color. */
	SDL_FillSurfaceRect(preparation, NULL, SDL_MapRGBA(SDL_GetPixelFormatDetails(preparation->format), NULL, fg_color.r, fg_color.g, fg_color.b, fg_color.a));

	/* Render font into the returning surface. */
	src_rect = (SDL_Rect){0, 0, font->glyphWidth, font->glyphHeight};
	dest_rect = (SDL_Rect){0, 0, font->glyphWidth, font->glyphHeight};
	for (i = 0; i < text_length; ++i) {
		/* Get index for the i-th glyph. */
		glyph_index = font->glyphIndexes[(uint8_t)str[i]];
		if (glyph_index >= 0) {
			/* Adjust the x component of source rectangle. */
			src_rect.x = glyph_index * font->glyphWidth;

			/* Render the glyph into the preparation surface. */
			/* The 'font->bitmap' has already SDL_BLENDMODE_NONE set with color key transparency at the end of PCF_OpenFont function. This will result in the glyph painted in 'fg_color' with transparent background. */
			SDL_BlitSurface(font->bitmap, &src_rect, preparation, &dest_rect);
		}
		/* Adjust the x component of the destination rectangle. */
		dest_rect.x += font->glyphWidth;
	}

	/* Blend the preparation onto resulting surface. */
	SDL_BlitSurface(preparation, NULL, surface, NULL);

	SDL_DestroySurface(preparation);
	return surface;
}
#endif /* USE_SDL3 */
