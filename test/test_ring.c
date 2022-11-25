// Copyright 2011-2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#undef NDEBUG

#include "failing_allocator.h"
#include "test_args.h"

#include "zix/attributes.h"
#include "zix/ring.h"
#include "zix/thread.h"

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MSG_SIZE 20U

static ZixRing* ring       = 0;
static unsigned n_writes   = 0;
static bool     read_error = false;

static int
gen_msg(int* const msg, int start)
{
  for (unsigned i = 0U; i < MSG_SIZE; ++i) {
    msg[i] = start;
    start  = (start + 1) % INT_MAX;
  }
  return start;
}

ZIX_PURE_FUNC
static int
cmp_msg(const int* const msg1, const int* const msg2)
{
  for (unsigned i = 0U; i < MSG_SIZE; ++i) {
    assert(msg1[i] == msg2[i]);
  }

  return 1;
}

static ZixThreadResult ZIX_THREAD_FUNC
reader(void* ZIX_UNUSED(arg))
{
  printf("Reader starting\n");

  int      ref_msg[MSG_SIZE];  // Reference generated for comparison
  int      read_msg[MSG_SIZE]; // Read from ring
  unsigned count = 0;
  int      start = gen_msg(ref_msg, 0);
  for (unsigned i = 0; i < n_writes; ++i) {
    if (zix_ring_read_space(ring) >= MSG_SIZE * sizeof(int)) {
      if (zix_ring_read(ring, read_msg, MSG_SIZE * sizeof(int))) {
        assert(cmp_msg(ref_msg, read_msg));
        start = gen_msg(ref_msg, start);
        ++count;
      }
    }
  }

  printf("Reader finished\n");
  return ZIX_THREAD_RESULT;
}

static ZixThreadResult ZIX_THREAD_FUNC
writer(void* ZIX_UNUSED(arg))
{
  printf("Writer starting\n");

  int write_msg[MSG_SIZE]; // Written to ring
  int start = gen_msg(write_msg, 0);
  for (unsigned i = 0; i < n_writes; ++i) {
    if (zix_ring_write_space(ring) >= MSG_SIZE * sizeof(int)) {
      if (zix_ring_write(ring, write_msg, MSG_SIZE * sizeof(int))) {
        start = gen_msg(write_msg, start);
      }
    }
  }

  printf("Writer finished\n");
  return ZIX_THREAD_RESULT;
}

static int
test_ring(const unsigned size)
{
  zix_ring_free(NULL);

  printf("Testing %u writes of %u ints to a %u int ring...\n",
         n_writes,
         MSG_SIZE,
         size);

  ring = zix_ring_new(NULL, size);
  assert(ring);
  assert(zix_ring_read_space(ring) == 0);
  assert(zix_ring_write_space(ring) == zix_ring_capacity(ring));

  zix_ring_mlock(ring);

  ZixThread reader_thread; // NOLINT
  assert(!zix_thread_create(&reader_thread, MSG_SIZE * 4UL, reader, NULL));

  ZixThread writer_thread; // NOLINT
  assert(!zix_thread_create(&writer_thread, MSG_SIZE * 4UL, writer, NULL));

  assert(!zix_thread_join(reader_thread));
  assert(!zix_thread_join(writer_thread));

  assert(!read_error);
  assert(ring);
  zix_ring_reset(ring);
  assert(zix_ring_read_space(ring) == 0);
  assert(zix_ring_write_space(ring) == zix_ring_capacity(ring));

  zix_ring_write(ring, "a", 1);
  zix_ring_write(ring, "b", 1);

  char     buf = 0;
  uint32_t n   = zix_ring_peek(ring, &buf, 1);
  assert(n == 1);
  assert(buf == 'a');

  n = zix_ring_skip(ring, 1);
  assert(n == 1);

  assert(zix_ring_read_space(ring) == 1);

  n = zix_ring_read(ring, &buf, 1);
  assert(n == 1);
  assert(buf == 'b');

  assert(zix_ring_read_space(ring) == 0);

  n = zix_ring_peek(ring, &buf, 1);
  assert(n == 0);

  n = zix_ring_read(ring, &buf, 1);
  assert(n == 0);

  n = zix_ring_skip(ring, 1);
  assert(n == 0);

  char* big_buf = (char*)calloc(size, 1);
  n             = zix_ring_write(ring, big_buf, size - 1);
  assert(n == (uint32_t)size - 1);

  n = zix_ring_write(ring, big_buf, size);
  assert(n == 0);

  free(big_buf);
  zix_ring_free(ring);
  return 0;
}

static void
test_failed_alloc(void)
{
  ZixFailingAllocator allocator = zix_failing_allocator();

  // Successfully allocate a ring to count the number of allocations
  ring = zix_ring_new(&allocator.base, 512);
  assert(ring);

  // Test that each allocation failing is handled gracefully
  const size_t n_new_allocs = allocator.n_allocations;
  for (size_t i = 0U; i < n_new_allocs; ++i) {
    allocator.n_remaining = i;
    assert(!zix_ring_new(&allocator.base, 512));
  }

  zix_ring_free(ring);
}

int
main(int argc, char** argv)
{
  if (argc > 1 && argv[1][0] == '-') {
    printf("Usage: %s SIZE N_WRITES\n", argv[0]);
    return 1;
  }

  const unsigned size =
    (argc > 1) ? (unsigned)zix_test_size_arg(argv[1], 4U, 1U << 20U) : 1024U;

  n_writes = (argc > 2) ? (unsigned)zix_test_size_arg(argv[2], 4U, 1U << 20U)
                        : size * 1024;

  test_failed_alloc();
  test_ring(size);
  return 0;
}
