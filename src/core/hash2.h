#ifndef _FU_HASH_H_
#define _FU_HASH_H_

#include <stdint.h>
#include <stdio.h>

#include "misc.h"

typedef struct _FUHashTableIter {
    void* dummy[3];
} FUHashTableIter;

typedef struct _FUHashTable FUHashTable;
// uint64_t fu_hash_table_get_type();
// // FU_DECLARE_TYPE(FUHashTable, fu_hash_table)
// #define FU_TYPE_HASH_TABLE (fu_hash_table_get_type())

FUHashTable* fu_hash_table_new(FUHashFunc fnHash, FUNotify fnFree);
FUHashTable* fu_hash_table_ref(FUHashTable* table);
void fu_hash_table_unref(FUHashTable* table);

void fu_hash_table_iter_init(FUHashTableIter* iter, FUHashTable* table);
bool fu_hash_table_iter_next(FUHashTableIter* iter, void** value);

bool fu_hash_table_insert(FUHashTable* table, const void* key, void* value);
void* fu_hash_table_lookup(FUHashTable* table, const void* key);
bool fu_hash_table_remove(FUHashTable* table, const void* key);

uint64_t fu_str_bkdr_hash(const char* str);
uint64_t fu_str_djb_hash(const char* str);
uint64_t fu_str_sdbm_hash(const char* str);
uint64_t fu_str_fnv_hash(const char* str);
uint64_t fu_hash_sip64(const void* data, const size_t len);
uint64_t fu_hash_mm3(const void* data, const uint64_t len);
uint64_t fu_hash_xxh3(const void* data, size_t len);

#endif // HASHMAP_H
