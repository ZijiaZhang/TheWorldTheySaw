//
// Created by Gary on 3/19/2021.
//

#pragma once

#include "common.hpp"
#include "SoldierAi.hpp"

class GameInstance {
    public:
        static std::string currentLevel;
        static WeaponType selectedWeapon;
        static AIAlgorithm algorithm;
        static MagicWeapon selectedMagic;
        static int charges_left;
        static float light_quality;

    static bool isPlayableLevel(std::string level);

    static bool isPlayableLevel();
    static int getDefaultChargeOfMagic(MagicWeapon m);
};

