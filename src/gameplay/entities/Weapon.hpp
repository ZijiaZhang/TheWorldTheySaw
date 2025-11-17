#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

// Salmon enemy 
struct Weapon
{
    // Creates all the associated render resources and default transform
    static ECS::Entity createWeapon(vec2 offset, float offset_angle, ECS::Entity parent, std::string texture_path = "/soldier/weapon_heavy.png");
};