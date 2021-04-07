//
// Created by Gary on 2/4/2021.
//

#pragma once
#include "tiny_ecs.hpp"
#include "common.hpp"
#include "ai.hpp"
#include "SoldierAi.hpp"
#include <AiState.hpp>
#define ENEMY_DEFAULT_TEAM_ID 1
#define ENEMY_DEFAULT_HEALTH 2

class Enemy {

public:
    static ECS::Entity createEnemy(vec2 position, COLLISION_HANDLER overlap = [](ECS::Entity, const ECS::Entity, CollisionResult) {},
                                   COLLISION_HANDLER hit = [](ECS::Entity, const ECS::Entity , CollisionResult) {},
                                   int teamID =1, float health = 2);
    AiState enemyState = AiState::IDLE;
    int teamID = 1;

    static std::string ori_texture_path;
    static std::string frozen_texture_path;
    static std::string ori_shader_name;
    static std::string frozen_shader_name;

    static void enemy_bullet_hit_death(ECS::Entity self, const ECS::Entity e, CollisionResult);

    static void set_shader(ECS::Entity self, bool effect = false, std::string texture_path = Enemy::ori_texture_path, std::string shader_name = Enemy::ori_shader_name);
    static void set_frozen_shader(ECS::Entity self);
    static void set_activating_shader(ECS::Entity self);

};



