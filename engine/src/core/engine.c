#include "engine.h"

#include "logger.h"
#include "game_types.h"
#include "platform/platform.h"
#include "memory/memory.h"
#include "core/event.h"
#include "core/input.h"

#include "renderer/renderer_frontend.h"

typedef struct engine_state {
    b8 is_running;
    b8 is_suspended;

    window* main_window;

    i32 width;
    i32 height;

    f64 last_time;

    game* game_instance;
} engine_state;

static engine_state state;

b8 engine_on_event(u16 code, void* sender, void* listener, event_context ctx) {
    switch (code) {
        case SYSTEM_EVENT_CODE_APPLICATION_QUIT:
            state.is_running = false;
            break;
        
        case SYSTEM_EVENT_CODE_WINDOW_CREATED:
            renderer_on_window_created(ctx.data.p[0]);
            break;

        case SYSTEM_EVENT_CODE_WINDOW_RESIZED:
            renderer_on_window_resized(ctx.data.p[1], ctx.data.u16[0], ctx.data.u16[1]);
            break;

        case SYSTEM_EVENT_CODE_WINDOW_DESTROYED:
            renderer_on_window_destroyed(ctx.data.p[0]);
            break;
        
        default:
            break;
    }

    return true;
}

b8 engine_initialize(game* game_instance) {
    state.game_instance = game_instance;
    state.main_window = nullptr;

    // Initialize subsystems
    memory_system_initialize();
    logging_system_initialize();
    event_system_initialize();
    input_system_initialize();

    platform_system_config plat_config;
    plat_config.app_name = game_instance->config.app_name;
    // plat_config.x = game_instance->config.window_x;
    // plat_config.y = game_instance->config.window_y;
    // plat_config.width = game_instance->config.window_width;
    // plat_config.height = game_instance->config.window_height;
    if (!platform_system_initialize(&plat_config)) {
        MFATAL("Failed to initialize platform!");
        return false;
    }

    event_register(SYSTEM_EVENT_CODE_APPLICATION_QUIT, nullptr, engine_on_event);
    event_register(SYSTEM_EVENT_CODE_WINDOW_CREATED, nullptr, engine_on_event);
    event_register(SYSTEM_EVENT_CODE_WINDOW_RESIZED, nullptr, engine_on_event);
    event_register(SYSTEM_EVENT_CODE_WINDOW_DESTROYED, nullptr, engine_on_event);

    if (!renderer_system_initialize(game_instance->config.app_name, (u16)(i16)game_instance->config.window_width, (u16)(i16)game_instance->config.window_height)) {
        MFATAL("Failed to initialize renderer system!");
        return false;
    }

    window_config win_config;
    win_config.name = "main";
    win_config.title = "Game Engine";
    win_config.x = 100;
    win_config.y = 100;
    win_config.width = game_instance->config.window_width;
    win_config.height = game_instance->config.window_height;
    state.main_window = memory_allocate(sizeof(window), MEMORY_TAG_PLATFORM);
    if (!platform_window_create(&win_config, state.main_window, true)) {
        MFATAL("Failed to create main window!");
        return false;
    }

    state.is_running = true;
    state.is_suspended = false;

    return true;
}


b8 engine_run() {
    while (state.is_running) {
        if (!platform_pump_messages()) {
            state.is_running = false;
        }

        if (!state.game_instance->update(state.game_instance, 0.0f)) {
            MERROR("game_update failed!");
        }

        if (!state.game_instance->render(state.game_instance, 0.0f)) {
            MERROR("game_render failed!");
        }

    }

    state.is_running = false;

    platform_window_destroy(state.main_window);
    memory_free(state.main_window, sizeof(window), MEMORY_TAG_PLATFORM);
    state.main_window = nullptr;

    renderer_system_shutdown();
    platform_system_shutdown();
    input_system_shutdown();
    event_system_shutdown();
    logging_system_shutdown();
    memory_system_shutdown();

    MTRACE("Goodbye!");

    return true;
}