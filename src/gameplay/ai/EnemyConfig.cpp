#include "EnemyConfig.hpp"

#include <algorithm>

namespace {
constexpr float PLAYER_MAX_SPEED = 100.f;
constexpr float ENEMY_SPEED_MARGIN = 5.f;
constexpr float ENEMY_MAX_SPEED = PLAYER_MAX_SPEED - ENEMY_SPEED_MARGIN;
constexpr float STANDARD_TARGET_SPEED = 80.f;
constexpr float SUICIDE_TARGET_SPEED = 70.f;
constexpr float ELITE_TARGET_SPEED = 90.f;
constexpr float BACKPEDAL_TARGET_SPEED = 85.f;
constexpr float STANDARD_WANDER_SPEED = 60.f;
constexpr float SUICIDE_WANDER_SPEED = 55.f;
constexpr float ELITE_WANDER_SPEED = 70.f;

float clampEnemySpeed(float desiredSpeed) {
    return std::min(desiredSpeed, ENEMY_MAX_SPEED);
}

EnemyMovementConfig makeMovementConfig(float actDistance, float pathAccuracy,
                                       float targetSpeed, float wanderSpeed) {
    EnemyMovementConfig config;
    config.act_distance = actDistance;
    config.path_accuracy = pathAccuracy;
    config.speed = clampEnemySpeed(targetSpeed);
    config.wander_speed = clampEnemySpeed(wanderSpeed);
    config.backpedal_speed = clampEnemySpeed(BACKPEDAL_TARGET_SPEED);
    return config;
}
}

bool EnemyConfigRegistry::defaultsInitialized = false;

std::unordered_map<EnemyType, EnemyMovementConfig>& EnemyConfigRegistry::movementStorage() {
    static std::unordered_map<EnemyType, EnemyMovementConfig> storage;
    return storage;
}

void EnemyConfigRegistry::initializeDefaults() {
    if (defaultsInitialized) {
        return;
    }

    registerMovementConfig(STANDARD, makeMovementConfig(500.f, 100.f, STANDARD_TARGET_SPEED, STANDARD_WANDER_SPEED));
    registerMovementConfig(SUICIDE, makeMovementConfig(1000.f, 50.f, SUICIDE_TARGET_SPEED, SUICIDE_WANDER_SPEED));
    registerMovementConfig(ELITE, makeMovementConfig(1000.f, 100.f, ELITE_TARGET_SPEED, ELITE_WANDER_SPEED));

    defaultsInitialized = true;
}

void EnemyConfigRegistry::registerMovementConfig(EnemyType type, const EnemyMovementConfig& config) {
    movementStorage()[type] = config;
}

void EnemyConfigRegistry::removeMovementConfig(EnemyType type) {
    movementStorage().erase(type);
}

const EnemyMovementConfig* EnemyConfigRegistry::getMovementConfig(EnemyType type) {
    auto& storage = movementStorage();
    auto it = storage.find(type);
    if (it == storage.end()) {
        return nullptr;
    }
    return &it->second;
}


