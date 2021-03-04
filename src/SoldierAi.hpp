#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "PhysicsObject.hpp"

class SoldierAISystem
{
public:
    void step(float elapsed_ms, vec2 window_size_in_game_units);

private:
    float timeTicker = 0.f;

    float fireRate = 200.f;
    // Decision tree if statements
    void makeDecision(ECS::Entity& soldier_entity, float elapsed_ms);

    bool isEnemyExists();

    ECS::Entity getCloestEnemy(Motion& soldierMotion);

    bool isEnemyExistsInRange(Motion& soldierMotion, Motion& enemyMotion, float range);

    void idle(Motion& soldierMotion);

    void walkBackwardAndShoot(Motion& soldierMotion, Motion& enemyMotion);

    void walkForwardAndShoot(Motion& soldierMotion, Motion& enemyMotion);
};


