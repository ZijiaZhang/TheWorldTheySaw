#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "PhysicsObject.hpp"
#include "WeaponTypes.hpp"
#include "WeaponConfig.hpp"

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>

#include <functional>
#include <string>
#include <unordered_map>

typedef enum{
    DIRECT,
    A_STAR,
} AIAlgorithm;

class SoldierAISystem
{
public:
    static void step(float elapsed_ms, vec2 window_size_in_game_units);

// Decision tree if statements
static void direct_movement(ECS::Entity soldier_entity, float elapsed_ms);
static void a_star_to_closest_enemy(ECS::Entity soldier_entity, float elapsed_ms);

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

    static void underEffectControl(ECS::Entity soldier, float elapsed_ms);
    
    static void handleWeaponFire(ECS::Entity soldier_entity, WeaponType weaponType);
    static bool resolveAimAngle(Motion& soldierMotion, Motion& weaponMotion, float& outAngle);
    static BulletSpawnConfig makeSpawnConfig(const WeaponFireConfig& config, vec2 position, float angle, int teamId);
    static ECS::Entity spawnBullet(const BulletSpawnConfig& config);
    static void applySpawnScale(ECS::Entity bulletEntity, const BulletSpawnConfig& config);
    static void configureExplosionOnHit(ECS::Entity bulletEntity, const BulletSpawnConfig& config);
    static void applyPreSpawnModifiers(BulletSpawnConfig& config);
    static void applyPostSpawnModifiers(ECS::Entity bulletEntity, const BulletSpawnConfig& config);
    static void playWeaponSound(const WeaponFireConfig& config);

    static std::unordered_map<AIAlgorithm, std::function<void(ECS::Entity, float)>> algorithmMap;
};


