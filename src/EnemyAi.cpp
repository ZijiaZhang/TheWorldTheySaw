#include "EnemyAi.hpp"
#include "Enemy.hpp"
#include "soldier.hpp"
#include "debug.hpp"
#include "Explosion.hpp"
#include <Bullet.hpp>

void EnemyAISystem::step(float elapsed_ms, vec2 window_size_in_game_units)
{
	timeTicker += elapsed_ms;
	shoot_time += elapsed_ms;
	if (!ECS::registry<Enemy>.components.empty())
	{
        if (timeTicker > enemyMovementRefresh) {
            for (auto& enemy : ECS::registry<Enemy>.entities)
            {
                EnemyAISystem::makeDecision(enemy, elapsed_ms);
            }
            timeTicker = 0;
        }
        if (shoot_time > shoot_interval){
            for (auto& enemy_entity : ECS::registry<Enemy>.entities)
            {
                if (ECS::registry<Motion>.has(enemy_entity) && ECS::registry<Enemy>.has(enemy_entity)) {
                    auto& enemy_motion = ECS::registry<Motion>.get(enemy_entity);
                    auto& enemy = ECS::registry<Enemy>.get(enemy_entity);
                    auto callback = [](ECS::Entity e){
                        if(e.has<Motion>()) {
                            Explosion::CreateExplosion(e.get<Motion>().position, 20, 1);
                        }
                        ECS::ContainerInterface::remove_all_components_of(e);
                    };
                    Bullet::createBullet(enemy_motion.position, enemy_motion.angle, { 380, 0 }, 1, "rocket", 2000, callback);
                }
            }
            shoot_time = 0;
        }
	}
}

void EnemyAISystem::makeDecision(ECS::Entity enemy_entity, float elapsed_ms)
{
	// std::cout << "isEnemyExists\n";
	if (ECS::registry<Motion>.has(enemy_entity) && ECS::registry<Enemy>.has(enemy_entity)) {
		auto& enemy_motion = ECS::registry<Motion>.get(enemy_entity);
		auto& enemy = ECS::registry<Enemy>.get(enemy_entity);
		if (EnemyAISystem::isSoldierExists())
		{
			ECS::Entity soldier = ECS::registry<Soldier>.entities[0];
			auto& soldierMotion = ECS::registry<Motion>.get(soldier);

			if (EnemyAISystem::isSoldierExistsInRange(enemy_motion, soldierMotion, 500.f)) {
				enemy.enemyState = AiState::WALK_FORWARD;
				EnemyAISystem::shortestPathToSoldier(enemy_entity, elapsed_ms, soldierMotion.position);
			}
			else
			{
					enemy.enemyState = AiState::WANDER;
					EnemyAISystem::walkRandom(enemy_motion);
			}


		}
		else
		{
			enemy.enemyState = AiState::IDLE;
			EnemyAISystem::idle(enemy_motion);
		}
	}

	// std::cout << "direct_movement: " << &soldier_motion << "\n";

}

bool EnemyAISystem::isSoldierExists()
{
	return !ECS::registry<Soldier>.components.empty();
}

bool EnemyAISystem::isSoldierExistsInRange(Motion& enemyMotion, Motion& soldierMotion, float range)
{
	return sqrt(pow(soldierMotion.position.x - enemyMotion.position.x, 2) + pow(soldierMotion.position.y - enemyMotion.position.y, 2)) < range;
}

void EnemyAISystem::idle(Motion& enemyMotion)
{
	enemyMotion.velocity = vec2{ 0.f, 0.f };
}

void EnemyAISystem::walkBackwardAndShoot(Motion& enemyMotion, Motion& soldierMotion)
{
	// std::cout << "walkBackward: " << &soldierMotion << "\n";
	vec2 soldierPos = soldierMotion.position;
	vec2 enemyPos = enemyMotion.position;


	vec2 posDiff = vec2{ soldierPos.x - enemyPos.x, soldierPos.y - enemyPos.y };
	float distance = sqrt(pow(enemyPos.x - soldierPos.x, 2)) + sqrt(pow(enemyPos.y - soldierPos.y, 2));
	vec2 normalized = vec2{ posDiff.x / distance, posDiff.y / distance };

	enemyMotion.velocity = vec2{ normalized.x * -100.f, normalized.y * -100.f };
	// std::cout << enemyMotion.velocity.x << " ," << enemyMotion.velocity.y << "\n";

	// soldierMotion.velocity = vec2{ -100.f, 0 };

	// std::cout << "backward: " << soldierMotion.velocity.x;

	auto dir = soldierPos - enemyPos;
	float rad = atan2(dir.y, dir.x);
	enemyMotion.angle = rad;

	// std::cout << "backward: " << soldierMotion.velocity.x << ", " << soldierMotion.velocity.y << "\n";
}

void EnemyAISystem::walkRandom(Motion& enemyMotion)
{
	enemyMotion.velocity = vec2{ rand() % 200 - 99, rand() % 200 - 99 };
}

void EnemyAISystem::shortestPathToSoldier(ECS::Entity e, float elapsed_ms, vec2 dest)
{
	auto& motion = e.get<Motion>();
	ai.enemy_ai_step(e, elapsed_ms, dest);
}
