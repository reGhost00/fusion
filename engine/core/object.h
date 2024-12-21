#pragma once

#include <stdint.h>
#include <stdlib.h> // for FU_DEFINE_TYPE
// #include <string.h> // for FU_DEFINE_TYPE

// #include "logger.h" // for FU_DEFINE_TYPE
// #include "memory.h" // for FU_DEFINE_TYPE
#include "hash2.h"

typedef enum _EFUType {
    EFU_TYPE_NONE,
    EFU_TYPE_CHAR,
    EFU_TYPE_INT8,
    EFU_TYPE_INT32,
    EFU_TYPE_INT64,
    EFU_TYPE_FLOAT,
    EFU_TYPE_DOUBLE,
    EFU_TYPE_UCHAR,
    EFU_TYPE_UINT8,
    EFU_TYPE_UINT32,
    EFU_TYPE_UINT64,
    EFU_TYPE_SIZE,
    EFU_TYPE_SSIZE,
    EFU_TYPE_CHAR_PTR,
    EFU_TYPE_INT8_PTR,
    EFU_TYPE_INT32_PTR,
    EFU_TYPE_INT64_PTR,
    EFU_TYPE_FLOAT_PTR,
    EFU_TYPE_DOUBLE_PTR,
    EFU_TYPE_UCHAR_PTR,
    EFU_TYPE_UINT8_PTR,
    EFU_TYPE_UINT32_PTR,
    EFU_TYPE_UINT64_PTR,
    EFU_TYPE_VOID_PTR,
    EFU_TYPE_CNT
} EFUType;
typedef union _FUType {
    char c;
    int8_t i8;
    int32_t i32;
    int64_t i64;
    float f32;
    double f64;
    unsigned char uc;
    uint8_t ui8;
    uint32_t ui32;
    uint64_t ui64;
    size_t size;
    ssize_t ssize;
    char* pc;
    int8_t* pi8;
    int32_t* pi32;
    int64_t* pi64;
    float* pf32;
    double* pf64;
    unsigned char* puc;
    uint8_t* pui8;
    uint32_t* pui32;
    uint64_t* pui64;
    void* pv;
} FUType;
typedef struct _FUValue {
    alignas(sizeof(void*)) FUType value;
    alignas(sizeof(void*)) EFUType type;
    alignas(sizeof(void*)) uint32_t cnt;
} FUValue;

#ifdef __cplusplus
extern "C" {
#endif

void fu_value_init_ptr(FUValue* tar, EFUType type, void* val);
void fu_value_init_val(FUValue* tar, EFUType type, void* val);
void fu_value_init_str(FUValue* tar, char* val);
void fu_value_init_ch(FUValue* tar, char val);
void fu_value_init_int(FUValue* tar, int32_t val);
void fu_value_init_uint(FUValue* tar, uint32_t val);
void fu_value_init_int64(FUValue* tar, int64_t val);
void fu_value_init_uint64(FUValue* tar, uint64_t val);
void fu_value_init_float(FUValue* tar, float val);
void fu_value_init_double(FUValue* tar, double val);

typedef struct _FUObjectClass FUObjectClass;
typedef struct _FUObject FUObject;
typedef struct _FUSignal FUSignal;
typedef void (*FUCallback)(FUObject* obj, ...);
FUSignal* fu_signal_new(const char* name, FUObjectClass* oc, bool withParam);
void fu_signal_free(FUSignal* sig);

uint64_t fu_signal_connect(FUObject* obj, const char* name, FUCallback cb, void* usd);
void fu_signal_emit(FUSignal* sig, ...);
#define fu_signal_connect(obj, name, cb, usd) fu_signal_connect(fu_object_type(obj), name, (FUCallback)cb, usd)
/**
 * @brief 类类型描述
 * 每个类型都有其描述
 */
struct _FUObjectClass {
    FUObjectClass* parent;
    uint64_t idx, size;
    const char* name;
    void (*init)(FUObjectClass* oc);
    FUHashTable* signals;
    FUObject* (*constructor)(FUObjectClass* oc);
    void (*destroy)(FUObjectClass* oc);
    void (*dispose)(FUObject* obj);
    void (*finalize)(FUObject* obj);
};

/**
 * @brief 对象基类
 */
struct _FUObject {
    /** @private
     * 每个对象都有其描述，但是不给看
     */
    void* dummy[3];
};

#define FU_TYPE_OBJECT (fu_object_get_type())
#ifndef NDEBUG
#define FU_DECLARE_TYPE(TN, t_n)                                                             \
    uint64_t t_n##_get_type(void);                                                           \
    typedef struct _##TN TN;                                                                 \
    static bool t_n##_type_check(void* obj) { return fu_type_check(obj, t_n##_get_type()); } \
    static TN* t_n##_type(void* obj) { return t_n##_type_check(obj) ? (TN*)obj : NULL; }

#define FU_DEFINE_TYPE(TN, t_n, T_N)                                                                 \
    typedef struct _##TN TN;                                                                         \
    static void t_n##_class_init(FUObjectClass* oc);                                                 \
    uint64_t t_n##_get_type()                                                                        \
    {                                                                                                \
        static uint64_t idx = 0;                                                                     \
        if (!idx) {                                                                                  \
            FUObjectClass* parent = fu_type_get_class(T_N);                                          \
            if (!parent) {                                                                           \
                fu_error("unknown parent type: %lu\n", T_N);                                         \
                exit(1);                                                                             \
            }                                                                                        \
            FUObjectClass* oc = (FUObjectClass*)fu_memdup(parent, sizeof(FUObjectClass));            \
            oc->signals = fu_hash_table_new((FUHashFunc)fu_str_bkdr_hash, (FUNotify)fu_signal_free); \
            oc->name = #TN;                                                                          \
            oc->size = sizeof(TN);                                                                   \
            oc->parent = parent;                                                                     \
            oc->init = t_n##_class_init;                                                             \
            idx = oc->idx = fu_type_register(oc);                                                    \
        }                                                                                            \
        return idx;                                                                                  \
    }
#else

#define FU_DECLARE_TYPE(TN, t_n)                              \
    uint64_t t_n##_get_type(void);                            \
    typedef struct _##TN TN;                                  \
    static bool t_n##_type_check(void* obj) { return !!obj; } \
    static TN* t_n##_type(void* obj) { return (TN*)obj; }

#define FU_DEFINE_TYPE(TN, t_n, T_N)                                                                 \
    typedef struct _##TN TN;                                                                         \
    static void t_n##_class_init(FUObjectClass* oc);                                                 \
    uint64_t t_n##_get_type()                                                                        \
    {                                                                                                \
        static uint64_t idx = 0;                                                                     \
        if (!idx) {                                                                                  \
            FUObjectClass* parent = fu_type_get_class(T_N);                                          \
            if (!parent) {                                                                           \
                fu_error("unknown parent type: %lu\n", T_N);                                         \
                exit(1);                                                                             \
            }                                                                                        \
            FUObjectClass* oc = (FUObjectClass*)fu_memdup(parent, sizeof(FUObjectClass));            \
            oc->signals = fu_hash_table_new((FUHashFunc)fu_str_bkdr_hash, (FUNotify)fu_signal_free); \
            oc->size = sizeof(TN);                                                                   \
            oc->parent = parent;                                                                     \
            oc->init = t_n##_class_init;                                                             \
            idx = oc->idx = fu_type_register(oc);                                                    \
        }                                                                                            \
        return idx;                                                                                  \
    }
#endif
uint64_t fu_type_register(FUObjectClass* oc);
FUObjectClass* fu_type_get_class(uint64_t type);
uint64_t fu_object_get_type();
#ifndef NDEBUG
bool fu_type_check(void* obj, uint64_t type);
bool fu_object_type_check(void* obj);
#define fu_object_type(obj) (fu_object_type_check(obj) ? (FUObject*)obj : NULL)
#else
#define fu_type_check(obj, type) (true)
#define fu_object_type_check(obj) (true)
#define fu_object_type(obj) ((FUFUObject*)obj)
#endif

FUObject* fu_object_new(uint64_t type);
FUObject* fu_object_ref(FUObject* obj);
void fu_object_unref(FUObject* obj);
void fu_object_set_user_data(FUObject* obj, const char* key, void* data, FUNotify notify);
void* fu_object_get_user_data(FUObject* obj, const char* key);

#define fu_object_set_user_data(obj, key, data, notify) fu_object_set_user_data(fu_object_type(obj), key, data, (FUNotify)notify)
#define fu_object_get_user_data(obj, key) fu_object_get_user_data(fu_object_type(obj), key)
#define fu_object_ref(obj) ((typeof(obj))fu_object_ref(fu_object_type(obj)))
#define fu_object_unref(obj) (fu_object_unref(fu_object_type(obj)))
#ifdef __cplusplus
}
#endif