#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "PhysicsObject.hpp"

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>

#include <functional>
#include <string>
#include <unordered_map>

#define LAZER_RELOAD 300.f
#define BULLET_RELOAD 500.f
#define AMMO_RELOAD 650.f
#define ROCKET_RELOAD 1300.f

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

struct BulletSpawnConfig {
    vec2 position = {0.f, 0.f};
    float angle = 0.f;
    vec2 velocity = {0.f, 0.f};
    vec2 scaleMultiplier = {1.f, 1.f};
    float lifetime_ms = -1.f;
    WeaponType type = W_BULLET;
    std::string texture;
    int teamId = 0;
    bool explodeOnHit = false;
};

struct BulletModifier {
    std::function<void(BulletSpawnConfig&)> adjustSpawnConfig = nullptr;
    std::function<void(ECS::Entity, const BulletSpawnConfig&)> afterSpawn = nullptr;
};

struct WeaponFireConfig {
    WeaponType type = W_BULLET;
    float reloadMs = 0.f;
    vec2 muzzleVelocity = {0.f, 0.f};
    vec2 scaleMultiplier = {1.f, 1.f};
    float lifetime_ms = -1.f;
    std::string texture;
    std::string soundEffect;
    bool explodeOnHit = false;
};

class SoldierAISystem
{
public:
    static void step(float elapsed_ms, vec2 window_size_in_game_units);

    static void registerWeaponConfig(const WeaponFireConfig& config);
    static void removeWeaponConfig(WeaponType type);

    static void registerBulletModifier(const std::string& id, const BulletModifier& modifier);
    static void unregisterBulletModifier(const std::string& id);
    static void clearBulletModifiers();

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
    static std::unordered_map<WeaponType, WeaponFireConfig> weaponConfigs;
    static std::unordered_map<std::string, BulletModifier> bulletModifiers;
};


