//
// Created by Gary on 2/4/2021.
//

#pragma once
#include "tiny_ecs.hpp"
#include "common.hpp"
#include "ai.hpp"
#include <AiState.hpp>
#define ENEMY_DEFAULT_TEAM_ID 1
#define ENEMY_DEFAULT_HEALTH -1

class Enemy {

public:
    static ECS::Entity createEnemy(vec2 position, COLLISION_HANDLER overlap = [](ECS::Entity, const ECS::Entity, CollisionResult) {},
                                   COLLISION_HANDLER hit = [](ECS::Entity, const ECS::Entity , CollisionResult) {},
                                   int teamID =1, float health = -1);
    AiState enemyState = AiState::IDLE;
    int teamID = 1;

    static void enemy_bullet_hit_death(ECS::Entity self, const ECS::Entity e, CollisionResult);
};



