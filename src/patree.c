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

#define _XOPEN_SOURCE 500

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zix/common.h"
#include "zix/patree.h"

typedef uint_fast8_t n_edges_t;

typedef struct _ZixPatreeNode {
	char*                  label_first;       /* Start of incoming label */
	char*                  label_last;        /* End of incoming label */
	char*                  str;               /* String stored at this node */
	struct _ZixPatreeNode* children;          /* Children of this node */
	n_edges_t              num_children;      /* Number of outgoing edges */
	char                   label_first_char;  /* First char of label_first */
} ZixPatreeNode;

struct _ZixPatree {
	ZixPatreeNode* root;  /* Root of the tree */
};

static void
zix_patree_print_rec(ZixPatreeNode* node, FILE* fd)
{
	if (node->label_first) {
		size_t edge_label_len = node->label_last - node->label_first + 1;
		char*  edge_label     = malloc(edge_label_len + 1);
		strncpy(edge_label, node->label_first, edge_label_len);
		fprintf(fd, "\t\"%p\" [label=<"
		        "<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\">"
		        "<TR><TD>%s</TD></TR>", (void*)node, edge_label);
		free(edge_label);
		if (node->str) {
			fprintf(fd, "<TR><TD>\"%s\"</TD></TR>", node->str);
		}
		fprintf(fd, "</TABLE>>,shape=\"plaintext\"] ;\n");
	} else {
		fprintf(fd, "\t\"%p\" [label=\"\"] ;\n", (void*)node);
	}


	for (n_edges_t i = 0; i < node->num_children; ++i) {
		ZixPatreeNode* const child = &node->children[i];
		zix_patree_print_rec(child, fd);
		fprintf(fd, "\t\"%p\" -> \"%p\" ;\n", (void*)node, (void*)child);
	}
}

ZIX_API
void
zix_patree_print_dot(const ZixPatree* t, FILE* fd)
{
	fprintf(fd, "digraph Patree { \n");
	zix_patree_print_rec(t->root, fd);
	fprintf(fd, "}\n");
}

ZIX_API
ZixPatree*
zix_patree_new(void)
{
	ZixPatree* t = (ZixPatree*)malloc(sizeof(ZixPatree));
	memset(t, '\0', sizeof(struct _ZixPatree));

	t->root = (ZixPatreeNode*)malloc(sizeof(ZixPatreeNode));
	memset(t->root, '\0', sizeof(ZixPatreeNode));

	return t;
}

static void
zix_patree_free_rec(ZixPatreeNode* n)
{
	if (n) {
		for (n_edges_t i = 0; i < n->num_children; ++i) {
			zix_patree_free_rec(&n->children[i]);
		}
		free(n->children);
	}
}

ZIX_API
void
zix_patree_free(ZixPatree* t)
{
	zix_patree_free_rec(t->root);
	free(t->root);
	free(t);
}

static inline bool
patree_is_leaf(const ZixPatreeNode* n)
{
	return n->num_children == 0;
}

static bool
patree_find_edge(const ZixPatreeNode* n, const char c, n_edges_t* const index)
{
	for (n_edges_t i = 0; i < n->num_children; ++i) {
		if (n->children[i].label_first_char == c) {
			*index = i;
			return true;
		}
	}
	return false;
}

static void
patree_add_edge(ZixPatreeNode* n,
                char*          str,
                char*          first,
                char*          last)
{
	/* Interesting performance note: building a sorted array here makes
	   the search considerably slower, regardless of whether binary search
	   or the existing search algorithm is used.  I suppose moving things
	   around blows the cache for child->children which trumps everything.
	*/
	assert(last >= first);
	n->children = realloc(n->children,
	                      (n->num_children + 1) * sizeof(ZixPatreeNode));
	n->children[n->num_children].label_first      = first;
	n->children[n->num_children].label_last       = last;
	n->children[n->num_children].str              = str;
	n->children[n->num_children].children         = NULL;
	n->children[n->num_children].num_children     = 0;
	n->children[n->num_children].label_first_char = first[0];
	++n->num_children;
}

/* Split an outgoing edge (to a child) at the given index */
static void
patree_split_edge(ZixPatreeNode* child,
                  size_t         index)
{
	ZixPatreeNode* const children     = child->children;
	const n_edges_t      num_children = child->num_children;

	char* const first = child->label_first + index;
	char* const last  = child->label_last;
	assert(last >= first);

	child->children                     = malloc(sizeof(ZixPatreeNode));
	child->children[0].children         = children;
	child->children[0].num_children     = num_children;
	child->children[0].str              = child->str;
	child->children[0].label_first      = first;
	child->children[0].label_last       = last;
	child->children[0].label_first_char = first[0];

	child->label_last = child->label_first + (index - 1);  // chop end

	child->num_children = 1;

	child->str = NULL;
}

static ZixStatus
patree_insert_internal(ZixPatreeNode* n, char* str, char* first, char* last)
{
	n_edges_t      child_i;
	assert(first <= last);

	if (patree_find_edge(n, *first, &child_i)) {
		ZixPatreeNode* child       = &n->children[child_i];
		char* const    label_first = child->label_first;
		char* const    label_last  = child->label_last;
		size_t         split_i     = 0;
		const size_t   label_len   = label_last - label_first + 1;
		const size_t   s_len       = last - first + 1;
		const size_t   max_len     = (s_len < label_len) ? s_len : label_len;

		while (split_i < max_len && first[split_i] == label_first[split_i]) {
			++split_i;
		}
		child = n->children + child_i;

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
zix_patree_insert(ZixPatree* t, const char* str)
{
	const size_t len = strlen(str);
	// FIXME: awful casts
	return patree_insert_internal(t->root, (char*)str, (char*)str, (char*)str + len - 1);
}

ZIX_API
ZixStatus
zix_patree_find(const ZixPatree* t, const char* const str, char** match)
{
	ZixPatreeNode* n = t->root;
	n_edges_t      child_i;

	register const char* p = str;

	*match = NULL;

	while (*p != '\0') {
		if (patree_find_edge(n, p[0], &child_i)) {
			assert(child_i <= n->num_children);
			ZixPatreeNode* const child = &n->children[child_i];

			/* Prefix compare search string and label */
			register const char*       l     = child->label_first;
			register const char* const l_end = child->label_last;
			while (l <= l_end) {
				if (*l++ != *p++) {
					return ZIX_STATUS_NOT_FOUND;
				}
			}

			if (!*p) {
				/* Reached end of search string */
				if (l == l_end + 1) {
					/* Reached end of label string as well, match */
					*match = child->str;
					return *match ? ZIX_STATUS_SUCCESS : ZIX_STATUS_NOT_FOUND;
				} else {
					/* Label string is longer, no match (prefix match) */
					return ZIX_STATUS_NOT_FOUND;
				}
			} else {
				/* Didn't reach end of search string */
				if (patree_is_leaf(n)) {
					/* Hit leaf early, no match */
					return ZIX_STATUS_NOT_FOUND;
				} else {
					/* Match at this node, continue search downwards (or match) */
					n  = child;
				}
			}
		} else {
			return ZIX_STATUS_NOT_FOUND;
		}
	}

	return ZIX_STATUS_NOT_FOUND;
}
