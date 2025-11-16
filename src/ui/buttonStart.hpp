#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

// Salmon food
struct ButtonStart
{
    // Creates all the associated render resources and default transform
    static ECS::Entity createButtonStart(vec2 position);
};
