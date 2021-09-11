// Copyright 2011-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef BENCH_H
#define BENCH_H

#define _POSIX_C_SOURCE 199309L

#include <time.h>

typedef struct timespec BenchmarkTime;

static inline double
bench_elapsed_s(const BenchmarkTime* start, const BenchmarkTime* end)
{
  return ((double)(end->tv_sec - start->tv_sec) +
          ((double)(end->tv_nsec - start->tv_nsec) * 0.000000001));
}

static inline BenchmarkTime
bench_start(void)
{
  BenchmarkTime start_t;
  clock_gettime(CLOCK_REALTIME, &start_t);
  return start_t;
}

static inline double
bench_end(const BenchmarkTime* start_t)
{
  BenchmarkTime end_t;
  clock_gettime(CLOCK_REALTIME, &end_t);
  return bench_elapsed_s(start_t, &end_t);
}

#endif // BENCH_H
