// Copyright 2011-2025 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include <zix/hash.h>

#include "qualifiers.h"

#include <zix/allocator.h>
#include <zix/attributes.h>
#include <zix/status.h>

#include <assert.h>
#include <stdbool.h>

typedef struct ZixHashEntry {
  ZixHashCode    code;   ///< Non-folded hash code
  ZixHashRecord* record; ///< Pointer to user-owned record
} ZixHashEntry;

struct ZixHashImpl {
  ZixAllocator*   allocator;  ///< User allocator
  ZixKeyFunc      key_func;   ///< User key accessor
  ZixHashFunc     hash_func;  ///< User hashing function
  ZixKeyEqualFunc equal_func; ///< User equality comparison function
  size_t          count;      ///< Number of records stored in the table
  size_t          mask;       ///< Bit mask for fast modulo (n_entries - 1)
  size_t          n_entries;  ///< Power of two table size
  ZixHashEntry*   entries;    ///< Pointer to dynamically allocated table
};

static ZIX_CONSTEXPR size_t min_n_entries    = 4U;
static ZIX_CONSTEXPR size_t min_load_divisor = 4U;
static ZIX_CONSTEXPR size_t tombstone        = 0xDEADU;

ZixHash*
zix_hash_new(ZixAllocator* const   allocator,
             const ZixKeyFunc      key_func,
             const ZixHashFunc     hash_func,
             const ZixKeyEqualFunc equal_func)
{
  assert(key_func);
  assert(hash_func);
  assert(equal_func);

  ZixHash* const hash = (ZixHash*)zix_malloc(allocator, sizeof(ZixHash));
  if (!hash) {
    return NULL;
  }

  hash->allocator  = allocator;
  hash->key_func   = key_func;
  hash->hash_func  = hash_func;
  hash->equal_func = equal_func;
  hash->count      = 0U;
  hash->n_entries  = min_n_entries;
  hash->mask       = hash->n_entries - 1U;

  hash->entries =
    (ZixHashEntry*)zix_calloc(allocator, hash->n_entries, sizeof(ZixHashEntry));

  if (!hash->entries) {
    zix_free(allocator, hash);
    return NULL;
  }

  return hash;
}

void
zix_hash_free(ZixHash* const hash)
{
  if (hash) {
    zix_free(hash->allocator, hash->entries);
    zix_free(hash->allocator, hash);
  }
}

ZIX_NONBLOCKING ZixHashIter
zix_hash_begin(const ZixHash* const hash)
{
  assert(hash);
  return hash->entries[0U].record ? 0U : zix_hash_next(hash, 0U);
}

ZIX_REALTIME ZixHashIter
zix_hash_end(const ZixHash* const hash)
{
  assert(hash);
  return hash->n_entries;
}

ZIX_REALTIME ZixHashCode
zix_hash_get_code(const ZixHash* hash, const ZixHashIter i)
{
  assert(hash);
  assert(i < hash->n_entries);

  return hash->entries[i].code;
}

ZIX_REALTIME ZixHashRecord*
zix_hash_get(const ZixHash* hash, const ZixHashIter i)
{
  assert(hash);
  assert(i < hash->n_entries);

  return hash->entries[i].record;
}

ZIX_NONBLOCKING ZixHashIter
zix_hash_next(const ZixHash* const hash, ZixHashIter i)
{
  assert(hash);
  do {
    ++i;
  } while (i < hash->n_entries && !hash->entries[i].record);

  return i;
}

ZIX_REALTIME size_t
zix_hash_size(const ZixHash* const hash)
{
  assert(hash);
  return hash->count;
}

static inline size_t
fold_code(const ZixHashCode code, const size_t mask)
{
  return code & mask;
}

static inline bool
is_empty(const ZixHashEntry* const entry)
{
  return !entry->record && !entry->code;
}

static inline bool
is_match(const ZixHash* const hash,
         const ZixHashCode    code,
         const size_t         entry_index,
         ZixKeyEqualFunc      predicate,
         const void* const    user_data)
{
  const ZixHashEntry* const entry = &hash->entries[entry_index];

  return entry->record && entry->code == code &&
         predicate(hash->key_func(entry->record), user_data);
}

static inline size_t
next_index(const ZixHash* const hash, const size_t i)
{
  return (i == hash->mask) ? 0U : (i + 1U);
}

static inline ZixHashIter
find_entry(const ZixHash* const    hash,
           const ZixHashKey* const key,
           const ZixHashCode       code)
{
  size_t i = fold_code(code, hash->mask);

  while (!is_empty(&hash->entries[i]) &&
         !is_match(hash, code, i, hash->equal_func, key)) {
    i = next_index(hash, i);
  }

  return i;
}

static ZixStatus
rehash(ZixHash* const hash,
       const size_t   old_n_entries,
       const size_t   new_n_entries)
{
  ZixHashEntry* const old_entries = hash->entries;

  // Allocate a new entries array
  ZixHashEntry* const new_entries = (ZixHashEntry*)zix_calloc(
    hash->allocator, new_n_entries, sizeof(ZixHashEntry));
  if (!new_entries) {
    return ZIX_STATUS_NO_MEM;
  }

  // Replace the array in the hash first so we can use find_entry() normally
  hash->mask      = new_n_entries - 1U;
  hash->n_entries = new_n_entries;
  hash->entries   = new_entries;

  // Reinsert every entry into the new array
  for (size_t i = 0U; i < old_n_entries; ++i) {
    const ZixHashEntry* const entry = &old_entries[i];
    if (entry->record) {
      const ZixHashKey* const key   = hash->key_func(entry->record);
      const size_t            index = find_entry(hash, key, entry->code);
      new_entries[index]            = *entry;
    }
  }

  // Free the old entries array
  zix_free(hash->allocator, old_entries);
  return ZIX_STATUS_SUCCESS;
}

ZixHashIter
zix_hash_find(const ZixHash* const hash, const ZixHashKey* const key)
{
  assert(hash);
  assert(key);

  const ZixHashIter i = find_entry(hash, key, hash->hash_func(key));
  return is_empty(&hash->entries[i]) ? hash->n_entries : i;
}

ZixHashRecord*
zix_hash_find_record(const ZixHash* const hash, const ZixHashKey* const key)
{
  assert(hash);
  assert(key);

  const ZixHashIter i = find_entry(hash, key, hash->hash_func(key));
  return hash->entries[i].record;
}

ZixHashInsertPlan
zix_hash_plan_insert_prehashed(const ZixHash* const  hash,
                               const ZixHashCode     code,
                               const ZixKeyMatchFunc predicate,
                               const void* const     user_data)
{
  assert(hash);
  assert(predicate);

  // Calculate an ideal initial position
  ZixHashInsertPlan pos = {code, fold_code(code, hash->mask)};

  // Search for a free position starting at the ideal one
  const size_t start_index     = pos.index;
  size_t       first_tombstone = 0;
  bool         found_tombstone = false;
  while (!is_empty(&hash->entries[pos.index])) {
    if (is_match(hash, code, pos.index, predicate, user_data)) {
      return pos;
    }

    if (!found_tombstone && !hash->entries[pos.index].record) {
      assert(hash->entries[pos.index].code == tombstone);
      first_tombstone = pos.index; // Remember the first/best free index
      found_tombstone = true;
    }

    pos.index = next_index(hash, pos.index);
    if (pos.index == start_index) {
      break; // Rare edge case: entire table is full of entries/tombstones
    }
  }

  // If we found a tombstone before an empty slot, place the new element there
  if (found_tombstone) {
    pos.index = first_tombstone;
  }

  assert(!hash->entries[pos.index].record);
  return pos;
}

ZixHashInsertPlan
zix_hash_plan_insert(const ZixHash* const hash, const ZixHashKey* const key)
{
  assert(hash);
  assert(key);

  return zix_hash_plan_insert_prehashed(
    hash, hash->hash_func(key), hash->equal_func, key);
}

ZIX_REALTIME ZixHashRecord*
zix_hash_record_at(const ZixHash* const hash, const ZixHashInsertPlan position)
{
  assert(hash);
  return hash->entries[position.index].record;
}

ZixStatus
zix_hash_insert_at(ZixHash* const          hash,
                   const ZixHashInsertPlan position,
                   ZixHashRecord* const    record)
{
  assert(hash);
  assert(record);

  if (hash->entries[position.index].record) {
    return ZIX_STATUS_EXISTS;
  }

  // Set entry to new value, but copy the old one for reverting
  ZixHashEntry* const entry     = &hash->entries[position.index];
  const ZixHashEntry  old_entry = *entry;
  assert(!entry->record);
  entry->code   = position.code;
  entry->record = record;

  // Double the array and rehash if overfull
  const size_t max_load  = (hash->n_entries / 2U) + (hash->n_entries / 8U);
  const size_t new_count = hash->count + 1U;
  if (new_count >= max_load) {
    const ZixStatus st = rehash(hash, hash->n_entries, hash->n_entries << 1U);
    if (st) {
      *entry = old_entry;
      return st;
    }
  }

  hash->count = new_count;
  return ZIX_STATUS_SUCCESS;
}

ZixStatus
zix_hash_insert(ZixHash* const hash, ZixHashRecord* const record)
{
  assert(hash);
  assert(record);

  const ZixHashKey* const key      = hash->key_func(record);
  const ZixHashInsertPlan position = zix_hash_plan_insert(hash, key);

  return zix_hash_insert_at(hash, position, record);
}

ZixStatus
zix_hash_erase(ZixHash* const        hash,
               const ZixHashIter     i,
               ZixHashRecord** const removed)
{
  assert(hash);
  assert(removed);

  // Replace entry with a tombstone
  *removed                = hash->entries[i].record;
  hash->entries[i].code   = tombstone;
  hash->entries[i].record = NULL;
  --hash->count;

  // Halve the array and rehash if underfull
  const size_t min_load = (hash->n_entries / min_load_divisor);
  if (hash->count < min_load && hash->n_entries > min_n_entries) {
    return rehash(hash, hash->n_entries, hash->n_entries >> 1U);
  }

  return ZIX_STATUS_SUCCESS;
}

ZixStatus
zix_hash_remove(ZixHash* const          hash,
                const ZixHashKey* const key,
                ZixHashRecord** const   removed)
{
  assert(hash);
  assert(key);
  assert(removed);

  const ZixHashIter i = zix_hash_find(hash, key);

  return i == hash->n_entries ? ZIX_STATUS_NOT_FOUND
                              : zix_hash_erase(hash, i, removed);
}
