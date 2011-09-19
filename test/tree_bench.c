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

#include "bench.h"

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
int_ptr_cmp(const void* a, const void* b, void* user_data)
{
	const intptr_t ia = *(intptr_t*)a;
	const intptr_t ib = *(intptr_t*)b;
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

static int
bench_zix_tree(size_t n_elems,
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
bench_zix_sorted_array(size_t n_elems,
                       FILE* insert_dat, FILE* search_dat, FILE* iter_dat, FILE* del_dat)
{
	intptr_t    r;
	ZixSortedArrayIter ti;

	ZixSortedArray* t = zix_sorted_array_new(true, int_ptr_cmp, NULL, sizeof(intptr_t));

	srand(seed);

	// Insert n_elems elements
	struct timespec insert_start = bench_start();
	for (size_t i = 0; i < n_elems; ++i) {
		r = rand();
		int status = zix_sorted_array_insert(t, &r, &ti);
		if (status) {
			return test_fail("Insert failed\n");
		}
		if (*(intptr_t*)zix_sorted_array_get_data(ti) != r) {
			return test_fail("Data corrupt (saw %" PRIdPTR ", expected %zu)\n",
			                 *(intptr_t*)zix_sorted_array_get_data(ti), r);
		}
	}
	fprintf(insert_dat, "\t%lf", bench_end(&insert_start));

	srand(seed);

	// Search for all elements
	struct timespec search_start = bench_start();
	for (size_t i = 0; i < n_elems; ++i) {
		r = rand();
		if (zix_sorted_array_find(t, &r, &ti)) {
			return test_fail("Find failed\n");
		}
		if (*(intptr_t*)zix_sorted_array_get_data(ti) != r) {
			return test_fail("Data corrupt (saw %" PRIdPTR ", expected %zu)\n",
			                 *(intptr_t*)zix_sorted_array_get_data(ti), r);
		}
	}
	fprintf(search_dat, "\t%lf", bench_end(&search_start));

	srand(seed);

	// Iterate over all elements
	struct timespec iter_start = bench_start();
	size_t i = 0;
	intptr_t last = -1;
	for (ZixSortedArrayIter iter = zix_sorted_array_begin(t);
	     !zix_sorted_array_iter_is_end(t, iter);
	     iter = zix_sorted_array_iter_next(t, iter), ++i) {
		r = rand();
		const intptr_t iter_data = *(intptr_t*)zix_sorted_array_get_data(iter);
		if (iter_data < last) {
			return test_fail("Iter corrupt (%" PRIdPTR " < %zu)\n",
			                 iter_data, last);
		}
		last = iter_data;
	}
	fprintf(iter_dat, "\t%lf", bench_end(&iter_start));

	srand(seed);

	// Delete all elements
	struct timespec del_start = bench_start();
	for (size_t i = 0; i < n_elems; i++) {
		r = rand();
		ZixSortedArrayIter item;
		if (zix_sorted_array_find(t, &r, &item) != ZIX_STATUS_SUCCESS) {
			return test_fail("Failed to find item to remove\n");
		}
		if (zix_sorted_array_remove(t, item)) {
			return test_fail("Error removing item\n");
		}
	}
	fprintf(del_dat, "\t%lf", bench_end(&del_start));

	if (zix_sorted_array_size(t) != 0) {
		return test_fail("Array is not empty after removing all items\n");
	}

	zix_sorted_array_free(t);

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
	fprintf(insert_dat, "# n\tZixTree\tZixSortedArray\tGSequence\n");
	fprintf(search_dat, "# n\tZixTree\tZixSortedArray\tGSequence\n");
	fprintf(iter_dat, "# n\tZixTree\tZixSortedArray\tGSequence\n");
	fprintf(del_dat, "# n\tZixTree\tZixSortedArray\tGSequence\n");
	for (size_t n = min_n; n <= max_n; n *= 2) {
		fprintf(insert_dat, "%zu", n);
		fprintf(search_dat, "%zu", n);
		fprintf(iter_dat, "%zu", n);
		fprintf(del_dat, "%zu", n);
		bench_zix_tree(n, insert_dat, search_dat, iter_dat, del_dat);
		bench_zix_sorted_array(n, insert_dat, search_dat, iter_dat, del_dat);
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
