/**
 * gopher.h
 * A portable Gopher protocol implementation.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _LIBGOPHER_GOPHER_H_
#define _LIBGOPHER_GOPHER_H_

#include <stdint.h>
#include <stdlib.h>

/* Version information. */
#define LIBGOPHER_VER_STR "0.1"
#define LIBGOPHER_VER_MAJOR 0
#define LIBGOPHER_VER_MINOR 1

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Gopherspace address including host, port, and selector, also includes the
 * connection information.
 */
typedef struct {
	char *host;
	char *selector;
	uint16_t port;
	
	int sockfd;
	struct sockaddr_in *ipaddr;
} gopher_addr_t;

/* Gopherspace address handling. */
gopher_addr_t *gopher_addr_new(const char *host, uint16_t port,
							   const char *selector);
void gopher_addr_print(const gopher_addr_t *addr);
void gopher_addr_free(gopher_addr_t *addr);

/* Connection handling. */
int gopher_connect(gopher_addr_t *addr);


#ifdef __cplusplus
}
#endif

#endif /* _LIBGOPHER_GOPHER_H_ */
