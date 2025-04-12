#pragma once

#include "defines.h"

typedef enum system_event_code {
    SYSTEM_EVENT_CODE_APPLICATION_QUIT = 1,

    SYSTEM_EVENT_CODE_KEY_PRESSED = 2,
    SYSTEM_EVENT_CODE_KEY_RELEASED = 3,
    SYSTEM_EVENT_CODE_BUTTON_PRESSED = 4,
    SYSTEM_EVENT_CODE_BUTTON_RELEASED = 5,
    SYSTEM_EVENT_CODE_MOUSE_MOVE = 6,
    SYSTEM_EVENT_CODE_MOUSE_WHEEL = 7,
    // SYSTEM_EVENT_CODE_MOUSE_LEAVE

    SYSTEM_EVENT_CODE_WINDOW_CREATED = 10,
    SYSTEM_EVENT_CODE_WINDOW_RESIZED = 11,
    SYSTEM_EVENT_CODE_WINDOW_DESTROYED = 12,

    SYSTEM_EVENT_MAX_CODE = 255
} system_event_code;

typedef struct event_context {
    // 128 bytes
    union {
        i64 i64[2];
        u64 u64[2];
        f64 f64[2];

        i32 i32[4];
        u32 u32[4];
        f32 f32[4];

        i16 i16[8];
        u16 u16[8];

        i8 i8[16];
        u8 u8[16];
        b8 b8[16];

        char c[16];
        void* p[2];
    } data;
} event_context;

typedef b8 (*PFN_on_event)(u16 code, void* sender, void* listener, event_context ctx);

b8 event_system_initialize();
void event_system_shutdown();

MAPI b8 event_register(u16 code, void* listener, PFN_on_event on_event);

MAPI b8 event_unregister(u16 code, void* listener, PFN_on_event on_event);

MAPI b8 event_fire(u16 code, void* sender, event_context ctx);