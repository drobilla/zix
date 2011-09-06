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

#ifndef ZIX_TREE_H
#define ZIX_TREE_H

#include <stdbool.h>

#include "zix/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
   @addtogroup zix
   @{
   @name Tree
   @{
*/

/**
   Function for comparing two elements.
*/
typedef int (*ZixComparator)(const void* a, const void* b, void* user_data);

/**
   A balanced binary search tree.
*/
typedef struct ZixTreeImpl ZixTree;

/**
   A node in a @ref ZixTree.
*/
typedef struct ZixTreeNodeImpl ZixTreeNode;

/**
   An iterator over a @ref ZixTree.
*/
typedef ZixTreeNode* ZixTreeIter;

/**
   Create a new (empty) tree.
*/
ZixTree*
zix_tree_new(bool allow_duplicates, ZixComparator cmp, void* cmp_data);

/**
   Free @a t.
*/
void
zix_tree_free(ZixTree* t);

/**
   Insert the element @a e into @a t and point @a ti at the new element.
*/
int
zix_tree_insert(ZixTree* t, const void* e, ZixTreeIter* ti);

/**
   Remove the item pointed at by @a ti from @a t.
*/
int
zix_tree_remove(ZixTree* t, ZixTreeIter ti);

/**
   Set @a ti to be the largest element <= @a e in @a t.
   If no such item exists, @a ti is set to NULL.
*/
int
zix_tree_find(const ZixTree* t, const void* e, ZixTreeIter* ti);

/**
   Return the data associated with the given avltree item.
*/
const void*
zix_tree_get_data(ZixTreeIter ti);

/**
   Return an iterator to the first (smallest) element in @a t.
*/
ZixTreeIter
zix_tree_begin(ZixTree* t);

ZixTreeIter
zix_tree_end(ZixTree* t);

bool
zix_tree_iter_is_end(ZixTreeIter i);

ZixTreeIter
zix_tree_iter_next(ZixTreeIter i);

/**
   @}
   @}
*/

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* ZIX_TREE_H */
