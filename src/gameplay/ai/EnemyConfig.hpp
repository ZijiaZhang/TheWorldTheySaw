#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "Enemy.hpp"

#include <unordered_map>

struct EnemyMovementConfig {
    float act_distance = 0.f;
    float path_accuracy = 0.f;
    float speed = 0.f;
    float wander_speed = 0.f;
    float backpedal_speed = 0.f;
};

class EnemyConfigRegistry {
public:
    static void initializeDefaults();

    static void registerMovementConfig(EnemyType type, const EnemyMovementConfig& config);
    static void removeMovementConfig(EnemyType type);
    static const EnemyMovementConfig* getMovementConfig(EnemyType type);

private:
    static std::unordered_map<EnemyType, EnemyMovementConfig>& movementStorage();
    static bool defaultsInitialized;
};


