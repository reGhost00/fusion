#ifndef _FU_OBJECT_H_
#define _FU_OBJECT_H_

// #include "sc_map.h"
#include <stdint.h>

#include "hash2.h"

typedef struct _FUTypeInfo FUTypeInfo;
typedef struct _FUObject FUObject;

typedef struct _FUObjectClass FUObjectClass;

// typedef void (*FUNotify)(void* usd);
typedef void (*FUSignalCallback0)(FUObject* obj, void* usd);
typedef void (*FUSignalCallback1)(FUObject* obj, const void* res, void* usd);
typedef void (*FUObjectFunc)(FUObject* obj);
typedef void (*FUObjectClassFunc)(FUObjectClass* oc);
/**
 * @brief 类类型描述
 * 每个类型都有其描述
 */
struct _FUObjectClass {
    FUObjectClass* parent;
    uint64_t idx;
    FUTypeInfo* info;
    FUHashTable* signals;
    FUObjectFunc dispose;
    FUObjectFunc finalize;
};

/**
 * @brief 对象基类
 */
typedef struct _FUObject {
    /** @private
     * 每个对象都有其描述，但是不给看
     */
    void* dummy[3];
} FUObject;

typedef void (*FUTypeInfoDestroyFunc)(FUTypeInfo* type);
//
// 信号

/** 无参数信号 */
typedef struct _FUSignal FUSignal;

/**
 * @brief 类型描述详情
 * 万物均有规律
 */
struct _FUTypeInfo {
    /** 对象大小，包括用户定义的成员 */
    unsigned size;
    /** 父对象 */
    uint64_t parent;
    FUObjectClassFunc init;
    FUTypeInfoDestroyFunc destroy;
};
#define FU_TYPE_OBJECT 0
#define FU_DECLARE_TYPE(TN, t_n) \
    uint64_t t_n##_get_type();   \
    typedef struct _##TN TN;

#define FU_DEFINE_TYPE(TN, t_n, t_p)                           \
    static void t_n##_class_init(FUObjectClass* oc);           \
    uint64_t t_n##_get_type()                                  \
    {                                                          \
        static uint64_t tid = 0;                               \
        if (!tid) {                                            \
            FUTypeInfo* info = fu_malloc0(sizeof(FUTypeInfo)); \
            info->size = sizeof(TN);                           \
            info->init = t_n##_class_init;                     \
            tid = fu_type_register(info, t_p);                 \
        }                                                      \
        return tid;                                            \
    }

uint64_t fu_type_register(FUTypeInfo* info, uint64_t parentType);
FUSignal* fu_signal_new(const char* name, FUObjectClass* oc, bool withParam);
void fu_signal_emit(FUSignal* sig);
void fu_signal_emit_with_param(FUSignal* sig, const void* param);

uint64_t fu_signal_connect(FUObject* obj, const char* name, FUSignalCallback0 cb, void* usd);
uint64_t fu_signal_connect_with_param(FUObject* obj, const char* name, FUSignalCallback1 cb, void* usd);

FUObject* fu_object_new(uint64_t type);
FUObject* fu_object_ref(FUObject* obj);
void fu_object_unref(FUObject* obj);
#define fu_object_ref(obj) ((typeof(obj))fu_object_ref((FUObject*)obj))
#define fu_object_unref(obj) (fu_object_unref((FUObject*)obj))

void fu_object_set_user_data(FUObject* obj, const char* key, void* data, FUNotify notify);
void* fu_object_get_user_data(FUObject* obj, const char* key);

#define fu_object_set_user_data(obj, key, data, notify) fu_object_set_user_data((FUObject*)obj, key, data, notify)
#define fu_object_get_user_data(obj, key) fu_object_get_user_data((FUObject*)obj, key)

#endif /* _FU_OBJECT_H_ */