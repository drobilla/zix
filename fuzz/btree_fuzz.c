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

#include <zix/btree.h>

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef enum {
  INSERT = 0,
  REMOVE = 1,
} Opcode;

static int
int_cmp(const void* a, const void* b, const void* ZIX_UNUSED(user_data))
{
  const uint16_t ia = (uint16_t)(uintptr_t)a;
  const uint16_t ib = (uint16_t)(uintptr_t)b;

  return ia < ib ? -1 : ia > ib ? 1 : 0;
}

static void
execute(ZixBTree* const t, const Opcode opcode, const uint16_t operand)
{
  assert(opcode == INSERT || opcode == REMOVE);

  void* const  value    = (void*)(uintptr_t)operand;
  const size_t pre_size = zix_btree_size(t);

  if (opcode == INSERT) {
    /* fprintf(stderr, "I %u\n", operand); */
    const ZixStatus st = zix_btree_insert(t, value);

    assert(st == ZIX_STATUS_SUCCESS || st == ZIX_STATUS_EXISTS);

    if (st == ZIX_STATUS_EXISTS) {
      assert(zix_btree_size(t) == pre_size);
    } else {
      assert(zix_btree_size(t) == pre_size + 1);
    }

    ZixBTreeIter ti = zix_btree_end(t);
    assert(zix_btree_find(t, value, &ti) == ZIX_STATUS_SUCCESS);
    assert(!zix_btree_iter_is_end(ti));

    ZixBTreeIter tj = zix_btree_end(t);
    assert(zix_btree_lower_bound(t, int_cmp, NULL, value, &tj) ==
           ZIX_STATUS_SUCCESS);
    assert(!zix_btree_iter_is_end(tj));

    /* if (!zix_btree_iter_equals(ti, tj)) { */
    /* 	uint16_t vi = (uint16_t)(uintptr_t)zix_btree_get(ti); */
    /* 	uint16_t vj = (uint16_t)(uintptr_t)zix_btree_get(tj); */

    /* 	fprintf(stderr, "ERR levels: %u %u\n", ti->n_levels, tj->n_levels); */
    /* 	fprintf(stderr, "ERR level: %u %u\n", ti->level, tj->level); */
    /* 	for (unsigned l = 0; l < ti->level; ++l) { */
    /* 		fprintf(stderr, */
    /* 		        "ERR level %u: %p %u %p %u\n", */
    /* 		        l, */
    /* 		        (void*)ti->stack[l].node, */
    /* 		        ti->stack[l].index, */
    /* 		        (void*)tj->stack[l].node, */
    /* 		        tj->stack[l].index); */
    /* 	} */
    /* 	fprintf(stderr, "ERR: %u %u\n", vi, vj); */
    /* } */

    assert(zix_btree_iter_equals(ti, tj));

  } else if (opcode == REMOVE) {
    /* fprintf(stderr, "R %u\n", operand); */
    uintptr_t removed = 0;
    /* ZixBTreeIter* next    = NULL; */

    ZixBTreeIter    next = zix_btree_end(t);
    const ZixStatus st   = zix_btree_remove(t, value, (void**)&removed, &next);
    assert(st == ZIX_STATUS_SUCCESS || st == ZIX_STATUS_NOT_FOUND);

    if (st == ZIX_STATUS_NOT_FOUND) {
      assert(zix_btree_size(t) == pre_size);
    } else {
      assert(zix_btree_size(t) == pre_size - 1);
      assert(removed == operand);
    }

    ZixBTreeIter ti = zix_btree_end(t);
    assert(zix_btree_find(t, value, &ti) == ZIX_STATUS_NOT_FOUND);
    assert(zix_btree_iter_is_end(ti));
  }
}

int
main(int argc, char** argv)
{
  if (argc != 2) {
    fprintf(stderr, "Usage: %s INPUT_FILE\n", argv[0]);
    return 1;
  }

  FILE* const input = fopen(argv[1], "r");
  if (!input) {
    fprintf(stderr, "Usage: %s INPUT_FILE\n", argv[0]);
    return 2;
  }

  ZixBTree* const t = zix_btree_new(NULL, int_cmp, NULL);

  while (1) {
    uint16_t instruction = 0;
    if (fread(&instruction, sizeof(instruction), 1, input) != 1) {
      if (ferror(input)) {
        fprintf(
          stderr, "error: Failed to read instruction (%s)\n", strerror(errno));
        zix_btree_free(t, NULL, NULL);
        fclose(input);
        return 3;
      } else if (feof(input)) {
        break;
      }
    }

    const Opcode   opcode = (Opcode)(instruction >> 15u);
    const uint16_t value  = instruction & ~(1u << 15u);

    execute(t, opcode, value);
  }

  zix_btree_free(t, NULL, NULL);
  fclose(input);
  return 0;
}
