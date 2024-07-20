/**
 * test.c
 * A very simple application to test the implementation of our Gopher library.
 * This minimal implementation shows everything that's needed in order to fully
 * implement the library.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include <stdlib.h>
#include <stdio.h>

#include "gopher.h"

/**
 * Application's main entry point.
 *
 * @param argc Number of command line arguments.
 * @param argv Command line arguments.
 *
 * @return Return code.
 */
int main(int argc, char **argv) {
	gopher_addr_t *addr;
	int ret;
	char buf[100];
	size_t len;

	printf("libgopher v" LIBGOPHER_VER_STR " tester\n");
	
	/* Use default address if none were supplied. */
	if (argc < 2) {
		printf("No address was supplied, using floodgap's for testing.\n");
		addr = gopher_addr_new("gopher.floodgap.com", 70, "/overbite");
	}
	
	/* Print information about the requested address. */
	printf("requesting ");
	gopher_addr_print(addr);
	
	/* Connect to the server. */
	ret = gopher_connect(addr);
	if (ret != 0) {
		perror("Failed to connect");
		gopher_addr_free(addr);

		return ret;
	}
	
	/* Send selector of our request. */
	ret = gopher_send_line(addr, "", NULL);
	if (ret != 0) {
		perror("Failed to send selector");
		goto cleanup;
	}

	/* Print out everything we receive from the server. */
	while ((gopher_recv_raw(addr, buf, 99, &len, 0) == 0) && (len > 0)) {
		/* Ensure we terminate the received string. */
		buf[len] = '\0';
		printf("%s", buf);
	}
	
	/* Gracefully disconnect from the server. */
	ret = gopher_disconnect(addr);
	if (ret != 0)
		perror("Failed to disconnect");

cleanup:	
	/* Free up any resources and return. */
	gopher_addr_free(addr);
	return 0;
}

