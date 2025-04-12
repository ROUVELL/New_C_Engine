#pragma once

#include "core/engine.h"
#include "game_types.h"

extern b8 create_game(game* out_game);

int main(int argc, char* argv[]) {
    game g;

    if (!create_game(&g)) {
        return -1;
    }

    if (!g.initialize || !g.update || !g.render || !g.on_resize) {
        return -2;
    }

    if (!engine_initialize(&g)) {
        return -3;
    }

    if (!g.initialize(&g)) {
        return -4;
    }

    engine_run();

    return 0;
}