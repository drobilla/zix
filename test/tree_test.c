/*
  Copyright 2011 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <sys/time.h>

#include "zix/zix.h"

unsigned seed = 1;

static int
int_cmp(const void* a, const void* b, void* user_data)
{
	const intptr_t ia = (intptr_t)a;
	const intptr_t ib = (intptr_t)b;
	return ia - ib;
}

static int
ith_elem(int test_num, size_t n_elems, int i)
{
	switch (test_num % 3) {
	case 0:
		return i;  // Increasing (worst case)
	case 1:
		return n_elems - i;  // Decreasing (worse case)
	case 2:
	default:
		return rand() % 100;  // Random
	}
}

static int
test_fail()
{
	return EXIT_FAILURE;
}

static int
stress(int test_num, size_t n_elems)
{
	intptr_t    r;
	ZixTreeIter ti;

	ZixTree* t = zix_tree_new(true, int_cmp, NULL);

	srand(seed);

	// Insert n_elems elements
	for (size_t i = 0; i < n_elems; i++) {
		r = ith_elem(test_num, n_elems, i);
		int status = zix_tree_insert(t, (void*)r, &ti);
		if (status == ZIX_STATUS_EXISTS) {
			continue;
		} else if (status != ZIX_STATUS_SUCCESS) {
			fprintf(stderr, "Insert failed\n");
			return test_fail();
		}
		if ((intptr_t)zix_tree_get_data(ti) != r) {
			fprintf(stderr, "Data is corrupted! Saw %" PRIdPTR ", expected %zu\n",
			        (intptr_t)zix_tree_get_data(ti), i);
			return test_fail();
		}
	}

	srand(seed);

	// Search for all elements
	for (size_t i = 0; i < n_elems; i++) {
		r = ith_elem(test_num, n_elems, i);
		if (zix_tree_find(t, (void*)r, &ti)) {
			fprintf(stderr, "Find failed\n");
			return test_fail();
		}
		if ((intptr_t)zix_tree_get_data(ti) != r) {
			fprintf(stderr, "Data corrupted (saw %" PRIdPTR ", expected %zu\n",
			        (intptr_t)zix_tree_get_data(ti), i);
			return test_fail();
		}
	}

	srand(seed);

	// Delete all elements
	for (size_t i = 0; i < n_elems; i++) {
		r = ith_elem(test_num, n_elems, i);
		ZixTreeIter item;
		if (zix_tree_find(t, (void*)r, &item) != ZIX_STATUS_SUCCESS) {
			fprintf(stderr, "Failed to find item to remove\n");
			return test_fail();
		}
		if (zix_tree_remove(t, item)) {
			fprintf(stderr, "Error removing item\n");
		}
	}

	zix_tree_free(t);

	return EXIT_SUCCESS;
}

int
main(int argc, char** argv)
{
	const size_t n_tests = 3;
	size_t       n_elems = 0;

	struct timeval time;
	gettimeofday(&time, NULL);

	if (argc == 1) {
		n_elems = 4096;
	} else {
		n_elems = atol(argv[1]);
		if (argc > 2) {
			seed = atol(argv[2]);
		} else {
			seed = time.tv_sec + time.tv_usec;
		}
	}

	if (n_elems == 0) {
		fprintf(stderr, "USAGE: %s [N_ELEMS]\n", argv[0]);
		return 1;
	}

	printf("Running %zu tests with %zu elements (seed %d)",
	       n_tests, n_elems, seed);

	for (size_t i = 0; i < n_tests; ++i) {
		printf(".");
		fflush(stdout);
		if (stress(i, n_elems)) {
			fprintf(stderr, "FAIL: Random seed %u\n", seed);
			return test_fail();
		}
	}
	printf("\n");
	return EXIT_SUCCESS;
}
