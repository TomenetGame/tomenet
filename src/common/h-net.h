/* This file includes all the needed networking stuff */

/* Include the socket buffer library */
#include "sockbuf.h"

/* Include the socket library for the correct OS */
#ifdef MSDOS
 #include "net-ibm.h"
#elif defined(WIN32)
 #include "net-win.h"
#elif defined(USE_SDL3)
 #include "net-sdl3.h"
#else
 #include "net-unix.h"
#endif 

/* Include the various packet types and error codes */
#include "pack.h"

/* Include some bit-manipulation functions used in the networking code */
#include "bit.h"
