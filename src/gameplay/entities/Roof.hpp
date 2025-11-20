#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

struct Roof {
    ECS::Entity parent_building; // Reference to parent building
    float current_opacity = 1.0f;
    float transparency_distance = 50.f; // Distance at which roof starts fading
    float fade_range = 20.f; // Distance over which fade occurs
};

class RoofSystem {
public:
    static ECS::Entity createRoof(vec2 position, vec2 size, float rotation, ECS::Entity parent_building);
    
    static void updateRoofTransparency(vec2 player_pos);
};
