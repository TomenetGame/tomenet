
/* account flags */
#define ACC_TRIAL 0x0001	/* Account is awaiting validation */
#define ACC_ADMIN 0x0002	/* Account members are admins */
#define ACC_MULTI 0x0004	/* Simultaneous play */
#define ACC_NOSCORE 0x0008	/* No scoring allowed */
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
