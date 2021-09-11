/*
  Copyright 2011-2020 David Robillard <d@drobilla.net>

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

#ifndef ZIX_BTREE_H
#define ZIX_BTREE_H

#include "zix/common.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   @addtogroup zix
   @{
   @name BTree
   @{
*/

/**
   The maximum height of a ZixBTree.

   This is exposed because it determines the size of iterators, which are
   statically sized so they can used on the stack.  The usual degree (or
   "fanout") of a B-Tree is high enough that a relatively short tree can
   contain many elements.  With the default page size of 4 KiB, the default
   height of 6 is enough to store trillions.
*/
#ifndef ZIX_BTREE_MAX_HEIGHT
#  define ZIX_BTREE_MAX_HEIGHT 6u
#endif

/// A B-Tree
typedef struct ZixBTreeImpl ZixBTree;

/// A B-Tree node (opaque)
typedef struct ZixBTreeNodeImpl ZixBTreeNode;

/**
   An iterator over a B-Tree.

   Note that modifying the tree invalidates all iterators.

   The contents of this type are considered an implementation detail and should
   not be used directly by clients.  They are nevertheless exposed here so that
   iterators can be allocated on the stack.
*/
typedef struct {
  ZixBTreeNode* nodes[ZIX_BTREE_MAX_HEIGHT];   ///< Parallel node pointer stack
  uint16_t      indexes[ZIX_BTREE_MAX_HEIGHT]; ///< Parallel child index stack
  uint16_t      level;                         ///< Current level in stack
} ZixBTreeIter;

/// A static end iterator for convenience
static const ZixBTreeIter zix_btree_end_iter = {
  {NULL, NULL, NULL, NULL, NULL, NULL},
  {0u, 0u, 0u, 0u, 0u, 0u},
  0u};

/// Create a new (empty) B-Tree
ZIX_API
ZixBTree*
zix_btree_new(ZixComparator cmp, const void* cmp_data);

/**
   Free `t` and all the nodes it contains.

   @param destroy Function to call once for every value in the tree.  This can
   be used to free values if they are dynamically allocated.
*/
ZIX_API
void
zix_btree_free(ZixBTree* t, ZixDestroyFunc destroy);

/**
   Clear everything from `t`, leaving it empty.

   @param destroy Function called exactly once for every value in the tree,
   just before that value is removed from the tree.
*/
ZIX_API
void
zix_btree_clear(ZixBTree* t, ZixDestroyFunc destroy);

/// Return the number of elements in `t`
ZIX_PURE_API
size_t
zix_btree_size(const ZixBTree* t);

/// Insert the element `e` into `t`
ZIX_API
ZixStatus
zix_btree_insert(ZixBTree* t, void* e);

/**
   Remove the value `e` from `t`.

   @param t Tree to remove from.

   @param e Value to remove.

   @param out Set to point to the removed pointer (which may not equal `e`).

   @param next On successful return, set to point at the value that immediately
   followed `e`.
*/
ZIX_API
ZixStatus
zix_btree_remove(ZixBTree* t, const void* e, void** out, ZixBTreeIter* next);

/**
   Set `ti` to an element equal to `e` in `t`.

   If no such item exists, `ti` is set to the end.
*/
ZIX_API
ZixStatus
zix_btree_find(const ZixBTree* t, const void* e, ZixBTreeIter* ti);

/**
   Set `ti` to the smallest element in `t` that is not less than `e`.

   Wildcards are supported, so if the search key `e` compares equal to many
   values in the tree, `ti` will be set to the least such element.  The search
   key `e` is always passed as the second argument to the comparator.
*/
ZIX_API
ZixStatus
zix_btree_lower_bound(const ZixBTree* t, const void* e, ZixBTreeIter* ti);

/// Return the data associated with the given tree item
ZIX_PURE_API
void*
zix_btree_get(ZixBTreeIter ti);

/// Return an iterator to the first (smallest) element in `t`
ZIX_PURE_API
ZixBTreeIter
zix_btree_begin(const ZixBTree* t);

/// Return an iterator to the end of `t` (one past the last element)
ZIX_CONST_API
ZixBTreeIter
zix_btree_end(const ZixBTree* t);

/// Return true iff `lhs` is equal to `rhs`
ZIX_PURE_API
bool
zix_btree_iter_equals(ZixBTreeIter lhs, ZixBTreeIter rhs);

/// Return true iff `i` is an iterator at the end of a tree
static inline bool
zix_btree_iter_is_end(const ZixBTreeIter i)
{
  return i.level == 0 && !i.nodes[0];
}

/// Increment `i` to point to the next element in the tree
ZIX_API
ZixStatus
zix_btree_iter_increment(ZixBTreeIter* i);

/**
   @}
   @}
*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZIX_BTREE_H */
