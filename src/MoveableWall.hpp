//
// Created by Gary on 1/19/2021.
//

#pragma once


#include "common.hpp"
#include "tiny_ecs.hpp"
#include "PhysicsObject.hpp"
#include "render.hpp"

class MoveableWall {
public:
    static ECS::Entity createMoveableWall(vec2 location, vec2 size, float rotation,
                                          COLLISION_HANDLER overlap = [](ECS::Entity, const ECS::Entity , CollisionResult) {},
                                          COLLISION_HANDLER hit = wall_hit);
    static ECS::Entity createCustomMoveableWall(vec2 location, vec2 scale, std::vector<ColoredVertex> vertexes,
                                                vec2 world_velocity,
                                                float rotation,
                                          COLLISION_HANDLER overlap = [](ECS::Entity, const ECS::Entity , CollisionResult) {},
                                          COLLISION_HANDLER hit = wall_hit);

    static void wall_hit(ECS::Entity self, ECS::Entity e, CollisionResult collision);

};

