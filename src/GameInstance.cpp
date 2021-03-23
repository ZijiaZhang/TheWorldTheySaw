//
// Created by Gary on 3/19/2021.
//

#include "GameInstance.hpp"

std::string GameInstance::currentLevel = "menu";
WeaponType GameInstance::selectedWeapon = W_BULLET;
AIAlgorithm GameInstance::algorithm = DIRECT;
MagicWeapon GameInstance::selectedMagic = FIREBALL;