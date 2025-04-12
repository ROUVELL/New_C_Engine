#pragma once

#include "defines.h"

// Represents a basic clock, which can be used to track time deltas in the system
typedef struct clock {
    f64 start;
    f64 elapsed;
} clock;

// Starts the provided clock. Resets elapsed time
MAPI void clock_start(clock* c);

// Updates the provided clock. Should be called just before checking elapsed time.
// Has no effect on non-started clocks
MAPI void clock_update(clock* c);

// Stops the provided clock. Does not reset elapsed time
MAPI void clock_stop(clock* c);