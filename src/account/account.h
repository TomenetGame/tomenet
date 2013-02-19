#include <stdint.h>

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

#define u32b uint32_t
#define s32b int32_t

struct account{
	u32b id;        /* account id */
	u32b flags;     /* account flags */
	char name[30];  /* login */
	char pass[20];  /* some crypts are not 13 */
	time_t acc_laston;      /* last time this account logged on (for expiry check) */
	s32b cheeze;    /* value in gold of cheezed goods or money */
	s32b cheeze_self; /* value in gold of cheezed goods or money to own characters */
	char deed_event;        /* receive a deed for a global event participation? */
	char deed_achievement;  /* receive a deed for a (currently PvP) achievement? */
	s32b guild_id;  /* auto-rejoin its guild after a char perma-died */
};
