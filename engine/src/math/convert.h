#pragma once

#include "defines.h"
#include "constants.h"


MINLINE f32 deg_to_rad(f32 angle) {
    return angle * M_DEG2RAD_MULTIPLIER;
}

MINLINE f32 rad_to_deg(f32 angle) {
    return angle * M_RAD2DEG_MULTIPLIER;
}


MINLINE f32 sec_to_ms(f32 angle) {
    return angle * M_SEC_TO_MS_MULTIPLIER;
}

MINLINE f32 ms_to_sec(f32 angle) {
    return angle * M_MS_TO_SEC_MULTIPLIER;
}


MINLINE f32 range_convert_f32(f32 value, f32 old_min, f32 old_max, f32 new_min, f32 new_max) {
    return (((value - old_min) * (new_max - new_min)) / (old_max - old_min)) + new_min;
}