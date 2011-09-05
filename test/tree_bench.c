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
test_fail(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "error: ");
	vfprintf(stderr, fmt, args);
	va_end(args);
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
bench_zix(size_t n_elems,
          FILE* insert_dat, FILE* search_dat, FILE* iter_dat, FILE* del_dat)
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
		if (status) {
			return test_fail("Failed to insert %zu\n", r);
		}
	}
	fprintf(insert_dat, "\t%lf", bench_end(&insert_start));

	srand(seed);

	// Search for all elements
	struct timespec search_start = bench_start();
	for (size_t i = 0; i < n_elems; i++) {
		r = rand();
		if (zix_tree_find(t, (void*)r, &ti)) {
			return test_fail("Failed to find %zu\n", r);
		}
		if ((intptr_t)zix_tree_get_data(ti) != r) {
			return test_fail("Failed to get %zu\n", r);
		}
	}
	fprintf(search_dat, "\t%lf", bench_end(&search_start));

	srand(seed);

	// Iterate over all elements
	struct timespec iter_start = bench_start();
	for (ZixTreeIter iter = zix_tree_begin(t);
	     !zix_tree_iter_is_end(iter);
	     iter = zix_tree_iter_next(iter)) {
		zix_tree_get_data(iter);
	}
	fprintf(iter_dat, "\t%lf", bench_end(&iter_start));

	srand(seed);

	// Delete all elements
	struct timespec del_start = bench_start();
	for (size_t i = 0; i < n_elems; i++) {
		r = rand();
		ZixTreeIter item;
		if (zix_tree_find(t, (void*)r, &item)) {
			return test_fail("Failed to find %zu to delete\n", r);
		}
		if (zix_tree_remove(t, item)) {
			return test_fail("Failed to remove %zu\n", r);
		}
	}
	fprintf(del_dat, "\t%lf", bench_end(&del_start));

	zix_tree_free(t);

	return EXIT_SUCCESS;
}

static int
bench_glib(size_t n_elems,
           FILE* insert_dat, FILE* search_dat, FILE* iter_dat, FILE* del_dat)
{
	intptr_t   r;
	GSequence* t = g_sequence_new(NULL);

	srand(seed);

	// Insert n_elems elements
	struct timespec insert_start = bench_start();
	for (size_t i = 0; i < n_elems; ++i) {
		r = rand();
		GSequenceIter* iter = g_sequence_insert_sorted(t, (void*)r, int_cmp, NULL);
		if (!iter || g_sequence_iter_is_end(iter)) {
			return test_fail("Failed to insert %zu\n", r);
		}
	}
	fprintf(insert_dat, "\t%lf", bench_end(&insert_start));

	srand(seed);

	// Search for all elements
	struct timespec search_start = bench_start();
	for (size_t i = 0; i < n_elems; ++i) {
		r = rand();
		GSequenceIter* iter = g_sequence_lookup(t, (void*)r, int_cmp, NULL);
		if (!iter || g_sequence_iter_is_end(iter)) {
			return test_fail("Failed to find %zu\n", r);
		}
	}
	fprintf(search_dat, "\t%lf", bench_end(&search_start));

	srand(seed);

	// Iterate over all elements
	struct timespec iter_start = bench_start();
	for (GSequenceIter* iter = g_sequence_get_begin_iter(t);
	     !g_sequence_iter_is_end(iter);
	     iter = g_sequence_iter_next(iter)) {
		g_sequence_get(iter);
	}
	fprintf(iter_dat, "\t%lf", bench_end(&iter_start));

	srand(seed);

	// Delete all elements
	struct timespec del_start = bench_start();
	for (size_t i = 0; i < n_elems; ++i) {
		r = rand();
		GSequenceIter* iter = g_sequence_lookup(t, (void*)r, int_cmp, NULL);
		if (!iter || g_sequence_iter_is_end(iter)) {
			return test_fail("Failed to remove %zu\n", r);
		}
		g_sequence_remove(iter);
	}
	fprintf(del_dat, "\t%lf", bench_end(&del_start));

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
	FILE* iter_dat   = fopen("iterate.dat", "w");
	FILE* del_dat    = fopen("del.dat", "w");
	fprintf(insert_dat, "# n\tZixTree\tGSequence\n");
	fprintf(search_dat, "# n\tZixTree\tGSequence\n");
	fprintf(iter_dat, "# n\tZixTree\tGSequence\n");
	fprintf(del_dat, "# n\tZixTree\tGSequence\n");
	for (size_t n = min_n; n <= max_n; n *= 2) {
		fprintf(insert_dat, "%zu", n);
		fprintf(search_dat, "%zu", n);
		fprintf(iter_dat, "%zu", n);
		fprintf(del_dat, "%zu", n);
		bench_zix(n, insert_dat, search_dat, iter_dat, del_dat);
		bench_glib(n, insert_dat, search_dat, iter_dat, del_dat);
		fprintf(insert_dat, "\n");
		fprintf(search_dat, "\n");
		fprintf(iter_dat, "\n");
		fprintf(del_dat, "\n");
	}
	fclose(insert_dat);
	fclose(search_dat);
	fclose(iter_dat);
	fclose(del_dat);

	return EXIT_SUCCESS;
}
