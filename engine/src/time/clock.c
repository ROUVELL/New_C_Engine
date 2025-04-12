#include "clock.h"

#include "platform/platform.h"


void clock_start(clock* c) {
    c->start = platform_get_absolute_time();
    c->elapsed = 0.0;
}

void clock_update(clock* c) {
    if (c->start != 0.0) {
        c->elapsed = platform_get_absolute_time() - c->start;
    }
}

void clock_stop(clock* c) {
    c->start = 0.0;
}