#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "PhysicsObject.hpp"

struct DestructibleWall {
    // Configurable parameters
    int debris_count = 5;
    float explosion_force = 200.f;
};

struct Debris {
    float life_time = 1000.f; // ms
    float collision_disable_timer = 1000.f; // ms
};

class DestructibleWallSystem {
public:
    static ECS::Entity createDestructibleWall(vec2 location, vec2 size, float rotation);
    
    static void breakWall(ECS::Entity wall_entity, vec2 impact_point);
    
    static void updateDebris(float elapsed_ms);

    static void onOverlap(ECS::Entity self, const ECS::Entity e, CollisionResult collision);
};
