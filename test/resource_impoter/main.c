#include "core/atomic.h"
#include "core/buddy.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// #define FU_USE_BUILTIN_ATOMIC
// #include "atomic.h"
static int main2()
{
    size_t arena_size = 65536;
    typedef struct _test {
        int a, b;
    } test_t;
    /* You need space for arena and builtin metadata */
    void* buddy_arena = malloc(arena_size);
    struct buddy* buddy = buddy_embed(buddy_arena, arena_size);
    test_t** arr = buddy_malloc(buddy, 30 * sizeof(int));
    for (int i = 0; i < 30; i++) {
        arr[i] = buddy_malloc(buddy, sizeof(test_t));
        arr[i]->a = i;
        arr[i]->b = i * 2;
    }

    /* Allocate using the buddy allocator */
    void* data = buddy_malloc(buddy, 2048);
    /* Free using the buddy allocator */
    buddy_free(buddy, data);

    free(buddy_arena);
}
static atomic_int shared_counter;

// 线程函数，用于增加共享计数器
void* inc_counter(void* arg)
{
    for (int i = 0; i < 100000; ++i) {
        // atomic_fetch_add(&shared_counter, 1); // 安全地增加计数器
        atomic_fetch_add(&shared_counter, 1);
        // shared_counter++;
    }
    return NULL;
}

void* dec_counter(void* arg)
{
    for (int i = 0; i < 1000; ++i) {
        // atomic_fetch_add(&shared_counter, 1); // 安全地增加计数器
        atomic_fetch_sub(&shared_counter, 1);
        // shared_counter++;
    }
    return NULL;
}

int main()
{
    pthread_t threads[20];
    int result;

    // 初始化原子变量
    atomic_init(&shared_counter, 0);

    // 创建多个线程
    for (int i = 0; i < 10; ++i) {
        result = pthread_create(&threads[i], NULL, inc_counter, NULL);
        result *= pthread_create(&threads[10 + i], NULL, dec_counter, NULL);
        if (result != 0) {
            fprintf(stderr, "Thread creation failed: %d\n", result);
            return 1;
        }
    }

    // 等待所有线程完成
    for (int i = 0; i < 20; ++i) {
        pthread_join(threads[i], NULL);
    }

    // 输出最终的计数器值
    // printf("Final counter value: %d\n", atomic_load(&shared_counter));
    printf("Final counter value: %d\n", atomic_load(&shared_counter));

    return 0;
}