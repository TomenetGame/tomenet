/* $Id$ */
/* File: externs.h */

/* Purpose: extern declarations (variables and functions) */

/*
 * Note that some files have their own header files
 * (z-virt.h, z-util.h, z-form.h, term.h, random.h)
 */


/*
 * Not-so-Automatically generated "variable" declarations
 */


/* variable.c */
extern sockbuf_t ibuf;

extern char server_name[80];
extern char pass[40];
extern bool server_shutdown;
extern bool force_cui;

extern term *ang_term[8];
extern u32b window_flag[8];

extern cptr ANGBAND_SYS;

extern s16b command_cmd;
extern s16b command_dir;

extern cptr ANGBAND_DIR;
extern cptr ANGBAND_DIR_SCPT;
extern cptr ANGBAND_DIR_TEXT;
extern cptr ANGBAND_DIR_USER;
extern cptr ANGBAND_DIR_XTRA;

extern s32b cfg_console_port;

/*
 * Not-so-Automatically generated "function declarations"
 */


/* c-cmd.c */
extern bool process_command(void);
extern void process_reply(void);

/* c-init.c */
extern void console_init(void);

/* c-util.c */
extern void move_cursor(int row, int col);
extern void flush(void);
extern char inkey(void);
extern void bell(void);
extern void c_prt(byte attr, cptr str, int row, int col);
extern void prt(cptr str, int row, int col);
extern bool get_string(cptr prompt, char *buf, int len);
extern bool get_com(cptr prompt, char *command);
extern void request_command(bool shopping);
extern bool get_dir(int *dp);
extern void c_put_str(byte attr, cptr str, int row, int col);
extern void put_str(cptr str, int row, int col);
extern bool get_check(cptr prompt);
extern s16b message_num(void);
extern cptr message_str(s16b age);
extern void c_message_add(cptr msg);
extern void c_msg_print(cptr msg);
extern s16b c_get_quantity(cptr prompt, int max);
extern errr path_build(char *buf, int max, cptr path, cptr file);
extern bool askfor_aux(char *buf, int len, char private);
extern void clear_from(int row);
extern void prt_num(cptr header, int num, int row, int col, byte color);
extern void prt_lnum(cptr header, s32b num, int row, int col, byte color);

/* common/common.c */
extern cptr longVersion;
extern cptr shortVersion;
extern void version_build(void);


/*
 * Hack -- conditional (or "bizarre") externs
 */

#ifdef SET_UID
/* util.c */
extern void user_name(char *buf, int id);
#endif

#ifndef HAS_MEMSET
/* util.c */
extern char *memset(char*, int, huge);
#endif

#ifndef HAS_STRICMP
/* util.c */
extern int stricmp(cptr a, cptr b);
#endif

#ifdef MACINTOSH
/* main-mac.c */
/* extern void main(void); */
#endif

#ifdef WINDOWS
/* main-win.c */
/* extern int FAR PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, ...); */
#endif
