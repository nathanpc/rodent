/**
 * 01_url.c
 * Tests the URL parser and builder functions.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include <tap.h>

#include "gopher.h"

#define NUM_URL_TESTS 4
void test_url(const char *url, gopher_addr_t *ref);

/**
 * Testing program's main entry point.
 */
int main() {
	gopher_addr_t *ref;

	/* Setup the test harness. */
	plan((32 * NUM_URL_TESTS) + 1);

	/* Simplest test without a selector. */
	printf("#\n# URLs without a selector\n");
	ref = gopher_addr_new("g.test.com", 70, NULL, GOPHER_TYPE_UNKNOWN);
	test_url("gopher://g.test.com/", ref);
	test_url("gopher://g.test.com", ref);
	test_url("gopher://g.test.com:70/", ref);
	test_url("gopher://g.test.com:70", ref);
	gopher_addr_free(ref);
	ref = NULL;
	ref = gopher_addr_new("g.test.com", 70, NULL, GOPHER_TYPE_DIR);
	test_url("gopher://g.test.com/1/", ref);
	test_url("gopher://g.test.com:70/1/", ref);
	test_url("gopher://g.test.com/1", ref);
	test_url("gopher://g.test.com:70/1", ref);
	gopher_addr_free(ref);
	ref = NULL;

	/* Tests with a slash-based selector. */
	printf("#\n# URLs with a slash-based selectors\n");
	ref = gopher_addr_new("g.test.com", 70, "/testdir", GOPHER_TYPE_DIR);
	test_url("gopher://g.test.com/1/testdir", ref);
	test_url("gopher://g.test.com:70/1/testdir", ref);
	/*test_url("gopher://g.test.com/testdir", ref);
	test_url("gopher://g.test.com:70/testdir", ref);*/
	gopher_addr_free(ref);
	ref = NULL;
	ref = gopher_addr_new("g.test.com", 70, "/testdir/testfile.txt",
		GOPHER_TYPE_TEXT);
	test_url("gopher://g.test.com/0/testdir/testfile.txt", ref);
	test_url("gopher://g.test.com:70/0/testdir/testfile.txt", ref);
	gopher_addr_free(ref);
	ref = NULL;

	/* Tests with the old non-slash-as-root selectors. */
	printf("#\n# URLs with old style selectors\n");
	ref = gopher_addr_new("g.test.com", 70, "testdir", GOPHER_TYPE_DIR);
	test_url("gopher://g.test.com/1testdir", ref);
	test_url("gopher://g.test.com:70/1testdir", ref);
	gopher_addr_free(ref);
	ref = NULL;
	ref = gopher_addr_new("g.test.com", 70, "testdir/testfile.txt",
		GOPHER_TYPE_TEXT);
	test_url("gopher://g.test.com/0testdir/testfile.txt", ref);
	test_url("gopher://g.test.com:70/0testdir/testfile.txt", ref);
	gopher_addr_free(ref);
	ref = NULL;

	/* Test URLs without the gopher protocol prefix. */
	printf("#\n# URLs without the protocol type prefix\n");
	ref = gopher_addr_new("g.test.com", 70, NULL, GOPHER_TYPE_UNKNOWN);
	test_url("g.test.com/", ref);
	test_url("g.test.com", ref);
	test_url("g.test.com:70/", ref);
	test_url("g.test.com:70", ref);
	gopher_addr_free(ref);
	ref = NULL;
	ref = gopher_addr_new("g.test.com", 70, NULL, GOPHER_TYPE_DIR);
	test_url("g.test.com/1/", ref);
	test_url("g.test.com:70/1/", ref);
	test_url("g.test.com/1", ref);
	test_url("g.test.com:70/1", ref);
	gopher_addr_free(ref);
	ref = NULL;
	ref = gopher_addr_new("g.test.com", 70, "/testdir", GOPHER_TYPE_DIR);
	test_url("g.test.com/1/testdir", ref);
	test_url("g.test.com:70/1/testdir", ref);
	gopher_addr_free(ref);
	ref = NULL;
	ref = gopher_addr_new("g.test.com", 70, "/testdir/testfile.txt",
		GOPHER_TYPE_TEXT);
	test_url("g.test.com/0/testdir/testfile.txt", ref);
	test_url("g.test.com:70/0/testdir/testfile.txt", ref);
	gopher_addr_free(ref);
	ref = NULL;
	ref = gopher_addr_new("g.test.com", 70, "testdir", GOPHER_TYPE_DIR);
	test_url("g.test.com/1testdir", ref);
	test_url("g.test.com:70/1testdir", ref);
	gopher_addr_free(ref);
	ref = NULL;
	ref = gopher_addr_new("g.test.com", 70, "testdir/testfile.txt",
		GOPHER_TYPE_TEXT);
	test_url("g.test.com/0testdir/testfile.txt", ref);
	test_url("g.test.com:70/0testdir/testfile.txt", ref);
	gopher_addr_free(ref);
	ref = NULL;

	/* Test invalid URLs. */
	printf("#\n# Invalid URLs\n");
	test_url("http://g.test.com/", NULL);

	/* Finish the tests. */
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
	tmp = gopher_addr_parse(url);
	if (ref != NULL) {
		is(tmp->host, ref->host, "%s host is %s", url, ref->host);
		ok(tmp->port == ref->port, "%s port is %u", url, ref->port);
		is(tmp->selector, ref->selector, "%s selector is %s", url,
			ref->selector);
		cmp_ok(tmp->type, "==", ref->type, "%s is of type %c", url,
			(char)ref->type);
	} else {
		ok(tmp == NULL, "%s is invalid", url);
	}

	/* Free up any unused resources. */
	if (tmp)
		free(tmp);
	tmp = NULL;
}
