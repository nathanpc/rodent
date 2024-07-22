/**
 * Tester.cpp
 * A simple application to test the port of the librodent library.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "stdafx.h"

/**
 * Application's main entry point.
 *
 * @param argc Number of command line arguments.
 * @param argv Command line arguments.
 *
 * @return Return code.
 */
int _tmain(int argc, TCHAR* argv[])
{
	gopher_addr_t *addr;
	gopher_item_t *item;
	gopher_dir_t *dir;
	int ret;

#ifdef _WIN32
	WORD wVersionRequested;
	WSADATA wsaData;

	// Initialize the Winsock stuff.
	wVersionRequested = MAKEWORD(2, 2);
	if ((ret = WSAStartup(wVersionRequested, &wsaData)) != 0) {
		printf("WSAStartup failed with error %d\n", ret);
		return 1;
	}
#endif /* _WIN32 */

	// Initialize the core.
	_tprintf(_T("libgopher v") LIBGOPHER_VER_STR _T(" tester\r\n"));
	item = NULL;
	dir = NULL;

	// Use default address if none were supplied.
	if (argc < 2) {
		_tprintf(_T("No address was supplied, using floodgap's for testing.\r\n"));
		addr = gopher_addr_new("gopher.floodgap.com", 70, "/overbite");
	} else {
		// Parse Gopher URI from argument.
		//addr = gopher_addr_parse(argv[1]);
	}
	
	// Ensure we got a gopherspace address.
	if (addr == NULL) {
		_tprintf(_T("Failed to get gopherspace address\r\n"));
		return 1;
	}
	
	// Print information about the requested address.
	_tprintf(_T("Requesting "));
	gopher_addr_print(addr);
	
	// Connect to the server.
	ret = gopher_connect(addr);
	if (ret != 0) {
		perror("Failed to connect");
		goto cleanup;
	}
	
	// Get directory from address.
	ret = gopher_dir_request(addr, &dir);
	if (ret != 0) {
		perror("Failed to request directory");
		goto cleanup;
	}
	
	// Print out every item from the directory.
	if (dir->items_len > 0) {
		item = dir->items;
		do {
			gopher_item_print(item);
			item = item->next;
		} while (item != NULL);
	} else {
		_tprintf(_T("Empty directory.\r\n"));
	}
	
	// Shame non-compliant servers.
	if (dir->err_count > 0) {
		_tprintf(_T("%u errors encountered while parsing the server's output\r\n"),
			dir->err_count);
	}

	// Gracefully disconnect from the server.
	ret = gopher_disconnect(addr);
	if (ret != 0)
		perror("Failed to disconnect");

cleanup:	
	// Free up any resources and return.
	if (dir == NULL) {
		gopher_addr_free(addr);
	} else {
		gopher_dir_free(dir, (gopher_recurse_dir_t)(RECURSE_FORWARD |
			RECURSE_BACKWARD), 1);
	}

#ifdef _WIN32
	// Clean up the Winsock stuff.
	WSACleanup();
	system("pause");
#endif /* _WIN32 */

	return 0;
}
