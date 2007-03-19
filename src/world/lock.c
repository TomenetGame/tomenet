#include <stdlib.h>

#include "world.h"

struct objlock *artml=NULL;
struct objlock *monml=NULL;	/* not yet */

void attempt_lock(struct client *ccl, unsigned short type, unsigned long ttl, unsigned long oval){
//	struct objlock *c_lock;
	if(type!=LT_ARTIFACT) return;
}

void attempt_unlock(struct client *ccl, unsigned short type, unsigned long ttl, unsigned long oval){
	if(type!=LT_ARTIFACT) return;
}
