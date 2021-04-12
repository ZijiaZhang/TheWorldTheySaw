//
// Created by Gary on 3/19/2021.
//

#include "GameInstance.hpp"

std::string GameInstance::currentLevel = "menu";
WeaponType GameInstance::selectedWeapon = W_BULLET;
AIAlgorithm GameInstance::algorithm = DIRECT;
MagicWeapon GameInstance::selectedMagic = FIREBALL;
float GameInstance::light_quality = 32.f;
float GameInstance::global_speed = 1.f;

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


static std::map<std::string, bool> entered_level = {
        {"menu", false},
        {"level_select", false},
        {"loadout", false},
        {"win", false},
        {"lose", false},
        {"intro", false},
        {"level_1", false},
        {"level_2", false},
        {"level_3", false},
        {"level_4", false},
        {"level_5", false},
        {"level_6", false},
        {"level_7", false},
        {"level_8", false},
        {"level_9", false},
        {"level_10", false}
};

bool GameInstance::isPlayableLevel(std::string level)
{
    return playableLevelMap[level];
}

bool GameInstance::isPlayableLevel()
{
    return playableLevelMap[currentLevel];
}

bool GameInstance::fist_enter_level(std::string level){
    return !entered_level[level];
}


void GameInstance::set_enter_level(std::string level) {
    entered_level[level] = true;
}