#include <stdarg.h>
#include <stdatomic.h>
#include <string.h>

#include "platform/misc.linux.h"
#include "platform/misc.window.h"

#include "logger.h"
#include "memory.h"
#include "object.h"

//
// User Data
typedef struct _TUserData {
    char* key;
    void* data;
    FUNotify notify;
} TUserData;

void fu_value_init_ptr(FUValue* tar, EFUType type, void* val)
{
    fu_return_if_fail(tar);
    fu_return_if_fail(EFU_TYPE_CHAR_PTR <= type);
    tar->type = type;
    if (EFU_TYPE_INT8_PTR == type) {
        tar->value.pi8 = val;
        return;
    }
    if (EFU_TYPE_INT32_PTR == type) {
        tar->value.pi32 = val;
        return;
    }
    if (EFU_TYPE_INT64_PTR == type) {
        tar->value.pi64 = val;
        return;
    }
    if (EFU_TYPE_FLOAT_PTR == type) {
        tar->value.pf32 = val;
        return;
    }
    if (EFU_TYPE_DOUBLE_PTR == type) {
        tar->value.pf64 = val;
        return;
    }
    if (EFU_TYPE_UINT8_PTR == type) {
        tar->value.pui8 = val;
        return;
    }
    if (EFU_TYPE_UINT32_PTR == type) {
        tar->value.pui32 = val;
        return;
    }
    if (EFU_TYPE_UINT64_PTR == type) {
        tar->value.pui64 = val;
        return;
    }
    if (EFU_TYPE_CHAR_PTR == type) {
        tar->value.pc = val;
        tar->cnt = strlen(val);
        return;
    }
    if (EFU_TYPE_UCHAR_PTR == type) {
        tar->value.puc = val;
        tar->cnt = strlen(val);
        return;
    }
    tar->value.pv = val;
}

void fu_value_init_val(FUValue* tar, EFUType type, void* val)
{
    fu_return_if_fail(tar);
    fu_return_if_fail(EFU_TYPE_CHAR_PTR > type);
    tar->type = type;
    if (EFU_TYPE_CHAR == type) {
        tar->value.c = *(char*)val;
        return;
    }
    if (EFU_TYPE_INT8 == type) {
        tar->value.i8 = *(int8_t*)val;
        return;
    }
    if (EFU_TYPE_INT32 == type) {
        tar->value.i32 = *(int32_t*)val;
        return;
    }
    if (EFU_TYPE_INT64 == type) {
        tar->value.i64 = *(int64_t*)val;
        return;
    }
    if (EFU_TYPE_FLOAT == type) {
        tar->value.f32 = *(float*)val;
        return;
    }
    if (EFU_TYPE_DOUBLE == type) {
        tar->value.f64 = *(double*)val;
        return;
    }
    if (EFU_TYPE_UCHAR == type) {
        tar->value.uc = *(u_char*)val;
        return;
    }
    if (EFU_TYPE_UINT8 == type) {
        tar->value.ui8 = *(uint8_t*)val;
        return;
    }
    if (EFU_TYPE_UINT32 == type) {
        tar->value.ui32 = *(uint32_t*)val;
        return;
    }
    if (EFU_TYPE_UINT64 == type) {
        tar->value.ui64 = *(uint64_t*)val;
        return;
    }
    if (EFU_TYPE_SIZE == type) {
        tar->value.size = *(size_t*)val;
        return;
    }
    tar->value.ssize = *(ssize_t*)val;
}

void fu_value_init_str(FUValue* tar, char* val)
{
    tar->type = EFU_TYPE_CHAR_PTR;
    tar->cnt = strlen(val);
    tar->value.pc = val;
}

void fu_value_init_ch(FUValue* tar, char val)
{
    tar->type = EFU_TYPE_CHAR_PTR;
    tar->value.c = val;
    tar->cnt = 1;
}

void fu_value_init_int(FUValue* tar, int32_t val)
{
    tar->type = EFU_TYPE_INT32;
    tar->value.i32 = val;
}

void fu_value_init_uint(FUValue* tar, uint32_t val)
{
    tar->type = EFU_TYPE_UINT32;
    tar->value.ui32 = val;
}

void fu_value_init_int64(FUValue* tar, int64_t val)
{
    tar->type = EFU_TYPE_INT64;
    tar->value.i64 = val;
}

void fu_value_init_uint64(FUValue* tar, uint64_t val)
{
    tar->type = EFU_TYPE_UINT64;
    tar->value.ui64 = val;
}

void fu_value_init_float(FUValue* tar, float val)
{
    tar->type = EFU_TYPE_FLOAT;
    tar->value.f32 = val;
}

void fu_value_init_double(FUValue* tar, double val)
{
    tar->type = EFU_TYPE_DOUBLE;
    tar->value.f64 = val;
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
// Signal
typedef struct _TSignalHandler TSignalHandler;
typedef struct _TSignalHandler {
    TSignalHandler* next;
    TSignalHandler* prev;
    FUObject* host;
    FUCallback cb;
    uint32_t idx;
    void* usd;
    bool active;
} TSignalHandler;
typedef struct _FUSignal {
    char* name;
    bool withParam;
    FUObjectClass* oc;
    TSignalHandler* handlers;
} FUSignal;

static FUMutex defSignalMutex;
static bool ifSignalMutexInit = false;

//
// 真实的对象结构
// 用户看不到这个结构
// 大小需要和 FUObject 对齐
typedef struct _TObject {
    FUObjectClass* parent;
    FUHashTable* data;
    atomic_uint_fast32_t ref;
} TObject;
static_assert(sizeof(FUObject) == sizeof(TObject));

static FUObjectClass** defStaticObjectClass = NULL;
static uint64_t defStaticObjectClassAlloc = 9;
static uint64_t defStaticObjectClassCount = 0;

void fu_signal_free(FUSignal* sig)
{
    if (!sig)
        return;
    TSignalHandler* h = sig->handlers;
    while (h) {
        sig->handlers = h->next;
        fu_free(h);
        h = sig->handlers;
    }
    fu_free(sig->name);
    fu_free(sig);
}

/**
 * @brief 创建新信号
 * 信号不是对象，所有权归属 ClassObject
 * 注意：在相同对象上创建同名信号，会覆盖之前的信号
 * @param name 信号名称
 * @param oc 对象类
 * @param ret 返回值类型
 * @param argc 参数个数，如果非零，后面的参数为参数类型
 * @return FUSignal*
 */
FUSignal* fu_signal_new(const char* name, FUObjectClass* oc, bool withParam)
{
    fu_return_val_if_fail(name && oc, NULL);
    FUSignal* sig = fu_malloc0(sizeof(FUSignal));
    sig->name = fu_strdup(name);
    sig->withParam = withParam;
    sig->oc = oc;

    fu_hash_table_insert(sig->oc->signals, sig->name, sig);
    if (FU_UNLIKELY(!ifSignalMutexInit)) {
        fu_mutex_init(defSignalMutex);
        ifSignalMutexInit = true;
    }
    return sig;
}

uint64_t(fu_signal_connect)(FUObject* obj, const char* name, FUCallback cb, void* usd)
{
    fu_return_val_if_fail(obj && name && cb, ~0UL);
    FUObjectClass* oc = ((TObject*)obj)->parent;
    FUSignal* sig = fu_hash_table_lookup(oc->signals, name);
    if (FU_UNLIKELY(!sig)) {
        fu_warning("Signal '%s' not found", name);
        return ~0UL;
    }
    TSignalHandler* sh = fu_malloc0(sizeof(TSignalHandler));
    if (sig->handlers) {
        sig->handlers->prev = sh;
        sh->next = sig->handlers;
        sh->idx = sig->handlers->idx + 1;
    }
    sh->host = obj;
    sh->cb = cb;
    sh->usd = usd;
    sh->active = true;
    sig->handlers = sh;
    return sh->idx;
}

void fu_signal_emit(FUSignal* sig, ...)
{
    fu_return_if_fail(sig);
    if (!sig->handlers)
        return;
    va_list ap;
    va_start(ap, sig);

    if (sig->withParam) {
        void* p = va_arg(ap, void*);
        TSignalHandler* sh = sig->handlers;
        while (sh) {
            if (sh->active)
                sh->cb(sh->host, p, sh->usd);
            sh = sh->next;
        }
        goto out;
    }
    TSignalHandler* sh = sig->handlers;
    while (sh) {
        if (sh->active)
            sh->cb(sh->host, sh->usd);
        sh = sh->next;
    }
out:
    va_end(ap);
}

static void _fu_object_class_finalize()
{
    for (uint64_t i = 0; i < defStaticObjectClassCount; i++) {
        if (defStaticObjectClass[i]->destroy)
            defStaticObjectClass[i]->destroy(defStaticObjectClass[i]);
    }
    fu_free(defStaticObjectClass);
}

uint64_t fu_type_register(FUObjectClass* oc)
{
    if (!defStaticObjectClass) {
        defStaticObjectClass = (FUObjectClass**)fu_malloc(defStaticObjectClassAlloc * sizeof(FUObjectClass*));
        atexit(_fu_object_class_finalize);
    }
    if (defStaticObjectClassAlloc <= defStaticObjectClassCount) {
        if (defStaticObjectClassAlloc + 1 >= ~0UL >> 1) {
            fu_critical("Too many types\n");
            return 0;
        }
        defStaticObjectClassAlloc *= 2;
        defStaticObjectClass = (FUObjectClass**)fu_realloc(defStaticObjectClass, defStaticObjectClassAlloc * sizeof(FUObjectClass*));
    }
    if (oc->init)
        oc->init(oc);
    defStaticObjectClass[defStaticObjectClassCount] = oc;
    return defStaticObjectClassCount++;
}

static FUObject* _fu_object_class_constructor(FUObjectClass* oc)
{
    return fu_malloc0(oc->size);
}

static void _fu_object_class_destroy(FUObjectClass* oc)
{
    fu_hash_table_unref(oc->signals);
    fu_free(oc);
}

FUObjectClass* fu_type_get_class(uint64_t type)
{
    if (FU_LIKELY(type < defStaticObjectClassCount))
        return defStaticObjectClass[type];
    fu_error("unknown type: %lu\n", type);
    return NULL;
}

#ifndef NDEBUG
bool fu_type_check(void* obj, uint64_t type)
{
    if (!obj)
        return false;
    FUObjectClass* exp = defStaticObjectClass[type];
    if (FU_UNLIKELY(!exp)) {
        fu_error("unknown type: %lu\n", type);
        return false;
    }
    FUObjectClass* oc = ((TObject*)obj)->parent;
    do {
        if (oc == exp)
            return true;
        oc = oc->parent;
    } while (oc);
    fu_error("target is not a %s\n", exp->name);
    return false;
}

bool fu_object_type_check(void* obj)
{
    if (!obj)
        return false;
    FUObjectClass* oc = ((TObject*)obj)->parent;
    do {
        if (!oc->idx)
            return true;
        oc = oc->parent;
    } while (oc);

    fu_warning("Unknown object type: %p", obj);
    return false;
}
#endif
uint64_t fu_object_get_type()
{
    static uint64_t idx = ~0UL;
    if (~0UL <= idx) {
        FUObjectClass* oc = (FUObjectClass*)fu_malloc0(sizeof(FUObjectClass));
        oc->signals = fu_hash_table_new((FUHashFunc)fu_str_bkdr_hash, (FUNotify)fu_signal_free);
#ifndef NDEBUG
        oc->name = "FUObject";
#endif
        oc->size = sizeof(TObject);
        oc->constructor = _fu_object_class_constructor;
        oc->destroy = _fu_object_class_destroy;
        idx = oc->idx = fu_type_register(oc);
        fu_abort_if_fail(!idx);
    }
    return idx;
}

FUObject*(fu_object_ref)(FUObject* object)
{
    TObject* obj = (TObject*)object;
    atomic_fetch_add_explicit(&obj->ref, 1, memory_order_relaxed);
    return object;
}

void(fu_object_unref)(FUObject* object)
{
    if (!object)
        return;
    TObject* obj = (TObject*)object;
    /* obj_gtype will be needed for TRACE(GOBJECT_OBJECT_UNREF()) later. Note
     * that we issue the TRACE() after decrementing the ref-counter. If at that
     * point the reference counter does not reach zero, somebody else can race
     * and destroy the object.
     *
     * This means, TRACE() can be called with a dangling object pointer. This
     * could only be avoided, by emitting the TRACE before doing the actual
     * unref, but at that point we wouldn't know the correct "old_ref" value.
     * Maybe this should change.
     *
     * Anyway. At that later point we can also no longer safely get the GType
     * for the TRACE(). Do it now.
     */
    uint64_t old_ref = atomic_load_explicit(&obj->ref, memory_order_relaxed);
    if (1 < old_ref) {
        /* We have many references. If we can decrement the ref counter, we are done. */
        atomic_fetch_sub_explicit(&obj->ref, 1, memory_order_release);
        return;
    }

    if (!old_ref)
        return;
    /* At this point, we checked with an atomic read that we only hold only one
     * reference. Weak locations are cleared (and toggle references are not to
     * be considered in this case). Proceed with dispose().
     *
     * First, freeze the notification queue, so we don't accidentally emit
     * notifications during dispose() and finalize().
     *
     * The notification queue stays frozen unless the instance acquires a
     * reference during dispose(), in which case we thaw it and dispatch all the
     * notifications. If the instance gets through to finalize(), the
     * notification queue gets automatically drained when g_object_finalize() is
     * reached and the qdata is cleared.
     *
     * Important: Note that g_object_notify_queue_freeze() takes a
     * object_bit_lock(), which happens to be the same lock that is also taken
     * by toggle_refs_check_and_ref(), that is very important. See also the code
     * comment in toggle_refs_check_and_ref().
     */
    //  retry_decrement:
    /* Here, old_ref is 1 if we just come from dispose(). If the object was
     * resurrected, we can hit `goto retry_decrement` and be here with a larger
     * old_ref. */

    if (obj->parent->dispose)
        obj->parent->dispose(object);

    if (old_ref != obj->ref) {
        // obj->ref -= 1;
        atomic_fetch_sub_explicit(&obj->ref, 1, memory_order_release);
        return;
    }

    /* The object is almost gone. Finalize. */
    if (obj->parent->finalize)
        obj->parent->finalize(object);
    fu_hash_table_unref(obj->data);
    fu_free(obj);
}

FUObject* fu_object_new(uint64_t type)
{
    FUObjectClass* oc = fu_type_get_class(type);
    if (!oc)
        return NULL;
    FUObject* rev;
    TObject* obj = (TObject*)(rev = oc->constructor(oc));
    obj->data = fu_hash_table_new((FUHashFunc)fu_str_bkdr_hash, (FUNotify)t_usd_free);
    obj->parent = oc;
    atomic_init(&obj->ref, 1);
    return rev;
}

void(fu_object_set_user_data)(FUObject* obj, const char* key, void* data, FUNotify notify)
{
    TObject* real = (TObject*)obj;
    TUserData* usd = t_usd_new(key, data, notify);
    fu_hash_table_insert(real->data, usd->key, usd);
}

void*(fu_object_get_user_data)(FUObject* obj, const char* key)
{
    TObject* real = (TObject*)obj;
    TUserData* usd = fu_hash_table_lookup(real->data, key);
    if (usd)
        return usd->data;
    return NULL;
}
