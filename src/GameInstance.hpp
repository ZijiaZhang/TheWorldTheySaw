//
// Created by Gary on 3/19/2021.
//

#pragma once

#include "common.hpp"
#include "SoldierAi.hpp"

#define MENU_NAME "menu"
#define TUTORIAL_NAME "tutorial"
#define WEAPON_SELECT_NAME "loadout"


class GameInstance {
    public:
        static std::string currentLevel;
        static WeaponType selectedWeapon;
        static AIAlgorithm algorithm;
        static MagicWeapon selectedMagic;
        static int charges_left;
        static float light_quality;

        // Game speed overrides
        static float global_speed;
        static float popup_speed;
        static float pause_speed;
        static float ability_speed;

    static bool isPlayableLevel(std::string level);

    static bool isPlayableLevel();
    static int getDefaultChargeOfMagic(MagicWeapon m);

    static bool fist_enter_level(std::string level);
    static void set_enter_level(std::string level);
    static float get_current_speed();
};

