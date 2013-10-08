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

/*
 * IPv6 conversion for TomeNET by evileye. This is experimental, and
 * some stuff has not been tested yet, namely the binding IP stuff.
 */
#include "angband.h"

#ifdef SET_UID

#ifdef TERMNET
/* support for running clients over term, but not servers please. */
#include "termnet.h"
#endif

/* _SOCKLIB_LIBSOURCE must be defined int this file */
#define _SOCKLIB_LIBSOURCE

/* Include files */
#include <sys/param.h>
#include <unistd.h>
#include <sys/types.h>
#ifdef VMS
#include <ucx$inetdef.h>
#else
#include <sys/ioctl.h>
#endif
#if (SVR4)
#include <sys/filio.h>
#endif
#if (_SEQUENT_)
#include <sys/fcntl.h>
#else
#include <fcntl.h>
#endif
#if defined(__hpux)
#include <time.h>
#else
#include <sys/time.h>
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#if defined(__sun__)
#include <arpa/nameser.h>
#include <resolv.h>
#endif

#ifdef UNIX_SOCKETS
#include <sys/un.h>
#endif

#ifdef __cplusplus
#ifndef __STDC__
#define __STDC__	1
#endif
#endif

/* Socklib Includes And Definitions */
#include "version.h"
#include "net-unix.h"
#ifdef SUNCMW
#include "cmw.h"
#else
#define cmw_priv_assert_netaccess() /* empty */
#define cmw_priv_deassert_netaccess() /* empty */
#endif /* SUNCMW */

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

/* Environment buffer for setjmp and longjmp */
static			jmp_buf env;

/* Global socklib errno variable */
int			sl_errno = 0;

/* Global timeout variable. May be modified by users */
int			sl_timeout_s = DEFAULT_S_TIMEOUT_VALUE;
int			sl_timeout_us = DEFAULT_US_TIMEOUT_VALUE;

/* Global default retries variable used by DgramSendRec */
int			sl_default_retries = DEFAULT_RETRIES;

/* Global variable containing the last address from DgramReceiveAny */
#ifdef UNIX_SOCKETS
struct sockaddr_un	sl_dgram_lastaddr;
#else
struct sockaddr_in6	sl_dgram_lastaddr;
#endif

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
#ifdef __STDC__
SetTimeout(int s, int us)
#else
SetTimeout(s, us)
int s, us;
#endif /* __STDC__ */
{
    sl_timeout_us = us;
    sl_timeout_s = s;
} /* SetTimeout */

#ifdef UNIX_SOCKETS
#define MAX_BOUND_SOCKETS 10
static char bound_socket[MAX_BOUND_SOCKETS][80];
static int num_bound_sockets = 0;

void
add_bound_socket(char *path)
{
   strcpy(bound_socket[num_bound_sockets++], path);
}

void
delete_bound_socket(char *path)
{
   register int i;

   for (i = 0; i < num_bound_sockets; i++)
      if (!strcmp(bound_socket[i], path))
         strcpy(bound_socket[i], bound_socket[--num_bound_sockets]);
}

void
SocketCloseAll()
{
   register int i;

   for (i = 0; i < num_bound_sockets; i++)
      unlink(bound_socket[i]);
}
#endif

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
#ifdef __STDC__
CreateServerSocket(int port)
#else
CreateServerSocket(port)
int	port;
#endif /* __STDC__ */
{
    int			fd;
    int			retval;
    int			option = 1;

#ifdef UNIX_SOCKETS     
    struct sockaddr_un  addr_in;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
    {
	sl_errno = SL_ESOCKET;
	return (-1);
    }

    memset((char *)&addr_in, 0, sizeof(addr_in));
    addr_in.sun_family          = AF_UNIX; 
    if (port) {
       sprintf(addr_in.sun_path, "/tmp/tomenet%d", port);
       retval = bind(fd, (struct sockaddr *)&addr_in, sizeof(addr_in));
    } else {
       for (port = getpid(); port > 0; port--) {
          sprintf(addr_in.sun_path, "/tmp/tomenet%d", port);
          retval = bind(fd, (struct sockaddr *)&addr_in, sizeof(addr_in));
          if (!retval)
             break;
       }
    }
    add_bound_socket(addr_in.sun_path);
#else
    struct sockaddr_in6  addr_in;
    memset((char *)&addr_in, 0, sizeof(addr_in));
    addr_in.sin6_family		= AF_INET6;
#ifdef BIND_IP
    inet_pton(AF_INET6, BIND_IP, &addr_in.sin6_addr); /* v6 format.. */
    /* fprintf( stderr, "Server Socket Binding By Request to %s\n",inet_ntoa(addr_in.sin6_addr)); -RLS*/
#else
    addr_in.sin6_addr		= in6addr_any;
#endif
    addr_in.sin6_port		= htons(port);
    fd = socket(AF_INET6, SOCK_STREAM, 0);
    /* Set this so we don't wait forever on startups */
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR , (void*)&option, sizeof(int)); 
    /* Allow binding on IPv4 port if not in use */
    option = 0;
    setsockopt(fd, IPPROTO_IPV6, IPV6_BINDV6ONLY, (void*)&option, sizeof(int)); 
    if (fd < 0)
    {
	sl_errno = SL_ESOCKET;
	return (-1);
    }

    retval = bind(fd, (struct sockaddr *)&addr_in, sizeof(addr_in));
#endif      

    if (retval < 0)
    {
	sl_errno = SL_EBIND;
	/* fprintf( stderr, " Server Socket Bind Error: %d,%d\n",retval,errno); */
	(void) close(fd);
	return (-1);
    }

    retval = listen(fd, 5);
    if (retval < 0)
    {
	sl_errno = SL_ELISTEN;
	(void) close(fd);
	return (-1);
    }

    return (fd);
} /* CreateServerSocket */

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
#ifdef __STDC__
GetPortNum(int fd)
#else
GetPortNum(fd)
int	fd;
#endif /* __STDC__ */
{
    socklen_t		len;
#ifdef UNIX_SOCKETS
    struct sockaddr_un	addr;
    int port;

    len = sizeof(addr);
    if (getsockname(fd, (struct sockaddr *)&addr, &len) < 0)
	return (-1);

    if (sscanf(addr.sun_path, "/tmp/tomenet%d", &port) < 1)
        return (-1);
    return port;
#else
    struct sockaddr_in6	addr;

    len = sizeof(addr);
    if (getsockname(fd, (struct sockaddr *)&addr, &len) < 0)
	return (-1);

    return (ntohs(addr.sin6_port));
#endif
} /* GetPortNum */


#if 0
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
#ifdef __STDC__
GetSockAddr(int fd)
#else
GetSockAddr(fd)
int	fd;
#endif /* __STDC__ */
{
#ifdef UNIX_SOCKETS
    return "localhost";
#else
    socklen_t		len;
    struct sockaddr_in6	addr;
    static char addbuff[INET6_ADDRSTRLEN];

    len = sizeof(struct sockaddr_in6);
    if (getsockname(fd, (struct sockaddr *)&addr, &len) < 0)
	return (NULL);

    return(inet_ntop(AF_INET6, &addr.sin6_addr, &addbuff, INET6_ADDRSTRLEN));
#endif
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
 * Originally coded by Bert Gÿsbers
 */
int
#ifdef __STDC__
SLGetPeerName(int fd, char *name, int namelen)
#else
SLGetPeerName(fd, name, namelen)
int	fd;
char	*name;
int	namelen;
#endif /* __STDC__ */
{
#ifdef UNIX_SOCKETS
    strcpy(name, "localhost");
#else
    socklen_t		len;
    struct sockaddr_in6	addr;
    struct hostent	*hp;

    len = sizeof(struct sockaddr_in6);
    if (getpeername(fd, (struct sockaddr *)&addr, &len) < 0)
	return (-1);

    hp = gethostbyaddr((char *)&addr.sin6_addr.s_addr, 4, AF_INET6);
    if (hp != NULL)
    { 
	strncpy(name, hp->h_name, namelen);
    }
    else
    {
	strncpy(name, inet_ntoa(addr.sin6_addr), namelen);
    }
    name[namelen - 1] = '\0';
#endif

    return (0);
} /* GetPeerName */

#endif /* 0 */

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
 *	close
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Arne Helme
 */
int
#ifdef __STDC__
CreateClientSocket(char *host, int port)
#else
CreateClientSocket(host, port)
char	*host;
int	port;
#endif /* __STDC__ */
{
    int			fd;

#ifdef UNIX_SOCKETS     
    struct sockaddr_un  peer;
    memset((char *)&peer, 0, sizeof(peer));
    peer.sun_family          = AF_UNIX; 
    sprintf(peer.sun_path, "/tmp/tomenet%d", (port)? port : getpid());
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
#else
    struct hostent	*hp;
    struct sockaddr_in6  peer;
    char temp[INET6_ADDRSTRLEN];
    memset((char *)&peer, 0, sizeof(peer));
    peer.sin6_len = sizeof(peer);
    peer.sin6_family = AF_INET6;
    peer.sin6_port   = htons(port);
    if((inet_pton(AF_INET6, host, &peer.sin6_addr))<1){
	hp = gethostbyname2(host, AF_INET6);
	if (hp == NULL)
	{
	    printf("1 No IP6 address for %s\n", host);
	    hp = gethostbyname2(host, AF_INET);
	    if (hp == NULL) {
	   	sl_errno = SL_EHOSTNAME;
	    	return (-1);
	    }
	    else{
		/* I dont like this really */
		*((u_int32_t*)&peer.sin6_addr.s6_addr[8]) = ntohl(0x0000ffff);
	    	*((struct in_addr*)(&peer.sin6_addr.s6_addr[12])) = *((struct in_addr*)(hp->h_addr));

	    }
	}
	else
	    peer.sin6_addr = *((struct in6_addr*)(hp->h_addr));
    }
    fd = socket(AF_INET6, SOCK_STREAM, 0);
#endif      

    if (fd < 0)
    {
	sl_errno = SL_ESOCKET;
	return (-1);
    }

    inet_ntop(AF_INET6, &peer.sin6_addr, &temp, INET6_ADDRSTRLEN);
    printf("Connecting to %s\n", temp);

    if (connect(fd, (struct sockaddr *)&peer, sizeof(peer)) < 0)
    {
	printf("1 connect failed\n");
	sl_errno = SL_ECONNECT;
	(void) close(fd);
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
#ifdef __STDC__
SocketAccept(int fd)
#else
SocketAccept(fd)
int	fd;
#endif /* __STDC__ */
{
    int		retval;
    socklen_t	socklen = sizeof(struct sockaddr_in6);
    char temp[INET6_ADDRSTRLEN];

    cmw_priv_assert_netaccess();
    inet_ntop(AF_INET6, &sl_dgram_lastaddr, &temp, INET6_ADDRSTRLEN);
    printf("socketaccept: %s\n", temp);
    retval = accept(fd, (struct sockaddr*)&sl_dgram_lastaddr, &socklen);
    //retval = accept(fd, NULL, 0);
    cmw_priv_deassert_netaccess();

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
#ifdef __STDC__
SocketLinger(int fd)
#else
SocketLinger(fd)
int	fd;
#endif /* __STDC__ */
{
#if defined(LINUX0) || !defined(SO_LINGER)
    /*
     * As of 0.99.12 Linux doesn't have LINGER stuff.
     */
    return 0;
#else
#ifdef	__hp9000s300
    long			linger = 1;
    int				lsize  = sizeof(long);
#else
    static struct linger	linger = {1, 300};
    int				lsize  = sizeof(struct linger);
#endif
    return setsockopt(fd, SOL_SOCKET, SO_LINGER, (void *)&linger, lsize);
#endif
} /* SocketLinger */

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
 * Originally coded by Bert Gÿsbers
 */
int
#ifdef __STDC__
SetSocketReceiveBufferSize(int fd, int size)
#else
SetSocketReceiveBufferSize(fd, size)
int	fd;
int	size;
#endif /* __STDC__ */
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
 * Originally coded by Bert Gÿsbers
 */
int
#ifdef __STDC__
SetSocketSendBufferSize(int fd, int size)
#else
SetSocketSendBufferSize(fd, size)
int	fd;
int	size;
#endif /* __STDC__ */
{
    return (setsockopt(fd, SOL_SOCKET, SO_SNDBUF,
		       (void *)&size, sizeof(size)));
} /* SetSocketSendBufferSize */


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
 * Originally coded by Bert Gÿsbers
 */
#ifdef TCP_NODELAY
int
#ifdef __STDC__
SetSocketNoDelay(int fd, int flag)
#else
SetSocketNoDelay(fd, flag)
int	fd;
int	flag;
#endif /* __STDC__ */
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
 * Originally coded by Bert Gÿsbers
 */
int
#ifdef __STDC__
SetSocketNonBlocking(int fd, int flag)
#else
SetSocketNonBlocking(fd, flag)
int	fd;
int	flag;
#endif /* __STDC__ */
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
    if (ioctl(fd, FIONBIO, &flag) != -1)
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

#if 0
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
 * Originally coded by Bert Gÿsbers
 */
int
#ifdef __STDC__
SetSocketBroadcast(int fd, int flag)
#else
SetSocketBroadcast(fd, flag)
int	fd;
int	flag;
#endif /* __STDC__ */
{
    return setsockopt(fd, SOL_SOCKET, SO_BROADCAST,
		      (void *)&flag, sizeof(flag));
} /* SetSocketBroadcast */
#endif /* if 0 */


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
 * Originally coded by Bert Gÿsbers
 */
int
#ifdef __STDC__
GetSocketError(int fd)
#else
GetSocketError(fd)
int	fd;
#endif /* __STDC__ */
{
    int	error;
    socklen_t size;

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
 */
int
#ifdef __STDC__
SocketReadable(int fd)
#else
SocketReadable(fd)
int	fd;
#endif /* __STDC__ */
{
    fd_set		readfds;
    struct timeval	timeout;

#ifndef timerclear
#define timerclear(tvp)   (tvp)->tv_sec = (tvp)->tv_usec = 0
#endif
    timerclear(&timeout); /* macro function */
    timeout.tv_sec = sl_timeout_s;
    timeout.tv_usec = sl_timeout_us;

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    if (select(fd + 1, &readfds, NULL, NULL, &timeout) == -1)
	return ((errno == EINTR) ? 0 : -1);

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
#ifdef __STDC__
static void inthandler(int signum)
#else
static inthandler()
#endif /* __STDC__ */
{
    DEB(fprintf(stderr, "Connection interrupted, timeout\n"));
    (void) longjmp(env, 1);
} /* inthandler */


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
 *	alarm
 *	signal
 *	read
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Arne Helme
 */

 /* This function is not compatible with MAngband.  This is probably
  * because it messes up the signals.
  */
int
#ifdef __STDC__
SocketRead(int fd, char *buf, int size)
#else
SocketRead(fd, buf, size)
int	fd, size;
char	*buf;
#endif /* __STDC__ */
{
    int	ret, ret1;

    if (setjmp(env))
    {
	(void) alarm(0);
	(void) signal(SIGALRM, SIG_DFL);
	return (-1);
    }
    ret = 0;
    cmw_priv_assert_netaccess();
    while (ret < size)
    {
	(void) signal(SIGALRM, inthandler);
	(void) alarm(sl_timeout_s);
	ret1 = read(fd, &buf[ret], size - ret);
//	DEB(fprintf(stderr, "Read %d bytes\n", ret1));
	(void) alarm(0);
	(void) signal(SIGALRM, SIG_DFL);
	ret += ret1;
	if (ret1 <= 0)
	    return (ret);
    }
    cmw_priv_deassert_netaccess();
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
#ifdef __STDC__
SocketWrite(int fd, char *buf, int size)
#else
SocketWrite(fd, buf, size)
int	fd, size;
char	*buf;
#endif /* __STDC__ */
{
    int		retval;

    cmw_priv_assert_netaccess();
    /*
     * A SIGPIPE exception may occur if the peer entity has disconnected.
     */
    retval = write(fd, buf, size);
    cmw_priv_deassert_netaccess();

    return retval;
} /* SocketWrite */


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
 * Globals Referenced
 *	sl_errno	- If any errors occured: SL_ESHUTD, SL_ECLOSE.
 *
 * External Calls
 *	shutdown
 *	close
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Arne Helme
 */
int
#ifdef __STDC__
SocketClose(int fd)
#else
SocketClose(fd)
int	fd;
#endif /* __STDC__ */
{
#ifdef UNIX_SOCKETS
    struct sockaddr_un  addr;
    int len = sizeof(addr);
    if (getsockname(fd, (struct sockaddr *)&addr, &len) == 0)
       unlink(addr.sun_path);
    delete_bound_socket(addr.sun_path);
#endif

    if (shutdown(fd, 2) == -1)
    {
	sl_errno = SL_ESHUTD;
	/* return (-1);  ***BG: need close always */
    }

    if (close(fd) == -1)
    {
	sl_errno = SL_ECLOSE;
	return (-1);
    }
    return (1);
} /* SocketClose */

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
int
#ifdef __STDC__
CreateDgramSocket(int port)
#else
CreateDgramSocket(port)
int	port;
#endif /* __STDC__ */
{
    int			fd;
    int			retval;

#ifdef UNIX_SOCKETS
    struct sockaddr_un	addr_in;
    memset((char *)&addr_in, 0, sizeof(addr_in));
    addr_in.sun_family          = AF_UNIX;
    fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0)
    {
	sl_errno = SL_ESOCKET;
	return (-1);
    }

    if (port) {
       sprintf(addr_in.sun_path, "/tmp/tomenet%d", port);
       retval = bind(fd, (struct sockaddr *)&addr_in, sizeof(addr_in));
    } else {
       for (port = getpid(); port > 0; port--) {
          sprintf(addr_in.sun_path, "/tmp/tomenet%d", port);
          retval = bind(fd, (struct sockaddr *)&addr_in, sizeof(addr_in));
          if (!retval)
             break;
       }
    }
    add_bound_socket(addr_in.sun_path);
#else
    struct sockaddr_in6	addr_in;
    memset((char *)&addr_in, 0, sizeof(addr_in));
    addr_in.sin6_family		= AF_INET6;
    addr_in.sin6_addr		= in6addr_any;
#ifdef BIND_IP
    inet_pton(AF_INET6, BIND_IP, &addr_in.sin6_addr);
    /* fprintf( stderr, "Dgram Socket Binding By Request to %s\n",inet_ntoa(addr_in.sin6_addr)); -RLS*/
#else
    addr_in.sin6_addr		= in6addr_any;
#endif
    addr_in.sin6_port		= htons(port);
    fd = socket(AF_INET6, SOCK_DGRAM, 0);

    if (fd < 0)
    {
	sl_errno = SL_ESOCKET;
	return (-1);
    }

    retval = bind(fd, (struct sockaddr *)&addr_in, sizeof(addr_in));
#endif

    if (retval < 0)
    {
	sl_errno = SL_EBIND;
	fprintf( stderr, "Dgram Socket Bind Error: %d,%d\n",retval,errno);
	retval = errno;
	(void) close(fd);
	errno = retval;
	return (-1);
    }

    return (fd);
} /* CreateDgramSocket */

#if 0
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
 *	close
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Bert Gijsbers, adapted from CreateDgramSocket().
 */
int
#ifdef __STDC__
CreateDgramAddrSocket(char *dotaddr, int port)
#else
CreateDgramAddrSocket(dotaddr, port)
char	*dotaddr;
int	port;
#endif /* __STDC__ */
{
    int			fd;
    int			retval;

#ifdef UNIX_SOCKETS     
    struct sockaddr_un  addr_in;
    memset((char *)&addr_in, 0, sizeof(addr_in));
    addr_in.sun_family          = AF_UNIX; 
    fd = socket(AF_UNIX, SOCK_DGRAM, 0);

    if (fd < 0)
    {
	sl_errno = SL_ESOCKET;
	return (-1);
    }

    if (port) {
       sprintf(addr_in.sun_path, "/tmp/tomenet%d", port);
       retval = bind(fd, (struct sockaddr *)&addr_in, sizeof(addr_in));
    } else {
       for (port = getpid(); port > 0; port--) {
          sprintf(addr_in.sun_path, "/tmp/tomenet%d", port);
          retval = bind(fd, (struct sockaddr *)&addr_in, sizeof(addr_in));
          if (!retval)
             break;
       }
    }
    add_bound_socket(addr_in.sun_path);
#else
    struct sockaddr_in6  addr_in;
    memset((char *)&addr_in, 0, sizeof(addr_in));
    addr_in.sin6_family		= AF_INET6;
#ifdef BIND_IP
    inet_pton(AF_INET6, BIND_IP, &addr_in.sin6_addr);
    /* fprintf( stderr, "DgramAddr Binding By Request to %s\n",inet_ntoa(addr_in.sin6_addr)); -RLS*/
#else
    addr_in.sin6_addr.s_addr	= inet_addr(dotaddr);  
#endif
    addr_in.sin6_port		= htons(port);
    fd = socket(AF_INET6, SOCK_DGRAM, 0);

    if (fd < 0)
    {
	sl_errno = SL_ESOCKET;
	return (-1);
    }

    retval = bind(fd, (struct sockaddr *)&addr_in, sizeof(addr_in));
#endif      

    if (retval < 0)
    {
	sl_errno = SL_EBIND;
	retval = errno;
	(void) close(fd);
	errno = retval;
	return (-1);
    }

    return (fd);
} /* CreateDgramAddrSocket */

#endif /* 0 */

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
 * Originally coded by Bert Gijsbers, adapted from CreateDgramAddrSocket().
 */
int
#ifdef __STDC__
DgramBind(int fd, char *dotaddr, int port)
#else
DgramBind(fd, dotaddr, port)
int	fd;
char	*dotaddr;
int	port;
#endif /* __STDC__ */
{
    int			retval;

#ifdef UNIX_SOCKETS     
    struct sockaddr_un  addr_in;
    memset((char *)&addr_in, 0, sizeof(addr_in));
    addr_in.sun_family          = AF_UNIX; 

    if (port) {
       sprintf(addr_in.sun_path, "/tmp/tomenet%d", port);
       retval = bind(fd, (struct sockaddr *)&addr_in, sizeof(addr_in));
    } else {
       for (port = getpid(); port > 0; port--) {
          sprintf(addr_in.sun_path, "/tmp/tomenet%d", port);
          retval = bind(fd, (struct sockaddr *)&addr_in, sizeof(addr_in));
          if (!retval)
             break;
       }
    }
    add_bound_socket(addr_in.sun_path);
#else
    struct sockaddr_in6  addr_in;
    memset((char *)&addr_in, 0, sizeof(addr_in));
#ifdef BIND_IP
    inet_pton(AF_INET6, BIND_IP, &addr_in.sin6_addr);
    /* fprintf( stderr, "DgramAddr Binding By Request to %s\n",inet_ntoa(addr_in.sin6_addr)); -RLS*/
#else
    inet_pton(AF_INET6, dotaddr, &addr_in.sin6_addr);
#endif
    addr_in.sin6_family		= AF_INET6;
    inet_pton(AF_INET6, dotaddr, &addr_in.sin6_addr);
    addr_in.sin6_port		= htons(port);

    retval = bind(fd, (struct sockaddr *)&addr_in, sizeof(addr_in));
#endif      

    if (retval < 0)
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
 * Originally coded by Bert Gÿsbers
 */
int
#ifdef __STDC__
DgramConnect(int fd, char *host, int port)
#else
DgramConnect(fd, host, port)
int	fd;
char	*host;
int	port;
#endif /* __STDC__ */
{
    int			retval;

#ifdef UNIX_SOCKETS
    struct sockaddr_un  addr_in;
    memset((char *)&addr_in, 0, sizeof(addr_in));
    addr_in.sun_family          = AF_UNIX;
    sprintf(addr_in.sun_path, "/tmp/tomenet%d", (port)? port : getpid());
#else
    struct hostent	*hp;
    struct sockaddr_in6  addr_in;
    memset((char *)&addr_in, 0, sizeof(addr_in));
    addr_in.sin6_family          = AF_INET6;
    addr_in.sin6_port            = htons(port);
    if((inet_pton(AF_INET6, host, &addr_in.sin6_addr)<1))
    {
#ifdef SERVER 
	printf("DgramConnect called with hostname %s.\n", host);
#endif
	hp = gethostbyname2(host, AF_INET6);
	if (hp == NULL)
	{
	    printf("2 No IP6 address for %s\n", host);
	    hp = gethostbyname2(host, AF_INET);
	    if (hp == NULL) {
	   	sl_errno = SL_EHOSTNAME;
	    	return (-1);
	    }
	    else{
		/* I dont like this really */
		*((u_int32_t*)&addr_in.sin6_addr.s6_addr[8]) = ntohl(0x0000ffff);
	    	*((struct in_addr*)(&addr_in.sin6_addr.s6_addr[12])) = *((struct in_addr*)(hp->h_addr));

	    }
	}
	else
	    addr_in.sin6_addr =
		*((struct in6_addr*)(hp->h_addr));
    } /**/
#endif  
#if 0
    hp = gethostbyname2(host, AF_INET6);
    if(hp == NULL)
    {
        sl_errno = SL_EHOSTNAME;
        return(-1);
    }
    memcpy(&addr_in.sin6_addr, hp->h_addr, hp->h_length);
#endif
    retval = connect(fd, (struct sockaddr *)&addr_in, sizeof(addr_in));
    if (retval < 0)
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
#ifdef __STDC__
DgramSend(int fd, char *host, int port,
	  char *sbuf, int size)
#else
DgramSend(fd, host, port, sbuf, size)
int	fd, port, size;
char	*host, *sbuf;
#endif /* __STDC__ */
{
    int			retval;

#ifdef UNIX_SOCKETS     
    struct sockaddr_un  the_addr;
    memset((char *)&the_addr, 0, sizeof(the_addr));
    the_addr.sun_family          = AF_UNIX; 
    sprintf(the_addr.sun_path, "/tmp/tomenet%d", (port)? port : getpid());
    sl_errno = 0;
#else
    struct hostent	*hp;
    struct sockaddr_in6  the_addr;
    char temp[INET6_ADDRSTRLEN];
    memset((char *)&the_addr, 0, sizeof(the_addr));
    the_addr.sin6_family	= AF_INET6;
    the_addr.sin6_port		= htons(port);
    sl_errno = 0;

#if 0	/* Dont bother with this ATM */
    if (sl_broadcast_enabled)
	the_addr.sin6_addr.s_addr	= INADDR_BROADCAST;
    else
#endif
    {
	printf("Getting info for %s\n", host);
	if((inet_pton(AF_INET6, host, &the_addr.sin6_addr)<1))
	{
#ifdef SERVER
	    printf("DgramSend called with host %s\n", host);
#endif
	    printf("%s not a valid IPv6 address\n", host);
	    hp = gethostbyname2(host, AF_INET6);
	    if (hp == NULL)
	    { 
	        printf("3 No IP6 address for %s\n", host);
	        hp = gethostbyname2(host, AF_INET);
	        if (hp == NULL) {
	   	    sl_errno = SL_EHOSTNAME;
	    	    return (-1);
	        }
	        else {
		    /* I dont like this really */
		    *((u_int32_t*)&the_addr.sin6_addr.s6_addr[8]) = ntohl(0x0000ffff);
	    	    *((struct in_addr*)(&the_addr.sin6_addr.s6_addr[12])) = *((struct in_addr*)(hp->h_addr));

	        }
	    }
	    else{
		the_addr.sin6_addr =
		    *((struct in6_addr*)(hp->h_addr));
	    }
	}
    }
#endif      

    inet_ntop(AF_INET6, &the_addr.sin6_addr, &temp, INET6_ADDRSTRLEN);
    cmw_priv_assert_netaccess();
    retval = sendto(fd, sbuf, size, 0, (struct sockaddr *)&the_addr,
		   sizeof(the_addr));
    cmw_priv_deassert_netaccess();
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
#ifdef __STDC__
DgramReceiveAny(int fd, char *rbuf, int size)
#else
DgramReceiveAny(fd, rbuf, size)
int	fd;
char	*rbuf;
int	size;
#endif /* __STDC__ */
{
    int		retval;
    socklen_t	addrlen = sizeof(sl_dgram_lastaddr);

    (void) memset((char *)&sl_dgram_lastaddr, 0, addrlen);
    cmw_priv_assert_netaccess();
    retval = recvfrom(fd, rbuf, size, 0, (struct sockaddr *)&sl_dgram_lastaddr,
	&addrlen);
    cmw_priv_deassert_netaccess();
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
#ifdef __STDC__
DgramReceive(int fd, char *from, char *rbuf, int size)
#else
DgramReceive(fd, from, rbuf, size)
int	fd, size;
char	*from; /* IP address string, or Unix path */
char	*rbuf;
#endif /* __STDC__ */
{
    int			retval;

#ifdef UNIX_SOCKETS
    struct sockaddr_un	tmp_addr;
    strcpy(tmp_addr.sun_path, from);
#else
    struct hostent	*hp;
    struct sockaddr_in6	tmp_addr;

    if((inet_pton(AF_INET6, from, &tmp_addr.sin6_addr)<1))
    {
#ifdef SERVER
	printf("DgramReceive called with host %s.\n", from);
#endif
	hp = gethostbyname2(from, AF_INET6);
	if (hp == NULL)
	{
	    printf("4 No IP6 address for %s\n", from);
	    hp = gethostbyname2(from, AF_INET);
	    if (hp == NULL) {
	   	sl_errno = SL_EHOSTNAME;
	    	return (-1);
	    }
	    else {
		/* I dont like this really */
		*((u_int32_t*)&tmp_addr.sin6_addr.s6_addr[8]) = ntohl(0x0000ffff);
	    	*((struct in_addr*)(&tmp_addr.sin6_addr.s6_addr[12])) = *((struct in_addr*)(hp->h_addr));

	    }
	}
	else
	    tmp_addr.sin6_addr =
		*((struct in6_addr*)(hp->h_addr));
    }
#endif
    retval = DgramReceiveAny(fd, rbuf, size);
    if (retval == -1 ||
#ifdef UNIX_SOCKETS
        strcmp(tmp_addr.sun_path, sl_dgram_lastaddr.sun_path)
#else
	!IN6_ARE_ADDR_EQUAL(&tmp_addr.sin6_addr, &sl_dgram_lastaddr.sin6_addr)
#endif
    ) {
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
#ifdef __STDC__
DgramReply(int fd, char *sbuf, int size)
#else
DgramReply(fd, sbuf, size)
int	fd, size;
char	*sbuf;
#endif /* __STDC__ */
{
    int			retval;

    cmw_priv_assert_netaccess();
    retval = sendto(fd, sbuf, size, 0, (struct sockaddr *)&sl_dgram_lastaddr,
		   sizeof(sl_dgram_lastaddr));
    cmw_priv_deassert_netaccess();
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
int
#ifdef __STDC__
DgramRead(int fd, char *rbuf, int size)
#else
DgramRead(fd, rbuf, size)
int	fd;
char	*rbuf;
int	size;
#endif /* __STDC__ */
{
    int		retval;

    cmw_priv_assert_netaccess();
    retval = recv(fd, rbuf, size, 0);
    cmw_priv_deassert_netaccess();
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
int
#ifdef __STDC__
DgramWrite(int fd, char *wbuf, int size)
#else
DgramWrite(fd, wbuf, size)
int	fd;
char	*wbuf;
int	size;
#endif /* __STDC__ */
{
    int		retval;

    cmw_priv_assert_netaccess();
    retval = send(fd, wbuf, size, 0);
    cmw_priv_deassert_netaccess();
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
#ifdef __STDC__
static void
DgramInthandler(int signum)
#else
static
DgramInthandler()
#endif /* __STDC__ */
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
 *	alarm
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
#ifdef __STDC__
DgramSendRec(int fd, char *host, int port, char *sbuf,
	     int sbuf_size, char *rbuf, int rbuf_size)
#else
DgramSendRec(fd, host, port, sbuf, sbuf_size, rbuf, rbuf_size)
int	fd, port, sbuf_size, rbuf_size;
char	*host, *sbuf, *rbuf;
#endif /* __STDC__ */
{
    int		retval = -1;
    int		retry = sl_default_retries;

    (void) signal(SIGALRM, DgramInthandler);
    while (retry > 0)
    {
	if (DgramSend(fd, host, port, sbuf, sbuf_size) == -1)
	    return (-1);

	(void) alarm(sl_timeout_s);
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
    (void) alarm(0);
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
    static char addbuff[INET6_ADDRSTRLEN];
    socklen_t len = sizeof(struct sockaddr_in6);
    getpeername(fd, (struct sockaddr*)&sl_dgram_lastaddr, &len);
    return ((char*)inet_ntop(AF_INET6, &sl_dgram_lastaddr.sin6_addr, (char*)&addbuff, INET6_ADDRSTRLEN));
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
    static char addbuff[INET6_ADDRSTRLEN];
    socklen_t len = sizeof(struct sockaddr_in6);
    getpeername(fd, (struct sockaddr*)&sl_dgram_lastaddr, &len);

    he = gethostbyaddr((char *)&sl_dgram_lastaddr.sin6_addr,
		       sizeof(struct in_addr), AF_INET6);
    if (he == NULL) {
	str = (char*)inet_ntop(AF_INET6, &sl_dgram_lastaddr.sin6_addr, (char*)&addbuff, INET6_ADDRSTRLEN);
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
    socklen_t len = sizeof(struct sockaddr_in6);
    getpeername(fd, (struct sockaddr*)&sl_dgram_lastaddr, &len);
    return (ntohs((int)sl_dgram_lastaddr.sin6_port));
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
 *	close
 *
 * Called By
 *	User applications.
 *
 * Originally coded by Bert Gijsbers
 */
void
#ifdef __STDC__
DgramClose(int fd)
#else
DgramClose(fd)
int	fd;
#endif /* __STDC__ */
{
    close(fd);
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
#ifdef VMS
#define MAXHOSTNAMELEN  256
#endif
#ifdef SOLARIS
#define MAXHOSTNAMELEN 256
#endif
#ifdef USE_EMX
#define MAXHOSTNAMELEN 256
#endif
#ifdef __STDC__
void GetLocalHostName(char *name, unsigned size)
#else
void GetLocalHostName(name, size)
    char		*name;
    unsigned		size;
#endif /* __STDC__ */
{
#ifdef UNIX_SOCKETS
   strcpy(name, "localhost");
#else
    struct hostent	*he;
    char		*temp;
#ifdef VMS
    char                vms_inethost[MAXHOSTNAMELEN]   = "UCX$INET_HOST";
    char                vms_inetdomain[MAXHOSTNAMELEN] = "UCX$INET_DOMAIN";
    char                vms_host[MAXHOSTNAMELEN];
    char                vms_domain[MAXHOSTNAMELEN];
    int                 namelen;
#endif

    temp = getenv("ANGBAND_HOST");
    if (temp) {
	strncpy(name, temp, size);
        name[size - 1] = '\0';
	return;
    }

    gethostname(name, size);
    if ((he = gethostbyname2(name, AF_INET6)) == NULL) {
	printf("5 No IP6 address for %s\n", name);
	he = gethostbyname2(name, AF_INET);
	if (he == NULL) {
	    return;
	}
    }
    strncpy(name, he->h_name, size);
    name[size - 1] = '\0';
    /*
     * If there are no dots in the name then we don't have the FQDN,
     * and if the address is of the normal Internet type
     * then we try to get the FQDN via the backdoor of the IP address.
     * Let's hope it works :)
     */

    if (strchr(he->h_name, '.') == NULL
	&& he->h_addrtype == AF_INET6
	&& he->h_length == 4) {
	unsigned long a = 0;
	memcpy((void *)&a, he->h_addr_list[0], 4);
	if ((he = gethostbyaddr((char *)&a, 4, AF_INET6)) != NULL
	    && strchr(he->h_name, '.') != NULL) {
	    strncpy(name, he->h_name, size);
	    name[size - 1] = '\0';
	}
	else {
#if !defined(VMS)
	    FILE *fp = fopen("/etc/resolv.conf", "r");
	    if (fp) {
		char *s, buf[256];
		while (fgets(buf, sizeof buf, fp)) {
		    if ((s = strtok(buf, " \t\r\n")) != NULL
			&& !strcmp(s, "domain")
			&& (s = strtok(NULL, " \t\r\n")) != NULL) {
			strcat(name, ".");
			strcat(name, s);
			break;
		    }
		}
		fclose(fp);
	    }
#else
            vms_trnlnm(vms_inethost, namelen, vms_host);
            vms_trnlnm(vms_inetdomain, namelen, vms_domain);
            strcpy(name, vms_host);
            strcat(name, ".");
            strcat(name, vms_domain);
#endif
	    return;
	}
    }
#endif
} /* GetLocalHostName */


#if defined(__sun__)
/*
 * A workaround for a bug in inet_ntoa() on Suns.
 */
char *inet_ntoa (struct in_addr in)
{
	unsigned long addr = ntohl (in.s_addr);
	static char ascii[16];

	sprintf (ascii, "%d.%d.%d.%d",
		addr >> 24 & 0xFF,
		addr >> 16 & 0xFF,
		addr >> 8 & 0xFF,
		addr & 0xFF);

	return ascii;
}
#endif




#endif /* SET_UID */
