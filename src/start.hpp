#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

// Salmon food
struct Start
{
    // Creates all the associated render resources and default transform
    static ECS::Entity createStart(vec2 position);
};
