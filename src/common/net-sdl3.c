/*
 * This is a general "fill-in-the-blanks" network module based on SDL3_net.
 *
 * It is designed to work cross-platform (Linux, Windows, etc.) using the SDL3 networking library.
 * SDL3_net exposes asynchronous NET_StreamSocket handles, while the rest of the client expects small integer descriptors and mostly synchronous calls.
 * This module keeps that pseudo-fd facade local to the SDL3 client path.
 */

#define _SOCKLIB_LIBSOURCE

#include "angband.h"
#include "version.h"
#include "net-sdl3.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>

/* Debug macro. */
#ifdef SDL3_NET_DEBUG
 #define DEB(x) x
#else
 #define DEB(x)
#endif

/* Default timeout values. */
#define DEFAULT_S_TIMEOUT_VALUE        10
#define DEFAULT_US_TIMEOUT_VALUE       0

/* Global socklib errno variable. */
int sl_errno = 0;

/* Global timeout variables (seconds and microseconds). */
int sl_timeout_s = DEFAULT_S_TIMEOUT_VALUE;
int sl_timeout_us = DEFAULT_US_TIMEOUT_VALUE;

static int timeout_ms(void) {
	return sl_timeout_s * 1000 + sl_timeout_us / 1000;
}

/* Maximum stream sockets used. */
#define MAX_FD 1024
/* Start returning file descriptors from this value so that descriptors 0-2
 * remain unused, matching the typical STDIN/STDOUT/STDERR reservations. */
#define FIRST_FD 3

/* Mapping table for StreamSocket pointers. */
static NET_StreamSocket *stream_sockets[MAX_FD] = { NULL };
static int stream_socket_errors[MAX_FD] = { 0 };
static int invalid_fd_error = 0;



/* Forward declarations */
static int addStreamSocket(NET_StreamSocket *sock);
static NET_StreamSocket *getStreamSocket(int fd);
static void delStreamSocket(int fd);

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
void SetTimeout(int s, int us) {
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
 *	Resolves a host and opens an SDL3_net client stream socket.
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
 * Globals Referenced:
 *	sl_errno	- If errors occurred: SL_EHOSTNAME, SL_ESOCKET, SL_ECONNECT (when running out of internal fd).
 *	invalid_fd_error - If errors occurred, same as sl_errno.
 *	stream_socket_errors - If no error, clears error for fd.
 *
 * External Calls:
 *	NET_ResolveHostname
 *	NET_WaitUntilResolved
 *	NET_UnrefAddress
 *	NET_CreateClient
 *	NET_WaitUntilConnected
 *	NET_DestroyStreamSocket
 */
int CreateClientSocket(char *host, int port) {
	NET_Address *addr;
	NET_StreamSocket *sock;
	NET_Status status;
	int fd;

	invalid_fd_error = 0;

	/* Hack: SDL3 net Resolves "localhost" to ipv6 ::1 so it does not connect to current server. So translate "localhost" to "127.0.0.1"*/
	addr = NET_ResolveHostname(SDL_strcasecmp(host, "localhost") == 0 ? "127.0.0.1" : host);
	if (addr == NULL) {
		DEB(fprintf(stderr, "ERROR: can't resolve host name \"%s\": %s\n", host, SDL_GetError());)
		sl_errno = SL_EHOSTNAME;
		invalid_fd_error = sl_errno;
		return -1;
	}

	status = NET_WaitUntilResolved(addr, timeout_ms());
	if (status != NET_SUCCESS) {
		DEB(fprintf(stderr, "ERROR: resolving host \"%s\": %s\n", host, status == NET_WAITING ? "timed out" : SDL_GetError());)
		NET_UnrefAddress(addr);
		sl_errno = SL_EHOSTNAME;
		invalid_fd_error = sl_errno;
		return -1;
	}

	sock = NET_CreateClient(addr, (Uint16)port, 0);
	NET_UnrefAddress(addr);
	if (sock == NULL) {
		DEB(fprintf(stderr, "ERROR: creating client to connect to \"%s:%d\": %s\n", host, port, SDL_GetError());)
		sl_errno = SL_ESOCKET;
		invalid_fd_error = sl_errno;
		return -1;
	}

	status = NET_WaitUntilConnected(sock, timeout_ms());
	if (status != NET_SUCCESS) {
		DEB(fprintf(stderr, "ERROR: connecting client to \"%s:%d\": %s\n", host, port, status == NET_WAITING ? "timed out" : SDL_GetError());)
		NET_DestroyStreamSocket(sock);
		sl_errno = SL_ECONNECT;
		invalid_fd_error = sl_errno;
		return -1;
	}

	fd = addStreamSocket(sock);
	if (fd == -1) {
		DEB(fprintf(stderr, "ERROR: failed to add connected client (%s:%d) to storage\n", host, port);)
		NET_DestroyStreamSocket(sock);
		sl_errno = SL_ECONNECT;
		invalid_fd_error = sl_errno;
		return -1;
	}

	stream_socket_errors[fd] = 0;
	return fd;
} /* CreateClientSocket */


/*
 *******************************************************************************
 *
 *	GetPortNum()
 *
 *******************************************************************************
 * Description:
 *	Returns the SDL3_net local port number for the underlying stream socket
 *	connection.
 *
 * Input Parameters:
 *	fd	- The file descriptor (index in our mapping table).
 *
 * Output Parameters:
 *	None.
 *
 * Return Value:
 *	The port number in host byte order, or -1 on failure.
 *	SDL3_net normally leaves the local port field at 0 for client sockets.
 *
 * Globals Referenced:
 *	stream_socket_errors	- If errors occurred sets the SL_ESOCKET error for fd, otherwise clears any error for fd.
 *	invalid_fd_error	- If fd is invalid, sets the SL_ESOCKET error.
 *
 */
int GetPortNum(int fd) {
	NET_StreamSocket *sock = getStreamSocket(fd);

	if (sock == NULL) {
		if ((fd >= FIRST_FD) && (fd < MAX_FD)) stream_socket_errors[fd] = SL_ESOCKET;
		else invalid_fd_error = SL_ESOCKET;
		return -1;
	}

	/* SDL3_net does not expose the local ephemeral port for stream sockets. */
	stream_socket_errors[fd] = 0;
	return 0;
} /* GetPortNum */





/*
 *******************************************************************************
 *
 *	GetSocketError()
 *
 *******************************************************************************
 * Description:
 *	Clears the stored SDL3 socket error for <fd> and copies it into errno.
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
 * Globals Referenced:
 *	errno	- Stores the error or 0 if none.
 *	stream_socket_errors	- Clears the error for fd after storing to errno.
 *	invalid_fd_error	- Clears the error for fd after storing to errno.
 *
 */
int GetSocketError(int fd) {
	if ((fd >= FIRST_FD) && (fd < MAX_FD)) {
		errno = stream_socket_errors[fd];
		stream_socket_errors[fd] = 0;
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
 *	Checks if data is available on the SDL3_net stream socket before the
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
 * Globals Referenced:
 *	stream_socket_errors	- If errors occurred for fd: SL_ESOCKET, SL_ERECEIVE, otherwise clears any error for fd.
 *	invalid_fd_error	- If fd is invalid, sets the SL_ESOCKET error.
 *
 * External Calls:
 *	NET_WaitUntilInputAvailable
 */
int SocketReadable(int fd) {
	NET_StreamSocket *sock;
	void *vsockets[1] = { NULL };
	int ret;

	sock = getStreamSocket(fd);
	if (sock == NULL) {
		if ((fd >= FIRST_FD) && (fd < MAX_FD)) stream_socket_errors[fd] = SL_ESOCKET;
		else invalid_fd_error = SL_ESOCKET;
		return -1;
	}

	vsockets[0] = sock;
	ret = NET_WaitUntilInputAvailable(vsockets, 1, timeout_ms());
	if (ret < 0) {
		stream_socket_errors[fd] = SL_ERECEIVE;
		return -1;
	}
	stream_socket_errors[fd] = 0;
	if (ret == 0) return 0;
	return 1;
} /* SocketReadable */


/*
 *******************************************************************************
 *
 *	SocketRead()
 *
 *******************************************************************************
 * Description:
 *	Receives up to <size> bytes from the stream socket into <buf>,
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
 * Globals Referenced:
 *	stream_socket_errors	- If errors occurred sets the SL_ESOCKET or SL_ERECEIVE error for fd, otherwise clears any error for fd.
 *	invalid_fd_error	- If fd is invalid, sets the SL_ESOCKET error.
 *	errno - Stores EIO orror if SL_ERECEIVE error occurred.
 *	sl_timeout_s, sl_timeout_us - Used to compute the timeout in milliseconds.
 *
 * External Calls:
 *	NET_WaitUntilInputAvailable
 *	NET_ReadFromStreamSocket
 */
int SocketRead(int fd, char *buf, int size) {
	NET_StreamSocket *sock;
	void *vsockets[1];
	int bytes, received = 0;
	int readable;

	sock = getStreamSocket(fd);
	if (sock == NULL) {
		if ((fd >= FIRST_FD) && (fd < MAX_FD)) stream_socket_errors[fd] = SL_ESOCKET;
		else invalid_fd_error = SL_ESOCKET;
		return -1;
	}

	if (size <= 0) {
		stream_socket_errors[fd] = 0;
		return 0;
	}

	stream_socket_errors[fd] = 0;
	while (received < size) {
		vsockets[0] = sock;
		readable = NET_WaitUntilInputAvailable(vsockets, 1, timeout_ms());
		if (readable <= 0) {
			if (received == 0) {
				if (readable < 0) stream_socket_errors[fd] = SL_ERECEIVE;
				errno = EIO;
				return -1;
			}
			return received;
		}

		bytes = NET_ReadFromStreamSocket(sock, &buf[received], size - received);
		if (bytes <= 0) {
			if (received == 0) {
				stream_socket_errors[fd] = SL_ERECEIVE;
				errno = EIO;
				return bytes;
			}
			return received;
		}

		received += bytes;
	}

	return received;
} /* SocketRead */


/*
 *******************************************************************************
 *
 *	SocketWrite()
 *
 *******************************************************************************
 * Description:
 *	Attempts to send <size> bytes on a connected SDL3_net stream socket.
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
 * Globals Referenced:
 *	stream_socket_errors	- If errors occurred sets the SL_ESOCKET or SL_ECONNECT error for fd, otherwise clears any error for fd.
 *	invalid_fd_error	- If fd is invalid, sets the SL_ESOCKET error.
 *
 * External Calls:
 *	NET_WriteToStreamSocket
 */
int SocketWrite(int fd, char *wbuf, int size) {
	NET_StreamSocket *sock;

	sock = getStreamSocket(fd);
	if (sock == NULL) {
		if ((fd >= FIRST_FD) && (fd < MAX_FD)) stream_socket_errors[fd] = SL_ESOCKET;
		else invalid_fd_error = SL_ESOCKET;
		return -1;
	}

	if (!NET_WriteToStreamSocket(sock, wbuf, size)) {
		stream_socket_errors[fd] = SL_ECONNECT;
		errno = EIO;
		return -1;
	}

	stream_socket_errors[fd] = 0;
	return size;
} /* SocketWrite */


/*
 *******************************************************************************
 *
 *	GetLocalHostName()
 *
 *******************************************************************************
 * Description:
 *	Best-effort local host name lookup using SDL3_net local addresses.
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
 * Globals Referenced:
 *
 * External Calls:
 *	NET_GetLocalAddresses
 *	NET_GetAddressString
 *	NET_FreeLocalAddresses
 */
void GetLocalHostName(char *name, unsigned size) {
	NET_Address **addrs;
	const char *host = NULL;
	int count = 0, i;

	addrs = NET_GetLocalAddresses(&count);
	for (i = 0; addrs && i < count; i++) {
		host = NET_GetAddressString(addrs[i]);
		if (host && *host) {
			strncpy(name, host, size);
			name[size - 1] = '\0';
			NET_FreeLocalAddresses(addrs);
			return;
		}
	}

	if (addrs) NET_FreeLocalAddresses(addrs);
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
 *	Closes an SDL3_net stream socket and removes its descriptor mapping.
 *	SDL3_net does not expose a separate graceful shutdown step.
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
 * Globals Referenced:
 *	stream_socket_errors	- If errors occurred sets the SL_ECLOSE error for fd, otherwise clears any error for fd.
 *	invalid_fd_error	- If fd is invalid, sets the SL_ECLOSE error.
 *
 * External Calls:
 *	NET_DestroyStreamSocket
 */
int SocketClose(int fd) {
	NET_StreamSocket *sock;

	sock = getStreamSocket(fd);
	if (sock == NULL) {
		if ((fd >= FIRST_FD) && (fd < MAX_FD)) stream_socket_errors[fd] = SL_ECLOSE;
		else invalid_fd_error = SL_ECLOSE;
		return -1;
	}

	NET_DestroyStreamSocket(sock);
	delStreamSocket(fd);
	stream_socket_errors[fd] = 0;
	return 1;
} /* SocketClose */


/*
 *******************************************************************************
 *
 *	DgramWrite()
 *
 *******************************************************************************
 * Description:
 *	Attempts to send <size> bytes on a connected SDL3_net stream socket.
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
 * Globals Referenced:
 *
 * External Calls:
 */
int DgramWrite(int fd, char *wbuf, int size) {
	return SocketWrite(fd, wbuf, size);
} /* DgramWrite */


/*
 *******************************************************************************
 *
 *	DgramRead()
 *
 *******************************************************************************
 * Description:
 *	Receives up to <size> bytes from the SDL3_net stream socket into <buf>.
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
 * Globals Referenced:
 *	stream_socket_errors	- If errors occurred sets the SL_ESOCKET or SL_ERECEIVE error for fd, otherwise clears any error for fd.
 *	invalid_fd_error	- If fd is invalid, sets the SL_ESOCKET error.
 *
 * External Calls:
 *	NET_ReadFromStreamSocket
 */
int	DgramRead(int fd, char *buf, int size) {
	NET_StreamSocket *sock;
	int bytes;

	sock = getStreamSocket(fd);
	if (sock == NULL) {
		if ((fd >= FIRST_FD) && (fd < MAX_FD)) stream_socket_errors[fd] = SL_ESOCKET;
		else invalid_fd_error = SL_ESOCKET;
		return -1;
	}

	bytes = NET_ReadFromStreamSocket(sock, buf, size);
	if (bytes <= 0) {
		stream_socket_errors[fd] = SL_ERECEIVE;
		errno = EIO;
	} else {
		stream_socket_errors[fd] = 0;
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
 *	removes its descriptor mapping. The same as SocketClose().
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
 * Globals Referenced:
 *
 * External Calls:
 */
void DgramClose(int fd) {
	SocketClose(fd);
} /* DgramClose */


/*
 *******************************************************************************
 *
 * Stream socket helper functions.
 *
 *******************************************************************************
 */

/*
 *******************************************************************************
 *
 * addStreamSocket()
 *
 *******************************************************************************
 * Description:
 *	Adds a NET_StreamSocket to the mapping table.
 *	Returns the file descriptor (index) if successful, or -1 if the table is full.
 *
 */
static int addStreamSocket(NET_StreamSocket *sock) {
	int i;

	for (i = FIRST_FD; i < MAX_FD; i++) {
		if (stream_sockets[i] == NULL) {
			stream_sockets[i] = sock;
			stream_socket_errors[i] = 0;
			return i;
		}
	}
	/* Table is full. */
	return -1;
}

/*
 *******************************************************************************
 *
 * getStreamSocket()
 *
 *******************************************************************************
 * Description:
 *  Retrieves the NET_StreamSocket pointer corresponding to the given file descriptor.
 *  Returns NULL if the fd is out of range or if no socket is stored at that index.
 */
static NET_StreamSocket *getStreamSocket(int fd) {
	if ((fd < FIRST_FD) || (fd >= MAX_FD)) return NULL;
	return stream_sockets[fd];
}

/*
 *******************************************************************************
 *
 * delStreamSocket()
 *
 *******************************************************************************
 * Description:
 *  Removes the NET_StreamSocket associated with the given file descriptor from the mapping table.
 */
static void delStreamSocket(int fd) {
	if ((fd >= FIRST_FD) && (fd < MAX_FD)) {
		stream_sockets[fd] = NULL;
		stream_socket_errors[fd] = 0;
	}
}
