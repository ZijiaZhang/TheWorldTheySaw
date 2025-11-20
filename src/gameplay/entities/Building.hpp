#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

struct Building {
    ECS::Entity roof; // Reference to roof entity
};

class BuildingSystem {
public:
    static ECS::Entity createBuilding(vec2 position, vec2 size, float rotation);
};
