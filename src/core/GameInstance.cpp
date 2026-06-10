//
// Created by Gary on 3/19/2021.
//

#include "GameInstance.hpp"

std::string GameInstance::currentLevel = "menu";
WeaponType GameInstance::selectedWeapon = W_BULLET;
AIAlgorithm GameInstance::algorithm = DIRECT;
MagicWeapon GameInstance::selectedMagic = FIREBALL;
float GameInstance::light_quality = 16.f;

int GameInstance::charges_left = 0;
float GameInstance::global_speed = 1.f;
float GameInstance::popup_speed = 1.f;
float GameInstance::pause_speed = 1.f;
float GameInstance::ability_speed = 1.f;
float GameInstance::frame_time = 0.f;
float GameInstance::game_time = 0.f;
float GameInstance::volume = 50.f;
float GameInstance::effect_volume = 50.f;
bool GameInstance::weaponAutoAim = false;

static std::map<std::string, bool> playableLevelMap = {
        {"menu", false},
        {"win", false},
        {"lose", false},
        {"level_1", true},
        {"settings", false}
};


static std::map<std::string, bool> entered_level = {
        {"menu", false},
        {"win", false},
        {"lose", false},
        {"level_1", false},
        {"settings", false}
};


static std::map<MagicWeapon, int> charge_of_magic = {
    {FIREBALL, 3},
    {FIELD, 1}
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

bool GameInstance::fist_enter_level(std::string level){
    return !entered_level[level];
}


void GameInstance::set_enter_level(std::string level) {
    entered_level[level] = true;
}

float GameInstance::get_current_speed()
{
    return GameInstance::global_speed * GameInstance::popup_speed * GameInstance::pause_speed * GameInstance::ability_speed;
}
