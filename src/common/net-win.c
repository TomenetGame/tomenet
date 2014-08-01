/*
 * This file was taken from the net-unix.c file instead of the
 * general "fill-in-the-blanks" network module.
 *
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

#include "angband.h"

#if 0 //unusued
#ifndef lint
static char sourceid[] =
    "@(#)$Id$";
#endif
#endif

#ifdef TERMNET
/* support for running clients over term, but not servers please. */
#include "termnet.h"
#endif

/* _SOCKLIB_LIBSOURCE must be defined int this file */
#define _SOCKLIB_LIBSOURCE

/* Include files */
#include <winsock.h>	/* includes netinet/in.h info */
#include <io.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
/* may need to find a replacement for netdb.h */
/* #include <netdb.h> */
#include <signal.h>
#ifndef CYGWIN
#define SIGALRM	293		/* not listed in win's signal.h -GP */
#endif
#include <setjmp.h>
#include <errno.h>

/* This should be complied by a std c compiler */
/*
#ifdef _cplusplus
#ifndef __STDC__
#define __STDC__	1
#endif
#endif
*/

/* Socklib Includes And Definitions */
#include "version.h"
#include "net-win.h"

char socklib_version[] = VERSION;

/* Default timeout value of socklib_timeout */
#define DEFAULT_S_TIMEOUT_VALUE		10
#define DEFAULT_US_TIMEOUT_VALUE	0

/* Default retry value of sl_default_retries */
#define DEFAULT_RETRIES			5

/* Environment buffer for setjmp and longjmp */
static			jmp_buf env;

/* Global socklib errno variable */
int			sl_errno = 0;
#if !defined(CYGWIN) && !defined(MINGW)
int			errno = 0;
#endif

/* Global timeout variable. May be modified by users */
int			sl_timeout_s = DEFAULT_S_TIMEOUT_VALUE;
int			sl_timeout_us = DEFAULT_US_TIMEOUT_VALUE;

/* Global default retries variable used by DgramSendRec */
int			sl_default_retries = DEFAULT_RETRIES;

/* Global variable containing the last address from DgramReceiveAny */
struct sockaddr_in	sl_dgram_lastaddr;

/* Global broadcast enable variable (super-user only), default disabled */
int			sl_broadcast_enabled = 0;


/*
 *******************************************************************************
 *
 *	SetTimeout()
 *
 *******************************************************************************
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
void
SetTimeout(int s, int us)
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
int
GetPortNum(int fd)
{
    int			len;
    struct sockaddr_in	addr;

    len = sizeof(struct sockaddr_in);
    if (getsockname(fd, (struct sockaddr *)&addr, &len) == SOCKET_ERROR)
	return (-1);

    return (ntohs(addr.sin_port));
} /* GetPortNum */
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
int
SetSocketReceiveBufferSize(int fd, int size)
{
    return (setsockopt(fd, SOL_SOCKET, SO_RCVBUF,
		       (void *)&size, sizeof(size)));
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
int
SetSocketSendBufferSize(int fd, int size)
{
    return (setsockopt(fd, SOL_SOCKET, SO_SNDBUF,
		       (void *)&size, sizeof(size)));
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
int
SetSocketNonBlocking(int fd, int flag)
{
/*
 * There are some problems on some particular systems (suns) with
 * getting sockets to be non-blocking.  Just try all possible ways
 * until one of them succeeds.  Please keep us informed by e-mail
 * to xpilot@cs.uit.no.
 */

#ifndef USE_FCNTL_O_NONBLOCK
# ifndef USE_FCNTL_O_NDELAY
#  ifndef USE_FCNTL_FNDELAY
#   ifndef USE_IOCTL_FIONBIO

#    if defined(_SEQUENT_) || defined(__svr4__) || defined(SVR4)
#     define USE_FCNTL_O_NDELAY
#    elif defined(__sun__) && defined(FNDELAY)
#     define USE_FCNTL_FNDELAY
#    elif defined(FIONBIO)
#     define USE_IOCTL_FIONBIO
#    elif defined(FNDELAY)
#     define USE_FCNTL_FNDELAY
#    elif defined(O_NONBLOCK)
#     define USE_FCNTL_O_NONBLOCK
#    else
#     define USE_FCNTL_O_NDELAY
#    endif

#    if 0
#     if defined(FNDELAY) && defined(F_SETFL)
#      define USE_FCNTL_FNDELAY
#     endif
#     if defined(O_NONBLOCK) && defined(F_SETFL)
#      define USE_FCNTL_O_NONBLOCK
#     endif
#     if defined(FIONBIO)
#      define USE_IOCTL_FIONBIO
#     endif
#     if defined(O_NDELAY) && defined(F_SETFL)
#      define USE_FCNTL_O_NDELAY
#     endif
#    endif

#   endif
#  endif
# endif
#endif

    char buf[128];

#ifdef USE_FCNTL_FNDELAY
    if (fcntl(fd, F_SETFL, (flag != 0) ? FNDELAY : 0) != -1)
	return 0;
    sprintf(buf, "fcntl FNDELAY failed in socklib.c line %d", __LINE__);
    perror(buf);
#endif

#ifdef USE_IOCTL_FIONBIO
    u_long flag_long = flag;
    if (ioctlsocket(fd, FIONBIO, &flag_long) != SOCKET_ERROR)
	return 0;
    sprintf(buf, "ioctl FIONBIO failed in socklib.c line %d", __LINE__);
    perror(buf);
#endif

#ifdef USE_FCNTL_O_NONBLOCK
    if (fcntl(fd, F_SETFL, (flag != 0) ? O_NONBLOCK : 0) != -1)
	return 0;
    sprintf(buf, "fcntl O_NONBLOCK failed in socklib.c line %d", __LINE__);
    perror(buf);
#endif

#ifdef USE_FCNTL_O_NDELAY
    if (fcntl(fd, F_SETFL, (flag != 0) ? O_NDELAY : 0) != -1)
	return 0;
    sprintf(buf, "fcntl O_NDELAY failed in socklib.c line %d", __LINE__);
    perror(buf);
#endif

    return (-1);
} /* SetSocketNonBlocking */

/*
 *******************************************************************************
 *
 *	SetSocketBroadcast()
 *
 *******************************************************************************
 * Description
 *	Enable broadcasting on a datagram socket.
 *
 * Input Parameters
 *	fd		- The stream socket descriptor to operate on.
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
 *	setsockopt
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Bert G sbers
 */
int
SetSocketBroadcast(int fd, int flag)
{
    return setsockopt(fd, SOL_SOCKET, SO_BROADCAST,
		      (void *)&flag, sizeof(flag));
} /* SetSocketBroadcast */


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
int
GetSocketError(int fd)
{
    int	error, size;

    size = sizeof(error);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR,
	(char *)&error, &size) == -1) {
	return -1;
    }
    errno = error;
    return 0;
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
 *
 */
int
SocketReadable(int fd)
{
    fd_set		readfds;
    struct timeval	timeout;

    timerclear(&timeout); /* macro function */
    timeout.tv_sec = sl_timeout_s;
    timeout.tv_usec = sl_timeout_us;

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    if (select(fd, &readfds, NULL, NULL, &timeout) == SOCKET_ERROR)
	return ((errno == EINTR) ? 0 : -1);
/* I am not getting a readable socket. */
    if (FD_ISSET(fd, &readfds))  
	return (1);
    return (0);
} /* SocketReadable */

/*
 *******************************************************************************
 *
 *	inthandler()
 *
 *******************************************************************************
 * Description
 *	Library routine used to jump to a previous state.
 *
 * Input Parameters
 *	None
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	None
 *
 * Globals Referenced
 *	env
 *
 * External Calls
 *	longjmp
 *
 * Called By
 *	SocketRead
 *
 * Originally coded by Arne Helme
 */
static void inthandler(int signum)
{
    (void) longjmp(env, 1);
} /* inthandler */

/*
 *******************************************************************************
 *
 *	SocketClose()
 *
 *******************************************************************************
 * Description
 *	performs a gracefule shutdown and close on a TCP/IP socket. May
 * 	cause errounous behaviour when used on the same connection from
 *	more than one process.
 *
 * Input Parameters
 *	fd		- The socket to be closed.
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	-1 if any errors occured, else 1.
 *
 * External Calls
 *	shutdown
 *	closesocket		was close for BSD
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Arne Helme
 */
int
SocketClose(int fd)
{
	/*whether there was an error or not, we close the socket */
    (void)shutdown(fd, 2);

    if (closesocket(fd) == -1)
    {
		return (-1);
    }
    return (1);
} /* SocketClose */
/*
 *******************************************************************************
 *
 *	CleanupDgramSocket()
 *
 *******************************************************************************
 * Description
 * Cleanup all the network buffers and close the datagram socket.
 * Also try to send the server a quit packet if possible.
 * Because this quit packet may get lost we send one at the
 * beginning and one at the end.
 *
 * Input Parameters
 *	sock		- The socket to close.
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	None
 *
 * Globals Referenced
 *  None
 *
 * External Calls
 *	DgramWrite
 *	GetSocketError
 *	DgramClose
 *  Sleep (winbase.h - kernel32.lib)
 *
 * Called By
 *	User applications.
 *
 */
void CleanupDgramSocket(int sock)
{
	char	ch;

	if (sock > 2)
	{
		ch = PKT_QUIT;
		if (DgramWrite(sock, &ch, 1) != 1)
		{
			GetSocketError(sock);
			DgramWrite(sock, &ch, 1);
		}
		Sleep(1000);
	}

	if (sock > 2)
	{
		ch = PKT_QUIT;
		if (DgramWrite(sock, &ch, 1) != 1)
		{
			GetSocketError(sock);
			DgramWrite(sock, &ch, 1);
		}
		Sleep(1000);
		if (DgramWrite(sock, &ch, 1) != 1)
		{
			GetSocketError(sock);
			DgramWrite(sock, &ch, 1);
		}
		DgramClose(sock);
	}
}

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
 *	closesocket				was close in BSD
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Arne Helme
 */
int
CreateDgramSocket(int port)
{
    struct sockaddr_in	addr_in;
    int			fd;
    int			retval;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == INVALID_SOCKET)
    {
	sl_errno = SL_ESOCKET;
	return (-1);
    }

    memset((char *)&addr_in, 0, sizeof(struct sockaddr_in));
    addr_in.sin_family		= AF_INET;
    addr_in.sin_addr.s_addr	= INADDR_ANY;
    addr_in.sin_port		= htons(port);
    retval = bind(fd, (struct sockaddr *)&addr_in, sizeof(struct sockaddr_in));
    if (retval == SOCKET_ERROR)
    {
	sl_errno = SL_EBIND;
	retval = errno;
	(void) closesocket(fd);
	errno = retval;
	return (-1);
    }

    return (fd);
} /* CreateDgramSocket */

/*
 *******************************************************************************
 *
 *	CreateDgramAddrSocket()
 *
 *******************************************************************************
 * Description
 *	Creates a UDP/IP datagram socket on a know interface address.
 *
 * Input Parameters
 *	dotaddr		- Pointer to string containing of IP address in dot-format.
 *	port		- The port number. A value of zero may be specified in
 *			  clients to assign any available port number.
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	A UDP/IP datagram socket descriptor, or -1 on error.
 *
 * Globals Referenced
 *	sl_errno	- If any errors occured: SL_ESOCKET, SL_EBIND.
 *
 * External Calls
 *	socket
 *	memset
 *	bind
 *	closesocket			-was close in BSD
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Bert G jsbers, adapted from CreateDgramSocket().
 */
int
CreateDgramAddrSocket(char *dotaddr, int port)
{
    struct sockaddr_in	addr_in;
    int			fd;
    int			retval;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == INVALID_SOCKET)
    {
	sl_errno = SL_ESOCKET;
	return (-1);
    }

    memset((char *)&addr_in, 0, sizeof(struct sockaddr_in));
    addr_in.sin_family		= AF_INET;
    addr_in.sin_addr.s_addr	= inet_addr(dotaddr);
    addr_in.sin_port		= htons(port);
    retval = bind(fd, (struct sockaddr *)&addr_in, sizeof(struct sockaddr_in));
    if (retval == SOCKET_ERROR)
    {
	sl_errno = SL_EBIND;
	retval = errno;
	(void) closesocket(fd);
	errno = retval;
	return (-1);
    }

    return (fd);
} /* CreateDgramAddrSocket */

/*
 *******************************************************************************
 *
 *	DgramBind()
 *
 *******************************************************************************
 * Description
 *	Bind a UDP/IP datagram socket to a local interface address.
 *
 * Input Parameters
 *	fd		- Filedescriptor of existing datagram socket.
 *	dotaddr		- Pointer to string containing of IP address in dot-format.
 *	port		- The port number. A value of zero may be specified in
 *			  clients to assign any available port number.
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	The UDP/IP datagram socket descriptor, or -1 on error.
 *
 * Globals Referenced
 *	sl_errno	- If any errors occured: SL_ESOCKET, SL_EBIND.
 *
 * External Calls
 *	memset
 *	bind
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Bert G jsbers, adapted from CreateDgramAddrSocket().
 */
int
DgramBind(int fd, char *dotaddr, int port)
{
    struct sockaddr_in	addr_in;
    int			retval;

    memset((char *)&addr_in, 0, sizeof(struct sockaddr_in));
    addr_in.sin_family		= AF_INET;
    addr_in.sin_addr.s_addr	= inet_addr(dotaddr);
    addr_in.sin_port		= htons(port);
    retval = bind(fd, (struct sockaddr *)&addr_in, sizeof(struct sockaddr_in));
    if (retval == SOCKET_ERROR)
    {
	sl_errno = SL_EBIND;
	return (-1);
    }

    return (fd);
} /* DgramBind */

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
int
DgramConnect(int fd, char *host, int port)
{
    struct sockaddr_in	addr_in;
    struct hostent	*hp;
    int			retval;

    memset((char *)&addr_in, 0, sizeof(addr_in));
    addr_in.sin_addr.s_addr 	= inet_addr(host);
    if (addr_in.sin_addr.s_addr == (unsigned long)-1)
    {
	hp = gethostbyname(host);
	if (hp == NULL)
	{
	    sl_errno = SL_EHOSTNAME;
	    return (-1);
	}
	else
	    addr_in.sin_addr.s_addr =
		((struct in_addr*)(hp->h_addr))->s_addr;
    }
    addr_in.sin_family		= AF_INET;
    addr_in.sin_port		= htons(port);
    retval = connect(fd, (struct sockaddr *)&addr_in, sizeof(addr_in));
    if (retval == SOCKET_ERROR)
    {
	sl_errno = SL_ECONNECT;
	return (-1);
    }

    return (0);
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
int
DgramSend(int fd, char *host, int port,
	  char *sbuf, int size)
{
    int			retval;
    struct sockaddr_in	the_addr;
    struct hostent	*hp;

    sl_errno = 0;
    (void) memset((char *)&the_addr, 0, sizeof(struct sockaddr_in));
    the_addr.sin_family		= AF_INET;
    the_addr.sin_port		= htons(port);
    if (sl_broadcast_enabled)
	the_addr.sin_addr.s_addr	= INADDR_BROADCAST;
    else
    {
	the_addr.sin_addr.s_addr 	= inet_addr(host);
	if (the_addr.sin_addr.s_addr == (int)-1)
	{
	    hp = gethostbyname(host);
	    if (hp == NULL)
	    {
		sl_errno = SL_EHOSTNAME;
		return (-1);
	    }
	    else
		the_addr.sin_addr.s_addr =
		    ((struct in_addr*)(hp->h_addr))->s_addr;
	}
    }

    retval = sendto(fd, sbuf, size, 0, (struct sockaddr *)&the_addr,
		   sizeof(struct sockaddr_in));

    return retval;
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
int
DgramReceiveAny(int fd, char *rbuf, int size)
{
    int		retval;
    int		addrlen = sizeof(struct sockaddr_in);

    (void) memset((char *)&sl_dgram_lastaddr, 0, addrlen);

    retval = recvfrom(fd, rbuf, size, 0, (struct sockaddr *)&sl_dgram_lastaddr,
	&addrlen);

    return retval;
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
int
DgramReceive(int fd, char *from, char *rbuf, int size)
{
    struct sockaddr_in	tmp_addr;
    struct hostent	*hp;
    int			retval;

    tmp_addr.sin_addr.s_addr = inet_addr(from);
    if (tmp_addr.sin_addr.s_addr == (int)-1)
    {
	hp = gethostbyname(from);
	if (hp == NULL)
	{
	    sl_errno = SL_EHOSTNAME;
	    return (-1);
	}
	else
	    tmp_addr.sin_addr.s_addr =
		((struct in_addr*)(hp->h_addr))->s_addr;
    }
    retval = DgramReceiveAny(fd, rbuf, size);
    if (retval == -1 ||
	tmp_addr.sin_addr.s_addr != sl_dgram_lastaddr.sin_addr.s_addr)
    {
	sl_errno = SL_EWRONGHOST;
	return (-1);
    }
    return (retval);
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
int
DgramReply(int fd, char *sbuf, int size)
{
    int			retval;


    retval = sendto(fd, sbuf, size, 0, (struct sockaddr *)&sl_dgram_lastaddr,
		   sizeof(struct sockaddr_in));

    return retval;
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
 *	errno	holds any error value.
 *
 * External Calls
 *	recv()
 *
 * Called By
 *	User applications
 *
 * Originally coded by Bert Gijsbers
 */
int
DgramRead(int fd, char *rbuf, int size)
{
    int		retval;


    retval = recv(fd, rbuf, size, 0);
	/* if necessary, setup errno for calling function to check */
	if (retval == SOCKET_ERROR){
		errno = WSAGetLastError();
		return -1;
	}

    return retval;
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
 *	errno	for returning an error value
 *
 * External Calls
 *	send()
 *
 * Called By
 *	User applications
 *
 * Originally coded by Bert Gijsbers
 */
int
DgramWrite(int fd, char *wbuf, int size)
{
    int		retval;

    retval = send(fd, wbuf, size, 0);

	/* if necessary, set errno */
	if (retval == SOCKET_ERROR){
		errno = WSAGetLastError();
		return -1;
	}

    return retval;
} /* DgramWrite */

/*
 *******************************************************************************
 *
 *	DgramInthandler()
 *
 *******************************************************************************
 * Description
 *	Library routine used by DgramSendRec to handle alarm interrupts.
 *
 * Input Parameters
 *	None
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	None
 *
 * Globals Referenced
 *	None
 *
 * External Calls
 *	signal
 *
 * Called By
 *	DgramSendRec
 *
 * Originally coded by Arne Helme
 */
static void
DgramInthandler(int signum)
{
    (void) signal(SIGALRM, DgramInthandler);
} /* DgramInthandler */

/*
 *******************************************************************************
 *
 *	DgramSendRec()
 *
 *******************************************************************************
 * Description
 *	Sends a message to a specified host and receives a reply from the
 *	same host. Messages arriving from other hosts when this routine is
 *	called will be discarded. Timeouts and retries can be modified
 * 	by setting the global variables sl_timeout and sl_default_retries.
 *
 * Input Parameters
 *	fd		- The socket descriptor.
 *	host		- Pointer to string contaning a hostname.
 *	port		- The specified port.
 *	sbuf		- Pointer to buffer containing message to be sent.
 *	sbuf_size	- The size of the outgoing message.
 *	rbuf_size	- Expected size of incoming message.
 *
 * Output Parameters
 *	rbuf		- Pointer to message buffer.
 *
 * Return Value
 *	The number of bytes received from the specified host or -1 if any
 *	errors occured.
 *
 * Globals Referenced
 *	errno
 *	sl_errno
 *	sl_timeout
 *	sl_default_retries
 *
 * External Calls
 
 *	signal
 *	DgramSend
 *	DgramReceive
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Arne Helme
 */
int
DgramSendRec(int fd, char *host, int port, char *sbuf,
	     int sbuf_size, char *rbuf, int rbuf_size)
{
    int		retval = -1;
    int		retry = sl_default_retries;

    (void) signal(SIGALRM, DgramInthandler);
    while (retry > 0)
    {
	if (DgramSend(fd, host, port, sbuf, sbuf_size) == -1)
	    return (-1);


	retval = DgramReceive(fd, host, rbuf, rbuf_size);
	if (retval == -1)
	    if (errno == EINTR || sl_errno == SL_EWRONGHOST)
		/* We have a timeout or a message from wrong host */
		if (--retry)
		    continue;	/* Try one more time */
		else
		{
		    sl_errno = SL_ENORESP;
		    break;	/* Unable to get response */
		}
	    else
	    {
		sl_errno = SL_ERECEIVE;
		break;		/* Unable to receive response */
	    }
	else
	    break;		/* Datagram from <host> arrived */
    }

    (void) signal(SIGALRM, SIG_DFL);
    return (retval);
} /* DgramInthandler */

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
char *
DgramLastaddr(int fd)
{
#ifdef UNIX_SOCKETS
    return "localhost";
#else
    int len = sizeof(struct sockaddr_in);
    getpeername(fd, (struct sockaddr*)&sl_dgram_lastaddr, &len);
    return (inet_ntoa(sl_dgram_lastaddr.sin_addr));
#endif
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
char *
DgramLastname(int fd)
{
#ifdef UNIX_SOCKETS
    return "localhost";
#else
    struct hostent	*he;
    char		*str;
    int len = sizeof(struct sockaddr_in);
    getpeername(fd, (struct sockaddr*)&sl_dgram_lastaddr, &len);

    he = gethostbyaddr((char *)&sl_dgram_lastaddr.sin_addr,
		       sizeof(struct in_addr), AF_INET);
    if (he == NULL) {
	str = inet_ntoa(sl_dgram_lastaddr.sin_addr);
    } else {
	str = (char *) he->h_name;
    }
    return str;
#endif
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
int
DgramLastport(int fd)
{
#ifdef UNIX_SOCKETS
    int port;

    if (sscanf(sl_dgram_lastaddr.sun_path, "/tmp/tomenet%d", &port) < 1)
        return (-1);
    return port;
#else
    int len = sizeof(struct sockaddr_in);
    getpeername(fd, (struct sockaddr*)&sl_dgram_lastaddr, &len);
    return (ntohs((int)sl_dgram_lastaddr.sin_port));
#endif
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
 *	closesocket		was close in BSD
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Bert Gijsbers
 */
void
DgramClose(int fd)
{
	int retval;
	/* clean up the socket */
	retval = shutdown(fd, 2);

	if (retval == 0) {
		retval = closesocket(fd);
	}
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
    struct hostent	*he;

    gethostname(name, size);
    if ((he = gethostbyname(name)) == NULL) {
	return;
    }
    strncpy(name, he->h_name, size);
    name[size - 1] = '\0';

} /* GetLocalHostName */



/*
 *******************************************************************************
 *		possible SERVER functions.
 *******************************************************************************
 */
/* #ifdef SERVER */

/*
 *******************************************************************************
 *
 *	CreateServerSocket()
 *
 *******************************************************************************
 * Description
 *	Creates a TCP/IP server socket in the Internet domain.
 *
 * Input Parameters
 *	port		- Server's listen port.
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	The function returns the socket descriptor, or -1 if
 *	any errors occured.
 *
 * Globals Referenced
 *	sl_errno	- if errors occured: SL_ESOCKET, SL_EBIND,
 *			  SL_ELISTEN
 *
 * External Calls
 *	socket
 *	bind
 *	listen
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Arne Helme
 */
int
CreateServerSocket(int port)
{
    struct sockaddr_in	addr_in;
    int			fd;
    int			retval;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == INVALID_SOCKET)
    {
	sl_errno = SL_ESOCKET;
	return (-1);
    }
    memset((char *)&addr_in, 0, sizeof(struct sockaddr_in));
    addr_in.sin_family		= AF_INET;
    addr_in.sin_addr.s_addr	= INADDR_ANY;
    addr_in.sin_port		= htons(port);

    retval = bind(fd, (struct sockaddr *)&addr_in, sizeof(struct sockaddr_in));
    if (retval == SOCKET_ERROR)
    {
	sl_errno = SL_EBIND; /*  WSAGetLastError*/
	(void) closesocket(fd);
	return (-1);
    }

    retval = listen(fd, 5);
    if (retval ==  SOCKET_ERROR)
    {
	sl_errno = SL_ELISTEN;
	(void) closesocket(fd);
	return (-1);
    }

    return (fd);
} /* CreateServerSocket */

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
char *
GetSockAddr(int fd)
{
    int			len;
    struct sockaddr_in	addr;

    len = sizeof(struct sockaddr_in);
    if (getsockname(fd, (struct sockaddr *)&addr, &len) ==  SOCKET_ERROR)
	return (NULL);

    return (inet_ntoa(addr.sin_addr));
} /* GetSockAddr */

/*
 *******************************************************************************
 *
 *	GetPeerName()
 *
 *******************************************************************************
 * Description
 *	Returns the hostname of the peer of connected stream socket.
 *
 * Input Parameters
 *	fd		- The connected stream socket descriptor.
 *	namelen		- Maximum length of the peer name.
 *
 * Output Parameters
 *	The hostname of the peer in a byte array.
 *
 * Return Value
 *	-1 on failure, 0 on success.
 *
 * Globals Referenced
 *	None
 *
 * External Calls
 *	getpeername
 *	gethostbyaddr
 *	inet_ntoa
 *
 * Called By
 *	User applications
 *
 * Originally coded by Bert G sbers
 */
int
SLGetPeerName(int fd, char *name, int namelen)
{
    int			len;
    struct sockaddr_in	addr;
    struct hostent	*hp;

    len = sizeof(struct sockaddr_in);
    if (getpeername(fd, (struct sockaddr *)&addr, &len) == SOCKET_ERROR)
	return (-1);

    hp = gethostbyaddr((char *)&addr.sin_addr.s_addr, 4, AF_INET);
    if (hp != NULL)
    {
	strncpy(name, hp->h_name, namelen);
    }
    else
    {
	strncpy(name, inet_ntoa(addr.sin_addr), namelen);
    }
    name[namelen - 1] = '\0';

    return (0);
} /* GetPeerName */

/*
 *******************************************************************************
 *
 *	CreateClientSocket()
 *
 *******************************************************************************
 * Description
 *	Creates a client TCP/IP socket in the Internet domain.
 *
 * Input Parameters
 *	host		- Pointer to string containing name of the peer
 *			  host on either dot-format or ascii-name format.
 *	port		- The requested port number.
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	Returns the socket descriptor or the error value -1.
 *
 * Globals Referenced
 *	sl_errno	- If errors occured: SL_EHOSTNAME, SL_ESOCKET,
 *			  SL_ECONNECT.
 *
 * External Calls
 *	memset
 *	gethostbyname
 *	socket
 *	connect
 *	closesocket		close in BSD
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Arne Helme
 */
int
CreateClientSocket(char *host, int port)
{
    struct sockaddr_in	peer;
    struct hostent	*hp;
    int			fd;

    memset((char *)&peer, 0, sizeof(struct sockaddr_in));
    peer.sin_family = AF_INET;
    peer.sin_port   = htons(port);

    peer.sin_addr.s_addr = inet_addr(host);
    if (peer.sin_addr.s_addr == (int)-1)
    {
	hp = gethostbyname(host);
	if (hp == NULL)
	{
	    sl_errno = SL_EHOSTNAME;
	    return (-1);
	}
	else
	    peer.sin_addr.s_addr = ((struct in_addr*)(hp->h_addr))->s_addr;
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == INVALID_SOCKET)
    {
	sl_errno = SL_ESOCKET;
	return (-1);
    }

    if (connect(fd, (struct sockaddr *)&peer, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
    {
	sl_errno = SL_ECONNECT;
	(void) closesocket(fd);
	return (-1);
    }

    return (fd);
} /* CreateClientSocket */


/*
 *******************************************************************************
 *
 *	SocketAccept()
 *
 *******************************************************************************
 * Description
 *	This function is called in a TCP/IP server to accept incoming calls.
 *
 * Input Parameters
 *	fd		- The listen socket.
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	The functions returns a new descriptor which is used to the
 *	actual data transfer.
 *
 * Globals Referenced
 *	None
 *
 * External Calls
 *	none
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Arne Helme.
 */
int
SocketAccept(int fd)
{
    int		retval;

    retval = accept(fd, NULL, 0);

    return retval;
} /* SocketAccept */


/*
 *******************************************************************************
 *
 *	SocketLinger()
 *
 *******************************************************************************
 * Description
 *	This function is called on a stream socket to set the linger option.
 *
 * Input Parameters
 *	fd		- The stream socket to set the linger option on.
 *
 * Output Parameters
 *	None
 *
 * Return Value
 *	-1 on failure, 0 on success.
 *
 * Globals Referenced
 *	None
 *
 * External Calls
 *	setsockopt
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Arne Helme, but moved out of SocketAccept by Bert.
 */
int
SocketLinger(int fd)
{
    static struct linger	linger = {1, 300};
    int				lsize  = sizeof(struct linger);

    return setsockopt(fd, SOL_SOCKET, SO_LINGER, (void *)&linger, lsize);
} /* SocketLinger */


/*
 *******************************************************************************
 *
 *	SetSocketNoDelay()
 *
 *******************************************************************************
 * Description
 *	Set the TCP_NODELAY option on a connected stream socket.
 *
 * Input Parameters
 *	fd		- The stream socket descriptor to operate on.
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
 *	setsockopt
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Bert G sbers
 */
#ifdef TCP_NODELAY
int
SetSocketNoDelay(int fd, int flag)
{
    /*
     * The fcntl(O_NDELAY) option has nothing to do
     * with the setsockopt(TCP_NODELAY) option.
     * They control completely different features!
     */
    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
		      (void *)&flag, sizeof(flag));
} /* SetSocketNoDelay */
#endif

/*
 *******************************************************************************
 *
 *	SocketRead()
 *
 *******************************************************************************
 * Description
 *	Receives <size> bytes and put them into buffer <buf> from
 *	socket <fd>.
 *
 * Input Parameters
 *	fd		- The socket descriptor.
 *	size		- The expected amount of data to receive.
 *
 * Output Parameters
 *	buf		- Pointer to a message buffer.
 *
 * Return Value
 *	The number of bytes received or -1 if any errors occured.
 *
 * Globals Referenced
 *	sl_timeout
 *
 * External Calls
 *	setjmp
 
 *	signal
 *	read
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Arne Helme
 */
int
SocketRead(int fd, char *buf, int size)
{
    int	ret, ret1;

    if (setjmp(env))
    {
	(void) signal(SIGALRM, SIG_DFL);
	return (-1);
    }
    ret = 0;

    while (ret < size)
    {
	(void) signal(SIGALRM, inthandler);

	ret1 = recv(fd, &buf[ret], size - ret, 0);

	(void) signal(SIGALRM, SIG_DFL);
	ret += ret1;
	if (ret1 <= 0)
	    return (ret);
    }

    return (ret);
} /* SocketRead */

/*
 *******************************************************************************
 *
 *	SocketWrite()
 *
 *******************************************************************************
 * Description
 *	Writes <size> bytes from buffer <buf> onto socket <fd>.
 *
 * Input Parameters
 *	fd		- The socket descriptor.
 *	buf		- Pointer to a send buffer.
 *	size		- The amount of data to send.
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
 *	write
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Arne Helme
 */
int
SocketWrite(int fd, char *buf, int size)
{
    int		retval;


    /*
     * A SIGPIPE exception may occur if the peer entity has disconnected.
     */
    retval = write(fd, buf, size);


    return retval;
} /* SocketWrite */

/*#endif */	/*SERVER function? */
