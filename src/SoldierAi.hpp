#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "PhysicsObject.hpp"

typedef enum{
    DIRECT,
    A_STAR,
} AIAlgorithm;

typedef enum{
    W_BULLET,
    W_ROCKET,
    W_LASER,
    W_AMMO
} WeaponType;

class SoldierAISystem
{
public:
    static void step(float elapsed_ms, vec2 window_size_in_game_units);

// Decision tree if statements
static void direct_movement(ECS::Entity soldier_entity, float elapsed_ms);
static void a_star_to_closest_enemy(ECS::Entity soldier_entity, float elapsed_ms);
    static void shoot_bullet(ECS::Entity soldier_entity, float elapsed_ms);
    static void shoot_rocket(ECS::Entity soldier_entity, float elapsed_ms);
    static void shoot_laser(ECS::Entity soldier_entity, float elapsed_ms);
    static void shoot_ammo(ECS::Entity soldier_entity, float elapsed_ms);

private:
    static float weaponTicker;

    static float pathTicker;

    static float updateRate;

    static bool isEnemyExists();

    static ECS::Entity getCloestEnemy(Motion& soldierMotion);

    static bool isEnemyExistsInRange(Motion& soldierMotion, Motion& enemyMotion, float range);

    static void idle(Motion& soldierMotion);

    static void walkBackward(Motion& soldierMotion, Motion& enemyMotion);

    static void walkForward(Motion& soldierMotion, Motion& enemyMotion);


    static std::unordered_map<AIAlgorithm, std::function<void(ECS::Entity, float)>> algorithmMap;
    static std::unordered_map<WeaponType, std::function<void(ECS::Entity, float)>> weaponMap;
};


