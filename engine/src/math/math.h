#pragma once

#include "math_types.h"

MAPI f32 msin(f32 x);
MAPI f32 mcos(f32 x);
MAPI f32 mtan(f32 x);
MAPI f32 masin(f32 x);
MAPI f32 macos(f32 x);
MAPI f32 matan(f32 x);
MAPI f32 msqrt(f32 x);
MAPI f32 mabs(f32 x);

MINLINE b8 is_power_of_two(u64 value) {
    return (value != 0) && (value & (value - 1)) == 0;
}
