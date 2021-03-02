#include "EnemyAi.hpp"
#include "Enemy.hpp"
#include "soldier.hpp"
#include "debug.hpp"

void EnemyAISystem::step(float elapsed_ms, vec2 window_size_in_game_units) 
{

}

void makeDecision(ECS::Entity enemy_entity, float elapsed_ms)
{

}

bool isSoldierExists()
{
	return false;
}

bool isSoldierExistsInRange(Motion& enemyMotion, Motion& soldierMotion, float range) 
{
	return false;
}

void idle(Motion& soldierMotion, Motion& enemyMotion) 
{

}

void walkBackwardAndShoot(Motion& enemyMotion) 
{

}

void walkRandom(Motion& enemyMotion)
{

}