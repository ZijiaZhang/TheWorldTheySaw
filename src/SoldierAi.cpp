#include "SoldierAi.hpp"
#include "Enemy.hpp"
#include "soldier.hpp"
#include "debug.hpp"
#include <Bullet.hpp>

void SoldierAISystem::step(float elapsed_ms, vec2 window_size_in_game_units)
{
	timeTicker += elapsed_ms;
	if (!ECS::registry<Soldier>.components.empty())
	{
		auto& soldier = ECS::registry<Soldier>.entities[0];
		SoldierAISystem::makeDecision(soldier, elapsed_ms);
		//std::cout << "velocity: " << ECS::registry<Motion>.get(soldier).velocity.x << ", " << ECS::registry<Motion>.get(soldier).velocity.y << "\n";
	}
}

void SoldierAISystem::makeDecision(ECS::Entity& soldier_entity, float elapsed_ms)
{
	// std::cout << "isEnemyExists\n";
	auto& soldier_motion = ECS::registry<Motion>.get(soldier_entity);
	auto& soldier = ECS::registry<Soldier>.get(soldier_entity);
	// std::cout << "makeDecision: " << &soldier_motion << "\n";
	if (SoldierAISystem::isEnemyExists())
	{
		ECS::Entity& cloestEnemy = SoldierAISystem::getCloestEnemy(soldier_motion);
		auto& enemyMotion = ECS::registry<Motion>.get(cloestEnemy);

		AiState aState = soldier.soldierState;
		if (SoldierAISystem::isEnemyExistsInRange(soldier_motion, enemyMotion, 300) && aState == AiState::WALK_FORWARD_AND_SHOOT) {
			if (timeTicker > fireRate) {
				soldier.soldierState = AiState::WALK_BACKWARD_AND_SHOOT;
				SoldierAISystem::walkBackwardAndShoot(soldier_motion, enemyMotion);
				timeTicker = 0.f;
			}
		}
		else if (SoldierAISystem::isEnemyExistsInRange(soldier_motion, enemyMotion, 400) && aState == AiState::WALK_BACKWARD_AND_SHOOT) {
			if (timeTicker > fireRate) {
				soldier.soldierState = AiState::WALK_BACKWARD_AND_SHOOT;
				SoldierAISystem::walkBackwardAndShoot(soldier_motion, enemyMotion);
				timeTicker = 0.f;
			}
		}
		else
		{
			if (timeTicker > fireRate) {
				soldier.soldierState = AiState::WALK_FORWARD_AND_SHOOT;
				SoldierAISystem::walkForwardAndShoot(soldier_motion, enemyMotion);
				timeTicker = 0.f;
			}
			
		}
	}
	else
	{
		soldier.soldierState = AiState::IDLE;
		SoldierAISystem::idle(soldier_motion);
	}
}

bool SoldierAISystem::isEnemyExists()
{
	return !ECS::registry<Enemy>.components.empty();
}

bool SoldierAISystem::isEnemyExistsInRange(Motion& soldierMotion, Motion& enemyMotion, float range)
{
	// std::cout << "isEnemyExistsInRange: " << &soldierMotion << "\n";
	return sqrt(pow(soldierMotion.position.x - enemyMotion.position.x, 2) + pow(soldierMotion.position.y - enemyMotion.position.y, 2)) < range;
}

void SoldierAISystem::idle(Motion& soldierMotion)
{
	soldierMotion.velocity = vec2{ 0.f, 0.f };
}

void SoldierAISystem::walkBackwardAndShoot(Motion& soldierMotion, Motion& enemyMotion)
{
	// std::cout << "walkBackwardAndShoot: " << &soldierMotion << "\n";
	vec2 soldierPos = soldierMotion.position;
	vec2 enemyPos = enemyMotion.position;


	vec2 posDiff = vec2{ enemyPos.x - soldierPos.x, enemyPos.y - soldierPos.y };
	float distance = sqrt(pow(enemyPos.x - soldierPos.x, 2)) + sqrt(pow(enemyPos.y - soldierPos.y, 2));
	vec2 normalized = vec2{ posDiff.x / distance, posDiff.y / distance };

	soldierMotion.velocity = vec2{ normalized.x * -100.f, normalized.y * -100.f };


	// soldierMotion.velocity = vec2{ -100.f, 0 };

	// std::cout << "backward: " << soldierMotion.velocity.x;

	auto dir = enemyPos - soldierPos;
	float rad = atan2(dir.y, dir.x);
	soldierMotion.angle = rad;

	auto bullet = Bullet::createBullet(soldierMotion.position, soldierMotion.angle);
	auto& bullet_motion = ECS::registry<Motion>.get(bullet);
	bullet_motion.velocity = normalized * 380.f;
	// std::cout << "backward: " << soldierMotion.velocity.x << ", " << soldierMotion.velocity.y << "\n";
}

void SoldierAISystem::walkForwardAndShoot(Motion& soldierMotion, Motion& enemyMotion)
{
	// std::cout << "walkForwardAndShoot: " << &soldierMotion << "\n";
	vec2 soldierPos = soldierMotion.position;
	vec2 enemyPos = enemyMotion.position;



	vec2 posDiff = vec2{ enemyPos.x - soldierPos.x, enemyPos.y - soldierPos.y };
	float distance = sqrt(pow(enemyPos.x - soldierPos.x, 2)) + sqrt(pow(enemyPos.y - soldierPos.y, 2));
	vec2 normalized = vec2{ posDiff.x / distance, posDiff.y / distance };

	soldierMotion.velocity = vec2{ normalized.x * 100.f, normalized.y * 100.f };


	// soldierMotion.velocity = vec2{ 100.f, 0 };

	auto dir = enemyPos - soldierPos;
	float rad = atan2(dir.y, dir.x);
	soldierMotion.angle = rad;

	auto bullet = Bullet::createBullet(soldierMotion.position, soldierMotion.angle);
	auto& bullet_motion = ECS::registry<Motion>.get(bullet);
	bullet_motion.velocity = normalized * 380.f;
	// std::cout << "forward: " << soldierMotion.velocity.x << ", " << soldierMotion.velocity.y << "\n";
}

ECS::Entity& SoldierAISystem::getCloestEnemy(Motion& soldierMotion)
{
	// std::cout << "getCloestEnemy: " << &soldierMotion << "\n";
	auto& enemyList = ECS::registry<Enemy>.entities;
	float minDistance = FLT_MAX;
	ECS::Entity& closestEnemy = enemyList[0];
	for (ECS::Entity& enemyEntity : enemyList) {
		auto& enemyMotion = ECS::registry<Motion>.get(enemyEntity);
		float distance = pow(soldierMotion.position.x - enemyMotion.position.x, 2) + pow(soldierMotion.position.y - enemyMotion.position.y, 2);
		if (distance < minDistance) {
			minDistance = distance;
			closestEnemy = enemyEntity;
		}
	}
	return closestEnemy;
}