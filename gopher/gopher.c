/**
 * gopher.c
 * A portable Gopher protocol implementation.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "gopher.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

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
		fprintf(stderr, "Failed to allocate memory for gopherspace address\n");
		return NULL;
	}

	/* Populate the object. */
	addr->host = strdup(host);
	addr->port = port;
	addr->selector = (selector) ? strdup(selector) : NULL;
	addr->sockfd = 0;
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
	if (addr->sockfd != 0)
		gopher_disconnect(addr);
	if (addr->ipaddr) {
		/* TODO: Close the connection. */
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
		fprintf(stderr, "gopher_addr_getaddrinfo() failed = (%d) %s\n", ret,
			gai_strerror(ret));
		freeaddrinfo(query);
		return ret;
	}

	/* Allocate memory for the IP address. */
	addr->ipaddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	if (addr->ipaddr == NULL) {
		fprintf(stderr, "Failed to allocate memory for server IP address\n");
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
		fprintf(stderr, "Couldn't resolve an address for %s\n", addr->host);
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
	if ((addr == NULL) || (addr->sockfd == 0))
		return EBADF;

	/* Close the socket file descriptor. */
	ret = close(addr->sockfd);
	addr->sockfd = 0;

	return ret;
}
