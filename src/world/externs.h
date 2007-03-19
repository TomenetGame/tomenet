extern struct serverinfo slist[];
extern int snum;
struct list *clist;

extern void initrand();
extern int createsocket(int port, unsigned long ip);
extern void world(int ser);
extern void l_account(struct wpacket *wpk, struct client *ccl);
extern short pwcheck(char *cpasswd, unsigned long val);
extern void send_sinfo(struct client *ccl, struct client *priv);
extern void send_rplay(struct client *ccl);
extern void add_rplayer(struct wpacket *wpk);
extern void attempt_lock(struct client *ccl, unsigned short type, unsigned long ttl, unsigned long oval);
extern void attempt_unlock(struct client *ccl, unsigned short type, unsigned long ttl, unsigned long oval);
extern void reply(struct wpacket *wpk, struct client *ccl);
extern void reply(struct wpacket *wpk, struct client *ccl);
extern void initauth(struct client *ccl);
extern void rem_players(short id);
extern void relay(struct wpacket *wpk, struct client *talker);
