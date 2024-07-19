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
int sockaddrstr(char **buf, const struct sockaddr *sock_addr);
int gopher_getaddrinfo(const gopher_addr_t *addr, struct addrinfo **ai);
#ifdef _WIN32
char *win_wcstombs(const wchar_t *wstr);
#endif /* _WIN32 */

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
	addr->ipaddr_len = 0;

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
	if (ret == SOCKET_ERROR)
		log_sockerrno(LOG_WARNING, "Failed to shutdown connection", sockerrno);

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
 */
int gopher_send_raw(const gopher_addr_t *addr, const void *buf, size_t len,
					size_t *sent_len) {
	ssize_t bytes_sent;

	/* Try to send some information through a socket. */
	bytes_sent = send(addr->sockfd, buf, len, 0);
	if (bytes_sent == SOCKET_ERROR) {
		log_sockerrno(LOG_ERROR, "Failed to send raw data over socket",
			sockerrno);
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
 */
int gopher_send(const gopher_addr_t *addr, const char *buf, size_t *sent_len) {
	return gopher_send_raw(addr, (const void *)buf, strlen(buf), sent_len);
}

/*
 * +===========================================================================+
 * |                                                                           |
 * |                           Logging and Debugging                           |
 * |                                                                           |
 * +===========================================================================+
 */

#ifdef _WIN32
/**
 * Converts a UTF-16 wide-character string into a UTF-8 multibyte string.
 * @warning This function allocates memory that must be free'd by you!
 *
 * @param wstr UTF-16 string to be converted.
 *
 * @return UTF-8 multibyte converted string or NULL if an error occurred.
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
	nLen = WideCharToMultiByte(cap_utf8() ? CP_UTF8 : CP_OEMCP, 0, wstr, -1,
							   str, nLen, NULL, NULL);
	if (nLen == 0) {
failure:
		MessageBox(NULL, _T("Failed to convert UTF-16 string to UTF-8."),
			_T("String Conversion Failure"), MB_ICONERROR | MB_OK);

		return NULL;
	}

	return str;
}
#endif /* _WIN32 */

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
			WSAAddressToString(sock_addr, sizeof(struct sockaddr_in), NULL,
							   tmp, &dwLen);
#else
			inet_ntop(AF_INET,
					  &(((const struct sockaddr_in *)sock_addr)->sin_addr),
					  tmp, INET_ADDRSTRLEN);
#endif /* _WIN32 */
			break;
		case AF_INET6:
#ifdef _WIN32
			WSAAddressToString(sock_addr, sizeof(struct sockaddr_in6), NULL,
							   tmp, &dwLen);
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
	*buf = utf16_wcstombs(tmp);
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
