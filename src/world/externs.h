extern struct serverinfo slist[];
extern int snum;
struct list *clist;

extern void initrand();
extern int createsocket(int port, uint32_t ip);
extern void world(int ser);
extern void l_account(struct wpacket *wpk, struct client *ccl);
extern int16_t pwcheck(char *cpasswd, uint32_t val);
extern void send_sinfo(struct client *ccl, struct client *priv);
extern void send_rplay(struct client *ccl);
extern void add_rplayer(struct wpacket *wpk);
extern void attempt_lock(struct client *ccl, uint16_t type, uint32_t ttl, uint32_t oval);
extern void attempt_unlock(struct client *ccl, uint16_t type, uint32_t ttl, uint32_t oval);
extern void reply(struct wpacket *wpk, struct client *ccl);
extern void reply(struct wpacket *wpk, struct client *ccl);
extern void initauth(struct client *ccl);
extern void rem_players(int16_t id);
extern void relay(struct wpacket *wpk, struct client *talker);
