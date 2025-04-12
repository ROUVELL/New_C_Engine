#include "platform.h"

#ifdef PLATFORM_LINUX

// #include <X11/extensions/Xrender.h>

#ifndef XRANDR_ROTATION_LEFT
#define XRANDR_ROTATION_LEFT (1 << 1)
#endif
#ifndef XRANDR_ROTATION_RIGHT
#define XRANDR_ROTATION_RIGHT (1 << 9)
#endif


#include <pthread.h>

#include <X11/XKBlib.h>   // sudo apt-get install libx11-dev
#include <X11/Xlib-xcb.h> // sudo apt-get install libxkbcommon-x11-dev libx11-xcb-dev
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <bits/time.h>

// #include <X11/extensions/Xrandr.h>
#include <X11/keysym.h>
#include <sys/time.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

#include "containers/darray.h"
#include "core/asserts.h"
#include "core/logger.h"
#include "core/input.h"
#include "core/event.h"
#include "memory/memory.h"
#include "strings/string.h"
#include "threads/thread.h"
#include "threads/mutex.h"
#include "renderer/renderer_types.h"

#if _POSIX_C_SOURCE >= 199309L
#include <time.h> // nanosleep
#endif

#include <errno.h> // For error reporting
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <unistd.h>


typedef struct linux_handle_info {
    xcb_connection_t* connection;
    xcb_screen_t* screen;
} linux_handle_info;

typedef struct window_platform_state {
    xcb_window_t window;
    f32 device_pixel_ratio;
} window_platform_state;

typedef struct platform_state {
    Display* display;
    linux_handle_info handle;
    xcb_screen_t* screen;
    xcb_atom_t wm_protocols;
    xcb_atom_t wm_delete_win;
    i32 screen_count;
    // darray
    window** windows;
} platform_state;

static platform_state* state_ptr;

static keys translate_keycode(u32 x_keycode);
static window* window_from_handle(xcb_window_t window);


b8 platform_system_initialize(const platform_system_config *config) {
    state_ptr = malloc(sizeof(platform_state));

    state_ptr->display = XOpenDisplay(NULL);

    state_ptr->handle.connection = XGetXCBConnection(state_ptr->display);

    if (xcb_connection_has_error(state_ptr->handle.connection)) {
        MFATAL("Failed to connect to X server via XCB");
        return false;
    }

    const struct xcb_setup_t *setup = xcb_get_setup(state_ptr->handle.connection);

    xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup);
    for (i32 s = 0; s < state_ptr->screen_count; s++) {
        // f32 w_inches = it.data->width_in_millimeters * 0.0394;
        // f32 h_inches = it.data->height_in_millimeters * 0.0394;
        // f32 dpi = (f32)it.data->width_in_pixels / w_inches;

        // MINFO("Monitor '%s' has a DPI of %.2f for a device pixelratio of %0.2f", it.index, dpi, dpi / 96.0f);
        // state_ptr->device_pixel_ratio = dpi / 96.0f;

        xcb_screen_next(&it);
    }

    state_ptr->screen = it.data;
    state_ptr->handle.screen = state_ptr->screen;

    state_ptr->windows = darray_create(window*);

    return true;
}

void platform_system_shutdown() {
    if (state_ptr) {
        if (state_ptr->windows) {
            u64 len = darray_length(state_ptr->windows);
            for (u64 i = 0; i < len; ++i) {
                platform_window_destroy(state_ptr->windows[i]);
            }
            darray_destroy(state_ptr->windows);
            state_ptr->windows = nullptr;
        }

        if (state_ptr->handle.connection) {
            // xcb_disconnect(state_ptr->handle.connection);
            free(state_ptr->handle.connection);
            state_ptr->handle.connection = nullptr;
        }
    }
}

b8 platform_pump_messages() {
    if (!state_ptr) {
        return true;
    }

    xcb_generic_event_t* event;
    xcb_client_message_event_t* cm;

    b8 quit_fragged = false;

    while ((event = xcb_poll_for_event(state_ptr->handle.connection))) {
        switch (event->response_type & ~0x80) {
            case XCB_KEY_PRESS:
            case XCB_KEY_RELEASE: {
                xcb_key_press_event_t* key_event = (xcb_key_press_event_t*)event;
                b8 pressed = event->response_type == XCB_KEY_PRESS;
                xcb_keycode_t code = key_event->detail;
                KeySym key_sym = XkbKeycodeToKeysym(state_ptr->display, (KeyCode)code, 0, 0 /*code & ShiftMask ? 1 : 0*/);

                keys key = translate_keycode(key_sym);

                input_process_key(key, pressed);
            } break;

            case XCB_BUTTON_PRESS:
            case XCB_BUTTON_RELEASE: {
                xcb_button_press_event_t* button_event = (xcb_button_press_event_t*)event;
                b8 pressed = event->response_type == XCB_BUTTON_PRESS;
                mouse_buttons button = MOUSE_BUTTON_MAX;
                switch (button_event->detail) {
                    case XCB_BUTTON_INDEX_1: button = MOUSE_BUTTON_LEFT; break;
                    case XCB_BUTTON_INDEX_2: button = MOUSE_BUTTON_MIDDLE; break;
                    case XCB_BUTTON_INDEX_3: button = MOUSE_BUTTON_RIGHT; break;
                }

                if (button != MOUSE_BUTTON_MAX) {
                    input_process_button(button, pressed);
                }
            } break;

            case XCB_MOTION_NOTIFY: {
                xcb_motion_notify_event_t* motion_event = (xcb_motion_notify_event_t*)event;
                input_process_mouse_move(motion_event->event_x, motion_event->event_y);
            } break;

            case XCB_CONFIGURE_NOTIFY: {
                xcb_configure_notify_event_t* configure_event = (xcb_configure_notify_event_t*)event;

                u16 width = configure_event->width;
                u16 height = configure_event->height;

                window* w = window_from_handle(configure_event->window);
                if (!w) {
                    MERROR("Failed to find window from handle %u", configure_event->window);
                    return false;
                }                

                if (width != w->width || height != w->height) {
                    w->width = width;
                    w->height = height;

                    // w->resizing = true;
                    // w->frames_since_resize = 0;

                    event_context ctx = {};
                    ctx.data.u16[0] = width;
                    ctx.data.u16[1] = height;
                    ctx.data.p[1] = w;
                    event_fire(SYSTEM_EVENT_CODE_WINDOW_RESIZED, nullptr, ctx);
                }
            } break;

            case XCB_CLIENT_MESSAGE: {
                cm = (xcb_client_message_event_t*)event;

                if (cm->data.data32[0] == state_ptr->wm_delete_win) {
                    quit_fragged = true;
                }
            } break;

            default:
                break;
        }

        free(event);
    }

    return !quit_fragged;
}

b8 platform_window_create(const window_config *config, struct window *out_window, b8 show_immediately) {
    if (!out_window) {
        return false;
    }

    i32 x = config->x;
    i32 y = config->y;
    i32 width = config->width;
    i32 height = config->height;

    out_window->width = width;
    out_window->height = height;

    out_window->platform_state = memory_allocate(sizeof(window_platform_state), MEMORY_TAG_PLATFORM);
    out_window->renderer_state = memory_allocate(sizeof(window_renderer_state), MEMORY_TAG_RENDERER);

    out_window->platform_state->window = xcb_generate_id(state_ptr->handle.connection);

    u32 event_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

    u32 event_values = XCB_EVENT_MASK_BUTTON_PRESS |
                       XCB_EVENT_MASK_BUTTON_RELEASE |
                       XCB_EVENT_MASK_KEY_PRESS |
                       XCB_EVENT_MASK_KEY_RELEASE |
                       XCB_EVENT_MASK_EXPOSURE |
                       XCB_EVENT_MASK_POINTER_MOTION |
                       XCB_EVENT_MASK_STRUCTURE_NOTIFY;
                       // XCB_EVENT_MASK_ENTER_WINDOW |
                       // XCB_EVENT_MASK_LEAVE_WINDOW;
    
    u32 value_list[] = { state_ptr->screen->black_pixel, event_values };
    
    xcb_create_window(
        state_ptr->handle.connection,
        XCB_COPY_FROM_PARENT,
        out_window->platform_state->window,
        state_ptr->screen->root,
        x, y, width, height,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        state_ptr->screen->root_visual,
        event_mask,
        value_list
    );

    if (config->title) {
        out_window->title = cstr_duplicate(config->title);
    } else {
        out_window->title = cstr_duplicate("New Window");
    }

    xcb_intern_atom_cookie_t utf8_string_cookie = xcb_intern_atom(state_ptr->handle.connection, 0, 11, "UTF8_STRING");
    xcb_intern_atom_reply_t* utf8_string_reply = xcb_intern_atom_reply(state_ptr->handle.connection, utf8_string_cookie, NULL);

    xcb_intern_atom_cookie_t net_wm_name_cookie = xcb_intern_atom(state_ptr->handle.connection, 0, 12, "_NET_WM_NAME");
    xcb_intern_atom_reply_t* net_wm_name_reply = xcb_intern_atom_reply(state_ptr->handle.connection, net_wm_name_cookie, NULL);

    xcb_change_property(
        state_ptr->handle.connection,
        XCB_PROP_MODE_REPLACE,
        out_window->platform_state->window,
        XCB_ATOM_WM_NAME,
        utf8_string_reply->atom,
        8,
        cstr_len(out_window->title),
        out_window->title
    );

    xcb_change_property(
        state_ptr->handle.connection,
        XCB_PROP_MODE_REPLACE,
        out_window->platform_state->window,
        net_wm_name_reply->atom,
        utf8_string_reply->atom,
        8,
        cstr_len(out_window->title),
        out_window->title
    );

    free(utf8_string_reply);
    free(net_wm_name_reply);

    xcb_intern_atom_cookie_t wm_delete_cookie = xcb_intern_atom(state_ptr->handle.connection, 0, 16, "WM_DELETE_WINDOW");
    xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom(state_ptr->handle.connection, 0, 12, "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* wm_delete_reply = xcb_intern_atom_reply(state_ptr->handle.connection, wm_delete_cookie, NULL);
    xcb_intern_atom_reply_t* wm_protocols_reply = xcb_intern_atom_reply(state_ptr->handle.connection, wm_protocols_cookie, NULL);
    state_ptr->wm_delete_win = wm_delete_reply->atom;
    state_ptr->wm_protocols = wm_protocols_reply->atom;

    xcb_change_property(
        state_ptr->handle.connection,
        XCB_PROP_MODE_REPLACE,
        out_window->platform_state->window,
        wm_protocols_reply->atom,
        4,
        32,
        1,
        &wm_delete_reply->atom
    );

    free(wm_delete_reply);
    free(wm_protocols_reply);

    xcb_map_window(state_ptr->handle.connection, out_window->platform_state->window);

    i32 stream_result = xcb_flush(state_ptr->handle.connection);
    if (stream_result <= 0) {
        MFATAL("An error ocurred while flushing the XCB stream: %s", stream_result);
        return false;
    }

    darray_push(state_ptr->windows, out_window);

    event_context ctx = {};
    ctx.data.p[0] = out_window;
    event_fire(SYSTEM_EVENT_CODE_WINDOW_CREATED, nullptr, ctx);

    return true;
}

void platform_window_destroy(struct window *w) {
    if (w) {
        u32 len = darray_length(state_ptr->windows);
        for (u32 i = 0; i < len; ++i) {
            if (state_ptr->windows[i] == w) {
                MTRACE("Destroying window...");

                event_context ctx = {};
                ctx.data.p[0] = w;
                event_fire(SYSTEM_EVENT_CODE_WINDOW_DESTROYED, nullptr, ctx);

                state_ptr->windows[i] = nullptr;
                goto destroy;
            }
        }

        MWARN("Destroying a window that was somehow not registered with the platform layer!");

    destroy:
        xcb_destroy_window(state_ptr->handle.connection, w->platform_state->window);
        // xcb_flush(state_ptr->handle.connection);

        if (w->renderer_state) {
            memory_free(w->renderer_state, sizeof(window_renderer_state), MEMORY_TAG_RENDERER);
        }

        if (w->platform_state) {
            memory_free(w->platform_state, sizeof(window_platform_state), MEMORY_TAG_PLATFORM);
        }

        if (w->name) {
            cstr_free(w->name);
        }

        if (w->title) {
            cstr_free(w->title);
        }
    }
}

b8 platform_window_show(struct window *w) {
    if (!w) {
        return false;
    }

    xcb_map_window(state_ptr->handle.connection, w->platform_state->window);
    i32 stream_result = xcb_flush(state_ptr->handle.connection);
    if (stream_result <= 0) {
        MFATAL("An error ocurred while flushing the XCB stream: %s", stream_result);
        return false;
    }

    return true;
}

b8 platform_window_hide(struct window *w) {
    if (!w) {
        return false;
    }

    xcb_unmap_window(state_ptr->handle.connection, w->platform_state->window);
    i32 stream_result = xcb_flush(state_ptr->handle.connection);
    if (stream_result <= 0) {
        MFATAL("An error ocurred while flushing the XCB stream: %s", stream_result);
        return false;
    }

    return true;
}

void platform_get_handle_info(u64 *out_size, void *memory) {
    *out_size = sizeof(linux_handle_info);
    if (!memory) {
        return;
    }

    memory_copy(memory, &state_ptr->handle, *out_size);
}

void platform_console_write(log_level level, const char *msg) {
    b8 is_error = level == LOG_LEVEL_ERROR || level == LOG_LEVEL_FATAL;
    FILE* console_handle = is_error ? stderr : stdout;
    // FATAL, ERROR, WARN, INFO, DEBUG, TRACE
    const char* color_strings[6] = { "0;41", "1;31", "1;33", "1;32", "1;34", "1;30" };
    fprintf(console_handle, "\033[%sm%s\033[0m", color_strings[level], msg);
}

f64 platform_get_absolute_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (f64)ts.tv_sec + (f64)ts.tv_nsec * 0.000000001;
}

void platform_sleep(u64 ms) {
#if _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
#else
    if (ms >= 1000) {
        sleep(ms / 1000);
    }
    usleep((ms % 1000) * 1000);
#endif
}

// Thread


b8 thread_create(PFN_thread_start start_func, void* args, b8 auto_detach, thread* out_thread) {
    // if (!out_thread) {
        return false;
    // }

    // pthread_attr_t attr;
    // pthread_attr_init(&attr);
    // pthread_attr_setdetachstate(&attr, auto_detach ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE);

    // pthread_t thread_id;
    // int result = pthread_create(&thread_id, &attr, (void*(*)(void*))start_func, args);
    // if (result != 0) {
    //     MFATAL("Failed to create thread: %s", strerror(result));
    //     return false;
    // }

    // out_thread->thread_id = thread_id;

    // pthread_attr_destroy(&attr);

    // return true;
}

void thread_destroy(thread* t) {
    // if (t) {
    //     pthread_t thread_id = t->thread_id;
    //     if (thread_id) {
    //         pthread_cancel(thread_id);
    //         pthread_join(thread_id, NULL);
    //     }
    // }
}

b8 thread_is_active(thread* t) {
    // if (!t) {
    //     return false;
    // }

    // pthread_t thread_id = t->thread_id;
    // if (thread_id) {
    //     int result = pthread_kill(thread_id, 0);
    //     if (result == 0) {
    //         return true;
    //     } else if (result == ESRCH) {
    //         return false;
    //     } else {
    //         MFATAL("Failed to check thread status: %s", strerror(result));
    //         return false;
    //     }
    // }

    return false;
}

void thread_detach(thread* t) {
    // if (!t) {
    //     return;
    // }

    // pthread_t thread_id = t->thread_id;
    // if (thread_id) {
    //     pthread_detach(thread_id);
    //     t->thread_id = 0;
    // }
}

void thread_cancel(thread* t) {
    // if (!t) {
    //     return;
    // }

    // pthread_t thread_id = t->thread_id;
    // if (thread_id) {
    //     pthread_cancel(thread_id);
    //     t->thread_id = 0;
    // }
}

b8 thread_wait(thread* t) {
    if (!t) {
        return false;
    }

    // pthread_t thread_id = t->thread_id;
    // if (thread_id) {
    //     int result = pthread_join(thread_id, NULL);
    //     if (result != 0) {
    //         MFATAL("Failed to join thread: %s", strerror(result));
    //         return false;
    //     }
    //     t->thread_id = 0;
    // }

    return true;
}

b8 thread_wait_timeout(thread* t,u64 ms) {
    if (!t) {
        return false;
    }

    // pthread_t thread_id = t->thread_id;
    // if (thread_id) {
    //     struct timespec ts;
    //     clock_gettime(CLOCK_REALTIME, &ts);
    //     ts.tv_sec += ms / 1000;
    //     ts.tv_nsec += (ms % 1000) * 1000000;

    //     int result = pthread_timedjoin_np(thread_id, NULL, &ts);

    //     if (result != 0) {
    //         if (result == ETIMEDOUT) {
    //             return false;
    //         } else {
    //             MFATAL("Failed to join thread: %s", strerror(result));
    //             return false;
    //         }
    //     }
    //     t->thread_id = 0;
    // }

    return true;
}

void thread_sleep(thread* t, u64 ms) {
    // if (!t) {
    //     return;
    // }

    // pthread_t thread_id = t->thread_id;
    // if (thread_id) {
    //     struct timespec ts;
    //     clock_gettime(CLOCK_REALTIME, &ts);
    //     ts.tv_sec += ms / 1000;
    //     ts.tv_nsec += (ms % 1000) * 1000000;

    //     int result = pthread_timedjoin_np(thread_id, NULL, &ts);

    //     if (result != 0) {
    //         MFATAL("Failed to join thread: %s", strerror(result));
    //         return;
    //     }
    // }
}

u64 platform_current_thread_id(void) {
    return (u64)pthread_self();
}

// Mutex

b8 mutex_create(mutex* out_mutex) {
    if (!out_mutex) {
        return false;
    }

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_t* mutex = malloc(sizeof(pthread_mutex_t));
    if (!mutex) {
        MFATAL("Failed to allocate memory for mutex");
        return false;
    }

    int result = pthread_mutex_init(mutex, &attr);
    if (result != 0) {
        MFATAL("Failed to initialize mutex: %s", strerror(result));
        free(mutex);
        return false;
    }

    out_mutex->internal_data = mutex;

    pthread_mutexattr_destroy(&attr);

    return true;
}

void mutex_destroy(mutex* m) {
    if (!m) {
        return;
    }

    pthread_mutex_t* mutex = (pthread_mutex_t*)m->internal_data;
    if (mutex) {
        int result = pthread_mutex_destroy(mutex);
        if (result != 0) {
            MFATAL("Failed to destroy mutex: %s", strerror(result));
        }
        free(mutex);
        m->internal_data = nullptr;
    }
}

b8 mutex_lock(mutex* m) {
    if (!m) {
        return false;
    }

    pthread_mutex_t* mutex = (pthread_mutex_t*)m->internal_data;
    if (!mutex) {
        return false;
    }

    int result = pthread_mutex_lock(mutex);
    if (result != 0) {
        MFATAL("Failed to lock mutex: %s", strerror(result));
        return false;
    }

    return true;
}

b8 mutex_unlock(mutex* m) {
    if (!m) {
        return false;
    }

    pthread_mutex_t* mutex = (pthread_mutex_t*)m->internal_data;
    if (!mutex) {
        return false;
    }

    int result = pthread_mutex_unlock(mutex);
    if (result != 0) {
        MFATAL("Failed to unlock mutex: %s", strerror(result));
        return false;
    }

    return true;
}

static keys translate_keycode(u32 x_keycode) {
    switch (x_keycode) {
    case XK_BackSpace:
        return KEY_BACKSPACE;
    case XK_Return:
        return KEY_ENTER;
    case XK_Tab:
        return KEY_TAB;
        // case XK_Shift: return KEY_SHIFT;
        // case XK_Control: return KEY_CONTROL;

    case XK_Pause:
        return KEY_PAUSE;
    case XK_Caps_Lock:
        return KEY_CAPITAL;

    case XK_Escape:
        return KEY_ESCAPE;

        // Not supported
        // case : return KEY_CONVERT;
        // case : return KEY_NONCONVERT;
        // case : return KEY_ACCEPT;

    case XK_Mode_switch:
        return KEY_MODECHANGE;

    case XK_space:
        return KEY_SPACE;
    case XK_Prior:
        return KEY_PAGEUP;
    case XK_Next:
        return KEY_PAGEDOWN;
    case XK_End:
        return KEY_END;
    case XK_Home:
        return KEY_HOME;
    case XK_Left:
        return KEY_LEFT;
    case XK_Up:
        return KEY_UP;
    case XK_Right:
        return KEY_RIGHT;
    case XK_Down:
        return KEY_DOWN;
    case XK_Select:
        return KEY_SELECT;
    case XK_Print:
        return KEY_PRINT;
    case XK_Execute:
        return KEY_EXECUTE_;
    // case XK_snapshot: return KEY_SNAPSHOT; // not supported
    case XK_Insert:
        return KEY_INSERT;
    case XK_Delete:
        return KEY_DELETE;
    case XK_Help:
        return KEY_HELP;

    case XK_Meta_L:
    case XK_Super_L:
        // Treat the "meta" key (if mapped) as super
        return KEY_LSUPER;
    case XK_Meta_R:
    case XK_Super_R:
        // Treat the "meta" key (if mapped) as super
        return KEY_RSUPER;
        // case XK_apps: return KEY_APPS; // not supported

        // case XK_sleep: return KEY_SLEEP; //not supported

    case XK_KP_0:
        return KEY_NUMPAD0;
    case XK_KP_1:
        return KEY_NUMPAD1;
    case XK_KP_2:
        return KEY_NUMPAD2;
    case XK_KP_3:
        return KEY_NUMPAD3;
    case XK_KP_4:
        return KEY_NUMPAD4;
    case XK_KP_5:
        return KEY_NUMPAD5;
    case XK_KP_6:
        return KEY_NUMPAD6;
    case XK_KP_7:
        return KEY_NUMPAD7;
    case XK_KP_8:
        return KEY_NUMPAD8;
    case XK_KP_9:
        return KEY_NUMPAD9;
    case XK_multiply:
        return KEY_MULTIPLY;
    case XK_KP_Add:
        return KEY_ADD;
    case XK_KP_Separator:
        return KEY_SEPARATOR;
    case XK_KP_Subtract:
        return KEY_SUBTRACT;
    case XK_KP_Decimal:
        return KEY_DECIMAL;
    case XK_KP_Divide:
        return KEY_DIVIDE;
    case XK_F1:
        return KEY_F1;
    case XK_F2:
        return KEY_F2;
    case XK_F3:
        return KEY_F3;
    case XK_F4:
        return KEY_F4;
    case XK_F5:
        return KEY_F5;
    case XK_F6:
        return KEY_F6;
    case XK_F7:
        return KEY_F7;
    case XK_F8:
        return KEY_F8;
    case XK_F9:
        return KEY_F9;
    case XK_F10:
        return KEY_F10;
    case XK_F11:
        return KEY_F11;
    case XK_F12:
        return KEY_F12;
    case XK_F13:
        return KEY_F13;
    case XK_F14:
        return KEY_F14;
    case XK_F15:
        return KEY_F15;
    case XK_F16:
        return KEY_F16;
    case XK_F17:
        return KEY_F17;
    case XK_F18:
        return KEY_F18;
    case XK_F19:
        return KEY_F19;
    case XK_F20:
        return KEY_F20;
    case XK_F21:
        return KEY_F21;
    case XK_F22:
        return KEY_F22;
    case XK_F23:
        return KEY_F23;
    case XK_F24:
        return KEY_F24;

    case XK_Num_Lock:
        return KEY_NUMLOCK;
    case XK_Scroll_Lock:
        return KEY_SCROLL;

    case XK_KP_Equal:
        return KEY_NUMPAD_EQUAL;

    case XK_Shift_L:
        return KEY_LSHIFT;
    case XK_Shift_R:
        return KEY_RSHIFT;
    case XK_Control_L:
        return KEY_LCONTROL;
    case XK_Control_R:
        return KEY_RCONTROL;
    case XK_Alt_L:
        return KEY_LALT;
    case XK_Alt_R:
        return KEY_RALT;

    case XK_semicolon:
        return KEY_SEMICOLON;
    case XK_plus:
        return KEY_EQUAL;
    case XK_comma:
        return KEY_COMMA;
    case XK_minus:
        return KEY_MINUS;
    case XK_period:
        return KEY_PERIOD;
    case XK_slash:
        return KEY_SLASH;
    case XK_grave:
        return KEY_GRAVE;

    case XK_0:
        return KEY_0;
    case XK_1:
        return KEY_1;
    case XK_2:
        return KEY_2;
    case XK_3:
        return KEY_3;
    case XK_4:
        return KEY_4;
    case XK_5:
        return KEY_5;
    case XK_6:
        return KEY_6;
    case XK_7:
        return KEY_7;
    case XK_8:
        return KEY_8;
    case XK_9:
        return KEY_9;

    case XK_a:
    case XK_A:
        return KEY_A;
    case XK_b:
    case XK_B:
        return KEY_B;
    case XK_c:
    case XK_C:
        return KEY_C;
    case XK_d:
    case XK_D:
        return KEY_D;
    case XK_e:
    case XK_E:
        return KEY_E;
    case XK_f:
    case XK_F:
        return KEY_F;
    case XK_g:
    case XK_G:
        return KEY_G;
    case XK_h:
    case XK_H:
        return KEY_H;
    case XK_i:
    case XK_I:
        return KEY_I;
    case XK_j:
    case XK_J:
        return KEY_J;
    case XK_k:
    case XK_K:
        return KEY_K;
    case XK_l:
    case XK_L:
        return KEY_L;
    case XK_m:
    case XK_M:
        return KEY_M;
    case XK_n:
    case XK_N:
        return KEY_N;
    case XK_o:
    case XK_O:
        return KEY_O;
    case XK_p:
    case XK_P:
        return KEY_P;
    case XK_q:
    case XK_Q:
        return KEY_Q;
    case XK_r:
    case XK_R:
        return KEY_R;
    case XK_s:
    case XK_S:
        return KEY_S;
    case XK_t:
    case XK_T:
        return KEY_T;
    case XK_u:
    case XK_U:
        return KEY_U;
    case XK_v:
    case XK_V:
        return KEY_V;
    case XK_w:
    case XK_W:
        return KEY_W;
    case XK_x:
    case XK_X:
        return KEY_X;
    case XK_y:
    case XK_Y:
        return KEY_Y;
    case XK_z:
    case XK_Z:
        return KEY_Z;

    default:
        return 0;
    }
}

static window* window_from_handle(xcb_window_t window) {
    u64 len = darray_length(state_ptr->windows);
    for (u64 i = 0; i < len; ++i) {
        if (state_ptr->windows[i]->platform_state->window == window) {
            return state_ptr->windows[i];
        }
    }

    return nullptr;
}

#endif