//
// Created by Gary on 2/12/2021.
//

#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

class Bullet {
public:
    static ECS::Entity createBullet(vec2 position, float angle);
};


