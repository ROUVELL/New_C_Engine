#include "input.h"

#include "core/event.h"
#include "memory/memory.h"
#include "core/logger.h"

typedef struct keyboard_state {
    b8 keys[256];
} keyboard_state;

typedef struct mouse_state {
    i16 x;
    i16 y;
    b8 buttons[MOUSE_BUTTON_MAX];
} mouse_state;

typedef struct input_state {
    keyboard_state keyboard_current;
    keyboard_state keyboard_previous;
    mouse_state mouse_current;
    mouse_state mouse_previous;
} input_state;

static input_state state;

b8 input_system_initialize() {
    memory_zero(&state, sizeof(input_state));
    return true;
}

void input_system_shutdown() {

}

void input_update(f64 dt) {
    memory_copy(&state.keyboard_previous, &state.keyboard_current, sizeof(keyboard_state));
    memory_copy(&state.mouse_previous, &state.mouse_current, sizeof(mouse_state));
}


void input_process_key(keys key, b8 pressed) {
    if (state.keyboard_current.keys[key] != pressed) {
        state.keyboard_current.keys[key] = pressed;

        event_context ctx;
        ctx.data.u16[0] = key;
        event_fire(pressed ? SYSTEM_EVENT_CODE_KEY_PRESSED : SYSTEM_EVENT_CODE_KEY_RELEASED, nullptr, ctx);
    }
}

void input_process_button(mouse_buttons button, b8 pressed) {
    if (state.mouse_current.buttons[button] != pressed) {
        state.mouse_current.buttons[button] = pressed;

        event_context ctx;
        ctx.data.u16[0] = button;
        event_fire(pressed ? SYSTEM_EVENT_CODE_BUTTON_PRESSED : SYSTEM_EVENT_CODE_BUTTON_RELEASED, nullptr, ctx);
    }
}

void input_process_mouse_move(i16 x, i16 y) {
    if (state.mouse_current.x != x || state.mouse_current.y != y) {
        state.mouse_current.x = x;
        state.mouse_current.y = y;

        event_context ctx;
        ctx.data.u16[0] = x;
        ctx.data.u16[1] = y;
        event_fire(SYSTEM_EVENT_CODE_MOUSE_MOVE, nullptr, ctx);
    }
}

void input_process_mouse_wheel(i8 z_delta) {
    event_context ctx;
    ctx.data.i8[0] = z_delta;
    event_fire(SYSTEM_EVENT_CODE_MOUSE_WHEEL, nullptr, ctx);
}


b8 input_is_key_down(keys key) {
    return state.keyboard_current.keys[key] == true;
}

b8 input_is_key_up(keys key) {
    return state.keyboard_current.keys[key] == false;
}

b8 input_was_key_down(keys key) {
    return state.keyboard_previous.keys[key] == true;
}

b8 input_was_key_up(keys key) {
    return state.keyboard_previous.keys[key] == false;
}


b8 input_is_button_down(mouse_buttons button) {
    return state.mouse_current.buttons[button] == true;
}

b8 input_is_button_up(mouse_buttons button) {
    return state.mouse_current.buttons[button] == false;
}

b8 input_was_button_down(mouse_buttons button) {
    return state.mouse_previous.buttons[button] == true;
}

b8 input_was_button_up(mouse_buttons button) {
    return state.mouse_previous.buttons[button] == false;
}

void input_get_mouse_position(i32* x, i32* y) {
    *x = state.mouse_current.x;
    *y = state.mouse_current.y;
}

void input_get_previous_mouse_position(i32* x, i32* y) {
    *x = state.mouse_previous.x;
    *y = state.mouse_previous.y;
}

void input_get_mouse_offset(i32* x, i32* y) {
    *x = state.mouse_current.x - state.mouse_previous.x;
    *y = state.mouse_current.y - state.mouse_previous.y;
}
