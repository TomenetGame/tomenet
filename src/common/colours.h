
/* Purpose: Define colour schemes and unify colour usage easily. - C. Blue */

/* AFK messages / status change / indicator [was o] */
#define COLOUR_AFK		'o'

/* new character mode colours */
#define COLOUR_MODE_PVP		'y'

/* Anti-magic interruption messages */
#if 0
#define COLOUR_AM_OWN          'w'
#define COLOUR_AM_PLY          'o'
#define COLOUR_AM_MON          'o'
#define COLOUR_AM_GOOD         'y'
#define COLOUR_AM_NEAR         'w'
#endif
#if 1 /* alternative scheme (makes more sense) */
#define COLOUR_AM_OWN          'w'
#define COLOUR_AM_PLY          's'
#define COLOUR_AM_MON          's'
#define COLOUR_AM_GOOD         'W'
#define COLOUR_AM_NEAR         'w'
#endif

/* Interception/interference messages */
#if 0
#define COLOUR_IC_PLY          'o'     /* <- currently unused (!) */
#define COLOUR_IC_MON          'o'
#define COLOUR_IC_GOOD         'y'
#define COLOUR_IC_NEAR         'w'
#endif
#if 1 /* alternative scheme (makes less sense but looks better?) */
#define COLOUR_IC_PLY          'y'     /* <- currently unused (!) */
#define COLOUR_IC_MON          'y'
#define COLOUR_IC_GOOD         'W'
#define COLOUR_IC_NEAR         'w'
#endif

/* Magic feedback messages */
#define COLOUR_MD_FAIL		'y'	/* Magic devices */
#define COLOUR_MIMIC_FAIL	'y'	/* Mimic spells */
//#define COLOUR_SPELL_FAIL	'y'	/* Normal spells -- currently hardcoded in s_aux.lua instead */

/* Dodging messages */
#define COLOUR_DODGE_MON       'y'     /* <- currently unused (!) */
#define COLOUR_DODGE_PLY       'y'
#define COLOUR_DODGE_GOOD      'W'
#define COLOUR_DODGE_NEAR      'w'

/* Shield-block messages */
#define COLOUR_BLOCK_PLY       'y'
#define COLOUR_BLOCK_MON       'y'
#define COLOUR_BLOCK_GOOD      'W'
#define COLOUR_BLOCK_NEAR      'w'

/* Parry messags */
#define COLOUR_PARRY_PLY       'y'
#define COLOUR_PARRY_MON       'y'
#define COLOUR_PARRY_GOOD      'W'
#define COLOUR_PARRY_NEAR      'w'

/* Misc event messages */
#define COLOUR_DUNGEON		'u' /* enter/leave dungeon or tower */
#define COLOUR_SERVER		'U' /* enter/leave the server */
#define COLOUR_GAMBLE		's' /* throwing dice, flipping coins, dealing cards */

/* Chat modes (server-side) */
#define COLOUR_CHAT		'B'
#define COLOUR_CHAT_PARTY	'G'
#define COLOUR_CHAT_GUILD	'U'
#define COLOUR_CHAT_LEVEL	'y'
/* .. and client-side */
#define C_COLOUR_CHAT		TERM_L_BLUE
#define C_COLOUR_CHAT_PARTY	TERM_L_GREEN
#define C_COLOUR_CHAT_GUILD	TERM_L_UMBER
#define C_COLOUR_CHAT_LEVEL	TERM_YELLOW


