/*
 * Copyright © 2011 Ryan Lortie
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */
#ifndef _FU_ATOMIC_H__
#define _FU_ATOMIC_H__

#include <stdint.h>
// #define FU_USE_BUILTIN_ATOMIC
/* We prefer the new C11-style atomic extension of GCC if available */
#ifndef FU_DISABLED_ATOMIC_LOCK_FREE
#ifdef FU_USE_BUILTIN_ATOMIC
typedef int atomic_int;
typedef uint32_t atomic_uint;
int fu_atomic_int_get(const volatile int* atomic);
void fu_atomic_int_set(volatile int* atomic, int newval);
void fu_atomic_int_inc(volatile int* atomic);
int fu_atomic_int_add(volatile int* atomic, int val);
uint32_t fu_atomic_int_and(volatile uint32_t* atomic, uint32_t val);
uint32_t fu_atomic_int_or(volatile uint32_t* atomic, uint32_t val);
uint32_t fu_atomic_int_xor(volatile uint32_t* atomic, uint32_t val);
int fu_atomic_int_exchange(int* atomic, int newval);
bool fu_atomic_int_compare_and_exchange(volatile int* atomic, int oldval, int newval);
bool fu_atomic_int_dec_and_test(volatile int* atomic);
bool fu_atomic_int_compare_and_exchange_full(int* atomic, int oldval, int newval, int* preval);

void* fu_atomic_pointer_get(const volatile void* atomic);
void fu_atomic_pointer_set(volatile void* atomic, void* newval);
intptr_t fu_atomic_pointer_add(volatile void* atomic, ssize_t val);
uintptr_t fu_atomic_pointer_and(volatile void* atomic, size_t val);
uintptr_t fu_atomic_pointer_or(volatile void* atomic, size_t val);
uintptr_t fu_atomic_pointer_xor(volatile void* atomic, size_t val);
void* fu_atomic_pointer_exchange(void* atomic, void* newval);
bool fu_atomic_pointer_compare_and_exchange(volatile void* atomic, void* oldval, void* newval);
bool fu_atomic_pointer_compare_and_exchange_full(void* atomic, void* oldval, void* newval, void* preval);

#define fu_atomic_int_get(atomic)                              \
    (__extension__({                                           \
        static_assert(sizeof *(atomic) == sizeof(int));        \
        int tmp;                                               \
        (void)(0 ? *(atomic) ^ *(atomic) : 1);                 \
        __atomic_load((int*)(atomic), &tmp, __ATOMIC_SEQ_CST); \
        (int)tmp;                                              \
    }))
#define fu_atomic_int_set(atomic, newval)                       \
    (__extension__({                                            \
        static_assert(sizeof *(atomic) == sizeof(int));         \
        int tmp = (int)(newval);                                \
        (void)(0 ? *(atomic) ^ (newval) : 1);                   \
        __atomic_store((int*)(atomic), &tmp, __ATOMIC_SEQ_CST); \
    }))

#define fu_atomic_int_inc(atomic)                                \
    (__extension__({                                             \
        static_assert(sizeof *(atomic) == sizeof(int));          \
        (void)(0 ? *(atomic) ^ *(atomic) : 1);                   \
        (void)__atomic_fetch_add((atomic), 1, __ATOMIC_SEQ_CST); \
    }))

#define fu_atomic_int_add(atomic, val)                              \
    (__extension__({                                                \
        static_assert(sizeof *(atomic) == sizeof(int));             \
        (void)(0 ? *(atomic) ^ (val) : 1);                          \
        (int)__atomic_fetch_add((atomic), (val), __ATOMIC_SEQ_CST); \
    }))

#define fu_atomic_int_and(atomic, val)                                    \
    (__extension__({                                                      \
        static_assert(sizeof *(atomic) == sizeof(int));                   \
        (void)(0 ? *(atomic) ^ (val) : 1);                                \
        (uint32_t) __atomic_fetch_and((atomic), (val), __ATOMIC_SEQ_CST); \
    }))
#define fu_atomic_int_or(atomic, val)                                    \
    (__extension__({                                                     \
        static_assert(sizeof *(atomic) == sizeof(int));                  \
        (void)(0 ? *(atomic) ^ (val) : 1);                               \
        (uint32_t) __atomic_fetch_or((atomic), (val), __ATOMIC_SEQ_CST); \
    }))
#define fu_atomic_int_xor(atomic, val)                                    \
    (__extension__({                                                      \
        static_assert(sizeof *(atomic) == sizeof(int));                   \
        (void)(0 ? *(atomic) ^ (val) : 1);                                \
        (uint32_t) __atomic_fetch_xor((atomic), (val), __ATOMIC_SEQ_CST); \
    }))

#define fu_atomic_int_exchange(atomic, newval)                          \
    (__extension__({                                                    \
        static_assert(sizeof *(atomic) == sizeof(int));                 \
        (void)(0 ? *(atomic) ^ (newval) : 1);                           \
        (int)__atomic_exchange_n((atomic), (newval), __ATOMIC_SEQ_CST); \
    }))

#define fu_atomic_int_dec_and_test(atomic)                      \
    (__extension__({                                            \
        static_assert(sizeof *(atomic) == sizeof(int));         \
        (void)(0 ? *(atomic) ^ *(atomic) : 1);                  \
        __atomic_fetch_sub((atomic), 1, __ATOMIC_SEQ_CST) == 1; \
    }))

#define fu_atomic_int_compare_and_exchange(atomic, oldval, newval)                                                                        \
    (__extension__({                                                                                                                      \
        int tmpOldval = (oldval);                                                                                                         \
        static_assert(sizeof *(atomic) == sizeof(int));                                                                                   \
        (void)(0 ? *(atomic) ^ (newval) ^ (oldval) : 1);                                                                                  \
        __atomic_compare_exchange_n((atomic), (void*)(&(tmpOldval)), (newval), false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ? true : false; \
    }))

#define fu_atomic_int_compare_and_exchange_full(atomic, oldval, newval, preval) \
    (__extension__({                                                            \
        static_assert(sizeof *(atomic) == sizeof(int));                         \
        static_assert(sizeof *(preval) == sizeof(int));                         \
        (void)(0 ? *(atomic) ^ (newval) ^ (oldval) ^ *(preval) : 1);            \
        *(preval) = (oldval);                                                   \
        __atomic_compare_exchange_n((atomic), (preval), (newval), false,        \
            __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)                                 \
            ? true                                                              \
            : false;                                                            \
    }))
/* See comments below about equivalent fu_atomic_pointer_compare_and_exchange()
 * shenanigans for type-safety when compiling in C++ mode. */
/* This is typesafe because we check we can assign oldval to the type of
 * (*atomic). Unfortunately it can only be done in C++ because gcc/clang warn
 * when atomic is volatile and not oldval, or when atomic is gsize* and oldval
 * is NULL. Note that clang++ force us to be typesafe because it is an error if the 2nd
 * argument of __atomic_compare_exchange_n() has a different type than the
 * first.
 * https://gitlab.gnome.org/GNOME/glib/-/merge_requests/1919
 * https://gitlab.gnome.org/GNOME/glib/-/merge_requests/1715#note_1024120. */
#define fu_atomic_pointer_get(atomic)                        \
    (__extension__({                                         \
        static_assert(sizeof *(atomic) == sizeof(void*));    \
        typeof(*(atomic)) tmpVal;                            \
        typeof((atomic)) tmpAtomic = (atomic);               \
        __atomic_load(tmpAtomic, &tmpVal, __ATOMIC_SEQ_CST); \
        tmpVal;                                              \
    }))
#define fu_atomic_pointer_set(atomic, newval)                 \
    (__extension__({                                          \
        static_assert(sizeof *(atomic) == sizeof(void*));     \
        typeof((atomic)) tmpAtomic = (atomic);                \
        typeof(*(atomic)) tmpVal = (newval);                  \
        (void)(0 ? (void*)*(atomic) : NULL);                  \
        __atomic_store(tmpAtomic, &tmpVal, __ATOMIC_SEQ_CST); \
    }))

#define fu_atomic_pointer_add(atomic, val)                                \
    (__extension__({                                                      \
        static_assert(sizeof *(atomic) == sizeof(void*));                 \
        (void)(0 ? (void*)*(atomic) : NULL);                              \
        (void)(0 ? (val) ^ (val) : 1);                                    \
        (intptr_t) __atomic_fetch_add((atomic), (val), __ATOMIC_SEQ_CST); \
    }))
#define fu_atomic_pointer_and(atomic, val)                                  \
    (__extension__({                                                        \
        uintptr_t* tmpAtomic = (uintptr_t*)(atomic);                        \
        static_assert(sizeof *(atomic) == sizeof(void*));                   \
        static_assert(sizeof *(atomic) == sizeof(uintptr_t));               \
        (void)(0 ? (void*)*(atomic) : NULL);                                \
        (void)(0 ? (val) ^ (val) : 1);                                      \
        (uintptr_t) __atomic_fetch_and(tmpAtomic, (val), __ATOMIC_SEQ_CST); \
    }))
#define fu_atomic_pointer_or(atomic, val)                                  \
    (__extension__({                                                       \
        uintptr_t* tmpAtomic = (uintptr_t*)(atomic);                       \
        static_assert(sizeof *(atomic) == sizeof(void*));                  \
        static_assert(sizeof *(atomic) == sizeof(uintptr_t));              \
        (void)(0 ? (void*)*(atomic) : NULL);                               \
        (void)(0 ? (val) ^ (val) : 1);                                     \
        (uintptr_t) __atomic_fetch_or(tmpAtomic, (val), __ATOMIC_SEQ_CST); \
    }))
#define fu_atomic_pointer_xor(atomic, val)                                  \
    (__extension__({                                                        \
        uintptr_t* tmpAtomic = (uintptr_t*)(atomic);                        \
        static_assert(sizeof *(atomic) == sizeof(void*));                   \
        static_assert(sizeof *(atomic) == sizeof(uintptr_t));               \
        (void)(0 ? (void*)*(atomic) : NULL);                                \
        (void)(0 ? (val) ^ (val) : 1);                                      \
        (uintptr_t) __atomic_fetch_xor(tmpAtomic, (val), __ATOMIC_SEQ_CST); \
    }))
#define fu_atomic_pointer_exchange(atomic, newval)                        \
    (__extension__({                                                      \
        static_assert(sizeof *(atomic) == sizeof(void*));                 \
        (void)(0 ? (void*)*(atomic) : NULL);                              \
        (void*)__atomic_exchange_n((atomic), (newval), __ATOMIC_SEQ_CST); \
    }))

#define fu_atomic_pointer_compare_and_exchange(atomic, oldval, newval)                                                                    \
    (__extension__({                                                                                                                      \
        static_assert(sizeof(oldval) == sizeof(void*));                                                                                   \
        void* tmpOldVal = (void*)(oldval);                                                                                                \
        static_assert(sizeof *(atomic) == sizeof(void*));                                                                                 \
        (void)(0 ? (void*)*(atomic) : NULL);                                                                                              \
        __atomic_compare_exchange_n((atomic), (void*)(&(tmpOldVal)), (newval), false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ? true : false; \
    }))

#define fu_atomic_pointer_compare_and_exchange_full(atomic, oldval, newval, preval) \
    (__extension__({                                                                \
        static_assert(sizeof *(atomic) == sizeof(void*));                           \
        static_assert(sizeof *(preval) == sizeof(void*));                           \
        (void)(0 ? (void*)*(atomic) : NULL);                                        \
        (void)(0 ? (void*)*(preval) : NULL);                                        \
        *(preval) = (oldval);                                                       \
        __atomic_compare_exchange_n((atomic), (preval), (newval), false,            \
            __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)                                     \
            ? true                                                                  \
            : false;                                                                \
    }))
#else // FU_USE_BUILTIN_ATOMIC
#include <stdatomic.h>

#endif // FU_USE_BUILTIN_ATOMIC
// #else // FU_ATOMIC_LOCK_FREE
// #define fu_atomic_int_get(atomic) (fu_atomic_int_get((int*)(atomic)))
// #define fu_atomic_int_set(atomic, newval) (fu_atomic_int_set((int*)(atomic), (int)(newval)))
// #define fu_atomic_int_inc(atomic) (fu_atomic_int_inc((int*)(atomic)))
// #define fu_atomic_int_add(atomic, val) (fu_atomic_int_add((int*)(atomic), (val)))
// #define fu_atomic_int_and(atomic, val) (fu_atomic_int_and((uint32_t*)(atomic), (val)))
// #define fu_atomic_int_or(atomic, val) (fu_atomic_int_or((uint32_t*)(atomic), (val)))
// #define fu_atomic_int_xor(atomic, val) (fu_atomic_int_xor((uint32_t*)(atomic), (val)))
// #define fu_atomic_int_exchange(atomic, newval) (fu_atomic_int_exchange((int*)(atomic), (newval)))
// #define fu_atomic_int_dec_and_test(atomic) (fu_atomic_int_dec_and_test((int*)(atomic)))
// #define fu_atomic_int_compare_and_exchange(atomic, oldval, newval) (fu_atomic_int_compare_and_exchange((int*)(atomic), (oldval), (newval)))
// #define fu_atomic_int_compare_and_exchange_full(atomic, oldval, newval, preval) (fu_atomic_int_compare_and_exchange_full((int*)(atomic), (oldval), (newval), (int*)(preval)))

// /* The (void *) cast in the middle *looks* redundant, because
//  * fu_atomic_pointer_get returns void * already, but it's to silence
//  * -Werror=bad-function-cast when we're doing something like:
//  * uintptr_t a, b; ...; a = fu_atomic_pointer_get (&b);
//  * which would otherwise be assigning the void * result of
//  * fu_atomic_pointer_get directly to the pointer-sized but
//  * non-pointer-typed result. */
// #define fu_atomic_pointer_get(atomic) (typeof(*(atomic)))(void*)((fu_atomic_pointer_get)((void*)atomic))
// #define fu_atomic_pointer_set(atomic, newval) (fu_atomic_pointer_set((atomic), (void*)(newval)))
// #define fu_atomic_pointer_add(atomic, val) (fu_atomic_pointer_add((atomic), (ssize_t)(val)))
// #define fu_atomic_pointer_and(atomic, val) (fu_atomic_pointer_and((atomic), (size_t)(val)))
// #define fu_atomic_pointer_or(atomic, val) (fu_atomic_pointer_or((atomic), (size_t)(val)))
// #define fu_atomic_pointer_xor(atomic, val) (fu_atomic_pointer_xor((atomic), (size_t)(val)))
// #define fu_atomic_pointer_exchange(atomic, newval) (fu_atomic_pointer_exchange((atomic), (void*)(newval)))
// #define fu_atomic_pointer_compare_and_exchange(atomic, oldval, newval) (fu_atomic_pointer_compare_and_exchange((atomic), (void*)(oldval), (void*)(newval)))
// #define fu_atomic_pointer_compare_and_exchange_full(atomic, oldval, newval, prevval) (fu_atomic_pointer_compare_and_exchange_full((atomic), (void*)(oldval), (void*)(newval), (prevval)))
#endif // FU_ATOMIC_LOCK_FREE

#define fu_atomic_int_get(arc) (atomic_load(arc))
// void fu_atomic_uint_set(volatile atomic_uint* atomic, uint32_t newval);
// uint32_t fu_atomic_uint_add(volatile atomic_uint* atomic);
// uint32_t fu_atomic_uint_sub(volatile atomic_uint* atomic, uint32_t val);
// uint32_t fu_atomic_uint_exchange(volatile atomic_uint* atomic, uint32_t val);

#define fu_atomic_ref_count_init(arc) (atomic_init((arc), 1))
#define fu_atomic_ref_count_inc(arc) (atomic_fetch_add(arc, 1))
bool fu_atomic_ref_count_dec(atomic_int* arc);
#define fu_atomic_ref_count_compare(arc, val) (atomic_load(arc) == val)

#endif /* __fu_atomic_H__ */
