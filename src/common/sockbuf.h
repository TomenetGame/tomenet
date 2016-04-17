/* $Id$
 *
 * XPilot, a multiplayer gravity war game.  Copyright (C) 1991-95 by
 *
 *      Bjørn Stabell        (bjoerns@staff.cs.uit.no)
 *      Ken Ronny Schouten   (kenrsc@stud.cs.uit.no)
 *      Bert Gÿsbers         (bert@mc.bio.uva.nl)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef	NET_H
#define	NET_H


#define MIN_SOCKBUF_SIZE	1024
#define MAX_SOCKBUF_SIZE	(64*1024)

#define SERVER_RECV_SIZE	(64*1024)
#define SERVER_SEND_SIZE	(64*1024)

#define CLIENT_SEND_SIZE	(64*1024)
#define CLIENT_RECV_SIZE	(64*1024)

/*
 * Definitions for the states a socket buffer can be in.
 */
#define SOCKBUF_READ		0x01	/* if readable */
#define SOCKBUF_WRITE		0x02	/* if writeable */
#define SOCKBUF_LOCK		0x04	/* if locked against kernel i/o */
#define SOCKBUF_ERROR		0x08	/* if i/o error occurred */
#define SOCKBUF_DGRAM		0x10	/* if datagram socket */

/*
 * Hack: leave some spare room for the last terminating packet
 * of a frame update.
 */
#define SOCKBUF_WRITE_SPARE	8

/*
 * Maximum number of socket i/o retries if datagram socket.
 */
#define MAX_SOCKBUF_RETRIES	2

/*
 * A buffer to reduce the number of system calls made and to reduce
 * the number of network packets.
 */
typedef struct {
    int		sock;		/* socket filedescriptor */
    char	*buf;		/* i/o data buffer */
    int		size;		/* size of buffer */
    int		len;		/* amount of data in buffer (writing/reading) */
    char	*ptr;		/* current position in buffer (reading) */
    int		state;		/* read/write/locked/error status flags */
} sockbuf_t;

int Sockbuf_init(sockbuf_t *sbuf, int sock, int size, int state);
int Sockbuf_cleanup(sockbuf_t *sbuf);
int Sockbuf_clear(sockbuf_t *sbuf);
int Sockbuf_advance(sockbuf_t *sbuf, int len);
int Sockbuf_rollback(sockbuf_t *sbuf, int len);
int Sockbuf_flush(sockbuf_t *sbuf);
int Sockbuf_write(sockbuf_t *sbuf, char *buf, int len);
int Sockbuf_read(sockbuf_t *sbuf);
int Sockbuf_copy(sockbuf_t *dest, sockbuf_t *src, int len);

#if !defined(STDVA)
#   if defined(__STDC__) && !defined(__sun__) || defined(__cplusplus) || defined(SOLARIS)
#	define STDVA	1		/* has ANSI stdarg stuff */
#   else
#	define STDVA	0		/* nope, still the K&R way */
#   endif
#endif
#if STDVA
    // Disable noisy compiler warnings by default
    //int Packet_printf(sockbuf_t *, char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
    //int Packet_scanf(sockbuf_t *, char *fmt, ...) __attribute__ ((format (scanf, 2, 3))) __attribute__ ((warn_unused_result));
    int Packet_printf(sockbuf_t *, char *fmt, ...);
    int Packet_scanf(sockbuf_t *, char *fmt, ...);
#else
    int Packet_printf();
    int Packet_scanf();
#endif

#endif

