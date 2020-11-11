/*
  Copyright 2020 David Robillard <http://drobilla.net>

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

#undef NDEBUG

#include "zix/digest.h"

#include <assert.h>
#include <stddef.h>

static void
test_bytes(void)
{
	static const uint8_t data[] = {1, 2, 3, 4, 5, 6, 7, 8};

	for (size_t offset = 0; offset < 7; ++offset) {
		const size_t len = 8 - offset;

		uint32_t d = zix_digest_start();
		for (size_t i = offset; i < 8; ++i) {
			const uint32_t new_d = zix_digest_add(d, &data[i], 1);
			assert(new_d != d);
			d = new_d;
		}

		assert(zix_digest_add(zix_digest_start(), &data[offset], len) == d);
	}
}

static void
test_64(void)
{
	static const uint64_t data[] = {1, 2, 3, 4, 5, 6, 7, 8};

	uint32_t d = zix_digest_start();
	for (size_t i = 0; i < 8; ++i) {
		const uint32_t new_d = zix_digest_add_64(d, &data[i], sizeof(uint64_t));
		assert(new_d != d);
		d = new_d;
	}

	assert(zix_digest_add(zix_digest_start(), data, 8 * sizeof(uint64_t)) == d);
}

static void
test_ptr(void)
{
	static const uint64_t pointees[] = {1, 2, 3, 4, 5, 6, 7, 8};

	static const void* data[] = {&pointees[0],
	                             &pointees[1],
	                             &pointees[2],
	                             &pointees[3],
	                             &pointees[4],
	                             &pointees[5],
	                             &pointees[6],
	                             &pointees[7],
	                             &pointees[8]};

	uint32_t d = zix_digest_start();
	for (size_t i = 0; i < 8; ++i) {
		const uint32_t new_d = zix_digest_add_ptr(d, data[i]);
		assert(new_d != d);
		d = new_d;
	}

	assert(zix_digest_add(zix_digest_start(), data, 8 * sizeof(void*)) == d);
}

ZIX_PURE_FUNC int
main(void)
{
	test_bytes();
	test_64();
	test_ptr();

	return 0;
}
