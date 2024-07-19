/**
 * gopher.h
 * A portable, single header/source, Gopher protocol implementation.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _LIBGOPHER_GOPHER_H_
#define _LIBGOPHER_GOPHER_H_

#include <stdint.h>
#include <stdlib.h>
#ifdef _WIN32
	#include <winsock2.h>
#else
	#include <sys/socket.h>
#endif /* _WIN32 */

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
	socklen_t ipaddr_len;
} gopher_addr_t;

/* Gopherspace address handling. */
gopher_addr_t *gopher_addr_new(const char *host, uint16_t port,
							   const char *selector);
void gopher_addr_print(const gopher_addr_t *addr);
void gopher_addr_free(gopher_addr_t *addr);

/* Connection handling. */
int gopher_connect(gopher_addr_t *addr);
int gopher_disconnect(gopher_addr_t *addr);

/* Networking operations. */
int gopher_send_raw(const gopher_addr_t *addr, const void *buf, size_t len,
					size_t *sent_len);
int gopher_send(const gopher_addr_t *addr, const char *buf, size_t *sent_len);


#ifdef __cplusplus
}
#endif

#endif /* _LIBGOPHER_GOPHER_H_ */
