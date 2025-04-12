#pragma once

#include "renderer_types.h"

b8 renderer_system_initialize(const char* app_name, u16 window_width, u16 window_height);
void renderer_system_shutdown();

void renderer_on_window_created(struct window* w);
void renderer_on_window_resized(struct window* w, u16 width, u16 height);
void renderer_on_window_destroyed(struct window* w);