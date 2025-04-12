#include "platform.h"

#ifdef PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>  // param input extraction

#include <stdlib.h>
#include <timeapi.h>

#include "core/logger.h"
#include "core/event.h"
#include "core/input.h"

#include "threads/mutex.h"
#include "threads/thread.h"
#include "time/clock.h"
#include "memory/memory.h"
#include "strings/string.h"
#include "containers/darray.h"
#include "renderer/renderer_types.h"

typedef struct win32_handle_info {
    HINSTANCE h_instance;
} win32_handle_info;

typedef struct window_platform_state {
    HWND hwnd;
} window_platform_state;

typedef struct win32_platform_state {
    win32_handle_info handle_info;

    CONSOLE_SCREEN_BUFFER_INFO std_output_csbi;
    CONSOLE_SCREEN_BUFFER_INFO err_output_csbi;

    // darray
    window** windows;
} win32_platform_state;

static win32_platform_state* state_ptr;

// Clock
static f64 clock_frequency;
static UINT min_period;
static LARGE_INTEGER start_time;

// cstr
static LPCWSTR cstr_to_wcstr(const char* str);
static const char* wcstr_to_cstr(LPCWSTR wstr);
static void wcstr_free(LPCWSTR wstr);

LRESULT win32_process_message(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param);

void clock_setup() {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    clock_frequency = 1.0 / (f64)frequency.QuadPart;
    QueryPerformanceCounter(&start_time);

    TIMECAPS tc;
    timeGetDevCaps(&tc, sizeof(tc));
    min_period = tc.wPeriodMin;
}

b8 platform_system_initialize(const platform_system_config* config) {
    state_ptr = malloc(sizeof(win32_platform_state));

    state_ptr->handle_info.h_instance = GetModuleHandleW(0);

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &state_ptr->std_output_csbi);
    GetConsoleScreenBufferInfo(GetStdHandle(STD_ERROR_HANDLE), &state_ptr->err_output_csbi);

    state_ptr->windows = darray_create(window*);

    // Only available in the Creators update for Windows 10+.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    // NOTE: Older versions of windows might have to use this:
    // SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);

    // Setup and register window class
    HICON icon = LoadIcon(state_ptr->handle_info.h_instance, IDI_APPLICATION);
    WNDCLASSEXW wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_DBLCLKS;  // Get double-clicks
    wc.lpfnWndProc = win32_process_message;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = state_ptr->handle_info.h_instance;
    wc.hIcon = icon;
    wc.hIconSm = icon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"my_window_class";
    wc.lpszMenuName = 0;

    if (!RegisterClassExW(&wc)) {
        DWORD last_error = GetLastError();
        LPWSTR wmsg_buf = NULL;

        u64 size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                  NULL, last_error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&wmsg_buf, 0, NULL);
        if (size) {
            const char* err_msg = wcstr_to_cstr(wmsg_buf);
            const char* msg = cstr_format("Window registration failed with error: %s", err_msg);
            LocalFree(wmsg_buf);

            const WCHAR* wmsg = cstr_to_wcstr(msg);
            MessageBoxW(NULL, wmsg, L"Error!", MB_ICONEXCLAMATION | MB_OK);
            MFATAL(msg);
            cstr_free(msg);
            wcstr_free(wmsg);
        } else {
            MessageBoxW(NULL, L"Window registration failed", L"Error", MB_ICONEXCLAMATION | MB_OK);
            MFATAL("Window registration failed!");
        }

        return false;
    }

    clock_setup();

    return true;
}

void platform_system_shutdown() {
    u64 len = darray_length(state_ptr->windows);
    for (u64 i = 0; i < len; ++i) {
        platform_window_destroy(state_ptr->windows[i]);
    }
    darray_destroy(state_ptr->windows);
    state_ptr->windows = nullptr;

    free(state_ptr);
    state_ptr = nullptr;
}

b8 platform_pump_messages() {
    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return true;
}

b8 platform_window_create(const window_config* config, struct window* out_window, b8 show_immediately) {
    out_window->name = cstr_duplicate(config->name);

    if (config->title) {
        out_window->title = cstr_duplicate(config->title);
    } else {
        out_window->title = cstr_duplicate("New Window");
    }

    out_window->width = config->width;
    out_window->height = config->height;
    out_window->device_pixel_ratio = 1.0f;

    out_window->platform_state = memory_allocate(sizeof(window_platform_state), MEMORY_TAG_ENGINE);
    out_window->renderer_state = memory_allocate(sizeof(window_renderer_state), MEMORY_TAG_RENDERER);

    // Create window
    u32 x = config->x;
    u32 y = config->y;
    u32 width = config->width;
    u32 height = config->height;

    u32 window_style = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;
    u32 window_ex_style = WS_EX_APPWINDOW;

    window_style |= WS_MAXIMIZEBOX;
    window_style |= WS_MINIMIZEBOX;
    window_style |= WS_THICKFRAME;

    // Obtain the size of the border
    RECT border_rect = {0, 0, 0, 0};
    AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);

    x += border_rect.left;
    y += border_rect.top;
    width += border_rect.right - border_rect.left;
    height += border_rect.bottom - border_rect.top;

    WCHAR wtitle[256];
    int len = MultiByteToWideChar(CP_UTF8, 0, out_window->title, -1, wtitle, 256);
    if (!len) {
    }

    out_window->platform_state->hwnd = CreateWindowExW(
        window_ex_style, L"my_window_class", wtitle,
        window_style, x, y, width, height,
        0, 0, state_ptr->handle_info.h_instance, 0
    );

    if (!out_window->platform_state->hwnd) {
        DWORD last_error = GetLastError();
        LPWSTR wmsg_buf = NULL;

        u64 size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                  NULL, last_error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&wmsg_buf, 0, NULL);
        if (size) {
            const char* err_msg = wcstr_to_cstr(wmsg_buf);
            const char* msg = cstr_format("Window creation failed with error: %s", err_msg);
            LocalFree(wmsg_buf);

            const WCHAR* wmsg = cstr_to_wcstr(msg);
            MessageBoxW(NULL, wmsg, L"Error!", MB_ICONEXCLAMATION | MB_OK);
            MFATAL(msg);
            cstr_free(msg);
            wcstr_free(wmsg);
        } else {
            MessageBoxW(NULL, L"Window creation failed", L"Error", MB_ICONEXCLAMATION | MB_OK);
            MFATAL("Window creation failed!");
        }

        memory_free(out_window->renderer_state, sizeof(window_renderer_state), MEMORY_TAG_RENDERER);
        memory_free(out_window->platform_state, sizeof(window_platform_state), MEMORY_TAG_ENGINE);
        cstr_free(out_window->title);
        cstr_free(out_window->name);
        out_window = nullptr;

        return false;
    }

    darray_push(state_ptr->windows, out_window);

    event_context ctx = {};
    ctx.data.p[0] = out_window;
    event_fire(SYSTEM_EVENT_CODE_WINDOW_CREATED, nullptr, ctx);

    if (show_immediately) {
        return platform_window_show(out_window);
    }

    return true;

}

void platform_window_destroy(struct window* w) {
    if (w) {
        u64 len = darray_length(state_ptr->windows);
        for (u64 i = 0; i < len; ++i) {
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
        DestroyWindow(w->platform_state->hwnd);
        if (w->renderer_state) {
            memory_free(w->renderer_state, sizeof(window_renderer_state), MEMORY_TAG_RENDERER);
        }

        if (w->platform_state) {
            memory_free(w->platform_state, sizeof(window_platform_state), MEMORY_TAG_ENGINE);
        }

        if (w->name) {
            cstr_free(w->name);
        }

        if (w->title) {
            cstr_free(w->title);
        }
    }
}

b8 platform_window_show(struct window* w) {
    if (!w) {
        return false;
    }

    b8 should_activate = 1;  // NOTE: If the window should not accept input, this should be fasle
    i32 show_window_command_flags = should_activate ? SW_SHOW : SW_SHOWNOACTIVATE;
    // If initially minimized, use SW_MINIMIZE : SW_SHOWMINNOACTIVATE;
    // If initially maximized, use SW_SHOWMAXIMIZED : SW_MAXIMIZE;
    ShowWindow(w->platform_state->hwnd, show_window_command_flags);

    return true;
}

b8 platform_window_hide(struct window* w) {
    if (!w) {
        return false;
    }

    i32 show_window_command_flags = SW_HIDE;
    ShowWindow(w->platform_state->hwnd, show_window_command_flags);

    return true;
}

void platform_get_handle_info(u64* out_size, void* memory) {
    *out_size = sizeof(win32_handle_info);
    if (!memory) {
        return;
    }

    memory_copy(memory, &state_ptr->handle_info, *out_size);
}

void platform_console_write(log_level level, const char* msg) {
    b8 is_error = (level == LOG_LEVEL_ERROR || level == LOG_LEVEL_FATAL);
    HANDLE console_handle = GetStdHandle(is_error ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
    
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (state_ptr) {
        csbi = is_error ? state_ptr->err_output_csbi : state_ptr->std_output_csbi;
    } else {
        GetConsoleScreenBufferInfo(console_handle, &csbi);
    }

    // FATAL, ERROR, WARN, INFO, DEBUG, TRACE
    static u8 colors[6] = {64, 4, 6, 2, 1, 8};
    SetConsoleTextAttribute(console_handle, colors[level]);

    LPCWSTR wmsg = cstr_to_wcstr(msg);

    OutputDebugStringW(wmsg);

    i32 length = lstrlen(msg);
    DWORD written = 0;
    WriteConsoleW(console_handle, wmsg, (DWORD)length, &written, 0);
    wcstr_free(wmsg);

    SetConsoleTextAttribute(console_handle, csbi.wAttributes);
}

f64 platform_get_absolute_time() {
    if (!clock_frequency) {
        clock_setup();
    }

    LARGE_INTEGER now_time;
    QueryPerformanceCounter(&now_time);
    return (f64)now_time.QuadPart * clock_frequency;
}

void platform_sleep(u64 ms) {
    clock c;
    clock_start(&c);
    timeBeginPeriod(min_period);
    Sleep(ms - min_period);
    timeEndPeriod(min_period);

    clock_update(&c);
    f64 observed = c.elapsed * 1000.0;
    f64 ms_remaining = (f64)ms - observed;

    clock_start(&c);
    while (c.elapsed * 1000.0 < ms_remaining) {
        _mm_pause();
        clock_update(&c);
    }
}

static window* window_from_handle(HWND hwnd) {
    u64 len = darray_length(state_ptr->windows);
    for (u64 i = 0; i < len; ++i) {
        window* w = state_ptr->windows[i];
        if (w && w->platform_state->hwnd == hwnd) {
            return w;
        }
    }

    return nullptr;
}

LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param) {
    switch (msg) {
        case WM_ERASEBKGND:
            // Notify the OS that erasing will be handled by the application to prevent flicker.
            return 1;

        // case WM_CREATE: {
            
        // }

        case WM_CLOSE: {
            event_context ctx = {};
            ctx.data.p[0] = window_from_handle(hwnd);
            event_fire(SYSTEM_EVENT_CODE_APPLICATION_QUIT, nullptr, ctx);
            return 0;
        }

        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }

        case WM_DPICHANGED:
            i32 x_dpi = GET_X_LPARAM(w_param);
            window* w = window_from_handle(hwnd);

            w->device_pixel_ratio = (f32)x_dpi / USER_DEFAULT_SCREEN_DPI;
            MINFO("Display device pixel ratio: %.2f", (f32)x_dpi / USER_DEFAULT_SCREEN_DPI);
            
            return 0;

        case WM_SIZE: {
            // Get the updated size.
            RECT r;
            GetClientRect(hwnd, &r);
            u32 width = r.right - r.left;
            u32 height = r.bottom - r.top;

            window* w = window_from_handle(hwnd);
            w->width = (u16)width;
            w->height = (u16)height;

            // Fire the event. The application layer should pick this up, but not handle it
            // as it shouldn be visible to other parts of the application.
            event_context ctx;
            ctx.data.u16[0] = (u16)width;
            ctx.data.u16[1] = (u16)height;
            ctx.data.p[1] = w;
            event_fire(SYSTEM_EVENT_CODE_WINDOW_RESIZED, nullptr, ctx);
        } break;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            // Key pressed/released
            b8 pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
            keys key = (u16)w_param;

            // Check for extended scan code.
            b8 is_extended = (HIWORD(l_param) & KF_EXTENDED) == KF_EXTENDED;

            // Keypress only determines if _any_ alt/ctrl/shift key is pressed. Determine which one if so.
            if (w_param == VK_MENU) {
                key = is_extended ? KEY_RALT : KEY_LALT;
            } else if (w_param == VK_SHIFT) {
                // Annoyingly, KF_EXTENDED is not set for shift keys.
                u32 left_shift = MapVirtualKey(VK_LSHIFT, MAPVK_VK_TO_VSC);
                u32 scancode = ((l_param & (0xFF << 16)) >> 16);
                key = scancode == left_shift ? KEY_LSHIFT : KEY_RSHIFT;
            } else if (w_param == VK_CONTROL) {
                key = is_extended ? KEY_RCONTROL : KEY_LCONTROL;
            }

            if (key == VK_OEM_1) {
                key = KEY_SEMICOLON;
            }

            // Pass to the input subsystem for processing.
            input_process_key(key, pressed);

            // Return 0 to prevent default window behaviour for some keypresses, such as alt.
            return 0;
        }

        case WM_MOUSEMOVE: {
            // Mouse move
            i32 mx = GET_X_LPARAM(l_param);
            i32 my = GET_Y_LPARAM(l_param);

            // Pass over to the input subsystem.
            input_process_mouse_move(mx, my);
        } break;

        case WM_MOUSEWHEEL: {
            i32 z_delta = GET_WHEEL_DELTA_WPARAM(w_param);
            // Flatten the input to an OS-independent (-1, 1)
            input_process_mouse_wheel(MSIGN(z_delta));
        } break;

        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP: {
            b8 pressed = msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN;
            mouse_buttons mouse_button = MOUSE_BUTTON_MAX;
            switch (msg) {
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                    mouse_button = MOUSE_BUTTON_LEFT;
                    break;
                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP:
                    mouse_button = MOUSE_BUTTON_MIDDLE;
                    break;
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                    mouse_button = MOUSE_BUTTON_RIGHT;
                    break;
            }

            // Pass over to the input subsystem.
            if (mouse_button != MOUSE_BUTTON_MAX) {
                input_process_button(mouse_button, pressed);
            }
        } break;

        case WM_SHOWWINDOW: {
            if (!GetLayeredWindowAttributes(hwnd, NULL, NULL, NULL)) {
                SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
                DefWindowProc(hwnd, WM_ERASEBKGND, (WPARAM)GetDC(hwnd), l_param);
                SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
                AnimateWindow(hwnd, 200, AW_ACTIVATE | AW_BLEND);
                return 0;
            }
        } break;
    }

    return DefWindowProcW(hwnd, msg, w_param, l_param);
}

// Thread

b8 thread_create(PFN_thread_start start_func, void* args, b8 auto_detach, thread* out_thread) {
    if (!start_func) {
        return false;
    }

    out_thread->internal_data = CreateThread(
        nullptr,
        0,
        (LPTHREAD_START_ROUTINE)start_func,
        args,
        0,
        (DWORD*)&out_thread->thread_id
    );

    if (!out_thread->internal_data) {
        return false;
    }

    MDEBUG("Starting process on thread id: %#x", out_thread->thread_id);

    if (auto_detach) {
        CloseHandle((HANDLE)out_thread->internal_data);
    }

    return true;
}

void thread_destroy(thread* t) {
    if (t && t->internal_data) {
        DWORD exit_code;
        GetExitCodeThread(t->internal_data, &exit_code);
        // if (exit_code == STILL_ACTIVE) {
        //     TerminateThread(t->internal_data, 0);  // 0 - failure
        // }
        CloseHandle((HANDLE)t->internal_data);
        t->internal_data = 0;
        t->thread_id = 0;
    }
}

b8 thread_is_active(thread* t) {
    if (t && t->internal_data) {
        DWORD exit_code = WaitForSingleObject((HANDLE)t->internal_data, 0);
        if (exit_code == WAIT_TIMEOUT) {
            return true;
        }
    }

    return false;
}

void thread_detach(thread* t) {
    if (t && t->internal_data) {
        CloseHandle((HANDLE)t->internal_data);
        t->internal_data = 0;
    }
}

void thread_cancel(thread* t) {
    if (t && t->internal_data) {
        TerminateThread(t->internal_data, 0);  // 0 - failure
        t->internal_data = 0;
    }
}

b8 thread_wait(thread* t) {
    if (t && t->internal_data) {
        DWORD exit_code = WaitForSingleObject((HANDLE)t->internal_data, INFINITE);
        if (exit_code == WAIT_OBJECT_0) {
            return true;
        }
    }

    return false;
}

b8 thread_wait_timeout(thread* t,u64 ms) {
    if (t && t->internal_data) {
        DWORD exit_code = WaitForSingleObject((HANDLE)t->internal_data, ms);
        if (exit_code == WAIT_OBJECT_0) {
            return true;
        }  // else if (exit_code == WAIT_TIMEOUT)
    }

    return false;
}

void thread_sleep(thread* t, u64 ms) {
    platform_sleep(ms);
}

u64 platform_current_thread_id(void) {
    return (u64)GetCurrentThreadId();
}

// Mutex

b8 mutex_create(mutex* out_mutex) {
    if (!out_mutex) {
        return false;
    }

    out_mutex->internal_data = CreateMutex(nullptr, false, nullptr);
    if (!out_mutex->internal_data) {
        MERROR("Failed to create mutex!");
        return false;
    }

    return true;
}

void mutex_destroy(mutex* m) {
    if (m && m->internal_data) {
        CloseHandle((HANDLE)m->internal_data);
        m->internal_data = 0;
    }
}

b8 mutex_lock(mutex* m) {
    if (!m || !m->internal_data) {
        return false;
    }

    DWORD result = WaitForSingleObject((HANDLE)m->internal_data, INFINITE);
    switch (result) {
        // The thread got ownership of the mutex
        case WAIT_OBJECT_0:
            return true;
        
        // The thread got ownership of an abandoned mutex
        case WAIT_ABANDONED:
            MERROR("Mutex lock failed!");
            return false;
    }

    return true;  // NOTE: true ?
}

b8 mutex_unlock(mutex* m) {
    if (!m || !m->internal_data) {
        return false;
    }

    i32 result = ReleaseMutex((HANDLE)m->internal_data);
    return result != 0;  // 0 - failure
}

// static

static LPCWSTR cstr_to_wcstr(const char* str) {
    if (!str) {
        return 0;
    }

    i32 len = MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);
    if (len == 0) {
        return 0;
    }

    LPWSTR wstr = memory_allocate(sizeof(WCHAR) * len, MEMORY_TAG_STRING);
    if (!wstr) {
        return 0;
    }

    if (MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, len) == 0) {
        memory_free(wstr, sizeof(WCHAR) * len, MEMORY_TAG_STRING);
        return 0;
    }

    return wstr;
}

static const char* wcstr_to_cstr(LPCWSTR wstr) {
    if (!wstr) {
        return 0;
    }

    i32 len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (len == 0) {
        return 0;
    }

    char* str = memory_allocate(sizeof(char) * len, MEMORY_TAG_STRING);
    if (!str) {
        return 0;
    }

    if (WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, nullptr, nullptr) == 0) {
        memory_free(str, sizeof(char) * len, MEMORY_TAG_STRING);
        return 0;
    }

    return str;
}

static void wcstr_free(LPCWSTR wstr) {
    if (wstr) {
        i32 len = lstrlenW(wstr);
        memory_free((WCHAR*)wstr, sizeof(WCHAR) * (len + 1), MEMORY_TAG_STRING);
        wstr = nullptr;
    }
}

#endif