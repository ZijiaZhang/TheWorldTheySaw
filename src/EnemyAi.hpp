#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "PhysicsObject.hpp"

class EnemyAISystem
{
public:
    void step(float elapsed_ms, vec2 window_size_in_game_units);

private:

    // Decision tree if statements
    void makeDecision(ECS::Entity enemy_entity, float elapsed_ms);

    bool isSoldierExists();
    
    bool isSoldierExistsInRange(Motion& enemyMotion, Motion& soldierMotion, float range);
    
    void idle(Motion& enemyMotion);

    void walkBackwardAndShoot(Motion& soldierMotion, Motion& enemyMotion);

    void walkRandom(Motion& enemyMotion);
};
