/**
 * gopher.c
 * A portable Gopher protocol implementation.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "gopher.h"

/* General includes. */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
	#include <tchar.h>
#else
	#include <errno.h>
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

/* Private definitions. */
#ifdef _WIN32
	/* Shim things that Microsoft forgot to include in their implementation. */
	#ifndef in_addr_t
		typedef unsigned long in_addr_t;
	#endif /* in_addr_t */
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

#ifdef _WIN32
/* FormatMessage default flags. */
#define FORMAT_MESSAGE_FLAGS \
	(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM)

/* FormatMessage default language. */
#define FORMAT_MESSAGE_LANG MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)
#endif /* _WIN32 */

/* Log levels. */
typedef enum {
	LOG_FATAL = 0,
	LOG_ERROR,
	LOG_WARNING,
	LOG_INFO
} log_level_t;

/* Logging private methods. */
void log_printf(log_level_t level, const char *format, ...);
void log_errno(log_level_t level, const char *msg);
void log_sockerrno(log_level_t level, const char *msg, int err);

/* Private methods. */
int gopher_addr_getaddrinfo(const gopher_addr_t *addr, struct addrinfo **ai);
int gopher_disconnect(gopher_addr_t *addr);

/**
 * Allocates and populates a gopherspace address object.
 *
 * @warning This function dinamically allocates memory.
 *
 * @param host     Domain name or IP address of the Gopher server.
 * @param port     Port to use for communicating with the Gopher server.
 * @param selector Selector of the content to retrieve.
 *
 * @return Newly populated gopherspace address object.
 *
 * @see gopher_addr_free
 */
gopher_addr_t *gopher_addr_new(const char *host, uint16_t port,
							   const char *selector) {
	gopher_addr_t *addr;

	/* Allocate the object. */
	addr = (gopher_addr_t *)malloc(sizeof(gopher_addr_t));
	if (addr == NULL) {
		log_printf(LOG_ERROR, "Failed to allocate memory for gopherspace "
			"address\n");
		return NULL;
	}

	/* Populate the object. */
	addr->host = strdup(host);
	addr->port = port;
	addr->selector = (selector) ? strdup(selector) : NULL;
	addr->sockfd = INVALID_SOCKET;
	addr->ipaddr = NULL;

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
 * @return 0 if the operation was successful.
 */
int gopher_addr_getaddrinfo(const gopher_addr_t *addr, struct addrinfo **ai) {
	struct addrinfo hints;
	
	/* Build up the hints for the address we want to resolve. */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	
	return getaddrinfo(addr->host, NULL, &hints, ai);
}

/**
 * Prints out the gopherspace address object internals for debugging.
 *
 * @param addr Gopherspace address object.
 */
void gopher_addr_print(const gopher_addr_t *addr) {
	/* Is this a valid address? */
	if (addr == NULL) {
		printf("(null)\n");
		return;
	}
	
	/* Print out the address data. */
	printf("%s [%u] %s\n", addr->host, addr->port, addr->selector);
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
	if (addr->sockfd != INVALID_SOCKET)
		gopher_disconnect(addr);
	if (addr->ipaddr) {
		free(addr->ipaddr);
	}
	
	/* Free the object itself. */
	free(addr);
}

/**
 * Establishes a connection to a Gopher server.
 *
 * @param addr Gopherspace address object.
 *
 * @return 0 if the operation was successful. Check return against strerror() in
 *         case of failure.
 */
int gopher_connect(gopher_addr_t *addr) {
	int ret;
	struct addrinfo *query;
	struct addrinfo *ai;
	
	/* Resolve the server's IP address. */
	ret = gopher_addr_getaddrinfo(addr, &query);
	if (ret != 0) {
		log_printf(LOG_ERROR, "Failed to get address IP: (%d) %s\n", ret,
			gai_strerror(ret));
		freeaddrinfo(query);
		return ret;
	}

	/* Allocate memory for the IP address. */
	addr->ipaddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	if (addr->ipaddr == NULL) {
		log_printf(LOG_ERROR, "Failed to allocate memory for server IP address"
			"\n");
		freeaddrinfo(query);
		return ENOMEM;
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
	
	/* Copy the server's IP address and free the resolve object. */
	memcpy(addr->ipaddr, ai->ai_addr, sizeof(struct sockaddr_in));
	freeaddrinfo(query);
	query = NULL;

	return ret;
}

/**
 * Disconnects gracefully from a Gopher server.
 *
 * @param addr Gopherspace address object.
 *
 * @return 0 if the operation was successful. Check return against strerror() in
 *         case of failure.
 */
int gopher_disconnect(gopher_addr_t *addr) {
	int ret;

	/* Is this even a valid address or socket? */
	if ((addr == NULL) || (addr->sockfd == INVALID_SOCKET))
		return EBADF;

	/* Shutdown the connection. */
	#ifdef _WIN32
		ret = shutdown(addr->sockfd, SD_BOTH);
	#else
		ret = shutdown(addr->sockfd, SHUT_RDWR);
	#endif /* _WIN32 */	
	if (ret == SOCKET_ERROR) {
		addr->sockfd = INVALID_SOCKET;
		log_sockerrno(LOG_ERROR, "Failed to shutdown connection", sockerrno);
		return ret;
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
	vfprintf(stderr, format, args);
	va_end(args);
}
