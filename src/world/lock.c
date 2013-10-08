#include <stdlib.h>

#include "world.h"

struct objlock *artml = NULL;
struct objlock *monml = NULL;	/* not yet */

void attempt_lock(struct client *ccl, uint16_t type, uint32_t ttl, uint32_t oval){
//	struct objlock *c_lock;
	if (type != LT_ARTIFACT) return;
}

void attempt_unlock(struct client *ccl, uint16_t type, uint32_t ttl, uint32_t oval){
	if (type != LT_ARTIFACT) return;
}
