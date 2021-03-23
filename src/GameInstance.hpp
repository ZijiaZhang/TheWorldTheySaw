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
};

