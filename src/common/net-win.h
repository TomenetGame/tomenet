#ifdef WIN32

/* -*-C-*-
 *
 * Project :	 TRACE
 *
 * File    :	 socklib.h
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
 * Revision 1.2  91/10/02  08:38:20  08:38:20  arne (Arne Helme)
 * "ANSI C prototypes added."
 *
 * Revision 1.1  91/10/02  08:34:53  08:34:53  arne (Arne Helme)
 * Initial revision
 *
 */


#ifndef _SOCKLIB_INCLUDED
#define _SOCKLIB_INCLUDED


/* Error values and their meanings */
#define SL_ESOCKET		0	/* socket system call error */
#define SL_EBIND		1	/* bind system call error */
#define SL_ELISTEN		2	/* listen system call error */
#define SL_EHOSTNAME		3	/* Invalid host name format */
#define SL_ECONNECT		5	/* connect system call error */
#define SL_ESHUTD		6	/* shutdown system call error */
#define SL_ECLOSE		7	/* close system call error */
#define SL_EWRONGHOST		8	/* message arrived from unspec. host */
#define SL_ENORESP		9	/* No response */
#define SL_ERECEIVE		10	/* Receive error */

#include <winsock.h>	/* includes netinet/in.h's sockaddr_in */

extern void	SetTimeout(int, int);
extern int	GetPortNum(int);
extern int	SetSocketReceiveBufferSize(int, int);
extern int	SetSocketSendBufferSize(int, int);
extern int	SetSocketNonBlocking(int, int);
extern int	SetSocketNoDelay(int, int);
extern int	CreateClientSocket(char *, int);
extern int	SocketAccept(int fd);
extern int	SocketLinger(int fd);
extern int	SocketRead(int, char *, int);
extern int	GetSocketError(int);
extern int	SocketReadable(int);
extern int	CreateDgramSocket(int);
extern int	DgramConnect(int, char *, int);
extern int	DgramSend(int, char *, int, char *, int);
extern int	DgramReceiveAny(int, char *, int);
extern int	DgramReceive(int, char *, char *, int);
extern int	DgramReply(int, char *, int);
extern int	DgramRead(int fd, char *rbuf, int size);
extern int	DgramWrite(int fd, char *wbuf, int size);
extern char	*DgramLastaddr(int fd);
extern char	*DgramLastname(int fd);
extern int	DgramLastport(int fd);
extern void	DgramClose(int);
extern void	GetLocalHostName(char *, unsigned);
extern int	CreateServerSocket(int port);
extern int SocketClose(int fd);
extern void CleanupDgramSocket(int sock);

#if !defined(select) && defined(__linux__)
#define select(N, R, W, E, T)	select((N),		\
	(fd_set*)(R), (fd_set*)(W), (fd_set*)(E), (T))
#endif

#endif /* _SOCKLIB_INCLUDED */


#endif /* WIN32 */
