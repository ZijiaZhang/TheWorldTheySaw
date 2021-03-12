//
// Created by Gary on 1/19/2021.
//

#pragma once


#include "common.hpp"
#include "tiny_ecs.hpp"
#include "PhysicsObject.hpp"

class Wall {
public:
    static ECS::Entity createWall(vec2 location, vec2 size, float rotation,
                                  COLLISION_HANDLER overlap,
                                  COLLISION_HANDLER hit);

    static void wall_hit(ECS::Entity self, const ECS::Entity e, CollisionResult collision);

};

