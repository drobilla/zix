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

#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zix/ring.h"

#define MSG_SIZE 4

ZixRing* ring       = 0;
size_t   n_writes   = 0;
bool     read_error = false;

static int
gen_msg(int* msg, int start)
{
	for (int i = 0; i < MSG_SIZE; ++i) {
		msg[i] = start;
		start  = (start + 1) % INT_MAX;
	}
	return start;
}

static int
cmp_msg(int* msg1, int* msg2)
{
	for (int i = 0; i < MSG_SIZE; ++i) {
		if (msg1[i] != msg2[i]) {
			fprintf(stderr, "ERROR: %d != %d @ %d\n", msg1[i], msg2[i], i);
			return 0;
		}
	}

	return 1;
}

static void*
reader(void* arg)
{
	printf("Reader starting\n");

	int    ref_msg[MSG_SIZE];   // Reference generated for comparison
	int    read_msg[MSG_SIZE];  // Read from ring
	size_t count = 0;
	int    start = gen_msg(ref_msg, 0);
	for (size_t i = 0; i < n_writes; ++i) {
		if (zix_ring_read_space(ring) >= MSG_SIZE * sizeof(int)) {
			if (zix_ring_read(ring, read_msg, MSG_SIZE * sizeof(int))) {
				if (!cmp_msg(ref_msg, read_msg)) {
					printf("FAIL: Message %zu is corrupt\n", count);
					read_error = true;
					return NULL;
				}
				start = gen_msg(ref_msg, start);
				++count;
			}
		}
	}

	printf("Reader finished\n");
	return NULL;
}

static void*
writer(void* arg)
{
	printf("Writer starting\n");

	int write_msg[MSG_SIZE];  // Written to ring
	int start = gen_msg(write_msg, 0);
	for (size_t i = 0; i < n_writes; ++i) {
		if (zix_ring_write_space(ring) >= MSG_SIZE * sizeof(int)) {
			if (zix_ring_write(ring, write_msg, MSG_SIZE * sizeof(int))) {
				start = gen_msg(write_msg, start);
			}
		}
	}

	printf("Writer finished\n");
	return NULL;
}

int
main(int argc, char** argv)
{
	if (argc > 1 && argv[1][0] == '-') {
		printf("Usage: %s SIZE N_WRITES\n", argv[0]);
		return 1;
	}

	int size = 1024;
	if (argc > 1) {
		size = atoi(argv[1]);
	}

	n_writes = size * 1024;
	if (argc > 2) {
		n_writes = atoi(argv[2]);
	}

	printf("Testing %zu writes of %d ints to a %d int ring...\n",
	       n_writes, MSG_SIZE, size);

	ring = zix_ring_new(size);

	pthread_t reader_thread;
	if (pthread_create(&reader_thread, NULL, reader, NULL)) {
		fprintf(stderr, "Failed to create reader thread\n");
		return 1;
	}

	pthread_t writer_thread;
	if (pthread_create(&writer_thread, NULL, writer, NULL)) {
		fprintf(stderr, "Failed to create writer thread\n");
		return 1;
	}

	pthread_join(reader_thread, NULL);
	pthread_join(writer_thread, NULL);

	if (read_error) {
		fprintf(stderr, "FAIL: Read error\n");
		return 1;
	}

	return 0;
}
