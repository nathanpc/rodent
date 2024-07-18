/**
 * test.c
 * A very simple application to test the implementation of our Gopher library.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
	printf("libgopher v" LIBGOPHER_VER_STR " tester\n");
	
	return 0;
}
