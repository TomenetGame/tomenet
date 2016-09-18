/* $Id$ */
/*
 * Packet types
 */

/* Packet types 0-12 are "administrative" */
#define PKT_UNDEFINED		0
#define PKT_VERIFY		1
#define PKT_REPLY		2
#define PKT_PLAY		3
#define PKT_QUIT		4
#define PKT_LEAVE		5
#define PKT_MAGIC		6
#define PKT_RELIABLE		7
#define PKT_ACK			8
#define PKT_TALK		9

#define PKT_START		10
#define PKT_END			11
#define PKT_LOGIN		12
#define PKT_KEEPALIVE		13

/* reserved 14 to 19 for non play purposes - evileye */
#define PKT_FILE		14	/* internal file transfer */


/* Packet types 20-59 are info that is sent to the client */
#define PKT_PLUSSES		20
#define PKT_AC			21
#define PKT_EXPERIENCE		22
#define PKT_GOLD		23
#define PKT_HP			24
#define PKT_SP			25
#define PKT_CHAR_INFO		26
#define PKT_VARIOUS		27
#define PKT_STAT		28
#define PKT_HISTORY		29

#define PKT_INVEN		30
#define PKT_EQUIP		31
#define PKT_TITLE		32
#define PKT_LEVEL		33
#define PKT_DEPTH		34
#define PKT_FOOD		35
#define PKT_BLIND		36
#define PKT_CONFUSED		37
#define PKT_FEAR		38
#define PKT_POISON		39

#define PKT_STATE		40
#define PKT_LINE_INFO		41
#define PKT_SPEED		42
#define PKT_STUDY		43
#define PKT_CUT			44
#define PKT_STUN		45
#define PKT_MESSAGE		46
#define PKT_CHAR		47
#define PKT_SPELL_INFO		48
#define PKT_FLOOR		49

#define PKT_SPECIAL_OTHER	50
#define PKT_STORE		51
#define PKT_STORE_INFO		52
#define PKT_TARGET_INFO		53
#define PKT_SOUND		54
#define PKT_MINI_MAP		55
#define PKT_PICKUP_CHECK	56
#define PKT_SKILLS		57
#define PKT_PAUSE		58
#define PKT_MONSTER_HEALTH	59


/* Packet types 60-65 are sent from either the client or server */
#define PKT_DIRECTION		60
#define PKT_ITEM		61
#define PKT_SELL		62
#define PKT_PARTY		63
#define PKT_SPECIAL_LINE	64
#define PKT_SKILL_MOD   	65


/* Packet types 67-116 are sent from the client */
#define PKT_TAKE_OFF_AMT	66
#define PKT_MIND		67
#define PKT_STORE_EXAMINE	68
#define PKT_KING		69
#define PKT_WALK		70
#define PKT_RUN			71
#define PKT_TUNNEL		72
#define PKT_AIM_WAND		73
#define PKT_DROP		74
#define PKT_FIRE		75
#define PKT_STAND		76
#define PKT_DESTROY		77
#define PKT_LOOK		78
#define PKT_SPELL		79	/* reuse now for Receive_request_spell/Send_spell */

#define PKT_OPEN		80
#define PKT_PRAY		81
#define PKT_QUAFF		82
#define PKT_READ		83
#define PKT_SEARCH		84
#define PKT_TAKE_OFF		85
#define PKT_USE			86
#define PKT_THROW		87
#define PKT_WIELD		88
#define PKT_ZAP			89

#define PKT_TARGET		90
#define PKT_INSCRIBE		91
#define PKT_UNINSCRIBE		92
#define PKT_ACTIVATE		93
#define PKT_BASH		94
#define PKT_DISARM		95
#define PKT_EAT			96
#define PKT_FILL		97
#define PKT_LOCATE		98
#define PKT_MAP			99

#define PKT_SEARCH_MODE		100
#define PKT_FIGHT		101
#define PKT_CLOSE		103
#define PKT_GAIN		104
#define PKT_GO_UP		105
#define PKT_GO_DOWN		106
#define PKT_PURCHASE		107
#define PKT_STORE_LEAVE		108
#define PKT_STORE_CONFIRM	109

#define PKT_DROP_GOLD		110
#define PKT_REDRAW		111
#define PKT_REST		112
#define PKT_GHOST		113
#define PKT_SUICIDE		114
#define PKT_STEAL		115
#define PKT_OPTIONS		116
#define PKT_TARGET_FRIENDLY	117
#define PKT_MASTER		118 /* dungeon master commands */
#define PKT_AUTOPHASE		119 /* automatically try to phase */
#define PKT_HOUSE		120 /* house admin */

/* Packet types 121-122 are more administrative stuff */
#define PKT_FAILURE		121
#define PKT_SUCCESS		122
#define PKT_CLEAR_BUFFER	123
#define PKT_SCRIPT		124
#define PKT_CLEAR_ACTIONS	125
#define PKT_ZAP_DIR		126
#define PKT_ACTIVATE_DIR	127
#define PKT_SKILL_DEV		128 /* Sent from client->server: collapse/expand subtree of a skill */
#define PKT_BPR			129 /* BpR display replaces Study display */

#define PKT_SANITY		130
#define PKT_SCREEN_DIM		131	/* client's screen dimensions */
#define PKT_GUILD_CFG		132
#define PKT_BONI_COL		133	/* client csheet boni tracking */
#define PKT_MINI_MAP_POS	134
#define PKT_AUTOINSCRIBE	135
#define PKT_MARTYR		136	/* client lua notification client for updating spell mana costs */

//XXX 137-143

//debugging:
#define PKT_CLIENT_SETUP_U	140
#define PKT_CLIENT_SETUP_F	141
#define PKT_CLIENT_SETUP_K	142
#define PKT_CLIENT_SETUP_R	143

#define PKT_CLIENT_SETUP	144	/* Resend F:/R:/K:/U: definitions */
#define PKT_ITEM_NEWEST		145	/* Transmit inventory index of the latest item the character picked up */
#define PKT_CONFIRM		146	/* Hacky: Signal the client to break an ongoing sync_sleep() early since its previous command has finished being processed by the server. */
#define PKT_KEYPRESS		147	/* Quest/hint guidance: actually send keypresses to the client as if the player entered them */

#define PKT_SFX_VOLUME		148	/* Tell client to modify volume level of ambient and/or weather sfx  */
#define PKT_SFX_AMBIENT		149	/* background ambient sfx */

/* Packet types 150- are hacks */
#define PKT_FLUSH		150
#define PKT_OBSERVE		151
#define PKT_SPIKE		152
#define PKT_GUILD		153
#define PKT_SKILL_INIT		154
#define PKT_ACTIVATE_SKILL	155
#define PKT_RAW_KEY		156
#define PKT_CHARDUMP		157
#define PKT_BACT		158
#define PKT_STORE_CMD		159

/* its a cleanup, not a hack */
#define PKT_SKILL_PTS		160

/* Page someone who is afk */
#define PKT_BEEP		161
/* Give the client some info about the server details. - C. Blue
   For example: Is it running 'RPG_SERVER' settings? or 'FUN_SERVER'? */
#define PKT_SERVERDETAILS	162
#define PKT_AFK			163
#define PKT_ENCUMBERMENT	164
#define PKT_PARTY_STATS		165 /* DEG Packet type for party member info */
#define PKT_PING		166 /* Ping packet */
/* 167+ are further below.. */

/* HACK -- used for SKILL_INIT */
#define PKT_SKILL_INIT_NAME     0
#define PKT_SKILL_INIT_DESC     1
#define PKT_SKILL_INIT_MKEY     2

/* Not hack - file transfer packet subtypes */
/* DO NOT TOUCH - work in progress */
#define PKT_FILE_DATA		1
#define PKT_FILE_END		2
#define PKT_FILE_CHECK		3
#define PKT_FILE_ACK		4	/* acknowledge whatever */
#define PKT_FILE_ERR		5	/* failure - close */
#define PKT_FILE_SUM		6	/* checksum reply */
#define	PKT_FILE_INIT		7	/* initiate a transfer */

/* new things for v4.4.1 or 4.4.0d (dual-wield & co) - C. Blue */
#define PKT_SIP			167
#define PKT_TELEKINESIS		168
#define PKT_BBS			169
#define PKT_WIELD2		170
#define PKT_CLOAK		171

#define PKT_STAMINA		172
#define PKT_TECHNIQUE_INFO	173
#define PKT_EXTRA_STATUS	174	/* combat stance, fire mode, etc */
#define PKT_INVEN_WIDE		175	/* similar to PKT_INVEN, but with additional info */
#define PKT_UNIQUE_MONSTER	176	/* for client-side unique kills tracking */
#define PKT_WEATHER		177	/* client-side weather (4.4.2.1) */
#define PKT_INVENTORY_REV	178	/* inventory revision notification (4.4.2.2) */
#define PKT_ACCOUNT_INFO	179	/* account information (4.4.2.3) */
#define PKT_CHANGE_PASSWORD	180	/* password change request (4.4.2.3) */
#define PKT_STORE_WIDE		181	/* for browsing custom books in home/stores */

#define PKT_FORCE_STACK		182	/* force stacking of level 0 items with normal-level items in your inventory */
#define PKT_MUSIC		183	/* background music */

#define PKT_REQUEST_KEY		184	/* (special) gets a keypress from the player */
#define PKT_REQUEST_NUM		185	/* (special) gets a number from the player */
#define PKT_REQUEST_STR		186	/* (special) gets a 1-line string from the player */
#define PKT_REQUEST_CFR		187	/* (special) requests confirmation ('y/n') from player */
#define PKT_REQUEST_ABORT	188	/* (special) abort pending requests (eg if player left store) */
#define PKT_STORE_SPECIAL_STR	189	/* for SPECIAL flag stores (those don't have an inventory) */
#define PKT_STORE_SPECIAL_CHAR	190	/* for SPECIAL flag stores (those don't have an inventory) */
#define PKT_STORE_SPECIAL_CLR	191	/* for SPECIAL flag stores (those don't have an inventory) */

#define PKT_WARNING_BEEP	192
#define PKT_STAND_ONE		193


/*
 * Possible error codes returned
 */
#define SUCCESS		0x00
#define E_VERSION_OLD	0x01
#define E_GAME_FULL	0x02
#define E_NEED_INFO	0x03
#define E_TWO_PLAYERS	0x04
#define E_PASSWORD	0x05
//#define E_		0x06
#define E_LETTER	0x07
#define E_IN_USE	0x08
#define E_SOCKET	0x09
#define E_INVAL		0x0A
#define E_INVITE	0x0B
#define E_BANNED	0x0C
#define E_VERSION_UNKNOWN	0x0D
#ifdef CLIENT_SIDE /* RETRY_LOGIN: */
 #define E_RETRY_CONTACT	0xFF
 #define E_RETRY_LOGIN		0xFE
#endif


/*
 * Some commands send to the server
 */
#define ENTER_GAME_pack	0x00
#define CONTACT_pack	0x31
