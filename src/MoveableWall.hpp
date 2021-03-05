//
// Created by Gary on 1/19/2021.
//

#pragma once


#include "common.hpp"
#include "tiny_ecs.hpp"

class MoveableWall {
public:
    static ECS::Entity createMoveableWall(vec2 location, vec2 size, float rotation,
                                          std::function<void(ECS::Entity&, const  ECS::Entity&)> overlap = [](ECS::Entity&, const ECS::Entity &) {},
                                          std::function<void(ECS::Entity&, const  ECS::Entity&)> hit = [](ECS::Entity&, const ECS::Entity &) {});
};

