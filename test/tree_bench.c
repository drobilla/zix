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

#define _POSIX_C_SOURCE 199309L

#include <time.h>
#include <sys/time.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <glib.h>

#include "zix/zix.h"

const unsigned seed = 1;

static int
int_cmp(const void* a, const void* b, void* user_data)
{
	const intptr_t ia = (intptr_t)a;
	const intptr_t ib = (intptr_t)b;
	return ia - ib;
}

static int
test_fail()
{
	fprintf(stderr, "ERROR\n");
	return EXIT_FAILURE;
}

static inline double
elapsed_s(const struct timespec* start, const struct timespec* end)
{
	return ( (end->tv_sec - start->tv_sec)
	         + ((end->tv_nsec - start->tv_nsec) * 0.000000001) );
}

static inline struct timespec
bench_start()
{
	struct timespec start_t;
	clock_gettime(CLOCK_REALTIME, &start_t);
	return start_t;
}

static inline double
bench_end(const struct timespec* start_t)
{
	struct timespec end_t;
	clock_gettime(CLOCK_REALTIME, &end_t);
	return elapsed_s(start_t, &end_t);
}

static int
bench_zix(size_t n_elems, FILE* insert_dat, FILE* search_dat)
{
	intptr_t     r;
	ZixTreeIter ti;

	ZixTree* t = zix_tree_new(true, int_cmp, NULL);

	srand(seed);

	// Insert n_elems elements
	struct timespec insert_start = bench_start();
	for (size_t i = 0; i < n_elems; i++) {
		r = rand();
		int status = zix_tree_insert(t, (void*)r, &ti);
		if (status && status != ZIX_STATUS_EXISTS) {
			return test_fail();
		}
	}
	fprintf(insert_dat, "\t%lf", bench_end(&insert_start));

	srand(seed);

	// Search for all elements
	struct timespec search_start = bench_start();
	for (size_t i = 0; i < n_elems; i++) {
		r = rand();
		if (zix_tree_find(t, (void*)r, &ti)) {
			return test_fail();
		}
		if ((intptr_t)zix_tree_get_data(ti) != r) {
			return test_fail();
		}
	}
	fprintf(search_dat, "\t%lf", bench_end(&search_start));

	srand(seed);

	// Delete all elements
	for (size_t i = 0; i < n_elems; i++) {
		r = rand();
		ZixTreeIter item;
		if (zix_tree_find(t, (void*)r, &item)) {
			return test_fail();
		}
		if (zix_tree_remove(t, item)) {
			return test_fail();
		}
	}

	zix_tree_free(t);

	return EXIT_SUCCESS;
}

static int
bench_glib(size_t n_elems, FILE* insert_dat, FILE* search_dat)
{
	intptr_t   r;
	GSequence* t = g_sequence_new(NULL);

	srand(seed);

	// Insert n_elems elements
	struct timespec insert_start = bench_start();
	for (size_t i = 0; i < n_elems; i++) {
		r = rand();
		g_sequence_insert_sorted(t, (void*)r, int_cmp, NULL);
	}
	fprintf(insert_dat, "\t%lf", bench_end(&insert_start));

	srand(seed);

	// Search for all elements
	struct timespec search_start = bench_start();
	for (size_t i = 0; i < n_elems; i++) {
		r = rand();
		if (!g_sequence_search(t, (void*)r, int_cmp, NULL)) {
			return test_fail();
		}
	}
	fprintf(search_dat, "\t%lf", bench_end(&search_start));

#if 0
	srand(seed);

	// Delete all elements
	for (size_t i = 0; i < n_elems; i++) {
		r = rand();
		GSequenceIter item;
		if (g_sequence_find(t, (void*)r, &item)) {
			return test_fail();
		}
		if (g_sequence_remove(t, item)) {
			return test_fail();
		}
	}
#endif

	g_sequence_free(t);

	return EXIT_SUCCESS;
}

int
main(int argc, char** argv)
{
	if (argc != 3) {
		fprintf(stderr, "USAGE: %s MIN_N MAX_N\n", argv[0]);
		return 1;
	}

	const size_t min_n = atol(argv[1]);
	const size_t max_n = atol(argv[2]);

	fprintf(stderr, "Benchmarking %zu .. %zu elements\n", min_n, max_n);

	FILE* insert_dat = fopen("insert.dat", "w");
	FILE* search_dat = fopen("search.dat", "w");
	fprintf(insert_dat, "# n\tZixTree\tGSequence\n");
	fprintf(search_dat, "# n\tZixTree\tGSequence\n");
	for (size_t n = min_n; n <= max_n; n *= 2) {
		fprintf(insert_dat, "%zu", n);
		fprintf(search_dat, "%zu", n);
		bench_zix(n, insert_dat, search_dat);
		bench_glib(n, insert_dat, search_dat);
		fprintf(insert_dat, "\n");
		fprintf(search_dat, "\n");
	}
	fclose(insert_dat);
	fclose(search_dat);

	return EXIT_SUCCESS;
}
