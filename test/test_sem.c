// Copyright 2012-2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#undef NDEBUG

#include "zix/attributes.h"
#include "zix/sem.h"
#include "zix/thread.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static ZixSem   sem;
static unsigned n_signals = 1024;

static void*
reader(void* ZIX_UNUSED(arg))
{
  printf("Reader starting\n");

  for (unsigned i = 0; i < n_signals; ++i) {
    zix_sem_wait(&sem);
  }

  printf("Reader finished\n");
  return NULL;
}

static void*
writer(void* ZIX_UNUSED(arg))
{
  printf("Writer starting\n");

  for (unsigned i = 0; i < n_signals; ++i) {
    zix_sem_post(&sem);
  }

  printf("Writer finished\n");
  return NULL;
}

int
main(int argc, char** argv)
{
  if (argc > 2) {
    printf("Usage: %s N_SIGNALS\n", argv[0]);
    return 1;
  }

  if (argc > 1) {
    n_signals = (unsigned)strtol(argv[1], NULL, 10);
  }

  printf("Testing %u signals...\n", n_signals);

  assert(!zix_sem_init(&sem, 0));

  ZixThread reader_thread; // NOLINT
  assert(!zix_thread_create(&reader_thread, 128, reader, NULL));

  ZixThread writer_thread; // NOLINT
  assert(!zix_thread_create(&writer_thread, 128, writer, NULL));

  zix_thread_join(reader_thread, NULL);
  zix_thread_join(writer_thread, NULL);

  zix_sem_destroy(&sem);
  return 0;
}
