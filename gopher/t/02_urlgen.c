/**
 * 02_urlgen.c
 * Tests the URL builder functions.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include <string.h>
#include <tap.h>

#include "gopher.h"

/* Private definitions. */
static void test_url_gen(const char *url, const char *ref);

/**
 * Gets the number of planned tests.
 *
 * @return Number of planned tests.
 */
int t_urlgen_plan(void) {
	return 16;
}

/**
 * Runs unit tests.
 */
void t_urlgen_run(void) {
	char *ref;

	/* gopher://g.test.com:70/1/ */
	printf("#\n# Building various URLs\n");
	ref = strdup("gopher://g.test.com:70/");
	test_url_gen("gopher://g.test.com/", ref);
	test_url_gen("gopher://g.test.com", ref);
	test_url_gen("gopher://g.test.com:70/", ref);
	test_url_gen("gopher://g.test.com:70", ref);
	test_url_gen("gopher://g.test.com/1/", ref);
	test_url_gen("gopher://g.test.com:70/1/", ref);
	test_url_gen("gopher://g.test.com/1", ref);
	test_url_gen("gopher://g.test.com:70/1", ref);
	free(ref);
	ref = NULL;

	/* gopher://g.test.com:70/1/testdir */
	ref = strdup("gopher://g.test.com:70/1/testdir");
	test_url_gen("gopher://g.test.com/1/testdir", ref);
	test_url_gen("gopher://g.test.com:70/1/testdir", ref);
	/*test_url_gen("gopher://g.test.com/testdir", ref);
	test_url_gen("gopher://g.test.com:70/testdir", ref);*/
	free(ref);
	ref = NULL;

	/* gopher://g.test.com:70/0/testdir/testfile.txt */
	ref = strdup("gopher://g.test.com:70/0/testdir/testfile.txt");
	test_url_gen("gopher://g.test.com/0/testdir/testfile.txt", ref);
	test_url_gen("gopher://g.test.com:70/0/testdir/testfile.txt", ref);
	free(ref);
	ref = NULL;

	/* gopher://g.test.com:70/1testdir */
	ref = strdup("gopher://g.test.com:70/1testdir");
	test_url_gen("gopher://g.test.com/1testdir", ref);
	test_url_gen("gopher://g.test.com:70/1testdir", ref);
	free(ref);
	ref = NULL;

	/* gopher://g.test.com:70/0testdir/testfile.txt */
	ref = strdup("gopher://g.test.com:70/0testdir/testfile.txt");
	test_url_gen("gopher://g.test.com/0testdir/testfile.txt", ref);
	test_url_gen("gopher://g.test.com:70/0testdir/testfile.txt", ref);
	free(ref);
	ref = NULL;
}

/**
 * Tests a URL against a reference Gopherspace URL.
 *
 * @param url A gopherspace URL to be tested.
 * @param ref The expected gopherspace URL.
 */
static void test_url_gen(const char *url, const char *ref) {
	gopher_addr_t *addr;
	char *tmp;

	/* Parse the URL and check against the reference. */
	addr = gopher_addr_parse(url);
	tmp = gopher_addr_str(addr);
	if (addr != NULL) {
		is(tmp, ref, "%s is %s", url, ref);
	} else {
		ok(tmp == NULL, "%s is invalid", url);
	}

	/* Free up any unused resources. */
	if (addr)
		free(addr);
	addr = NULL;
	if (tmp)
		free(tmp);
	tmp = NULL;
}
