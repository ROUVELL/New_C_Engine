#pragma once

#include "defines.h"

struct window;

b8 vulkan_renderer_initialize(const char* app_name, u16 window_width, u16 window_height);

void vulkan_renderer_shutdown(void);

void vulkan_renderer_on_window_created(struct window* w);
void vulkan_renderer_on_window_resized(struct window* w, u16 width, u16 height);
void vulkan_renderer_on_window_destroyed(struct window* w);