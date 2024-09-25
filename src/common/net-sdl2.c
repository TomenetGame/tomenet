/*
 * This is a general "fill-in-the-blanks" network module based on SDL_net.
 *
 * It is designed to work cross-platform (Linux, Windows, etc.) using the SDL2 networking library.
 * SDL_net exposes TCPsocket handles instead of OS file descriptors, so for compatibility reasons
 * this module maps small integer descriptors to TCPsocket pointers for the rest of the client.
 */

#define _SOCKLIB_LIBSOURCE

/* Maximum number of addresses returned by SDLNet_GetLocalAddresses. */
#define MAX_LOCAL_ADDRESSES 32

#include "angband.h"
#include "version.h"
#include "net-sdl2.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <errno.h>

/* Debug macro */
#ifdef DEBUG
 #define DEB(x) x
#else
 #define DEB(x)
#endif

/* Default timeout values */
#define DEFAULT_S_TIMEOUT_VALUE        10
#define DEFAULT_US_TIMEOUT_VALUE       0

/* Default retry value */
#define DEFAULT_RETRIES                5

/* Global socklib errno variable */
int sl_errno = 0;

/* Global timeout variables (seconds and microseconds) */
int sl_timeout_s = DEFAULT_S_TIMEOUT_VALUE;
int sl_timeout_us = DEFAULT_US_TIMEOUT_VALUE;
/* Mapping table for TCPsocket pointers */
#define MAX_TCP_FD 1024
/* Start returning file descriptors from this value so that descriptors 0-2
 * remain unused, matching the typical STDIN/STDOUT/STDERR reservations. */
#define FIRST_TCP_FD 3
static TCPsocket tcp_socket_table[MAX_TCP_FD] = { NULL };
static int tcp_error_table[MAX_TCP_FD] = { 0 };
static int invalid_fd_error = 0;



/* Forward declarations */
static int addTCPsocket(TCPsocket sock);
static TCPsocket fd2TCPsocket(int fd);
static void delTCPsocket(int fd);

/*
 *******************************************************************************
 *
 *	SetTimeout()
 *
 *******************************************************************************
 *
 * Description:
 *	Sets the global timeout value to s seconds and us microseconds.
 *
 * Input Parameters:
 *	s   - Timeout value in seconds.
 *	us  - Timeout value in microseconds.
 *
 * Output Parameters:
 *	None.
 *
 * Return Value:
 *	void.
 *
 */
void SetTimeout(int s, int us)
{
	sl_timeout_s = s;
	sl_timeout_us = us;
} /* SetTimeout */


/*
 *******************************************************************************
 *
 *	CreateClientSocket()
 *
 *******************************************************************************
 * Description:
 *	Resolves a host and opens an SDL_net TCP client socket.
 *
 * Input Parameters:
 *	host	- Pointer to a string containing the peer host name.
 *	port	- The port number to connect to.
 *
 * Output Parameters:
 *	None.
 *
 * Return Value:
 *	Returns the file descriptor (index in our mapping table) or -1 on error.
 *
 * Globals Referenced
 *	sl_errno	- If errors occurred: SL_EHOSTNAME, SL_ESOCKET, SL_ECONNECT (when running out of internal fd).
 *	invalid_fd_error - If errors occurred, same as sl_errno.
 *	tcp_error_table - If no error, clears error for fd.
 *
 * External Calls
 *	SDLNet_ResolveHost
 *	SDLNet_TCP_Open
 *	SDLNet_TCP_Close
 *
 */
int CreateClientSocket(char *host, int port)
{
	IPaddress ip;
	TCPsocket sock;
	int fd;

	invalid_fd_error = 0;
	if (SDLNet_ResolveHost(&ip, host, port) < 0) {
		sl_errno = SL_EHOSTNAME;
		invalid_fd_error = sl_errno;
		return -1;
	}

	sock = SDLNet_TCP_Open(&ip);
	if (sock == NULL) {
		sl_errno = SL_ESOCKET;
		invalid_fd_error = sl_errno;
		return -1;
	}

	fd = addTCPsocket(sock);
	if (fd == -1) {
		SDLNet_TCP_Close(sock);
		sl_errno = SL_ECONNECT;
		invalid_fd_error = sl_errno;
		return -1;
	}
	tcp_error_table[fd] = 0;
	return fd;
} /* CreateClientSocket */


/*
 *******************************************************************************
 *
 *	GetPortNum()
 *
 *******************************************************************************
 * Description:
 *	Returns the SDL_net local port number for a TCP socket connection.
 *
 * Input Parameters:
 *	fd	- The file descriptor (index in our mapping table).
 *
 * Output Parameters:
 *	None.
 *
 * Return Value:
 *	The port number in host byte order, or -1 on failure.
 *	SDL_net normally leaves the local port field at 0 for client sockets.
 *
 * Globals Referenced
 *	tcp_error_table	- If errors occurred sets the SL_ESOCKET error for fd.
 *	invalid_fd_error	- If fd is invalid, sets the SL_ESOCKET error.
 *
 * External Calls
 *	SDL_SwapBE16
 */
int GetPortNum(int fd)
{
	TCPsocket sock;
	Uint16 net, port;

	sock = fd2TCPsocket(fd);
	if (sock == NULL) {
		if ((fd >= FIRST_TCP_FD) && (fd < MAX_TCP_FD)) tcp_error_table[fd] = SL_ESOCKET;
		else invalid_fd_error = SL_ESOCKET;
		return -1;
	}

	/*
	 * SDL_net does not provide a public API to query the local port number.
	 * The returned TCPsocket pointer is therefore cast to the private `_TCPsocket` structure defined in SDL_net (see https://github.com/libsdl-org/SDL_net/blob/SDL2/src/SDLnetTCP.c#L31).
	 * This relies on SDL_net keeping the same layout; future versions could change it and break this code.
	 */
	struct _TCPsocket {
		int ready;
		int channel;
		IPaddress remoteAddress;
		IPaddress localAddress;
		int sflag;
	};

	/* The port is in network byte order and needs to be converted. */
	/* Note: The localAddress is never filled when creating a TCPsocket and the localAddress.port will be always 0. Maybe remoteAddress port needs to be returned? */
	//Uint16 net = ((struct _TCPsocket*)sock)->remoteAddress.port;
	net = ((struct _TCPsocket*)sock)->localAddress.port;
	port = SDL_SwapBE16(net);
	tcp_error_table[fd] = 0;
	return port;
} /* GetPortNum */





/*
 *******************************************************************************
 *
 *	GetSocketError()
 *
 *******************************************************************************
 * Description:
 *	Clears the stored SDL2 socket error for <fd> and copies it into errno.
 *
 * Input Parameters:
 *	fd	- The file descriptor (index in our mapping table).
 *
 * Output Parameters:
 *	None.
 *
 * Return Value:
 *	0. The stored error code is placed in errno.
 *
 * Globals Referenced
 *	errno	- stores the error or 0 if none.
 *	tcp_error_table	- clears the error for fd after storing to errno.
 *	invalid_fd_error	- clears the error for fd after storing to errno.
 *
 */
int GetSocketError(int fd)
{
	if ((fd >= FIRST_TCP_FD) && (fd < MAX_TCP_FD)) {
		errno = tcp_error_table[fd];
		tcp_error_table[fd] = 0;
	} else {
		errno = invalid_fd_error;
		invalid_fd_error = 0;
	}
	return 0;
} /* GetSocketError */


/*
 *******************************************************************************
 *
 *	SocketReadable()
 *
 *******************************************************************************
 * Description:
 *	Checks if data is available on the SDL_net TCP socket before the
 *	timeout configured by SetTimeout() expires.
 *
 * Input Parameters:
 *	fd	- The file descriptor (index in our mapping table).
 *
 * Output Parameters:
 *	None.
 *
 * Return Value:
 *	1 if readable, 0 if not, -1 on error.
 *
 * Globals Referenced
 *	tcp_error_table	- If errors occurred for fd: SL_ESOCKET, SL_ECONNECT, SL_EBIND, SL_ERECEIVE.
 *
 * External Calls
 *	SDLNet_AllocSocketSet
 *	SDLNet_TCP_AddSocket
 *	SDLNet_CheckSockets
 *	SDLNet_FreeSocketSet
 *	SDLNet_DelSocket
 *
 */
int SocketReadable(int fd)
{
	TCPsocket sock;
	SDLNet_SocketSet set;
	int timeout_ms, ret;

	sock = fd2TCPsocket(fd);
	if (sock == NULL) {
		if ((fd >= FIRST_TCP_FD) && (fd < MAX_TCP_FD)) tcp_error_table[fd] = SL_ESOCKET;
		else invalid_fd_error = SL_ESOCKET;
		return -1;
	}

	set = SDLNet_AllocSocketSet(1);
	if (set == NULL) {
		tcp_error_table[fd] = SL_ECONNECT;
		return -1;
	}
	if (SDLNet_TCP_AddSocket(set, sock) == -1) {
		SDLNet_FreeSocketSet(set);
		tcp_error_table[fd] = SL_EBIND;
		return -1;
	}
	/* Calculate timeout in milliseconds */
	timeout_ms = sl_timeout_s * 1000 + sl_timeout_us / 1000;
	ret = SDLNet_CheckSockets(set, timeout_ms);
	SDLNet_DelSocket(set, (SDLNet_GenericSocket)sock);
	SDLNet_FreeSocketSet(set);
	if (ret > 0) {
		tcp_error_table[fd] = 0;
		return 1;
	} else if (ret == 0) {
		tcp_error_table[fd] = 0;
		return 0;
	} else {
		tcp_error_table[fd] = SL_ERECEIVE;
		return -1;
	}
} /* SocketReadable */


/*
 *******************************************************************************
 *
 *	SocketRead()
 *
 *******************************************************************************
 * Description:
 *	Receives up to <size> bytes from the TCP/IP socket into <buf>,
 *	accumulating chunks in a loop until the requested size is satisfied,
 *	the peer closes the connection, an error occurs, or the next chunk is
 *	not available for a time set in SetTimeout().
 *	Returns any partial data already received unless the first read attempt
 *	fails outright.
 *
 * Input Parameters:
 *	fd	- The file descriptor (index in our mapping table).
 *	size	- Maximum number of bytes to read.
 *
 * Output Parameters:
 *	buf	- Pointer to the buffer to store received data.
 *
 * Return Value:
 *	Number of bytes received, 0 if the peer closes before any data is read,
 *	or -1 on error/timeout when no data was read.
 *
 */
int SocketRead(int fd, char *buf, int size)
{
	TCPsocket sock;
	SDLNet_SocketSet set;
	int bytes, received, readable;
	int timeout_ms;

	sock = fd2TCPsocket(fd);
	if (sock == NULL) {
		if ((fd >= FIRST_TCP_FD) && (fd < MAX_TCP_FD)) tcp_error_table[fd] = SL_ESOCKET;
		else invalid_fd_error = SL_ESOCKET;
		return -1;
	}

	if (size <= 0) {
		tcp_error_table[fd] = 0;
		return 0;
	}

	timeout_ms = sl_timeout_s * 1000 + sl_timeout_us / 1000;

	/* Poll the socket for data so we only read if it won't block. */
	set = SDLNet_AllocSocketSet(1);
	if (set == NULL) {
		tcp_error_table[fd] = SL_ECONNECT;
		return -1;
	}
	if (SDLNet_TCP_AddSocket(set, sock) == -1) {
		SDLNet_FreeSocketSet(set);
		tcp_error_table[fd] = SL_EBIND;
		return -1;
	}

	tcp_error_table[fd] = 0;
	received = 0;
	while (received < size) {
		/*
		 * This metaserver-style read has no known exact length, and the
		 * reply can arrive in multiple TCP chunks after the caller's initial
		 * SocketReadable() check.  Keep the full timeout between partial
		 * chunks, matching the X11 client behaviour; a zero-timeout probe can
		 * report idle between chunks and truncate the server list.
		 */
		readable = SDLNet_CheckSockets(set, timeout_ms);
		if (readable <= 0) {
			SDLNet_DelSocket(set, (SDLNet_GenericSocket)sock);
			SDLNet_FreeSocketSet(set);
			if (received == 0) {
				if (readable < 0) tcp_error_table[fd] = SL_ERECEIVE;
				errno = EIO;
				return -1;
			}
			return received;
		}

		bytes = SDLNet_TCP_Recv(sock, &buf[received], size - received);
		if (bytes <= 0) {
			SDLNet_DelSocket(set, (SDLNet_GenericSocket)sock);
			SDLNet_FreeSocketSet(set);
			if (received == 0) {
				tcp_error_table[fd] = SL_ERECEIVE;
				errno = EIO;
				return bytes;
			}
			return received;
		}

		received += bytes;
	}

	SDLNet_DelSocket(set, (SDLNet_GenericSocket)sock);
	SDLNet_FreeSocketSet(set);
	return received;
} /* SocketRead */


/*
 *******************************************************************************
 *
 *	SocketWrite()
 *
 *******************************************************************************
 * Description:
 *	Attempts to send <size> bytes on a connected SDL_net TCP socket.
 *	This is a single send attempt; callers must handle short writes.
 *
 * Input Parameters:
 *	fd	- The file descriptor (index in our mapping table).
 *	wbuf	- Pointer to the message buffer to send.
 *	size	- Size of the data to send.
 *
 * Output Parameters:
 *	None.
 *
 * Return Value:
 *	Number of bytes sent, or -1 on error.
 *
 */
int SocketWrite(int fd, char *wbuf, int size)
{
	TCPsocket sock;
	int bytes;

	/* Retrieve the TCP socket from the mapping table */
	sock = fd2TCPsocket(fd);
	if (sock == NULL) {
		if ((fd >= FIRST_TCP_FD) && (fd < MAX_TCP_FD)) tcp_error_table[fd] = SL_ESOCKET;
		else invalid_fd_error = SL_ESOCKET;
		return -1;
	}

	/* Send data over the TCP socket */
	bytes = SDLNet_TCP_Send(sock, wbuf, size);

	/* Check if the send operation was successful. */
	if (bytes < 0) {
		tcp_error_table[fd] = SL_ECONNECT;
		errno = EIO;
	} else {
		tcp_error_table[fd] = 0;
		if (bytes == 0) errno = EAGAIN;
	}

	return bytes;
} /* SocketWrite */


/*
 *******************************************************************************
 *
 *	GetLocalHostName()
 *
 *******************************************************************************
 * Description:
 *	Best-effort local host name lookup using SDL_net local addresses.
 *
 * Input Parameters:
 *	name	- Pointer to an array to store the hostname.
 *	size	- Size of the array.
 *
 * Output Parameters:
 *	The hostname is copied into the provided array.
 *
 * Return Value:
 *	None.
 *
 * Note:
 *	If no usable local address can be resolved, this implementation uses
 *	"127.0.0.1" as a fallback.
 *
 */
void GetLocalHostName(char *name, unsigned size)
{
	IPaddress addrs[MAX_LOCAL_ADDRESSES];
	int count, i;
	const char *host = NULL;

	count = SDLNet_GetLocalAddresses(addrs, MAX_LOCAL_ADDRESSES);
	if (count > MAX_LOCAL_ADDRESSES) count = MAX_LOCAL_ADDRESSES;

	for (i = 0; i < count; ++i) {
		if (addrs[i].host == INADDR_NONE || addrs[i].host == INADDR_ANY) continue;

		host = SDLNet_ResolveIP(&addrs[i]);
		if (host && *host) {
			strncpy(name, host, size);
			name[size - 1] = '\0';
			return;
		}
	}

	strncpy(name, "127.0.0.1", size);
	name[size - 1] = '\0';
} /* GetLocalHostName */


/*
 *******************************************************************************
 *
 *	SocketClose()
 *
 *******************************************************************************
 * Description:
 *	Closes an SDL_net TCP socket and removes its descriptor mapping.
 *	SDL_net does not expose a separate graceful shutdown step.
 *
 * Input Parameters:
 *	fd	- The file descriptor (index in our mapping table).
 *
 * Output Parameters:
 *	None.
 *
 * Return Value:
 *	1 on success, -1 on error.
 *
 */
int SocketClose(int fd)
{
	TCPsocket sock;

	sock = fd2TCPsocket(fd);
	if (sock == NULL) {
		if ((fd >= FIRST_TCP_FD) && (fd < MAX_TCP_FD)) tcp_error_table[fd] = SL_ECLOSE;
		else invalid_fd_error = SL_ECLOSE;
		return -1;
	}

	SDLNet_TCP_Close(sock);
	delTCPsocket(fd);
	tcp_error_table[fd] = 0;
	return 1;
} /* SocketClose */

/*
 *******************************************************************************
 *
 * TCP socket helper functions.
 *
 *******************************************************************************
 */


/*
 *******************************************************************************
 *
 * addTCPsocket()
 *
 *******************************************************************************
 * Description:
 *	Adds a TCPsocket to the mapping table.
 *	Returns the file descriptor (index) if successful, or -1 if the table is full.
 *
 */
static int addTCPsocket(TCPsocket sock)
{
	int i;

	/* descriptors 0-2 are reserved; start allocating from FIRST_TCP_FD */
	for (i = FIRST_TCP_FD; i < MAX_TCP_FD; i++) {
		if (tcp_socket_table[i] == NULL) {
			tcp_socket_table[i] = sock;
			tcp_error_table[i] = 0;
			return i;
		}
	}
	return -1; /* Table is full */
}

/*
 * fd2TCPsocket:
 *  Retrieves the TCPsocket pointer corresponding to the given file descriptor.
 *  Returns NULL if the fd is out of range or if no socket is stored at that index.
 */
static TCPsocket fd2TCPsocket(int fd)
{
	if ((fd < FIRST_TCP_FD) || (fd >= MAX_TCP_FD)) {
		return NULL;
	}
	return tcp_socket_table[fd];
}

/*
 * delTCPsocket:
 *  Removes the TCPsocket associated with the given file descriptor from the mapping table.
 */
static void delTCPsocket(int fd)
{
	if ((fd >= FIRST_TCP_FD) && (fd < MAX_TCP_FD)) {
		tcp_socket_table[fd] = NULL;
		tcp_error_table[fd] = 0;
	}
}

/*
 *******************************************************************************
 *
 *	DgramWrite()
 *
 *******************************************************************************
 * Description:
 *	Attempts to send <size> bytes on a connected SDL_net TCP socket.
 *	This mirrors the native client DgramWrite() contract used by sockbuf:
 *	one send attempt, with short writes handled by the caller.
 *
 * Input Parameters:
 *	fd	- The file descriptor (index in our mapping table).
 *	wbuf	- Pointer to the message buffer to send.
 *	size	- Size of the data to send.
 *
 * Output Parameters:
 *	None.
 *
 * Return Value:
 *	Number of bytes sent, or -1 on error.
 *
 */
int	DgramWrite(int fd, char *wbuf, int size) {
	return SocketWrite(fd, wbuf, size);
}


/*
 *******************************************************************************
 *
 *	DgramRead()
 *
 *******************************************************************************
 * Description:
 *	Receives up to <size> bytes from the SDL_net TCP socket into <buf>.
 *	This is a single receive attempt; it does not loop to fill the buffer.
 *
 * Input Parameters:
 *	fd	- The file descriptor (index in our mapping table).
 *	size	- Maximum number of bytes to read.
 *
 * Output Parameters:
 *	buf	- Pointer to the buffer to store received data.
 *
 * Return Value:
 *	Number of bytes received, or -1 on error.
 *
 */
int	DgramRead(int fd, char *buf, int size)
{
	TCPsocket sock;
	int bytes;

	sock = fd2TCPsocket(fd);
	if (sock == NULL) {
		if ((fd >= FIRST_TCP_FD) && (fd < MAX_TCP_FD)) tcp_error_table[fd] = SL_ESOCKET;
		else invalid_fd_error = SL_ESOCKET;
		return -1;
	}

	bytes = SDLNet_TCP_Recv(sock, buf, size);
	if (bytes <= 0) {
		tcp_error_table[fd] = SL_ERECEIVE;
		errno = EIO;
	} else {
		tcp_error_table[fd] = 0;
	}
	return bytes;
} /* DgramRead */

/*
 *******************************************************************************
 *
 *	DgramClose()
 *
 *******************************************************************************
 * Description:
 *	Closes an SDL_net TCP socket used through the historical Dgram API and
 *	removes its descriptor mapping.
 *
 * Input Parameters:
 *	fd	- The file descriptor (index in our mapping table).
 *
 * Output Parameters:
 *	None.
 *
 * Return Value:
 *	None.
 *
 */
void	DgramClose(int fd) {
	SocketClose(fd);
	return;
}
