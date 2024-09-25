#ifdef USE_SDL2
/* main-sdl2.c
 *
 * SDL2 bindings implementation for the TomeNET game project.
 * Handles window creation, event handling, keyboard input, and graphics/text output using SDL2.
 *
 * Note: Requires SDL2, SDL2_ttf and SDL2_net libraries.
 * Screenshot saving uses SDL2_image for PNG when available; otherwise BMP is used.
 */

#include "angband.h"
#include "graphics_common.h"

#include "../common/z-util.h"
#include "../common/z-virt.h"
#include "../common/z-form.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_net.h>
#ifdef SDL2_IMAGE
 #include <SDL2/SDL_image.h>
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


	s16b			wid;
	s16b			hgt;

	uint			emulateMonospace:1;
	uint			nuke:1;
};

/**** Available Macros ****/

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
 * Init an infowin by giving some data.
 *
 * Inputs:
 *	x,y: The position of this Window
 *	w,h: The size of this Window
 *	b:   The border width
 *	b_color, bg_color: Border and background colors
 *
 */
static errr Infowin_init(int x, int y, int w, int h, unsigned int b, Pixel b_color, Pixel bg_color) {
	/* Wipe it clean */
	WIPE(Infowin, infowin);

	/*** Create the SDL_Window* 'window' from data ***/

	/* Create the Window. */
	Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
	if (!sdl2_window_decorations) flags |= SDL_WINDOW_BORDERLESS;
	SDL_Window *window = SDL_CreateWindow("", x, y, w, h, flags);

	if (window == NULL) {
		fprintf(stderr, "Error creating window in Infowin_init\n");
		return(1);
	}

	SDL_Surface *surface = SDL_GetWindowSurface(window);
	SDL_FillRect(surface, &(SDL_Rect){0, 0, w, h}, SDL_MapRGBA(surface->format, Pixel_quadruplet(b_color)));
	SDL_FillRect(surface, &(SDL_Rect){b, b, w-(2*b), h-(2*b)}, SDL_MapRGBA(surface->format, Pixel_quadruplet(bg_color)));
	SDL_UpdateWindowSurface(window);

	/* Assign stuff */
	Infowin->windowID = SDL_GetWindowID(window);
	Infowin->window = window;
	Infowin->surface = surface;

	/* Apply the above info */
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

	uint32_t windowFlags = SDL_GetWindowFlags(window);
	Infowin->shown = windowFlags & SDL_WINDOW_SHOWN;

	/* Mark it as nukable. */
	Infowin->nuke = 1;

	return(0);
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
static errr Infofnt_init_ttf(cptr name) {
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
	if (!my_fexists(buf) && !sdl2_paths_same(ANGBAND_USER_DIR_XTRA_FONT, ANGBAND_DIR_XTRA_FONT)) {
		path_build(buf, 1024, ANGBAND_DIR_XTRA_FONT, font_name);
	}

	/* Attempt to load the font with the given size */
	font = TTF_OpenFont(buf, font_size);

	/* The load failed, try to recover */
	if (!font) {
		fprintf(stderr, "Loading font \"%s\" error: %s\n", buf, TTF_GetError());
		return(-1);
	}


	/*** Init the font ***/

	/* Wipe the thing */
	WIPE(Infofnt, infofnt);

	/* Assign the struct */
	Infofnt->type = FONT_TYPE_TTF;
	Infofnt->font = font;

	/* Extract default sizing info */
	Infofnt->hgt = TTF_FontHeight(font);
	/* Assume monospaced font */
	if (TTF_FontFaceIsFixedWidth(font) == 1) {
		Infofnt->emulateMonospace = false;
		/* Check all printable basic ASCII characters (32 to 126) for consistent width and height */
		char str[3] = "  ";
		SDL_Surface *surface;
		for (int ch = 32; ch <= 126; ++ch) {
			str[0] = (char)ch;
			surface = TTF_RenderText_Solid(font, str, (Pixel){255, 255, 255});
			if (surface == NULL) {
				fprintf(stderr, "Failed to render character %d!\n", ch);
				/* Free the font */
				TTF_CloseFont(font);
				/* Fail */
				return(-1);
			}

			int str_w = surface->w;
			SDL_FreeSurface(surface);

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
 * - A SDL2 surface with all the glyphs. The glyphs are stored in one row.
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
static PCF_Font* PCF_OpenFont(const char *name);
SDL_Surface* PCF_RenderText(PCF_Font* font, const char* str, Pixel bg_color, Pixel fg_color);

/*
 * Init an infofnt by its Name. Assumes font is a monospaced .pcf bitmap font.
 *
 * Inputs:
 *	name: The name of the requested Font without extension.
 */
static errr Infofnt_init_pcf(cptr name) {
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
	if (!my_fexists(buf) && !sdl2_paths_same(ANGBAND_USER_DIR_XTRA_FONT, ANGBAND_DIR_XTRA_FONT)) {
		path_build(buf, 1024, ANGBAND_DIR_XTRA_FONT, font_name);
	}

	/* Attempt to load the font with the given size. */
	font = PCF_OpenFont(buf);

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
static errr Infofnt_init(cptr name) {
	/* TrueType fonts are assumed unless the file name denotes a PCF font. */
	if (is_pcf_font(name)) {
		return Infofnt_init_pcf(name);
	}
	return Infofnt_init_ttf(name);

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
 * Request to focus Infowin.
 */
static errr Infowin_set_focus(void) {
	/* No input focus for window in SDL2. Try raise window. */
	SDL_RaiseWindow(Infowin->window);

	/* Success */
	return(0);
}

#if 0
/*
 * Move an infowin.
 */
static errr Infowin_move(int x, int y) {
	/* Execute the request */
	Infowin->x = x;
	Infowin->y = y;
	SDL_SetWindowPosition(Infowin->window, x, y);

	/* Success */
	return(0);
}
#endif

/*
 * Resize an infowin
 */
static errr Infowin_resize(int w, int h) {
	/* Execute the request */
	Infowin->wb = w;
	Infowin->hb = h;
	SDL_SetWindowSize(Infowin->window, w, h);

	/* Success */
	return(0);
}

/*
 * Visually clear Infowin
 */
static errr Infowin_wipe(void) {
	/* Draw borders. */

	uint8_t r = Infowin->b_color.r;
	uint8_t g = Infowin->b_color.g;
	uint8_t b = Infowin->b_color.b;
	uint8_t a = Infowin->b_color.a;
	uint32_t bc = SDL_MapRGBA(Infowin->surface->format, r, g, b, a);
	/* Draw top border. */
	SDL_FillRect(Infowin->surface, &(SDL_Rect){0, 0, Infowin->wb, Infowin->bh}, bc);
	/* Draw bottom border. */
	SDL_FillRect(Infowin->surface, &(SDL_Rect){0, Infowin->bh + Infowin->hd, Infowin->wb, (Infowin->hb - Infowin->hd - Infowin->bh)}, bc);
	/* Draw left border. */
	SDL_FillRect(Infowin->surface, &(SDL_Rect){0, Infowin->bh + 1, Infowin->bw, Infowin->hd}, bc);
	/* Draw right border. */
	SDL_FillRect(Infowin->surface, &(SDL_Rect){Infowin->bw + Infowin->wd, Infowin->bh + 1, Infowin->wb - Infowin->wd - Infowin->bw, Infowin->hd}, bc);

	/* Wipe drawing field. */
	SDL_FillRect(Infowin->surface, &(SDL_Rect){Infowin->bw, Infowin->bh, Infowin->wd, Infowin->hd}, SDL_MapRGBA(Infowin->surface->format, Pixel_quadruplet(Infowin->bg_color)));

	/* Success */
	return(0);
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
		surface = TTF_RenderText_LCD((TTF_Font*)Infofnt->font, str, fgColor, bgColor);
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

	SDL_FreeSurface(surface);

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
	SDL_FillRect(Infowin->surface, &(SDL_Rect){x, y, w, h}, SDL_MapRGBA(Infowin->surface->format, Pixel_quadruplet(Infoclr->fg)));

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
void terminal_window_real_coords_sdl2(int term_idx, int *ret_x, int *ret_y);
void resize_main_window_sdl2(int cols, int rows);
void resize_window_sdl2(int term_idx, int cols, int rows);
static term_data* term_idx_to_term_data(int term_idx);

#if defined(USE_GRAPHICS) && defined(TILE_CACHE_SIZE)
bool disable_tile_cache = FALSE;

/*
 * The structure holds a single entry for tile cache.
 *
 * - Array of colors used to build the tile. Size of the array is `ncolors`.
 * - A SDL2 surface holding the cached tile.
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

#define SDL2_DEFAULT_BORDER_WIDTH 1
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

static term_data *sdl2_terms_term_data[ANGBAND_TERM_MAX] = {
	&term_main, &term_1, &term_2, &term_3, &term_4,
	&term_5, &term_6, &term_7, &term_8, &term_9
};
static char *sdl2_terms_font_env[ANGBAND_TERM_MAX] = {
	"TOMENET_SDL2_FONT_TERM_MAIN", "TOMENET_SDL2_FONT_TERM_1", "TOMENET_SDL2_FONT_TERM_2", "TOMENET_SDL2_FONT_TERM_3", "TOMENET_SDL2_FONT_TERM_4",
	"TOMENET_SDL2_FONT_TERM_5", "TOMENET_SDL2_FONT_TERM_6", "TOMENET_SDL2_FONT_TERM_7", "TOMENET_SDL2_FONT_TERM_8", "TOMENET_SDL2_FONT_TERM_9"
};
static char *sdl2_terms_font_default[ANGBAND_TERM_MAX] = {
	SDL2_DEFAULT_FONT_TERM_MAIN, SDL2_DEFAULT_FONT_TERM_1, SDL2_DEFAULT_FONT_TERM_2, SDL2_DEFAULT_FONT_TERM_3, SDL2_DEFAULT_FONT_TERM_4,
	SDL2_DEFAULT_FONT_TERM_5, SDL2_DEFAULT_FONT_TERM_6, SDL2_DEFAULT_FONT_TERM_7, SDL2_DEFAULT_FONT_TERM_8, SDL2_DEFAULT_FONT_TERM_9
};
static int sdl2_terms_ttf_size_default[ANGBAND_TERM_MAX] = {
	SDL2_DEFAULT_TTF_FONT_SIZE_TERM_MAIN, SDL2_DEFAULT_TTF_FONT_SIZE_TERM_1, SDL2_DEFAULT_TTF_FONT_SIZE_TERM_2, SDL2_DEFAULT_TTF_FONT_SIZE_TERM_3, SDL2_DEFAULT_TTF_FONT_SIZE_TERM_4,
	SDL2_DEFAULT_TTF_FONT_SIZE_TERM_5, SDL2_DEFAULT_TTF_FONT_SIZE_TERM_6, SDL2_DEFAULT_TTF_FONT_SIZE_TERM_7, SDL2_DEFAULT_TTF_FONT_SIZE_TERM_8, SDL2_DEFAULT_TTF_FONT_SIZE_TERM_9
};
const char *sdl2_terms_wid_env[ANGBAND_TERM_MAX] = {
	"TOMENET_SDL2_WID_TERM_MAIN", "TOMENET_SDL2_WID_TERM_1", "TOMENET_SDL2_WID_TERM_2", "TOMENET_SDL2_WID_TERM_3", "TOMENET_SDL2_WID_TERM_4",
	"TOMENET_SDL2_WID_TERM_5", "TOMENET_SDL2_WID_TERM_6", "TOMENET_SDL2_WID_TERM_7", "TOMENET_SDL2_WID_TERM_8", "TOMENET_SDL2_WID_TERM_9"
};
const char *sdl2_terms_hgt_env[ANGBAND_TERM_MAX] = {
	"TOMENET_SDL2_HGT_TERM_MAIN", "TOMENET_SDL2_HGT_TERM_1", "TOMENET_SDL2_HGT_TERM_2", "TOMENET_SDL2_HGT_TERM_3", "TOMENET_SDL2_HGT_TERM_4",
	"TOMENET_SDL2_HGT_TERM_5", "TOMENET_SDL2_HGT_TERM_6", "TOMENET_SDL2_HGT_TERM_7", "TOMENET_SDL2_HGT_TERM_8", "TOMENET_SDL2_HGT_TERM_9"
};

/*
 * Keycode macros, used on Keycode to test for classes of symbols.
 */
#define IsModifierKey(keycode) \
	((keycode == SDLK_LSHIFT) || (keycode == SDLK_RSHIFT) || \
	 (keycode == SDLK_LCTRL)  || (keycode == SDLK_RCTRL)  || \
	 (keycode == SDLK_LALT)   || (keycode == SDLK_RALT)   || \
	 (keycode == SDLK_LGUI)   || (keycode == SDLK_RGUI)   || \
	 (keycode == SDLK_NUMLOCKCLEAR) || (keycode == SDLK_CAPSLOCK) || \
	 (keycode == SDLK_MODE))

/*
 * Checks if the keysym is a special key or a normal key.
 * Special keys in SDL2 start from SDLK_WORLD_0 keycode.
 */
#define IsSpecialKey(keycode) \
	(((keycode) >= SDLK_F1 && (keycode) <= SDLK_F12) || \
	 (keycode) == SDLK_ESCAPE || (keycode) == SDLK_RETURN || \
	 (keycode) == SDLK_TAB || (keycode) == SDLK_BACKSPACE || \
	 (keycode) == SDLK_DELETE || (keycode) == SDLK_HOME || \
	 (keycode) == SDLK_END || (keycode) == SDLK_PAGEUP || \
	 (keycode) == SDLK_PAGEDOWN || (keycode) == SDLK_INSERT || \
	 (keycode) == SDLK_PRINTSCREEN || (keycode) == SDLK_PAUSE)

#ifdef SDL2_STICKY_KEYS
bool ctrl_forced = false;
bool shift_forced = false;
bool alt_forced = false;
#endif
static int Term_win_update(int flush, int sync, int discard);

/* Used for keypress communication from react_keypress() to react_textinput(). */
typedef struct {
	bool pending;
	SDL_Keycode keycode;
	bool mc, ms, ma, mn;
} sdl2_pending_keypress;

static sdl2_pending_keypress pending_keypress;
static bool suppress_next_textinput = false;

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
		case SDLK_DELETE: return('\177'); /* DEL (127, 0x7F) */
		case SDLK_BACKSPACE: return('\010');
		default: return(-1);
	}
}

static bool is_special_keysym(SDL_Keycode keycode) {
	switch (keycode) {
		case SDLK_ESCAPE:
		case SDLK_RETURN:
		case SDLK_TAB:
		case SDLK_DELETE:
		case SDLK_BACKSPACE:
			return true;
	}
	return false;
}

static bool is_printable_ascii(uint8_t ch) {
	return(ch >= 32 && ch < 127);
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
#endif

static void react_textinput(SDL_Event *event) {
	sdl2_pending_keypress *pending = &pending_keypress;
	char *text = event->text.text;
	char msg[128];
	int text_len;
	bool not_printable;

	/* When there is a pending modification, handle it and nothing else. */
	if (pending->pending) {
#ifndef ALLOW_NAVI_KEYS_IN_PROMPT
		/* Just in case someone decides to compile sdl2 client without navigational key support. */
		if (strlen(text) == 1) {
			Term_keypress((pending->mc && (text[0] >= 'a' && text[0] <= 'z')) ? text[0] & 0x1F : text[0]);
		} else {
			for (int i = 0; text[i]; i++) Term_keypress(text[i]);
		}
#else
		text_len = strlen(text);
		if (text_len > 1 || text_len == 0) {
			/* The character after keypress is decoded to zero or more than one byte (unicode?). */
			navikey_symbol(msg, 128, pending->mc, pending->ms, pending->ma, (uint32_t)pending->keycode);
			for (int i = 0; msg[i]; i++) Term_keypress(msg[i]);
			Term_keypress(28);
			for (int i = 0; text[i]; i++) Term_keypress(text[i]);
			Term_keypress(28);
			return;
		}

		/* Calculate the control character if there is pendin control modifier and character is a small letter. */
		if (pending->mc && (text[0] >= 'a' && text[0] <= 'z')) {
			text[0] = text[0] & 0x1F;
		}

		not_printable = !is_printable_ascii((uint8_t)text[0]);
		/* Hack for shift+space. */
		if (pending->keycode == 32 && pending->ms) {
			not_printable = true;
		}

		if (pending->mc || pending->ma || not_printable) {
			navikey_symbol(msg, 128, pending->mc, pending->ms, pending->ma, (uint32_t)text[0]);
			for (int i = 0; msg[i]; i++) Term_keypress(msg[i]);
			Term_keypress(28);
			Term_keypress(text[0]);
			Term_keypress(28);
			return;
		}

		Term_keypress(text[0]);
#endif
		pending->pending = false;
		return;
	}

	if (suppress_next_textinput) {
		/* Note: The 'suppress_next_textinput' is set to false right after return from this function.*/
		return;
	}

	/* Just press all the character in textinput text. */
	for (int i = 0; text[i]; i++) Term_keypress(text[i]);
}

/*
 * Process a keypress event
 */
static void react_keypress(SDL_Event *event) {
	SDL_Scancode scancode = event->key.keysym.scancode;
	SDL_Keycode keycode = event->key.keysym.sym;
	SDL_Keymod mod;
	SDL_Event next;
	int sym;
	char msg[128] = "";
	bool mc, ms, ma, mn, force_not_special;

	if (scancode == SDL_SCANCODE_UNKNOWN) {
		fprintf(stderr, "Key with unknown scancode pressed.\n");
		return;
	}

	/* Ignore "modifier keys" */
	if (IsModifierKey(keycode)) {
#ifdef SDL2_STICKY_KEYS
		if (keycode == SDLK_LCTRL || keycode == SDLK_RCTRL) { ctrl_forced = true; }
		if (keycode == SDLK_LSHIFT || keycode == SDLK_RSHIFT) { shift_forced = true; }
		if (keycode == SDLK_LALT || keycode == SDLK_RALT) { alt_forced = true; }
#endif
		return;
	}

	/* Extract four modifier flags. */
	/* Note: AltGr is handled as ordinary Alt key. */
	/* Note: Handle num-lock as always off. */
	mod = SDL_GetModState();
	ms = (mod & KMOD_SHIFT) ? true : false;
	mc = (mod & KMOD_CTRL) ? true : false;
	ma = (mod & KMOD_ALT) ? true : false;
	mn = false; /* (mod & KMOD_NUM) ? true : false; */

#ifdef SDL2_STICKY_KEYS
	if (ctrl_forced) {
		mc = true;
		ctrl_forced = false;
	}
	if (shift_forced) {
		ms = true;
		shift_forced = false;
	}
	if (alt_forced) {
		mc = true;
		alt_forced = false;
	}
#endif

#ifdef ENABLE_SHIFT_SPECIALKEYS
	/* As we're shortcutting the whole key-evaluation with the various 'return' calls here, set the shiftkey flags manually */
	if (ms) inkey_shift_special |= 0x1;
	if (mc) inkey_shift_special |= 0x2;
	if (ma) inkey_shift_special |= 0x4;
#endif

	/* Printable text is normally handled by SDL_TEXTINPUT, but SDL2 does not emit usable text events for most Ctrl combinations with non-letter printable keys. */
	/* Peek if there is a SDL_TEXTINPUT event in queue and if there is one, let it handle the input event. */
	/* Note: The window_id does not matter key presses from all windows are handled the same. */
	SDL_PumpEvents();
	if (SDL_PeepEvents(&next, 1, SDL_PEEKEVENT, SDL_TEXTINPUT, SDL_TEXTINPUT) > 0) {
		pending_keypress = (sdl2_pending_keypress){true, keycode, mc, ms, ma, mn};
		return;
	}

	suppress_next_textinput = true;

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
			Term_keypress(sym);
			if (mc || ms || ma) {
				Term_keypress(28);
			}
			return;
		}

		navikey_symbol(msg, 128, mc, ms, ma, (uint32_t)sym);
		for (int i = 0; msg[i]; i++) Term_keypress(msg[i]);
#endif
		return;
	}

	/* All Ctrl & Alt combinations should be handled here. */
	/* Although in SDL2 linux client all Alt combinations without Ctrl and with printable keys are handled in text input, in mingw clients running in WINE many Alt combinations are not handled in textinput. Handling them here is OK, cause if it is handled by textinput, the peek into queue in code above should catch them. */
	/* Note: Found out, that the textinput handling of Alt combination is true for running the client from fedora 41 distrobox. Running from manjaro does not produce textinput event. */
	if (mc || ma) {
		if (!ma && !ms && keycode >= SDLK_a && keycode <= SDLK_z) {
			Term_keypress(keycode & 0x1F);
		} else {
#ifndef ALLOW_NAVI_KEYS_IN_PROMPT
			/* No navigational keys support. */
			Term_keypress((mc && !ma && keycode >= SDLK_a && keycode <= SDLK_z) ? keycode & 0x1F : keycode);
#else
			/* Hack: For more compatibility with x11/windows clients, at least convert lowercase letters to uppercase if shift is pressed. */
			if (ms && !mc && keycode >= 'a' && keycode <= 'z') {
				keycode = keycode - 'a' + 'A';
			}

			navikey_symbol(msg, 128, mc, ms, ma, (uint32_t)keycode);
			for (int i = 0; msg[i]; i++) Term_keypress(msg[i]);

			if (mc && ((keycode >= 'a' && keycode <= 'z') || (keycode >= 'A' && keycode <= 'Z'))) {
				keycode = keycode & 0x1F;
			}

			Term_keypress(28);
			Term_keypress(keycode);
			Term_keypress(28);
#endif
		}
		return;
	}

	/* All other keys (keys without Ctrl or Alt combinations) should be for sure handled in textinput event. */
	suppress_next_textinput = false;
	return;
}

/*
 * Handles all terminal windows resize timers.
 */
static void resize_window_sdl2_timers_handle(void) {
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

				/* Calculate new columns/rows based on window border and font dimensions. */
				resize_window_sdl2(t_idx, (td->win->wb - (2 * td->win->bw)) / td->fnt->wid, (td->win->hb - (2 * td->win->bh)) / td->fnt->hgt);

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
	resize_window_sdl2_timers_handle();

	SDL_Event event;

	if (!wait) {
		if (!SDL_PollEvent(&event)) return (1);
	} else {
		if (!SDL_WaitEvent(&event)) return(1);
	}

	if (event.type == SDL_QUIT) {
		return(1); /* Signal to exit */
	}

	if (event.type == SDL_KEYDOWN) window_id = event.key.windowID;
	else if (event.type == SDL_TEXTINPUT) window_id = event.text.windowID;
	else if (event.type == SDL_WINDOWEVENT) window_id = event.window.windowID;

	/* Determine which window was the event for. */
	for (int i = 0; i < ANGBAND_TERM_MAX; i++) {
		if (sdl2_terms_term_data[i]->win && sdl2_terms_term_data[i]->win->window && sdl2_terms_term_data[i]->win->windowID == window_id) {
			td = sdl2_terms_term_data[i];
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
		case SDL_KEYDOWN:

			/* Hack -- use "old" term */
			Term_activate(&old_td->t);

			/* Process the key */
			suppress_next_textinput = false;
			react_keypress(&event);

			break;

		case SDL_TEXTINPUT:

			/* Hack -- use "old" term */
			Term_activate(&old_td->t);

			/* Process printable text */
			react_textinput(&event);
			suppress_next_textinput = false;

			break;

		case SDL_WINDOWEVENT:
			if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
				/* Handle window resize. */
				int new_wid = event.window.data1;
				int new_hgt = event.window.data2;

				/* Check if window is resized. */
				bool resized = Infowin->wb != new_wid || Infowin->hb != new_hgt;

				Infowin->wb = new_wid;
				Infowin->hb = new_hgt;

				if (resized) {
					/* Window resize timer start. */
					td->resize_timer = SDL_GetTicks() + 500; /* Add 1/2 second (500 ms). */
				}
				break;
			}

			if (event.window.event == SDL_WINDOWEVENT_MOVED) {
				Infowin->x = event.window.data1;
				Infowin->y = event.window.data2;

				int top, left, bottom, right;
				SDL_GetWindowBordersSize(Infowin->window, &top, &left, &bottom, &right);

				Infowin->x -= left;
				Infowin->y -= top;
				(void)bottom; (void)right;
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
			if (td->tiles_layers[i] != NULL) SDL_FreeSurface(td->tiles_layers[i]);
		}

		C_FREE(td->tiles_layers, layers, SDL_Surface*);
	}
	td->tiles_layers = NULL;
	td->nlayers = 0;
	if (td->tilePreparation) {
		SDL_FreeSurface(td->tilePreparation);
		td->tilePreparation = NULL;
	}
	if (td->tiles_surface) {
		SDL_FreeSurface(td->tiles_surface);
		td->tiles_surface = NULL;
	}
	td->rawpict_scale_wid_org = td->rawpict_scale_hgt_org = 0;
	td->rawpict_scale_wid_use = td->rawpict_scale_hgt_use = 0;
	C_WIPE(td->tiles_rawpict, MAX_TILES_RAWPICT + 1, rawpict_tile);

	/* Sub-tilesets */
	for (int s = 0; s < MAX_SUBFONTS; s++) {
		if (td->tiles_layers_sub[s]) {
			for (i = 0; i < layers; i++) {
				if (td->tiles_layers_sub[s][i]) SDL_FreeSurface(td->tiles_layers_sub[s][i]);
			}
			C_FREE(td->tiles_layers_sub[s], layers, SDL_Surface*);
		}
		td->tiles_layers_sub[s] = NULL;
		if (td->tiles_surface_sub[s]) {
			SDL_FreeSurface(td->tiles_surface_sub[s]);
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
			SDL_FreeSurface(td->tile_cache[i].tile);
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

/* Closes all SDL2 windows and frees all allocated data structures for input parameter. */
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
static void Term_nuke_sdl2(term *t) {
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
static errr Term_xtra_sdl2_level(int v) {
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

#ifdef SDL2_STICKY_KEYS
		if (td == &term_main) {
			uint8_t r = td->win->b_color.r;
			uint8_t g = td->win->b_color.g;
			uint8_t b = td->win->b_color.b;
			uint8_t a = td->win->b_color.a;
			if (ctrl_forced) r ^= 0xFF;
			if (shift_forced) g ^= 0xFF;
			if (alt_forced) b ^= 0xFF;
			uint32_t bc = SDL_MapRGBA(td->win->surface->format, r, g, b, a);
			/* Draw top, bottom, left and right border. */
			SDL_FillRect(td->win->surface, &(SDL_Rect){0, 0, td->win->wb, td->win->bh}, bc);
			SDL_FillRect(td->win->surface, &(SDL_Rect){0, td->win->bh + td->win->hd, td->win->wb, (td->win->hb - td->win->hd - td->win->bh)}, bc);
			SDL_FillRect(td->win->surface, &(SDL_Rect){0, td->win->bh + 1, td->win->bw, td->win->hd}, bc);
			SDL_FillRect(td->win->surface, &(SDL_Rect){td->win->bw + td->win->wd, td->win->bh + 1, td->win->wb - td->win->wd - td->win->bw, td->win->hd}, bc);
		}
#endif

		SDL_UpdateWindowSurface(td->win->window);
	}

	/* Sync if desired, using 'discard' */
	if (sync) {
		/* Call SDL_PumpEvents to update the input state, which can be analogous to sync operations */
		SDL_PumpEvents();

		if (discard) {
			/* If we want to discard events, we can clear the event queue */
			SDL_FlushEvent(SDL_FIRSTEVENT);
		}
	}

	/* Success */
	return 0;
}

/*
 * Handle a "special request"
 */
static errr Term_xtra_sdl2(int n, int v) {
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
		case TERM_XTRA_FLUSH: while (!CheckEvent(FALSE)); return(0);

		/* Handle change in the "level" */
		case TERM_XTRA_LEVEL: return(Term_xtra_sdl2_level(v));

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
static errr Term_wipe_sdl2(int x, int y, int n) {
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
static errr Term_curs_sdl2(int x, int y) {
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

		cursor = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
		if (cursor) {
			SDL_FillRect(cursor, NULL, SDL_MapRGBA(cursor->format, Pixel_quadruplet(cursor_color)));
			SDL_SetSurfaceBlendMode(cursor, SDL_BLENDMODE_BLEND);
			SDL_BlitSurface(cursor, NULL, Infowin->surface, &(SDL_Rect){px, py, w, h});
			SDL_FreeSurface(cursor);
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
static errr Term_text_sdl2(int x, int y, int n, byte a, cptr s) {
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

/* These variables are computed at image load (in 'init_sdl2'). */
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
/* Reinitialize graphic tiles for all windows after sdl2_graphics_pref_file_processed(). */
static bool graphics_reinitialize;
/* If there is any mask color set in prefs. */
static bool graphics_image_prefs_has_masks = false;
/* If an outline color is set in the pref file. */
static bool graphics_image_prefs_has_outline = false;
/* If there is any mask color set in prefs for subtileset. */
static bool graphics_image_sub_prefs_has_masks[MAX_SUBFONTS] = {false};
/* If an outline color is set in the pref file for subtileset. */
static bool graphics_image_sub_prefs_has_outline[MAX_SUBFONTS] = {false};

#if defined(USE_GRAPHICS) && defined(GRAPHICS_BG_MASK)

bool sdl2_apply_graphics_image_force_outline(int value) {
	if (value < -1) value = -1;
	if (value > SDL2_FORCE_OUTLINE_MAX_RADIUS) value = SDL2_FORCE_OUTLINE_MAX_RADIUS;

	if (sdl2_graphics_image_force_outline == value) return(TRUE);

	sdl2_graphics_image_force_outline = value;

	if (use_graphics == UG_NONE) return(TRUE);
	if (graphics_image == NULL) return(TRUE);

	graphics_reinitialize = true;
	return(sdl2_graphics_pref_file_processed());
}

#endif

bool sdl2_apply_graphics_resize_type(int value) {
	int old_gfx_resize_type = gfx_resize_type;

	if (value < 0) value = 0;
	if (value >= INTERPOLATION_TYPES_COUNT) value = INTERPOLATION_LINEAR;
	if (gfx_resize_type == value) return(TRUE);

	gfx_resize_type = value;

	if (use_graphics == UG_NONE) return(TRUE);
	if (graphics_image == NULL) return(TRUE);

	graphics_reinitialize = true;
	if (!sdl2_graphics_pref_file_processed()) {
		gfx_resize_type = old_gfx_resize_type;
		graphics_reinitialize = true;
		if (!sdl2_graphics_pref_file_processed()) {
			quit_fmt("Failed to reinitialize graphics after failed resize type change");
		}
		return(false);
	}

	return(true);
}

static errr term_data_init_graphics(term_data *td);
static errr Term_pict_sdl2_2mask(int x, int y, byte a, char32_t c, byte a_back, char32_t c_back);
static errr Term_pict_sdl2(int x, int y, byte a, char32_t c);
#ifdef USE_GRAPHICS
static errr Term_rawpict_sdl2(int x, int y, int c);
#endif
static uint32_t graphics_default_mask(uint8_t n);
static void sdl2_ensure_graphics_mask_colors(void);

/* Gets called after graphics image pref file is loaded. */
bool sdl2_graphics_pref_file_processed() {
	bool success = true;

	if (use_graphics) {
		sdl2_ensure_graphics_mask_colors();
	}
	if (use_graphics && graphics_reinitialize) {
		if (!graphics_image_prefs_has_masks) {
			fprintf(stderr, "Warning: Masks colors are not set in pref file. Using default values%s.\n", graphics_image_mpt == 3 ? " and compatibility mode" : "");
		}

		/* Initialize graphics for each initialized term. */
		for (int i = 0; i < ANGBAND_TERM_MAX; i++) {
			if (!term_get_visibility(i)) continue;

			term_data *td = term_idx_to_term_data(i);
			if (term_data_init_graphics(td)) success = false;
			if (td->tiles_layers != NULL && td->tilePreparation != NULL) {
				/* Graphics hook */
#ifdef GRAPHICS_BG_MASK
				if (use_graphics == UG_2MASK) {
					td->t.pict_hook_2mask = Term_pict_sdl2_2mask;
				}
#endif
				td->t.pict_hook = Term_pict_sdl2;
				td->t.rawpict_hook = Term_rawpict_sdl2;

				/* Use graphics sometimes */
				td->t.higher_pict = TRUE;
			}
			else {
				fprintf(stderr, "Couldn't prepare images for terminal %d\n", i);
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
		SDL_FillRect(entry->tile, NULL, 0x00000000);
	}

	return(entry->tile);
}
 #endif

static void draw_colored_layers_to_surface(SDL_Rect dst_rect, SDL_Surface *dst, uint8_t n, SDL_Rect src_rect, SDL_Surface *layers[], Pixel colors[], SDL_Surface *preparation) {
	for (uint32_t i = 0; i < n; i++) {
		if (colors[i].a == 0) continue;

		/* Draw mask of i-th layer in desired color to preparation surface. */
		SDL_FillRect(preparation, NULL, SDL_MapRGBA(preparation->format, Pixel_quadruplet(colors[i])));
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
	SDL_FillRect(td->win->surface, &(SDL_Rect){x, y, td->fnt->wid, td->fnt->hgt}, SDL_MapRGBA(td->win->surface->format, Pixel_quadruplet(colors[0])));

	for (uint32_t i = 0; i < graphics_image_tpc; i++) {
		if (indexes[i] == 0xFFFFFFFF) continue;
		term_data_draw_graphics_tile(td, x, y, indexes[i], &colors[i * graphics_image_mpt], subsets ? subsets[i] : -1);
	}
	return(0);
}

static bool Term_pict_sdl2_show_error = true;

/*
 * Draw some graphical characters.
 */
static errr Term_pict_sdl2(int x, int y, byte a, char32_t c) {

	if (graphics_image_mpt != 2 || graphics_image_tpc != 1) {
		if (Term_pict_sdl2_show_error) fprintf(stderr, "ERROR: Term_pict_sdl2: masks per tile %d or tiles per coord %d is wrong, should be %d, %d\n", graphics_image_mpt, graphics_image_tpc, 2, 1);
		Term_pict_sdl2_show_error = false;
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
static bool Term_pict_sdl2_2mask_show_error = true;

static errr Term_pict_sdl2_2mask(int x, int y, byte a, char32_t c, byte a_back, char32_t c_back) {

	if (graphics_image_mpt != 3 || graphics_image_tpc != 2) {
		if (Term_pict_sdl2_2mask_show_error) fprintf(stderr, "ERROR: Term_pict_sdl2_2mask: masks per tile %d or tiles per coordinate %d is wrong, should be %d, %d\n", graphics_image_mpt, graphics_image_tpc, 3, 2);
		Term_pict_sdl2_2mask_show_error = false;
		return(Term_pict_sdl2(x, y, a, c));
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

static errr Term_rawpict_sdl2(int x, int y, int c) {
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

static Pixel sdl2_tileset_preview_mask_color(int type, int index, int count) {
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

static term_data *sdl2_tileset_preview_term(void) {
	term_data *td = term_idx_to_term_data(0);

	if (!td || !td->win || !td->win->surface || !td->fnt) return(NULL);
	return(td);
}

bool sdl2_tileset_preview_ready(void) {
	term_data *td = sdl2_tileset_preview_term();

	if (!td) return(FALSE);
	if (!use_graphics) return(FALSE);
	if (td->tiles_layers == NULL || td->nlayers <= 0) return(FALSE);

	return(TRUE);
}

void sdl2_tileset_preview_fill_cell(int col, int row, int count, uint8_t background_value) {
	term_data *td = sdl2_tileset_preview_term();

	if (!td) return;
	if (count <= 0) return;

	SDL_Rect rect = {td->win->bw + col * td->fnt->wid, td->win->bh + row * td->fnt->hgt, count * td->fnt->wid, td->fnt->hgt};
	SDL_FillRect(td->win->surface, &rect, SDL_MapRGBA(td->win->surface->format, background_value, background_value, background_value, 255));
}

void sdl2_tileset_preview_draw_tile(int col, int row, int type, char32_t tile_char, char32_t background_char) {
	term_data *td = sdl2_tileset_preview_term();

	sdl2_tileset_preview_fill_cell(col, row, 4 + (2 * GRAPHICS_MAX_MPT), 0);
	sdl2_tileset_preview_fill_cell(col, row + 1, 4 + (2 * GRAPHICS_MAX_MPT), 0);

	if (!td || !sdl2_tileset_preview_ready()) {
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
		colors[1 + i] = sdl2_tileset_preview_mask_color(type, i, mask_count);
	}

	for (int i = 0; i < 2; i++) {
		/* Draw the requested tile on black/white background, with outline of the same color. */
		colors[0] = (Pixel){255*(i%2), 255*(i%2), 255*(i%2), 255};
		/* The last mask is outline. Make it background color. */
		if (mask_count > 1) colors[1 + mask_count - 1] = colors[0];
		int px = td->win->bw + (col + i) * td->fnt->wid;
		int py = td->win->bh + row * td->fnt->hgt;
		SDL_FillRect(td->win->surface, &(SDL_Rect){px, py, td->fnt->wid, td->fnt->hgt}, SDL_MapRGBA(td->win->surface->format, Pixel_quadruplet(colors[0])));
		term_data_draw_graphics_tile(td, px, py, tile_index, colors, tile_subset);
	}

	uint32_t srcX = (tile_index % graphics_image_tpr) * td->fnt->wid;
	uint32_t srcY = (tile_index / graphics_image_tpr) * td->fnt->hgt;
	uint32_t dstY = td->win->bh + row * td->fnt->hgt;
	Uint32 fill = SDL_MapRGBA(td->win->surface->format, 0, 0, 0, 255);
	for (int l = 0; l < td->nlayers; l++) {
		uint32_t dstX = td->win->bw + (col + 4 + (2*l)) * td->fnt->wid;
		SDL_Rect src_rect = {srcX, srcY, td->fnt->wid, td->fnt->hgt};
		SDL_Rect dst_rect = {dstX, dstY, td->fnt->wid, td->fnt->hgt};
		SDL_FillRect(td->win->surface, &dst_rect, fill);
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
		colors[1 + i] = sdl2_tileset_preview_mask_color(0, i, mask_count);
		/* Top tile colors. */
		colors[graphics_image_mpt + 1 + i] = sdl2_tileset_preview_mask_color(type, i, mask_count);
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
					td->tiles_rawpict[i].defined = FALSE;
					continue;
				}
				td->tiles_rawpict[i].defined = TRUE;
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
					td->tiles_rawpict_sub[sub][i].defined = FALSE;
					continue;
				}
				td->tiles_rawpict_sub[sub][i].defined = TRUE;
				td->tiles_rawpict_sub[sub][i].x = (tiles_rawpict_org_sub[sub][i].x * width2) / width1;
				td->tiles_rawpict_sub[sub][i].y = (tiles_rawpict_org_sub[sub][i].y * height2) / height1;
				td->tiles_rawpict_sub[sub][i].w = (tiles_rawpict_org_sub[sub][i].w * width2) / width1;
				td->tiles_rawpict_sub[sub][i].h = (tiles_rawpict_org_sub[sub][i].h * height2) / height1;
			}
		}
	}
}

#endif /* USE_GRAPHICS */

static color_rgb sdl2_surface_get_pixel_rgb(SDL_Surface *surface, const uint32_t *pixels, int pitch, int x, int y) {
	Uint8 red, green, blue, alpha;
	color_rgb color;

	SDL_GetRGBA(pixels[y * pitch + x], surface->format, &red, &green, &blue, &alpha);
	color.red = red;
	color.green = green;
	color.blue = blue;

	(void)alpha;
	return(color);
}

static bool sdl2_surface_pixel_is_mask(SDL_Surface *surface, const uint32_t *pixels, int pitch, int x, int y, const uint32_t mask_colors[]) {
	color_rgb sample = sdl2_surface_get_pixel_rgb(surface, pixels, pitch, x, y);

	for (uint8_t i = 0; i < graphics_image_mpt; i++) {
		if (mask_colors[i] == 0) continue;
		if (sample.red == ((mask_colors[i] >> 24) & 0xFF) &&
		    sample.green == ((mask_colors[i] >> 16) & 0xFF) &&
		    sample.blue == ((mask_colors[i] >> 8) & 0xFF)) {
			return(TRUE);
		}
	}

	return(FALSE);
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
	dst = SDL_CreateRGBSurface(0, new_wid, new_hgt, 32, src->format->Rmask, src->format->Gmask, src->format->Bmask, src->format->Amask);
	if (!dst) {
		fprintf(stderr, "Error creating scaled surface: %s\n", SDL_GetError());
		return NULL;
	}
	src_pitch = src->pitch / src->format->BytesPerPixel;
	dst_pitch = dst->pitch / dst->format->BytesPerPixel;

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
		scaled[rawpict_index].defined = TRUE;
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

	dst = SDL_CreateRGBSurface(0, new_wid, new_hgt, 32, src->format->Rmask, src->format->Gmask, src->format->Bmask, src->format->Amask);
	if (!dst) {
		fprintf(stderr, "Error creating linear scaled surface: %s\n", SDL_GetError());
		return NULL;
	}
	src_pitch = src->pitch / src->format->BytesPerPixel;
	dst_pitch = dst->pitch / dst->format->BytesPerPixel;

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
				SDL_GetRGBA(src_pixels[p[i].y * src_pitch + p[i].x], src->format, &c.r, &c.g, &c.b, &c.a);

				/* If mask_colors are defined and src pixel is one of mask_colors, skip it (skipping treats the color as transparent black). */
				if (is_mask_color(c, mask_colors)) {
					continue;
				}

				/* Update destination pixel colors according to weights and source color. */
				r += w[i] * c.r; g += w[i] * c.g; b += w[i] * c.b; a += w[i] * c.a;
			}

			/* Write the computed scaled pixel color. */
			dst_pixels[dst_y * dst_pitch + dst_x] = SDL_MapRGBA(
					dst->format,
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

	dst = SDL_CreateRGBSurface(0, new_wid, new_hgt, 32, src->format->Rmask, src->format->Gmask, src->format->Bmask, src->format->Amask);
	if (!dst) {
		fprintf(stderr, "Error creating lanczos scaled surface: %s\n", SDL_GetError());
		return NULL;
	}
	src_pitch = src->pitch / src->format->BytesPerPixel;
	dst_pitch = dst->pitch / dst->format->BytesPerPixel;

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
					SDL_GetRGBA(src_pixels[cy * src_pitch + cx], src->format, &c.r, &c.g, &c.b, &c.a);

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
					dst->format,
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
static void sdl2_ensure_graphics_mask_colors(void) {
	bool has_masks = FALSE;

	if (!graphics_image_mpt) return;

	graphics_image_prefs_has_masks = FALSE;
	graphics_image_prefs_has_outline = FALSE;
	for (int s = 0; s < MAX_SUBFONTS; s++) {
		graphics_image_sub_prefs_has_masks[s] = FALSE;
		graphics_image_sub_prefs_has_outline[s] = FALSE;
	}

	/* Check whether primary mask colors are defined in prefs. */
	for (uint8_t i = 0; i < graphics_image_mpt; i++) {
		if (graphics_image_masks_colors_prefs[i] != 0) {
			has_masks = TRUE;
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
		bool sub_has_masks = FALSE;

		/* Detect whether subfont overrides are defined in prefs. */
		for (uint8_t i = 0; i < graphics_image_mpt; i++) {
			if (graphics_image_masks_colors_sub_prefs[s][i] != 0) {
				sub_has_masks = TRUE;
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
		(*layers_out)[i] = SDL_CreateRGBSurfaceWithFormat(0, scaled_image->w, scaled_image->h, 32, SDL_PIXELFORMAT_RGBA32);
		if (!(*layers_out)[i]) {
			for (int j = 0; j < i; j++) SDL_FreeSurface((*layers_out)[j]);
			C_FREE(*layers_out, nlayers, SDL_Surface*);
			*layers_out = NULL;
			return(1);
		}

		/* The transparent color has to be different from mask color (color key is only RGB, alpha is not considered). */
		SDL_FillRect((*layers_out)[i], NULL, SDL_MapRGBA((*layers_out)[i]->format, ((mask_colors[i+1] >> 24) & 0xFF) ^ 0xFF, ((mask_colors[i+1] >> 16) & 0XFF) ^ 0XFF, ((mask_colors[i+1] >> 8) & 0xFF) ^ 0xFF, 0x00));
		SDL_SetColorKey((*layers_out)[i], SDL_TRUE, SDL_MapRGB((*layers_out)[i]->format, ((mask_colors[i+1] >> 24) & 0xFF), ((mask_colors[i+1] >> 16) & 0xFF), ((mask_colors[i+1] >> 8) & 0xFF)));
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
	SDL_SetColorKey(scaled_image, SDL_TRUE, SDL_MapRGB(scaled_image->format, ((mask_colors[0] >> 24) & 0xFF), ((mask_colors[0] >> 16) & 0xFF), ((mask_colors[0] >> 8) & 0xFF)));
	SDL_BlitSurface(scaled_image, NULL, layers[0], NULL);

	/* Separate the mask colours into each layer. */
	if (nlayers > 1) {
		src_pixels = (uint32_t*)layers[0]->pixels;
		src_pitch = layers[0]->pitch / layers[0]->format->BytesPerPixel;

		for (y = 0; y < layers[0]->h; y++) {
			for (x = 0; x < layers[0]->w; x++) {
				src_pos = (y * src_pitch) + x;
				for (l = 1; l < nlayers; l++) {
					/* Skip layers with zeroed colors and alpha. */
					if (mask_colors[l + 1] == 0) continue;

					if (src_pixels[src_pos] == SDL_MapRGBA(layers[0]->format, ((mask_colors[l + 1] >> 24) & 0xFF), ((mask_colors[l + 1] >> 16) & 0xFF), ((mask_colors[l + 1] >> 8) & 0xFF), 0xFF)) {
						dst_pixels = (uint32_t*)layers[l]->pixels;
						dst_pitch = layers[l]->pitch / layers[l]->format->BytesPerPixel;

						dst_pixels[y * dst_pitch + x] = src_pixels[src_pos];
						src_pixels[src_pos] = SDL_MapRGBA(layers[0]->format, ((mask_colors[0] >> 24) & 0xFF) ^ 0xFF, ((mask_colors[0] >> 16) & 0xFF) ^ 0xFF, ((mask_colors[0] >> 8) & 0xFF) ^ 0xFF, 0);
					}
				}
			}
		}
	}
}

/* Replace opaque pixels in the base layer with pixels from the filtered tileset. */
static errr copy_filtered_base_layer_pixels(SDL_Surface *layer, SDL_Surface *color_image, const uint32_t mask_colors[]) {
	if (!layer || !color_image) return 1;

	if (layer->format->format != SDL_PIXELFORMAT_RGBA32 || color_image->format->format != SDL_PIXELFORMAT_RGBA32) return 2;
	if (layer->h != color_image->h || layer->w != color_image->w) return 3;
	if (SDL_MUSTLOCK(layer)) if (SDL_LockSurface(layer) != 0) return 4;
	if (SDL_MUSTLOCK(color_image)) if (SDL_LockSurface(color_image) != 0) {
		if (SDL_MUSTLOCK(layer)) SDL_UnlockSurface(layer);
		return 5;
	}

	uint32_t *layer_row;
	int x, y;

	for (y = 0; y < layer->h; y++) {
		layer_row = (uint32_t *)((uint8_t*)layer->pixels + y * layer->pitch);
		for (x = 0; x < layer->w; x++) {
			/* Don't copy transparent and masked pixels. */
			if ((layer_row[x] & layer->format->Amask) == 0) continue;
			if (mask_colors != NULL && sdl2_surface_pixel_is_mask(layer, layer_row, 0, x, 0, mask_colors)) continue;

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
	if (sdl2_graphics_image_force_outline < 0) return;

	int ol = nlayers - 1;
	SDL_FillRect(layers[ol], NULL, SDL_MapRGBA(layers[ol]->format, ((mask_colors[ol + 1] >> 24) & 0xFF) ^ 0xFF, ((mask_colors[ol + 1] >> 16) & 0XFF) ^ 0XFF, ((mask_colors[ol + 1] >> 8) & 0xFF) ^ 0xFF, 0x00));

	if (sdl2_graphics_image_force_outline == 0) return;

	int tiles_per_row = tile_w > 0 ? scaled_image->w / tile_w : 0;
	int tiles_per_col = tile_h > 0 ? scaled_image->h / tile_h : 0;
	int outline_radius_x = sdl2_graphics_image_force_outline;
	int outline_radius_y = sdl2_graphics_image_force_outline;
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
		outline_radius_x = (sdl2_graphics_image_force_outline * tile_w + graphics_tile_wid / 2) / graphics_tile_wid;
	}
	if (graphics_tile_hgt > 0) {
		outline_radius_y = (sdl2_graphics_image_force_outline * tile_h + graphics_tile_hgt / 2) / graphics_tile_hgt;
	}

	if (outline_radius_x < 1) outline_radius_x = 1;
	if (outline_radius_y < 1) outline_radius_y = 1;

	bg_mask_color = SDL_MapRGBA(scaled_image->format, ((mask_colors[0] >> 24) & 0xFF), ((mask_colors[0] >> 16) & 0xFF), ((mask_colors[0] >> 8) & 0xFF), 0xFF);
	ol_mask_color = SDL_MapRGBA(scaled_image->format, ((mask_colors[ol + 1] >> 24) & 0xFF), ((mask_colors[ol + 1] >> 16) & 0xFF), ((mask_colors[ol + 1] >> 8) & 0xFF), 0xFF);
	src_pitch = scaled_image->pitch / scaled_image->format->BytesPerPixel;
	dst_pitch = layers[ol]->pitch / layers[ol]->format->BytesPerPixel;

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
					found_bg = FALSE;
					if (src_pixels[src_pos + right] == bg_mask_color) found_bg = TRUE;
					if (mask_colors[ol + 1] != 0 && src_pixels[src_pos + right] == ol_mask_color) found_bg = TRUE;
					if (!found_bg) {
						cnt++;
					}
				}
				/* Shrink left to max(start, x-r). */
				bound = start > x - outline_radius_x ? start : x - outline_radius_x;
				while (left < bound) {
					found_bg = FALSE;
					if (src_pixels[src_pos + left] == bg_mask_color) found_bg = TRUE;
					if (mask_colors[ol + 1] != 0 && src_pixels[src_pos + left] == ol_mask_color) found_bg = TRUE;
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
			found_bg = FALSE;
			if (src_pixels[src_pos] == bg_mask_color) found_bg = TRUE;
			if (mask_colors[ol + 1] != 0 && src_pixels[src_pos] == ol_mask_color) found_bg = TRUE;
			if(vpass[pass_pos] == 1 && found_bg) {
				dst_pixels = (uint32_t*)layers[ol]->pixels;
				dst_pixels[y * dst_pitch + x] = SDL_MapRGBA(layers[ol]->format, ((mask_colors[ol + 1] >> 24) & 0xFF), ((mask_colors[ol + 1] >> 16) & 0xFF), ((mask_colors[ol + 1] >> 8) & 0xFF), 0xFF);
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
	*out_surface = SDL_CreateRGBSurfaceWithFormat(0, scaled_image->w, scaled_image->h, 32, SDL_PIXELFORMAT_RGBA32);
	if (!*out_surface) return(1);

	SDL_SetSurfaceBlendMode(*out_surface, SDL_BLENDMODE_BLEND);
	if (SDL_BlitSurface(scaled_image, NULL, *out_surface, NULL) != 0) {
		SDL_FreeSurface(*out_surface);
		*out_surface = NULL;
		return(2);
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
			rawpict_dst[i].defined = FALSE;
			continue;
		}

		rawpict_dst[i].defined = TRUE;
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
	int src_pitch = layers[0]->pitch / layers[0]->format->BytesPerPixel;
	int x, y;
	uint32_t pos;

	for (y = 0; y < layers[0]->h; y++) {
		for (x = 0; x < layers[0]->w; x++) {
			pos = (y * src_pitch) + x;
			if (src_pixels[pos] == SDL_MapRGBA(layers[0]->format, ((mask_colors[2] >> 24) & 0xFF), ((mask_colors[2] >> 16) & 0xFF), ((mask_colors[2] >> 8) & 0xFF), 0xFF)) {
				src_pixels[pos] = SDL_MapRGBA(layers[0]->format, ((mask_colors[0] >> 24) & 0xFF) ^ 0xFF, ((mask_colors[0] >> 16) & 0xFF) ^ 0xFF, ((mask_colors[0] >> 8) & 0xFF) ^ 0xFF, 0);
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
static errr term_data_init_graphics_tileset(term_data *td, SDL_Surface *src_image, SDL_Surface ***layers_out, SDL_Surface **tiles_surface_out, int *wid_org, int *hgt_org, int *wid_use, int *hgt_use, rawpict_tile *rawpict_org, rawpict_tile *rawpict_dst, const uint32_t mask_colors[], bool has_outline) {
	if (!td || !td->fnt || !src_image || !layers_out || !tiles_surface_out || !wid_org || !hgt_org || !wid_use || !hgt_use || !rawpict_org || !rawpict_dst || !mask_colors) return(TDGTS_ERR_RAWPICT);

	/* Resize the loaded graphics image, so tile size match font size. */
	/* Note: Always scale the mask source to INTERPOLATION_NEAR.
	 * Layer separation and forced outline generation rely on exact single-color masks,
	 * so filtered scaling cannot be used there until SDL2 graphics gain a proper
	 * semi-transparent / coverage-mask pipeline.
	 */
	SDL_Surface *scaled_image = ScaleSurfaceNearest(src_image, graphics_tile_wid, graphics_tile_hgt, td->fnt->wid, td->fnt->hgt);
	SDL_Surface *scaled_rawpict = scaled_image;
	if (!scaled_image) return(TDGTS_ERR_SCALE);

	if (gfx_resize_type != INTERPOLATION_NEAR) {
		/* Filtered scaling currently applies only to non-mask pixel content copied back
		 * into layer[0] / rawpict. Mask layers and generated outline intentionally keep
		 * nearest-neighbour edges for exact single-color masking.
		 */
		if (gfx_resize_type == INTERPOLATION_LINEAR) {
			scaled_rawpict = ScaleSurfaceLinear(src_image, graphics_tile_wid, graphics_tile_hgt, td->fnt->wid, td->fnt->hgt, rawpict_org, NULL);
		} else if (gfx_resize_type == INTERPOLATION_LANCZOS) {
			scaled_rawpict = ScaleSurfaceLanczos(src_image, graphics_tile_wid, graphics_tile_hgt, td->fnt->wid, td->fnt->hgt, rawpict_org, NULL);
		} else {
			SDL_FreeSurface(scaled_image);
			return(TDGTS_ERR_RESIZETYPE);
		}
		if (!scaled_rawpict) {
			SDL_FreeSurface(scaled_image);
			return(TDGTS_ERR_SCALE);
		}
	}

	/* Keep a copy of the scaled sheet for rawpict drawing. */
	if (init_scaled_rawpict(scaled_rawpict, src_image, tiles_surface_out, wid_org, hgt_org, wid_use, hgt_use, rawpict_org, rawpict_dst)) {
		if (scaled_rawpict != scaled_image) SDL_FreeSurface(scaled_rawpict);
		SDL_FreeSurface(scaled_image);
		return(TDGTS_ERR_RAWPICT);
	}

	/* Initialize surfaces for all layers. */
	if (init_tile_layers(scaled_image, td->nlayers, layers_out, mask_colors)) {
		if (scaled_rawpict != scaled_image) SDL_FreeSurface(scaled_rawpict);
		SDL_FreeSurface(scaled_image);
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
		SDL_Surface *scaled_nomask = NULL;
		if (gfx_resize_type == INTERPOLATION_LINEAR) {
			scaled_nomask = ScaleSurfaceLinear(src_image, graphics_tile_wid, graphics_tile_hgt, td->fnt->wid, td->fnt->hgt, NULL, mask_colors);
		} else if (gfx_resize_type == INTERPOLATION_LANCZOS) {
			scaled_nomask = ScaleSurfaceLanczos(src_image, graphics_tile_wid, graphics_tile_hgt, td->fnt->wid, td->fnt->hgt, NULL, mask_colors);
		}
		copy_filtered_base_layer_pixels((*layers_out)[0], scaled_nomask, mask_colors);
		SDL_FreeSurface(scaled_nomask);
	}

	/* The scaled images are not needed anymore. */
	if (scaled_rawpict != scaled_image) SDL_FreeSurface(scaled_rawpict);
	SDL_FreeSurface(scaled_image);

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
static errr term_data_init_graphics(term_data *td) {
	/* Free old tiles. */
	free_graphics(td);

	if (!graphics_image) {
		fprintf(stderr, "No graphics image loaded for SDL2 term.\n");
		return(TDGTS_ERR_NOIMAGE);
	}

	/* Initialize surfaces for all layers. */
	td->nlayers = graphics_image_mpt - 1;

	errr err = term_data_init_graphics_tileset(td, graphics_image, &td->tiles_layers, &td->tiles_surface, &td->rawpict_scale_wid_org, &td->rawpict_scale_hgt_org, &td->rawpict_scale_wid_use, &td->rawpict_scale_hgt_use, tiles_rawpict_org, td->tiles_rawpict, graphics_image_masks_colors, graphics_image_prefs_has_outline);
	if (err != TDGTS_ERR_NONE) {
		if (err == TDGTS_ERR_SCALE) fprintf(stderr, "Failed to scale graphics surface for SDL2 term.\n");
		if (err == TDGTS_ERR_LAYERS) fprintf(stderr, "Failed to initialize graphics layers for SDL2 term.\n");
		if (err == TDGTS_ERR_RAWPICT) fprintf(stderr, "Failed to initialize SDL2 term rawpict surface.\n");
		if (err == TDGTS_ERR_RESIZETYPE) fprintf(stderr, "Failed to scale due unknown resize type.\n");
		free_graphics(td);
		return(err);
	}

	/* Prepare (partial) sub-tilesets */
	for (int s = 0; s < MAX_SUBFONTS; s++) {
		if (!graphic_subtiles[s]) continue;
		if (!graphics_image_sub[s]) {
			fprintf(stderr, "Warning: Graphics sub-tileset %d has no loaded image; disabling.\n", s);
			graphic_subtiles[s] = FALSE;
			continue;
		}

		err = term_data_init_graphics_tileset(td, graphics_image_sub[s], &td->tiles_layers_sub[s], &td->tiles_surface_sub[s], &td->rawpict_scale_wid_org_sub[s], &td->rawpict_scale_hgt_org_sub[s], &td->rawpict_scale_wid_use_sub[s], &td->rawpict_scale_hgt_use_sub[s], tiles_rawpict_org_sub[s], td->tiles_rawpict_sub[s], graphics_image_masks_colors_sub[s], graphics_image_sub_prefs_has_outline[s]);
		if (err != TDGTS_ERR_NONE) {
			if (err == TDGTS_ERR_SCALE) fprintf(stderr, "Warning: Failed to scale graphics sub-tileset %d; disabling.\n", s);
			if (err == TDGTS_ERR_LAYERS) fprintf(stderr, "Warning: Failed to initialize graphics layers for SDL2 sub-tileset %d; disabling.\n", s);
			if (err == TDGTS_ERR_RAWPICT) fprintf(stderr, "Warning: Failed to initialize rawpict surface for SDL2 sub-tileset %d; disabling.\n", s);
			if (err == TDGTS_ERR_RESIZETYPE) fprintf(stderr, "Warning: Failed to scale due unknown resize type for SDL2 sub-tileset %d; disabling.\n", s);
			graphic_subtiles[s] = FALSE;
			continue;
		}
	}

	/* Initialize preparation surface. */
	td->tilePreparation = SDL_CreateRGBSurfaceWithFormat(0, td->fnt->wid, td->fnt->hgt, 32, SDL_PIXELFORMAT_RGBA32);
	if (!td->tilePreparation) {
		fprintf(stderr, "Failed to initialize SDL2 term tile preparation surface.\n");
		free_graphics(td);
		return(TDGTS_ERR_TILEPREP);
	}
	SDL_SetSurfaceBlendMode(td->tilePreparation, SDL_BLENDMODE_BLEND);

	/* Note: If we want to cache even more graphics for faster drawing, we could initialize 16 copies of the graphics image with all possible mask colors already applied. */
	/* Memory cost could become "large" quickly though (eg 5MB bitmap -> 80MB). Not a real issue probably. */
#ifdef TILE_CACHE_SIZE
	for (int i = 0; i < TILE_CACHE_SIZE; i++) {
		td->tile_cache[i].tile = SDL_CreateRGBSurfaceWithFormat(0, td->fnt->wid, td->fnt->hgt, 32, SDL_PIXELFORMAT_RGBA32);
		SDL_SetSurfaceBlendMode(td->tile_cache[i].tile, SDL_BLENDMODE_BLEND);
		td->tile_cache[i].ncolors = graphics_image_mpt - 1;
		C_MAKE(td->tile_cache[i].colors, td->tile_cache[i].ncolors, uint32_t);
		td->tile_cache[i].subset = -1;
		td->tile_cache[i].index = 0;
	}
#endif

	return(TDGTS_ERR_NONE);
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

	/* Prepare the standard font */
	MAKE(td->fnt, infofnt);
	infofnt *old_infofnt = Infofnt;
	Infofnt_set(td->fnt);
	if (Infofnt_init(font) == -1) {
		/* Initialization failed, log and try to use the default font. */
		fprintf(stderr, "Failed to load the \"%s\" font for terminal %d\n", font, index);
		if (in_game) {
			/* If in game, inform the user. */
			Infofnt_set(old_infofnt);
			plog_fmt("Failed to load the \"%s\" font! Falling back to default font.\n", font);
			Infofnt_set(td->fnt);
		} 
		if (Infofnt_init(sdl2_terms_font_default[index]) == -1) {
			/* Initialization of the default font failed too. Log, free allocated memory and return with error. */
			fprintf(stderr, "Failed to load the default \"%s\" font for terminal %d\n", sdl2_terms_font_default[index], index);
			Infofnt_set(old_infofnt);
			if (in_game) {
				/* If in game, inform the user. */
				plog_fmt("Failed to load the default \"%s\" font too! Try to change font manually.\n", sdl2_terms_font_default[index]);
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
			n = getenv(sdl2_terms_wid_env[i]);
			if (n) win_cols = atoi(n);
			n = getenv(sdl2_terms_hgt_env[i]);
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
	int wid_border = wid_draw + (2 * SDL2_DEFAULT_BORDER_WIDTH);
	int hgt_border = hgt_draw + (2 * SDL2_DEFAULT_BORDER_WIDTH);

	MAKE(td->win, infowin);
	Infowin_set(td->win);

	Infowin_init(topx, topy, wid_border, hgt_border, SDL2_DEFAULT_BORDER_WIDTH, color_default_b, color_default_bg);

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

	/* Use a "soft" cursor */
	t->soft_cursor = TRUE;

	/* Erase with "white space" */
	t->attr_blank = TERM_WHITE;
	t->char_blank = ' ';

	/* Hooks */
	t->xtra_hook = Term_xtra_sdl2;
	t->curs_hook = Term_curs_sdl2;
	t->wipe_hook = Term_wipe_sdl2;
	t->text_hook = Term_text_sdl2;
	t->nuke_hook = Term_nuke_sdl2;

#ifdef USE_GRAPHICS
	/* Use graphics. */
	if (use_graphics) {
		/* Note: Some preferences that affect graphic tile initialization are not loaded yet.
		 * Initialization is therefore done in `sdl2_graphics_pref_file_processed()`,
		 * which runs after relevant preferences are loaded.
		 * A minimal initialization is still needed here, because the login screen
		 * needs pict_hook to render the TomeNET logo.
		 */

		sdl2_ensure_graphics_mask_colors();

		if (!term_data_init_graphics(td) && td->tiles_layers != NULL && td->tilePreparation != NULL) {
			/* Graphics hook */
 #ifdef GRAPHICS_BG_MASK
			if (use_graphics == UG_2MASK) {
				t->pict_hook_2mask = Term_pict_sdl2_2mask;
			}
 #endif
			t->pict_hook = Term_pict_sdl2;
			t->rawpict_hook = Term_rawpict_sdl2;

			/* Use graphics sometimes */
			t->higher_pict = TRUE;
		} else {
			fprintf(stderr, "Couldn't prepare images for terminal %d\n", index);
		}
	}
#endif /* USE_GRAPHICS */

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
 * These colors are overwritten with the generic, OS-independent client_color_map[] in enable_common_colormap_sdl2()!
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

static void enable_common_colormap_sdl2() {
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

void enable_readability_blue_sdl2(void) {
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
 * `term_idx` selects the fallback size from sdl2_terms_ttf_size_default[].
 * */
static void sanitize_font_format(char *font, size_t font_len, int term_idx) {
	if (font == NULL || font_len == 0) return;

	char   font_base[256];
	int8_t size = 0;
	size_t len;

	if (is_ttf_font(font, font_base, sizeof(font_base), &size)) {
		/* Fix missing or out-of-range size. */
		if (size < 0) size = sdl2_terms_ttf_size_default[term_idx];

		if (size < SDL2_MIN_TTF_FONT_SIZE) size = SDL2_MIN_TTF_FONT_SIZE;
		else if (size > SDL2_MAX_TTF_FONT_SIZE) size = SDL2_MAX_TTF_FONT_SIZE;

		/* Rebuild the canonical "<file> <size>" string. */
		snprintf(font, font_len, "%s %d", font_base, size);
	} else {
		/* Trim trailing spaces for non-TTF descriptions. */
		len = strlen(font);
		while (len && isspace((unsigned char)font[len - 1])) font[--len] = '\0';
	}
}

/*
 * Initialization of i-th SDL2 terminal window.
 */
static errr sdl2_term_init(int term_id) {
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

	/* Check environment for SDL2 terminal font. */
	fnt_name = getenv(sdl2_terms_font_env[term_id]);
	/* Check environment for "base" font. */
	if (!fnt_name) fnt_name = getenv("TOMENET_SDL2_FONT");
	/* Use loaded (from config file) or predefined default font. */
	if (!fnt_name && strlen(term_prefs[term_id].font)) fnt_name = term_prefs[term_id].font;
	/* Paranoia, use the default. */
	if (!fnt_name) fnt_name = sdl2_terms_font_default[term_id];

	strncpy(sanitized_fnt_name, fnt_name, sizeof(sanitized_fnt_name));
	sanitized_fnt_name[sizeof(sanitized_fnt_name)-1] = '\0';
	sanitize_font_format(sanitized_fnt_name, sizeof(sanitized_fnt_name), term_id);

	/* Initialize the terminal window, allow resizing, for font changes. */
	err = term_data_init(term_id, sdl2_terms_term_data[term_id], FALSE, ang_term_name[term_id], sanitized_fnt_name);
	/* Store created terminal with SDL2 term data to ang_term array, even if term_data_init failed, but only if there is one. */
	if (Term && term_data_to_term_idx(Term->data) == term_id) ang_term[term_id] = Term;

	if (err) {
		fprintf(stderr, "Error initializing term_data for SDL2 terminal with index %d\n", term_id);
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
errr init_graphics_sdl2(void) {
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
		path_build(path, 1024, ANGBAND_USER_DIR_XTRA, "graphics");
		ANGBAND_USER_DIR_XTRA_GRAPHICS = string_make(path);
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
	if (my_fexists(filename_user) || sdl2_paths_same(ANGBAND_USER_DIR_XTRA_GRAPHICS, ANGBAND_DIR_XTRA_GRAPHICS)) strcpy(filename, filename_user);
	else strcpy(filename, filename_game);

	/* Reset subtile filenames before discovery. */
	for (int i = 0; i < MAX_SUBFONTS; i++) {
		if (!graphic_subtiles[i]) continue;
		graphic_subtiles_file[i][0] = 0;
	}

	/* Discover (partial) subtile files matching selected tileset */
	const char *graphics_dirs[] = {ANGBAND_USER_DIR_XTRA_GRAPHICS, ANGBAND_DIR_XTRA_GRAPHICS};
	int graphics_dir_count = sdl2_paths_same(graphics_dirs[0], graphics_dirs[1]) ? 1 : 2;
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
	if (graphics_image->format->format != SDL_PIXELFORMAT_RGBA32) {
		SDL_Surface* convertedSurface = SDL_ConvertSurfaceFormat(graphics_image, SDL_PIXELFORMAT_RGBA32, 0);
		if (!convertedSurface) {
			snprintf(use_graphics_errstr, sizeof(use_graphics_errstr), "Error converting loaded image into RGBA32 format: %s\n", SDL_GetError());
			SDL_FreeSurface(graphics_image);
			return(104);
		}
		SDL_FreeSurface(graphics_image);
		graphics_image = convertedSurface;
	}

	/* Ensure the BMP isn't empty or too small */
	if (graphics_image->w < graphics_tile_wid || graphics_image->h < graphics_tile_hgt) {
		snprintf(use_graphics_errstr, sizeof(use_graphics_errstr), "Invalid image dimensions (width x height): %dx%d", graphics_image->w, graphics_image->h);
		SDL_FreeSurface(graphics_image);
		graphics_image = NULL;
		return(105);
	}

	/* Calculate tiles per row. */
	graphics_image_tpr = graphics_image->w / graphics_tile_wid;
	if (graphics_image_tpr <= 0) { /* Paranoia. */
		snprintf(use_graphics_errstr, sizeof(use_graphics_errstr), "Invalid image tiles per row count: %d", graphics_image_tpr);
		SDL_FreeSurface(graphics_image);
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
		const bool same_graphics_dir = sdl2_paths_same(ANGBAND_USER_DIR_XTRA_GRAPHICS, ANGBAND_DIR_XTRA_GRAPHICS);
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
				graphic_subtiles[i] = FALSE;
				continue;
			}
		}

		/* Normalize format so blitting logic can assume RGBA32 everywhere. */
		if (img->format->format != SDL_PIXELFORMAT_RGBA32) {
			SDL_Surface* convertedSurface = SDL_ConvertSurfaceFormat(img, SDL_PIXELFORMAT_RGBA32, 0);
			if (!convertedSurface) {
				snprintf(use_graphics_errstr, sizeof(use_graphics_errstr), "Error converting subimage into RGBA32 format: %s\n", SDL_GetError());
				SDL_FreeSurface(img);
				logprint(format("%s\n", use_graphics_errstr));
				graphic_subtiles[i] = FALSE;
				continue;
			}
			SDL_FreeSurface(img);
			img = convertedSurface;
		}

		/* Bail out on malformed files to avoid divide-by-zero and layout bugs. */
		if (img->w < graphics_tile_wid || img->h < graphics_tile_hgt) {
			snprintf(use_graphics_errstr, sizeof(use_graphics_errstr), "Invalid subimage dimensions (width x height): %dx%d", img->w, img->h);
			logprint(format("%s\n", use_graphics_errstr));
			SDL_FreeSurface(img);
			graphic_subtiles[i] = FALSE;
			continue;
		}

		graphics_image_tpr_sub[i] = img->w / graphics_tile_wid;
		if (graphics_image_tpr_sub[i] <= 0) {
			snprintf(use_graphics_errstr, sizeof(use_graphics_errstr), "Invalid subimage tiles per row count: %d", graphics_image_tpr_sub[i]);
			logprint(format("%s\n", use_graphics_errstr));
			SDL_FreeSurface(img);
			graphic_subtiles[i] = FALSE;
			continue;
		}

		/* Keep a successfully loaded subset around for rendering. */
		graphics_image_sub[i] = img;
	}

	/* Flag to recreate term surfaces with the newly loaded graphics. */
	graphics_reinitialize = true;

	return(0);
}

static void nuke_graphics_sdl2(void) {
		for (int i = 0; i < ANGBAND_TERM_MAX; i++) {
			term_data *td = term_idx_to_term_data(i);
			if (!td) continue;
 #ifdef GRAPHICS_BG_MASK
			td->t.pict_hook_2mask = NULL;
 #endif
			td->t.pict_hook = NULL;
			td->t.rawpict_hook = NULL;
			td->t.higher_pict = FALSE;
			free_graphics(td);
		}
		if (graphics_image) {
			SDL_FreeSurface(graphics_image);
			graphics_image = NULL;
		}
		for (int s = 0; s < MAX_SUBFONTS; s++) {
			if (graphics_image_sub[s]) {
				SDL_FreeSurface(graphics_image_sub[s]);
				graphics_image_sub[s] = NULL;
			}
			graphics_image_tpr_sub[s] = 0;
		}
		graphics_image_tpr = 0;
		graphics_image_mpt = 0;
		graphics_image_tpc = 0;
		graphics_reinitialize = false;
}

static void reset_graphics_masks_sdl2(void) {
	WIPE(graphics_image_masks_colors_prefs, graphics_image_masks_colors_prefs);
	WIPE(graphics_image_masks_colors_sub_prefs, graphics_image_masks_colors_sub_prefs);
	WIPE(graphics_image_masks_colors, graphics_image_masks_colors);
	WIPE(graphics_image_masks_colors_sub, graphics_image_masks_colors_sub);
	WIPE(graphics_image_sub_prefs_has_masks, graphics_image_sub_prefs_has_masks);
	WIPE(graphics_image_sub_prefs_has_outline, graphics_image_sub_prefs_has_outline);
	graphics_image_prefs_has_masks = FALSE;
	graphics_image_prefs_has_outline = FALSE;
}

bool sdl2_set_graphics_mode(byte mode) {
	if (mode == use_graphics) return(TRUE);

	byte previous = use_graphics;
	use_graphics = mode;

	if (previous != UG_NONE) nuke_graphics_sdl2();

	if (use_graphics == UG_NONE) return(TRUE);

	reset_graphics_masks_sdl2();

	use_graphics_err = init_graphics_sdl2();
	if (use_graphics_err != 0) {
		use_graphics = UG_NONE;
		return(FALSE);
	}

	return(TRUE);
}

bool sdl2_reload_graphics_tileset(void) {
	if (use_graphics == UG_NONE) return(TRUE);

	nuke_graphics_sdl2();

	reset_graphics_masks_sdl2();

	use_graphics_err = init_graphics_sdl2();
	if (use_graphics_err != 0) {
		return(FALSE);
	}

	return(TRUE);
}
#endif
/*
 * Initialization function for an "SDL2" module to Angband
 */
errr init_sdl2(void) {
	SDL_DisplayMode dm;
	SDL_PixelFormat* pixel_format;
	bool dpy_color;
	int i;

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "ERROR: init_sdl2: Video SDL_Init error: %s\n", SDL_GetError());
		return(-1);
	}
	/* Ensure SDL shuts down on exit */
	atexit(SDL_Quit);
	SDL_StartTextInput();

	if (TTF_Init() != 0) {
		fprintf(stderr, "ERROR: init_sdl2: TTF_Init error: %s\n", TTF_GetError());
		SDL_Quit();
		return(-1);
	}

#ifdef SDL2_IMAGE
	if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
		fprintf(stderr, "ERROR: init_sdl2: IMG_Init error: %s\n", IMG_GetError());
		TTF_Quit();
		SDL_Quit();
		return(-1);
	}
#endif

	/* Initialize SDL_net */
	if (SDLNet_Init() < 0) {
		fprintf(stderr, "ERROR: init_sdl2: SDLNet_Init error: %s\n", SDLNet_GetError());
		SDL_Quit();
		return 1;
	}

	if (SDL_GetCurrentDisplayMode(0, &dm) != 0) {
		fprintf(stderr, "ERROR: init_sdl2: SDL_GetCurrentDisplayMode failed: %s", SDL_GetError());
		TTF_Quit();
		SDL_Quit();
		return(-1);
	}

	pixel_format = SDL_AllocFormat(dm.format);
	if (pixel_format) {
		dpy_color = ((pixel_format->BitsPerPixel > 1) ? 1 : 0);
		SDL_FreeFormat(pixel_format);
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
	resize_main_window = resize_main_window_sdl2;

	enable_common_colormap_sdl2();

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
		use_graphics_err = init_graphics_sdl2();
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
			if (sdl2_term_init(i) != 0) {
				fprintf(stderr, "Error initializing SDL2 terminal window with index %d\n", i);
			}
		}
	}
	if (sdl2_term_init(0) != 0) {
		fprintf(stderr, "Error initializing SDL2 main terminal window\n");
		/* Can't run without main screen. */
		return(1);
	}


	/* Activate the "Angband" main window screen. */
	Term_activate(&term_main.t);

	/* Raise the "Angband" main window. */
	Infowin_set(term_main.win);
	Infowin_set_focus();
	Infowin_raise();

	/* Success */
	return(0);
}

static void term_force_font(int term_idx, cptr fnt_name);

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
		strcpy(font_name, SDL2_DEFAULT_FONT);
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
			int size = sdl2_terms_ttf_size_default[i] + s*2;
			if (size > SDL2_MAX_TTF_FONT_SIZE) size = SDL2_MAX_TTF_FONT_SIZE;
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
	if (Infofnt_init(fnt_name)) {
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

		resize_window_sdl2(term_idx, td->t.wid, td->t.hgt);

#ifdef USE_GRAPHICS
		/* Resize graphic tiles if needed too. */
		if (use_graphics) {
			/* Recreate new tiles layers and initialize surfaces. */
			if (term_data_init_graphics(td) || td->tiles_layers == NULL || td->tilePreparation == NULL) {
				quit_fmt("Couldn't prepare images after font resize in terminal %d\n", term_idx);
			}
		}
#endif
	}
	SDL_UpdateWindowSurface(td->win->window);

	/* Reload custom font prefs on main screen font change */
	if (td == &term_main) handle_process_font_file();
}

/* Used for checking window position on mapping and saving window positions on quitting.
   Returns ret_x, ret_y containing window coordinates relative to display window. */
void terminal_window_real_coords_sdl2(int term_idx, int *ret_x, int *ret_y) {
	term_data *td = term_idx_to_term_data(term_idx);

	/* non-visible window has no window info .. */
	if (!term_get_visibility(term_idx)) {
		*ret_x = *ret_y = 0;
		return;
	}

	/* Get the window position relative to the display */
	SDL_GetWindowPosition(td->win->window, ret_x, ret_y);
}

/* Resizes the graphics terminal window described by 'td' to dimensions given in 'cols', 'rows' inputs.
 * Stops the resize timer and validates input.
 * Resizes the SDL2 window if current dimensions don't match the validated dimensions.
 * The terminal stored in 'td->t' is resized to desired size if needed.
 * When the window is the main window, update the screen globals, handle bigmap and notify server if in game.
 */
void resize_window_sdl2(int term_idx, int cols, int rows) {
	/* The 'term_idx_to_term_data()' returns '&term_main' if 'term_idx' is out of bounds and it is not desired to resize term_main terminal window in that case, so validate before. */
	if (term_idx < 0 || term_idx >= ANGBAND_TERM_MAX) return;
	term_data *td = term_idx_to_term_data(term_idx);

	/* Clear timer. */
	td->resize_timer = 0;

	/* Validate input dimensions. */
	/* Our 'term_data' indexes in 'term_idx' are the same as 'ang_term' indexes so it's safe to use 'validate_term_dimensions()'. */
	bool rounding_down = validate_term_dimensions(term_idx, &cols, &rows);
	/* Are we actually enlarging the window? */
	if (td == &term_main && rounding_down && screen_hgt == SCREEN_HGT) rows = MAX_SCREEN_HGT + SCREEN_PAD_Y;

	/* Calculate dimensions in pixels. */
	int wid_draw = cols * td->fnt->wid;
	int hgt_draw = rows * td->fnt->hgt;
	int wid_border = wid_draw + (2 * SDL2_DEFAULT_BORDER_WIDTH);
	int hgt_border = hgt_draw + (2 * SDL2_DEFAULT_BORDER_WIDTH);

	/* Save current Infowin and activated Term. */
	infowin *iwin = Infowin;
	term *t = Term;

	/* Activate Infowin and Term from term data belonging to term_idx. */
	Infowin_set(td->win);
	Term_activate(&td->t);

	/* Correct the size of the window if new calculated dimensions differ. */
	if (Infowin->wb != wid_border || Infowin->hb != hgt_border) {
		/* If window is maximized and the dimensions are not as calculated, after we try to correct the size, window manager will again maximize the window again. That will result in a resize loop. To disrupt the loop, don't correct the size of window, if window is maximized. */
		uint32_t windowFlags = SDL_GetWindowFlags(td->win->window);
		if ((windowFlags & SDL_WINDOW_MAXIMIZED) == false) {
			Infowin_resize(wid_border, hgt_border);
		}
	}

	/* Calculate drawing box ad border dimensions. */
	Infowin->wd = wid_draw;
	Infowin->hd = hgt_draw;
	Infowin->bw = (Infowin->wb - Infowin->wd + 1)/2;
	Infowin->bh = (Infowin->hb - Infowin->hd + 1)/2;

	/* Window was resized, surface needs to be regenerated too. */
	Infowin->surface = SDL_GetWindowSurface(Infowin->window);

	/* Make the changes go live (triggers on next c_message_add() call). No need to check if dimensions differ, Term_resize handles it. */
	Term_resize(cols, rows);

	if (!in_game) {
		Infowin_wipe();
		Term_win_update(1, 0, 0);
		Term_redraw();
	}

	/* Restore saved Infowin and Tterm. */
	Infowin_set(iwin);
	Term_activate(t);

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
					c_cfg.big_map = FALSE;
					Client_setup.options[CO_BIGMAP] = FALSE;
				} else if (!Client_setup.options[CO_BIGMAP] && rows != DEFAULT_TERM_HGT) {
					c_cfg.big_map = TRUE;
					Client_setup.options[CO_BIGMAP] = TRUE;
				}
#else
				if (global_c_cfg_big_map && rows == DEFAULT_TERM_HGT) {
					global_c_cfg_big_map = FALSE;
				} else if (!global_c_cfg_big_map && rows != DEFAULT_TERM_HGT) {
					global_c_cfg_big_map = TRUE;
				}
#endif
				/* Notify server and ask for a redraw. */
				Send_screen_dimensions();
			}
		}
	}

	/* Ask server for a redraw if in game. */
	if (in_game) cmd_redraw();
}

/* Resizes main terminal window to dimensions in input. */
/* Used for OS-specific resize_main_window() hook. */
void resize_main_window_sdl2(int cols, int rows) {
	resize_window_sdl2(0, cols, rows);
}

bool ask_for_bigmap(void) {
	return(ask_for_bigmap_generic());
}

const char* get_font_name(int term_idx) {
	term_data *td = term_idx_to_term_data(term_idx);

	if (td->fnt) return(td->fnt->name);
	if (strlen(term_prefs[term_idx].font)) return(term_prefs[term_idx].font);
	return(sdl2_terms_font_default[term_idx]);
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
	errr err = sdl2_term_init(term_idx);
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
	SDL_bool bordered = sdl2_window_decorations ? SDL_TRUE : SDL_FALSE;
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
	write_mangrc(TRUE, TRUE, FALSE);
}

/* Palette animation - 2018 *testing* */
void animate_palette(void) {
	byte i;
	byte rv, gv, bv;
	unsigned long code;
	char cn[8], tmp[3];

	static bool init = FALSE;
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
			gfx_palanim_repaint_hack = FALSE;
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

void set_window_title_sdl2(int term_idx, cptr title) {
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

errr sdl2_win_term_main_screenshot(cptr name) {
	if (!term_main.win || !term_main.win->surface) return 1;

	/* Ensure latest content is displayed */
	SDL_UpdateWindowSurface(term_main.win->window);

	SDL_Surface *surface = term_main.win->surface;
	SDL_Surface *conv = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
	if (!conv) return 1;

#ifdef SDL2_IMAGE
	if (IMG_SavePNG(conv, name) != 0) {
		SDL_Log("IMG_SavePNG failed: %s", IMG_GetError());
#else
	if (SDL_SaveBMP(conv, name) != 0) {
		SDL_Log("SDL_SaveBMP failed: %s", SDL_GetError());
#endif
		SDL_FreeSurface(conv);
		return 1;
	}

	SDL_FreeSurface(conv);
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
static PCF_Font* PCF_OpenFont(const char *name) {
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
	font->bitmap = SDL_CreateRGBSurfaceWithFormat(0, totalWidth, totalHeight, 32, SDL_PIXELFORMAT_RGBA32);
	if(!font->bitmap) {
		fprintf(stderr, "Error: SDL_CreateRGBSurfaceWithFormat failed.\n");
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
	fontBitmapPitch = font->bitmap->pitch / font->bitmap->format->BytesPerPixel;
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

	/* The bitmap contains letters in solid white on black transparent background. Set blend mode and color key now for PCF_RenderText(). */
	SDL_SetSurfaceBlendMode(font->bitmap, SDL_BLENDMODE_NONE);
	SDL_SetColorKey(font->bitmap, SDL_TRUE, SDL_MapRGBA(font->bitmap->format, 255, 255, 255, 0));

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
		SDL_FreeSurface(font->bitmap);
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
	surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
	if (!surface) {
		fprintf(stderr, "PCF_RenderText: Error creating resulting surface: %s\n", TTF_GetError());
		return NULL;
	}
	/* Fill the returning surface with background color. */
	SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, bg_color.r, bg_color.g, bg_color.b, bg_color.a));

	/* If background and foreground colors are the same, there is no need for font rendering. */
	if (fg_color.r == bg_color.r && fg_color.g == bg_color.g && fg_color.b == bg_color.b) {
			return surface;
	}

	/* Create a surface for font preparation. */
	preparation = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
	if (!preparation) {
		fprintf(stderr, "PCF_RenderText: Error creating preparation surface: %s\n", TTF_GetError());
		SDL_FreeSurface(surface);
		return NULL;
	}
	/* Fill the preparation surface with foreground color. */
	SDL_FillRect(preparation, NULL, SDL_MapRGBA(preparation->format, fg_color.r, fg_color.g, fg_color.b, fg_color.a));

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

	SDL_FreeSurface(preparation);
	return surface;
}
#endif /* USE_SDL2 */
