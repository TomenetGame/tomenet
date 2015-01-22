/* $Id$ */
#include "angband.h"

/* Console global variables */

sockbuf_t ibuf;

char server_name[80] = "undefined";
char pass[40] = "undefined";
bool server_shutdown = FALSE;
bool force_cui = FALSE;

term *ang_term[8];
u32b window_flag[8];

/* hack - evileye (common files) */
bool screen_icky = FALSE;

cptr ANGBAND_SYS;

s16b command_cmd;
s16b command_dir;

cptr ANGBAND_DIR;
cptr ANGBAND_DIR_SCPT;
cptr ANGBAND_DIR_TEXT;
cptr ANGBAND_DIR_USER;
cptr ANGBAND_DIR_XTRA;

s32b cfg_console_port = 18349;
