#include <stdlib.h>
#include <time.h>

#include "rand.h"
#include "utils.h"

#define DEF_RAND_BUFF_SIZE 120
#define DEF_RAND_SEED_SIZE 7

/* 线性同余常用参数 */
#define DEF_RAND_LCG_A 0x0014096b
#define DEF_RAND_LCG_C 0xbaa40beb
#define DEF_RAND_LCG_M 0xffffffff

/* Period parameters */
#define DEF_RAND_MT_LOWER_MASK 0x7fffffff /* least significant r bits */

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

#ifdef FU_OS_WINDOW
#include <bcrypt.h>
#include <windows.h>

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
    fu_winapi_return_val_if_fail(!BCryptOpenAlgorithmProvider(&alg, L"RNG", NULL, 0), NULL);

    ifOpenAlg = true;
    size_t len = DEF_RAND_BUFF_SIZE * sizeof(uint64_t);
    uint64_t* buff = fu_malloc(len);
    fu_winapi_goto_if_fail(!BCryptGenRandom(alg, (PUCHAR)buff, len, 0), out);
    BCryptCloseAlgorithmProvider(alg, 0);
    return buff;
out:
    if (ifOpenAlg) {
        fu_free(buff);
        BCryptCloseAlgorithmProvider(alg, 0);
    }
    return NULL;
}
#else
#include <sys/random.h>
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
