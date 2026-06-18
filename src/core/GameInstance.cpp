//
// Created by Gary on 3/19/2021.
//

#include "GameInstance.hpp"

float GameInstance::global_speed = 1.f;
float GameInstance::popup_speed = 1.f;
float GameInstance::pause_speed = 1.f;
float GameInstance::ability_speed = 1.f;
float GameInstance::frame_time = 0.f;
float GameInstance::game_time = 0.f;
float GameInstance::volume = 50.f;
float GameInstance::effect_volume = 50.f;

float GameInstance::get_current_speed()
{
    return GameInstance::global_speed * GameInstance::popup_speed * GameInstance::pause_speed * GameInstance::ability_speed;
}
