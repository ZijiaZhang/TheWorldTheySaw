#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

// Salmon food
struct ButtonSetting
{
    // Creates all the associated render resources and default transform
    static ECS::Entity createButtonSetting(vec2 position);
};
