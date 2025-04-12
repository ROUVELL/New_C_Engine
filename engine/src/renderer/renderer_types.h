#pragma once

#include "defines.h"

typedef enum renderer_backend_type {
    RENDERER_BACKEND_TYPE_VULKAN = 0x1,
    // RENDERER_BACKEND_TYPE_OPENGL = 0x2
} renderer_backend_type;

typedef enum renderer_config_flag_bits {
    RENDERER_CONFIG_FLAG_VSYNC_ENABLED_BIT = 0x1,
    RENDERER_CONFIG_FLAG_POWER_SAVING_BIT = 0x2,
    RENDERER_CONFIG_FLAG_ENABLE_VALIDATION = 0x4
} renderer_config_flag_bits;

typedef u32 renderer_config_flags;

struct window;

typedef struct renderer_backend {
    b8 (*initialize)(const char* app_name, u16 window_width, u16 window_height);
    void (*shutdown)(void);

    void (*on_window_created)(struct window* w);
    void (*on_window_resized)(struct window* w, u16 width, u16 height);
    void (*on_window_destroyed)(struct window* w);
} renderer_backend;

struct window;
struct window_renderer_backend_state;

typedef struct window_renderer_state {
    struct window* window;

    struct window_renderer_backend_state* backend_state;
} window_renderer_state;