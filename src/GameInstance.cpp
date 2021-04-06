//
// Created by Gary on 3/19/2021.
//

#include "GameInstance.hpp"

std::string GameInstance::currentLevel = "menu";
WeaponType GameInstance::selectedWeapon = W_BULLET;
AIAlgorithm GameInstance::algorithm = DIRECT;
MagicWeapon GameInstance::selectedMagic = FIREBALL;

static std::map<std::string, bool> playableLevelMap = {
        {"menu", false},
        {"level_select", false},
        {"loadout", false},
        {"win", false},
        {"lose", false},
        {"intro", false},
        {"level_1", true},
        {"level_2", true},
        {"level_3", true},
        {"level_4", true},
        {"level_5", true},
        {"level_6", true},
        {"level_7", true},
        {"level_8", true},
        {"level_9", true},
        {"level_10", true}
};

bool GameInstance::isPlayableLevel(std::string level)
{
    return playableLevelMap[level];
}

bool GameInstance::isPlayableLevel()
{
    return playableLevelMap[currentLevel];
}