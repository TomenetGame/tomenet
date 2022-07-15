/* File: h-type.h */

#ifndef INCLUDED_H_TYPE_H
#define INCLUDED_H_TYPE_H

/*
 * Basic "types".
 *
 * Note the attempt to make all basic types have 4 letters.
 * This improves readibility and standardizes the code.
 *
 * Likewise, all complex types are at least 4 letters.
 * Thus, almost every three letter word is a legal variable.
 * But beware of certain reserved words ('for' and 'if' and 'do').
 *
 * Note that the type used in structures for bit flags should be uint.
 * As long as these bit flags are sequential, they will be space smart.
 *
 * Note that on some machines, apparently "signed char" is illegal.
 *
 * It must be true that char/byte takes exactly 1 byte
 * It must be true that sind/uind takes exactly 2 bytes
 * It must be true that sbig/ubig takes exactly 4 bytes
 *
 * On Sparc's, a sint takes 4 bytes (2 is legal)
 * On Sparc's, a uint takes 4 bytes (2 is legal)
 * On Sparc's, a long takes 4 bytes (8 is legal)
 * On Sparc's, a huge takes 4 bytes (8 is legal)
 * On Sparc's, a vptr takes 4 bytes (8 is legal)
 * On Sparc's, a real takes 8 bytes (4 is legal)
 *
 * Note that some files have already been included by "h-include.h"
 * These include <stdio.h> and <sys/types>, which define some types
 * In particular, uint is defined so we do not have to define it
 *
 * Also, see <limits.h> for min/max values for sind, uind, long, huge
 * (SHRT_MIN, SHRT_MAX, USHRT_MAX, LONG_MIN, LONG_MAX, ULONG_MAX)
 * These limits should be verified and coded into "h-constant.h".
 */



/*** Special 4 letter names for some standard types ***/


/* A standard pointer (to "void" because ANSI C says so) */
typedef void *vptr;

/* A simple pointer (to unmodifiable strings) */
typedef const char *cptr;


/* Since float's are silly, hard code real numbers as doubles */
typedef double real;


/* Error codes for function return values */
/* Success = 0, Failure = -N, Problem = +N */
typedef int errr;


/*
 * Hack -- prevent problems with non-MACINTOSH
 */
#undef uint
#define uint uint_hack

/*
 * Hack -- prevent problems with MSDOS and WINDOWS
 */
#undef huge
#define huge huge_hack

/*
 * Hack -- prevent problems with AMIGA
 */
#undef byte
#define byte byte_hack

/*
 * Hack -- prevent problems with C++
 */
#if 0 /* C99 _Bool */
 #undef bool
 #define bool bool_hack
#endif


/* Note that "signed char" is not always "defined" */
/* So always use "s16b" to hold small signed values */
/* A signed byte of memory */
/* typedef signed char syte; */

/* Note that unsigned values can cause math problems */
/* An unsigned byte of memory */
typedef unsigned char byte;

/* Note that a bool is smaller than a full "int" */
/* Simple True/False type */
//typedef char bool; /* C99 _Bool */


/* A signed, standard integer (at least 2 bytes) */
typedef int sint;

/* An unsigned, "standard" integer (often pre-defined) */
typedef unsigned int uint;


/* The largest possible signed integer (pre-defined) */
/* typedef long long; */

/* The largest possible unsigned integer */
typedef unsigned long huge;


/* Signed/Unsigned 16 bit value */
typedef signed short s16b;
typedef unsigned short u16b;

/* inttypes.h was added in the C99 standard. It includes stdint.h. */
#include <inttypes.h>

/* Signed/Unsigned 32 bit value */
typedef int32_t s32b;
typedef uint32_t u32b;

/* Signed/Unsigned 64 bit value */
typedef int64_t s64b;
typedef uint64_t u64b;

/* An int big enough to store a pointer. */
typedef uintptr_t uintptr;



/*** Pointers to all the basic types defined above ***/

typedef real *real_ptr;
typedef errr *errr_ptr;
typedef char *char_ptr;
typedef byte *byte_ptr;
typedef bool *bool_ptr; /* C99 _Bool */
typedef sint *sint_ptr;
typedef uint *uint_ptr;
typedef long *long_ptr;
typedef huge *huge_ptr;
typedef s16b *s16b_ptr;
typedef u16b *u16b_ptr;
typedef s32b *s32b_ptr;
typedef u32b *u32b_ptr;
typedef vptr *vptr_ptr;
typedef cptr *cptr_ptr;



/*** Pointers to Functions with simple return types and any args ***/

typedef void	(*func_void)();
typedef errr	(*func_errr)();
typedef char	(*func_char)();
typedef byte	(*func_byte)();
typedef bool	(*func_bool)();
typedef sint	(*func_sint)();
typedef uint	(*func_uint)();
typedef real	(*func_real)();
typedef vptr	(*func_vptr)();
typedef cptr	(*func_cptr)();



/*** Pointers to Functions of special types (for various purposes) ***/

/* A generic function takes a user data and a special data */
typedef errr	(*func_gen)(vptr, vptr);

/* An equality testing function takes two things to compare (bool) */
typedef bool	(*func_eql)(vptr, vptr);

/* A comparison function takes two things and to compare (-1,0,+1) */
typedef sint	(*func_cmp)(vptr, vptr);

/* A hasher takes a thing (and a max hash size) to hash (0 to siz - 1) */
typedef uint	(*func_hsh)(vptr, uint);

/* A key extractor takes a thing and returns (a pointer to) some key */
typedef vptr	(*func_key)(vptr);


/* Placeholder used to hold the value for visual world character printed on client. */
typedef u32b char32_t;

#endif

