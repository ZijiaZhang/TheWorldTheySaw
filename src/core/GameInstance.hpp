//
// Created by Gary on 3/19/2021.
//

#pragma once

#include "common.hpp"

// Global, system-wide game state. Keep this small: only values that genuinely
// must be shared across systems (timing, speed, and audio levels) live here.
class GameInstance {
    public:
        static float frame_time;
        static float game_time;
        static float volume;
        static float effect_volume;

        // Game speed overrides (multiplied together by get_current_speed())
        static float global_speed;
        static float popup_speed;
        static float pause_speed;
        static float ability_speed;

    static float get_current_speed();
};
