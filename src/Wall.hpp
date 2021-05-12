//
// Created by Gary on 1/19/2021.
//

#pragma once


#include "common.hpp"
#include "tiny_ecs.hpp"
#include "PhysicsObject.hpp"

class Wall {
public:
    static ECS::Entity createWall(vec2 location, vec2 size, float rotation, bool visible = true,
                                  COLLISION_HANDLER overlap = Wall:: wall_overlap,
                                  COLLISION_HANDLER hit = Wall::wall_hit);
    static ECS::Entity createWall(Motion m, Wall w, PhysicsObject po);

    static void wall_hit(ECS::Entity self, const ECS::Entity e, CollisionResult collision);

    static void wall_overlap(ECS::Entity self, const ECS::Entity e, CollisionResult collision);

};

