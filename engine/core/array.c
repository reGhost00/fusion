/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

/*
 * MT safe
 */

// #include "config.h"

#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "hash2.h"
#include "logger.h"
#include "memory.h"

#define MIN_ARRAY_SIZE 16
/**
 * g_direct_equal:
 * @v1: (nullable): a key
 * @v2: (nullable): a key to compare with @v1
 *
 * Compares two #void* arguments and returns %TRUE if they are equal.
 * It can be passed to g_hash_table_new() as the @fnKeyEqual
 * parameter, when using opaque pointers compared by pointer value as
 * keys in a #GHashTable.
 *
 * This equality function is also appropriate for keys that are integers
 * stored in pointers, such as `int_TO_POINTER (n)`.
 *
 * Returns: %TRUE if the two keys match.
 */
static bool fu_direct_equal(const void* v1, const void* v2)
{
    return v1 == v2;
}

/* Returns the smallest power of 2 greater than or equal to n,
 * or 0 if such power does not fit in a size_t
 */
static inline size_t fu_nearest_pow(size_t num)
{
    size_t n = num - 1;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n + 1;
}

typedef struct _TStableSorterParam {
    size_t s;
    size_t var;
    FUCompareDataFunc cmp;
    void* arg;
    char* t;
} TStableSorterParam;

static void fu_sort_with_tmp(const TStableSorterParam* p, void* b, size_t n)
{
    char *b1, *b2;
    size_t n1, n2;
    char* tmp = p->t;
    const size_t s = p->s;
    FUCompareDataFunc cmp = p->cmp;
    void* arg = p->arg;

    if (n <= 1)
        return;

    n1 = n / 2;
    n2 = n - n1;
    b1 = b;
    b2 = (char*)b + (n1 * p->s);

    fu_sort_with_tmp(p, b1, n1);
    fu_sort_with_tmp(p, b2, n2);

    switch (p->var) {
    case 0:
        while (n1 > 0 && n2 > 0) {
            if ((*cmp)(b1, b2, arg) <= 0) {
                *(uint32_t*)tmp = *(uint32_t*)b1;
                b1 += sizeof(uint32_t);
                --n1;
            } else {
                *(uint32_t*)tmp = *(uint32_t*)b2;
                b2 += sizeof(uint32_t);
                --n2;
            }
            tmp += sizeof(uint32_t);
        }
        break;
    case 1:
        while (n1 > 0 && n2 > 0) {
            if ((*cmp)(b1, b2, arg) <= 0) {
                *(uint64_t*)tmp = *(uint64_t*)b1;
                b1 += sizeof(uint64_t);
                --n1;
            } else {
                *(uint64_t*)tmp = *(uint64_t*)b2;
                b2 += sizeof(uint64_t);
                --n2;
            }
            tmp += sizeof(uint64_t);
        }
        break;
    case 2:
        while (n1 > 0 && n2 > 0) {
            uintptr_t* tmpl = (uintptr_t*)tmp;
            uintptr_t* bl;

            tmp += s;
            if ((*cmp)(b1, b2, arg) <= 0) {
                bl = (uintptr_t*)b1;
                b1 += s;
                --n1;
            } else {
                bl = (uintptr_t*)b2;
                b2 += s;
                --n2;
            }
            while (tmpl < (uintptr_t*)tmp)
                *tmpl++ = *bl++;
        }
        break;
    case 3:
        while (n1 > 0 && n2 > 0) {
            if ((*cmp)(*(const void**)b1, *(const void**)b2, arg) <= 0) {
                *(void**)tmp = *(void**)b1;
                b1 += sizeof(void*);
                --n1;
            } else {
                *(void**)tmp = *(void**)b2;
                b2 += sizeof(void*);
                --n2;
            }
            tmp += sizeof(void*);
        }
        break;
    default:
        while (n1 > 0 && n2 > 0) {
            if ((*cmp)(b1, b2, arg) <= 0) {
                memcpy(tmp, b1, s);
                tmp += s;
                b1 += s;
                --n1;
            } else {
                memcpy(tmp, b2, s);
                tmp += s;
                b2 += s;
                --n2;
            }
        }
        break;
    }

    if (n1 > 0)
        memcpy(tmp, b1, n1 * s);
    memcpy(b, p->t, (n - n2) * s);
}

static void fu_sort(void* b, size_t n, size_t s, FUCompareDataFunc cmp, void* arg)
{
    size_t size = n * s;
    char* tmp = NULL;
    TStableSorterParam p;

    /* For large object sizes use indirect sorting.  */
    if (s > 32)
        size = 2 * n * sizeof(void*) + s;
    tmp = p.t = fu_malloc(size);
    //   if (size < 1024)
    //     /* The temporary array is small, so put it on the stack.  */
    //     p.t = g_alloca (size);
    //   else
    //     {
    //       /* It's large, so malloc it.  */
    //       tmp = g_malloc (size);
    //       p.t = tmp;
    //     }

    p.s = s;
    p.var = 4;
    p.cmp = cmp;
    p.arg = arg;

    if (s > 32) {
        /* Indirect sorting.  */
        char* ip = (char*)b;
        void** tp = (void**)(p.t + n * sizeof(void*));
        void** t = tp;
        void* tmp_storage = (void*)(tp + n);
        char* kp;
        size_t i;

        while ((void*)t < tmp_storage) {
            *t++ = ip;
            ip += s;
        }
        p.s = sizeof(void*);
        p.var = 3;
        fu_sort_with_tmp(&p, p.t + n * sizeof(void*), n);

        /* tp[0] .. tp[n - 1] is now sorted, copy around entries of
           the original array.  Knuth vol. 3 (2nd ed.) exercise 5.2-10.  */
        for (i = 0, ip = (char*)b; i < n; i++, ip += s)
            if ((kp = tp[i]) != ip) {
                size_t j = i;
                char* jp = ip;
                memcpy(tmp_storage, ip, s);

                do {
                    size_t k = (kp - (char*)b) / s;
                    tp[j] = jp;
                    memcpy(jp, kp, s);
                    j = k;
                    jp = kp;
                    kp = tp[k];
                } while (kp != ip);

                tp[j] = jp;
                memcpy(jp, tmp_storage, s);
            }
    } else {
        if ((s & (sizeof(uint32_t) - 1)) == 0
            && (size_t)(uintptr_t)b % alignof(uint32_t) == 0) {
            if (s == sizeof(uint32_t))
                p.var = 0;
            else if (s == sizeof(uint64_t)
                && (size_t)(uintptr_t)b % alignof(uint64_t) == 0)
                p.var = 1;
            else if ((s & (sizeof(void*) - 1)) == 0
                && (size_t)(uintptr_t)b % alignof(void*) == 0)
                p.var = 2;
        }
        fu_sort_with_tmp(&p, b, n);
    }
    fu_free(tmp);
}
//
// Bytes

/**
 * FUBytes: (copy-func fu_bytes_ref) (free-func fu_bytes_unref)
 *
 * A simple refcounted data type representing an immutable sequence of zero or
 * more bytes from an unspecified origin.
 *
 * The purpose of a #FUBytes is to keep the memory region that it holds
 * alive for as long as anyone holds a reference to the bytes.  When
 * the last reference count is dropped, the memory is released. Multiple
 * unrelated callers can use byte data in the #FUBytes without coordinating
 * their activities, resting assured that the byte data will not change or
 * move while they hold a reference.
 *
 * A #FUBytes can come from many different origins that may have
 * different procedures for freeing the memory region.  Examples are
 * memory from g_malloc(), from memory slices, from a #GMappedFile or
 * memory from other allocators.
 *
 * #FUBytes work well as keys in #GHashTable. Use fu_bytes_equal() and
 * fu_bytes_hash() as parameters to g_hash_table_new() or g_hash_table_new_full().
 * #FUBytes can also be used as keys in a #GTree by passing the fu_bytes_compare()
 * function to g_tree_new().
 *
 * The data pointed to by this bytes must not be modified. For a mutable
 * array of bytes see #GByteArray. Use fu_bytes_unref_to_array() to create a
 * mutable array for a #FUBytes sequence. To create an immutable #FUBytes from
 * a mutable #GByteArray, use the g_byte_array_free_to_bytes() function.
 *
 * Since: 2.32
 **/

/* Keep in sync with glib/tests/bytes.c */
struct _FUBytes {
    const void* data; /* may be NULL iff (size == 0) */
    size_t size; /* may be 0 */
    FUNotify fnFree;
    void* usd;
    atomic_int ref;
};

// typedef struct
// {
//     FUBytes bytes;
//     /* Despite no guarantee about alignment in FUBytes, it is nice to
//      * provide that to ensure that any code which predates support
//      * for inline data continues to work without disruption. malloc()
//      * on glibc systems would guarantee 2*sizeof(void*) aligned
//      * allocations and this matches that.
//      */
//     size_t padding;
//     uint8_t inline_data[];
// } GBytesInline;

// G_STATIC_ASSERT(G_STRUCT_OFFSET(GBytesInline, inline_data) == (6 * GLIB_SIZEOF_VOID_P));
/**
 * fu_bytes_new_with_free_func: (skip)
 * @data: (array length=size) (element-type uint8_t) (nullable):
 *        the data to be used for the bytes
 * @size: the size of @data
 * @free_func: the function to call to release the data
 * @user_data: data to pass to @free_func
 *
 * Creates a #FUBytes from @data.
 *
 * When the last reference is dropped, @free_func will be called with the
 * @user_data argument.
 *
 * @data must not be modified after this call is made until @free_func has
 * been called to indicate that the bytes is no longer in use.
 *
 * @data may be %NULL if @size is 0.
 *
 * Returns: (transfer full): a new #FUBytes
 *
 * Since: 2.32
 */
FUBytes* fu_bytes_new_full(const void* data, size_t size, FUNotify fnFree, void* usd)
{
    fu_return_val_if_fail(data && size, NULL);
    FUBytes* bytes = fu_malloc0(sizeof(FUBytes));
    bytes->data = data;
    bytes->size = size;
    bytes->fnFree = fnFree;
    bytes->usd = usd;
    atomic_init(&bytes->ref, 1);

    return bytes;
}

/**
 * fu_bytes_new_take:
 * @data: (transfer full) (array length=size) (element-type uint8_t) (nullable):
 *        the data to be used for the bytes
 * @size: the size of @data
 *
 * Creates a new #FUBytes from @data.
 *
 * After this call, @data belongs to the #FUBytes and may no longer be
 * modified by the caller. The memory of @data has to be dynamically
 * allocated and will eventually be freed with g_free().
 *
 * For creating #FUBytes with memory from other allocators, see
 * fu_bytes_new_with_free_func().
 *
 * @data may be %NULL if @size is 0.
 *
 * Returns: (transfer full): a new #FUBytes
 *
 * Since: 2.32
 */
FUBytes* fu_bytes_new_take(void** data, size_t size)
{
    return fu_bytes_new_full(fu_steal_pointer(data), size, fu_free, data);
}

/**
 * fu_bytes_new_static: (skip)
 * @data: (transfer full) (array length=size) (element-type uint8_t) (nullable):
 *        the data to be used for the bytes
 * @size: the size of @data
 *
 * Creates a new #FUBytes from static data.
 *
 * @data must be static (ie: never modified or freed). It may be %NULL if @size
 * is 0.
 *
 * Returns: (transfer full): a new #FUBytes
 *
 * Since: 2.32
 */
FUBytes* fu_bytes_new_static(const void* data, size_t size)
{
    return fu_bytes_new_full(data, size, NULL, NULL);
}

/**
 * fu_bytes_new_from_bytes:
 * @bytes: a #FUBytes
 * @offset: offset which subsection starts at
 * @length: length of subsection
 *
 * Creates a #FUBytes which is a subsection of another #FUBytes. The @offset +
 * @length may not be longer than the size of @bytes.
 *
 * A reference to @bytes will be held by the newly created #FUBytes until
 * the byte data is no longer needed.
 *
 * Since 2.56, if @offset is 0 and @length matches the size of @bytes, then
 * @bytes will be returned with the reference count incremented by 1. If @bytes
 * is a slice of another #FUBytes, then the resulting #FUBytes will reference
 * the same #FUBytes instead of @bytes. This allows consumers to simplify the
 * usage of #FUBytes when asynchronously writing to streams.
 *
 * Returns: (transfer full): a new #FUBytes
 *
 * Since: 2.32
 */
FUBytes* fu_bytes_new_from_bytes(FUBytes* bytes, size_t offset, size_t length)
{
    /* Note that length may be 0. */
    fu_return_val_if_fail(bytes != NULL, NULL);
    fu_return_val_if_fail(offset <= bytes->size, NULL);
    fu_return_val_if_fail(offset + length <= bytes->size, NULL);

    /* Avoid an extra FUBytes if all bytes were requested */
    if (!offset && length == bytes->size)
        return fu_bytes_ref(bytes);

    char* base = (char*)bytes->data + offset;

    // /* Avoid referencing intermediate FUBytes. In practice, this should
    //  * only loop once.
    //  */
    // while (bytes->fnFree == (void*)fu_bytes_unref)
    //     bytes = bytes->usd;

    if (base < (char*)bytes->data)
        return NULL;
    if (base > (char*)bytes->data + bytes->size)
        return NULL;
    if (base + length > (char*)bytes->data + bytes->size)
        return NULL;

    return fu_bytes_new_full(base, length, (FUNotify)fu_bytes_unref, fu_bytes_ref(bytes));
}

/**
 * fu_bytes_get_data:
 * @bytes: a #FUBytes
 * @size: (out) (optional): location to return size of byte data
 *
 * Get the byte data in the #FUBytes. This data should not be modified.
 *
 * This function will always return the same pointer for a given #FUBytes.
 *
 * %NULL may be returned if @size is 0. This is not guaranteed, as the #FUBytes
 * may represent an empty string with @data non-%NULL and @size as 0. %NULL will
 * not be returned if @size is non-zero.
 *
 * Returns: (transfer none) (array length=size) (element-type uint8_t) (nullable):
 *          a pointer to the byte data, or %NULL
 *
 * Since: 2.32
 */
const void* fu_bytes_get_data(FUBytes* bytes, size_t* size)
{
    fu_return_val_if_fail(bytes, NULL);
    if (size)
        *size = bytes->size;
    return bytes->data;
}

/**
 * fu_bytes_get_size:
 * @bytes: a #FUBytes
 *
 * Get the size of the byte data in the #FUBytes.
 *
 * This function will always return the same value for a given #FUBytes.
 *
 * Returns: the size
 *
 * Since: 2.32
 */
size_t fu_bytes_get_size(FUBytes* bytes)
{
    fu_return_val_if_fail(bytes, 0);
    return bytes->size;
}

/**
 * fu_bytes_ref:
 * @bytes: a #FUBytes
 *
 * Increase the reference count on @bytes.
 *
 * Returns: the #FUBytes
 *
 * Since: 2.32
 */
FUBytes* fu_bytes_ref(FUBytes* bytes)
{
    fu_return_val_if_fail(bytes, NULL);
    atomic_fetch_add(&bytes->ref, 1);

    return bytes;
}

/**
 * fu_bytes_unref:
 * @bytes: (nullable): a #FUBytes
 *
 * Releases a reference on @bytes.  This may result in the bytes being
 * freed. If @bytes is %NULL, it will return immediately.
 *
 * Since: 2.32
 */
void fu_bytes_unref(FUBytes* bytes)
{
    if (!bytes)
        return;

    if (1 >= atomic_fetch_sub(&bytes->ref, 1)) {
        if (bytes->fnFree)
            bytes->fnFree(bytes->usd);
        fu_free(bytes);
    }
}

// static inline void* fu_bytes_try_steal_and_unref(FUBytes* bytes, FUNotify fnFree, size_t* size)
// {
//     void* result;

//     if (bytes->fnFree != fnFree || !bytes->data || bytes->usd != bytes->data)
//         return NULL;

//     /* Are we the only reference? */
//     if (1 >= atomic_fetch_sub(&bytes->ref, 1)) {
//         *size = bytes->size;
//         result = (void*)bytes->data;
//         // g_assert(result != NULL); /* otherwise the case of @bytes being freed can’t be distinguished */
//         fu_free(bytes);
//         return result;
//     }

//     return NULL;
// }
/**
 * fu_bytes_unref_to_data:
 * @bytes: (transfer full): a #FUBytes
 * @size: (out): location to place the length of the returned data
 *
 * Unreferences the bytes, and returns a pointer the same byte data
 * contents.
 *
 * As an optimization, the byte data is returned without copying if this was
 * the last reference to bytes and bytes was created with fu_bytes_new(),
 * fu_bytes_new_take() or g_byte_array_free_to_bytes() and the buffer was larger
 * than the size #FUBytes may internalize within its allocation. In all other
 * cases the data is copied.
 *
 * Returns: (transfer full) (array length=size) (element-type uint8_t)
 *          (not nullable): a pointer to the same byte data, which should be
 *          freed with g_free()
 *
 * Since: 2.32
 */
void* fu_bytes_unref_to_data(FUBytes* bytes, size_t* size)
{
    fu_return_val_if_fail(bytes && size, NULL);
    /*
     * Optimal path: if this is was the last reference, then we can return
     * the data from this FUBytes without copying.
     */
    /* Are we the only reference? */
    *size = bytes->size;
    if (1 >= atomic_fetch_sub(&bytes->ref, 1)) {
        void* res = (void*)bytes->data;
        // g_assert(result != NULL); /* otherwise the case of @bytes being freed can’t be distinguished */
        fu_free(bytes);
        return res;
    }
    return fu_memdup(bytes->data, bytes->size);

    // void *result = fu_bytes_try_steal_and_unref(bytes, fu_free, size);
    // if (!result) {
    /*
     * Copy: Non g_malloc (or compatible) allocator, or static memory,
     * so we have to copy, and then unref.
     */
    //     result = fu_memdup(bytes->data, bytes->size);
    //     *size = bytes->size;
    //     fu_bytes_unref(bytes);
    // }

    // return result;
}

/**
 * fu_bytes_unref_to_array:
 * @bytes: (transfer full): a #FUBytes
 *
 * Unreferences the bytes, and returns a new mutable #GByteArray containing
 * the same byte data.
 *
 * As an optimization, the byte data is transferred to the array without copying
 * if this was the last reference to bytes and bytes was created with
 * fu_bytes_new(), fu_bytes_new_take() or g_byte_array_free_to_bytes() and the
 * buffer was larger than the size #FUBytes may internalize within its allocation.
 * In all other cases the data is copied.
 *
 * Do not use it if @bytes contains more than %G_MAXUINT
 * bytes. #GByteArray stores the length of its data in #guint, which
 * may be shorter than #size_t, that @bytes is using.
 *
 * Returns: (transfer full): a new mutable #GByteArray containing the same byte data
 *
 * Since: 2.32
 */
FUByteArray* fu_bytes_unref_to_array(FUBytes* bytes)
{
    fu_return_val_if_fail(bytes, NULL);
    size_t size;
    void* data = fu_bytes_unref_to_data(bytes, &size);
    return fu_byte_array_new_take(data, size);
}

/**
 * fu_bytes_equal:
 * @bytes1: (type GLib.Bytes): a pointer to a #FUBytes
 * @bytes2: (type GLib.Bytes): a pointer to a #FUBytes to compare with @bytes1
 *
 * Compares the two #FUBytes values being pointed to and returns
 * %TRUE if they are equal.
 *
 * This function can be passed to g_hash_table_new() as the @key_equal_func
 * parameter, when using non-%NULL #FUBytes pointers as keys in a #GHashTable.
 *
 * Returns: %TRUE if the two keys match.
 *
 * Since: 2.32
 */
bool fu_bytes_equal(const FUBytes* bytes1, const FUBytes* bytes2)
{
    fu_return_val_if_fail(bytes1 && bytes2, false);

    return bytes1->size == bytes2->size && (!(bytes1->size && memcmp(bytes1->data, bytes2->data, bytes1->size)));
}

/**
 * fu_bytes_hash:
 * @bytes: (type GLib.Bytes): a pointer to a #FUBytes key
 *
 * Creates an integer hash code for the byte data in the #FUBytes.
 *
 * This function can be passed to g_hash_table_new() as the @key_hash_func
 * parameter, when using non-%NULL #FUBytes pointers as keys in a #GHashTable.
 *
 * Returns: a hash value corresponding to the key.
 *
 * Since: 2.32
 */
uint64_t fu_bytes_hash(const FUBytes* bytes)
{
    fu_return_val_if_fail(bytes, 0);
    return fu_hash_mm3(bytes->data, bytes->size);
    // const FUBytes* a = bytes;
    // const signed char *p, *e;
    // guint32 h = 5381;

    // g_return_val_if_fail(bytes != NULL, 0);

    // for (p = (signed char*)a->data, e = (signed char*)a->data + a->size; p != e; p++)
    //     h = (h << 5) + h + *p;

    // return h;
}

/**
 * fu_bytes_get_region:
 * @bytes: a #FUBytes
 * @element_size: a non-zero element size
 * @offset: an offset to the start of the region within the @bytes
 * @n_elements: the number of elements in the region
 *
 * Gets a pointer to a region in @bytes.
 *
 * The region starts at @offset many bytes from the start of the data
 * and contains @n_elements many elements of @element_size size.
 *
 * @n_elements may be zero, but @element_size must always be non-zero.
 * Ideally, @element_size is a static constant (eg: sizeof a struct).
 *
 * This function does careful bounds checking (including checking for
 * arithmetic overflows) and returns a non-%NULL pointer if the
 * specified region lies entirely within the @bytes. If the region is
 * in some way out of range, or if an overflow has occurred, then %NULL
 * is returned.
 *
 * Note: it is possible to have a valid zero-size region. In this case,
 * the returned pointer will be equal to the base pointer of the data of
 * @bytes, plus @offset.  This will be non-%NULL except for the case
 * where @bytes itself was a zero-sized region.  Since it is unlikely
 * that you will be using this function to check for a zero-sized region
 * in a zero-sized @bytes, %NULL effectively always means "error".
 *
 * Returns: (nullable): the requested region, or %NULL in case of an error
 *
 * Since: 2.70
 */
const void* fu_bytes_get_region(FUBytes* bytes, size_t elementSize, size_t offset, size_t n_elements)
{
    size_t totalSize;
    size_t endOffset;

    fu_return_val_if_fail(elementSize > 0, NULL);

    /* No other assertion checks here.  If something is wrong then we will
     * simply crash (via NULL dereference or divide-by-zero).
     */

    if (!__builtin_mul_overflow(elementSize, n_elements, &totalSize))
        return NULL;

    if (!__builtin_add_overflow(offset, totalSize, &endOffset))
        return NULL;

    /* We now have:
     *
     *   0 <= offset <= end_offset
     *
     * So we need only check that end_offset is within the range of the
     * size of @bytes and we're good to go.
     */

    if (endOffset > bytes->size)
        return NULL;

    /* We now have:
     *
     *   0 <= offset <= end_offset <= bytes->size
     */

    return ((uintptr_t*)bytes->data) + offset;
}

/**
 * GArray:
 * @data: a pointer to the element data. The data may be moved as
 *     elements are added to the #GArray.
 * @len: the number of elements in the #GArray not including the
 *     possible terminating zero element.
 *
 * Contains the public fields of a GArray.
 */
typedef struct _TArray {
    uint8_t* data;
    uint32_t len;
    uint32_t elCapacity;
    /** the size of each element */
    uint32_t elSize;
    atomic_int ref;
} TArray;
static_assert(sizeof(FUArray) == sizeof(TArray));
/**
 * fu_array_index:
 * @a: a #GArray
 * @t: the type of the elements
 * @i: the index of the element to return
 *
 * Returns the element of a #GArray at the given index. The return
 * value is cast to the given type. This is the main way to read or write an
 * element in a #GArray.
 *
 * Writing an element is typically done by reference, as in the following
 * example. This example gets a pointer to an element in a #GArray, and then
 * writes to a field in it:
 * |[<!-- language="C" -->
 *   EDayViewEvent *event;
 *   // This gets a pointer to the 4th element in the array of
 *   // EDayViewEvent structs.
 *   event = &fu_array_index (events, EDayViewEvent, 3);
 *   event->start_time = g_get_current_time ();
 * ]|
 *
 * This example reads from and writes to an array of integers:
 * |[<!-- language="C" -->
 *   g_autoptr(GArray) int_array = fu_array_new (FALSE, FALSE, sizeof (uint32_t));
 *   for (uint32_t i = 0; i < 10; i++)
 *     fu_array_append_val (int_array, i);
 *
 *   uint32_t *my_int = &fu_array_index (int_array, uint32_t, 1);
 *   g_print ("Int at index 1 is %u; decrementing it\n", *my_int);
 *   *my_int = *my_int - 1;
 * ]|
 *
 * Returns: the element of the #GArray at the index given by @i
 */

#define fu_array_elt_len(array, i) ((size_t)(array)->elSize * (i))
#define fu_array_elt_pos(array, i) ((array)->data + fu_array_elt_len((array), (i)))
#define fu_array_elt_zero(array, pos, len) \
    (memset(fu_array_elt_pos((array), pos), 0, fu_array_elt_len((array), len)))
#define fu_array_zero_terminate(array)                   \
    G_STMT_START                                         \
    {                                                    \
        if ((array)->zero_terminated)                    \
            fu_array_elt_zero((array), (array)->len, 1); \
    }                                                    \
    G_STMT_END

static void fu_array_maybe_expand(TArray* array, uint32_t len)
{
    /* The maximum array length is derived from following constraints:
     * - The number of bytes must fit into a gsize / 2.
     * - The number of elements must fit into uint32_t.
     * - zero terminated arrays must leave space for the terminating element
     */

    /* Detect potential overflow */
    const uint32_t maxLen = (uint32_t)-2;
    if (FU_UNLIKELY(maxLen - array->len < len)) {
        fprintf(stderr, "adding %u to array would overflow", len);
        return;
    }

    uint32_t nextLen = array->len + len + 1;
    if (nextLen > array->elCapacity) {
        size_t alloc = fu_nearest_pow(fu_array_elt_len(array, nextLen));
        if (MIN_ARRAY_SIZE > alloc)
            alloc = MIN_ARRAY_SIZE;

        array->data = fu_realloc(array->data, alloc);

        memset(fu_array_elt_pos(array, array->elCapacity), 0, fu_array_elt_len(array, nextLen - array->elCapacity));

        array->elCapacity = alloc / array->elSize;
        if (FU_UNLIKELY(maxLen < array->elCapacity))
            array->elCapacity = maxLen;
    }
}
/**
 * fu_array_sized_new:
 * @zero_terminated: %TRUE if the array should have an extra element at
 *     the end with all bits cleared
 * @clear_: %TRUE if all bits in the array should be cleared to 0 on
 *     allocation
 * @element_size: size of each element in the array
 * @reserved_size: number of elements preallocated
 *
 * Creates a new #GArray with @reserved_size elements preallocated and
 * a reference count of 1. This avoids frequent reallocation, if you
 * are going to add many elements to the array. Note however that the
 * size of the array is still 0.
 *
 * Returns: the new #GArray
 */
FUArray* fu_array_new_full(uint32_t elSize, uint32_t reservedSize)
{
    fu_return_val_if_fail(elSize, NULL);
    TArray* arr = fu_malloc0(sizeof(TArray));
    arr->elSize = elSize;
    // g_atomic_ref_count_init(&arr->ref_count);

    if (reservedSize)
        fu_array_maybe_expand(arr, reservedSize);
    atomic_init(&arr->ref, 1);
    return (FUArray*)arr;
}
/**
 * fu_array_new_take: (skip)
 * @data: (array length=len) (transfer full) (nullable): an array of
 *   elements of @element_size, or %NULL for an empty array
 * @len: the number of elements in @data
 * @clear: %TRUE if #GArray elements should be automatically cleared
 *     to 0 when they are allocated
 * @element_size: the size of each element in bytes
 *
 * Creates a new #GArray with @data as array data, @len as length and a
 * reference count of 1.
 *
 * This avoids having to copy the data manually, when it can just be
 * inherited.
 * After this call, @data belongs to the #GArray and may no longer be
 * modified by the caller. The memory of @data has to be dynamically
 * allocated and will eventually be freed with g_free().
 *
 * In case the elements need to be cleared when the array is freed, use
 * fu_array_set_clear_func() to set a #FUNotify function to perform
 * such task.
 *
 * Do not use it if @len or @element_size are greater than %G_MAXUINT.
 * #GArray stores the length of its data in #uint32_t, which may be shorter
 * than #gsize.
 *
 * Returns: (transfer full): A new #GArray
 *
 * Since: 2.76
 */
FUArray* fu_array_new_take(void* data, uint32_t len, uint32_t elementSize)
{
    fu_return_val_if_fail(data && len && elementSize && (uint32_t)-2 > elementSize, NULL);
    FUArray* rev;
    TArray* arr = (TArray*)(rev = fu_array_new_full(elementSize, 0));
    arr->data = (uint8_t*)fu_steal_pointer(&data);
    arr->len = len;
    arr->elCapacity = len;
    return rev;
}
/**
 * fu_array_new:
 * @zero_terminated: %TRUE if the array should have an extra element at
 *     the end which is set to 0
 * @clear_: %TRUE if #GArray elements should be automatically cleared
 *     to 0 when they are allocated
 * @element_size: the size of each element in bytes
 *
 * Creates a new #GArray with a reference count of 1.
 *
 * Returns: the new #GArray
 */
FUArray* fu_array_new(uint32_t elSize)
{
    fu_return_val_if_fail(elSize, NULL);
    return fu_array_new_full(elSize, 0);
}
/**
 * fu_array_ref:
 * @array: A #GArray
 *
 * Atomically increments the reference count of @array by one.
 * This function is thread-safe and may be called from any thread.
 *
 * Returns: The passed in #GArray
 *
 * Since: 2.22
 */
FUArray* fu_array_ref(FUArray* array)
{
    fu_return_val_if_fail(array, NULL);
    TArray* arr = (TArray*)array;
    atomic_fetch_add(&arr->ref, 1);
    return array;
}

/**
 * fu_array_unref:
 * @array: A #GArray
 *
 * Atomically decrements the reference count of @array by one. If the
 * reference count drops to 0, all memory allocated by the array is
 * released. This function is thread-safe and may be called from any
 * thread.
 *
 * Since: 2.22
 */
void fu_array_unref(FUArray* array)
{
    fu_return_if_fail(array);
    TArray* arr = (TArray*)array;
    if (1 >= atomic_fetch_sub(&arr->ref, 1))
        fu_array_free(array, true);
}

/**
 * fu_array_free:
 * @array: a #GArray
 * @free_segment: if %TRUE the actual element data is freed as well
 *
 * Frees the memory allocated for the #GArray. If @free_segment is
 * %TRUE it frees the memory block holding the elements as well. Pass
 * %FALSE if you want to free the #GArray wrapper but preserve the
 * underlying array for use elsewhere. If the reference count of
 * @array is greater than one, the #GArray wrapper is preserved but
 * the size of  @array will be set to zero.
 *
 * If array contents point to dynamically-allocated memory, they should
 * be freed separately if @free_seg is %TRUE and no @clear_func
 * function has been set for @array.
 *
 * This function is not thread-safe. If using a #GArray from multiple
 * threads, use only the atomic fu_array_ref() and fu_array_unref()
 * functions.
 *
 * Returns: the element data if @free_segment is %FALSE, otherwise
 *     %NULL. The element data should be freed using g_free().
 */
uint8_t* fu_array_free(FUArray* array, bool freeSegment)
{
    fu_return_val_if_fail(array, NULL);
    uint8_t* rev = NULL;
    if (!freeSegment)
        rev = fu_steal_pointer(&array->data);
    fu_free(array->data);
    fu_free(array);
    return rev;
}

/**
* fu_array_steal:
* @array: a #GArray.
* @len: (optional) (out): pointer to retrieve the number of
*    elements of the original array
*
* Frees the data in the array and resets the size to zero, while
* the underlying array is preserved for use elsewhere and returned
* to the caller.
*
* If the array was created with the @zero_terminate property
* set to %TRUE, the returned data is zero terminated too.
*
* If array elements contain dynamically-allocated memory,
* the array elements should also be freed by the caller.
*
* A short example of use:
* |[<!-- language="C" -->
* ...
* void* data;
* gsize data_len;
* data = fu_array_steal (some_array, &data_len);
* ...
* ]|

* Returns: (transfer full): the element data, which should be
*     freed using g_free().
*
* Since: 2.64
*/
void* fu_array_steal(FUArray* array, uint32_t* len)
{
    fu_return_val_if_fail(array, NULL);

    TArray* arr = (TArray*)array;
    void* rev = fu_steal_pointer(&arr->data);
    if (len)
        *len = arr->len;

    arr->len = arr->elCapacity = 0;
    return rev;
}

/**
 * g_array_copy:
 * @array: A #GArray.
 *
 * Create a shallow copy of a #GArray. If the array elements consist of
 * pointers to data, the pointers are copied but the actual data is not.
 *
 * Returns: (transfer container): A copy of @array.
 *
 * Since: 2.62
 **/
FUArray* fu_array_copy(FUArray* array)
{
    fu_return_val_if_fail(array, NULL);
    FUArray* rev;
    TArray* arr1 = (TArray*)array;
    TArray* arr2 = (TArray*)(rev = fu_array_new_full(arr1->elSize, arr1->elCapacity));
    if (0 < (arr2->len = arr1->len))
        memcpy(arr2->data, arr1->data, arr1->len * arr1->elSize);
    return rev;
}

/**
 * fu_array_get_element_size:
 * @array: A #GArray
 *
 * Gets the size of the elements in @array.
 *
 * Returns: Size of each element, in bytes
 *
 * Since: 2.22
 */
uint32_t fu_array_get_element_size(FUArray* array)
{
    fu_return_val_if_fail(array, 0);
    TArray* arr = (TArray*)array;
    return arr->elSize;
}

/**
 * fu_array_append_vals:
 * @array: a #GArray
 * @data: (not nullable): a pointer to the elements to append to the end of the array
 * @len: the number of elements to append
 *
 * Adds @len elements onto the end of the array.
 *
 * Returns: the #GArray
 */
/**
 * fu_array_append_val:
 * @a: a #GArray
 * @v: the value to append to the #GArray
 *
 * Adds the value on to the end of the array. The array will grow in
 * size automatically if necessary.
 *
 * fu_array_append_val() is a macro which uses a reference to the value
 * parameter @v. This means that you cannot use it with literal values
 * such as "27". You must use variables.
 *
 * Returns: the #GArray
 */
FUArray* fu_array_append_vals(FUArray* array, const void* data, uint32_t len)
{
    fu_return_val_if_fail(array, NULL);
    if (FU_UNLIKELY(!(len && data)))
        return array;
    TArray* arr = (TArray*)array;
    fu_array_maybe_expand(arr, len);

    memcpy(fu_array_elt_pos(arr, arr->len), data, fu_array_elt_len(arr, len));
    arr->len += len;
    return array;
}

/**
 * fu_array_prepend_vals:
 * @array: a #GArray
 * @data: (nullable): a pointer to the elements to prepend to the start of the array
 * @len: the number of elements to prepend, which may be zero
 *
 * Adds @len elements onto the start of the array.
 *
 * @data may be %NULL if (and only if) @len is zero. If @len is zero, this
 * function is a no-op.
 *
 * This operation is slower than fu_array_append_vals() since the
 * existing elements in the array have to be moved to make space for
 * the new elements.
 *
 * Returns: the #GArray
 */
/**
 * fu_array_prepend_val:
 * @a: a #GArray
 * @v: the value to prepend to the #GArray
 *
 * Adds the value on to the start of the array. The array will grow in
 * size automatically if necessary.
 *
 * This operation is slower than fu_array_append_val() since the
 * existing elements in the array have to be moved to make space for
 * the new element.
 *
 * fu_array_prepend_val() is a macro which uses a reference to the value
 * parameter @v. This means that you cannot use it with literal values
 * such as "27". You must use variables.
 *
 * Returns: the #GArray
 */
FUArray* fu_array_prepend_vals(FUArray* array, const void* data, uint32_t len)
{
    fu_return_val_if_fail(array, NULL);
    TArray* arr = (TArray*)array;
    if (!len)
        return array;

    fu_array_maybe_expand(arr, len);
    memmove(fu_array_elt_pos(arr, len), fu_array_elt_pos(arr, 0), fu_array_elt_len(arr, arr->len));
    memcpy(fu_array_elt_pos(arr, 0), data, fu_array_elt_len(arr, len));
    arr->len += len;
    return array;
}

/**
 * fu_array_insert_vals:
 * @array: a #GArray
 * @index_: the index to place the elements at
 * @data: (nullable): a pointer to the elements to insert
 * @len: the number of elements to insert
 *
 * Inserts @len elements into a #GArray at the given index.
 *
 * If @index_ is greater than the array’s current length, the array is expanded.
 * The elements between the old end of the array and the newly inserted elements
 * will be initialised to zero if the array was configured to clear elements;
 * otherwise their values will be undefined.
 *
 * If @index_ is less than the array’s current length, new entries will be
 * inserted into the array, and the existing entries above @index_ will be moved
 * upwards.
 *
 * @data may be %NULL if (and only if) @len is zero. If @len is zero, this
 * function is a no-op.
 *
 * Returns: the #GArray
 */
/**
 * fu_array_insert_val:
 * @a: a #GArray
 * @i: the index to place the element at
 * @v: the value to insert into the array
 *
 * Inserts an element into an array at the given index.
 *
 * fu_array_insert_val() is a macro which uses a reference to the value
 * parameter @v. This means that you cannot use it with literal values
 * such as "27". You must use variables.
 *
 * Returns: the #GArray
 */
FUArray* fu_array_insert_vals(FUArray* array, uint32_t idx, const void* data, uint32_t len)
{
    fu_return_val_if_fail(array, NULL);
    if (!len)
        return array;
    TArray* arr = (TArray*)array;
    /* Is the index off the end of the array, and hence do we need to over-allocate
     * and clear some elements? */
    if (idx >= arr->len) {
        fu_array_maybe_expand(arr, idx - arr->len + len);
        return fu_array_append_vals(fu_array_set_size(array, idx), data, len);
    }

    fu_array_maybe_expand(arr, len);

    memmove(fu_array_elt_pos(arr, len + idx), fu_array_elt_pos(arr, idx), fu_array_elt_len(arr, arr->len - idx));
    memcpy(fu_array_elt_pos(arr, idx), data, fu_array_elt_len(arr, len));

    arr->len += len;
    return array;
}

/**
 * fu_array_set_size:
 * @array: a #GArray
 * @length: the new size of the #GArray
 *
 * Sets the size of the array, expanding it if necessary. If the array
 * was created with @clear_ set to %TRUE, the new elements are set to 0.
 *
 * Returns: the #GArray
 */
FUArray* fu_array_set_size(FUArray* array, uint32_t length)
{
    fu_return_val_if_fail(array, NULL);
    TArray* arr = (TArray*)array;
    if (length > arr->len) {
        fu_array_maybe_expand(arr, length - arr->len);
        fu_array_elt_zero(arr, arr->len, length - arr->len);
    } else if (length < arr->len)
        fu_array_remove_range(array, length, arr->len - length);

    arr->len = length;
    return array;
}

/**
 * fu_array_remove_index:
 * @array: a #GArray
 * @index_: the index of the element to remove
 *
 * Removes the element at the given index from a #GArray. The following
 * elements are moved down one place.
 *
 * Returns: the #GArray
 */
FUArray* fu_array_remove_index(FUArray* array, uint32_t idx)
{
    fu_return_val_if_fail(array && idx < array->len, NULL);
    TArray* arr = (TArray*)array;

    if (idx != arr->len - 1)
        memmove(fu_array_elt_pos(arr, idx), fu_array_elt_pos(arr, idx + 1), fu_array_elt_len(arr, arr->len - idx - 1));

    arr->len -= 1;
    fu_array_elt_zero(arr, arr->len, 1);
    return array;
}

/**
 * fu_array_remove_range:
 * @array: a @GArray
 * @index_: the index of the first element to remove
 * @length: the number of elements to remove
 *
 * Removes the given number of elements starting at the given index
 * from a #GArray.  The following elements are moved to close the gap.
 *
 * Returns: the #GArray
 *
 * Since: 2.4
 */
FUArray* fu_array_remove_range(FUArray* array, uint32_t idx, uint32_t length)
{
    fu_return_val_if_fail(array && idx <= array->len, NULL);
    fu_return_val_if_fail(idx <= (uint32_t)-2 - length && idx + length <= array->len, NULL);
    TArray* arr = (TArray*)array;

    if (idx + length != array->len)
        memmove(fu_array_elt_pos(arr, idx), fu_array_elt_pos(arr, idx + length), (arr->len - (idx + length)) * arr->elSize);

    arr->len -= length;
    fu_array_elt_zero(arr, arr->len, length);

    return array;
}

void fu_array_remove_all(FUArray* array)
{
    fu_return_if_fail(array);
    TArray* arr = (TArray*)array;
    memset(arr->data, 0, arr->ref * arr->elSize);
    arr->len = 0;
}

/**
 * fu_array_sort:
 * @array: a #GArray
 * @compare_func: comparison function
 *
 * Sorts a #GArray using @compare_func which should be a qsort()-style
 * comparison function (returns less than zero for first arg is less
 * than second arg, zero for equal, greater zero if first arg is
 * greater than second arg).
 *
 * This is guaranteed to be a stable sort since version 2.32.
 */
void fu_array_sort(FUArray* array, FUCompareFunc compareFunc)
{
    fu_return_if_fail(array);
    TArray* arr = (TArray*)array;

    /* Don't use qsort as we want a guaranteed stable sort */
    if (0 < arr->len)
        fu_sort(arr->data, arr->len, arr->elSize, (FUCompareDataFunc)compareFunc, NULL);
}

/**
 * fu_array_sort_with_data:
 * @array: a #GArray
 * @compare_func: comparison function
 * @user_data: data to pass to @compare_func
 *
 * Like fu_array_sort(), but the comparison function receives an extra
 * user data argument.
 *
 * This is guaranteed to be a stable sort since version 2.32.
 *
 * There used to be a comment here about making the sort stable by
 * using the addresses of the elements in the comparison function.
 * This did not actually work, so any such code should be removed.
 */
void fu_array_sort_with_data(FUArray* array, FUCompareDataFunc compareFunc, void* usd)
{
    fu_return_if_fail(array);
    TArray* arr = (TArray*)array;
    if (0 < array->len)
        fu_sort(arr->data, arr->len, arr->elSize, compareFunc, usd);
}

/**
 * fu_array_binary_search:
 * @array: a #GArray.
 * @target: a pointer to the item to look up.
 * @compare_func: A #GCompareFunc used to locate @target.
 * @out_match_index: (optional) (out): return location
 *    for the index of the element, if found.
 *
 * Checks whether @target exists in @array by performing a binary
 * search based on the given comparison function @compare_func which
 * get pointers to items as arguments. If the element is found, %TRUE
 * is returned and the element’s index is returned in @out_match_index
 * (if non-%NULL). Otherwise, %FALSE is returned and @out_match_index
 * is undefined. If @target exists multiple times in @array, the index
 * of the first instance is returned. This search is using a binary
 * search, so the @array must absolutely be sorted to return a correct
 * result (if not, the function may produce false-negative).
 *
 * This example defines a comparison function and search an element in a #GArray:
 * |[<!-- language="C" -->
 * static gint
 * cmpint (gconstpointer a, gconstpointer b)
 * {
 *   const gint *_a = a;
 *   const gint *_b = b;
 *
 *   return *_a - *_b;
 * }
 * ...
 * gint i = 424242;
 * uint32_t matched_index;
 * gboolean result = fu_array_binary_search (garray, &i, cmpint, &matched_index);
 * ...
 * ]|
 *
 * Returns: %TRUE if @target is one of the elements of @array, %FALSE otherwise.
 *
 * Since: 2.62
 */
bool fu_array_binary_search(FUArray* array, const void* target, FUCompareFunc compareFunc, uint32_t* outMatchIndex)
{
    fu_return_val_if_fail(compareFunc && array, false);
    bool result = false;
    TArray* arr = (TArray*)array;
    uint32_t left, middle = 0, right;
    int val;

    if (FU_LIKELY(arr->len)) {
        left = 0;
        right = arr->len - 1;

        while (left <= right) {
            middle = left + (right - left) / 2;

            val = compareFunc(arr->data + (arr->elSize * middle), target);
            if (val == 0) {
                result = true;
                break;
            } else if (val < 0)
                left = middle + 1;
            else if (/* val > 0 && */ middle > 0)
                right = middle - 1;
            else
                break; /* element not found */
        }
    }

    if (result && outMatchIndex != NULL)
        *outMatchIndex = middle;

    return result;
}

//
// Byte array

/**
 * fu_byte_array_new:
 *
 * Creates a new #FUByteArray with a reference count of 1.
 *
 * Returns: (transfer full): the new #FUByteArray
 */
FUByteArray* fu_byte_array_new(void)
{
    return (FUByteArray*)fu_array_new_full(1, 0);
}

/**
 * fu_byte_array_sized_new:
 * @reserved_size: number of bytes preallocated
 *
 * Creates a new #FUByteArray with @reserved_size bytes preallocated.
 * This avoids frequent reallocation, if you are going to add many
 * bytes to the array. Note however that the size of the array is still
 * 0.
 *
 * Returns: (transfer full): the new #FUByteArray
 */
FUByteArray* fu_byte_array_sized_new(uint32_t reservedSize)
{
    return (FUByteArray*)fu_array_new_full(1, reservedSize);
}

/**
 * fu_byte_array_new_take:
 * @data: (transfer full) (array length=len): byte data for the array
 * @len: length of @data
 *
 * Creates a byte array containing the @data.
 * After this call, @data belongs to the #FUByteArray and may no longer be
 * modified by the caller. The memory of @data has to be dynamically
 * allocated and will eventually be freed with g_free().
 *
 * Do not use it if @len is greater than %G_MAXUINT. #FUByteArray
 * stores the length of its data in #guint, which may be shorter than
 * #gsize.
 *
 * Since: 2.32
 *
 * Returns: (transfer full): a new #FUByteArray
 */
FUByteArray* fu_byte_array_new_take(uint8_t** data, uint32_t len)
{
    fu_return_val_if_fail((uint32_t)-2 > len, NULL);
    FUByteArray* rev;
    TArray* arr = (TArray*)(rev = fu_byte_array_new());
    arr->data = fu_steal_pointer(data);
    arr->len = len;
    arr->elCapacity = len;
    return rev;
}

/**
 * fu_byte_array_steal:
 * @array: a #FUByteArray.
 * @len: (optional) (out): pointer to retrieve the number of
 *    elements of the original array
 *
 * Frees the data in the array and resets the size to zero, while
 * the underlying array is preserved for use elsewhere and returned
 * to the caller.
 *
 * Returns: (transfer full): the element data, which should be
 *     freed using g_free().
 *
 * Since: 2.64
 */
uint8_t* fu_byte_array_steal(FUByteArray* array, uint32_t* len)
{
    return (uint8_t*)fu_array_steal((FUArray*)array, len);
}

/**
 * fu_byte_array_free:
 * @array: a #FUByteArray
 * @free_segment: if %TRUE the actual byte data is freed as well
 *
 * Frees the memory allocated by the #FUByteArray. If @free_segment is
 * %TRUE it frees the actual byte data. If the reference count of
 * @array is greater than one, the #FUByteArray wrapper is preserved but
 * the size of @array will be set to zero.
 *
 * Returns: the element data if @free_segment is %FALSE, otherwise
 *          %NULL.  The element data should be freed using g_free().
 */
uint8_t* fu_byte_array_free(FUByteArray* array, bool freeSegment)
{
    return (uint8_t*)fu_array_free((FUArray*)array, freeSegment);
}

/**
 * fu_byte_array_free_to_bytes:
 * @array: (transfer full): a #FUByteArray
 *
 * Transfers the data from the #FUByteArray into a new immutable #GBytes.
 *
 * The #FUByteArray is freed unless the reference count of @array is greater
 * than one, the #FUByteArray wrapper is preserved but the size of @array
 * will be set to zero.
 *
 * This is identical to using fu_bytes_new_take() and fu_byte_array_free()
 * together.
 *
 * Since: 2.32
 *
 * Returns: (transfer full): a new immutable #GBytes representing same
 *     byte data that was in the array
 */
FUBytes* fu_byte_array_free_to_bytes(FUByteArray* array)
{
    fu_return_val_if_fail(array, NULL);

    size_t length = array->len;
    void* data = fu_byte_array_free(array, false);
    return fu_bytes_new_take(&data, length);
}

FUBytes* fu_byte_array_to_bytes(FUByteArray* array)
{
    fu_return_val_if_fail(array, NULL);
    if (FU_UNLIKELY(!array->len))
        return NULL;
    TArray* arr = (TArray*)array;
    void* dt = fu_memdup(arr->data, arr->len);
    return fu_bytes_new_take(&dt, arr->len);
}

/**
 * fu_byte_array_ref:
 * @array: A #FUByteArray
 *
 * Atomically increments the reference count of @array by one.
 * This function is thread-safe and may be called from any thread.
 *
 * Returns: (transfer full): The passed in #FUByteArray
 *
 * Since: 2.22
 */
FUByteArray* fu_byte_array_ref(FUByteArray* array)
{
    return (FUByteArray*)fu_array_ref((FUArray*)array);
}

/**
 * fu_byte_array_unref:
 * @array: A #FUByteArray
 *
 * Atomically decrements the reference count of @array by one. If the
 * reference count drops to 0, all memory allocated by the array is
 * released. This function is thread-safe and may be called from any
 * thread.
 *
 * Since: 2.22
 */
void fu_byte_array_unref(FUByteArray* array)
{
    fu_array_unref((FUArray*)array);
}

/**
 * fu_byte_array_append:
 * @array: a #FUByteArray
 * @data: the byte data to be added
 * @len: the number of bytes to add
 *
 * Adds the given bytes to the end of the #FUByteArray.
 * The array will grow in size automatically if necessary.
 *
 * Returns: (transfer none): the #FUByteArray
 */
FUByteArray* fu_byte_array_append(FUByteArray* array, const uint8_t* data, uint32_t len)
{
    return (FUByteArray*)fu_array_append_vals((FUArray*)array, (uint8_t*)data, len);
}

/**
 * fu_byte_array_prepend:
 * @array: a #FUByteArray
 * @data: the byte data to be added
 * @len: the number of bytes to add
 *
 * Adds the given data to the start of the #FUByteArray.
 * The array will grow in size automatically if necessary.
 *
 * Returns: (transfer none): the #FUByteArray
 */
FUByteArray* fu_byte_array_prepend(FUByteArray* array, const uint8_t* data, uint32_t len)
{
    return (FUByteArray*)fu_array_prepend_vals((FUArray*)array, (uint8_t*)data, len);
}

/**
 * fu_byte_array_set_size:
 * @array: a #FUByteArray
 * @length: the new size of the #FUByteArray
 *
 * Sets the size of the #FUByteArray, expanding it if necessary.
 *
 * Returns: (transfer none): the #FUByteArray
 */
FUByteArray* fu_byte_array_set_size(FUByteArray* array, uint32_t length)
{
    return (FUByteArray*)fu_array_set_size((FUArray*)array, length);
}

/**
 * fu_byte_array_remove_index:
 * @array: a #FUByteArray
 * @index_: the index of the byte to remove
 *
 * Removes the byte at the given index from a #FUByteArray.
 * The following bytes are moved down one place.
 *
 * Returns: (transfer none): the #FUByteArray
 **/
FUByteArray* fu_byte_array_remove_index(FUByteArray* array, uint32_t index)
{
    return (FUByteArray*)fu_array_remove_index((FUArray*)array, index);
}

/**
 * fu_byte_array_remove_range:
 * @array: a @FUByteArray
 * @index_: the index of the first byte to remove
 * @length: the number of bytes to remove
 *
 * Removes the given number of bytes starting at the given index from a
 * #FUByteArray.  The following elements are moved to close the gap.
 *
 * Returns: (transfer none): the #FUByteArray
 *
 * Since: 2.4
 */
FUByteArray* fu_byte_array_remove_range(FUByteArray* array, uint32_t index, uint32_t length)
{
    fu_return_val_if_fail(array, NULL);
    fu_return_val_if_fail(index <= array->len, NULL);
    fu_return_val_if_fail(index <= ((uint32_t)-2) - length, NULL);
    fu_return_val_if_fail(index + length <= array->len, NULL);

    return (FUByteArray*)fu_array_remove_range((FUArray*)array, index, length);
}

/**
 * fu_byte_array_sort:
 * @array: a #FUByteArray
 * @compare_func: (scope call): comparison function
 *
 * Sorts a byte array, using @compare_func which should be a
 * qsort()-style comparison function (returns less than zero for first
 * arg is less than second arg, zero for equal, greater than zero if
 * first arg is greater than second arg).
 *
 * If two array elements compare equal, their order in the sorted array
 * is undefined. If you want equal elements to keep their order (i.e.
 * you want a stable sort) you can write a comparison function that,
 * if two elements would otherwise compare equal, compares them by
 * their addresses.
 */
void fu_byte_array_sort(FUByteArray* array, FUCompareFunc compareFunc)
{
    fu_array_sort((FUArray*)array, compareFunc);
}

/**
 * fu_byte_array_sort_with_data:
 * @array: a #FUByteArray
 * @compare_func: (scope call): comparison function
 * @user_data: data to pass to @compare_func
 *
 * Like fu_byte_array_sort(), but the comparison function takes an extra
 * user data argument.
 */
void fu_byte_array_sort_with_data(FUByteArray* array, FUCompareDataFunc compareFunc, void* usd)
{
    fu_array_sort_with_data((FUArray*)array, compareFunc, usd);
}
//
// Pointer Array

/**
 * FUPtrArray:
 * @pdata: points to the array of pointers, which may be moved when the
 *     array grows
 * @len: number of pointers in the array
 *
 * Contains the public fields of a pointer array.
 */
typedef struct _TPtrArray {
    void** pdata;
    uint32_t len;
    uint32_t alloc;
    // gatomicrefcount ref_count;
    // bool nullTerminated : true; /* always either 0 or 1, so it can be added to array lengths */
    FUNotify fnFree;
    atomic_int ref;
} TPtrArray;
static_assert(sizeof(FUPtrArray) == sizeof(TPtrArray));
/**
 * g_ptr_array_index:
 * @array: a #FUPtrArray
 * @index_: the index of the pointer to return
 *
 * Returns the pointer at the given index of the pointer array.
 *
 * This does not perform bounds checking on the given @index_,
 * so you are responsible for checking it against the array length.
 *
 * Returns: the pointer at the given index
 */

static void fu_ptr_array_maybe_expand(TPtrArray* array, uint32_t len)
{
    /* The maximum array length is derived from following constraints:
     * - The number of bytes must fit into a gsize / 2.
     * - The number of elements must fit into uint32_t.
     */
    const uint32_t maxLen = (uint32_t)-2;

    /* Detect potential overflow */
    if (FU_UNLIKELY(maxLen - array->len < len)) {
        fprintf(stderr, "adding %u to array would overflow", len);
        return;
    }
    if ((array->len + len + 1) > array->alloc) {
        size_t alloc = fu_nearest_pow(sizeof(void*) * (array->len + len + 1));
        if (MIN_ARRAY_SIZE > alloc)
            alloc = MIN_ARRAY_SIZE;

        array->alloc = alloc / sizeof(void*);
        if (FU_UNLIKELY(maxLen < array->alloc))
            array->alloc = maxLen;
        array->pdata = fu_realloc(array->pdata, alloc);
        memset(array->pdata + array->len + 1, 0, alloc - sizeof(void*) * (array->len + 1));
    }
}

FUPtrArray* fu_ptr_array_new_full(uint32_t reservedSize, FUNotify elementFreeFunc)
{
    TPtrArray* array = fu_malloc0(sizeof(TPtrArray));
    array->fnFree = elementFreeFunc;

    if (reservedSize) {
        if (FU_LIKELY(reservedSize < (uint32_t)-2))
            reservedSize++;

        fu_ptr_array_maybe_expand(array, reservedSize);
        assert(array->pdata != NULL);
    }
    atomic_init(&array->ref, 1);
    return (FUPtrArray*)array;
}

/**
 * g_ptr_array_new_take: (skip)
 * @data: (array length=len) (transfer full) (nullable): an array of pointers,
 *    or %NULL for an empty array
 * @len: the number of pointers in @data
 * @element_free_func: (nullable): A function to free elements on @array
 *   destruction or %NULL
 *
 * Creates a new #FUPtrArray with @data as pointers, @len as length and a
 * reference count of 1.
 *
 * This avoids having to copy such data manually.
 * After this call, @data belongs to the #FUPtrArray and may no longer be
 * modified by the caller. The memory of @data has to be dynamically
 * allocated and will eventually be freed with g_free().
 *
 * It also sets @element_free_func for freeing each element when the array is
 * destroyed either via g_ptr_array_unref(), when g_ptr_array_free() is called
 * with @free_segment set to %TRUE or when removing elements.
 *
 * Do not use it if @len is greater than %G_MAXUINT. #FUPtrArray
 * stores the length of its data in #uint32_t, which may be shorter than
 * #gsize.
 *
 * Returns: (transfer full): A new #FUPtrArray
 *
 * Since: 2.76
 */
FUPtrArray* fu_ptr_array_new_take(void** data, uint32_t len, FUNotify elementFreeFunc)
{
    fu_return_val_if_fail(data && len && (uint32_t)-2 > len, NULL);

    FUPtrArray* rev;
    TPtrArray* arr = (TPtrArray*)(rev = fu_ptr_array_new_full(0, elementFreeFunc));

    arr->pdata = fu_steal_pointer(&data);
    arr->len = len;
    arr->alloc = len;

    return rev;
}
/**
 * g_ptr_array_new_from_array: (skip)
 * @data: (array length=len) (transfer none) (nullable): an array of pointers,
 * or %NULL for an empty array
 * @len: the number of pointers in @data
 * @copy_func: (nullable): a copy function used to copy every element in the
 *   array or %NULL.
 * @copy_func_user_data: user data passed to @copy_func, or %NULL
 * @element_free_func: (nullable): a function to free elements on @array
 *   destruction or %NULL
 *
 * Creates a new #FUPtrArray, copying @len pointers from @data, and setting
 * the array’s reference count to 1.
 *
 * This avoids having to manually add each element one by one.
 *
 * If @copy_func is provided, then it is used to copy each element before
 * adding them to the new array. If it is %NULL then the pointers are copied
 * directly.
 *
 * It also sets @element_free_func for freeing each element when the array is
 * destroyed either via g_ptr_array_unref(), when g_ptr_array_free() is called
 * with @free_segment set to %TRUE or when removing elements.
 *
 * Do not use it if @len is greater than %G_MAXUINT. #FUPtrArray
 * stores the length of its data in #uint32_t, which may be shorter than
 * #gsize.
 *
 * Returns: (transfer full): A new #FUPtrArray
 *
 * Since: 2.76
 */
FUPtrArray* fu_ptr_array_new_from_array(void** data, uint32_t len, FUCopyFunc copyFunc, void* copyFuncUserData, FUNotify elementFreeFunc)
{
    fu_return_val_if_fail(data && len && (uint32_t)-2 > len, NULL);

    FUPtrArray* rev;
    TPtrArray* arr = (TPtrArray*)(rev = fu_ptr_array_new_full(len, elementFreeFunc));

    if (copyFunc) {
        for (uint32_t i = 0; i < len; i++)
            arr->pdata[i] = copyFunc(data[i], copyFuncUserData);
    } else if (len) {
        memcpy(arr->pdata, data, len * sizeof(void*));
    }

    arr->len = len;
    return rev;
}
/**
 * g_ptr_array_new:
 *
 * Creates a new #FUPtrArray with a reference count of 1.
 *
 * Returns: the new #FUPtrArray
 */
FUPtrArray* fu_ptr_array_new(void)
{
    return fu_ptr_array_new_full(0, NULL);
}

/**
 * g_ptr_array_steal:
 * @array: a #FUPtrArray.
 * @len: (optional) (out): pointer to retrieve the number of
 *    elements of the original array
 *
 * Frees the data in the array and resets the size to zero, while
 * the underlying array is preserved for use elsewhere and returned
 * to the caller.
 *
 * Note that if the array is %NULL terminated this may still return
 * %NULL if the length of the array was zero and pdata was not yet
 * allocated.
 *
 * Even if set, the #FUNotify function will never be called
 * on the current contents of the array and the caller is
 * responsible for freeing the array elements.
 *
 * An example of use:
 * |[<!-- language="C" -->
 * g_autoptr(FUPtrArray) chunk_buffer = g_ptr_array_new_with_free_func (fu_bytes_unref);
 *
 * // Some part of your application appends a number of chunks to the pointer array.
 * g_ptr_array_add (chunk_buffer, fu_bytes_new_static ("hello", 5));
 * g_ptr_array_add (chunk_buffer, fu_bytes_new_static ("world", 5));
 *
 * …
 *
 * // Periodically, the chunks need to be sent as an array-and-length to some
 * // other part of the program.
 * GBytes **chunks;
 * gsize n_chunks;
 *
 * chunks = g_ptr_array_steal (chunk_buffer, &n_chunks);
 * for (gsize i = 0; i < n_chunks; i++)
 *   {
 *     // Do something with each chunk here, and then free them, since
 *     // g_ptr_array_steal() transfers ownership of all the elements and the
 *     // array to the caller.
 *     …
 *
 *     fu_bytes_unref (chunks[i]);
 *   }
 *
 * g_free (chunks);
 *
 * // After calling g_ptr_array_steal(), the pointer array can be reused for the
 * // next set of chunks.
 * g_assert (chunk_buffer->len == 0);
 * ]|
 *
 * Returns: (transfer full) (nullable): the element data, which should be
 *     freed using g_free(). This may be %NULL if the array doesn’t have any
 *     elements (i.e. if `*len` is zero).
 *
 * Since: 2.64
 */
void** fu_ptr_array_steal(FUPtrArray* array, uint32_t* len)
{
    fu_return_val_if_fail(array, NULL);

    TPtrArray* arr = (TPtrArray*)array;
    void** segment = (void**)arr->pdata;

    if (len != NULL)
        *len = arr->len;

    arr->pdata = NULL;
    arr->len = arr->alloc = 0;
    return segment;
}

/**
 * g_ptr_array_copy:
 * @array: #FUPtrArray to duplicate
 * @func: (nullable): a copy function used to copy every element in the array
 * @user_data: user data passed to the copy function @func, or %NULL
 *
 * Makes a full (deep) copy of a #FUPtrArray.
 *
 * @func, as a #GCopyFunc, takes two arguments, the data to be copied
 * and a @user_data pointer. On common processor architectures, it's safe to
 * pass %NULL as @user_data if the copy function takes only one argument. You
 * may get compiler warnings from this though if compiling with GCC’s
 * `-Wcast-function-type` warning.
 *
 * If @func is %NULL, then only the pointers (and not what they are
 * pointing to) are copied to the new #FUPtrArray.
 *
 * The copy of @array will have the same #FUNotify for its elements as
 * @array. The copy will also be %NULL terminated if (and only if) the source
 * array is.
 *
 * Returns: (transfer full): a deep copy of the initial #FUPtrArray.
 *
 * Since: 2.62
 **/
FUPtrArray* fu_ptr_array_copy(FUPtrArray* array, FUCopyFunc func, void* userData)
{
    fu_return_val_if_fail(array, NULL);
    FUPtrArray* rev;
    TPtrArray *arr1 = (TPtrArray*)array, *arr2 = (TPtrArray*)(rev = fu_ptr_array_new_full(0, arr1->fnFree));

    if (0 < arr1->alloc) {
        fu_ptr_array_maybe_expand(arr2, arr1->len);

        if (0 < arr1->len) {
            if (func) {
                for (uint32_t i = 0; i < arr1->len; i++)
                    arr2->pdata[i] = func(arr1->pdata[i], userData);
            } else
                memcpy(arr2->pdata, arr1->pdata, arr1->len * sizeof(*arr1->pdata));

            arr2->len = arr1->len;
        }
    }

    return rev;
}

/**
 * g_ptr_array_ref:
 * @array: a #FUPtrArray
 *
 * Atomically increments the reference count of @array by one.
 * This function is thread-safe and may be called from any thread.
 *
 * Returns: The passed in #FUPtrArray
 *
 * Since: 2.22
 */
FUPtrArray* fu_ptr_array_ref(FUPtrArray* array)
{
    fu_return_val_if_fail(array, NULL);
    // ((TPtrArray*)array)->ref += 1;
    atomic_fetch_add(&((TPtrArray*)array)->ref, 1);
    return array;
}

/**
 * g_ptr_array_free:
 * @array: a #FUPtrArray
 * @free_seg: if %TRUE the actual pointer array is freed as well
 *
 * Frees the memory allocated for the #FUPtrArray. If @free_seg is %TRUE
 * it frees the memory block holding the elements as well. Pass %FALSE
 * if you want to free the #FUPtrArray wrapper but preserve the
 * underlying array for use elsewhere. If the reference count of @array
 * is greater than one, the #FUPtrArray wrapper is preserved but the
 * size of @array will be set to zero.
 *
 * If array contents point to dynamically-allocated memory, they should
 * be freed separately if @free_seg is %TRUE and no #FUNotify
 * function has been set for @array.
 *
 * Note that if the array is %NULL terminated and @free_seg is %FALSE
 * then this will always return an allocated %NULL terminated buffer.
 * If pdata is previously %NULL, a new buffer will be allocated.
 *
 * This function is not thread-safe. If using a #FUPtrArray from multiple
 * threads, use only the atomic g_ptr_array_ref() and g_ptr_array_unref()
 * functions.
 *
 * Returns: (transfer full) (nullable): the pointer array if @free_seg is
 *     %FALSE, otherwise %NULL. The pointer array should be freed using g_free().
 */
void** fu_ptr_array_free(FUPtrArray* array, bool freeSegment)
{
    fu_return_val_if_fail(array, NULL);
    TPtrArray* arr = (TPtrArray*)array;
    void** rev = NULL;
    if (freeSegment) {
        if (arr->fnFree) {
            for (uint32_t i = 0; i < arr->len; i++)
                arr->fnFree(arr->pdata[i]);
        }
        fu_free(arr->pdata);
    } else
        rev = fu_steal_pointer(&arr->pdata);

    fu_free(arr);
    return rev;
}
/**
 * g_ptr_array_unref:
 * @array: A #FUPtrArray
 *
 * Atomically decrements the reference count of @array by one. If the
 * reference count drops to 0, the effect is the same as calling
 * g_ptr_array_free() with @free_segment set to %TRUE. This function
 * is thread-safe and may be called from any thread.
 *
 * Since: 2.22
 */
void fu_ptr_array_unref(FUPtrArray* array)
{
    fu_return_if_fail(array);
    TPtrArray* arr = (TPtrArray*)array;
    // arr->ref -= 1;
    if (1 >= atomic_fetch_sub(&arr->ref, 1))
        fu_ptr_array_free(array, true);
}

/**
 * g_ptr_array_set_free_func:
 * @array: A #FUPtrArray
 * @element_free_func: (nullable): A function to free elements with
 *     destroy @array or %NULL
 *
 * Sets a function for freeing each element when @array is destroyed
 * either via g_ptr_array_unref(), when g_ptr_array_free() is called
 * with @free_segment set to %TRUE or when removing elements.
 *
 * Since: 2.22
 */
void fu_ptr_array_set_free_func(FUPtrArray* array, FUNotify elementFreeFunc)
{
    fu_return_if_fail(array);
    ((TPtrArray*)array)->fnFree = elementFreeFunc;
}

/**
 * g_ptr_array_remove_index:
 * @array: a #FUPtrArray
 * @index_: the index of the pointer to remove
 *
 * Removes the pointer at the given index from the pointer array.
 * The following elements are moved down one place. If @array has
 * a non-%NULL #FUNotify function it is called for the removed
 * element. If so, the return value from this function will potentially point
 * to freed memory (depending on the #FUNotify implementation).
 *
 * Returns: (nullable): the pointer which was removed
 */
void* fu_ptr_array_remove_index(FUPtrArray* array, uint32_t idx)
{
    fu_return_val_if_fail(array && array->pdata && idx < array->len, NULL);
    TPtrArray* arr = (TPtrArray*)array;
    void* rev = arr->pdata[idx];

    if (arr->fnFree)
        arr->fnFree(arr->pdata[idx]);
    if (idx < arr->len - 1)
        memmove(arr->pdata + idx, arr->pdata + idx + 1, sizeof(void*) * (arr->len - idx - 1));

    arr->len -= 1;
    arr->pdata[arr->len] = NULL;

    return rev;
}

void fu_ptr_array_remove_all(FUPtrArray* array, FUNotify elementFreeFunc)
{
    fu_return_if_fail(array && array->pdata);
    TPtrArray* arr = (TPtrArray*)array;
    if (!elementFreeFunc)
        elementFreeFunc = arr->fnFree;
    if (!elementFreeFunc) {
        fprintf(stderr, "You have to provide a element free function!\n");
        return;
    }
    for (uint32_t i = 0; i < arr->len; i++)
        elementFreeFunc(arr->pdata[i]);

    memset(arr->pdata, 0, arr->alloc * sizeof(void*));
    arr->len = 0;
}

/**
 * g_ptr_array_steal_index:
 * @array: a #FUPtrArray
 * @index_: the index of the pointer to steal
 *
 * Removes the pointer at the given index from the pointer array.
 * The following elements are moved down one place. The #FUNotify for
 * @array is *not* called on the removed element; ownership is transferred to
 * the caller of this function.
 *
 * Returns: (transfer full) (nullable): the pointer which was removed
 * Since: 2.58
 */
void* fu_ptr_array_steal_index(FUPtrArray* array, uint32_t idx)
{
    fu_return_val_if_fail(array && array->pdata && idx < array->len, NULL);
    TPtrArray* arr = (TPtrArray*)array;
    void* rev = arr->pdata[idx];

    if (idx < arr->len - 1)
        memmove(arr->pdata + idx, arr->pdata + idx + 1, sizeof(void*) * (arr->len - idx - 1));

    arr->len -= 1;
    arr->pdata[arr->len] = NULL;

    return rev;
}

void** fu_ptr_array_steal_all(FUPtrArray* array)
{
    fu_return_val_if_fail(array && array->pdata, NULL);
    TPtrArray* arr = (TPtrArray*)array;
    void** rev = fu_steal_pointer(&arr->pdata);
    arr->alloc = arr->len = 0;
    fu_ptr_array_unref(array);
    // fu_ptr_array_maybe_expand(arr, 2u);
    return rev;
}

/**
 * g_ptr_array_add:
 * @array: a #FUPtrArray
 * @data: the pointer to add
 *
 * Adds a pointer to the end of the pointer array. The array will grow
 * in size automatically if necessary.
 */
void fu_ptr_array_push(FUPtrArray* array, void* data)
{
    fu_return_if_fail(array && array->pdata);
    TPtrArray* arr = (TPtrArray*)array;

    fu_ptr_array_maybe_expand(arr, 2u);

    arr->pdata[arr->len++] = data;
}

/**
 * g_ptr_array_extend:
 * @array_to_extend: a #FUPtrArray.
 * @array: (transfer none): a #FUPtrArray to add to the end of @array_to_extend.
 * @func: (nullable): a copy function used to copy every element in the array
 * @user_data: user data passed to the copy function @func, or %NULL
 *
 * Adds all pointers of @array to the end of the array @array_to_extend.
 * The array will grow in size automatically if needed. @array_to_extend is
 * modified in-place.
 *
 * @func, as a #GCopyFunc, takes two arguments, the data to be copied
 * and a @user_data pointer. On common processor architectures, it's safe to
 * pass %NULL as @user_data if the copy function takes only one argument. You
 * may get compiler warnings from this though if compiling with GCC’s
 * `-Wcast-function-type` warning.
 *
 * If @func is %NULL, then only the pointers (and not what they are
 * pointing to) are copied to the new #FUPtrArray.
 *
 * Whether @array_to_extend is %NULL terminated stays unchanged by this function.
 *
 * Since: 2.62
 **/
void fu_ptr_array_extend(FUPtrArray* arrayToExtend, FUPtrArray* array, FUCopyFunc func, void* usd)
{
    fu_return_if_fail(arrayToExtend && array);
    if (!array->len)
        return;
    if (FU_UNLIKELY((uint32_t)-2 < arrayToExtend->len + array->len)) {
        fprintf(stderr, "adding %u to array would overflow", array->len);
        return;
    }
    TPtrArray* arr = (TPtrArray*)arrayToExtend;

    fu_ptr_array_maybe_expand(arr, array->len);

    if (func != NULL) {
        for (uint32_t i = 0; i < array->len; i++)
            arr->pdata[i + arr->len] = func(array->pdata[i], usd);
    } else if (array->len)
        memcpy(arr->pdata + arr->len, array->pdata, array->len * sizeof(*array->pdata));

    arr->len += array->len;
}

/**
 * g_ptr_array_extend_and_steal:
 * @array_to_extend: (transfer none): a #FUPtrArray.
 * @array: (transfer container): a #FUPtrArray to add to the end of
 *     @array_to_extend.
 *
 * Adds all the pointers in @array to the end of @array_to_extend, transferring
 * ownership of each element from @array to @array_to_extend and modifying
 * @array_to_extend in-place. @array is then freed.
 *
 * As with g_ptr_array_free(), @array will be destroyed if its reference count
 * is 1. If its reference count is higher, it will be decremented and the
 * length of @array set to zero.
 *
 * Since: 2.62
 **/
void fu_ptr_array_extend_and_steal(FUPtrArray* arrayToExtend, FUPtrArray* array)
{
    fu_return_if_fail(arrayToExtend && array);
    if (FU_UNLIKELY((uint32_t)-2 < arrayToExtend->len + array->len)) {
        fprintf(stderr, "adding %u to array would overflow", array->len);
        return;
    }

    fu_ptr_array_extend(arrayToExtend, array, NULL, NULL);

    /* Get rid of @array without triggering the FUNotify attached
     * to the elements moved from @array to @array_to_extend. */
    void** dt = fu_steal_pointer(&array->pdata);
    array->len = 0;
    ((TPtrArray*)array)->alloc = 0;
    fu_ptr_array_unref(array);
    fu_free(dt);
}

/**
 * g_ptr_array_insert:
 * @array: a #FUPtrArray
 * @index_: the index to place the new element at
 * @data: the pointer to add.
 *
 * Inserts an element into the pointer array at the given index. The
 * array will grow in size automatically if necessary.
 *
 * Since: 2.40
 */
void fu_ptr_array_insert(FUPtrArray* array, const uint32_t idx, void* data)
{
    fu_return_if_fail(array);
    if (idx >= array->len) {
        fu_ptr_array_push(array, data);
        return;
    }
    TPtrArray* arr = (TPtrArray*)array;

    fu_ptr_array_maybe_expand(arr, 1);
    memmove(&(arr->pdata[idx + 1]), &(arr->pdata[idx]), (arr->len - idx) * sizeof(void*));

    arr->len++;
    arr->pdata[idx] = data;
}

/**
 * g_ptr_array_foreach:
 * @array: a #FUPtrArray
 * @func: the function to call for each array element
 * @user_data: user data to pass to the function
 *
 * Calls a function for each element of a #FUPtrArray. @func must not
 * add elements to or remove elements from the array.
 *
 * Since: 2.4
 */
void fu_ptr_array_foreach(FUPtrArray* array, FUFunc func, void* usd)
{
    fu_return_if_fail(array);

    for (uint32_t i = 0; i < array->len; i++)
        (*func)(array->pdata[i], usd);
}

/**
 * g_ptr_array_find_with_equal_func: (skip)
 * @haystack: pointer array to be searched
 * @needle: pointer to look for
 * @equal_func: (nullable): the function to call for each element, which should
 *    return %TRUE when the desired element is found; or %NULL to use pointer
 *    equality
 * @index_: (optional) (out): return location for the index of
 *    the element, if found
 *
 * Checks whether @needle exists in @haystack, using the given @equal_func.
 * If the element is found, %TRUE is returned and the element’s index is
 * returned in @index_ (if non-%NULL). Otherwise, %FALSE is returned and @index_
 * is undefined. If @needle exists multiple times in @haystack, the index of
 * the first instance is returned.
 *
 * @equal_func is called with the element from the array as its first parameter,
 * and @needle as its second parameter. If @equal_func is %NULL, pointer
 * equality is used.
 *
 * Returns: %TRUE if @needle is one of the elements of @haystack
 * Since: 2.54
 */
bool fu_ptr_array_find_with_equal_func(FUPtrArray* haystack, const void* needle, FUEqualFunc equalFunc, uint32_t* idx)
{
    fu_return_val_if_fail(haystack != NULL, false);

    if (equalFunc == NULL)
        equalFunc = fu_direct_equal;

    for (uint32_t i = 0; i < haystack->len; i++) {
        if (equalFunc(fu_ptr_array_index(haystack, i), needle)) {
            if (idx)
                *idx = i;
            return true;
        }
    }

    return false;
}

/**
 * g_ptr_array_find: (skip)
 * @haystack: pointer array to be searched
 * @needle: pointer to look for
 * @index_: (optional) (out): return location for the index of
 *    the element, if found
 *
 * Checks whether @needle exists in @haystack. If the element is found, %TRUE is
 * returned and the element’s index is returned in @index_ (if non-%NULL).
 * Otherwise, %FALSE is returned and @index_ is undefined. If @needle exists
 * multiple times in @haystack, the index of the first instance is returned.
 *
 * This does pointer comparisons only. If you want to use more complex equality
 * checks, such as string comparisons, use g_ptr_array_find_with_equal_func().
 *
 * Returns: %TRUE if @needle is one of the elements of @haystack
 * Since: 2.54
 */
bool fu_ptr_array_find(FUPtrArray* haystack, const void* needle, uint32_t* idx)
{
    return fu_ptr_array_find_with_equal_func(haystack, needle, NULL, idx);
}
