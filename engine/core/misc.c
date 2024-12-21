#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef FU_OS_LINUX
#include "platform/misc.linux.inner.h"
#include <sys/random.h>
#elif FU_OS_WINDOW
#include "platform/misc.window.inner.h"
#include <windows.h>
#endif
#include "logger.h"
#include "main.inner.h"
#include "memory.h"
#include "misc.h"
#include "object.h"

#define DEF_RAND_BUFF_SIZE 120
#define DEF_RAND_SEED_SIZE 7

/* 线性同余常用参数 */
#define DEF_RAND_LCG_A 0x0014096b
#define DEF_RAND_LCG_C 0xbaa40beb
#define DEF_RAND_LCG_M 0xffffffff

/* Period parameters */
#define DEF_RAND_MT_LOWER_MASK 0x7fffffff /* least significant r bits */

#define DEF_TIMER_FORMAT_BUFF_LEN 1600
//
//  random

typedef uint64_t* (*FPRandAlgorithm)();
typedef struct _FURandMT64 {
    uint64_t status[2];
    uint32_t mat1;
    uint32_t mat2;
    uint64_t tmat;
} FURandMT64;

static FURandMT64* defMT64 = NULL;

struct _FURand {
    FUObject parent;
    uint64_t* buff;
    FPRandAlgorithm alg;
    unsigned idx;
};
FU_DEFINE_TYPE(FURand, fu_rand, FU_TYPE_OBJECT)

static void fu_rand_mt_next_state()
{
    fu_return_if_fail(defMT64);
    uint64_t x;

    defMT64->status[0] &= DEF_RAND_MT_LOWER_MASK;
    x = defMT64->status[0] ^ defMT64->status[1];
    x ^= x << 12;
    x ^= x >> 32;
    x ^= x << 32;
    x ^= x << 11;
    defMT64->status[0] = defMT64->status[1];
    defMT64->status[1] = x;
    if ((x & 1) != 0) {
        defMT64->status[0] ^= defMT64->mat1;
        defMT64->status[1] ^= ((uint64_t)defMT64->mat2 << 32);
    }
}

static uint64_t fu_rand_mt_temper()
{
    fu_return_val_if_fail(defMT64, 0);
    uint64_t x;

    x = defMT64->status[0] + defMT64->status[1];
    x ^= defMT64->status[0] >> 8;
    if ((x & 1) != 0)
        x ^= defMT64->tmat;
    return x;
}

static void fu_rand_mt_certification()
{
    fu_return_if_fail(defMT64);
    if (!((defMT64->status[0] & DEF_RAND_MT_LOWER_MASK) || defMT64->status[1])) {
        defMT64->status[0] = 'T';
        defMT64->status[1] = 'M';
    }
}

static inline uint64_t fu_rand_mt_init_fn1(uint64_t x)
{
    return (x ^ (x >> 59)) * UINT64_C(2173292883993);
}

static inline uint64_t fu_rand_mt_init_fn2(uint64_t x)
{
    return (x ^ (x >> 59)) * UINT64_C(58885565329898161);
}

static void fu_rand_mt_init(const uint64_t* seed)
{
    fu_return_if_fail(defMT64);
    const unsigned int lag = 1;
    const unsigned int mid = 1;
    const unsigned int size = 4;
    unsigned int i, j;
    unsigned int count = DEF_RAND_SEED_SIZE + 1;
    uint64_t r;
    uint64_t st[4];

    st[0] = 0;
    st[1] = defMT64->mat1;
    st[2] = defMT64->mat2;
    st[3] = defMT64->tmat;

    r = fu_rand_mt_init_fn1(st[0] ^ st[mid % size] ^ st[(size - 1) % size]);
    st[mid % size] += r;
    r += DEF_RAND_SEED_SIZE;
    st[(mid + lag) % size] += r;
    st[0] = r;
    count--;
    for (i = 1, j = 0; (j < count) && (j < DEF_RAND_SEED_SIZE); j++) {
        r = fu_rand_mt_init_fn1(st[i] ^ st[(i + mid) % size] ^ st[(i + size - 1) % size]);
        st[(i + mid) % size] += r;
        r += seed[j] + i;
        st[(i + mid + lag) % size] += r;
        st[i] = r;
        i = (i + 1) % size;
    }
    for (; j < count; j++) {
        r = fu_rand_mt_init_fn1(st[i] ^ st[(i + mid) % size] ^ st[(i + size - 1) % size]);
        st[(i + mid) % size] += r;
        r += i;
        st[(i + mid + lag) % size] += r;
        st[i] = r;
        i = (i + 1) % size;
    }
    for (j = 0; j < size; j++) {
        r = fu_rand_mt_init_fn2(st[i] + st[(i + mid) % size] + st[(i + size - 1) % size]);
        st[(i + mid) % size] ^= r;
        r -= i;
        st[(i + mid + lag) % size] ^= r;
        st[i] = r;
        i = (i + 1) % size;
    }
    defMT64->status[0] = st[0] ^ st[1];
    defMT64->status[1] = st[2] ^ st[3];
    fu_rand_mt_certification();
}

static void fu_rand_mt_finalize()
{
    fu_return_if_fail(defMT64);
    fu_free(defMT64);
}

#ifdef FU_OS_LINUX
static uint64_t* fu_rand_new_seed()
{
    uint64_t* seed = fu_malloc(DEF_RAND_SEED_SIZE * sizeof(uint64_t));
    getrandom(seed, DEF_RAND_SEED_SIZE * sizeof(uint64_t), 0);
    return seed;
}

/** https://github.com/imneme/pcg-c-basic/tree/master */
static uint64_t* fu_rand_buff_new_os()
{
    uint64_t* buff = fu_malloc(DEF_RAND_BUFF_SIZE * sizeof(uint64_t));
    uint64_t* seed = fu_rand_new_seed();
    uint64_t x, r, ps = rand() % 0xff;
    for (int i = 0; i < DEF_RAND_BUFF_SIZE; i++) {
        ps = ps * 6364136223846793005ULL + seed[i % DEF_RAND_SEED_SIZE];
        x = ((ps >> 18u) ^ ps) >> 27u;
        r = ps >> 59u;
        buff[i] = x >> r | x << (-r & 31);
    }

    fu_free(seed);
    return buff;
}
#elif FU_OS_WINDOW
static uint64_t* fu_rand_new_seed()
{
    uint64_t* seed = fu_malloc(DEF_RAND_SEED_SIZE * sizeof(uint64_t));
    uint64_t i = GetTickCount64();
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    srand((unsigned)ts.tv_sec);
    QueryUnbiasedInterruptTime(&seed[0]);
    seed[1] = GetCurrentProcessId();
    seed[2] = i;
    seed[3] = ts.tv_sec;
    seed[4] = rand();
    seed[5] = rand();
    seed[6] = rand();
    return seed;
}

static uint64_t* fu_rand_buff_new_os()
{
    bool ifOpenAlg = false;
    BCRYPT_ALG_HANDLE alg;
    fu_os_return_val_if_fail(!BCryptOpenAlgorithmProvider(&alg, L"RNG", NULL, 0), NULL);

    ifOpenAlg = true;
    size_t len = DEF_RAND_BUFF_SIZE * sizeof(uint64_t);
    uint64_t* buff = fu_malloc(len);
    fu_os_goto_if_fail(!BCryptGenRandom(alg, (PUCHAR)buff, len, 0), out);
    BCryptCloseAlgorithmProvider(alg, 0);
    return buff;
out:
    if (ifOpenAlg) {
        fu_free(buff);
        BCryptCloseAlgorithmProvider(alg, 0);
    }
    return NULL;
}
#endif

static uint64_t* fu_rand_buff_new_lcg()
{
    uint64_t* buff = fu_malloc(DEF_RAND_BUFF_SIZE * sizeof(uint64_t));
    uint64_t* seed = fu_rand_new_seed();
    uint64_t y, c;

    for (int i = 0; i < DEF_RAND_BUFF_SIZE; i++) {
        y = i % DEF_RAND_SEED_SIZE;
        seed[y] = (DEF_RAND_LCG_A * seed[y] + DEF_RAND_LCG_C) % DEF_RAND_LCG_M;
        c = seed[y] >> 59;
        y = (seed[y] ^ (seed[y] >> 18)) >> 27;
        buff[i] = y >> c | y << (-c & 31);
    }

    fu_free(seed);
    return buff;
}

static uint64_t* fu_rand_buff_new_mt()
{
    if (FU_UNLIKELY(!defMT64)) {
        uint64_t* seed = fu_rand_new_seed();
        defMT64 = fu_malloc(sizeof(FURandMT64));
        atexit(fu_rand_mt_finalize);
        fu_rand_mt_init(seed);
        fu_free(seed);
    }
    uint64_t* buff = fu_malloc(DEF_RAND_BUFF_SIZE * sizeof(uint64_t));

    for (int i = 0; i < DEF_RAND_BUFF_SIZE; i++) {
        fu_rand_mt_next_state();
        buff[i] = fu_rand_mt_temper();
    }

    return buff;
}

static void fu_rand_finalize(FUObject* obj)
{
    FURand* self = (FURand*)obj;
    fu_free(self->buff);
}

static void fu_rand_class_init(FUObjectClass* oc)
{
    oc->finalize = fu_rand_finalize;
}

FURand* fu_rand_new(EFURandAlgorithm type)
{
    static const FPRandAlgorithm algs[] = {
        fu_rand_buff_new_os,
        fu_rand_buff_new_lcg,
        fu_rand_buff_new_mt,
        NULL
    };
    FPRandAlgorithm alg = algs[type];
    if (!alg)
        alg = algs[0];
    uint64_t* buff = alg();
    if (FU_UNLIKELY(!buff))
        return NULL;
    FURand* rand = (FURand*)fu_object_new(FU_TYPE_RAND);
    rand->alg = fu_steal_pointer(&alg);
    rand->buff = fu_steal_pointer(&buff);
    return rand;
}

int fu_rand_int_range(FURand* self, int min, int max)
{
    fu_return_val_if_fail(self && min < max, 0);
    self->idx += 1;
    if (FU_UNLIKELY(DEF_RAND_BUFF_SIZE <= self->idx)) {
        uint64_t* buff = self->alg();
        if (FU_LIKELY(buff)) {
            fu_free(self->buff);
            self->buff = fu_steal_pointer(&buff);
        }
        self->idx = 0;
    }
    return self->buff[self->idx++] % (max - min) + min;
}

//
// 计时器
// 精度：微秒
struct _FUTimer {
    FUObject parent;
    uint64_t prev;
};
// FU_DEFINE_TYPE(FUTimer, fu_timer, FU_TYPE_OBJECT)

static void fu_timer_class_init(FUObjectClass* oc)
{
}

uint64_t fu_timer_get_type()
{
    static uint64_t idx = 0;
    if (!idx) {
        FUObjectClass* parent = fu_type_get_class((fu_object_get_type()));
        if (!parent) {
            fu_log(EFU_LOG_LEVEL_ERROR, "unknown parent type: %lu\n", (fu_object_get_type()));
            exit(1);
        }
        FUObjectClass* oc = (FUObjectClass*)fu_malloc(sizeof(FUObjectClass));
        memcpy(oc, parent, sizeof(FUObjectClass));
        oc->size = sizeof(FUTimer);
        oc->parent = parent;
        oc->init = fu_timer_class_init;
        idx = oc->idx = fu_type_register(oc);
    }
    return idx;
}

FUTimer* fu_timer_new()
{
    return (FUTimer*)fu_object_new(FU_TYPE_TIMER);
}

void fu_timer_start(FUTimer* timer)
{
    timer->prev = fu_os_get_stmp();
}

/**
 * @brief 计算两个时间差
 * 精度: 微秒
 * @param obj
 * @return uint64_t
 */
uint64_t fu_timer_measure(FUTimer* timer)
{
    uint64_t curr = fu_os_get_stmp();
    uint64_t diff = curr - timer->prev;
    timer->prev = curr;
    return diff;
}

//
// 定时器事件源
// 精度：微秒
typedef struct _TTimeoutSource {
    FUSource parent;
    uint64_t prev;
    uint64_t dur;
} TTimeoutSource;

FU_DEFINE_TYPE(TTimeoutSource, t_timeout_source, FU_TYPE_SOURCE)

static void t_timeout_source_class_init(FUObjectClass* oc)
{
}

/**
 * @brief check if the timeout source should be triggered
 * precision: 1ms
 * @param src
 * @param usd
 * @return Check
 */
static bool t_timeout_source_check(TTimeoutSource* src, void* usd)
{
    uint64_t curr = fu_os_get_stmp();

    if (curr < src->prev + src->dur)
        return false;
    src->prev = curr;
    return true;
}

FUSource* fu_timeout_source_new_microseconds(unsigned dur)
{
    static const FUSourceFuncs fns = {
        .check = (FUSourceFunc)t_timeout_source_check
    };
    TTimeoutSource* src = (TTimeoutSource*)fu_source_init(fu_object_new(t_timeout_source_get_type()), &fns, NULL);
    src->prev = fu_os_get_stmp();
    src->dur = dur;
    return (FUSource*)src;
}

//
// 时间

char* fu_get_current_time_utc()
{
    struct timespec ts;
    char* rev = fu_malloc0(DEF_TIMER_FORMAT_BUFF_LEN);
    timespec_get(&ts, TIME_UTC);
    strftime(rev, DEF_TIMER_FORMAT_BUFF_LEN, "%F %T", gmtime(&ts.tv_sec));
    return rev;
}

char* fu_get_current_time_utc_format(const char* format)
{
    struct timespec ts;
    char* rev = fu_malloc0(DEF_TIMER_FORMAT_BUFF_LEN);
    timespec_get(&ts, TIME_UTC);
    strftime(rev, DEF_TIMER_FORMAT_BUFF_LEN, format, gmtime(&ts.tv_sec));
    return rev;
}

char* fu_get_current_time_local()
{
    struct timespec ts;
    char* rev = fu_malloc0(DEF_TIMER_FORMAT_BUFF_LEN);
    timespec_get(&ts, TIME_UTC);
    strftime(rev, DEF_TIMER_FORMAT_BUFF_LEN, "%x %X", localtime(&ts.tv_sec));
    return rev;
}

char* fu_get_current_time_local_format(const char* format)
{
    struct timespec ts;
    char* rev = fu_malloc0(DEF_TIMER_FORMAT_BUFF_LEN);
    timespec_get(&ts, TIME_UTC);
    strftime(rev, DEF_TIMER_FORMAT_BUFF_LEN, format, localtime(&ts.tv_sec));
    return rev;
}

//
// base64
// clang-format off
static const unsigned char mime_base64_rank[256] = {
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255, 62,255,255,255, 63,
   52, 53, 54, 55, 56, 57, 58, 59, 60, 61,255,255,255,  0,255,255,
  255,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
   15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,255,255,255,255,255,
  255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
   41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
};

static const char base64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
// clang-format on

/**
 * g_base64_encode_step:
 * @in: (array length=len) (element-type guint8): the binary data to encode
 * @len: the length of @in
 * @break_lines: whether to break long lines
 * @out: (out) (array) (element-type guint8): pointer to destination buffer
 * @state: (inout): Saved state between steps, initialize to 0
 * @save: (inout): Saved state between steps, initialize to 0
 *
 * Incrementally encode a sequence of binary data into its Base-64 stringified
 * representation. By calling this function multiple times you can convert
 * data in chunks to avoid having to have the full encoded data in memory.
 *
 * When all of the data has been converted you must call
 * g_base64_encode_close() to flush the saved state.
 *
 * The output buffer must be large enough to fit all the data that will
 * be written to it. Due to the way base64 encodes you will need
 * at least: (@len / 3 + 1) * 4 + 4 bytes (+ 4 may be needed in case of
 * non-zero state). If you enable line-breaking you will need at least:
 * ((@len / 3 + 1) * 4 + 4) / 76 + 1 bytes of extra space.
 *
 * @break_lines is typically used when putting base64-encoded data in emails.
 * It breaks the lines at 76 columns instead of putting all of the text on
 * the same line. This avoids problems with long lines in the email system.
 * Note however that it breaks the lines with `LF` characters, not
 * `CR LF` sequences, so the result cannot be passed directly to SMTP
 * or certain other protocols.
 *
 * Returns: The number of bytes of output that was written
 *
 * Since: 2.12
 */
static size_t fu_base64_encode_step(const uint8_t* in, size_t len, bool breakLines, char* out, int* state, int* save)
{
    fu_return_val_if_fail(in != NULL || !len, 0);
    fu_return_val_if_fail(out != NULL, 0);
    fu_return_val_if_fail(state != NULL, 0);
    fu_return_val_if_fail(save != NULL, 0);

    if (FU_UNLIKELY(!len))
        return 0;

    const uint8_t* inptr = in;
    char* outptr = out;

    if (len + ((char*)save)[0] > 2) {
        const uint8_t* inend = in + len - 2;
        int c1, c2, c3;
        int already = *state;

        switch (((char*)save)[0]) {
        case 1:
            c1 = ((unsigned char*)save)[1];
            goto skip1;
        case 2:
            c1 = ((unsigned char*)save)[1];
            c2 = ((unsigned char*)save)[2];
            goto skip2;
        }

        /*
         * yes, we jump into the loop, no i'm not going to change it,
         * it's beautiful!
         */
        while (inptr < inend) {
            c1 = *inptr++;
        skip1:
            c2 = *inptr++;
        skip2:
            c3 = *inptr++;
            *outptr++ = base64_alphabet[c1 >> 2];
            *outptr++ = base64_alphabet[c2 >> 4 | ((c1 & 0x3) << 4)];
            *outptr++ = base64_alphabet[((c2 & 0x0f) << 2) | (c3 >> 6)];
            *outptr++ = base64_alphabet[c3 & 0x3f];
            /* this is a bit ugly ... */
            if (breakLines && (++already) >= 19) {
                *outptr++ = '\n';
                already = 0;
            }
        }

        ((char*)save)[0] = 0;
        len = 2 - (inptr - inend);
        *state = already;
    }

    // g_assert (len == 0 || len == 1 || len == 2);

    /* points to the slot for the next char to save */
    char* saveout = &(((char*)save)[1]) + ((char*)save)[0];

    /* len can only be 0 1 or 2 */
    switch (len) {
    case 2:
        *saveout++ = *inptr++;
    case 1:
        *saveout++ = *inptr++;
    }
    ((char*)save)[0] += len;

    return outptr - out;
}

/**
 * g_base64_decode_step: (skip)
 * @in: (array length=len) (element-type guint8): binary input data
 * @len: max length of @in data to decode
 * @out: (out caller-allocates) (array) (element-type guint8): output buffer
 * @state: (inout): Saved state between steps, initialize to 0
 * @save: (inout): Saved state between steps, initialize to 0
 *
 * Incrementally decode a sequence of binary data from its Base-64 stringified
 * representation. By calling this function multiple times you can convert
 * data in chunks to avoid having to have the full encoded data in memory.
 *
 * The output buffer must be large enough to fit all the data that will
 * be written to it. Since base64 encodes 3 bytes in 4 chars you need
 * at least: (@len / 4) * 3 + 3 bytes (+ 3 may be needed in case of non-zero
 * state).
 *
 * Returns: The number of bytes of output that was written
 *
 * Since: 2.12
 **/
static size_t fu_base64_decode_step(const char* in, size_t len, uint8_t* out, int* state, uint32_t* save)
{
    //   const guchar *inptr;
    //   guchar *outptr;
    //   const guchar *inend;
    //   guchar c, rank;
    //   guchar last[2];
    //   unsigned int v;
    //   int i;

    fu_return_val_if_fail(in != NULL, 0);
    fu_return_val_if_fail(out != NULL, 0);
    fu_return_val_if_fail(state != NULL, 0);
    fu_return_val_if_fail(save != NULL, 0);

    if (FU_UNLIKELY(!len))
        return 0;

    const uint8_t* inend = (const uint8_t*)in + len;
    uint8_t* outptr = out;

    /* convert 4 base64 bytes to 3 normal bytes */
    uint32_t v = *save;
    int i = *state;

    uint8_t last[2] = { 0, 0 };

    /* we use the sign in the state to determine if we got a padding character
       in the previous sequence */
    if (i < 0) {
        i = -i;
        last[0] = '=';
    }

    const uint8_t* inptr = (const uint8_t*)in;
    uint8_t c, rank;
    while (inptr < inend) {
        c = *inptr++;
        rank = mime_base64_rank[c];
        if (rank != 0xff) {
            last[1] = last[0];
            last[0] = c;
            v = (v << 6) | rank;
            i++;
            if (i == 4) {
                *outptr++ = v >> 16;
                if (last[1] != '=')
                    *outptr++ = v >> 8;
                if (last[0] != '=')
                    *outptr++ = v;
                i = 0;
            }
        }
    }

    *save = v;
    *state = last[0] == '=' ? -i : i;

    return outptr - out;
}

/**
 * g_base64_encode_close:
 * @break_lines: whether to break long lines
 * @out: (out) (array) (element-type guint8): pointer to destination buffer
 * @state: (inout): Saved state from g_base64_encode_step()
 * @save: (inout): Saved state from g_base64_encode_step()
 *
 * Flush the status from a sequence of calls to g_base64_encode_step().
 *
 * The output buffer must be large enough to fit all the data that will
 * be written to it. It will need up to 4 bytes, or up to 5 bytes if
 * line-breaking is enabled.
 *
 * The @out array will not be automatically nul-terminated.
 *
 * Returns: The number of bytes of output that was written
 *
 * Since: 2.12
 */
static size_t fu_base64_encode_close(bool breakLines, char* out, int* state, int* save)
{
    fu_return_val_if_fail(out != NULL, 0);
    fu_return_val_if_fail(state != NULL, 0);
    fu_return_val_if_fail(save != NULL, 0);
    char* outptr = out;
    int c1 = ((unsigned char*)save)[1];
    int c2 = ((unsigned char*)save)[2];

    switch (((char*)save)[0]) {
    case 2:
        outptr[2] = base64_alphabet[((c2 & 0x0f) << 2)];
        //   g_assert (outptr [2] != 0);
        goto skip;
    case 1:
        outptr[2] = '=';
        c2 = 0; /* saved state here is not relevant */
    skip:
        outptr[0] = base64_alphabet[c1 >> 2];
        outptr[1] = base64_alphabet[c2 >> 4 | ((c1 & 0x3) << 4)];
        outptr[3] = '=';
        outptr += 4;
        break;
    }
    if (breakLines)
        *outptr++ = '\n';

    *save = 0;
    *state = 0;

    return outptr - out;
}

/**
 * g_base64_encode:
 * @data: (array length=len) (element-type guint8) (nullable): the binary data to encode
 * @len: the length of @data
 *
 * Encode a sequence of binary data into its Base-64 stringified
 * representation.
 *
 * Returns: (transfer full): a newly allocated, zero-terminated Base-64
 *               encoded string representing @data. The returned string must
 *               be freed with g_free().
 *
 * Since: 2.12
 */
char* fu_base64_encode(const uint8_t* data, size_t len)
{
    fu_return_val_if_fail(data != NULL || len == 0, NULL);
    /* We can use a smaller limit here, since we know the saved state is 0,
       +1 is needed for trailing \0, also check for unlikely integer overflow */
    fu_return_val_if_fail(len < ((~0UL - 1) / 4 - 1) * 3, NULL);
    int state = 0, save = 0;
    char* out = fu_malloc((len / 3 + 1) * 4 + 1);

    int outlen = fu_base64_encode_step(data, len, false, out, &state, &save);
    outlen += fu_base64_encode_close(false, out + outlen, &state, &save);
    out[outlen] = '\0';

    return out;
}

/**
 * g_base64_decode:
 * @text: (not nullable): zero-terminated string with base64 text to decode
 * @out_len: (out): The length of the decoded data is written here
 *
 * Decode a sequence of Base-64 encoded text into binary data.  Note
 * that the returned binary data is not necessarily zero-terminated,
 * so it should not be used as a character string.
 *
 * Returns: (transfer full) (array length=out_len) (element-type guint8):
 *               newly allocated buffer containing the binary data
 *               that @text represents. The returned buffer must
 *               be freed with g_free().
 *
 * Since: 2.12
 */
uint8_t* fu_base64_decode(const char* text, size_t* outLen)
{
    fu_return_val_if_fail(text && outLen, NULL);

    size_t input_length = strlen(text);

    /* We can use a smaller limit here, since we know the saved state is 0,
       +1 used to avoid calling g_malloc0(0), and hence returning NULL */
    uint8_t* ret = fu_malloc0((input_length / 4) * 3 + 1);
    uint32_t save = 0;
    int state = 0;
    *outLen = fu_base64_decode_step(text, input_length, ret, &state, &save);

    return ret;
}

//
// error

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

FUError* fu_error_new_from_code(int code)
{
#ifdef FU_OS_LINUX
    char* msg = strerror(code);
    if (FU_LIKELY(msg))
        return fu_error_new_take(code, &msg);
#elif FU_OS_WINDOW
    LPWSTR msgBuff = NULL;
    size_t len = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), msgBuff, 0, NULL);
    if (len) {
        char* msg = fu_wchar_to_utf8(msgBuff, &len);
        LocalFree(msgBuff);
        if (FU_LIKELY(msg))
            return fu_error_new_take(code, &msg);
    }
#endif
    FUError* err = fu_malloc(sizeof(FUError));
    err->message = fu_strdup("Unknown error");
    err->code = code;
    return err;
}
