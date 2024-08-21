/**
 * 01_url.c
 * Tests the URL parser and builder functions.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include <tap.h>

#include "gopher.h"

#define NUM_URL_TESTS 3
void test_url(const char *url, gopher_addr_t *ref);

/**
 * Testing program's main entry point.
 */
int main() {
	gopher_addr_t *ref;

	/* Setup the test harness. */
	plan(8 * NUM_URL_TESTS);
	
	/* Simplest test without a selector. */
	ref = gopher_addr_new("g.test.com", 70, NULL);
	test_url("gopher://g.test.com/", ref);
	test_url("gopher://g.test.com", ref);
	test_url("gopher://g.test.com:70/", ref);
	test_url("gopher://g.test.com:70", ref);
	test_url("gopher://g.test.com/1/", ref);
	test_url("gopher://g.test.com:70/1/", ref);
	test_url("gopher://g.test.com/1", ref);
	test_url("gopher://g.test.com:70/1", ref);

	/* Finish the tests. */
	gopher_addr_free(ref);
	ref = NULL;
	done_testing();
}

/**
 * Tests a URL against a reference Gopherspace address object.
 *
 * @param url A gopherspace URL to be tested.
 * @param ref A reference gopherspace address object.
 */
void test_url(const char *url, gopher_addr_t *ref) {
	gopher_addr_t *tmp;

	/* Announce the new test URL. */
	printf("#\n# %s\n", url);

	/* Parse the URL and check against the reference. */
	tmp = gopher_addr_parse(url, NULL);
	is(tmp->host, ref->host, "%s host is %s", url, ref->host);
	ok(tmp->port == ref->port, "%s port is %u", url, ref->port);
	is(tmp->selector, ref->selector, "%s selector is %s", url, ref->selector);
	
	/* Free up any unused resources. */
	free(tmp);
	tmp = NULL;
}
