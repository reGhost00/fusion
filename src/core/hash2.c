/**
 * @file hash.c
 * @author your name (you@domain.com)
 * @brief 哈希表实现
 * 此为哈希表链式实现
 * 设 pow 为整数
 * 哈希表数组长 2^pow
 * 数据插入时，数据装入 "桶" 中，同时保存计算的哈希值
 * “桶” 是双链表，放置在下标为哈希值与 2^pow 的模的位置，桶升序排列
 * 哈希表最大容量为 2^30
 * @version 0.1
 * @date 2024-10-03
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <stdatomic.h>
#include <string.h>

#include "hash2.h"
#include "utils.h"

#define DEF_MIN_MOD 4
#define DEF_MAX_MOD 30
static const uint64_t defPows[] = {
    1UL << 4, 1UL << 4, 1UL << 4, 1UL << 4, 1UL << 4,
    1UL << 5,
    1UL << 6,
    1UL << 7,
    1UL << 8,
    1UL << 9,
    1UL << 10,
    1UL << 11,
    1UL << 12,
    1UL << 13,
    1UL << 14,
    1UL << 15,
    1UL << 16,
    1UL << 17,
    1UL << 18,
    1UL << 19,
    1UL << 20,
    1UL << 21,
    1UL << 22,
    1UL << 23,
    1UL << 24,
    1UL << 25,
    1UL << 26,
    1UL << 27,
    1UL << 28,
    1UL << 29,
    1UL << 30
};

static const uint64_t defGrowThreshold[] = {
    (1UL << 4) * 0.6, (1UL << 4) * 0.6, (1UL << 4) * 0.6, (1UL << 4) * 0.6, (1UL << 4) * 0.6,
    (1UL << 5) * 0.6,
    (1UL << 6) * 0.7,
    (1UL << 7) * 0.7,
    (1UL << 8) * 0.8,
    (1UL << 9) * 0.8,
    (1UL << 10) * 0.8,
    (1UL << 11) * 0.8,
    (1UL << 12) * 0.8,
    (1UL << 13) * 0.8,
    (1UL << 14) * 0.8,
    (1UL << 15) * 0.8,
    (1UL << 16) * 0.8,
    (1UL << 17) * 0.8,
    (1UL << 18) * 0.8,
    (1UL << 19) * 0.8,
    (1UL << 20) * 0.8,
    (1UL << 21) * 0.8,
    (1UL << 22) * 0.8,
    (1UL << 23) * 0.8,
    (1UL << 24) * 0.8,
    (1UL << 25) * 0.8,
    (1UL << 26) * 0.8,
    (1UL << 27) * 0.8,
    (1UL << 28) * 0.8,
    (1UL << 29) * 0.8,
    1UL << 30
};

typedef struct _TBucket TBucket;
struct _TBucket {
    FUHashTable* table;
    TBucket* prev;
    TBucket* next;
    // uint64_t idx;
    /** 哈希值 */
    uint64_t hash;
    void* data;
};

struct _FUHashTable {
    /** 哈希桶数组，长度为 2^pow */
    TBucket** buckets;
    /** 哈希模数 */
    uint64_t mod;
    /** 桶总数 */
    uint64_t count;
    FUHashFunc fnHash;

    FUNotify fnFree;
    atomic_int ref;
};

typedef struct _TIter {
    FUHashTable* table;
    TBucket* bucket;
    uint64_t idx;
} TIter;

/**
 * @brief 创建新的哈希表
 * 可以指定用户数据释放函数，释放时调用
 * 哈希表初始用于引用计数 1
 * @param fnHash 哈希函数
 * @param fnFree 用户数据释放函数
 * @return FUHashTable*
 */
FUHashTable* fu_hash_table_new(FUHashFunc fnHash, FUNotify fnFree)
{
    fu_return_val_if_fail(fnHash, NULL);
    FUHashTable* table = (FUHashTable*)fu_malloc(sizeof(FUHashTable));
    table->buckets = (TBucket**)fu_malloc0(sizeof(TBucket*) * defPows[DEF_MIN_MOD]);
    table->mod = DEF_MIN_MOD;
    table->count = 0;
    table->fnHash = fnHash;
    table->fnFree = fnFree;

    atomic_init(&table->ref, 1);
    return table;
}

/**
 * @brief 引用计数原子 +1
 *
 * @param table
 * @return FUHashTable*
 */
FUHashTable* fu_hash_table_ref(FUHashTable* table)
{
    fu_return_val_if_fail(table, NULL);
    atomic_fetch_add(&table->ref, 1);
    return table;
}

/**
 * @brief 引用计数原子 -1
 * 引用计数归零时，释放哈希表
 * @param table
 */
void fu_hash_table_unref(FUHashTable* table)
{
    fu_return_if_fail(table);
    if (1 < atomic_fetch_sub(&table->ref, 1))
        return;
    if (table->count) {
        FUNotify fnFree = !table->fnFree ? fu_void1 : table->fnFree;
        TBucket* bucket = NULL;
        for (size_t i = 0; i < defPows[table->mod]; i++) {
            bucket = table->buckets[i];
            while (bucket) {
                table->buckets[i] = bucket->next;
                fnFree(bucket->data);
                fu_free(bucket);
                bucket = table->buckets[i];
            }
        }
    }
    fu_free(table->buckets);
    fu_free(table);
}

/**
 * @brief 初始化迭代器
 * 迭代器用于迭代哈希表
 * @param iter
 * @param table
 */
void fu_hash_table_iter_init(FUHashTableIter* iter, FUHashTable* table)
{
    fu_return_if_fail(iter && table);
    memset(iter, 0, sizeof(TIter));
    ((TIter*)iter)->table = table;
}

bool fu_hash_table_iter_next(FUHashTableIter* iter, void** value)
{
    TIter* it = (TIter*)iter;
    fu_return_val_if_fail(iter && it->table, false);
    if (it->bucket && it->bucket->next) {
        it->bucket = it->bucket->next;
        *value = it->bucket->data;
        return true;
    }

    while (defPows[it->table->mod] > it->idx) {
        if (!(it->bucket = it->table->buckets[it->idx++]))
            continue;
        *value = it->bucket->data;
        return true;
    }

    return false;
}
/**
 * @brief 判断哈希表是否需要扩容
 *
 * @param table
 * @return true
 * @return false
 */
static bool fu_hash_table_maybe_extend(FUHashTable* table)
{
    if (defGrowThreshold[table->mod] - 1 < table->count) {
        if (FU_UNLIKELY(DEF_MAX_MOD - 1 < table->mod)) {
            fprintf(stderr, "Hash table overflow\n");
            return false;
        }

        TBucket** buckets = fu_malloc0(sizeof(TBucket*) * defPows[table->mod + 1]);
        TBucket** tmp = fu_malloc(sizeof(TBucket*) * table->count);
        uint64_t cnt = 0, idx = 0;
        for (; idx < defPows[table->mod]; idx++) {
            while (table->buckets[idx]) {
                tmp[cnt++] = table->buckets[idx];
                table->buckets[idx] = table->buckets[idx]->next;
            }
            if (cnt >= table->count)
                break;
        }
        table->mod += 1;
        for (cnt = 0; cnt < table->count; cnt++) {
            idx = tmp[cnt]->hash & (defPows[table->mod] - 1);
            if (!buckets[idx]) {
                buckets[idx] = tmp[cnt];
                buckets[idx]->prev = buckets[idx]->next = NULL;
                continue;
            }
            tmp[cnt]->prev = NULL;
            tmp[cnt]->next = buckets[idx];
            buckets[idx]->prev = tmp[cnt];
            buckets[idx] = tmp[cnt];
        }
        fu_free(tmp);
        fu_free(table->buckets);
        table->buckets = buckets;
    }
    table->count += 1;
    return true;
}

bool fu_hash_table_insert(FUHashTable* table, const void* key, void* value)
{
    fu_return_val_if_fail(table, false);
    if (FU_UNLIKELY(defPows[table->mod] < table->count + 1)) {
        fprintf(stderr, "Hash table overflow\n");
        return false;
    }
    if (FU_UNLIKELY(!fu_hash_table_maybe_extend(table)))
        return false;
    //
    // 1 创建包含数据的新桶
    // 2 判断当前是否存在相同哈希值的桶，如果存在，释放旧桶
    // 3 将新桶插入到哈希表中
    const uint64_t hash = table->fnHash(key);
    const uint64_t idx = hash & (defPows[table->mod] - 1);

    TBucket* bucket = table->buckets[idx];
    while (bucket) {
        if (hash != bucket->hash) {
            bucket = bucket->next;
            continue;
        }
        if (table->fnFree)
            table->fnFree(bucket->data);
        bucket->data = value;
        return true;
    }

    bucket = fu_malloc0(sizeof(TBucket));
    bucket->table = table;
    bucket->data = value;
    bucket->hash = hash;
    if (!table->buckets[idx]) {
        table->buckets[idx] = bucket;
        return true;
    }
    bucket->next = table->buckets[idx];
    table->buckets[idx]->prev = bucket;
    table->buckets[idx] = bucket;
    return true;
}

void* fu_hash_table_lookup(FUHashTable* table, const void* key)
{
    fu_return_val_if_fail(table, NULL);
    if (FU_UNLIKELY(!key))
        return NULL;
    const uint64_t hash = table->fnHash(key);
    const uint64_t idx = hash & (defPows[table->mod] - 1);
    TBucket* bucket = table->buckets[idx];
    while (bucket) {
        if (hash != bucket->hash) {
            bucket = bucket->next;
            continue;
        }
        // // LRU
        if (bucket != table->buckets[idx]) {
            if (bucket->next)
                bucket->next->prev = bucket->prev;
            if (bucket->prev)
                bucket->prev->next = bucket->next;
            bucket->next = table->buckets[idx];
            table->buckets[idx]->prev = bucket;
            table->buckets[idx] = bucket;
            bucket->prev = NULL;
        }
        return bucket->data;
    }
    return NULL;
}

bool fu_hash_table_remove(FUHashTable* table, const void* key)
{
    fu_return_val_if_fail(table, false);
    if (FU_UNLIKELY(!key))
        return false;
    const uint64_t hash = table->fnHash(key);
    const uint64_t idx = hash & (defPows[table->mod] - 1);
    TBucket* bucket = table->buckets[idx];
    TBucket* prev = NULL;
    while (bucket) {
        if (hash != bucket->hash) {
            bucket = bucket->next;
            continue;
        }
        if (table->fnFree)
            table->fnFree(bucket->data);
        if (bucket->next)
            bucket->next->prev = prev;
        if (bucket->prev)
            bucket->prev->next = bucket->next;
        else
            table->buckets[idx] = bucket->next;
        table->count -= 1;
        fu_free(bucket);
        return true;
    }
    return false;
}

/**
 * g_str_hash:
 * @v: (not nullable): a string key
 *
 * Converts a string to a hash value.
 *
 * This function implements the widely used "djb" hash apparently
 * posted by Daniel Bernstein to comp.lang.c some time ago.  The 32
 * bit unsigned hash value starts at 5381 and for each byte 'c' in
 * the string, is updated: `hash = hash * 33 + c`. This function
 * uses the signed value of each byte.
 *
 * It can be passed to g_hash_table_new() as the @fnHash parameter,
 * when using non-%NULL strings as keys in a #GHashTable.
 *
 * Note that this function may not be a perfect fit for all use cases.
 * For example, it produces some hash collisions with strings as short
 * as 2.
 *
 * Returns: a hash value corresponding to the key
 */

uint64_t fu_str_djb_hash(const char* str)
{
    if (FU_UNLIKELY(!str))
        return 0;
    uint64_t hash = 5381;
    char ch;

    while ((ch = *str++))
        hash = (hash << 5) + hash + ch;

    return hash;
}

uint64_t fu_str_bkdr_hash(const char* str)
{
    if (FU_UNLIKELY(!str))
        return 0;
    uint64_t hash = 0;
    char ch;

    while ((ch = *str++))
        hash = hash * 1313 + ch;

    return hash;
}

uint64_t fu_str_sdbm_hash(const char* str)
{
    if (FU_UNLIKELY(!str))
        return 0;
    uint64_t hash = 0;
    char ch;

    while ((ch = *str++))
        hash = ch + (hash << 6) + (hash << 16) - hash;

    return hash;
}

uint64_t fu_str_fnv_hash(const char* str)
{
    if (FU_UNLIKELY(!str))
        return 0;
    const uint64_t fnv_prime = 0x01000193;
    uint64_t hash = 0x811c9dc5;
    int c;

    while ((c = *str++))
        hash = hash ^ c * fnv_prime;

    return hash;
}

//-----------------------------------------------------------------------------
// SipHash reference C implementation
//
// Copyright (c) 2012-2016 Jean-Philippe Aumasson
// <jeanphilippe.aumasson@gmail.com>
// Copyright (c) 2012-2014 Daniel J. Bernstein <djb@cr.yp.to>
//
// To the extent possible under law, the author(s) have dedicated all copyright
// and related and neighboring rights to this software to the public domain
// worldwide. This software is distributed without any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication along
// with this software. If not, see
// <http://creativecommons.org/publicdomain/zero/1.0/>.
//
// default: SipHash-2-4
//-----------------------------------------------------------------------------
uint64_t fu_hash_sip64(const void* data, const size_t len)
{
#define U8TO64_LE(p)                                                                                                                                                                                                                     \
    {                                                                                                                                                                                                                                    \
        (((uint64_t)((p)[0])) | ((uint64_t)((p)[1]) << 8) | ((uint64_t)((p)[2]) << 16) | ((uint64_t)((p)[3]) << 24) | ((uint64_t)((p)[4]) << 32) | ((uint64_t)((p)[5]) << 40) | ((uint64_t)((p)[6]) << 48) | ((uint64_t)((p)[7]) << 56)) \
    }
#define U64TO8_LE(p, v)                            \
    {                                              \
        U32TO8_LE((p), (uint64_t)((v)));           \
        U32TO8_LE((p) + 4, (uint64_t)((v) >> 32)); \
    }
#define U32TO8_LE(p, v)                \
    {                                  \
        (p)[0] = (uint8_t)((v));       \
        (p)[1] = (uint8_t)((v) >> 8);  \
        (p)[2] = (uint8_t)((v) >> 16); \
        (p)[3] = (uint8_t)((v) >> 24); \
    }
#define ROTL(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))
#define SIPROUND           \
    {                      \
        v0 += v1;          \
        v1 = ROTL(v1, 13); \
        v1 ^= v0;          \
        v0 = ROTL(v0, 32); \
        v2 += v3;          \
        v3 = ROTL(v3, 16); \
        v3 ^= v2;          \
        v0 += v3;          \
        v3 = ROTL(v3, 21); \
        v3 ^= v0;          \
        v2 += v1;          \
        v1 = ROTL(v1, 17); \
        v1 ^= v2;          \
        v2 = ROTL(v2, 32); \
    }
    const uint8_t* in = (uint8_t*)data;
    const uint64_t s0 = 31313ULL;
    const uint64_t s1 = 1313131ULL;
    uint64_t k0 = U8TO64_LE((uint8_t*)&s0);
    uint64_t k1 = U8TO64_LE((uint8_t*)&s1);
    uint64_t v3 = UINT64_C(0x7465646279746573) ^ k1;
    uint64_t v2 = UINT64_C(0x6c7967656e657261) ^ k0;
    uint64_t v1 = UINT64_C(0x646f72616e646f6d) ^ k1;
    uint64_t v0 = UINT64_C(0x736f6d6570736575) ^ k0;
    const uint8_t* end = in + len - (len % sizeof(uint64_t));
    for (; in != end; in += 8) {
        uint64_t m = U8TO64_LE(in);
        v3 ^= m;
        SIPROUND;
        SIPROUND;
        v0 ^= m;
    }
    const int left = len & 7;
    uint64_t b = ((uint64_t)len) << 56;
    switch (left) {
    case 7:
        b |= ((uint64_t)in[6]) << 48; /* fall through */
    case 6:
        b |= ((uint64_t)in[5]) << 40; /* fall through */
    case 5:
        b |= ((uint64_t)in[4]) << 32; /* fall through */
    case 4:
        b |= ((uint64_t)in[3]) << 24; /* fall through */
    case 3:
        b |= ((uint64_t)in[2]) << 16; /* fall through */
    case 2:
        b |= ((uint64_t)in[1]) << 8; /* fall through */
    case 1:
        b |= ((uint64_t)in[0]);
        break;
    case 0:
        break;
    }
    v3 ^= b;
    SIPROUND;
    SIPROUND;
    v0 ^= b;
    v2 ^= 0xff;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    b = v0 ^ v1 ^ v2 ^ v3;
    uint64_t out = 0;
    U64TO8_LE((uint8_t*)&out, b);
    return out;
}

//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.
//
// Murmur3_86_128
//-----------------------------------------------------------------------------
uint64_t fu_hash_mm3(const void* data, const uint64_t len)
{
#define ROTL32(x, r) ((x << r) | (x >> (32 - r)))
#define FMIX32(h)    \
    h ^= h >> 16;    \
    h *= 0x85ebca6b; \
    h ^= h >> 13;    \
    h *= 0xc2b2ae35; \
    h ^= h >> 16;
    const uint32_t seed = 31313UL;
    const uint8_t* _dt = (const uint8_t*)data;
    const int64_t nblocks = (int64_t)(len / 16LL);
    uint32_t h1 = seed;
    uint32_t h2 = seed;
    uint32_t h3 = seed;
    uint32_t h4 = seed;
    uint32_t c1 = 0x239b961b;
    uint32_t c2 = 0xab0e9789;
    uint32_t c3 = 0x38b34ae5;
    uint32_t c4 = 0xa1e38b93;
    const uint32_t* blocks = (const uint32_t*)(_dt + nblocks * 16);
    for (int64_t i = -nblocks; i; i++) {
        uint32_t k1 = blocks[i * 4 + 0];
        uint32_t k2 = blocks[i * 4 + 1];
        uint32_t k3 = blocks[i * 4 + 2];
        uint32_t k4 = blocks[i * 4 + 3];
        k1 *= c1;
        k1 = ROTL32(k1, 15);
        k1 *= c2;
        h1 ^= k1;
        h1 = ROTL32(h1, 19);
        h1 += h2;
        h1 = h1 * 5 + 0x561ccd1b;
        k2 *= c2;
        k2 = ROTL32(k2, 16);
        k2 *= c3;
        h2 ^= k2;
        h2 = ROTL32(h2, 17);
        h2 += h3;
        h2 = h2 * 5 + 0x0bcaa747;
        k3 *= c3;
        k3 = ROTL32(k3, 17);
        k3 *= c4;
        h3 ^= k3;
        h3 = ROTL32(h3, 15);
        h3 += h4;
        h3 = h3 * 5 + 0x96cd1c35;
        k4 *= c4;
        k4 = ROTL32(k4, 18);
        k4 *= c1;
        h4 ^= k4;
        h4 = ROTL32(h4, 13);
        h4 += h1;
        h4 = h4 * 5 + 0x32ac3b17;
    }
    const uint8_t* tail = (const uint8_t*)(_dt + nblocks * 16);
    uint32_t k1 = 0;
    uint32_t k2 = 0;
    uint32_t k3 = 0;
    uint32_t k4 = 0;
    switch (len & 15) {
    case 15:
        k4 ^= tail[14] << 16; /* fall through */
    case 14:
        k4 ^= tail[13] << 8; /* fall through */
    case 13:
        k4 ^= tail[12] << 0;
        k4 *= c4;
        k4 = ROTL32(k4, 18);
        k4 *= c1;
        h4 ^= k4;
        /* fall through */
    case 12:
        k3 ^= tail[11] << 24; /* fall through */
    case 11:
        k3 ^= tail[10] << 16; /* fall through */
    case 10:
        k3 ^= tail[9] << 8; /* fall through */
    case 9:
        k3 ^= tail[8] << 0;
        k3 *= c3;
        k3 = ROTL32(k3, 17);
        k3 *= c4;
        h3 ^= k3;
        /* fall through */
    case 8:
        k2 ^= tail[7] << 24; /* fall through */
    case 7:
        k2 ^= tail[6] << 16; /* fall through */
    case 6:
        k2 ^= tail[5] << 8; /* fall through */
    case 5:
        k2 ^= tail[4] << 0;
        k2 *= c2;
        k2 = ROTL32(k2, 16);
        k2 *= c3;
        h2 ^= k2;
        /* fall through */
    case 4:
        k1 ^= tail[3] << 24; /* fall through */
    case 3:
        k1 ^= tail[2] << 16; /* fall through */
    case 2:
        k1 ^= tail[1] << 8; /* fall through */
    case 1:
        k1 ^= tail[0] << 0;
        k1 *= c1;
        k1 = ROTL32(k1, 15);
        k1 *= c2;
        h1 ^= k1;
        /* fall through */
    };
    h1 ^= len;
    h2 ^= len;
    h3 ^= len;
    h4 ^= len;
    h1 += h2;
    h1 += h3;
    h1 += h4;
    h2 += h1;
    h3 += h1;
    h4 += h1;
    FMIX32(h1);
    FMIX32(h2);
    FMIX32(h3);
    FMIX32(h4);
    h1 += h2;
    h1 += h3;
    h1 += h4;
    h2 += h1;
    h3 += h1;
    h4 += h1;
    return (((uint64_t)h2) << 32) | h1;
}

//-----------------------------------------------------------------------------
// xxHash Library
// Copyright (c) 2012-2021 Yann Collet
// All rights reserved.
//
// BSD 2-Clause License (https://www.opensource.org/licenses/bsd-license.php)
//
// xxHash3
//-----------------------------------------------------------------------------
#define XXH_PRIME_1 11400714785074694791ULL
#define XXH_PRIME_2 14029467366897019727ULL
#define XXH_PRIME_3 1609587929392839161ULL
#define XXH_PRIME_4 9650029242287828579ULL
#define XXH_PRIME_5 2870177450012600261ULL

static uint64_t XXH_read64(const void* memptr)
{
    uint64_t val;
    memcpy(&val, memptr, sizeof(val));
    return val;
}

static uint64_t XXH_read32(const void* memptr)
{
    uint64_t val;
    memcpy(&val, memptr, sizeof(val));
    return val;
}

static uint64_t XXH_rotl64(uint64_t x, int r)
{
    return (x << r) | (x >> (64 - r));
}

uint64_t fu_hash_xxh3(const void* data, size_t len)
{
    const uint64_t seed = 31;
    const uint8_t* p = (const uint8_t*)data;
    const uint8_t* const end = p + len;
    uint64_t h64;

    if (len >= 32) {
        const uint8_t* const limit = end - 32;
        uint64_t v1 = seed + XXH_PRIME_1 + XXH_PRIME_2;
        uint64_t v2 = seed + XXH_PRIME_2;
        uint64_t v3 = seed + 0;
        uint64_t v4 = seed - XXH_PRIME_1;

        do {
            v1 += XXH_read64(p) * XXH_PRIME_2;
            v1 = XXH_rotl64(v1, 31);
            v1 *= XXH_PRIME_1;

            v2 += XXH_read64(p + 8) * XXH_PRIME_2;
            v2 = XXH_rotl64(v2, 31);
            v2 *= XXH_PRIME_1;

            v3 += XXH_read64(p + 16) * XXH_PRIME_2;
            v3 = XXH_rotl64(v3, 31);
            v3 *= XXH_PRIME_1;

            v4 += XXH_read64(p + 24) * XXH_PRIME_2;
            v4 = XXH_rotl64(v4, 31);
            v4 *= XXH_PRIME_1;

            p += 32;
        } while (p <= limit);

        h64 = XXH_rotl64(v1, 1) + XXH_rotl64(v2, 7) + XXH_rotl64(v3, 12) + XXH_rotl64(v4, 18);

        v1 *= XXH_PRIME_2;
        v1 = XXH_rotl64(v1, 31);
        v1 *= XXH_PRIME_1;
        h64 ^= v1;
        h64 = h64 * XXH_PRIME_1 + XXH_PRIME_4;

        v2 *= XXH_PRIME_2;
        v2 = XXH_rotl64(v2, 31);
        v2 *= XXH_PRIME_1;
        h64 ^= v2;
        h64 = h64 * XXH_PRIME_1 + XXH_PRIME_4;

        v3 *= XXH_PRIME_2;
        v3 = XXH_rotl64(v3, 31);
        v3 *= XXH_PRIME_1;
        h64 ^= v3;
        h64 = h64 * XXH_PRIME_1 + XXH_PRIME_4;

        v4 *= XXH_PRIME_2;
        v4 = XXH_rotl64(v4, 31);
        v4 *= XXH_PRIME_1;
        h64 ^= v4;
        h64 = h64 * XXH_PRIME_1 + XXH_PRIME_4;
    } else {
        h64 = seed + XXH_PRIME_5;
    }

    h64 += (uint64_t)len;

    while (p + 8 <= end) {
        uint64_t k1 = XXH_read64(p);
        k1 *= XXH_PRIME_2;
        k1 = XXH_rotl64(k1, 31);
        k1 *= XXH_PRIME_1;
        h64 ^= k1;
        h64 = XXH_rotl64(h64, 27) * XXH_PRIME_1 + XXH_PRIME_4;
        p += 8;
    }

    if (p + 4 <= end) {
        h64 ^= (uint64_t)(XXH_read32(p)) * XXH_PRIME_1;
        h64 = XXH_rotl64(h64, 23) * XXH_PRIME_2 + XXH_PRIME_3;
        p += 4;
    }

    while (p < end) {
        h64 ^= (*p) * XXH_PRIME_5;
        h64 = XXH_rotl64(h64, 11) * XXH_PRIME_1;
        p++;
    }

    h64 ^= h64 >> 33;
    h64 *= XXH_PRIME_2;
    h64 ^= h64 >> 29;
    h64 *= XXH_PRIME_3;
    h64 ^= h64 >> 32;

    return h64;
}
