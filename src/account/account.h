
/* account flags */
#define ACC_TRIAL	0x00000001	/* Account is awaiting validation */
#define ACC_ADMIN	0x00000002	/* Account members are admins */
#define ACC_MULTI	0x00000004	/* Simultaneous play */
#define ACC_NOSCORE	0x00000008	/* No scoring allowed */
#define ACC_RESTRICTED	0x00000010   /* is restricted (ie after cheezing) */
#define ACC_VRESTRICTED	0x00000020  /* is restricted (ie after cheating) */
#define ACC_PRIVILEGED	0x00000040   /* has privileged powers (ie for running quests) */
#define ACC_VPRIVILEGED	0x00000080  /* has privileged powers (ie for running quests) */
#define ACC_PVP		0x00000100          /* may kill other players */
#define ACC_NOPVP	0x00000200        /* is not able to kill other players */
#define ACC_ANOPVP	0x00000400       /* cannot kill other players; gets punished on trying */
#define ACC_GREETED	0x00000800          /* */
#define ACC_QUIET	0x00001000        /* may not chat or emote in public */
#define ACC_VQUIET	0x00002000       /* may not chat or emote, be it public or private */
#define ACC_BANNED	0x00004000       /* account is temporarily suspended */
#define ACC_DELD	0x00008000	/* Delete account/members */
#define ACC_WARN_REST	0x80000000	/* Received a one-time warning about resting */

/*
 * new account struct - pass in player_type will be removed
 * this will provide a better account management system
 */
struct account{
#if 0
	/* sorry evileye, I needed it to have this run -jir- */
	u_int32_t id;	/* account id */
	u_int16_t flags;	/* account flags */
#else
	unsigned int id;	/* account id */
	unsigned int flags;	/* account flags */
#endif	/* 0 */
	char name[30];	/* login */
	char pass[20];	/* some crypts are not 13 */
	/* new additions - C. Blue */
	time_t acc_laston;	/* last time this account logged on (for expiry check) */
	signed int cheeze;	/* value in gold of cheezed goods or money */
	signed int cheeze_self; /* value in gold of cheezed goods or money to own characters */
};
