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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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
#ifndef __G_ARRAY_H__
#define __G_ARRAY_H__

#include <stdint.h>

#include "types.h"

typedef struct _FUBytes FUBytes;
typedef struct _FUArray {
    uint8_t* data;
    uint32_t len;
    void* dummy[1];
} FUArray;

typedef struct _FUByteArray {
    uint8_t* data;
    uint32_t len;
} FUByteArray;

FUBytes* fu_bytes_new_full(const void* data, size_t size, FUNotify fnFree, void* usd);
FUBytes* fu_bytes_new_take(void** data, size_t size);
FUBytes* fu_bytes_new_static(const void* data, size_t size);
FUBytes* fu_bytes_new_from_bytes(FUBytes* bytes, size_t offset, size_t length);

FUBytes* fu_bytes_ref(FUBytes* bytes);
void fu_bytes_unref(FUBytes* bytes);
void* fu_bytes_unref_to_data(FUBytes* bytes, size_t* size);
FUByteArray* fu_bytes_unref_to_array(FUBytes* bytes);

const void* fu_bytes_get_data(FUBytes* bytes, size_t* size);
size_t fu_bytes_get_size(FUBytes* bytes);
uint64_t fu_bytes_hash(const FUBytes* bytes);
bool fu_bytes_equal(const FUBytes* bytes1, const FUBytes* bytes2);

const void* fu_bytes_get_region(FUBytes* bytes, size_t elementSize, size_t offset, size_t n_elements);

/* Resizable arrays. remove fills any cleared spot and shortens the
 * array, while preserving the order. remove_fast will distort the
 * order by moving the last element to the position of the removed.
 */

#define fu_array_append_val(a, v) fu_array_append_vals(a, &(v), 1)
#define fu_array_prepend_val(a, v) fu_array_prepend_vals(a, &(v), 1)
#define fu_array_insert_val(a, i, v) fu_array_insert_vals(a, i, &(v), 1)
#define fu_array_index(a, t, i) (((t*)(void*)(a)->data)[(i)])

FUArray* fu_array_new_full(uint32_t elementSize, uint32_t reservedSize);
FUArray* fu_array_new_take(void* data, uint32_t len, uint32_t elementSize);
FUArray* fu_array_new(uint32_t elementSize);

FUArray* fu_array_ref(FUArray* array);
void fu_array_unref(FUArray* array);
uint8_t* fu_array_free(FUArray* array, bool freeSegment);

void* fu_array_steal(FUArray* array, uint32_t* len);
FUArray* fu_array_copy(FUArray* array);
uint32_t fu_array_get_element_size(FUArray* array);

FUArray* fu_array_append_vals(FUArray* array, const void* data, uint32_t len);
FUArray* fu_array_prepend_vals(FUArray* array, const void* data, uint32_t len);
FUArray* fu_array_insert_vals(FUArray* array, uint32_t index_, const void* data, uint32_t len);
FUArray* fu_array_set_size(FUArray* array, uint32_t length);

FUArray* fu_array_remove_index(FUArray* array, uint32_t idx);
FUArray* fu_array_remove_range(FUArray* array, uint32_t idx, uint32_t length);
void fu_array_remove_all(FUArray* array);

void fu_array_sort(FUArray* array, FUCompareFunc compareFunc);
void fu_array_sort_with_data(FUArray* array, FUCompareDataFunc compareFunc, void* usd);

bool fu_array_binary_search(FUArray* array, const void* target, FUCompareFunc compareFunc, uint32_t* outMatchIndex);

/* Byte arrays, an array of uint32_t8.  Implemented as a FUArray,
 * but type-safe.
 */

FUByteArray* fu_byte_array_new(void);
FUByteArray* fu_byte_array_sized_new(uint32_t reservedSize);
FUByteArray* fu_byte_array_new_take(uint8_t** data, uint32_t len);
uint8_t* fu_byte_array_steal(FUByteArray* array, uint32_t* len);
uint8_t* fu_byte_array_free(FUByteArray* array, bool freeSegment);
FUBytes* fu_byte_array_free_to_bytes(FUByteArray* array);

FUByteArray* fu_byte_array_ref(FUByteArray* array);
void fu_byte_array_unref(FUByteArray* array);

FUByteArray* fu_byte_array_append(FUByteArray* array, const uint8_t* data, uint32_t len);
FUByteArray* fu_byte_array_prepend(FUByteArray* array, const uint8_t* data, uint32_t len);
FUByteArray* fu_byte_array_set_size(FUByteArray* array, uint32_t length);
FUByteArray* fu_byte_array_remove_index(FUByteArray* array, uint32_t index);
FUByteArray* fu_byte_array_remove_range(FUByteArray* array, uint32_t index, uint32_t length);
void fu_byte_array_sort(FUByteArray* array, FUCompareFunc compareFunc);
void fu_byte_array_sort_with_data(FUByteArray* array, FUCompareDataFunc compareFunc, void* usd);

/* Resizable pointer array.  This interface is much less complicated
 * than the above.  Add appends a pointer.  Remove fills any cleared
 * spot and shortens the array. remove_fast will again distort order.
 */
typedef struct _FUPtrArray {
    void** pdata;
    uint32_t len;
    void* dummy[2];
} FUPtrArray;

#define fu_ptr_array_index(arr, idx) ((arr)->pdata)[idx]

FUPtrArray* fu_ptr_array_new_full(uint32_t reservedSize, FUNotify elementFreeFunc);
FUPtrArray* fu_ptr_array_new_take(void** data, uint32_t len, FUNotify elementFreeFunc);
FUPtrArray* fu_ptr_array_new_from_array(void** data, uint32_t len, FUCopyFunc copyFunc, void* copyFuncUserData, FUNotify elementFreeFunc);
FUPtrArray* fu_ptr_array_new(void);

void** fu_ptr_array_steal(FUPtrArray* array, uint32_t* len);
FUPtrArray* fu_ptr_array_copy(FUPtrArray* array, FUCopyFunc func, void* userData);

FUPtrArray* fu_ptr_array_ref(FUPtrArray* array);
void fu_ptr_array_unref(FUPtrArray* array);
void** fu_ptr_array_free(FUPtrArray* array, bool freeSeg);

void fu_ptr_array_set_free_func(FUPtrArray* array, FUNotify elementFreeFunc);
void* fu_ptr_array_remove_index(FUPtrArray* array, uint32_t idx);
void fu_ptr_array_remove_all(FUPtrArray* array, FUNotify elementFreeFunc);
void* fu_ptr_array_steal_index(FUPtrArray* array, uint32_t idx);
void** fu_ptr_array_steal_all(FUPtrArray* array);

void fu_ptr_array_push(FUPtrArray* array, void* data);
void fu_ptr_array_extend(FUPtrArray* arrayToExtend, FUPtrArray* array, FUCopyFunc func, void* usd);
void fu_ptr_array_extend_and_steal(FUPtrArray* arrayToExtend, FUPtrArray* array);

void fu_ptr_array_insert(FUPtrArray* array, const uint32_t idx, void* data);
void fu_ptr_array_foreach(FUPtrArray* array, FUFunc func, void* usd);

bool fu_ptr_array_find(FUPtrArray* haystack, const void* needle, uint32_t* idx);

bool fu_ptr_array_find_with_equal_func(FUPtrArray* haystack, const void* needle, FUEqualFunc equalFunc, uint32_t* idx);

#endif /* __G_ARRAY_H__ */
