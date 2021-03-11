//
// Created by Gary on 2/4/2021.
//

#pragma once
#include "tiny_ecs.hpp"
#include "common.hpp"
#include "ai.hpp"
#include <AiState.hpp>

class Enemy {

public:
    static ECS::Entity createEnemy(vec2 position, COLLISION_HANDLER overlap = [](ECS::Entity&, const ECS::Entity &, CollisionResult) {},
                                   COLLISION_HANDLER hit = [](ECS::Entity&, const ECS::Entity &, CollisionResult) {}, int teamID =1);
    AiState enemyState = AiState::IDLE;
    int teamID = 1;
};



