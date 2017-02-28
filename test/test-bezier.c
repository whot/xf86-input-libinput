/*
 * Copyright © 2016 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Red Hat
 * not be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.  Red
 * Hat makes no representations about the suitability of this software
 * for any purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "bezier.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

static inline void
print_curve(int *bezier, size_t size)
{
	/* look at it with gnuplot, "plot 'output-file.txt'" */
	for (int i = 0; i < size; i++)
		printf("%d %d\n", i, bezier[i]);
}

static void
test_linear(void)
{
	const int size = 2048;
	int bezier[size];

	struct bezier_control_point controls[] = {
		{ 0.0, 0.0 },
		{ 0.0, 0.0 },
		{ 1.0, 1.0 },
		{ 1.0, 1.0 }
	};

	cubic_bezier(controls, bezier, size);

	assert(bezier[0] == 0);
	assert(bezier[size - 1] == size - 1);

	for (int x = 1; x < size; x++)
		assert(bezier[x] == x);
}

/* Center point pulled down towards X axis */
static void
test_flattened(void)
{
	const int size = 2048;
	int bezier[size];

	struct bezier_control_point controls[] = {
		{ 0.0, 0.0 },
		{ 0.1, 0.0 },
		{ 1.0, 0.9 },
		{ 1.0, 1.0 }
	};

	cubic_bezier(controls, bezier, size);

	assert(bezier[0] == 0);
	assert(bezier[size - 1] == size - 1);

	for (int x = 1; x < size - 1; x++) {
		assert(bezier[x] < x);
	}
}

/* Center point pulled up from X axis */
static void
test_raised(void)
{
	const int size = 2048;
	int bezier[size];

	struct bezier_control_point controls[] = {
		{ 0.0, 0.0 },
		{ 0.1, 0.4 },
		{ 0.4, 1.0 },
		{ 1.0, 1.0 }
	};

	cubic_bezier(controls, bezier, size);

	assert(bezier[0] == 0);
	assert(bezier[size - 1] == size - 1);

	for (int x = 1; x < size; x++)
		assert(bezier[x] >= x);

	for (int x = 10; x < size - 10; x++)
		assert(bezier[x] > x);
}

static void
test_windy(void)
{
	const int size = 2048;
	int bezier[size];

	struct bezier_control_point controls[] = {
		{ 0.0, 0.0 },
		{ 0.0, 0.3 },
		{ 1.0, 0.7 },
		{ 1.0, 1.0 }
	};

	cubic_bezier(controls, bezier, size);

	assert(bezier[0] == 0);
	assert(bezier[size - 1] == size - 1);

	for (int x = 1; x < size/2 - 20; x++)
		assert(bezier[x] > x);

	for (int x = size/2 + 20; x < size - 1; x++)
		assert(bezier[x] < x);
}

static void
test_nonzero_x_linear(void)
{
	const int size = 2048;
	int bezier[size];
	int x;

	struct bezier_control_point controls[] = {
		{ 0.2, 0.0 },
		{ 0.2, 0.0 },
		{ 0.8, 1.0 },
		{ 0.8, 1.0 }
	};

	cubic_bezier(controls, bezier, size);

	x = 0;
	do {
		assert(bezier[x] == 0);
	} while (++x < size * 0.2 - 1);

	/* ppc64le, ppc64, aarch64 have different math results at -O2,
	   resulting in one extra zero at the beginning of the array.
	   some other numbers are different too but within the error
	   margin (#99992) */
	if (bezier[x] == 0)
		x++;

	do {
		assert(bezier[x] > bezier[x-1]);
	} while (++x < size * 0.8 - 1);

	do {
		assert(bezier[x] == size - 1);
	} while (++x < size);
}

static void
test_nonzero_y_linear(void)
{
	const int size = 2048;
	int bezier[size];

	struct bezier_control_point controls[] = {
		{ 0.0, 0.2 },
		{ 0.0, 0.2 },
		{ 1.0, 0.8 },
		{ 1.0, 0.8 }
	};

	cubic_bezier(controls, bezier, size);

	assert(bezier[0] == (int)(size * 0.2));

	for (int x = 1; x < size; x++) {
		assert(bezier[x - 1] <= bezier[x]);
		assert(bezier[x] >= (int)(size * 0.2));
	}
}

int
main(int argc, char **argv)
{
	test_linear();
	test_flattened();
	test_raised();
	test_windy();
	test_nonzero_x_linear();
	test_nonzero_y_linear();

	return 0;
}
