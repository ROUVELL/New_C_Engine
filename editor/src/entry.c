
#include <entry.h>
#include <core/logger.h>
#include <memory/memory.h>

#include <math/vec3.h>

#include <core/event.h>
#include <core/input.h>

b8 game_on_key_up(u16 code, void* sender, void* listener, event_context ctx) {
    if (code == SYSTEM_EVENT_CODE_KEY_RELEASED && ctx.data.u16[0] == KEY_ESCAPE) {
        event_context new_ctx;
        event_fire(SYSTEM_EVENT_CODE_APPLICATION_QUIT, nullptr, new_ctx);
    
        return true;
    }

    return false;
}

b8 game_initialize(game* instance) {
    event_register(SYSTEM_EVENT_CODE_KEY_RELEASED, nullptr, game_on_key_up);

    MINFO("Game initialized!");
    MDEBUG(memory_get_usage_str());
    return true;
}

b8 game_update(game* instance, f32 dt) {
    return true;
}

b8 game_render(game* instance, f32 dt) {
    return true;
}

void game_on_resize(game* instance, u32 width, u32 height) {
    MDEBUG("Game resized! %ux%u", width, height);
}

b8 create_game(game* out_game) {
    out_game->config.app_name = "My game";
    out_game->config.window_width = 900;
    out_game->config.window_height = 600;

    out_game->initialize = game_initialize;
    out_game->update = game_update;
    out_game->render = game_render;
    out_game->on_resize = game_on_resize;

    out_game->game_state = nullptr;

    vec3 v = vec3_create(0.0f, 1.0f, 0.0f);

    MINFO("Game created!");
    return true;
}