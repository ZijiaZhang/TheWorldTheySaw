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

typedef enum {
    STANDARD,
    SUICIDE,
    ELITE
}EnemyType;

class Enemy {

public:
    static ECS::Entity createEnemy(vec2 position, COLLISION_HANDLER overlap = [](ECS::Entity, const ECS::Entity, CollisionResult) {},
                                   COLLISION_HANDLER hit = [](ECS::Entity, const ECS::Entity , CollisionResult) {},
                                   int teamID =1, EnemyType type = STANDARD, float health = -1.f);
    static ECS::Entity createEnemy(Motion m, Enemy e, Health h, AIPath ai, PhysicsObject po);
    AiState enemyState = AiState::IDLE;
    int teamID = 1;
    EnemyType type = STANDARD;

    static std::string ori_texture_path;
    static std::string frozen_texture_path;
    static std::string ori_shader_name;
    static std::string frozen_shader_name;

    static void enemy_bullet_hit_death(ECS::Entity self, const ECS::Entity e, CollisionResult);

    static void set_shader(ECS::Entity self, bool effect = false, std::string texture_path = Enemy::ori_texture_path, std::string shader_name = Enemy::ori_shader_name);
    static void set_frozen_shader(ECS::Entity self);
    static void set_activating_shader(ECS::Entity self);
    static std::unordered_map<std::string, EnemyType> enemy_type_map;
};



