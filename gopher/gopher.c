/**
 * gopher.c
 * A portable, single header/source, Gopher protocol implementation.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "gopher.h"

/* General includes. */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
	#include <tchar.h>
#else
	#include <unistd.h>
#endif /* _WIN32 */

/* Networking includes. */
#ifdef _WIN32
	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <netdb.h>
#endif /* _WIN32 */

/* Cross-platform socket function return error code. */
#ifndef SOCKET_ERROR
	#define SOCKET_ERROR (-1)
#endif /* !SOCKET_ERROR */

/* Cross-platform representation of an invalid socket file descriptor. */
#ifndef INVALID_SOCKET
	#ifdef _WIN32
		#define INVALID_SOCKET (SOCKET)(~0)
	#else
		#define INVALID_SOCKET (-1)
	#endif /* _WIN32 */
#endif /* !INVALID_SOCKET */

/* Cross-platform shim for socket error codes. */
#ifdef _WIN32
	#define sockerrno WSAGetLastError()
#else
	#define sockerrno errno
#endif /* _WIN32 */

/* Ensure we have ssize_t. */
#ifndef ssize_t
	#ifdef _WIN32
		typedef LONG_PTR ssize_t;
	#endif /* _WIN32 */
#endif /* ssize_t */

/* Ensure we have snprintf on Windows. */
#if defined(_WIN32) && !defined(snprintf)
	#define snprintf _snprintf
#endif /* defined(_WIN32) && !defined(snprintf) */

#ifdef _WIN32
/* FormatMessage default flags. */
#define FORMAT_MESSAGE_FLAGS \
	(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM)

/* FormatMessage default language. */
#define FORMAT_MESSAGE_LANG MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)
#endif /* _WIN32 */

/* Gopher line receive buffer size. */
#define RECV_LINE_BUF 200

/* Log levels. */
typedef enum {
	LOG_FATAL = 0,
	LOG_ERROR,
	LOG_WARNING,
	LOG_INFO
} log_level_t;

#ifdef DEBUG
/* Logging private methods. */
void log_printf(log_level_t level, const char *format, ...);
void log_errno(log_level_t level, const char *msg);
void log_sockerrno(log_level_t level, const char *msg, int err);
#else
#define log_printf(...) (void)0
#define log_errno(l, m) (void)0
#define log_sockerrno(l, m, e) (void)0
#endif /* DEBUG */

/* Private utility methods. */
char *strcatp(char *dest, const char *src);
const char *strdupsep(char **buf, const char *str, char sep);

/* Private methods. */
int sockaddrstr(char **buf, const struct sockaddr *sock_addr);
int gopher_getaddrinfo(const gopher_addr_t *addr, struct addrinfo **ai);
gopher_item_t *gopher_item_new(gopher_type_t type, const char *label);
gopher_dir_t *gopher_dir_new(gopher_addr_t *addr);

/*
 * +===========================================================================+
 * |                                                                           |
 * |                          Gopherspace Addressing                           |
 * |                                                                           |
 * +===========================================================================+
 */

/**
 * Allocates and populates a gopherspace address object.
 *
 * @warning This function dinamically allocates memory.
 *
 * @param host     Optional. Domain name or IP address of the Gopher server.
 * @param port     Port to use for communicating with the Gopher server.
 * @param selector Optional. Selector of the content to retrieve.
 *
 * @return Newly populated gopherspace address object. NULL if an error
 *         occurred. Check errno case of failure.
 *
 * @see gopher_addr_free
 */
gopher_addr_t *gopher_addr_new(const char *host, uint16_t port,
							   const char *selector) {
	gopher_addr_t *addr;

	/* Allocate the object. */
	addr = (gopher_addr_t *)malloc(sizeof(gopher_addr_t));
	if (addr == NULL) {
		log_errno(LOG_ERROR, "Failed to allocate memory for gopherspace "
			"address");
		return NULL;
	}

	/* Populate the object. */
	addr->host = (host) ? strdup(host) : NULL;
	addr->port = port;
	addr->selector = (selector) ? strdup(selector) : NULL;
	addr->sockfd = INVALID_SOCKET;
	addr->ipaddr = NULL;
	addr->ipaddr_len = 0;

	return addr;
}

/**
 * Parses a gopherspace address from an URI conforming to RFC 4266.
 *
 * @warning This function dinamically allocates memory.
 *
 * @param uri  Universal Resource Identifier of the gopherspace.
 * @param type Optional. Stores the parsed type. If URI didn't contain one a
 *             NUL terminator will be returned.
 *
 * @return Newly parsed gopherspace address object or NULL if a parsing error
 *         occurred. Check errno case of failure.
 *
 * @see gopher_addr_new
 * @see gopher_addr_free
 */
gopher_addr_t *gopher_addr_parse(const char *uri, gopher_type_t *type) {
	gopher_addr_t *addr;
	const char *p;
	const char *pend;
	
	/* Check if it starts with protocol. */
	p = strstr(uri, "gopher://");
	if (p != NULL) {
		/* Jump past the protocol part. */
		p += 9;
	} else {
		/* Ensure we have the right protocol. */
		if (strstr(uri, "://") != NULL) {
			log_printf(LOG_ERROR, "Tried parsing URI for other protocol\n");
			return NULL;
		}
	}
	
	/* Create the address object. */
	addr = gopher_addr_new(NULL, 70, NULL);
	if (addr == NULL)
		return NULL;

	/* Get host from URI. */
	pend = strpbrk(p, ":/");
	if (pend == NULL) {
		/* Top level URI. */
		addr->host = strdup(p);
		return addr;
	}
	strdupsep(&addr->host, p, *pend);

	/* Get port if there's one. */
	if (*pend == ':') {
		char *port;

		/* Get port string. */
		p = pend + 1;
		pend = strpbrk(p, "/");
		if (pend == NULL) {
			port = strdup(p);
		} else {
			strdupsep(&port, p, *pend);
		}
		
		/* Set port in address object. */
		addr->port = (uint16_t)atoi(port);
		free(port);
	}
	
	/* Get type identifier. */
	p = pend + 1;
	if (*p == '\0')
		return addr;
	if (type)
		*type = (gopher_type_t)*p;
	p++;
	
	/* Get the selector. */
	if (*(p + 1) == '\0')
		return addr;
	addr->selector = strdup(p);

	return addr;
}

/**
 * Resolves an IP address structure for connecting via plain sockets from a
 * gopherspace address object.
 *
 * @warning This function dinamically allocates memory.
 *
 * @param addr Gopherspace address object.
 * @param ai   IP address information structure to be allocated and populated.
 *
 * @return 0 if the operation was successful. Check return against strerror() in
 *         case of failure.
 *
 * @see getaddrinfo
 */
int gopher_getaddrinfo(const gopher_addr_t *addr, struct addrinfo **ai) {
	struct addrinfo hints;
	char port[6];
	
	/* Build up the hints for the address we want to resolve. */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	/* Convert port to string. */
	snprintf(port, 6, "%u", addr->port);
	port[5] = '\0';

	return getaddrinfo(addr->host, port, &hints, ai);
}

/**
 * Converts a gopherspace address object into an URL string.
 *
 * @warning This function dinamically allocates memory.
 *
 * @param addr Gopherspace address object.
 * @param type Entry type character. Use NUL terminator if should be omitted.
 *
 * @return URL string representation of the gopherspace address object.
 *
 * @see gopher_addr_print
 */
char *gopher_addr_str(const gopher_addr_t *addr, gopher_type_t type) {
	char port[6];
	char *url;
	char *buf;
	size_t len;

	/* Do we even have anything to do here? */
	if (addr == NULL)
		return NULL;

	/* Convert port number to string. */
	itoa(addr->port, port, 10);

	/* Estimate the length of the string needed to store the URL. */
	len = 9 + 1;  /* gopher:// + NUL */
	len += strlen(addr->host) + 1;
	len += strlen(port) + 1;
	if ((type != GOPHER_TYPE_UNKNOWN) && (type != GOPHER_TYPE_INTERNAL))
		len += 1;
	if (addr->selector)
		len += strlen(addr->selector) + 1;

	/* Allocate memory for the URL. */
	url = (char *)malloc(len * sizeof(char));
	if (url == NULL) {
		log_errno(LOG_ERROR, "Failed to allocate memory for gopherspace "
			"address URL string");
		return NULL;
	}

	/* Ensure we have a type in the URL always. As specified in RFC 4266 */
	if ((type == GOPHER_TYPE_UNKNOWN) || (type == GOPHER_TYPE_INTERNAL))
		type = GOPHER_TYPE_DIR;

	/* Build up the URL. */
	buf = strcatp(url, "gopher://");
	buf = strcatp(buf, addr->host);
	*buf++ = ':';
	buf = strcatp(buf, port);
	*buf++ = '/';
	*buf = '\0';
	if (addr->selector) {
		*buf++ = (char)type;
		buf = strcatp(buf, addr->selector);
	}

	return url;
}

/**
 * Prints out the gopherspace address object internals for debugging.
 *
 * @param addr Gopherspace address object.
 */
void gopher_addr_print(const gopher_addr_t *addr) {
	char *url;

	/* Is this a valid object? */
	if (addr == NULL) {
		printf("(null)\n");
		return;
	}
	
	/* Print out the object data. */
	url = gopher_addr_str(addr, GOPHER_TYPE_UNKNOWN);
	printf("%s\n", url);
	free(url);
}

/**
 * Frees an gopherspace address object.
 *
 * @param addr Gopherspace address object to be free'd.
 */
void gopher_addr_free(gopher_addr_t *addr) {
	/* Is this even necessary? */
	if (addr == NULL)
		return;

	/* Free the object's members. */
	if (addr->host)
		free(addr->host);
	addr->port = 0;
	if (addr->selector)
		free(addr->selector);
	if (addr->ipaddr)
		free(addr->ipaddr);
	if (addr->sockfd != INVALID_SOCKET) {
		log_printf(LOG_WARNING, "Disconnecting the socket on address free\n");
		gopher_disconnect(addr);
	}
	
	/* Free the object itself. */
	free(addr);
}

/*
 * +===========================================================================+
 * |                                                                           |
 * |                            Connection Handling                            |
 * |                                                                           |
 * +===========================================================================+
 */

/**
 * Establishes a connection to a Gopher server.
 *
 * @param addr Gopherspace address object.
 *
 * @return 0 if the operation was successful. Check return against strerror() in
 *         case of failure.
 *
 * @see gopher_disconnect
 */
int gopher_connect(gopher_addr_t *addr) {
	int ret;
	struct addrinfo *query;
	struct addrinfo *ai;
	
	/* Resolve the server's IP address. */
	ret = gopher_getaddrinfo(addr, &query);
	if (ret != 0) {
		log_printf(LOG_ERROR, "Failed to get address IP: (%d) %s\n", ret,
			gai_strerror(ret));
		freeaddrinfo(query);
		return ret;
	}

	/* Search for a resolved address that is compatible. */
	ai = NULL;
	for (ai = query; ai != NULL; ai = ai->ai_next) {
		if (ai->ai_family == AF_INET)
			break;
	}
	if (ai == NULL) {
		log_printf(LOG_ERROR, "Couldn't resolve an address for %s\n",
			addr->host);
		freeaddrinfo(query);
		return EAFNOSUPPORT;
	}

	/* Allocate memory for the IP address. */
	addr->ipaddr_len = ai->ai_addrlen;
	addr->ipaddr = (struct sockaddr_in *)malloc(addr->ipaddr_len);
	if (addr->ipaddr == NULL) {
		log_printf(LOG_ERROR, "Failed to allocate memory for server IP address"
			"\n");
		freeaddrinfo(query);
		return ENOMEM;
	}
	
	/* Copy the server's IP address and free the resolve object. */
	memcpy(addr->ipaddr, ai->ai_addr, addr->ipaddr_len);
	freeaddrinfo(query);
	query = NULL;
	
	/* Get a socket file descriptor for our connection. */
	addr->sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (addr->sockfd == INVALID_SOCKET) {
		log_sockerrno(LOG_FATAL, "Couldn't get a socket for our connection",
			sockerrno);
		return sockerrno;
	}
	
#ifdef DEBUG
	/* Log information about the address. */
	if (1) {
		char *buf;
		ret = sockaddrstr(&buf, (struct sockaddr *)addr->ipaddr);
		if (ret == 0) {
			log_printf(LOG_INFO, "sockaddr addr->ipaddr %s:%d\n", buf,
				ntohs(addr->ipaddr->sin_port));
			free(buf);
		} else {
			log_errno(LOG_ERROR, "Couldn't get debug address information");
		}
	}
#endif /* DEBUG */

	/* Connect ourselves to the address. */
	if (connect(addr->sockfd, (struct sockaddr *)addr->ipaddr,
				addr->ipaddr_len) == SOCKET_ERROR) {
		log_sockerrno(LOG_ERROR, "Couldn't connect to server", sockerrno);
		return sockerrno;
	}

	return ret;
}

/**
 * Disconnects gracefully from a Gopher server.
 *
 * @param addr Gopherspace address object.
 *
 * @return 0 if the operation was successful. Check return against strerror() in
 *         case of failure.
 *
 * @see gopher_connect
 */
int gopher_disconnect(gopher_addr_t *addr) {
	char c;
	int ret;

	/* Is this even a valid address or socket? */
	if ((addr == NULL) || (addr->sockfd == INVALID_SOCKET))
		return EBADF;

	/* Check if a shutdown is needed. */
	if (recv(addr->sockfd, &c, 1, MSG_PEEK) != 0) {
		log_printf(LOG_INFO, "Socket still connected, performing shutdown\n");

		/* Shutdown the connection. */
#ifdef _WIN32
		ret = shutdown(addr->sockfd, SD_BOTH);
#else
		ret = shutdown(addr->sockfd, SHUT_RDWR);
#endif /* _WIN32 */
		if (ret == SOCKET_ERROR) {
			log_sockerrno(LOG_WARNING, "Failed to shutdown connection",
				sockerrno);
		}
	}

	/* Close the socket file descriptor. */
#ifdef _WIN32
	ret = closesocket(addr->sockfd);
#else
	ret = close(addr->sockfd);
#endif /* _WIN32 */
	addr->sockfd = INVALID_SOCKET;
	if (ret == SOCKET_ERROR)
		log_sockerrno(LOG_ERROR, "Failed to close socket", sockerrno);

	return ret;
}

/*
 * +===========================================================================+
 * |                                                                           |
 * |                            Directory Handling                             |
 * |                                                                           |
 * +===========================================================================+
 */

/**
 * Allocates and initializes a Gopher directory object.
 *
 * @warning This function dinamically allocates memory.
 *
 * @param addr Gopherspace address object.
 *
 * @return Newly initialized Gopher directory object.
 *
 * @see gopher_dir_free
 */
gopher_dir_t *gopher_dir_new(gopher_addr_t *addr) {	
	gopher_dir_t *dir;

	/* Allocate the object. */
	dir = (gopher_dir_t *)malloc(sizeof(gopher_dir_t));
	if (dir == NULL) {
		log_errno(LOG_ERROR, "Failed to allocate memory for Gopher directory");
		return NULL;
	}

	/* Initialize the object. */
	dir->addr = addr;
	dir->prev = NULL;
	dir->next = NULL;
	dir->items = NULL;
	dir->items_len = 0;
	dir->err_count = 0;

	return dir;
}

/**
 * Requests a directory from a Gopher server.
 *
 * @warning This function dinamically allocates memory.
 *
 * @param addr Gopherspace address object already connected to the server.
 * @param dir  Pointer to where the results of the directory will be stored.
 *
 * @return 0 if the operation was successful. Check return against strerror() in
 *         case of failure.
 *
 * @see gopher_dir_free
 */
int gopher_dir_request(gopher_addr_t *addr, gopher_dir_t **dir) {
	gopher_dir_t *pd;
	gopher_item_t *prev;
	char *line;
	size_t len;
	int ret;
	int termlined;
	
	/* Send selector of our request. */
	ret = gopher_send_line(addr, (addr->selector) ? addr->selector : "", NULL);
	if (ret != 0) {
		log_errno(LOG_ERROR, "Failed to send line during directory request");
		return ret;
	}
	
	/* Initialize directory object. */
	*dir = gopher_dir_new(addr);
	pd = *dir;
	if (*dir == NULL) {
		log_errno(LOG_ERROR, "Failed to initialize directory object");
		return -1;
	}

	/* Go through lines received from the server. */
	termlined = 0;
	prev = NULL;
	line = NULL;
	len = 0;
	while (gopher_recv_line(addr, &line, &len) == 0) {
		gopher_item_t *item;
		
		/* Check if we have reached the termination line. */
		if (gopher_is_termline(line)) {
			termlined = 1;
			goto nextline;
		}

		/* Check if we have terminated the connection. */
		if (line == NULL)
			break;

		/* Check if a monstrosity of a server just sent a blank line. */
		if ((line[0] == '\r') && (line[1] == '\n')) {
			pd->err_count++;
			goto nextline;
		}

		/* Parse line item. */
		ret = gopher_item_parse(&item, line);
		if (ret != 0) {
			char *msg;
			log_printf(LOG_WARNING, "Failed to parse line item during "
				"directory request: \"%s\"\n", line);
			pd->err_count++;

			msg = (char *)malloc((20 + strlen(line)) * sizeof(char));
			sprintf(msg, "PARSING FAILED: \"%s\"", line);
			item = gopher_item_new(GOPHER_TYPE_INTERNAL, msg);
			free(msg);
			
#ifdef DEBUG
			free(line);
			break;
#endif
		}	

		/* Check if a monstrosity of a server just sent an incomplete item. */
		if (item->addr == NULL) {
			/* Fix this idiotic problem. */
			item->addr = gopher_addr_new("_server.fail", 0, "INCOMPLETE_LINE");
			pd->err_count++;
		}

		/* Push the item into the directory item stack. */
		if (pd->items == NULL)
			pd->items = item;
		if (prev != NULL)
			prev->next = item;
		prev = item;
		pd->items_len++;

nextline:
		/* Clean up any temporary resources. */
		free(line);
		line = NULL;
	}

	/* Check if server never sent the termination dot. */
	if (!termlined) {
		log_printf(LOG_WARNING, "Server never sent termination dot\n");
		pd->err_count++;
	}
	
	return ret;
}

/**
 * Frees a Gopher directory object.
 *
 * @param dir       Gopher directory object to be free'd.
 * @param recurse   Bitwise field to recursively free its history stack.
 * @param inclusive Should we also free ourselves? If FALSE will only free
 *                  history in specified recurse direction.
 */
void gopher_dir_free(gopher_dir_t *dir, gopher_recurse_dir_t recurse,
		int inclusive) {
	/* Is this even necessary? */
	if (dir == NULL)
		return;

	/* Ensure we remove ourselves from other objects in the stack. */
	if (inclusive) {
		if (dir->next)
			dir->next->prev = NULL;
		if (dir->prev)
			dir->prev->next = NULL;
	}

	/* Free history backwards. */
	if (dir->prev && (recurse & RECURSE_BACKWARD)) {
		gopher_dir_free(dir->prev, RECURSE_BACKWARD, 1);
		dir->prev = NULL;
	}

	/* Free history forwards. */
	if (dir->next && (recurse & RECURSE_FORWARD)) {
		gopher_dir_free(dir->next, RECURSE_FORWARD, 1);
		dir->next = NULL;
	}

	/* Free ourselves too. */
	if (inclusive) {
		/* Free the object's members. */
		dir->items_len = 0;
		if (dir->items)
			gopher_item_free(dir->items, RECURSE_FORWARD);
		if (dir->addr)
			gopher_addr_free(dir->addr);

		/* Free the object itself. */
		free(dir);
	}
}

/*
 * +===========================================================================+
 * |                                                                           |
 * |                             Item Line Parsing                             |
 * |                                                                           |
 * +===========================================================================+
 */

/**
 * Allocates and initializes a Gopher line item object.
 *
 * @warning This function dinamically allocates memory.
 *
 * @param type  File type of the item.
 * @param label Optional. Label of the item.
 *
 * @return Newly initialized Gopher line item object.
 *
 * @see gopher_item_free
 */
gopher_item_t *gopher_item_new(gopher_type_t type, const char *label) {	
	gopher_item_t *item;

	/* Allocate the object. */
	item = (gopher_item_t *)malloc(sizeof(gopher_item_t));
	if (item == NULL) {
		log_errno(LOG_ERROR, "Failed to allocate memory for Gopher item");
		return NULL;
	}

	/* Initialize the object. */
	item->type = type;
	item->label = NULL;
	if (label)
		item->label = strdup(label);
	item->addr = NULL;
	item->next = NULL;

	return item;
}

/**
 * Gets the URL which points to a respective item object.
 *
 * @warning This function dinamically allocates memory.
 *
 * @param item Gopher item object.
 *
 * @return Standardized URL representation of the item's location.
 *
 * @see gopher_addr_str
 * @see gopher_item_print
 */
char *gopher_item_url(const gopher_item_t *item) {
	return gopher_addr_str(item->addr, item->type);
}

/**
 * Parses a line received from a server into an item object.
 *
 * @warning This function dinamically allocates memory.
 *
 * @param item Pointer to location where the parsed line will be stored.
 * @param line Line as received from the server.
 *
 * @return 0 if the operation was successful. Check return against strerror() in
 *         case of failure.
 *
 * @see gopher_recv_line
 * @see gopher_item_free
 */
int gopher_item_parse(gopher_item_t **item, const char *line) {
	gopher_item_t *it;
	const char *p;
	char *selector;
	char *host;
	char *port;

	/* Am I a joke to you? */
	if ((item == NULL) || (line == NULL)) {
		log_printf(LOG_ERROR, "Item or line for parsing are NULL\n");
		return -1;
	}

	/* I can't parse a dot. */
	if (gopher_is_termline(line)) {
		log_printf(LOG_ERROR, "Tried to parse the termination line\n");
		return -1;
	}
	
	/* Check if a monstrosity of a server just sent a blank line. */
	if (line[0] == '\r') {
		log_printf(LOG_ERROR, "Tried parsing an empty line\n");
		return -1;
	}

	/* Initialize the item object. */
	p = line;
	*item = gopher_item_new((gopher_type_t)*p++, NULL);
	it = *item;
	if (it == NULL) {
		log_errno(LOG_ERROR, "Failed to allocate memory for parsed line item");
		return errno;
	}

	/* Initialize some parsed properties with sane defaults. */
	selector = NULL;
	host = NULL;
	port = NULL;

	/* Start parsing the line with the type and label. */
	p = strdupsep(&it->label, p, '\t');
	if (p == NULL) {
		log_errno(LOG_ERROR, "Failed to duplicate label string");
		gopher_item_free(it, 1);
		it = NULL;
		goto cleanup;
	}

	/* Check if idiotic server just sent line without the rest of the fields. */
	if (*p == '\0') {
		log_printf(LOG_WARNING, "Parsed incomplete line\n");
		it->label[strlen(it->label) - 2] = '\0';
		it->addr = NULL;
		return errno;
	}

	/* Parse the selector. */
	p++;
	p = strdupsep(&selector, p, '\t');
	if (p == NULL) {
		log_errno(LOG_ERROR, "Failed to duplicate selector string");
		gopher_item_free(it, 1);
		it = NULL;
		goto cleanup;
	}

	/* Parse the host. */
	p++;
	p = strdupsep(&host, p, '\t');
	if (p == NULL) {
		log_errno(LOG_ERROR, "Failed to duplicate host string");
		gopher_item_free(it, 1);
		it = NULL;
		goto cleanup;
	}

	/* Parse the port. */
	p++;
	p = strdupsep(&port, p, '\r');
	if (p == NULL) {
		log_errno(LOG_ERROR, "Failed to duplicate port string");
		gopher_item_free(it, 1);
		it = NULL;
		goto cleanup;
	}

	/* Finally create the address object. */
	it->addr = gopher_addr_new(host, (uint16_t)atoi(port), selector);
	if (it->addr == NULL) {
		log_errno(LOG_ERROR, "Failed to create address object for parsed line");
		gopher_item_free(it, 1);
		it = NULL;
		goto cleanup;
	}

cleanup:
	/* Free up resources. */
	if (selector)
		free(selector);
	if (host)
		free(host);
	if (port)
		free(port);
	
	return 0;
}

/**
 * Prints debugging information about a Gopher item type.
 *
 * @param item Gopher item to have its type printed out.
 */
void gopher_item_print_type(const gopher_item_t *item) {
	/* Regex to get this from gopher_type_t definition: */
	/* s/\s+GOPHER_TYPE_([^\s]+)\s+= '([^'])'(,?)/
	   case GOPHER_TYPE_$1:\n\tprintf("[$1]");\n\tbreak;\n/g */
	switch (item->type) {
		case GOPHER_TYPE_INTERNAL:
			printf("<[INTERNAL]>");
			break;
		case GOPHER_TYPE_TEXT:
			printf("[TEXT]");
			break;
		case GOPHER_TYPE_DIR:
			printf("[DIR]");
			break;
		case GOPHER_TYPE_CSO:
			printf("[CSO]");
			break;
		case GOPHER_TYPE_ERROR:
			printf("[ERROR]");
			break;
		case GOPHER_TYPE_BINHEX:
			printf("[BINHEX]");
			break;
		case GOPHER_TYPE_DOS:
			printf("[DOS]");
			break;
		case GOPHER_TYPE_UNIX:
			printf("[UNIX]");
			break;
		case GOPHER_TYPE_SEARCH:
			printf("[SEARCH]");
			break;
		case GOPHER_TYPE_TELNET:
			printf("[TELNET]");
			break;
		case GOPHER_TYPE_BINARY:
			printf("[BINARY]");
			break;
		case GOPHER_TYPE_MIRROR:
			printf("[MIRROR]");
			break;
		case GOPHER_TYPE_TN3270:
			printf("[TN3270]");
			break;
		case GOPHER_TYPE_GIF:
			printf("[GIF]");
			break;
		case GOPHER_TYPE_IMAGE:
			printf("[IMAGE]");
			break;
		case GOPHER_TYPE_BITMAP:
			printf("[BITMAP]");
			break;
		case GOPHER_TYPE_MOVIE:
			printf("[MOVIE]");
			break;
		case GOPHER_TYPE_AUDIO:
			printf("[AUDIO]");
			break;
		case GOPHER_TYPE_DOC:
			printf("[DOC]");
			break;
		case GOPHER_TYPE_HTML:
			printf("[HTML]");
			break;
		case GOPHER_TYPE_INFO:
			printf("[INFO]");
			break;
		case GOPHER_TYPE_PNG:
			printf("[PNG]");
			break;
		case GOPHER_TYPE_WAV:
			printf("[WAV]");
			break;
		case GOPHER_TYPE_PDF:
			printf("[PDF]");
			break;
		case GOPHER_TYPE_XML:
			printf("[XML]");
			break;
		default:
			printf("[UNKNOWN]");
			break;
	}
}

/**
 * Prints debugging information about a Gopher item object.
 *
 * @param item Gopher item to have its information printed out.
 */
void gopher_item_print(const gopher_item_t *item) {
	/* Is this a valid object? */
	if (item == NULL) {
		printf("(null)\n");
		return;
	}
	
	/* Print out the object data. */
	gopher_item_print_type(item);
	printf("\t'%s'\t'%s'\t%s:%u", item->label, item->addr->selector,
		item->addr->host, item->addr->port);
	if (item->next != NULL) {
		printf("\t->%p\n", item->next);
	} else {
		printf("\n");
	}
}

/**
 * Frees a Gopher item object.
 *
 * @param item    Gopher item object to be free'd.
 * @param recurse Recursively free next items as well?
 */
void gopher_item_free(gopher_item_t *item, gopher_recurse_dir_t recurse) {
	/* Is this even necessary? */
	if (item == NULL)
		return;

	/* Free the object's members. */
	if (item->label)
		free(item->label);
	if (item->addr)
		gopher_addr_free(item->addr);
	if (item->next && (recurse == RECURSE_FORWARD))
		gopher_item_free(item->next, recurse);

	/* Free the object itself. */
	free(item);
}

/**
 * Checks if a received line is in fact the termination one with a single dot.
 *
 * @param line Received line to be checked.
 *
 * @return TRUE if the line is the last one to be transmitted before closing the
 *         connection.
 */
int gopher_is_termline(const char *line) {
	return (line != NULL) && (line[0] == '.') && (line[1] == '\r') &&
		(line[2] == '\n');
}

/*
 * +===========================================================================+
 * |                                                                           |
 * |                     Socket and Networking Abstrations                     |
 * |                                                                           |
 * +===========================================================================+
 */

/**
 * Sends a raw data buffer to a gopher server.
 *
 * @param addr     Gopherspace address object.
 * @param buf      Data to be sent.
 * @param len      Length of the data to be sent.
 * @param sent_len Pointer to store the number of bytes actually sent. Ignored
 *                 if NULL is passed.
 *
 * @return 0 if the operation was successful. Check return against strerror() in
 *         case of failure.
 *
 * @see send
 */
int gopher_send_raw(const gopher_addr_t *addr, const void *buf, size_t len,
					size_t *sent_len) {
	ssize_t bytes_sent;

	/* Try to send some information through a socket. */
	bytes_sent = send(addr->sockfd, buf, len, 0);
	if (bytes_sent == SOCKET_ERROR) {
		log_sockerrno(LOG_ERROR, "Failed to send data over socket", sockerrno);
		return sockerrno;
	}

	/* Return the number of bytes sent. */
	if (sent_len != NULL)
		*sent_len = bytes_sent;

	return 0;
}

/**
 * Sends a string to a gopher server.
 *
 * @param addr     Gopherspace address object.
 * @param buf      String to be sent.
 * @param sent_len Pointer to store the number of bytes actually sent. Ignored
 *                 if NULL is passed.
 *
 * @return 0 if the operation was successful. Check return against strerror() in
 *         case of failure.
 *
 * @see gopher_send_raw
 */
int gopher_send(const gopher_addr_t *addr, const char *buf, size_t *sent_len) {
	return gopher_send_raw(addr, (const void *)buf, strlen(buf), sent_len);
}

/**
 * Sends a string to a gopher server automatically appending a CRLF.
 *
 * @param addr     Gopherspace address object.
 * @param buf      String to be sent.
 * @param sent_len Pointer to store the number of bytes actually sent. Ignored
 *                 if NULL is passed.
 *
 * @return 0 if the operation was successful. Check return against strerror() in
 *         case of failure.
 *
 * @see gopher_send
 */
int gopher_send_line(const gopher_addr_t *addr, const char *buf,
					 size_t *sent_len) {
	size_t len;
	char *nbuf;
	char *n;
	const char *b;
	int ret;

	/* Get string length and allocate a buffer big enough for it plus CRLF. */
	len = strlen(buf) + 2;
	nbuf = (char *)malloc((len + 1) * sizeof(char));
	
	/* Copy the string over and append CRLF very fast. */
	b = buf;
	n = nbuf;
	while (*b != '\0') {
		*n = *b;
		n++;
		b++;
	}
	*n = '\r';
	n++;
	*n = '\n';
	n++;
	*n = '\0';
	
#ifdef DEBUG
	log_printf(LOG_INFO, "Sent: \"%s\"\n", nbuf);
#endif /* DEBUG */
	
	/* Send the line over the network. */
	ret = gopher_send_raw(addr, (void *)nbuf, len, sent_len);

	/* Free temporary resources. */
	free(nbuf);

	return ret;
}

/**
 * Receive raw data from a gopher server.
 *
 * @param addr     Gopherspace address object.
 * @param buf      Buffer to store the received data.
 * @param buf_len  Length of the buffer to store the data.
 * @param recv_len Optional. Pointer to store the number of bytes actually
 *                 received.
 * @param flags    Flags to be passed to recv().
 *
 * @return 0 if the operation was successful. Check return against strerror() in
 *         case of failure.
 *
 * @see recv
 */
int gopher_recv_raw(const gopher_addr_t *addr, void *buf, size_t buf_len,
					size_t *recv_len, int flags) {
	size_t bytes_recv;
	ssize_t len;

	/* Check if we have a valid file descriptor. */
	if (addr->sockfd == INVALID_SOCKET)
		return EBADF;

	/* Receive data from the socket. */
	len = recv(addr->sockfd, buf, buf_len, flags);
	if (len == SOCKET_ERROR) {
		log_sockerrno(LOG_ERROR, "Failed to receive data from socket",
			sockerrno);
		return sockerrno;
	}
	bytes_recv = len;

	/* Return the number of bytes received. */
	if (recv_len != NULL)
		*recv_len = bytes_recv;

	/* Check if the connection was closed gracefully by the server. */
	if ((bytes_recv == 0) && !(flags | MSG_PEEK))
		log_printf(LOG_INFO, "Connection closed gracefully by server\n");

	return 0;
}

/**
 * Receive an entire line from a gopher server.
 *
 * @warning This function dinamically allocates memory.
 *
 * @param addr Gopherspace address object.
 * @param line Pointer to location where the received line including the CRLF
 *             characters will be stored.
 * @param len  Optional. Pointer to store the number of bytes actually received.
 *
 * @return 0 if the operation was successful. Check return against strerror() in
 *         case of failure.
 *
 * @see gopher_recv_raw
 */
int gopher_recv_line(const gopher_addr_t *addr, char **line, size_t *len) {
	char peek[RECV_LINE_BUF];
	char *buf;
	size_t prev_line_len;
	size_t line_len;
	size_t recv_len;
	int ret;
	
	/* Ensure we read an entire line. */
	ret = 0;
	prev_line_len = 0;
	line_len = 0;
	*line = NULL;
	buf = NULL;
	while (ret == 0) {
		int found;
		size_t i;
		char *pb;

		/* Read incoming data. */
		prev_line_len = line_len;
		found = 0;
		ret = gopher_recv_raw(addr, peek, RECV_LINE_BUF - 1, &recv_len,
			MSG_PEEK);
		if ((ret == 0) && (recv_len == 0)) {
			*line = NULL;
			free(buf);
			return 0;
		}
		if ((ret != 0) || (recv_len == 0)) {
			log_printf(LOG_ERROR, "Failed to read received line\n");
			*line = NULL;
			if (len != NULL)
				*len = 0;
			free(buf);

			return ret;
		}
		peek[recv_len] = '\0';

		/* Go through the received data looking for the end of the line. */
		for (i = 0; i < recv_len; i++) {
			if ((peek[i] == '\n') || (peek[i] == '\r') &&
					((i + 1) < recv_len) && (peek[i + 1] == '\n')) {
				line_len += 2;
				found = '\r';

				/* Handle monstrosities that send LF instead of CRLF. */
				if (peek[i] == '\n') {
					log_printf(LOG_INFO, "Non-compliant line in peek'd buffer "
						"\"%s\"\n", peek);
					found = '\n';
				}

				goto concatrecv;
			}

			line_len++;
		}

concatrecv:
		/* Reallocate the buffer. */
		pb = buf;
		buf = realloc(pb, (line_len + 1) * sizeof(char));
		if (buf == NULL) {
			log_printf(LOG_ERROR, "Failed to reallocate line buffer\n");
			free(pb);
			*line = NULL;

			return errno;
		}

		/* Read previously peek'd data into buffer. */
		pb = buf + prev_line_len;
		ret = gopher_recv_raw(addr, pb, line_len - prev_line_len -
			(found == '\n'), &recv_len, 0);
		if (ret != 0) {
			log_printf(LOG_ERROR, "Failed to read received line\n");
			free(*line);
			*line = NULL;

			return ret;
		}
		buf[line_len] = '\0';
		
		/* Ensure we convert a non-compliant server into a compliant one. */
		if (found == '\n') {
			buf[line_len - 2] = '\r';
			buf[line_len - 1] = '\n';
		}

		/* Are we finished here? */
		if (found)
			ret = 1;
	}

	/* Return the received line and length with the CRLF characters. */
	*line = buf;
	if (len != NULL)
		*len = line_len;
	
	return 0;
}

#ifdef DEBUG

/*
 * +===========================================================================+
 * |                                                                           |
 * |                           Logging and Debugging                           |
 * |                                                                           |
 * +===========================================================================+
 */

/**
 * Converts an IPv4 or IPv6 address from binary to a presentation format string
 * representation.
 *
 * @warning This function dinamically allocates memory.
 *
 * @param buf       Pointer to a string that will be populated with the
 *                  presentation format IP address.
 * @param sock_addr Generic IPv4 or IPv6 structure containing the address to be
 *                  converted.
 *
 * @return 0 if the operation was successful. Check return against strerror() in
 *         case of failure.
 *
 * @see inet_ntop
 * @see WSAAddressToString
 */
int sockaddrstr(char **buf, const struct sockaddr *sock_addr) {
#ifdef _WIN32
	TCHAR tmp[INET6_ADDRSTRLEN];
	DWORD dwLen;

	dwLen = INET6_ADDRSTRLEN;
#else
	char tmp[INET6_ADDRSTRLEN];
#endif /* _WIN32 */

	/* Determine which type of IP address we are dealing with. */
	switch (sock_addr->sa_family) {
		case AF_INET:
#ifdef _WIN32
			WSAAddressToString((LPSOCKADDR)sock_addr,
				sizeof(struct sockaddr_in), NULL, tmp, &dwLen);
#else
			inet_ntop(AF_INET,
					  &(((const struct sockaddr_in *)sock_addr)->sin_addr),
					  tmp, INET_ADDRSTRLEN);
#endif /* _WIN32 */
			break;
		case AF_INET6:
#ifdef _WIN32
			WSAAddressToString((LPSOCKADDR)sock_addr,
				sizeof(struct sockaddr_in6), NULL, tmp, &dwLen);
#else
			inet_ntop(AF_INET6,
					  &(((const struct sockaddr_in6 *)sock_addr)->sin6_addr),
					  tmp, INET6_ADDRSTRLEN);
#endif /* _WIN32 */
			break;
		default:
			*buf = NULL;
			return EAFNOSUPPORT;
	}

#ifdef _WIN32
	/* Remove the port number from the string. */
	for (dwLen = 0; tmp[dwLen] != '\0'; dwLen++) {
		if (tmp[dwLen] == ':') {
			tmp[dwLen] = '\0';
			break;
		}
	}

	/* Convert our string to UTF-8 assigning it to the return value. */
	*buf = win_wcstombs(tmp);
#else
	/* Allocate space for our return string. */
	*buf = (char *)malloc((strlen(tmp) + 1) * sizeof(char));
	if (*buf == NULL) {
		log_errno(LOG_FATAL, "Failed to allocate memory for IP address string");
		return ENOMEM;
	}

	/* Copy our IP address over and return. */
	strcpy(*buf, tmp);
#endif /* _WIN32 */

	return 0;
}

/**
 * Logs any system errors that set the errno variable whenever dealing with
 * sockets.
 *
 * @param level Severity of the logged information.
 * @param msg   Error message to be associated with the error message determined
 *              by the errno value.
 */
void log_errno(log_level_t level, const char *msg) {
#ifdef _WIN32
	DWORD dwLastError;
	LPTSTR szErrorMessage;

	/* Get the descriptive error message from the system. */
	dwLastError = GetLastError();
	if (!FormatMessage(FORMAT_MESSAGE_FLAGS, NULL, dwLastError,
					   FORMAT_MESSAGE_LANG, (LPTSTR)&szErrorMessage, 0,
					   NULL)) {
		szErrorMessage = _wcsdup(_T("FormatMessage failed"));
	}

	/* Print the error message. */
	log_printf(level, "%s: (%d) %ls", msg, dwLastError, szErrorMessage);

	/* Free up any resources. */
	LocalFree(szErrorMessage);
#else
	/* Print the error message. */
	log_printf(level, "%s: (%d) %s\n", msg, errno, strerror(errno));
#endif /* _WIN32 */
}

/**
 * Logs any system errors that set the errno variable whenever dealing with
 * sockets.
 *
 * @param level Severity of the logged information.
 * @param msg   Error message to be associated with the error message determined
 *              by the errno value.
 * @param err   errno or equivalent error code value. (Ignored on POSIX systems)
 */
void log_sockerrno(log_level_t level, const char *msg, int err) {
#ifdef _WIN32
	LPTSTR szErrorMessage;

	/* Get the descriptive error message from the system. */
	if (!FormatMessage(FORMAT_MESSAGE_FLAGS, NULL, err, FORMAT_MESSAGE_LANG,
					   (LPTSTR)&szErrorMessage, 0, NULL)) {
		szErrorMessage = _wcsdup(_T("FormatMessage failed"));
	}

	/* Print the error message. */
	log_printf(level, "%s: WSAError (%d) %ls", msg, err, szErrorMessage);

	/* Free up any resources. */
	LocalFree(szErrorMessage);
#else
	(void)err;

	/* Print the error message. */
	log_printf(level, "%s: %s\n", msg, strerror(errno));
#endif /* _WIN32 */
}

/**
 * Prints out logging information with an associated log level tag using the
 * printf style of function.
 *
 * @param level  Severity of the logged information.
 * @param format Format of the desired output without the tag.
 * @param ...    Additional variables to be populated.
 */
void log_printf(log_level_t level, const char *format, ...) {
	va_list args;
#ifdef _WIN32
	char szmbMsg[255];
	TCHAR *szMsg;
#endif /* _WIN32 */

	/* Print the log level tag. */
	switch (level) {
		case LOG_FATAL:
			fprintf(stderr, "[FATAL] ");
			break;
		case LOG_ERROR:
			fprintf(stderr, "[ERROR] ");
			break;
		case LOG_WARNING:
			fprintf(stderr, "[WARNING] ");
			break;
		case LOG_INFO:
			fprintf(stderr, "[INFO] ");
			break;
		default:
			fprintf(stderr, "[UNKNOWN] ");
			break;
	}

	/* Print the actual message. */
	va_start(args, format);
#ifndef _WIN32
	vfprintf(stderr, format, args);
#else
	vsnprintf(szmbMsg, 254, format, args);
	szmbMsg[254] = '\0';
	szMsg = win_mbstowcs(szmbMsg);
	OutputDebugString(szMsg);
	free(szMsg);
#endif /* !_WIN32 */
	va_end(args);
}

#endif /* DEBUG */

/*
 * +===========================================================================+
 * |                                                                           |
 * |                             Utility Functions                             |
 * |                                                                           |
 * +===========================================================================+
 */

/**
 * Appends a string to another and returns a pointer to the NUL termination
 * character of the destination string.
 *
 * @warning This function has the same safety issues as strcat.
 *
 * @param dest String to be appended to. Ensure it can hold the new value.
 * @param src  String to be appended to the destination.
 *
 * @return Pointer to the termination character of the new concatenated string.
 *
 * @see strcat
 */
char *strcatp(char *dest, const char *src) {
	const char *s;
	char *p;

	/* Are we supposed to do anything? */
	if (dest == NULL)
		return NULL;
	if (src == NULL)
		return dest;
	if (*src == '\0') {
		*dest = '\0';
		return dest;
	}

	/* Copy the string over. */
	s = src;
	p = dest;
	do {
		*p++ = *s++;
	} while (*s != '\0');
	*p = '\0';

	return p;
}

/**
 * Duplicates a string until a separator or termination character.
 *
 * @warning This function dinamically allocates memory.
 *
 * @param buf Pointer to location where the string will be stored.
 * @param str Original string to be duplicated.
 * @param sep Separator character.
 *
 * @return Pointer to the separator or termination character in the original
 *         string.
 */
const char *strdupsep(char **buf, const char *str, char sep) {
	const char *s;
	const char *send;
	char *p;

	/* Get position of the separator. */
	s = str;
	do {
		send = s++;
	} while ((*send != sep) && (*send != '\0'));
	
	/* Detect invalid position. */
	if ((send - str) == 0) {
		*buf = strdup("");
		return str;
	}

	/* Allocate memory for our string. */
	*buf = (char *)malloc((send - str + 1) * sizeof(char));
	if (*buf == NULL) {
		log_errno(LOG_ERROR, "Failed to allocate memory for string "
			"duplication");
		return NULL;
	}

	/* Copy the string over. */
	s = str;
	p = *buf;
	do {
		*p++ = *s++;
	} while (s != send);
	*p = '\0';

	return send;
}

#ifdef _WIN32
/**
 * Converts a UTF-16 wide-character string into a UTF-8 multibyte string.
 *
 * @warning This function dinamically allocates memory.
 *
 * @param wstr UTF-16 string to be converted.
 *
 * @return UTF-8 multibyte converted string or NULL if an error occurred.
 *
 * @see WideCharToMultiByte
 */
char *win_wcstombs(const wchar_t *wstr) {
	char *str;
	int nLen;

	/* Get required buffer size and allocate some memory for it. */
	nLen = WideCharToMultiByte(CP_OEMCP, 0, wstr, -1, NULL, 0, NULL, NULL);
	if (nLen == 0)
		goto failure;
	str = (char *)malloc(nLen * sizeof(char));
	if (str == NULL)
		return NULL;

	/* Perform the conversion. */
	nLen = WideCharToMultiByte(CP_OEMCP, 0, wstr, -1, str, nLen, NULL, NULL);
	if (nLen == 0) {
failure:
		MessageBox(NULL, _T("Failed to convert UTF-16 string to UTF-8."),
			_T("String Conversion Failure"), MB_ICONERROR | MB_OK);

		return NULL;
	}

	return str;
}

/**
 * Converts a UTF-8 multibyte string into an UTF-16 wide-character string.
 * @warning This function allocates memory that must be free'd by you!
 *
 * @param str UTF-8 string to be converted.
 *
 * @return UTF-16 wide-character converted string or NULL if an error occurred.
 */
wchar_t *win_mbstowcs(const char *str) {
	wchar_t *wstr;

#ifdef _WIN32
	int nLen;

	/* Get required buffer size and allocate some memory for it. */
	wstr = NULL;
	nLen = MultiByteToWideChar(CP_OEMCP, 0, str, -1, NULL, 0);
	if (nLen == 0)
		goto failure;
	wstr = (wchar_t *)malloc(nLen * sizeof(wchar_t));
	if (wstr == NULL)
		return NULL;

	/* Perform the conversion. */
	nLen = MultiByteToWideChar(CP_OEMCP, 0, str, -1, wstr, nLen);
	if (nLen == 0) {
failure:
		MessageBox(NULL, _T("Failed to convert UTF-8 string to UTF-16."),
			_T("String Conversion Failure"), MB_ICONERROR | MB_OK);
		if (wstr)
			free(wstr);

		return NULL;
	}
#else
	size_t len;

	/* Allocate some memory for our converted string. */
	len = mbstowcs(NULL, str, 0) + 1;
	wstr = (wchar_t *)malloc(len * sizeof(wchar_t));
	if (wstr == NULL)
		return NULL;

	/* Perform the string conversion. */
	len = mbstowcs(wstr, str, len);
	if (len == (size_t)-1) {
		free(wstr);
		return NULL;
	}
#endif /* _WIN32 */

	return wstr;
}
#endif /* _WIN32 */
