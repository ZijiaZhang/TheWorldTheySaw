//
// Created by Gary on 3/19/2021.
//

#include "GameInstance.hpp"

std::string GameInstance::currentLevel = "menu";
WeaponType GameInstance::selectedWeapon = W_BULLET;
AIAlgorithm GameInstance::algorithm = DIRECT;
MagicWeapon GameInstance::selectedMagic = FIREBALL;
float GameInstance::light_quality = 32.f;
int GameInstance::charges_left = 0;

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


static std::map<MagicWeapon, int> charge_of_magic = {
    {FIREBALL, 5},
    {FIELD, 3}
};

bool GameInstance::isPlayableLevel(std::string level)
{
    return playableLevelMap[level];
}

bool GameInstance::isPlayableLevel()
{
    return playableLevelMap[currentLevel];
}

int GameInstance::getDefaultChargeOfMagic(MagicWeapon m) {
    return charge_of_magic[m];
}