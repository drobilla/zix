/*
  Copyright 2011 David Robillard <http: //drobilla.net>

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

#define _XOPEN_SOURCE 500

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zix/common.h"
#include "zix/fat_patree.h"

#define N_CHILDREN 256

typedef uint_fast8_t n_edges_t;

typedef struct _ZixFatPatreeNode {
	char*                     label_first;           /* Start of incoming label */
	char*                     label_last;            /* End of incoming label */
	char*                     str;                   /* String stored at this node */
	struct _ZixFatPatreeNode* children[N_CHILDREN];  /* Children of this node */
} ZixFatPatreeNode;

struct _ZixFatPatree {
	ZixFatPatreeNode* root;  /* Root of the tree */
};

ZIX_API
ZixFatPatree*
zix_fat_patree_new(void)
{
	ZixFatPatree* t = (ZixFatPatree*)malloc(sizeof(ZixFatPatree));
	memset(t, '\0', sizeof(struct _ZixFatPatree));

	t->root = (ZixFatPatreeNode*)malloc(sizeof(ZixFatPatreeNode));
	memset(t->root, '\0', sizeof(ZixFatPatreeNode));

	return t;
}

static void
zix_fat_patree_free_rec(ZixFatPatreeNode* n)
{
	if (n) {
		for (int i = 0; i < N_CHILDREN; ++i) {
			zix_fat_patree_free_rec(n->children[i]);
		}
	}
}

ZIX_API
void
zix_fat_patree_free(ZixFatPatree* t)
{
	zix_fat_patree_free_rec(t->root);
	free(t->root);
	free(t);
}

static inline bool
patree_find_edge(ZixFatPatreeNode* n, uint8_t c, n_edges_t* index)
{
	if (n->children[c]) {
		*index = c;
		return true;
	}
	return false;
}

static void
patree_add_edge(ZixFatPatreeNode* n,
                char*             str,
                char*             first,
                char*             last)
{
	assert(last >= first);
	const int index = (uint8_t)first[0];
	assert(!n->children[index]);

	ZixFatPatreeNode* new_node = (ZixFatPatreeNode*)malloc(
		sizeof(ZixFatPatreeNode));
	n->children[index] = new_node;
	n->children[index]->label_first = first;
	n->children[index]->label_last  = last;
	n->children[index]->str         = str;
	for (int i = 0; i < N_CHILDREN; ++i) {
		n->children[index]->children[i] = NULL;
	}
}

/* Split an outgoing edge (to a child) at the given index */
static void
patree_split_edge(ZixFatPatreeNode* child,
                  size_t         index)
{
	char* const first = child->label_first + index;
	char* const last  = child->label_last;
	assert(last >= first);

	ZixFatPatreeNode* new_node = (ZixFatPatreeNode*)malloc(
		sizeof(ZixFatPatreeNode));
	new_node->label_first  = first;
	new_node->label_last   = last;
	new_node->str          = child->str;
	memcpy(new_node->children, child->children,
	       sizeof(ZixFatPatreeNode*) * N_CHILDREN);

	child->label_last = child->label_first + (index - 1);  // chop end
	child->str = NULL;

	for (int i = 0; i < N_CHILDREN; ++i) {
		child->children[i] = NULL;
	}
	const int new_node_index = (int)first[0];
	assert(new_node_index < N_CHILDREN);
	assert(!child->children[new_node_index]);
	child->children[new_node_index] = new_node;
}

static ZixStatus
patree_insert_internal(ZixFatPatreeNode* n, char* str, char* first, char* last)
{
	n_edges_t child_i;
	assert(first <= last);

	if (patree_find_edge(n, *first, &child_i)) {
		ZixFatPatreeNode* child       = n->children[child_i];
		char* const    label_first = child->label_first;
		char* const    label_last  = child->label_last;
		size_t         split_i     = 0;
		const size_t   label_len   = label_last - label_first + 1;
		const size_t   s_len       = last - first + 1;
		const size_t   max_len     = (s_len < label_len) ? s_len : label_len;

		while (split_i < max_len && first[split_i] == label_first[split_i]) {
			++split_i;
		}
		child = n->children[child_i];

		if (label_len < s_len) {
			if (split_i == label_len) {
				return patree_insert_internal(child, str, first + label_len, last);
			} else {
				patree_split_edge(child, split_i);
				return patree_insert_internal(child, str, first + split_i, last);
			}
		} else if (label_len != split_i) {
			patree_split_edge(child, split_i);
			if (split_i != s_len) {
				patree_add_edge(child, str, first + split_i, last);
			} else {
				assert(!child->str);
				child->str = str;
			}
			return ZIX_STATUS_SUCCESS;
		} else if (label_len == s_len && !child->str) {
			child->str = str;
		} else {
			return ZIX_STATUS_EXISTS;
		}

	} else {
		patree_add_edge(n, str, first, last);
	}

	return ZIX_STATUS_SUCCESS;
}

ZIX_API
ZixStatus
zix_fat_patree_insert(ZixFatPatree* t, const char* str)
{
	const size_t len = strlen(str);
	// FIXME: awful casts
	return patree_insert_internal(t->root, (char*)str, (char*)str, (char*)str + len - 1);
}

ZIX_API
ZixStatus
zix_fat_patree_find(ZixFatPatree* t, const char* p, char** match)
{
	ZixFatPatreeNode* n = t->root;
	n_edges_t         child_i;

	*match = NULL;

	while (*p != '\0') {
		if (patree_find_edge(n, p[0], &child_i)) {
			ZixFatPatreeNode* const child       = n->children[child_i];
			const char* const    label_first = child->label_first;
			const char* const    label_last  = child->label_last;

			/* Prefix compare search string and label */
			const char* l = label_first;
			while (*p != '\0' && l <= label_last) {
				if (*l++ != *p++) {
					return ZIX_STATUS_NOT_FOUND;
				}
			}

			if (!*p) {
				/* Reached end of search string */
				if (l == label_last + 1) {
					/* Reached end of label string as well, match */
					*match = child->str;
					return *match ? ZIX_STATUS_SUCCESS : ZIX_STATUS_NOT_FOUND;
				} else {
					/* Label string is longer, no match (prefix match) */
					return ZIX_STATUS_NOT_FOUND;
				}
			} else {
				/* Match at this node, continue search downwards.
				   Possibly we have prematurely hit a leaf, so the next
				   edge search will fail.
				*/
				n  = child;
			}
		} else {
			return ZIX_STATUS_NOT_FOUND;
		}
	}

	return ZIX_STATUS_NOT_FOUND;
}
