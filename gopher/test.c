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
	gopher_item_t *item;
	gopher_dir_t *dir;
	int ret;

	/* Initialize the core. */
	printf("libgopher v" LIBGOPHER_VER_STR " tester\n");
	item = NULL;
	dir = NULL;
	
	/* Use default address if none were supplied. */
	if (argc < 2) {
		printf("No address was supplied, using floodgap's for testing.\n");
		addr = gopher_addr_new("gopher.floodgap.com", 70, "/overbite");
	} else {
		/* Parse Gopher URI from argument. */
		addr = gopher_addr_parse(argv[1], NULL);
	}
	
	/* Ensure we got a gopherspace address. */
	if (addr == NULL) {
		printf("Failed to get gopherspace address\n");
		return 1;
	}
	
	/* Print information about the requested address. */
	printf("Requesting ");
	gopher_addr_print(addr);
	
	/* Connect to the server. */
	ret = gopher_connect(addr);
	if (ret != 0) {
		perror("Failed to connect");
		goto cleanup;
	}
	
	/* Get directory from address. */
	ret = gopher_dir_request(addr, &dir);
	if (ret != 0) {
		perror("Failed to request directory");
		goto cleanup;
	}
	
	/* Print out every item from the directory. */
	if (dir->items_len > 0) {
		item = dir->items;
		do {
			gopher_item_print(item);
			item = item->next;
		} while (item != NULL);
	} else {
		printf("Empty directory.\n");
	}
	
	/* Shame non-compliant servers. */
	if (dir->err_count > 0) {
		printf("%u errors encountered while parsing the server's output\n",
			dir->err_count);
	}

	/* Gracefully disconnect from the server. */
	ret = gopher_disconnect(addr);
	if (ret != 0)
		perror("Failed to disconnect");

cleanup:	
	/* Free up any resources and return. */
	if (dir == NULL) {
		gopher_addr_free(addr);
	} else {
		gopher_dir_free(dir, RECURSE_FORWARD | RECURSE_BACKWARD, 1);
	}

	return 0;
}

