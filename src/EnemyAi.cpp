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
    elite_shoot_time += elapsed_ms;
	if (!ECS::registry<Enemy>.components.empty())
	{
        if (timeTicker > ENEMY_MOVEMENT_REFRESH) {
            for (auto& enemy : ECS::registry<Enemy>.entities)
            {
                if (EnemyAISystem::underEffectControl(enemy, elapsed_ms)) {
                    continue;
                }
                EnemyAISystem::makeDecision(enemy, elapsed_ms);
            }
            timeTicker = 0;
        }
        if (shoot_time > SHOOT_INTERVAL){
            for (auto& enemy_entity : ECS::registry<Enemy>.entities)
            {
                if (EnemyAISystem::underEffectControl(enemy_entity, elapsed_ms)) {
                    continue;
                }if (ECS::registry<Motion>.has(enemy_entity) && ECS::registry<Enemy>.has(enemy_entity)) {
                    auto& enemy_motion = ECS::registry<Motion>.get(enemy_entity);
                    auto& enemy = ECS::registry<Enemy>.get(enemy_entity);
                    if (enemy.type == EnemyType::STANDARD) {
                        Bullet::createBullet(enemy_motion.position, enemy_motion.angle, { 380, 0 }, 1, W_BULLET, "bullet", 1000);
                    }
                }
            }
            shoot_time = 0;
        }
        if (elite_shoot_time > ELITE_SHOOT_INTERVAL) {
            for (auto& enemy_entity : ECS::registry<Enemy>.entities)
            {
                if (EnemyAISystem::underEffectControl(enemy_entity, elapsed_ms)) {
                    continue;
                }
                if (ECS::registry<Motion>.has(enemy_entity) && ECS::registry<Enemy>.has(enemy_entity)) {
                    auto& enemy_motion = ECS::registry<Motion>.get(enemy_entity);
                    auto& enemy = ECS::registry<Enemy>.get(enemy_entity);
                    if (enemy.type == EnemyType::ELITE) {
                        auto callback = [](ECS::Entity e) {
                            if (e.has<Motion>()) {
                                Explosion::CreateExplosion(e.get<Motion>().position, 50, 1, 1);
                            }
                            if (!e.has<DeathTimer>())
                                e.emplace<DeathTimer>();
                        };
                        Bullet::createBullet(enemy_motion.position, enemy_motion.angle, { 150, 0 }, 1, W_ROCKET, "rocket", 2000, callback);
                    }
                }
            }
            elite_shoot_time = 0;
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
      
            if (EnemyAISystem::isSoldierExistsInRange(enemy_motion, soldierMotion, 100.f) && enemy_entity.get<Enemy>().type == EnemyType::SUICIDE) {
                Explosion::CreateExplosion(enemy_motion.position, 100, 1, 7);
                if(!enemy_entity.has<DeathTimer>())
                    enemy_entity.emplace<DeathTimer>();
            }

			else if (EnemyAISystem::isSoldierExistsInRange(enemy_motion, soldierMotion, 500.f)) {
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

void EnemyAISystem::takeDamage(ECS::Entity enemy_entity, float damage) {
    if(enemy_entity.has<Health>()){
        auto& health = enemy_entity.get<Health>();
        health.hp -= damage;
        if (health.hp <= 0 && !enemy_entity.has<DeathTimer>()){
            enemy_entity.emplace<DeathTimer>();
        }
    } else {
        if (!enemy_entity.has<DeathTimer>()){
            enemy_entity.emplace<DeathTimer>();
        }
    }
}

bool EnemyAISystem::underEffectControl(ECS::Entity enemy, float elapsed_ms) {
    if (ECS::registry<FrozenTimer>.has(enemy)) {
        auto& fc = enemy.get<FrozenTimer>();
        fc.executing_ms -= elapsed_ms;
        if (fc.executing_ms <= 0.) {
            ECS::registry<FrozenTimer>.remove(enemy);
            ECS::registry<Activating>.remove(enemy);
            Enemy::set_shader(enemy);
            return false;
        } else {
            enemy.get<Enemy>().enemyState = AiState::IDLE;
            EnemyAISystem::idle(ECS::registry<Motion>.get(enemy));
            return true;
        }
    }
    return false;
}