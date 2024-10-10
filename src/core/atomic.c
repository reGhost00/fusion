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

#include "atomic.h"

#include <stddef.h>
/**
 * G_ATOMIC_LOCK_FREE:
 *
 * This macro is defined if the atomic operations of GLib are
 * implemented using real hardware atomic operations.  This means that
 * the GLib atomic API can be used between processes and safely mixed
 * with other (hardware) atomic APIs.
 *
 * If this macro is not defined, the atomic operations may be
 * emulated using a mutex.  In that case, the GLib atomic operations are
 * only atomic relative to themselves and within a single process.
 **/

/* NOTE CAREFULLY:
 *
 * This file is the lowest-level part of GLib.
 *
 * Other lowlevel parts of GLib (threads, slice allocator, g_malloc,
 * messages, etc) call into these functions and macros to get work done.
 *
 * As such, these functions can not call back into any part of GLib
 * without risking recursion.
 */

#ifndef FU_DISABLED_ATOMIC_LOCK_FREE
#ifdef FU_USE_BUILTIN_ATOMIC

/**
 * g_atomic_int_get:
 * @atomic: a pointer to a #int or #uint32_t
 *
 * Gets the current value of @atomic.
 *
 * This call acts as a full compiler and hardware
 * memory barrier (before the get).
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Returns: the value of the integer
 *
 * Since: 2.4
 **/
int(fu_atomic_int_get)(const volatile int* atomic)
{
    return fu_atomic_int_get(atomic);
}

/**
 * g_atomic_int_set:
 * @atomic: a pointer to a #int or #uint32_t
 * @newval: a new value to store
 *
 * Sets the value of @atomic to @newval.
 *
 * This call acts as a full compiler and hardware
 * memory barrier (after the set).
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Since: 2.4
 */
void(fu_atomic_int_set)(volatile int* atomic, int newval)
{
    fu_atomic_int_set(atomic, newval);
}

/**
 * g_atomic_int_inc:
 * @atomic: a pointer to a #int or #uint32_t
 *
 * Increments the value of @atomic by 1.
 *
 * Think of this operation as an atomic version of `{ *atomic += 1; }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Since: 2.4
 **/
void(fu_atomic_int_inc)(volatile int* atomic)
{
    fu_atomic_int_inc(atomic);
}

/**
 * g_atomic_int_add:
 * @atomic: a pointer to a #int or #uint32_t
 * @val: the value to add
 *
 * Atomically adds @val to the value of @atomic.
 *
 * Think of this operation as an atomic version of
 * `{ tmp = *atomic; *atomic += val; return tmp; }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * Before version 2.30, this function did not return a value
 * (but g_atomic_int_exchange_and_add() did, and had the same meaning).
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Returns: the value of @atomic before the add, signed
 *
 * Since: 2.4
 **/
int(fu_atomic_int_add)(volatile int* atomic, int val)
{
    return fu_atomic_int_add(atomic, val);
}

/**
 * g_atomic_int_and:
 * @atomic: a pointer to a #int or #uint32_t
 * @val: the value to 'and'
 *
 * Performs an atomic bitwise 'and' of the value of @atomic and @val,
 * storing the result back in @atomic.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * Think of this operation as an atomic version of
 * `{ tmp = *atomic; *atomic &= val; return tmp; }`.
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Returns: the value of @atomic before the operation, unsigned
 *
 * Since: 2.30
 **/
uint32_t(fu_atomic_int_and)(volatile uint32_t* atomic, uint32_t val)
{
    return fu_atomic_int_and(atomic, val);
}

/**
 * g_atomic_int_or:
 * @atomic: a pointer to a #int or #uint32_t
 * @val: the value to 'or'
 *
 * Performs an atomic bitwise 'or' of the value of @atomic and @val,
 * storing the result back in @atomic.
 *
 * Think of this operation as an atomic version of
 * `{ tmp = *atomic; *atomic |= val; return tmp; }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Returns: the value of @atomic before the operation, unsigned
 *
 * Since: 2.30
 **/
uint32_t(fu_atomic_int_or)(volatile uint32_t* atomic, uint32_t val)
{
    return fu_atomic_int_or(atomic, val);
}

/**
 * g_atomic_int_xor:
 * @atomic: a pointer to a #int or #uint32_t
 * @val: the value to 'xor'
 *
 * Performs an atomic bitwise 'xor' of the value of @atomic and @val,
 * storing the result back in @atomic.
 *
 * Think of this operation as an atomic version of
 * `{ tmp = *atomic; *atomic ^= val; return tmp; }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Returns: the value of @atomic before the operation, unsigned
 *
 * Since: 2.30
 **/
uint32_t(fu_atomic_int_xor)(volatile uint32_t* atomic, uint32_t val)
{
    return fu_atomic_int_xor(atomic, val);
}

/**
 * g_atomic_int_exchange:
 * @atomic: a pointer to a #int or #uint32_t
 * @newval: the value to replace with
 *
 * Sets the @atomic to @newval and returns the old value from @atomic.
 *
 * This exchange is done atomically.
 *
 * Think of this operation as an atomic version of
 * `{ tmp = *atomic; *atomic = val; return tmp; }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * Returns: the value of @atomic before the exchange, signed
 *
 * Since: 2.74
 **/
int(fu_atomic_int_exchange)(int* atomic, int newval)
{
    return fu_atomic_int_exchange(atomic, newval);
}
/**
 * g_atomic_int_dec_and_test:
 * @atomic: a pointer to a #int or #uint32_t
 *
 * Decrements the value of @atomic by 1.
 *
 * Think of this operation as an atomic version of
 * `{ *atomic -= 1; return (*atomic == 0); }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Returns: %TRUE if the resultant value is zero
 *
 * Since: 2.4
 **/
bool(fu_atomic_int_dec_and_test)(volatile int* atomic)
{
    return fu_atomic_int_dec_and_test(atomic);
}

/**
 * g_atomic_int_compare_and_exchange:
 * @atomic: a pointer to a #int or #uint32_t
 * @oldval: the value to compare with
 * @newval: the value to conditionally replace with
 *
 * Compares @atomic to @oldval and, if equal, sets it to @newval.
 * If @atomic was not equal to @oldval then no change occurs.
 *
 * This compare and exchange is done atomically.
 *
 * Think of this operation as an atomic version of
 * `{ if (*atomic == oldval) { *atomic = newval; return TRUE; } else return FALSE; }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Returns: %TRUE if the exchange took place
 *
 * Since: 2.4
 **/
bool(fu_atomic_int_compare_and_exchange)(volatile int* atomic, int oldval, int newval)
{
    return fu_atomic_int_compare_and_exchange(atomic, oldval, newval);
}

/**
 * g_atomic_int_compare_and_exchange_full:
 * @atomic: a pointer to a #int or #uint32_t
 * @oldval: the value to compare with
 * @newval: the value to conditionally replace with
 * @preval: (out): the contents of @atomic before this operation
 *
 * Compares @atomic to @oldval and, if equal, sets it to @newval.
 * If @atomic was not equal to @oldval then no change occurs.
 * In any case the value of @atomic before this operation is stored in @preval.
 *
 * This compare and exchange is done atomically.
 *
 * Think of this operation as an atomic version of
 * `{ *preval = *atomic; if (*atomic == oldval) { *atomic = newval; return TRUE; } else return FALSE; }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * See also g_atomic_int_compare_and_exchange()
 *
 * Returns: %TRUE if the exchange took place
 *
 * Since: 2.74
 **/
bool(fu_atomic_int_compare_and_exchange_full)(int* atomic, int oldval, int newval, int* preval)
{
    return fu_atomic_int_compare_and_exchange_full(atomic, oldval, newval, preval);
}

/**
 * g_atomic_pointer_get:
 * @atomic: (not nullable): a pointer to a #void*-sized value
 *
 * Gets the current value of @atomic.
 *
 * This call acts as a full compiler and hardware
 * memory barrier (before the get).
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Returns: the value of the pointer
 *
 * Since: 2.4
 **/
void*(fu_atomic_pointer_get)(const volatile void* atomic)
{
    return fu_atomic_pointer_get((void**)atomic);
}
/**
 * g_atomic_pointer_set:
 * @atomic: (not nullable): a pointer to a #void*-sized value
 * @newval: a new value to store
 *
 * Sets the value of @atomic to @newval.
 *
 * This call acts as a full compiler and hardware
 * memory barrier (after the set).
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Since: 2.4
 **/
void(fu_atomic_pointer_set)(volatile void* atomic, void* newval)
{
    fu_atomic_pointer_set((void**)atomic, newval);
}

/**
 * g_atomic_pointer_add:
 * @atomic: (not nullable): a pointer to a #void*-sized value
 * @val: the value to add
 *
 * Atomically adds @val to the value of @atomic.
 *
 * Think of this operation as an atomic version of
 * `{ tmp = *atomic; *atomic += val; return tmp; }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * In GLib 2.80, the return type was changed from #ssize_t to #intptr_t to add
 * support for platforms with 128-bit pointers. This should not affect existing
 * code.
 *
 * Returns: the value of @atomic before the add, signed
 *
 * Since: 2.30
 **/
intptr_t(fu_atomic_pointer_add)(volatile void* atomic, ssize_t val)
{
    return fu_atomic_pointer_add((void**)atomic, val);
}

/**
 * g_atomic_pointer_and:
 * @atomic: (not nullable): a pointer to a #void*-sized value
 * @val: the value to 'and'
 *
 * Performs an atomic bitwise 'and' of the value of @atomic and @val,
 * storing the result back in @atomic.
 *
 * Think of this operation as an atomic version of
 * `{ tmp = *atomic; *atomic &= val; return tmp; }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * In GLib 2.80, the return type was changed from #size_t to #uintptr_t to add
 * support for platforms with 128-bit pointers. This should not affect existing
 * code.
 *
 * Returns: the value of @atomic before the operation, unsigned
 *
 * Since: 2.30
 **/
uintptr_t(fu_atomic_pointer_and)(volatile void* atomic, size_t val)
{
    return fu_atomic_pointer_and((void**)atomic, val);
}

/**
 * g_atomic_pointer_or:
 * @atomic: (not nullable): a pointer to a #void*-sized value
 * @val: the value to 'or'
 *
 * Performs an atomic bitwise 'or' of the value of @atomic and @val,
 * storing the result back in @atomic.
 *
 * Think of this operation as an atomic version of
 * `{ tmp = *atomic; *atomic |= val; return tmp; }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * In GLib 2.80, the return type was changed from #size_t to #uintptr_t to add
 * support for platforms with 128-bit pointers. This should not affect existing
 * code.
 *
 * Returns: the value of @atomic before the operation, unsigned
 *
 * Since: 2.30
 **/
uintptr_t(fu_atomic_pointer_or)(volatile void* atomic, size_t val)
{
    return fu_atomic_pointer_or((void**)atomic, val);
}

/**
 * g_atomic_pointer_xor:
 * @atomic: (not nullable): a pointer to a #void*-sized value
 * @val: the value to 'xor'
 *
 * Performs an atomic bitwise 'xor' of the value of @atomic and @val,
 * storing the result back in @atomic.
 *
 * Think of this operation as an atomic version of
 * `{ tmp = *atomic; *atomic ^= val; return tmp; }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * In GLib 2.80, the return type was changed from #size_t to #uintptr_t to add
 * support for platforms with 128-bit pointers. This should not affect existing
 * code.
 *
 * Returns: the value of @atomic before the operation, unsigned
 *
 * Since: 2.30
 **/
uintptr_t(fu_atomic_pointer_xor)(volatile void* atomic, size_t val)
{
    return fu_atomic_pointer_xor((void**)atomic, val);
}

/**
 * g_atomic_pointer_exchange:
 * @atomic: a pointer to a #void*-sized value
 * @newval: the value to replace with
 *
 * Sets the @atomic to @newval and returns the old value from @atomic.
 *
 * This exchange is done atomically.
 *
 * Think of this operation as an atomic version of
 * `{ tmp = *atomic; *atomic = val; return tmp; }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * Returns: the value of @atomic before the exchange
 *
 * Since: 2.74
 **/
void*(fu_atomic_pointer_exchange)(void* atomic, void* newval)
{
    return fu_atomic_pointer_exchange((void**)atomic, newval);
}

/**
 * g_atomic_pointer_compare_and_exchange:
 * @atomic: (not nullable): a pointer to a #void*-sized value
 * @oldval: the value to compare with
 * @newval: the value to conditionally replace with
 *
 * Compares @atomic to @oldval and, if equal, sets it to @newval.
 * If @atomic was not equal to @oldval then no change occurs.
 *
 * This compare and exchange is done atomically.
 *
 * Think of this operation as an atomic version of
 * `{ if (*atomic == oldval) { *atomic = newval; return TRUE; } else return FALSE; }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * While @atomic has a `volatile` qualifier, this is a historical artifact and
 * the pointer passed to it should not be `volatile`.
 *
 * Returns: %TRUE if the exchange took place
 *
 * Since: 2.4
 **/
bool(fu_atomic_pointer_compare_and_exchange)(volatile void* atomic, void* oldval, void* newval)
{
    return fu_atomic_pointer_compare_and_exchange((void**)atomic, oldval, newval);
}

/**
 * g_atomic_pointer_compare_and_exchange_full:
 * @atomic: (not nullable): a pointer to a #void*-sized value
 * @oldval: the value to compare with
 * @newval: the value to conditionally replace with
 * @preval: (not nullable) (out): the contents of @atomic before this operation
 *
 * Compares @atomic to @oldval and, if equal, sets it to @newval.
 * If @atomic was not equal to @oldval then no change occurs.
 * In any case the value of @atomic before this operation is stored in @preval.
 *
 * This compare and exchange is done atomically.
 *
 * Think of this operation as an atomic version of
 * `{ *preval = *atomic; if (*atomic == oldval) { *atomic = newval; return TRUE; } else return FALSE; }`.
 *
 * This call acts as a full compiler and hardware memory barrier.
 *
 * See also g_atomic_pointer_compare_and_exchange()
 *
 * Returns: %TRUE if the exchange took place
 *
 * Since: 2.74
 **/
bool(fu_atomic_pointer_compare_and_exchange_full)(void* atomic, void* oldval, void* newval, void* preval)
{
    return fu_atomic_pointer_compare_and_exchange_full((void**)atomic, oldval, newval, (void**)preval);
}

#else

#endif

bool fu_atomic_ref_count_dec(atomic_int* arc)
{
    atomic_int ov = atomic_fetch_sub(arc, 1);
    return 1 < ov;
}

// bool fu_atomic_ref_count_compare(atomic_int* arc, int32_t val)
// {
//     return atomic_load(arc) == val;
// }
#else
/* This error occurs when `meson configure` decided that we should be capable
 * of lock-free atomics but we find at compile-time that we are not.
 */
// #error G_ATOMIC_LOCK_FREE defined, but incapable of lock-free atomics.

// #else /* G_ATOMIC_LOCK_FREE */
// #ifdef USE_PT
/* We are not permitted to call into any GLib functions from here, so we
 * can not use GMutex.
 *
 * Fortunately, we already take care of the Windows case above, and all
 * non-Windows platforms on which glib runs have pthreads.  Use those.
 */
#include <pthread.h>

static pthread_mutex_t defAtomicMutex = PTHREAD_MUTEX_INITIALIZER;

int(fu_atomic_int_get)(const volatile int* atomic)
{
    int value;

    pthread_mutex_lock(&defAtomicMutex);
    value = *atomic;
    pthread_mutex_unlock(&defAtomicMutex);

    return value;
}

void(fu_atomic_int_set)(volatile int* atomic, int value)
{
    pthread_mutex_lock(&defAtomicMutex);
    *atomic = value;
    pthread_mutex_unlock(&defAtomicMutex);
}

void(fu_atomic_int_inc)(volatile int* atomic)
{
    pthread_mutex_lock(&defAtomicMutex);
    (*atomic)++;
    pthread_mutex_unlock(&defAtomicMutex);
}

bool(fu_atomic_int_dec_and_test)(volatile int* atomic)
{
    bool rev;
    pthread_mutex_lock(&defAtomicMutex);
    (*atomic) -= 1;
    rev = !(*atomic);
    pthread_mutex_unlock(&defAtomicMutex);
    return rev;
}

bool(fu_atomic_int_compare_and_exchange)(volatile int* atomic, int oldval, int newval)
{
    bool success;

    pthread_mutex_lock(&defAtomicMutex);

    if ((success = (*atomic == oldval)))
        *atomic = newval;

    pthread_mutex_unlock(&defAtomicMutex);

    return success;
}

bool(fu_atomic_int_compare_and_exchange_full)(int* atomic, int oldval, int newval, int* preval)
{
    bool success;

    pthread_mutex_lock(&defAtomicMutex);

    *preval = *atomic;

    if ((success = (*atomic == oldval)))
        *atomic = newval;

    pthread_mutex_unlock(&defAtomicMutex);

    return success;
}

int(fu_atomic_int_exchange)(int* atomic, int newval)
{
    int* ptr = atomic;
    int oldval;

    pthread_mutex_lock(&defAtomicMutex);
    oldval = *ptr;
    *ptr = newval;
    pthread_mutex_unlock(&defAtomicMutex);

    return oldval;
}

int(fu_atomic_int_add)(volatile int* atomic, int val)
{
    int oldval;

    pthread_mutex_lock(&defAtomicMutex);
    oldval = *atomic;
    *atomic = oldval + val;
    pthread_mutex_unlock(&defAtomicMutex);

    return oldval;
}

uint32_t(fu_atomic_int_and)(volatile uint32_t* atomic, uint32_t val)
{
    uint32_t oldval;

    pthread_mutex_lock(&defAtomicMutex);
    oldval = *atomic;
    *atomic = oldval & val;
    pthread_mutex_unlock(&defAtomicMutex);

    return oldval;
}

uint32_t(fu_atomic_int_or)(volatile uint32_t* atomic, uint32_t val)
{
    uint32_t oldval;

    pthread_mutex_lock(&defAtomicMutex);
    oldval = *atomic;
    *atomic = oldval | val;
    pthread_mutex_unlock(&defAtomicMutex);

    return oldval;
}

uint32_t(fu_atomic_int_xor)(volatile uint32_t* atomic, uint32_t val)
{
    uint32_t oldval;

    pthread_mutex_lock(&defAtomicMutex);
    oldval = *atomic;
    *atomic = oldval ^ val;
    pthread_mutex_unlock(&defAtomicMutex);

    return oldval;
}

void*(fu_atomic_pointer_get)(const volatile void* atomic)
{
    const void** ptr = (void*)atomic;
    void* value;

    pthread_mutex_lock(&defAtomicMutex);
    value = (void*)(*ptr);
    pthread_mutex_unlock(&defAtomicMutex);

    return value;
}

void(fu_atomic_pointer_set)(volatile void* atomic, void* newval)
{
    void** ptr = (void*)atomic;

    pthread_mutex_lock(&defAtomicMutex);
    *ptr = newval;
    pthread_mutex_unlock(&defAtomicMutex);
}

bool(fu_atomic_pointer_compare_and_exchange)(volatile void* atomic, void* oldval, void* newval)
{
    void** ptr = (void**)atomic;
    bool success;

    pthread_mutex_lock(&defAtomicMutex);

    if ((success = (*ptr == oldval)))
        *ptr = newval;

    pthread_mutex_unlock(&defAtomicMutex);

    return success;
}

bool(fu_atomic_pointer_compare_and_exchange_full)(void* atomic, void* oldval, void* newval, void* preval)
{
    void** ptr = atomic;
    void** pre = preval;
    bool success;

    pthread_mutex_lock(&defAtomicMutex);

    *pre = *ptr;
    if ((success = (*ptr == oldval)))
        *ptr = newval;

    pthread_mutex_unlock(&defAtomicMutex);

    return success;
}

void*(fu_atomic_pointer_exchange)(void* atomic, void* newval)
{
    void** ptr = atomic;
    void* oldval;

    pthread_mutex_lock(&defAtomicMutex);
    oldval = *ptr;
    *ptr = newval;
    pthread_mutex_unlock(&defAtomicMutex);

    return oldval;
}

intptr_t(fu_atomic_pointer_add)(volatile void* atomic, ssize_t val)
{
    intptr_t* ptr = (void*)atomic;
    intptr_t oldval;

    pthread_mutex_lock(&defAtomicMutex);
    oldval = *ptr;
    *ptr = oldval + val;
    pthread_mutex_unlock(&defAtomicMutex);

    return oldval;
}

uintptr_t(fu_atomic_pointer_and)(volatile void* atomic, size_t val)
{
    uintptr_t* ptr = (void*)atomic;
    uintptr_t oldval;

    pthread_mutex_lock(&defAtomicMutex);
    oldval = *ptr;
    *ptr = oldval & val;
    pthread_mutex_unlock(&defAtomicMutex);

    return oldval;
}

uintptr_t(fu_atomic_pointer_or)(volatile void* atomic, size_t val)
{
    uintptr_t* ptr = (void*)atomic;
    uintptr_t oldval;

    pthread_mutex_lock(&defAtomicMutex);
    oldval = *ptr;
    *ptr = oldval | val;
    pthread_mutex_unlock(&defAtomicMutex);

    return oldval;
}

uintptr_t(fu_atomic_pointer_xor)(volatile void* atomic, size_t val)
{
    uintptr_t* ptr = (void*)atomic;
    uintptr_t oldval;

    pthread_mutex_lock(&defAtomicMutex);
    oldval = *ptr;
    *ptr = oldval ^ val;
    pthread_mutex_unlock(&defAtomicMutex);

    return oldval;
}

#endif
