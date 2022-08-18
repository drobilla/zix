// Copyright 2011-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef ZIX_TREE_DEBUG_H
#define ZIX_TREE_DEBUG_H

#include <inttypes.h>

#ifdef ZIX_TREE_HYPER_VERIFY
static size_t
height(ZixTreeNode* n)
{
  if (!n) {
    return 0;
  } else {
    return 1 + MAX(height(n->left), height(n->right));
  }
}
#endif

#if defined(ZIX_TREE_VERIFY) || !defined(NDEBUG)
static bool
verify_balance(ZixTreeNode* n)
{
  if (!n) {
    return true;
  }

  if (n->balance < -2 || n->balance > 2) {
    fprintf(stderr,
            "Balance out of range : %ld (balance %d)\n",
            (intptr_t)n->data,
            n->balance);
    return false;
  }

  if (n->balance < 0 && !n->left) {
    fprintf(stderr,
            "Bad balance : %ld (balance %d) has no left child\n",
            (intptr_t)n->data,
            n->balance);
    return false;
  }

  if (n->balance > 0 && !n->right) {
    fprintf(stderr,
            "Bad balance : %ld (balance %d) has no right child\n",
            (intptr_t)n->data,
            n->balance);
    return false;
  }

  if (n->balance != 0 && !n->left && !n->right) {
    fprintf(stderr,
            "Bad balance : %ld (balance %d) has no children\n",
            (intptr_t)n->data,
            n->balance);
    return false;
  }

#  ifdef ZIX_TREE_HYPER_VERIFY
  const intptr_t left_height  = (intptr_t)height(n->left);
  const intptr_t right_height = (intptr_t)height(n->right);
  if (n->balance != right_height - left_height) {
    fprintf(stderr,
            "Bad balance at %ld: h_r (%" PRIdPTR ")"
            "- l_h (%" PRIdPTR ") != %d\n",
            (intptr_t)n->data,
            right_height,
            left_height,
            n->balance);
    assert(false);
    return false;
  }
#  endif

  return true;
}
#endif

#ifdef ZIX_TREE_VERIFY
static bool
verify(ZixTree* t, ZixTreeNode* n)
{
  if (!n) {
    return true;
  }

  if (n->parent) {
    if ((n->parent->left != n) && (n->parent->right != n)) {
      fprintf(stderr, "Corrupt child/parent pointers\n");
      return false;
    }
  }

  if (n->left) {
    if (t->cmp(n->left->data, n->data, t->cmp_data) > 0) {
      fprintf(
        stderr, "Bad order: %" PRIdPTR " with left child\n", (intptr_t)n->data);
      fprintf(stderr, "%p ? %p\n", (void*)n, (void*)n->left);
      fprintf(stderr,
              "%" PRIdPTR " ? %" PRIdPTR "\n",
              (intptr_t)n->data,
              (intptr_t)n->left->data);
      return false;
    }

    if (!verify(t, n->left)) {
      return false;
    }
  }

  if (n->right) {
    if (t->cmp(n->right->data, n->data, t->cmp_data) < 0) {
      fprintf(stderr,
              "Bad order: %" PRIdPTR " with right child\n",
              (intptr_t)n->data);
      return false;
    }

    if (!verify(t, n->right)) {
      return false;
    }
  }

  if (!verify_balance(n)) {
    return false;
  }

  if (n->balance <= -2 || n->balance >= 2) {
    fprintf(stderr, "Imbalance: %p (balance %d)\n", (void*)n, n->balance);
    return false;
  }

  return true;
}
#endif

#endif // ZIX_TREE_DEBUG_H
