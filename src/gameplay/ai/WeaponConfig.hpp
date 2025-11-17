#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "WeaponTypes.hpp"

#include <functional>
#include <string>
#include <unordered_map>

#define LAZER_RELOAD 300.f
#define BULLET_RELOAD 500.f
#define AMMO_RELOAD 650.f
#define ROCKET_RELOAD 1300.f

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

class WeaponConfigRegistry {
public:
    static void initializeDefaults();

    static void registerWeaponConfig(const WeaponFireConfig& config);
    static void removeWeaponConfig(WeaponType type);
    static const WeaponFireConfig* getWeaponConfig(WeaponType type);

    static void registerBulletModifier(const std::string& id, const BulletModifier& modifier);
    static void unregisterBulletModifier(const std::string& id);
    static void clearBulletModifiers();
    static const std::unordered_map<std::string, BulletModifier>& getBulletModifiers();

private:
    static std::unordered_map<WeaponType, WeaponFireConfig>& weaponConfigStorage();
    static std::unordered_map<std::string, BulletModifier>& bulletModifierStorage();
    static bool defaultsInitialized;
};


