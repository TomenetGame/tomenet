/*
 * metaclient.c
 * Meta client implementation for TomeNET
 */

/* added this for consistency in some (unrelated) header-inclusion,
   it IS a server file, isn't it? */
#define SERVER

#include "angband.h"
#include <sys/types.h>
#ifndef WINDOWS
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

typedef enum {
	META_NONE = 0,
	META_CONNECTING,
	META_CONNECTED,
	META_DISABLED
} meta_state_t;

/* Has the code been initialized? */
static bool meta_initialized = FALSE;

/* Metaserver socket */
static int meta_fd = -1;

/* State of the metaserver connection */
static meta_state_t meta_state = META_DISABLED;

/* Local hostname */
static char meta_local_name[1024];

/* Sockaddr structure containing the address of the metaserver */
static struct sockaddr_in meta_sockaddr;

/* Has the output handler been registered with the scheduler? */
static bool meta_output_installed = FALSE;

/* Do we need to send updated information to the metaserver? */
static bool meta_needs_update = FALSE;

static void meta_init(void);
static void meta_connect(int blocking);
static int meta_write(int flag);
static void meta_close(void);
static void meta_sched_output_callback(int fd, int arg);

/*
 * Initialize the meta client code
 */
static void meta_init(void) {
	struct hostent *hp;
	
	if (!meta_initialized) {
		/* Find out the hostname of the server */
		if (cfg.bind_name) {
			strncpy(meta_local_name, cfg.bind_name, 1024);
		} else {
			GetLocalHostName(meta_local_name, 1024);
		}
		
		/* Initialize the metaserver address structure */
		memset(&meta_sockaddr, 0, sizeof(meta_sockaddr));
		meta_sockaddr.sin_family = AF_INET;
		meta_sockaddr.sin_port = htons(cfg.meta_port);
		
		/* Look up the metaserver hostname */
		hp = gethostbyname(cfg.meta_address);
		if (hp == NULL) {
			s_printf("Failed to resolve metaserver hostname!\n");
			meta_sockaddr.sin_addr.s_addr = 0;
		} else {
			meta_sockaddr.sin_addr.s_addr = ((struct in_addr*)(hp->h_addr))->s_addr;
		}
		
		/* Set the flag */
		meta_initialized = TRUE;
	}
}

/*
 * Called on every server tick from dungeon()
 */
void meta_tick(void) {
	switch (meta_state) {
		case META_NONE:
			if (meta_needs_update) {
				meta_connect(FALSE);
			}
			break;
		case META_CONNECTING:
			break;
		case META_CONNECTED:
			meta_write(META_UPDATE);
			meta_close();
			break;
		case META_DISABLED:
			break;
	}
}

/*
 * Connect to the metaserver
 */
static void meta_connect(int blocking) {
	int sock;
	
	/* Create a socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
#ifdef WINDOWS
		int errval = WSAGetLastError();
#else
		int errval = errno;
#endif
		s_printf("Failed to create meta socket! (errno = %d)\n", errval);
		return;
	}
	
	if (!blocking) {
		/* Make it non-blocking */
		if (SetSocketNonBlocking(sock, 1) == -1) {
			s_printf("Failed to make meta socket non-blocking!\n");
		}
	}
	
	if (connect(sock, (struct sockaddr *)&meta_sockaddr, sizeof(meta_sockaddr)) == -1) {
#ifdef WINDOWS
		int errval = WSAGetLastError();
		if (errval != WSAEINPROGRESS && errval != WSAEWOULDBLOCK) {
#else
		int errval = errno;
		if (errval != EINPROGRESS && errval != EWOULDBLOCK) {
#endif
			s_printf("Failed to connect to metaserver! (errno = %d)\n", errval);
			close(sock);
			return;
		}
	}
	
	install_output(meta_sched_output_callback, sock, 0);
	meta_output_installed = TRUE;
	meta_fd = sock;
	meta_state = META_CONNECTING;
}

/*
 * Send data to the metaserver
 */
static int meta_write(int flag) {
	char buf_meta[16384];
	char temp[160];
	char *url, *notes;
	int num_dungeon_masters = 0;
	int i;
	
	memset(buf_meta, 0, sizeof(buf_meta));
	
	url = html_escape(meta_local_name);
	sprintf(buf_meta, "<server url=\"%s\" port=\"%d\" protocol=\"2\"", url, (int) cfg.game_port);
	free(url);
	
	if (flag & META_DIE) {
		strcat(buf_meta, " death=\"true\"></server>");
	} else {
		strcat(buf_meta, ">");
		
		strcat(buf_meta, "<notes>");
		notes = html_escape(cfg.server_notes);
		strcat(buf_meta, notes);
		free(notes);
		strcat(buf_meta, "</notes>");
		
		for (i = 1; i <= NumPlayers; i++) {
			if (Players[i]->admin_dm && cfg.secret_dungeon_master) num_dungeon_masters++;
		}

		/* if someone other than a dungeon master is playing */
		if (NumPlayers - num_dungeon_masters) {
			char *name;
			for (i = 1; i <= NumPlayers; i++) {
				/* handle the cfg_secret_dungeon_master option */
				if (Players[i]->admin_dm && cfg.secret_dungeon_master) continue;

				strcat(buf_meta, "<player>");
				name = html_escape(Players[i]->name);
				strcat(buf_meta, name);
				free(name);
				strcat(buf_meta, "</player>");
			}
		}
	
#ifdef TEST_SERVER
		strcat(buf_meta, "<game>TomeNET TEST-ONLY</game>");
#else
 #ifdef ARCADE_SERVER /* made this one higher priority since using RPG_SERVER as an "ARCADE-addon" might be desired :) */
		//removed "Smash." -Moltor
		strcat(buf_meta, "<game>TomeNET Arcade</game>");
 #else
  #ifdef RPG_SERVER
//		strcat(buf_meta, "<game>TomeNET Ironman (not for beginners)</game>");
		strcat(buf_meta, "<game>TomeNET Ironman</game>");
  #else
   #ifdef FUN_SERVER
		strcat(buf_meta, "<game>TomeNET Fun</game>");
   #else
                strcat(buf_meta, "<game>TomeNET</game>");
   #endif
  #endif
 #endif
#endif
                /* Append the version number */
                snprintf(temp, sizeof(temp), "<version>%d.%d.%d%s</version></server>", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, SERVER_VERSION_TAG);
                strcat(buf_meta, temp);
	}
	
	/* Send data to metaserver */
	if (send(meta_fd, buf_meta, strlen(buf_meta), 0) == -1) {
#ifdef WINDOWS
		int errval = WSAGetLastError();
		if (errval != WSAEWOULDBLOCK) {
#else
		int errval = errno;
		if (errval != EINPROGRESS && errval != EWOULDBLOCK) {
#endif
			s_printf("Writing to meta socket failed! (errno = %d)\n", errval);
		} else {
			s_printf("Writing to meta socket would block!\n");
		}
		
		/* Failed to connect or send data */
		meta_needs_update = FALSE;
		return FALSE;
	}
	
	/* Toggle the update needed flag */
	meta_needs_update = FALSE;
	return TRUE;
}

/*
 * Close the metaserver connection
 */
static void meta_close(void) {
	if (meta_fd != -1) {
		if (meta_output_installed) {
			remove_output(meta_fd);
			meta_output_installed = FALSE;
		}
		close(meta_fd);
		meta_fd = -1;
	}
	
	meta_state = META_NONE;
}

/*
 * Called from sched() to inform us that the metaserver connection has been
 * established
 */
static void meta_sched_output_callback(int fd, int arg) {
	if (meta_fd == fd) {
		meta_state = META_CONNECTED;
		remove_output(fd);
	}
}

/*
 * Called from Report_to_meta() in nserver.c
 */
void meta_report(int flag) {
	if (flag & META_START) {
		meta_init();
		if (meta_state == META_DISABLED) {
			meta_state = META_NONE;
		}
		meta_needs_update = TRUE;
	} else if (flag & META_DIE) {
		meta_close();
		
		/* Block the timer so that connect() succeeds */
		block_timer();
		meta_connect(TRUE);
		meta_write(flag);
		allow_timer();
		meta_close();
		meta_state = META_DISABLED;
	} else if (flag & META_UPDATE) {
		meta_needs_update = TRUE;
	}
}
