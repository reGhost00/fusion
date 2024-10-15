
#include <stdarg.h>
#include <stdatomic.h>
#include <string.h>

#include "utils.h"
#include <mimalloc.h>

#ifdef FU_NO_TRACK_MEMORY
#define fu_track_memory(p, size) (p)
#else

#include "sc_map.h"
#include "thread.h"

static struct sc_map_64 defMemoryTable;
static atomic_uint defMemoryTableCount = 0;
static bool ifMemoryTableInit = false;
#ifdef FU_OS_WINDOW

#else
static pthread_mutex_t defMemoryTableMutex = PTHREAD_MUTEX_INITIALIZER;
static bool isMutexInit = false;
#endif
static void fu_check_memory_leak()
{
    const uint32_t leakCnt = atomic_load_explicit(&defMemoryTableCount, memory_order_relaxed);
    if (leakCnt) {
        uint64_t addr;
        size_t size, total = 0;
        sc_map_foreach(&defMemoryTable, addr, size)
        {
            total += size;
            mi_free((void*)addr);
        };
        if (total)
            fprintf(stderr, "Memory leak: %u blocks %zu bytes\n", leakCnt, total);
    }
    if (ifMemoryTableInit)
        sc_map_term_64(&defMemoryTable);
}

static void* fu_track_memory(void* p, size_t size)
{
    #ifdef FU_OS_WINDOW
    if (FU_UNLIKELY(!defMemoryTableMutex))
        fu_mutex_init(&defMemoryTableMutex);
    #endif
    fu_mutex_lock(&defMemoryTableMutex);
    if (!atomic_fetch_add_explicit(&defMemoryTableCount, 1, memory_order_relaxed) && !ifMemoryTableInit) {
        sc_map_init_64(&defMemoryTable, 0, 0);
        atexit(fu_check_memory_leak);
        ifMemoryTableInit = true;
    }
    sc_map_put_64(&defMemoryTable, (uintptr_t)p, size);
    fu_mutex_unlock(&defMemoryTableMutex);
    return p;
}

void* fu_malloc(size_t size)
{
    return fu_track_memory(mi_malloc(size), size);
}

void* fu_malloc_n(size_t count, size_t size)
{
    return fu_track_memory(mi_mallocn(count, size), count * size);
}

void* fu_malloc0(size_t size)
{
    return fu_track_memory(mi_zalloc(size), size);
}

void* fu_malloc0_n(size_t count, size_t size)
{
    return fu_track_memory(mi_calloc(count, size), count * size);
}

void* fu_realloc(void* p, size_t size)
{
    if (FU_UNLIKELY(!p))
        return fu_malloc(size);
#ifdef FU_OS_LINUX
    if (FU_UNLIKELY(!isMutexInit)) {
        isMutexInit = true;
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&defMemoryTableMutex, &attr);
        pthread_mutexattr_destroy(&attr);
    }
#endif
    fu_mutex_lock(&defMemoryTableMutex);
    atomic_fetch_sub_explicit(&defMemoryTableCount, 1, memory_order_relaxed);
    sc_map_del_64(&defMemoryTable, (uintptr_t)p);
    void* rev = fu_track_memory(mi_realloc(p, size), size);
    fu_mutex_unlock(&defMemoryTableMutex);
    return rev;
}

void fu_free(void* p)
{
    if (FU_UNLIKELY(!p))
        return;
    fu_mutex_lock(&defMemoryTableMutex);
    sc_map_del_64(&defMemoryTable, (uintptr_t)p);
    if (sc_map_found(&defMemoryTable) && atomic_load_explicit(&defMemoryTableCount, memory_order_relaxed))
        atomic_fetch_sub_explicit(&defMemoryTableCount, 1, memory_order_relaxed);
    // defMemoryTableCount -= 1;
    mi_free(p);
    fu_mutex_unlock(&defMemoryTableMutex);
}
#endif

/**
 * g_clear_pointer: (skip)
 * @pp: (nullable) (not optional) (inout) (transfer full): a pointer to a
 *   variable, struct member etc. holding a pointer
 * @destroy: a function to which a gpointer can be passed, to destroy `*pp`
 *
 * Clears a reference to a variable.
 *
 * @pp must not be %NULL.
 *
 * If the reference is %NULL then this function does nothing.
 * Otherwise, the variable is destroyed using @destroy and the
 * pointer is set to %NULL.
 *
 * A macro is also included that allows this function to be used without
 * pointer casts. This will mask any warnings about incompatible function types
 * or calling conventions, so you must ensure that your @destroy function is
 * compatible with being called as [callback@GLib.DestroyNotify] using the
 * standard calling convention for the platform that GLib was compiled for;
 * otherwise the program will experience undefined behaviour.
 *
 * Examples of this kind of undefined behaviour include using many Windows Win32
 * APIs, as well as many if not all OpenGL and Vulkan calls on 32-bit Windows,
 * which typically use the `__stdcall` calling convention rather than the
 * `__cdecl` calling convention.
 *
 * The affected functions can be used by wrapping them in a
 * [callback@GLib.DestroyNotify] that is declared with the standard calling
 * convention:
 *
 * ```c
 * // Wrapper needed to avoid mismatched calling conventions on Windows
 * static void
 * destroy_sync (void *sync)
 * {
 *   glDeleteSync (sync);
 * }
 *
 * // …
 *
 * g_clear_pointer (&sync, destroy_sync);
 * ```

 *
 * Since: 2.34
 **/

void fu_clear_pointer(void** pp, FUNotify destroy)
{
    void* _p = *pp;
    if (_p) {
        *pp = NULL;
        destroy(_p);
    }
}

//
// 字符串工具函数

/**
 * g_vasprintf:
 * @string: (out) (not optional) (nullable): the return location for the
 *   newly-allocated string, which will be `NULL` if (and only if)
 *   this function fails
 * @format: (not nullable): a standard `printf()` format string, but notice
 *   [string precision pitfalls](string-utils.html#string-precision-pitfalls)
 * @args: the list of arguments to insert in the output
 *
 * An implementation of the GNU `vasprintf()` function which supports
 * positional parameters, as specified in the Single Unix Specification.
 * This function is similar to [func@GLib.vsprintf], except that it allocates a
 * string to hold the output, instead of putting the output in a buffer
 * you allocate in advance.
 *
 * The returned value in @string is guaranteed to be non-`NULL`, unless
 * @format contains `%lc` or `%ls` conversions, which can fail if no
 * multibyte representation is available for the given character.
 *
 * `glib/gprintf.h` must be explicitly included in order to use this function.
 *
 * Returns: the number of bytes printed, or -1 on failure
 *
 * Since: 2.4
 **/
static void fu_vasprintf(char** string, char const* format, va_list args)
{
    va_list args2;
    char c;

    va_copy(args2, args);

    int max_len = vsnprintf(&c, 1, format, args);
    if (FU_UNLIKELY(0 > max_len)) {
        /* This can happen if @format contains `%ls` or `%lc` and @args contains
         * something not representable in the current locale’s encoding (which
         * should be UTF-8, but ymmv). Basically: don’t use `%ls` or `%lc`. */
        va_end(args2);
        *string = NULL;
        return;
    }
    max_len += 1;
    *string = fu_track_memory(mi_zalloc(max_len), max_len);

    if (FU_UNLIKELY(0 > vsprintf(*string, format, args2)))
        abort();
    va_end(args2);
}

/**
 * g_memdup2:
 * @mem: (nullable): the memory to copy
 * @byte_size: the number of bytes to copy
 *
 * Allocates @byte_size bytes of memory, and copies @byte_size bytes into it
 * from @mem. If @mem is `NULL` it returns `NULL`.
 *
 * This replaces [func@GLib.memdup], which was prone to integer overflows when
 * converting the argument from a `gsize` to a `guint`.
 *
 * Returns: (transfer full) (nullable): a pointer to the newly-allocated copy of the memory
 *
 * Since: 2.68
 */
void* fu_memdup(const void* mem, size_t size)
{
    if (FU_UNLIKELY(!(mem && size)))
        return NULL;
    void* dest = fu_track_memory(mi_malloc(size), size);
    memcpy(dest, mem, size);
    return dest;
}
/**
 * g_strdup:
 * @str: (nullable): the string to duplicate
 *
 * Duplicates a string. If @str is `NULL` it returns `NULL`.
 *
 * Returns: a newly-allocated copy of @str
 */
char* fu_strdup(const char* str)
{
    if (FU_UNLIKELY(!str))
        return NULL;
    size_t len = strlen(str) + 1;
    char* dup = fu_track_memory(mi_malloc(len), len);
    memcpy(dup, str, len);
    return dup;
}

/**
 * g_strndup:
 * @str: (nullable): the string to duplicate
 * @n: the maximum number of bytes to copy from @str
 *
 * Duplicates the first @n bytes of a string, returning a newly-allocated
 * buffer @n + 1 bytes long which will always be nul-terminated. If @str
 * is less than @n bytes long the buffer is padded with nuls. If @str is
 * `NULL` it returns `NULL`.
 *
 * To copy a number of characters from a UTF-8 encoded string,
 * use [func@GLib.utf8_strncpy] instead.
 *
 * Returns: (nullable): a newly-allocated buffer containing the first
 *    @n bytes of @str
 */
char* fu_strndup(const char* str, size_t n)
{
    if (FU_UNLIKELY(!str))
        return NULL;
    char* dest = fu_track_memory(mi_malloc(n + 1), n + 1);
    strncpy(dest, str, n);
    dest[n] = '\0';
    return dest;
}

/**
 * g_strnfill:
 * @length: the length of the new string
 * @fill_char: the byte to fill the string with
 *
 * Creates a new string @length bytes long filled with @fill_char.
 *
 * Returns: a newly-allocated string filled with @fill_char
 */
char* fu_strnfill(size_t length, char ch)
{
    char* dest = fu_track_memory(mi_malloc(length + 1), length + 1);
    memset(dest, ch, length);
    dest[length] = '\0';
    return dest;
}

/**
 * g_stpcpy:
 * @dest: destination buffer
 * @src: source string
 *
 * Copies a nul-terminated string into the destination buffer, including
 * the trailing nul byte, and returns a pointer to the trailing nul byte
 * in `dest`.  The return value is useful for concatenating multiple
 * strings without having to repeatedly scan for the end.
 *
 * Returns: a pointer to the trailing nul byte in `dest`
 **/
char* fu_stpcpy(char* dest, const char* src)
{
    fu_return_val_if_fail(dest && src, NULL);
#ifdef HAVE_STPCPY
    return stpcpy(dest, src);
#else
    char* d = dest;
    const char* s = src;

    do
        *d++ = *s;
    while (*s++ != '\0');

    return d - 1;
#endif
}

/**
 * g_strdup_printf:
 * @format: (not nullable): a standard `printf()` format string, but notice
 *   [string precision pitfalls](string-utils.html#string-precision-pitfalls)
 * @...: the parameters to insert into the format string
 *
 * Similar to the standard C `sprintf()` function but safer, since it
 * calculates the maximum space required and allocates memory to hold
 * the result.
 *
 * The returned string is guaranteed to be non-NULL, unless @format
 * contains `%lc` or `%ls` conversions, which can fail if no multibyte
 * representation is available for the given character.
 *
 * Returns: (nullable) (transfer full): a newly-allocated string holding the
 *   result
 */
char* fu_strdup_printf(const char* format, ...)
{
    char* buff = NULL;
    va_list args;

    va_start(args, format);
    fu_vasprintf(&buff, format, args);
    va_end(args);

    return buff;
}

#ifdef FU_OS_WINDOW
char* fu_wchar_to_utf8(const wchar_t* wstr, size_t* len)
{
    if (!(*len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL)))
        return NULL;
    char* str = fu_track_memory(mi_malloc(*len), *len);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, *len, NULL, NULL);
    return str;
}

wchar_t* fu_utf8_to_wchar(const char* str, size_t* len)
{
    if (!(*len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0)))
        return NULL;
    wchar_t* wstr = fu_track_memory(mi_malloc(*len * sizeof(wchar_t)), *len * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, *len);
    return wstr;
}

FUError* fu_error_new_from_code(int code)
{
    LPWSTR msgBuff = NULL;
    size_t len = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), msgBuff, 0, NULL);
    if (len) {
        char* msg = fu_wchar_to_utf8(msgBuff, &len);
        LocalFree(msgBuff);
        if (FU_LIKELY(msg))
            return fu_error_new_take(code, &msg);
    }
    FUError* err = fu_malloc(sizeof(FUError));
    err->message = fu_strdup("Unknown error");
    err->code = code;
    return err;
}
#else
FUError* fu_error_new_from_code(int code)
{
    FUError* err = fu_malloc(sizeof(FUError));
    err->message = fu_strdup(strerror(code));
    err->code = code;
    return err;
}
#endif

FUError* fu_error_new_take(int code, char** msg)
{
    FUError* err = fu_malloc(sizeof(FUError));
    err->message = fu_steal_pointer(msg);
    err->code = code;
    return err;
}

void fu_error_free(FUError* err)
{
    fu_free(err->message);
    fu_free(err);
}