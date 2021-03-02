#include "SoldierAi.hpp"
#include "Enemy.hpp"
#include "soldier.hpp"
#include "debug.hpp"

void SoldierAISystem::step(float elapsed_ms, vec2 window_size_in_game_units)
{
	if (!ECS::registry<Soldier>.components.empty())
	{
		auto soldier = ECS::registry<Soldier>.entities[0];
		auto& soldier_motion = soldier.get<Motion>();

	}
}

void makeDecision(ECS::Entity soldier_entity, float elapsed_ms)
{

}

bool isEnemyExists()
{
	return !ECS::registry<Enemy>.components.empty();
}

bool isEnemyExistsInRange(Motion& soldierMotion, Motion& enemyMotion, float range)
{
	return sqrt(pow(soldierMotion.position.x - enemyMotion.position.x, 2) + pow(soldierMotion.position.y - enemyMotion.position.y, 2)) < range;
}

void idle(Motion& soldierMotion)
{

}

void walkBackwardAndShoot(Motion& soldierMotion)
{

}

void walkForwardAndShoot(Motion& soldierMotion)
{

}

