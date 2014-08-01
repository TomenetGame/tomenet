/*
 * This is a general "fill-in-the-blanks" network module.
 *
 * It should be possible to adapt this file to any platform that supports
 * networking, without a lot of effort.
 *
 * The first step is to make sure that this file is defined correctly,
 * using the "USE_XXX" defines that should also be used to select which
 * display modules to compile.  Note that compiling in more that one network
 * module will cause compilation errors.
 */

#include "angband.h"


/*
 * The following are some copyright messages about the socket library I
 * borrowed from XPilot, which was in turn borrowed from something else.
 */


/* -*-C-*-
 *
 * Project :	 TRACE
 *
 * File    :	 socklib.c
 *
 * Description
 *
 * Copyright (C) 1991 by Arne Helme, The TRACE project
 *
 * Rights to use this source is granted for all non-commercial and research
 * uses. Creation of derivate forms of this software may be subject to
 * restriction. Please obtain written permission from the author.
 *
 * This software is provided "as is" without any express or implied warranty.
 *
 * RCS:      $Id$
 *
 * Revision 1.1.1.1  1992/05/11  12:32:34  bjoerns
 * XPilot v1.0
 *
 * Revision 1.2  91/10/02  08:38:01  08:38:01  arne (Arne Helme)
 * "ANSI C prototypes added.
 * Timeout interface changed."
 *
 * Revision 1.1  91/10/02  08:34:45  08:34:45  arne (Arne Helme)
 * Initial revision
 *
 */

#if 0 //unusued
#ifndef lint
static char sourceid[] =
    "@(#)$Id$";
#endif
#endif

/* _SOCKLIB_LIBSOURCE must be defined int this file */
#define _SOCKLIB_LIBSOURCE

/* Include files */

/*
 * Include any files that you'll need for your networking stuff.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


/* 
 * Socklib Includes And Definitions
 *
 * Change the value of the second include correctly.
 */

#include "version.h"
#include "net-xxx.h"

char socklib_version[] = VERSION;

/* Debug macro */
#ifdef DEBUG
#define DEB(x) x
#else
#define DEB(x)
#endif

/* Default timeout value of socklib_timeout */
#define DEFAULT_S_TIMEOUT_VALUE		10
#define DEFAULT_US_TIMEOUT_VALUE	0

/* Default retry value of sl_default_retries */
#define DEFAULT_RETRIES			5

/* Global socklib errno variable */
int			sl_errno = 0;

/* Global timeout variable. May be modified by users */
int			sl_timeout_s = DEFAULT_S_TIMEOUT_VALUE;
int			sl_timeout_us = DEFAULT_US_TIMEOUT_VALUE;

/* Global default retries variable used by DgramSendRec */
int			sl_default_retries = DEFAULT_RETRIES;


/* Global broadcast enable variable (super-user only), default disabled */
int			sl_broadcast_enabled = 0;


/*
 *******************************************************************************
 *
 *	SetTimeout()
 *
 *******************************************************************************
 *
 * Description
 *	Sets the global timout value to s + us.
 *
 * Input Parameters
 *	s			- Timeout value in seconds
 *	us			- Timeout value in useconds
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	void
 *
 * Globals Referenced
 *	sl_timeout_us		- Timeout value in useconds
 *	sl_timeout_s		- Timeout value in seconds
 *
 * External Calls
 *	None
 *
 * Called By
 *	User applications
 *
 * Originally coded by Arne Helme
 */
void SetTimeout(int s, int us)
{
    sl_timeout_us = us;
    sl_timeout_s = s;
} /* SetTimeout */


/*
 *******************************************************************************
 *
 *	GetPortNum()
 *
 *******************************************************************************
 * Description
 *	Returns the port number of a socket connection.
 *
 * Input Parameters
 *	fd		- The socket descriptor.
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	The port number on host standard format.
 *
 * Globals Referenced
 *	None
 *
 * External Calls
 *	getsockname
 *
 * Called By
 *	User applications
 *
 * Originally coded by Arne Helme
 */
int GetPortNum(int fd)
{
	/* Both the client and server use this */
} /* GetPortNum */


/*
 *******************************************************************************
 *
 *	GetSockAddr()
 *
 *******************************************************************************
 * Description
 *	Returns the address of a socket.
 *
 * Input Parameters
 *	fd		- The socket descriptor.
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	The address of the socket as a string.
 *	Note that this string is contained in a static area
 *	and must be saved by the caller before the next call
 *	to a socket function.
 *
 * Globals Referenced
 *	Stacic memory in inet_ntoa() containing the string.
 *
 * External Calls
 *	getsockname
 *	inet_ntoa
 *
 * Called By
 *	User applications
 *
 * Originally coded by Bert Gijsbers
 */
char *GetSockAddr(int fd)
{
	/* Both the client and the server use this */
} /* GetSockAddr */


/*
 *******************************************************************************
 *
 *	SetSocketReceiveBufferSize()
 *
 *******************************************************************************
 * Description
 *	Set the receive buffer size for either a stream or a datagram socket.
 *
 * Input Parameters
 *	fd		- The socket descriptor to operate on.
 *	size		- The new buffer size to use by the kernel.
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	-1 on failure, 0 on success
 *
 * Globals Referenced
 *	none
 *
 * External Calls
 *	setsockopt
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Bert G sbers
 */
int SetSocketReceiveBufferSize(int fd, int size)
{
	/* Both the client and the server use this */
} /* SetSocketReceiveBufferSize */


/*
 *******************************************************************************
 *
 *	SetSocketSendBufferSize()
 *
 *******************************************************************************
 * Description
 *	Set the send buffer size for either a stream or a datagram socket.
 *
 * Input Parameters
 *	fd		- The socket descriptor to operate on.
 *	size		- The new buffer size to use by the kernel.
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	-1 on failure, 0 on success
 *
 * Globals Referenced
 *	none
 *
 * External Calls
 *	setsockopt
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Bert G sbers
 */
int SetSocketSendBufferSize(int fd, int size)
{
	/* Both the client and the server use this */
} /* SetSocketSendBufferSize */


/*
 *******************************************************************************
 *
 *	SetSocketNonBlocking()
 *
 *******************************************************************************
 * Description
 *	Set the nonblocking option on a socket.
 *
 * Input Parameters
 *	fd		- The socket descriptor to operate on.
 *	flag		- One to turn it on, zero to turn it off.
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	-1 on failure, 0 on success
 *
 * Globals Referenced
 *	none
 *
 * External Calls
 *	ioctl
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Bert G sbers
 */
int SetSocketNonBlocking(int fd, int flag)
{
	/* Both the client and the server use this */
} /* SetSocketNonBlocking */


/*
 *******************************************************************************
 *
 *	GetSocketError()
 *
 *******************************************************************************
 * Description
 *	Clear the error status for the socket and return the error in errno.
 *
 * Input Parameters
 *	fd		- The socket descriptor to operate on.
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	-1 on failure, 0 on success
 *
 * Globals Referenced
 *	errno
 *
 * External Calls
 *	getsockopt
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Bert G sbers
 */
int GetSocketError(int fd)
{
	/* Both the client and the server use this */
} /* GetSocketError */


/*
 *******************************************************************************
 *
 *	SocketReadable()
 *
 *******************************************************************************
 * Description
 *	Checks if data have arrived on the TCP/IP socket connection.
 *
 * Input Parameters
 *	fd		- The socket descriptor to be checked.
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	TRUE (non-zero) or FALSE (zero) (or -1 if select() fails).
 *
 * Globals Referenced
 *	socket_timeout
 *
 * External Calls
 *	select
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Arne Helme
 */
int SocketReadable(int fd)
{
	/* Both the client and the server use this */
} /* SocketReadable */


/*
 *******************************************************************************
 *
 *	CreateDgramSocket()
 *
 *******************************************************************************
 * Description
 *	Creates a UDP/IP datagram socket in the Internet domain.
 *
 * Input Parameters
 *	port		- The port number. A value of zero may be specified in
 *			  clients to assign any available port number.
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	A UDP/IP datagram socket descriptor.
 *
 * Globals Referenced
 *	sl_errno	- If any errors occured: SL_ESOCKET, SL_EBIND.
 *
 * External Calls
 *	socket
 *	memset
 *	bind
 *	close
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Arne Helme
 */
int CreateDgramSocket(int port)
{
	/* Both the client and the server use this */
} /* CreateDgramSocket */


/*
 *******************************************************************************
 *
 *	DgramConnect()
 *
 *******************************************************************************
 * Description
 *	Associate a datagram socket with a peer.
 *
 * Input Parameters
 *	fd		- The socket to operate on.
 *	host		- The host name.
 *	port		- The port number.
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	-1 on error, 0 on success
 *
 * Globals Referenced
 *	sl_errno	- If any errors occured: SL_EHOSTNAME, SL_ECONNECT.
 *
 * External Calls
 *	connect
 *	gethostbyname
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Bert G sbers
 */
int DgramConnect(int fd, char *host, int port)
{
	/* Both the client and the server use this */
} /* DgramConnect */


/*
 *******************************************************************************
 *
 *	DgramSend()
 *
 *******************************************************************************
 * Description
 *	Transmits a UDP/IP datagram.
 *
 * Input Parameters
 *	fd		- The socket descriptor.
 *	host		- Pointer to string containing destination host name.
 *	port		- Destination port.
 *	sbuf		- Pointer to the message to be sent.
 *	size		- Message size.
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	The number of bytes sent or -1 if any errors occured.
 *
 * Globals Referenced
 *	sl_broadcast_enabled
 *	sl_errno	- If any errors occured: SL_EHOSTNAME.
 *
 * External Calls
 *	memset
 *	inet_addr
 *	gethostbyname
 *	sendto
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Arne Helme
 */
int DgramSend(int fd, char *host, int port, char *sbuf, int size)
{
	/* Both the client and the server use this */
} /* DgramSend */


/*
 *******************************************************************************
 *
 *	DgramReceiveAny()
 *
 *******************************************************************************
 * Description
 *	Receives a datagram from any sender.
 *
 * Input Parameters
 *	fd		- The socket descriptor.
 *	size		- Expected message size.
 *
 * Output Parameters
 *	rbuf		- Pointer to a message buffer.
 *
 * Return Value
 *	The number of bytes received or -1 if any errors occured.
 *
 * Globals Referenced
 *	sl_dgram_lastaddr
 *
 * External Calls
 *	memset
 *
 * Called By
 *	User applications
 *
 * Originally coded by Arne Helme
 */
int DgramReceiveAny(int fd, char *rbuf, int size)
{
	/* The client doesn't use this one, only the server */
} /* DgramReceiveAny */


/*
 *******************************************************************************
 *
 *	DgramReceive()
 *
 *******************************************************************************
 * Description
 *	Receive a datagram from a specifc host. If a message from another
 *	host arrives, an error value is returned.
 *
 * Input Parameters
 *	fd		- The socket descriptor.
 *	from		- Pointer to the specified hostname.
 *	size		- Expected message size.
 *
 * Output Parameters
 *	rbuf		- Pointer to message buffer.
 *
 * Return Value
 *	The number of bytes received or -1 if any errors occured.
 *
 * Globals Referenced
 *	sl_dgram_lastaddr
 *	sl_errno	- If any errors occured: SL_EHOSTNAME, SL_EWRONGHOST.
 *
 * External Calls
 *	inet_addr
 *	gethostbyname
 *	DgramReceiveAny
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Arne Helme
 */
int DgramReceive(int fd, char *from, char *rbuf, int size)
{
	/* The client doesn't use this one, only the server */
} /* DgramReceive */


/*
 *******************************************************************************
 *
 *	DgramReply()
 *
 *******************************************************************************
 * Description
 *	Transmits a UDP/IP datagram to the host/port the most recent datagram
 *	was received from.
 *
 * Input Parameters
 *	fd		- The socket descriptor.
 *	host		- Pointer to string containing destination host name.
 *	size		- Message size.
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	The number of bytes sent or -1 if any errors occured.
 *
 * Globals Referenced
 *	sl_dgram_lastaddr
 *
 * External Calls
 *	sendto
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Bert Gijsbers
 */
int DgramReply(int fd, char *sbuf, int size)
{
	/* The client doesn't use this one, only the server */
} /* DgramReply */


/*
 *******************************************************************************
 *
 *	DgramRead()
 *
 *******************************************************************************
 * Description
 *	Receives a datagram on a connected datagram socket.
 *
 * Input Parameters
 *	fd		- The socket descriptor.
 *	size		- Expected message size.
 *
 * Output Parameters
 *	rbuf		- Pointer to a message buffer.
 *
 * Return Value
 *	The number of bytes received or -1 if any errors occured.
 *
 * Globals Referenced
 *	None
 *
 * External Calls
 *	recv()
 *
 * Called By
 *	User applications
 *
 * Originally coded by Bert Gijsbers
 */
int DgramRead(int fd, char *rbuf, int size)
{
	/* Both the client and the server use this */
} /* DgramRead */


/*
 *******************************************************************************
 *
 *	DgramWrite()
 *
 *******************************************************************************
 * Description
 *	Sends a datagram on a connected datagram socket.
 *
 * Input Parameters
 *	fd		- The socket descriptor.
 *	wbuf		- Pointer to a message buffer.
 *	size		- Expected message size.
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	The number of bytes sent or -1 if any errors occured.
 *
 * Globals Referenced
 *	None
 *
 * External Calls
 *	send()
 *
 * Called By
 *	User applications
 *
 * Originally coded by Bert Gijsbers
 */
int DgramWrite(int fd, char *wbuf, int size)
{
	/* Both the client and the server use this */
} /* DgramWrite */


/*
 *******************************************************************************
 *
 *	DgramLastaddr()
 *
 *******************************************************************************
 * Description
 *	Extracts the last host address from the global variable
 *	sl_dgram_lastaddr.
 *
 * Input Parameters
 *	None
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	Pointer to string containing the host address. Warning, the string
 *	resides in static memory area.
 *
 * Globals Referenced
 *	sl_dgram_lastaddr
 *
 * External Calls
 *	inet_ntoa
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Arne Helme
 */
char *DgramLastaddr(void)
{
	/* The client doesn't use this one, only the server */
} /* DgramLastaddr */


/*
 *******************************************************************************
 *
 *	DgramLastname()
 *
 *******************************************************************************
 * Description
 *	Does a name lookup for the last host address from the
 *	global variable sl_dgram_lastaddr.  If this nameserver
 *	query fails then it resorts to DgramLastaddr().
 *
 * Input Parameters
 *	None
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	Pointer to string containing the hostname. Warning, the string
 *	resides in static memory area.
 *
 * Globals Referenced
 *	sl_dgram_lastaddr
 *
 * External Calls
 *	inet_ntoa
 *	gethostbyaddr
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Bert Gijsbers
 */
char *DgramLastname(void)
{
	/* The client doesn't use this one, only the server */
} /* DgramLastname */


/*
 *******************************************************************************
 *
 *	DgramLastport()
 *
 *******************************************************************************
 * Description
 *	Extracts the last host port from the global variable sl_dgram_lastaddr.
 *
 * Input Parameters
 *	None
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	The last port number on host standard format.
 *
 * Globals Referenced
 *	sl_dgram_lastaddr
 *
 * External Calls
 *	None
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Arne Helme
 */
int DgramLastport(void)
{
	/* The client doesn't use this one, only the server */
} /* DgramLastport */


/*
 *******************************************************************************
 *
 *	DgramClose()
 *
 *******************************************************************************
 * Description
 *	performs a close on a UDP/IP datagram socket.
 *
 * Input Parameters
 *	fd		- The datagram socket to be closed.
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	None.
 *
 * Globals Referenced
 *	None.
 *
 * External Calls
 *	close
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Bert Gijsbers
 */
void DgramClose(int fd)
{
	/* Both the client and the server use this */
} /* DgramClose */


/*
 *******************************************************************************
 *
 *	GetLocalHostName()
 *
 *******************************************************************************
 * Description
 *	Returns the Fully Qualified Domain Name for the local host.
 *
 * Input Parameters
 *	Size of output array.
 *
 * Output Parameters
 *	Array of size bytes to store the hostname.
 *
 * Return Value
 *	None
 *
 * Globals Referenced
 *	None
 *
 * External Calls
 *	gethostbyname
 *	gethostbyaddr
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Bert Gijsbers
 */
void GetLocalHostName(char *name, unsigned size)
{
	/* Both the client and the server use this */
} /* GetLocalHostName */



#endif /* USE_XXX */
