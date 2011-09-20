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

//#undef __SSE4_2__

#ifdef __SSE4_2__
#include <smmintrin.h>
//#warning Using SSE 4.2
#else
//#warning SSE 4.2 disabled
#endif

typedef uint_fast8_t n_edges_t;

struct _ZixPatreeNode;

typedef struct {
	struct _ZixPatreeNode* child;
	char                   first_char;
} ZixChildRef;

typedef struct _ZixPatreeNode {
	const char* label_first;   /* Start of incoming label */
	const char* label_last;    /* End of incoming label */
	const char* str;           /* String stored at this node */
	n_edges_t   num_children;  /* Number of outgoing edges */
	ZixChildRef children[];    /* Children of this node */
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
		ZixPatreeNode* const child = node->children[i].child;
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

static inline ZixPatreeNode*
realloc_node(ZixPatreeNode* n, int num_children)
{
	return (ZixPatreeNode*)realloc(n, sizeof(ZixPatreeNode)
	                               + num_children * sizeof(ZixChildRef));
}

ZIX_API
ZixPatree*
zix_patree_new(void)
{
	ZixPatree* t = (ZixPatree*)malloc(sizeof(ZixPatree));
	memset(t, '\0', sizeof(struct _ZixPatree));

	t->root = realloc_node(NULL, 0);
	memset(t->root, '\0', sizeof(ZixPatreeNode));

	return t;
}

static void
zix_patree_free_rec(ZixPatreeNode* n)
{
	if (n) {
		for (n_edges_t i = 0; i < n->num_children; ++i) {
			zix_patree_free_rec(n->children[i].child);
		}
		free(n);
	}
}

ZIX_API
void
zix_patree_free(ZixPatree* t)
{
	zix_patree_free_rec(t->root);
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
		if (n->children[i].first_char == c) {
			*index = i;
			return true;
		}
	}
	return false;
}

static void
patree_add_edge(ZixPatreeNode** n_ptr,
                const char*     str,
                const char*     first)
{
	ZixPatreeNode* n = *n_ptr;

	const char* last = first;
	for (; *last; ++last) ;
	--last;

	/* Interesting performance note: building a sorted array here makes
	   the search considerably slower, regardless of whether binary search
	   or the existing search algorithm is used.  I suppose moving things
	   around blows the cache for child->children which trumps everything.
	*/
	assert(last >= first);

	ZixPatreeNode* const child = realloc_node(NULL, 0);
	child->label_first       = first;
	child->label_last        = last;
	child->str               = str;
	child->num_children      = 0;

	++n->num_children;
	n = realloc_node(n, n->num_children);
	n->children[n->num_children - 1].first_char = first[0];
	n->children[n->num_children - 1].child      = child;

	*n_ptr = n;

	for (n_edges_t i = 0; i < n->num_children; ++i) {
		assert(n->children[i].first_char != '\0');
	}
	for (n_edges_t i = 0; i < child->num_children; ++i) {
		assert(child->children[i].first_char != '\0');
	}
}

/* Split an outgoing edge (to a child) at the given index */
static void
patree_split_edge(ZixPatreeNode** child_ptr,
                  size_t          index)
{
	ZixPatreeNode* child = *child_ptr;

	const size_t grandchild_size = sizeof(ZixPatreeNode)
		+ (child->num_children * sizeof(ZixChildRef));

	ZixPatreeNode* const grandchild = realloc_node(NULL, child->num_children);
	memcpy(grandchild, child, grandchild_size);
	grandchild->label_first = child->label_first + index;

	child = realloc_node(child, 1);
	child->children[0].first_char = grandchild->label_first[0];
	child->children[0].child = grandchild;
	child->label_last = child->label_first + (index - 1);  // chop end

	child->num_children = 1;

	child->str = NULL;

	*child_ptr = child;

	for (n_edges_t i = 0; i < child->num_children; ++i) {
		assert(child->children[i].first_char != '\0');
	}
	for (n_edges_t i = 0; i < grandchild->num_children; ++i) {
		assert(grandchild->children[i].first_char != '\0');
	}
}

static ZixStatus
patree_insert_internal(ZixPatreeNode** n_ptr, const char* str, const char* first)
{
	ZixPatreeNode* n = *n_ptr;
	n_edges_t      child_i;
	if (patree_find_edge(n, *first, &child_i)) {
		ZixPatreeNode**   child_ptr   = &n->children[child_i].child;
		ZixPatreeNode*    child       = *child_ptr;
		const char* const label_first = child->label_first;
		const char* const label_last  = child->label_last;
		size_t            split_i     = 0;
		const size_t      label_len   = label_last - label_first + 1;

		while (first[split_i] && split_i < label_len
		       && first[split_i] == label_first[split_i]) {
			++split_i;
		}

		if (first[split_i]) {
			if (split_i == label_len) {
				return patree_insert_internal(child_ptr, str, first + label_len);
			} else {
				patree_split_edge(child_ptr, split_i);
				return patree_insert_internal(child_ptr, str, first + split_i);
			}
		} else if (label_len != split_i) {
			patree_split_edge(child_ptr, split_i);
			if (first[split_i]) {
				patree_add_edge(child_ptr, str, first + split_i);
			} else {
				assert(!(*child_ptr)->str);
				(*child_ptr)->str = str;
			}
			return ZIX_STATUS_SUCCESS;
		} else if (label_first[split_i] && !child->str) {
			child->str = str;
		} else {
			return ZIX_STATUS_EXISTS;
		}
	} else {
		patree_add_edge(n_ptr, str, first);
	}

	return ZIX_STATUS_SUCCESS;
}

ZIX_API
ZixStatus
zix_patree_insert(ZixPatree* t, const char* str)
{
	return patree_insert_internal(&t->root, str, str);
}

static inline int
change_index_c(const char* a, const char* b, size_t len)
{
	for (size_t i = 0; i < len; ++i) {
		if (a[i] != b[i]) {
			return i;
		}
	}
	return len;
}

#ifdef __SSE4_2__
static inline int
change_index_sse(const char* a, const char* b, const size_t len)
{
	int ret;
	for (size_t i = 0; i < len; i += sizeof(__m128i)) {
		const __m128i  r    = _mm_loadu_si128((const __m128i*)(a + i));
		const __m128i* s    = (const __m128i*)(b + i);
		const int     index = _mm_cmpistri(
			r, *s, _SIDD_SBYTE_OPS|_SIDD_CMP_EQUAL_EACH|_SIDD_NEGATIVE_POLARITY);

		if (index != sizeof(__m128i)) {
			int ret = i + index;
			if (ret > len) {
				ret = len;
			}
			return ret;
		}
	}

	return len;
}
#endif

ZIX_API
ZixStatus
zix_patree_find(const ZixPatree* t, const char* const str, const char** match)
{
	ZixPatreeNode* n = t->root;
	n_edges_t      child_i;

	const char* p = str;

	while (patree_find_edge(n, p[0], &child_i)) {
		assert(child_i <= n->num_children);
		ZixPatreeNode* const child     = n->children[child_i].child;
		const char* const    child_str = child->str;

		/* Prefix compare search string and label */
		const char*       l     = child->label_first;
		const char* const l_end = child->label_last;
		const size_t      len   = l_end - l + 1;
#ifdef __SSE4_2__
		int change_index;
		if (len >= sizeof(__m128i)) {
			change_index = change_index_sse(p, l, len);
		} else {
			change_index = change_index_c(p, l, len);
		}
#else
		int change_index = change_index_c(p, l, len);
#endif
		
		l += change_index;
		p += change_index;

#if 0
		while (l <= l_end) {
			if (*l++ != *p++) {
				return ZIX_STATUS_NOT_FOUND;
			}
		}
#endif

		if (!*p) {
			/* Reached end of search string */
			if (l == l_end + 1) {
				/* Reached end of label string as well, match */
				*match = child_str;
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
	}

	return ZIX_STATUS_NOT_FOUND;
}
