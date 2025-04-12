#pragma once

#include "core/logger.h"
#include "defines.h"

typedef struct platform_system_config {
    const char* app_name;
} platform_system_config;

typedef struct window_config {
    const char* name;
    const char* title;

    i32 x;
    i32 y;
    u32 width;
    u32 height;
} window_config;

struct window_platform_state;
struct window_renderer_state;

typedef struct window {
    const char* name;
    const char* title;

    u16 width;
    u16 height;

    f32 device_pixel_ratio;

    b8 resizing;
    u16 frames_since_resize;

    struct window_platform_state* platform_state;
    struct window_renderer_state* renderer_state;
} window;

b8 platform_system_initialize(const platform_system_config* config);
void platform_system_shutdown();
b8 platform_pump_messages();

b8 platform_window_create(const window_config* config, struct window* out_window, b8 show_immediately);

void platform_window_destroy(struct window* w);

b8 platform_window_show(struct window* w);

b8 platform_window_hide(struct window* w);

void platform_get_handle_info(u64* out_size, void* memory);

MAPI void platform_console_write(log_level level, const char* msg);

MAPI f64 platform_get_absolute_time();

MAPI void platform_sleep(u64 ms);