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
	#include <ws2tcpip.h>
#else
	#include <sys/socket.h>
#endif /* _WIN32 */

/* Version information. */
#ifdef _WIN32
	#define LIBGOPHER_VER_STR _T("0.1")
#else
	#define LIBGOPHER_VER_STR "0.1"
#endif /* _WIN32 */
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
	GOPHER_TYPE_UNKNOWN  = '\0',
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
	gopher_type_t type;

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
	uint16_t err_count;
} gopher_dir_t;

/**
 * File download bytes transferred reporting callback function.
 *
 * @param gf  Gopher downloaded file object. Must be cast when handled.
 * @param arg Optional data set by the event handler setup.
 */
typedef void (*gopher_file_transfer_func)(const void *gf, void *arg);

/**
 * Gopher downloaded file object.
 */
typedef struct gopher_file_s {
	gopher_addr_t *addr;

	gopher_file_transfer_func transfer_cb;
	void *transfer_cb_arg;

	char *fpath;
	size_t fsize;
	gopher_type_t type;
} gopher_file_t;

/* Gopherspace address handling. */
gopher_addr_t *gopher_addr_new(const char *host, uint16_t port,
							   const char *selector, gopher_type_t type);
gopher_addr_t *gopher_addr_parse(const char *uri);
char *gopher_addr_str(const gopher_addr_t *addr);
int gopher_addr_up(gopher_addr_t **parent, const gopher_addr_t *addr);
void gopher_addr_print(const gopher_addr_t *addr);
void gopher_addr_free(gopher_addr_t *addr);

/* Connection handling. */
int gopher_connect(gopher_addr_t *addr);
int gopher_disconnect(gopher_addr_t *addr);

/* Directory handling. */
int gopher_dir_request(gopher_addr_t *addr, gopher_dir_t **dir);
void gopher_dir_free(gopher_dir_t *dir, gopher_recurse_dir_t recurse,
					 int inclusive);

/* File download handling. */
gopher_file_t *gopher_file_new(gopher_addr_t *addr, const char *path,
							   gopher_type_t hint);
int gopher_file_download(gopher_file_t *gf);
void gopher_file_free(gopher_file_t *gf);
char *gopher_file_basename(const gopher_addr_t *addr);
void gopher_file_set_transfer_cb(gopher_file_t *gf,
								 gopher_file_transfer_func func, void *arg);

/* Item line parsing */
int gopher_item_parse(gopher_item_t **item, const char *line);
void gopher_item_free(gopher_item_t *item, gopher_recurse_dir_t recurse);
int gopher_is_termline(const char *line);
char *gopher_item_url(const gopher_item_t *item);

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

#ifdef _WIN32
/* Easily convert to and from Unicode to multi-byte strings under Windows. */
char *win_wcstombs(const wchar_t *wstr);
wchar_t *win_mbstowcs(const char *str);
#endif /* _WIN32 */


#ifdef __cplusplus
}
#endif

#endif /* _LIBGOPHER_GOPHER_H_ */
