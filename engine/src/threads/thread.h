#pragma once

#include "containers/queue.h"


typedef struct thread {
    void* internal_data;
    u64 thread_id;
    queue work_queue;
} thread;

typedef u32 (*PFN_thread_start)(void* args);

MAPI b8 thread_create(PFN_thread_start start_func, void* args, b8 auto_detach, thread* out_thread);

MAPI void thread_destroy(thread* t);

MAPI b8 thread_is_active(thread* t);

MAPI void thread_detach(thread* t);

MAPI void thread_cancel(thread* t);

MAPI b8 thread_wait(thread* t);

MAPI b8 thread_wait_timeout(thread* t,u64 ms);

MAPI void thread_sleep(thread* t, u64 ms);

MAPI u64 platform_current_thread_id(void);