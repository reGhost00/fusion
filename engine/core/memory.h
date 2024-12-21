#pragma once
#ifdef FU_ENABLE_TRACK_MEMORY
#ifdef __cplusplus
#include <cstddef>
extern "C" {
#else
#include <stddef.h>
#endif
void* fu_malloc(size_t size);
void* fu_malloc0(size_t size);
void* fu_malloc_n(size_t count, size_t size);
void* fu_malloc0_n(size_t count, size_t size);
void* fu_realloc(void* p, size_t size);
void fu_free(void* p);
#else
#include <mimalloc.h>
#define fu_malloc mi_malloc
#define fu_malloc0 mi_zalloc
#define fu_malloc_n mi_mallocn
#define fu_malloc0_n mi_calloc
#define fu_realloc mi_realloc
#define fu_free mi_free
#ifdef __cplusplus
extern "C" {
#endif
#endif

/**
 * fu_steal_pointer:
 * @pp: (not nullable): a pointer to a pointer
 *
 * Sets @pp to %NULL, returning the value that was there before.
 *
 * Conceptually, this transfers the ownership of the pointer from the
 * referenced variable to the "caller" of the macro (ie: "steals" the
 * reference).
 *
 * The return value will be properly typed, according to the type of @pp. */
static inline void* fu_steal_pointer(void* pp)
{
    void** ptr = (void**)pp;
    void* ref;

    ref = *ptr;
    *ptr = NULL;

    return ref;
}

typedef void (*FUNotify)(void* usd);
#define fu_steal_pointer(pp) ((typeof(*pp))(fu_steal_pointer)(pp))
void fu_clear_pointer(void** pp, FUNotify destroy);

void* fu_memdup(const void* p, size_t size);
char* fu_strdup(const char* src);
char* fu_strndup(const char* str, size_t n);
char* fu_strnfill(size_t length, char ch);
char* fu_strdup_printf(const char* format, ...);

#ifdef __cplusplus
}
#endif