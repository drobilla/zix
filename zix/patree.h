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

#ifndef ZIX_PATREE_H
#define ZIX_PATREE_H

#include "zix/common.h"

/**
   @addtogroup zix
   @{
   @name Patree
   @{
*/

typedef struct _ZixPatree ZixPatree;

/**
   Construct a new Patree.
*/
ZIX_API
ZixPatree*
zix_patree_new(void);

/**
   Destroy @a t.
*/
ZIX_API
void
zix_patree_free(ZixPatree* t);

/**
   Print a DOT description of @a t to @a fd.
*/
ZIX_API
void
zix_patree_print_dot(const ZixPatree* t, FILE* fd);

/**
   Insert @a str into @a t.
*/
ZIX_API
ZixStatus
zix_patree_insert(ZixPatree* t, const char* str);

/**
   Search for @a str in @a t.
*/
ZIX_API
ZixStatus
zix_patree_find(const ZixPatree* t, const char* str, char** match);

/**
   @}
   @}
*/

#endif  /* ZIX_PATREE_H */
