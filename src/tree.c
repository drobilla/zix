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

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zix/zix.h"

// #define ZIX_TREE_DUMP         1
// #define ZIX_TREE_VERIFY       1
// #define ZIX_TREE_HYPER_VERIFY 1

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

struct _ZixTree {
	ZixTreeNode*  root;
	ZixComparator cmp;
	void*         cmp_data;
	bool          allow_duplicates;
};

struct _ZixTreeNode {
	struct _ZixTreeNode* parent;
	struct _ZixTreeNode* left;
	struct _ZixTreeNode* right;
	const void*          data;
	int8_t               balance;
};


#ifdef ZIX_TREE_DUMP
static void
zix_tree_print(ZixTreeNode* node, int level)
{
	if (node) {
		if (!node->parent) {
			printf("{{{\n");
		}
		zix_tree_print(node->right, level + 1);
		for (int i = 0; i < level; i++) {
			printf("  ");
		}
		printf("%ld.%d\n", (intptr_t)node->data, node->balance);
		zix_tree_print(node->left, level + 1);
		if (!node->parent) {
			printf("}}}\n");
		}
	}
}
#    define DUMP(t) zix_tree_print(t->root, 0)
#else
#    define DUMP(t)
#endif

ZIX_API
ZixTree*
zix_tree_new(bool allow_duplicates, ZixComparator cmp, void* cmp_data)
{
	ZixTree* t = malloc(sizeof(struct _ZixTree));
	t->root             = NULL;
	t->cmp              = cmp;
	t->cmp_data         = cmp_data;
	t->allow_duplicates = allow_duplicates;
	return t;
}

static void
zix_tree_free_rec(ZixTreeNode* n)
{
	if (n) {
		zix_tree_free_rec(n->left);
		zix_tree_free_rec(n->right);
		free(n);
	}
}

ZIX_API
void
zix_tree_free(ZixTree* t)
{
	zix_tree_free_rec(t->root);

	free(t);
}

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
		fprintf(stderr, "Balance out of range : %ld (balance %d)\n",
		        (intptr_t)n->data, n->balance);
		return false;
	}

	if (n->balance < 0 && !n->left) {
		fprintf(stderr, "Bad balance : %ld (balance %d) has no left child\n",
		        (intptr_t)n->data, n->balance);
		return false;
	}

	if (n->balance > 0 && !n->right) {
		fprintf(stderr, "Bad balance : %ld (balance %d) has no right child\n",
		        (intptr_t)n->data, n->balance);
		return false;
	}

	if (n->balance != 0 && !n->left && !n->right) {
		fprintf(stderr, "Bad balance : %ld (balance %d) has no children\n",
		        (intptr_t)n->data, n->balance);
		return false;
	}

#ifdef ZIX_TREE_HYPER_VERIFY
	const intptr_t left_height  = (intptr_t)height(n->left);
	const intptr_t right_height = (intptr_t)height(n->right);
	if (n->balance != right_height - left_height) {
		fprintf(stderr, "Bad balance at %ld: h_r (%zu) - l_h (%zu) != %d\n",
		        (intptr_t)n->data, right_height, left_height, n->balance);
		assert(false);
		return false;
	}
#endif

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
			fprintf(stderr, "Bad order: %zu with left child\n",
			        (intptr_t)n->data);
			fprintf(stderr, "%p ? %p\n", (void*)n, (void*)n->left);
			fprintf(stderr, "%" PRIdPTR " ? %" PRIdPTR "\n", (intptr_t)n->data,
			        (intptr_t)n->left->data);
			return false;
		}
		if (!verify(t, n->left)) {
			return false;
		}
	}

	if (n->right) {
		if (t->cmp(n->right->data, n->data, t->cmp_data) < 0) {
			fprintf(stderr, "Bad order: %zu with right child\n",
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
		fprintf(stderr, "Imbalance: %p (balance %d)\n",
		        (void*)n, n->balance);
		return false;
	}

	return true;
}
#endif

static void
rotate(ZixTreeNode* p, ZixTreeNode* q)
{
	assert(q->parent == p);
	assert(p->left == q || p->right == q);

	q->parent = p->parent;
	if (q->parent) {
		if (q->parent->left == p) {
			q->parent->left = q;
		} else {
			q->parent->right = q;
		}
	}

	if (p->right == q) {
		// Rotate left
		p->right = q->left;
		q->left  = p;
		if (p->right) {
			p->right->parent = p;
		}
	} else {
		// Rotate right
		assert(p->left == q);
		p->left  = q->right;
		q->right = p;
		if (p->left) {
			p->left->parent = p;
		}
	}

	p->parent = q;
}

/**
 * Rotate left about @a p.
 *
 *    p              q
 *   / \            / \
 *  A   q    =>    p   C
 *     / \        / \
 *    B   C      A   B
 */
static ZixTreeNode*
rotate_left(ZixTreeNode* p, int* height_change)
{
	ZixTreeNode* const q = p->right;
	*height_change = (q->balance == 0) ? 0 : -1;

#ifdef ZIX_TREE_DUMP
	printf("LL %ld\n", (intptr_t)p->data);
#endif

	assert(p->balance == 2);
	assert(q->balance == 0 || q->balance == 1);

	rotate(p, q);

	// p->balance -= 1 + MAX(0, q->balance);
	// q->balance -= 1 - MIN(0, p->balance);
	--q->balance;
	p->balance = -(q->balance);

	assert(verify_balance(p));
	assert(verify_balance(q));
	return q;
}

/**
 * Rotate right about @a p.
 *
 *      p          q
 *     / \        / \
 *    q   C  =>  A   p
 *   / \            / \
 *  A   B          B   C
 *
 */
static ZixTreeNode*
rotate_right(ZixTreeNode* p, int* height_change)
{
	ZixTreeNode* const q = p->left;
	*height_change = (q->balance == 0) ? 0 : -1;

#ifdef ZIX_TREE_DUMP
	printf("RR %ld\n", (intptr_t)p->data);
#endif

	assert(p->balance == -2);
	assert(q->balance == 0 || q->balance == -1);

	rotate(p, q);

	// p->balance += 1 - MIN(0, q->balance);
	// q->balance += 1 + MAX(0, p->balance);
	++q->balance;
	p->balance = -(q->balance);

	assert(verify_balance(p));
	assert(verify_balance(q));

	return q;
}

/**
 * Rotate left about @a p->left then right about @a p.
 *
 *      p             r
 *     / \           / \
 *    q   D  =>    q     p
 *   / \          / \   / \
 *  A   r        A   B C   D
 *     / \
 *    B   C
 *
 */
static ZixTreeNode*
rotate_left_right(ZixTreeNode* p, int* height_change)
{
	ZixTreeNode* const q = p->left;
	ZixTreeNode* const r = q->right;

	assert(p->balance == -2);
	assert(q->balance == 1);
	assert(r->balance == -1 || r->balance == 0 || r->balance == 1);

#ifdef ZIX_TREE_DUMP
	printf("LR %ld  P: %2d  Q: %2d  R: %2d\n",
	       (intptr_t)p->data, p->balance, q->balance, r->balance);
#endif

	rotate(q, r);
	rotate(p, r);

	q->balance -= 1 + MAX(0, r->balance);
	p->balance += 1 - MIN(MIN(0, r->balance) - 1, r->balance + q->balance);
	// r->balance += MAX(0, p->balance) + MIN(0, q->balance);

	// p->balance = (p->left && p->right) ? -MIN(r->balance, 0) : 0;
	// q->balance = - MAX(r->balance, 0);
	r->balance = 0;

	assert(verify_balance(p));
	assert(verify_balance(q));
	assert(verify_balance(r));

	*height_change = -1;
	return r;
}

/**
 * Rotate right about @a p->right then right about @a p.
 *
 *    p               r
 *   / \             / \
 *  A   q    =>    p     q
 *     / \        / \   / \
 *    r   D      A   B C   D
 *   / \
 *  B   C
 *
 */
static ZixTreeNode*
rotate_right_left(ZixTreeNode* p, int* height_change)
{
	ZixTreeNode* const q = p->right;
	ZixTreeNode* const r = q->left;

	assert(p->balance == 2);
	assert(q->balance == -1);
	assert(r->balance == -1 || r->balance == 0 || r->balance == 1);

#ifdef ZIX_TREE_DUMP
	printf("RL %ld  P: %2d  Q: %2d  R: %2d\n",
	       (intptr_t)p->data, p->balance, q->balance, r->balance);
#endif

	rotate(q, r);
	rotate(p, r);

	q->balance += 1 - MIN(0, r->balance);
	p->balance -= 1 + MAX(MAX(0, r->balance) + 1, r->balance + q->balance);
	// r->balance += MAX(0, q->balance) + MIN(0, p->balance);

	// p->balance = (p->left && p->right) ? -MAX(r->balance, 0) : 0;
	// q->balance = - MIN(r->balance, 0);
	r->balance = 0;
	// assert(r->balance == 0);

	assert(verify_balance(p));
	assert(verify_balance(q));
	assert(verify_balance(r));

	*height_change = -1;
	return r;
}

static ZixTreeNode*
zix_tree_rebalance(ZixTree* t, ZixTreeNode* node, int* height_change)
{
#ifdef ZIX_TREE_HYPER_VERIFY
	const size_t old_height = height(node);
#endif
#ifdef ZIX_TREE_DUMP
	printf("REBALANCE %ld (%d)\n", (intptr_t)node->data, node->balance);
#endif
	*height_change = 0;
	const bool is_root = !node->parent;
	assert((is_root && t->root == node) || (!is_root && t->root != node));
	ZixTreeNode* replacement = node;
	if (node->balance == -2) {
		assert(node->left);
		if (node->left->balance == 1) {
			replacement = rotate_left_right(node, height_change);
		} else {
			replacement = rotate_right(node, height_change);
		}
	} else if (node->balance == 2) {
		assert(node->right);
		if (node->right->balance == -1) {
			replacement = rotate_right_left(node, height_change);
		} else {
			replacement = rotate_left(node, height_change);
		}
	}
	if (is_root) {
		assert(!replacement->parent);
		t->root = replacement;
	}
	DUMP(t);
#ifdef ZIX_TREE_HYPER_VERIFY
	assert(old_height + *height_change == height(replacement));
#endif
	return replacement;
}

ZIX_API
int
zix_tree_insert(ZixTree* t, const void* e, ZixTreeIter* ti)
{
#ifdef ZIX_TREE_DUMP
	printf("**** INSERT %ld\n", (intptr_t)e);
#endif
	int          cmp = 0;
	ZixTreeNode* n   = t->root;
	ZixTreeNode* p   = NULL;

	// Find the parent p of e
	while (n) {
		p   = n;
		cmp = t->cmp(e, n->data, t->cmp_data);
		if (cmp < 0) {
			n = n->left;
		} else if (cmp > 0) {
			n = n->right;
		} else if (t->allow_duplicates) {
			n = n->right;
		} else {
			*ti = n;
#ifdef ZIX_TREE_DUMP
			printf("EXISTS!\n");
#endif
			return ZIX_STATUS_EXISTS;
		}
	}

	// Allocate a new node n
	if (!(n = malloc(sizeof(ZixTreeNode)))) {
		return ZIX_STATUS_NO_MEM;
	}
	memset(n, '\0', sizeof(ZixTreeNode));
	n->data    = e;
	n->balance = 0;
	*ti        = n;

	bool p_height_increased = false;

	// Make p the parent of n
	n->parent = p;
	if (!p) {
		t->root = n;
	} else {
		if (cmp < 0) {
			assert(!p->left);
			assert(p->balance == 0 || p->balance == 1);
			p->left = n;
			--p->balance;
			p_height_increased = !p->right;
		} else {
			assert(!p->right);
			assert(p->balance == 0 || p->balance == -1);
			p->right = n;
			++p->balance;
			p_height_increased = !p->left;
		}
	}

	DUMP(t);

	// Rebalance if necessary (at most 1 rotation)
	assert(!p || p->balance == -1 || p->balance == 0 || p->balance == 1);
	if (p && p_height_increased) {
		int height_change = 0;
		for (ZixTreeNode* i = p; i && i->parent; i = i->parent) {
			if (i == i->parent->left) {
				if (--i->parent->balance == -2) {
					zix_tree_rebalance(t, i->parent, &height_change);
					break;
				}
			} else {
				assert(i == i->parent->right);
				if (++i->parent->balance == 2) {
					zix_tree_rebalance(t, i->parent, &height_change);
					break;
				}
			}

			if (i->parent->balance == 0) {
				break;
			}
		}
	}

	DUMP(t);

#ifdef ZIX_TREE_VERIFY
	if (!verify(t, t->root)) {
		return ZIX_STATUS_ERROR;
	}
#endif

	return ZIX_STATUS_SUCCESS;
}

ZIX_API
int
zix_tree_remove(ZixTree* t, ZixTreeIter ti)
{
	ZixTreeNode*  n          = ti;
	ZixTreeNode** pp         = NULL;  // parent pointer
	ZixTreeNode*  to_balance = n->parent;  // lowest node to start rebalace at
	int8_t        d_balance  = 0;  // delta(balance) for n->parent

#ifdef ZIX_TREE_DUMP
	printf("*** REMOVE %ld\n", (intptr_t)n->data);
#endif

	if ((n == t->root) && !n->left && !n->right) {
		t->root = NULL;
		return ZIX_STATUS_SUCCESS;
	}

	// Set pp to the parent pointer to n, if applicable
	if (n->parent) {
		assert(n->parent->left == n || n->parent->right == n);
		if (n->parent->left == n) {  // n is left child
			pp        = &n->parent->left;
			d_balance = 1;
		} else {  // n is right child
			assert(n->parent->right == n);
			pp        = &n->parent->right;
			d_balance = -1;
		}
	}

	assert(!pp || *pp == n);

	int height_change = 0;
	if (!n->left && !n->right) {
		// n is a leaf, just remove it
		if (pp) {
			*pp           = NULL;
			to_balance    = n->parent;
			height_change = (!n->parent->left && !n->parent->right) ? -1 : 0;
		}
	} else if (!n->left) {
		// Replace n with right (only) child
		if (pp) {
			*pp        = n->right;
			to_balance = n->parent;
		} else {
			t->root = n->right;
		}
		n->right->parent = n->parent;
		height_change    = -1;
	} else if (!n->right) {
		// Replace n with left (only) child
		if (pp) {
			*pp        = n->left;
			to_balance = n->parent;
		} else {
			t->root = n->left;
		}
		n->left->parent = n->parent;
		height_change   = -1;
	} else {
		// Replace n with in-order successor (leftmost child of right subtree)
		ZixTreeNode* replace = n->right;
		while (replace->left) {
			assert(replace->left->parent == replace);
			replace = replace->left;
		}

		// Remove replace from parent (replace_p)
		if (replace->parent->left == replace) {
			height_change = replace->parent->right ? 0 : -1;
			d_balance     = 1;
			to_balance    = replace->parent;
			replace->parent->left = replace->right;
		} else {
			assert(replace->parent == n);
			height_change = replace->parent->left ? 0 : -1;
			d_balance     = -1;
			to_balance    = replace->parent;
			replace->parent->right = replace->right;
		}

		if (to_balance == n) {
			to_balance = replace;
		}

		if (replace->right) {
			replace->right->parent = replace->parent;
		}

		replace->balance = n->balance;

		// Swap node to delete with replace
		if (pp) {
			*pp = replace;
		} else {
			assert(t->root == n);
			t->root = replace;
		}
		replace->parent = n->parent;
		replace->left   = n->left;
		n->left->parent = replace;
		replace->right  = n->right;
		if (n->right) {
			n->right->parent = replace;
		}

		assert(!replace->parent
		       || replace->parent->left == replace
		       || replace->parent->right == replace);
	}

	// Rebalance starting at to_balance upwards.
	for (ZixTreeNode* i = to_balance; i; i = i->parent) {
		i->balance += d_balance;
		if (d_balance == 0 || i->balance == -1 || i->balance == 1) {
			break;
		}

		assert(i != n);
		i = zix_tree_rebalance(t, i, &height_change);
		if (i->balance == 0) {
			height_change = -1;
		}

		if (i->parent) {
			if (i == i->parent->left) {
				d_balance = height_change * -1;
			} else {
				assert(i == i->parent->right);
				d_balance = height_change;
			}
		}
	}

	DUMP(t);

#ifdef ZIX_TREE_VERIFY
	if (!verify(t, t->root)) {
		return ZIX_STATUS_ERROR;
	}
#endif

	return ZIX_STATUS_SUCCESS;
}

ZIX_API
int
zix_tree_find(const ZixTree* t, const void* e, ZixTreeIter* ti)
{
	ZixTreeNode* n = t->root;
	while (n) {
		const int cmp = t->cmp(e, n->data, t->cmp_data);
		if (cmp == 0) {
			break;
		} else if (cmp < 0) {
			n = n->left;
		} else {
			n = n->right;
		}
	}

	*ti = n;
	return (n) ? ZIX_STATUS_SUCCESS : ZIX_STATUS_NOT_FOUND;
}

ZIX_API
const void*
zix_tree_get_data(ZixTreeIter ti)
{
	return ti->data;
}

ZIX_API
ZixTreeIter
zix_tree_begin(ZixTree* t)
{
	if (!t->root) {
		return NULL;
	}

	ZixTreeNode* n = t->root;
	while (n->left) {
		n = n->left;
	}
	return n;
}

ZIX_API
ZixTreeIter
zix_tree_end(ZixTree* t)
{
	return NULL;
}

ZIX_API
bool
zix_tree_iter_is_end(ZixTreeIter i)
{
	return !i;
}

ZIX_API
ZixTreeIter
zix_tree_iter_next(ZixTreeIter i)
{
	if (!i) {
		return NULL;
	}

	if (i->right) {
		i = i->right;
		while (i->left) {
			i = i->left;
		}
	} else {
		while (i->parent && i->parent->right == i) {  // i is a right child
			i = i->parent;
		}

		i = i->parent;
	}

	return i;
}
