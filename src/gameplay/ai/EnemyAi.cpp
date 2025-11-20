#include "EnemyAi.hpp"
#include "Enemy.hpp"
#include "soldier.hpp"
#include "debug.hpp"
#include "Explosion.hpp"
#include <Bullet.hpp>



void EnemyAISystem::step(float elapsed_ms, vec2 window_size_in_game_units)
{
    EnemyConfigRegistry::initializeDefaults();

	timeTicker += elapsed_ms;
	shoot_time += elapsed_ms;
    elite_shoot_time += elapsed_ms;
	if (!ECS::registry<Enemy>.components.empty())
	{
        for (auto& enemy_entity : ECS::registry<Enemy>.entities) {
            auto& enemy_motion = ECS::registry<Motion>.get(enemy_entity);
            auto& enemy = ECS::registry<Enemy>.get(enemy_entity);
            if (EnemyAISystem::isSoldierExists())
            {
                ECS::Entity soldier = ECS::registry<Soldier>.entities[0];
                auto& soldierMotion = ECS::registry<Motion>.get(soldier);
                if (EnemyAISystem::isSoldierExistsInRange(enemy_motion, soldierMotion, 100.f) && enemy_entity.get<Enemy>().type == EnemyType::SUICIDE) {
                    Explosion::CreateExplosion(enemy_motion.position, 100, 1, 7);
                    if (!enemy_entity.has<DeathTimer>())
                        enemy_entity.emplace<DeathTimer>();
                }
            }
        }
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
      
            const auto* config = EnemyConfigRegistry::getMovementConfig(enemy.type);
            if (!config) {
                return;
            }

            if (EnemyAISystem::isSoldierExistsInRange(enemy_motion, soldierMotion, config->act_distance)) {
				enemy.enemyState = AiState::WALK_FORWARD;
                if (enemy_entity.has<AIPath>()) {
                    EnemyAISystem::shortestPathToSoldier(enemy_entity, elapsed_ms, soldierMotion.position, config->path_accuracy);
                    enemy_entity.get<AIPath>().desired_speed = { config->speed , 0.f };
                }
                

                
			}
			else
			{
					enemy.enemyState = AiState::WANDER;
                    EnemyAISystem::walkRandom(enemy_motion, config->wander_speed);
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

    EnemyConfigRegistry::initializeDefaults();
    float speed = 80.f;
    if (const auto* config = EnemyConfigRegistry::getMovementConfig(STANDARD)) {
        speed = config->backpedal_speed;
    }
	enemyMotion.velocity = vec2{ normalized.x * -speed, normalized.y * -speed };
	// std::cout << enemyMotion.velocity.x << " ," << enemyMotion.velocity.y << "\n";

	// soldierMotion.velocity = vec2{ -100.f, 0 };

	// std::cout << "backward: " << soldierMotion.velocity.x;

	auto dir = soldierPos - enemyPos;
	float rad = atan2(dir.y, dir.x);
	enemyMotion.angle = rad;

	// std::cout << "backward: " << soldierMotion.velocity.x << ", " << soldierMotion.velocity.y << "\n";
}

void EnemyAISystem::walkRandom(Motion& enemyMotion, float maxSpeed)
{
    float targetSpeed = maxSpeed;
    if (targetSpeed <= 0.f) {
        enemyMotion.velocity = vec2{0.f, 0.f};
        return;
    }

    vec2 randomDir = vec2{ float(rand() % 200 - 99), float(rand() % 200 - 99) };
    float magnitude = sqrt(randomDir.x * randomDir.x + randomDir.y * randomDir.y);
    if (magnitude < 0.001f) {
        enemyMotion.velocity = vec2{targetSpeed, 0.f};
        return;
    }
    float scale = targetSpeed / magnitude;
    enemyMotion.velocity = randomDir * scale;
}

void EnemyAISystem::shortestPathToSoldier(ECS::Entity e, float elapsed_ms, vec2 dest, float distance)
{
	auto& motion = e.get<Motion>();
	ai.enemy_ai_step(e, elapsed_ms, dest, distance);
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
    return false;
}