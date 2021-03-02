#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "PhysicsObject.hpp"

class SoldierAISystem
{
public:
    void step(float elapsed_ms, vec2 window_size_in_game_units);

private:

    // Decision tree if statements
    void makeDecision(ECS::Entity soldier_entity, float elapsed_ms);

    bool isEnemyExists();

    bool isEnemyExistsInRange(Motion& soldierMotion, Motion& enemyMotion, float range);

    void idle(Motion& soldierMotion);

    void walkBackwardAndShoot(Motion& soldierMotion);

    void walkForwardAndShoot(Motion& soldierMotion);
};


