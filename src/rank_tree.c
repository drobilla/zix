// Copyright 2011-2025 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include <zix/rank_tree.h>

#include <zix/allocator.h>
#include <zix/status.h>

#include <assert.h>
#include <stdint.h>
#include <string.h>

#define ZIX_RANK_TREE_PAGE_SIZE 4096U
#define ZIX_RANK_TREE_FANOUT \
  ((uint16_t)(ZIX_RANK_TREE_PAGE_SIZE / sizeof(void*)))

#if UINTPTR_MAX > UINT32_MAX // 64-bit
#  define ZIX_RANK_TREE_INDEX_BITS 9U
#  define ZIX_RANK_TREE_INDEX_MASK 0x1FFU
#  define ZIX_RANK_TREE_MAX_HEIGHT 7U
#  define ZIX_RANK_TREE_MAX_LEVELS 8U
#else // 32-bit
#  define ZIX_RANK_TREE_INDEX_BITS 10U
#  define ZIX_RANK_TREE_INDEX_MASK 0x3FFU
#  define ZIX_RANK_TREE_MAX_HEIGHT 3U
#  define ZIX_RANK_TREE_MAX_LEVELS 4U
#endif

typedef uint16_t ChildIndex;
typedef uint8_t  TreeHeight;

typedef union ZixRankTreeNodeImpl ZixRankTreeNode;

typedef struct {
  ChildIndex indices[ZIX_RANK_TREE_MAX_LEVELS];
} ZixRankTreePath;

struct ZixRankTreeImpl {
  ZixAllocator*    allocator; ///< Allocator for this structure and nodes
  ZixRankTreeNode* root;      ///< Pointer to root node
  size_t           size;      ///< Total number of elements in tree
  TreeHeight       height;    ///< Height where 0 has one node (the root)
};

union ZixRankTreeNodeImpl {
  uintptr_t        vals[ZIX_RANK_TREE_FANOUT];     ///< Elements (leaf)
  ZixRankTreeNode* children[ZIX_RANK_TREE_FANOUT]; ///< Children (internal)
};

#if ((defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L) || \
     (defined(__cplusplus) && __cplusplus >= 201103L))
static_assert(sizeof(ZixRankTree) <= ZIX_RANK_TREE_PAGE_SIZE, "");
static_assert(sizeof(ZixRankTreeNode) == ZIX_RANK_TREE_PAGE_SIZE, "");
#endif

static ZixRankTreePath
zix_rank_tree_parse_rank(const size_t rank)
{
  ZixRankTreePath path      = {{0U}};
  size_t          remaining = rank;
  for (unsigned i = 0U; i < ZIX_RANK_TREE_MAX_LEVELS; ++i) {
    path.indices[ZIX_RANK_TREE_MAX_HEIGHT - i] =
      remaining & ZIX_RANK_TREE_INDEX_MASK;
    remaining = remaining >> ZIX_RANK_TREE_INDEX_BITS;
  }

  return path;
}

static ZixRankTreeNode*
zix_rank_tree_node_new(ZixAllocator* const allocator)
{
  ZixRankTreeNode* const node = (ZixRankTreeNode*)zix_aligned_alloc(
    allocator, ZIX_RANK_TREE_PAGE_SIZE, ZIX_RANK_TREE_PAGE_SIZE);

  if (node) {
    memset(node, 0, ZIX_RANK_TREE_PAGE_SIZE);
  }

  return node;
}

ZixRankTree*
zix_rank_tree_new(ZixAllocator* const allocator)
{
#if !((defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L) || \
      (defined(__cplusplus) && __cplusplus >= 201103L))
  assert(sizeof(ZixRankTree) <= ZIX_RANK_TREE_PAGE_SIZE);
  assert(sizeof(ZixRankTreeNode) == ZIX_RANK_TREE_PAGE_SIZE);
#endif

  ZixRankTree* const t =
    (ZixRankTree*)zix_malloc(allocator, sizeof(ZixRankTree));

  if (t) {
    t->allocator = allocator;
    t->root      = NULL;
    t->size      = 0U;
    t->height    = 0U;
  }

  return t;
}

static void
zix_rank_tree_free_children(ZixRankTree* const           t,
                            ZixRankTreePath              this_path,
                            const ZixRankTreePath        last_path,
                            const unsigned               height,
                            ZixRankTreeNode* const       n,
                            const ZixRankTreeDestroyFunc destroy,
                            void* const                  destroy_user_data)
{
  if (height) {
    // Recursively free children of internal node
    for (ChildIndex i = 0U; i < ZIX_RANK_TREE_FANOUT; ++i) {
      this_path.indices[ZIX_RANK_TREE_MAX_HEIGHT - height] = i;

      ZixRankTreeNode* const child = n->children[i];
      if (child) {
        zix_rank_tree_free_children(t,
                                    this_path,
                                    last_path,
                                    height - 1U,
                                    child,
                                    destroy,
                                    destroy_user_data);
        zix_aligned_free(t->allocator, child);
      }
    }
  } else if (destroy) {
    // Determine how many elements are in this leaf
    unsigned n_vals = last_path.indices[ZIX_RANK_TREE_MAX_HEIGHT] + 1U;
    for (unsigned i = 1U; i <= t->height; ++i) {
      if (this_path.indices[ZIX_RANK_TREE_MAX_HEIGHT - i] <
          last_path.indices[ZIX_RANK_TREE_MAX_HEIGHT - i]) {
        n_vals = ZIX_RANK_TREE_FANOUT;
        break;
      }
    }

    // Destroy values in leaf node
    for (unsigned i = 0U; i < n_vals; ++i) {
      destroy(n->vals[i], destroy_user_data);
    }
  }
}

void
zix_rank_tree_free(ZixRankTree* const           t,
                   const ZixRankTreeDestroyFunc destroy,
                   void* const                  destroy_user_data)
{
  if (t) {
    zix_rank_tree_clear(t, destroy, destroy_user_data);
    zix_free(t->allocator, t);
  }
}

void
zix_rank_tree_clear(ZixRankTree* const           t,
                    const ZixRankTreeDestroyFunc destroy,
                    void* const                  destroy_user_data)
{
  assert(t);

  if (t->size) {
    const ZixRankTreePath first_path = {0U};
    const ZixRankTreePath last_path  = zix_rank_tree_parse_rank(t->size - 1U);
    zix_rank_tree_free_children(
      t, first_path, last_path, t->height, t->root, destroy, destroy_user_data);

    zix_aligned_free(t->allocator, t->root);
    t->root   = NULL;
    t->size   = 0U;
    t->height = 0U;
  }
}

size_t
zix_rank_tree_size(const ZixRankTree* const t)
{
  assert(t);
  return t->size;
}

uint8_t
zix_rank_tree_height(const ZixRankTree* const t)
{
  assert(t);
  return t->height;
}

uintptr_t
zix_rank_tree_at(const ZixRankTree* const t, const size_t rank)
{
  assert(t);
  if (rank >= t->size) {
    return 0U;
  }

  const ZixRankTreePath path       = zix_rank_tree_parse_rank(rank);
  const unsigned        root_level = ZIX_RANK_TREE_MAX_HEIGHT - t->height;
  ZixRankTreeNode*      node       = t->root;
  for (unsigned i = root_level; i < ZIX_RANK_TREE_MAX_HEIGHT; ++i) {
    assert(node->children[path.indices[i]]);
    node = node->children[path.indices[i]];
  }

  return node->vals[path.indices[ZIX_RANK_TREE_MAX_HEIGHT]];
}

ZixStatus
zix_rank_tree_push(ZixRankTree* const t, const uintptr_t e)
{
  assert(t);
  assert(t->height < ZIX_RANK_TREE_MAX_HEIGHT);

  if (!t->root) {
    assert(!t->size);
    if (!(t->root = zix_rank_tree_node_new(t->allocator))) {
      return ZIX_STATUS_NO_MEM;
    }

    t->root->vals[0] = e;
    ++t->size;
    return ZIX_STATUS_SUCCESS;
  }

  const size_t          rank        = t->size;
  const ZixRankTreePath path        = zix_rank_tree_parse_rank(rank);
  const unsigned        n_levels    = t->height + 1U;
  const unsigned        height_bits = n_levels * ZIX_RANK_TREE_INDEX_BITS;
  const unsigned        height_max  = ((1U << height_bits) - 1U);

  if (rank > height_max) {
    // Add a new root to grow upwards
    ZixRankTreeNode* const root = zix_rank_tree_node_new(t->allocator);
    if (!root) {
      return ZIX_STATUS_NO_MEM;
    }

    root->children[0] = t->root;
    t->height         = (TreeHeight)(t->height + !!t->root);
    t->root           = root;
  }

  // Walk down to the appropriate leaf
  const unsigned   root_level = ZIX_RANK_TREE_MAX_HEIGHT - t->height;
  ZixRankTreeNode* node       = t->root;
  for (unsigned i = root_level; i < ZIX_RANK_TREE_MAX_HEIGHT; ++i) {
    const ChildIndex index = path.indices[i];
    ZixRankTreeNode* child = node->children[index];
    if (!child) {
      child = zix_rank_tree_node_new(t->allocator);
      if (!child) {
        return ZIX_STATUS_NO_MEM;
      }

      node->children[index] = child;
    }

    node = child;
  }

  node->vals[path.indices[ZIX_RANK_TREE_MAX_HEIGHT]] = e;

  ++t->size;
  return ZIX_STATUS_SUCCESS;
}

ZixStatus
zix_rank_tree_pop(ZixRankTree* const t)
{
  assert(t);
  if (!t->size) {
    return ZIX_STATUS_NOT_FOUND;
  }

  // Decrease size, and handle now-empty case by clearing
  const size_t rank = --t->size;
  if (!rank) {
    zix_aligned_free(t->allocator, t->root);
    t->root = NULL;
    return ZIX_STATUS_SUCCESS;
  }

  // Parse the rank to get the path and walk down to the leaf
  const ZixRankTreePath path = zix_rank_tree_parse_rank(rank);
  ZixRankTreeNode*      parents[ZIX_RANK_TREE_MAX_LEVELS] = {NULL};
  ZixRankTreeNode*      node                              = t->root;
  const unsigned        root_level = ZIX_RANK_TREE_MAX_HEIGHT - t->height;
  for (unsigned i = root_level; i < ZIX_RANK_TREE_MAX_HEIGHT; ++i) {
    parents[i] = node;
    node       = parents[i]->children[path.indices[i]];
  }

  // Reset the element entry in the leaf
  node->vals[path.indices[ZIX_RANK_TREE_MAX_HEIGHT]] = 0U;

  if (!path.indices[ZIX_RANK_TREE_MAX_HEIGHT]) {
    // Leaf is now empty, walk up deleting now-empty nodes as necessary
    for (int i = ZIX_RANK_TREE_MAX_HEIGHT - 1U; i >= (int)root_level; --i) {
      ZixRankTreeNode* const parent = parents[i];
      const ChildIndex       index  = path.indices[i];

      assert(parent->children[index]);
      zix_aligned_free(t->allocator, parent->children[index]);
      parent->children[index] = NULL;
      if (index) {
        break;
      }
    }

    if (t->height && !t->root->children[1U]) {
      // Root has only one child left, make it the new root
      ZixRankTreeNode* const new_root = t->root->children[0U];
      assert(new_root);
      zix_aligned_free(t->allocator, t->root);
      t->root = new_root;
      --t->height;
    }
  }

  return ZIX_STATUS_SUCCESS;
}
