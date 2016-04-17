#ifndef MD5_H
#define MD5_H

#include <stdint.h>

/*  The following tests optimise behaviour on little-endian
    machines, where there is no need to reverse the byte order
    of 32 bit words in the MD5 computation.  By default,
    HIGHFIRST is defined, which indicates we're running on a
    big-endian (most significant byte first) machine, on which
    the byteReverse function in md5.c must be invoked. However,
    byteReverse is coded in such a way that it is an identity
    function when run on a little-endian machine, so calling it
    on such a platform causes no harm apart from wasting time. 
    If the platform is known to be little-endian, we speed
    things up by undefining HIGHFIRST, which defines
    byteReverse as a null macro.  Doing things in this manner
    insures we work on new platforms regardless of their byte
    order.  */

#define HIGHFIRST

#if defined(__i386__) || defined(__x86_64__) || defined(__amd64__)
#undef HIGHFIRST
#endif

/*  On machines where "long" is 64 bits, we need to declare
    uint32 as something guaranteed to be 32 bits.  */

typedef uint32_t MD5_uint32;

struct MD5Context {
	MD5_uint32 buf[4];
	MD5_uint32 bits[2];
	union {
		unsigned char in[64];
		MD5_uint32 in32[16];
	};
};

typedef struct MD5Context MD5_CTX;

extern void MD5Init(MD5_CTX *ctx);
extern void MD5Update(MD5_CTX *ctx, const unsigned char *buf, unsigned len);
extern void MD5Final(unsigned char digest[16], MD5_CTX *ctx);

#endif /* !MD5_H */
