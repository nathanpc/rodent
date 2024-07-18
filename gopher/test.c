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
	gopher_connect(addr);
	
	/* Free up any resources. */
	gopher_addr_free(addr);
	
	return 0;
}

