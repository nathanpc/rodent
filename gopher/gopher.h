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
 * Recursive free direction.
 */
typedef enum {
	RECURSE_NONE     = 0x00,
	RECURSE_FORWARD  = 0x01,
	RECURSE_BACKWARD = 0x02
} gopher_recurse_dir_t;

/**
 * Gopher data types.
 */
typedef enum {
	GOPHER_TYPE_INTERNAL = '\0',
	GOPHER_TYPE_TEXT   = '0',
	GOPHER_TYPE_DIR    = '1',
	GOPHER_TYPE_CSO    = '2',
	GOPHER_TYPE_ERROR  = '3',
	GOPHER_TYPE_BINHEX = '4',
	GOPHER_TYPE_DOS    = '5',
	GOPHER_TYPE_UNIX   = '6',
	GOPHER_TYPE_SEARCH = '7',
	GOPHER_TYPE_TELNET = '8',
	GOPHER_TYPE_BINARY = '9',
	GOPHER_TYPE_MIRROR = '+',
	GOPHER_TYPE_TN3270 = 'T',
	GOPHER_TYPE_GIF    = 'g',
	GOPHER_TYPE_IMAGE  = 'I',
	GOPHER_TYPE_BITMAP = ':',
	GOPHER_TYPE_MOVIE  = ';',
	GOPHER_TYPE_AUDIO  = '<',
	GOPHER_TYPE_DOC    = 'd',
	GOPHER_TYPE_HTML   = 'h',
	GOPHER_TYPE_INFO   = 'i',
	GOPHER_TYPE_PNG    = 'p',
	GOPHER_TYPE_WAV    = 's',
	GOPHER_TYPE_PDF    = 'P',
	GOPHER_TYPE_XML    = 'X'
} gopher_type_t;

/**
 * Gopherspace address including host, port, and selector, also includes the
 * connection information.
 */
typedef struct gopher_addr_s {
	char *host;
	char *selector;
	uint16_t port;
	
	int sockfd;
	struct sockaddr_in *ipaddr;
	socklen_t ipaddr_len;
} gopher_addr_t;

/**
 * Gopher line item object.
 */
typedef struct gopher_item_s {
	char *label;
	gopher_addr_t *addr;
	struct gopher_item_s *next;
	gopher_type_t type;
} gopher_item_t;

/**
 * Gopher directory object.
 */
typedef struct gopher_dir_s {
	gopher_addr_t *addr;
	struct gopher_dir_s *prev;
	gopher_item_t *items;
	struct gopher_dir_s *next;
	size_t items_len;
} gopher_dir_t;

/* Gopherspace address handling. */
gopher_addr_t *gopher_addr_new(const char *host, uint16_t port,
							   const char *selector);
void gopher_addr_print(const gopher_addr_t *addr);
void gopher_addr_free(gopher_addr_t *addr);

/* Connection handling. */
int gopher_connect(gopher_addr_t *addr);
int gopher_disconnect(gopher_addr_t *addr);

/* Directory handling. */
int gopher_dir_request(gopher_addr_t *addr, gopher_dir_t **dir);
void gopher_dir_free(gopher_dir_t *dir, gopher_recurse_dir_t recurse,
					 int inclusive);

/* Item line parsing */
int gopher_item_parse(gopher_item_t **item, const char *line);
void gopher_item_free(gopher_item_t *item, gopher_recurse_dir_t recurse);
int gopher_is_termline(const char *line);

/* Networking operations. */
int gopher_send_raw(const gopher_addr_t *addr, const void *buf, size_t len,
					size_t *sent_len);
int gopher_send(const gopher_addr_t *addr, const char *buf, size_t *sent_len);				
int gopher_send_line(const gopher_addr_t *addr, const char *buf,
					 size_t *sent_len);
int gopher_recv_raw(const gopher_addr_t *addr, void *buf, size_t buf_len,
					size_t *recv_len, int flags);
int gopher_recv_line(const gopher_addr_t *addr, char **line, size_t *len);

/* Debugging */
void gopher_addr_print(const gopher_addr_t *addr);
void gopher_item_print_type(const gopher_item_t *item);
void gopher_item_print(const gopher_item_t *item);


#ifdef __cplusplus
}
#endif

#endif /* _LIBGOPHER_GOPHER_H_ */
