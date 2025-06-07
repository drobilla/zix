// Copyright 2011-2025 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef ZIX_RANK_TREE_H
#define ZIX_RANK_TREE_H

#include <zix/allocator.h>
#include <zix/attributes.h>
#include <zix/status.h>

#include <stddef.h>
#include <stdint.h>

ZIX_BEGIN_DECLS

/**
   @defgroup zix_rank_tree RankTree
   @ingroup zix_data_structures
   @{
*/

/**
   @defgroup zix_rank_tree_types Types
   @{
*/

/**
   A "rank tree", an efficient B-tree with a vector-like interface.

   This is internally structured like a B-tree, but rather than storing ordered
   elements, presents a simple vector-like interface where elements can be
   retrieved by "rank" (0-based index). This scales more gracefully than an
   array-based vector (by avoiding relocation), and is packed more efficiently
   than a generic B-tree (the tree is always left-complete and nodes are filled
   entirely with either child pointers or values with zero overhead).

   The tree stores opaque pointer-sized values, and is structured so that
   retrieving an element by index is as fast as possible: nearly branchless,
   with only a few memory accesses.
*/
typedef struct ZixRankTreeImpl ZixRankTree;

/// Function to destroy a RankTree element
typedef void (*ZixRankTreeDestroyFunc)(uintptr_t             value,
                                       void* ZIX_UNSPECIFIED user_data);

/**
   @}
   @defgroup zix_rank_tree_setup Setup
   @{
*/

/**
   Create a new empty RankTree.

   This allocates the only the returned structure, no nodes are allocated until
   elements are inserted.

   @param allocator Allocator which is used for the returned structure, and
   later used to allocate tree nodes.
*/
ZIX_API ZIX_NODISCARD ZixRankTree* ZIX_ALLOCATED
zix_rank_tree_new(ZixAllocator* ZIX_NULLABLE allocator);

/**
   Free `t` and all the nodes it contains.

   @param t The tree to free.

   @param destroy Function to call once for every element in the tree.  This
   can be used to free elements if they are dynamically allocated.

   @param destroy_data Opaque user data pointer to pass to `destroy`.
*/
ZIX_API void
zix_rank_tree_free(ZixRankTree* ZIX_NULLABLE           t,
                   ZixRankTreeDestroyFunc ZIX_NULLABLE destroy,
                   void* ZIX_NULLABLE                  destroy_data);

/**
   Clear everything from `t`, leaving it empty.

   @param t The tree to clear.

   @param destroy Function called exactly once for every element in the tree,
   just before that element is removed from the tree.

   @param destroy_data Opaque user data pointer to pass to `destroy`.
*/
ZIX_API void
zix_rank_tree_clear(ZixRankTree* ZIX_NONNULL            t,
                    ZixRankTreeDestroyFunc ZIX_NULLABLE destroy,
                    void* ZIX_NULLABLE                  destroy_data);

/**
   Return the number of elements in `t`.

   The returned size is by definition 1 greater than the greatest stored rank.
*/
ZIX_PURE_API size_t
zix_rank_tree_size(const ZixRankTree* ZIX_NONNULL t);

/**
   Return the height of `t`.

   A tree with a single root note (that is, a single level) has a height of
   zero. The height is at most 7 on 64-bit systems, and 3 on 32-bit systems.
*/
ZIX_API uint8_t
zix_rank_tree_height(const ZixRankTree* ZIX_NONNULL t);

/**
   @}
   @defgroup zix_rank_tree_access Access
   @{
*/

/**
   Return an element of `t` by rank/index.

   Note that this function returns zero if `rank` is out of range, so if this
   isn't a sentinel value for the values stored in the tree, the caller is
   responsible for first checking that the rank is valid.

   @param t The tree to retrieve the element from.
   @param rank The 0-based rank of the element.
   @return The element at the given index, or zero if `rank` is out of range.
*/
ZIX_API uintptr_t
zix_rank_tree_at(const ZixRankTree* ZIX_NONNULL t, size_t rank);

/**
   @}
   @defgroup zix_rank_tree_modification Modification
   @{
*/

/**
   Append the element `e` to `t`.

   @param t Tree to append a new element to.
   @param e Opaque value of new element to store.
   @return #ZIX_STATUS_SUCCESS, or #ZIX_STATUS_NO_MEM.
*/
ZIX_API ZixStatus
zix_rank_tree_push(ZixRankTree* ZIX_NONNULL t, uintptr_t e);

/**
   Remove the element `e` from `t`.

   @param t Tree to remove the last element from.
   @return #ZIX_STATUS_SUCCESS, or #ZIX_STATUS_NOT_FOUND if `t` is empty.
*/
ZIX_API ZixStatus
zix_rank_tree_pop(ZixRankTree* ZIX_NONNULL t);

/**
   @}
   @}
*/

ZIX_END_DECLS

#endif /* ZIX_RANK_TREE_H */
