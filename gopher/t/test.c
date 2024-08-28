/**
 * test.c
 * Test suite for our Gopher library.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include <tap.h>

/* External functions. */
extern int t_urlpar_plan(void);
extern void t_urlpar_run(void);
extern int t_urlgen_plan(void);
extern void t_urlgen_run(void);

/**
 * Unit testing program's main entry point.
 */
int main() {
	/* Setup the test harness. */
	plan(t_urlpar_plan() + t_urlgen_plan());

	/* Run tests in sequence. */
	t_urlpar_run();
	t_urlgen_run();

	/* Finish the tests. */
	done_testing();
}
