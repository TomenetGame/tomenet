/* account flags */
#define ACC_TRIAL 0x0001	/* Account is awaiting validation */
#define ACC_ADMIN 0x0002	/* Account members are admins */
#define ACC_MULTI 0x0004	/* Simultaneous play */
#define ACC_NOSCORE 0x0008	/* No scoring allowed */
#define ACC_RESTRICTED 0x0010   /* is restricted (ie after cheezing) */
#define ACC_VRESTRICTED 0x0020  /* is restricted (ie after cheating) */
#define ACC_PRIVILEGED 0x0040   /* has privileged powers (ie for running quests) */
#define ACC_VPRIVILEGED 0x0080  /* has privileged powers (ie for running quests) */
#define ACC_PVP 0x0100          /* may kill other players */
#define ACC_NOPVP 0x0200        /* is not able to kill other players */
#define ACC_ANOPVP 0x0400       /* cannot kill other players; gets punished on trying */
#define ACC_0800 0x800          /* */
#define ACC_QUIET 0x1000        /* may not chat or emote in public */
#define ACC_VQUIET 0x2000       /* may not chat or emote, be it public or private */
#define ACC_BANNED 0x4000       /* account is temporarily suspended */
#define ACC_DELD  0x8000	/* Delete account/members */

/*
 * new account struct - pass in player_type will be removed
 * this will provide a better account management system
 */
struct account{
#if 0
	/* sorry evileye, I needed it to have this run -jir- */
	u_int32_t  id;	/* account id */
	u_int16_t flags;	/* account flags */
#else
	unsigned int id;	/* account id */
	unsigned short flags;	/* account flags */
#endif	/* 0 */
	char name[30];	/* login */
	char pass[20];	/* some crypts are not 13 */
};
