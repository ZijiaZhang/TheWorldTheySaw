//
// Created by Gary on 2/4/2021.
//

#pragma once
#include "tiny_ecs.hpp"
#include "common.hpp"
#include "ai.hpp"

class Enemy {

public:
    static ECS::Entity createEnemy(vec2 position);
    Path_with_heuristics path;
    vec2 desired_speed = {0.f, 0.f};

    enum state { IDLE, WALK_BACKWARD, WALK_FORWARD } enemyState;
};


