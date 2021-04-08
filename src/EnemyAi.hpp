#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "PhysicsObject.hpp"
#include "ai.hpp"

class EnemyAISystem
{
public:
    void step(float elapsed_ms, vec2 window_size_in_game_units);

    static void takeDamage(ECS::Entity enemy_entity, float damage);

private:
    AISystem ai;

    float timeTicker = 0.f;

    float enemyMovementRefresh = 30.f;

    float shoot_time = 0.f;
    float elite_shoot_time = 0.f;
    float shoot_interval = 500.f;
    float elite_shoot_interval = 1000.f;
    // Decision tree if statements
    void makeDecision(ECS::Entity enemy_entity, float elapsed_ms);

    bool isSoldierExists();
    
    bool isSoldierExistsInRange(Motion& enemyMotion, Motion& soldierMotion, float range);
    
    void idle(Motion& enemyMotion);

    void walkBackwardAndShoot(Motion& enemyMotion, Motion& soldierMotion);

    void walkRandom(Motion& enemyMotion);

    void shortestPathToSoldier(ECS::Entity e, float elapsed_ms, vec2 dest);

    bool underEffectControl(ECS::Entity enemy, float elapsed_ms);
};
