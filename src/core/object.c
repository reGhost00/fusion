#include <stdatomic.h>
// custom header
#include "array.h"
#include "object.h"
#include "utils.h"

static FUPtrArray* defObjectClassArray = NULL;

//
// 真实的对象结构
// 用户看不到这个结构
// 大小需要和 FUObject 对齐
typedef struct _TObject {
    FUObjectClass* parent;
    FUHashTable* data;
    atomic_int ref;
} TObject;
static_assert(sizeof(FUObject) == sizeof(TObject));
static_assert(alignof(FUObject) >= alignof(TObject));
//
// Signal
typedef struct _TSignalCallback TSignalCallback;
struct _TSignalCallback {
    TSignalCallback* next;
    TSignalCallback* prev;
    FUSignalCallback0 cb0;
    FUSignalCallback1 cb1;
    uint64_t idx;
    TObject* src;
    void* usd;
};

struct _FUSignal {
    /** 信号名称 */
    char* name;
    bool active;
    /** 信号是否带参数 */
    bool withParam;
    /** 信号对应的回调函数 */
    TSignalCallback* cbs;
};

//
// User Data
typedef struct _TUserData {
    char* key;
    void* data;
    FUNotify notify;
} TUserData;
//
// Signal
//
// 创建新信号，信号不是 Object，且信号注册后无需释放
FUSignal* fu_signal_new(const char* name, FUObjectClass* oc, bool withParam)
{
    FUSignal* sig = fu_malloc0(sizeof(FUSignal));
    sig->name = fu_strdup(name);
    sig->withParam = withParam;
    sig->cbs = NULL;
    // fu_hash_table_replace(oc->signals, sig->name, sig);
    fu_hash_table_insert(oc->signals, sig->name, sig);
    return sig;
}

static void fu_signal_free(FUSignal* sig)
{
    TSignalCallback* scb;
    while (sig->cbs) {
        scb = sig->cbs->next;
        fu_free(sig->cbs);
        sig->cbs = scb;
    }
    fu_free(sig->name);
    fu_free(sig);
}

void fu_signal_emit(FUSignal* sig)
{
    fu_return_if_fail(sig);
    TSignalCallback* scb = sig->cbs;
    while (scb) {
        if (scb->cb0)
            scb->cb0((FUObject*)scb->src, scb->usd);
        scb = scb->next;
    }
}

void fu_signal_emit_with_param(FUSignal* sig, const void* param)
{
    fu_return_if_fail(sig);
    TSignalCallback* scb = sig->cbs;
    while (scb) {
        if (scb->cb1)
            scb->cb1((FUObject*)scb->src, param, scb->usd);
        scb = scb->next;
    }
}

uint64_t fu_signal_connect(FUObject* obj, const char* name, FUSignalCallback0 cb, void* usd)
{
    TObject* real = (TObject*)obj;
    FUObjectClass* oc = real->parent;
    FUSignal* sig = fu_hash_table_lookup(oc->signals, name);
    while (!sig) {
        // if (!(oc = oc->parent))
        //     fu_return_val_if_fail_with_message(oc && real, "Signal not registered", 0);
        oc = oc->parent;
        fu_return_val_if_fail_with_message(oc, "Signal not registered", 0);
        sig = fu_hash_table_lookup(oc->signals, name);
    }

    TSignalCallback* scb = fu_malloc0(sizeof(TSignalCallback));
    if (sig->cbs) {
        sig->cbs->prev = scb;
        scb->idx = sig->cbs->idx + 1;
    } else
        scb->idx = 1;
    scb->next = sig->cbs;
    scb->src = real;
    scb->cb0 = cb;
    scb->usd = usd;
    sig->cbs = scb;
    return scb->idx;
}

uint64_t fu_signal_connect_with_param(FUObject* obj, const char* name, FUSignalCallback1 cb, void* usd)
{
    TObject* real = (TObject*)obj;
    FUObjectClass* oc = real->parent;
    FUSignal* sig = fu_hash_table_lookup(oc->signals, name);
    fu_return_val_if_fail_with_message(sig, "Signal not registered", 0);
    TSignalCallback* scb = fu_malloc0(sizeof(TSignalCallback));
    if (sig->cbs) {
        sig->cbs->prev = scb;
        scb->idx = sig->cbs->idx + 1;
    } else
        scb->idx = 1;
    scb->next = sig->cbs;
    scb->src = real;
    scb->cb1 = cb;
    scb->usd = usd;
    sig->cbs = scb;
    return scb->idx;
}

//
// Object Class
static inline FUObjectClass* t_oc_new(FUTypeInfo** info)
{
    FUObjectClass* oc = fu_malloc0(sizeof(FUObjectClass));
    oc->signals = fu_hash_table_new((FUHashFunc)fu_str_bkdr_hash, (FUNotify)fu_signal_free);
    oc->idx = defObjectClassArray->len;
    oc->info = fu_steal_pointer(info);
    return oc;
}

static void t_oc_free(FUObjectClass* oc)
{
    // 例外
    // oc 中含有用户创建的 info
    // if (FU_LIKELY(oc->info && oc->info->destroy))
    //     oc->info->destroy(oc->info);
    // else if (oc->info)
    //     fu_free(oc->info);
    if (oc->info->destroy)
        oc->info->destroy(oc->info);
    else
        fu_free(oc->info);
    fu_hash_table_unref(oc->signals);
    fu_free(oc);
}

//
// User Data
static TUserData* t_usd_new(const char* key, void* data, FUNotify notify)
{
    TUserData* usd = fu_malloc(sizeof(TUserData));
    usd->key = fu_strdup(key);
    usd->data = data;
    usd->notify = notify;
    return usd;
}

static void t_usd_free(TUserData* usd)
{
    if (usd->notify)
        usd->notify(usd->data);
    fu_free(usd->key);
    fu_free(usd);
}
//
// Type
static void fu_type_finalize()
{
    if (FU_UNLIKELY(!defObjectClassArray))
        return;
    fu_ptr_array_unref(defObjectClassArray);
}

static void fu_object_finalize(FUObject* obj)
{
    TObject* real = (TObject*)obj;
    FUObjectClass* oc = real->parent;
    do {
        if (oc->finalize)
            oc->finalize(obj);
        oc = oc->parent;
    } while (oc);
    fu_hash_table_unref(real->data);
    fu_free(obj);
}

uint64_t fu_type_register(FUTypeInfo* info, uint64_t parentType)
{
    if (FU_UNLIKELY(!defObjectClassArray)) {
        defObjectClassArray = fu_ptr_array_new_full(9, (FUNotify)t_oc_free);
        FUTypeInfo* baseInfo = fu_malloc0(sizeof(FUTypeInfo));
        FUObjectClass* baseOC = t_oc_new(&baseInfo);
        fu_ptr_array_push(defObjectClassArray, baseOC);
        atexit(fu_type_finalize);
    }

    FUObjectClass* oc = t_oc_new(&info);
    if (parentType)
        parentType -= 1;
    oc->parent = fu_ptr_array_index(defObjectClassArray, parentType);
    if (FU_LIKELY(oc->info->init))
        oc->info->init(oc);

    fu_ptr_array_push(defObjectClassArray, oc);

    return defObjectClassArray->len;
}

FUObject* fu_object_new(uint64_t type)
{
    fu_return_val_if_fail(type && type <= defObjectClassArray->len, NULL);
    FUObjectClass* oc = fu_ptr_array_index(defObjectClassArray, type - 1);
    fu_return_val_if_fail_with_message(oc, "Type not registered", NULL);

    unsigned size = oc->info->size;
    if (FU_UNLIKELY(sizeof(FUObject) > size))
        size = sizeof(FUObject);
    TObject* obj = fu_malloc0(size);
    obj->data = fu_hash_table_new((FUHashFunc)fu_str_bkdr_hash, (FUNotify)t_usd_free);
    obj->parent = oc;
    atomic_init(&obj->ref, 1);
    return (FUObject*)obj;
}
#undef fu_object_ref
FUObject* fu_object_ref(FUObject* obj)
{
    TObject* real = (TObject*)obj;
    atomic_fetch_add(&real->ref, 1);
    return obj;
}
#undef fu_object_unref
void fu_object_unref(FUObject* obj)
{
    if (!obj)
        return;
    TObject* real = (TObject*)obj;

    if (real->parent->dispose)
        real->parent->dispose(obj);

    if (1 >= atomic_fetch_sub(&real->ref, 1))
        fu_object_finalize(obj);
}

void fu_object_set_user_data(FUObject* obj, const char* key, void* data, FUNotify notify)
{
    TObject* real = (TObject*)obj;
    TUserData* usd = t_usd_new(key, data, notify);
    fu_hash_table_insert(real->data, usd->key, usd);
}

void* fu_object_get_user_data(FUObject* obj, const char* key)
{
    TObject* real = (TObject*)obj;
    TUserData* usd = fu_hash_table_lookup(real->data, key);
    if (usd)
        return usd->data;
    return NULL;
}
