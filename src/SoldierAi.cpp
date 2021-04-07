#include "SoldierAi.hpp"
#include "Enemy.hpp"
#include "soldier.hpp"
#include "debug.hpp"
#include "Weapon.hpp"
#include "Explosion.hpp"
#include "GameInstance.hpp"
#include <Bullet.hpp>
#include <float.h>

std::unordered_map<AIAlgorithm, std::function<void(ECS::Entity, float)>> SoldierAISystem::algorithmMap = {
        {DIRECT, SoldierAISystem::direct_movement},
        {A_STAR, a_star_to_closest_enemy}
};

std::unordered_map<WeaponType , std::function<void(ECS::Entity, float)>> SoldierAISystem::weaponMap = {
        {W_ROCKET, shoot_rocket},
        {W_AMMO, shoot_ammo},
        {W_LASER, shoot_laser},
        {W_BULLET, shoot_bullet}
};

float SoldierAISystem::pathTicker = 0.f;
float SoldierAISystem::weaponTicker = 0.f;

// TODO: debug pathfinding / decision making for A* algorithm, and set this value back to 200.f
float SoldierAISystem::updateRate = 500.f; 

void SoldierAISystem::step(float elapsed_ms, vec2 window_size_in_game_units)
{
    pathTicker += elapsed_ms;
    weaponTicker += elapsed_ms;

	if (!ECS::registry<Soldier>.components.empty())
	{
		auto& soldier = ECS::registry<Soldier>.entities[0];
        algorithmMap[GameInstance::algorithm](soldier, elapsed_ms);
        if(soldier.get<Soldier>().weapon.has<Weapon>()) {
            weaponMap[GameInstance::selectedWeapon](soldier, elapsed_ms);
        }
	}
}
void SoldierAISystem::shoot_bullet(ECS::Entity soldier_entity, float elapsed_ms) {
    if(weaponTicker > 200.f) {
        auto& weapon = soldier_entity.get<Soldier>().weapon;
        auto& soldier_motion = soldier_entity.get<Motion>();
        ECS::Entity cloestEnemy = SoldierAISystem::getCloestEnemy(soldier_motion);
        if (ECS::registry<Motion>.has(cloestEnemy)) {
            auto &enemyMotion = ECS::registry<Motion>.get(cloestEnemy);
            if (weapon.has<Motion>()) {
                auto &motion = weapon.get<Motion>();
                auto dir = enemyMotion.position - motion.position;
                float rad = atan2(dir.y, dir.x);
                motion.offset_angle = rad - soldier_motion.angle;
                Bullet::createBullet(motion.position, rad, {380, 0}, 0, W_BULLET, "bullet");
//                 Bullet::createBullet(motion.position, rad, {380, 0}, 0, "bullet");
                Mix_Chunk*  gun_fire = Mix_LoadWAV(audio_path("gun_fire.wav").c_str());
                // std::cout << "fire_bullet \n";

                if (gun_fire == nullptr)
                    throw std::runtime_error("Failed to load sounds make sure the data directory is present: " +
                        audio_path("gun_fire.wav"));

                Mix_PlayChannel(-1, gun_fire, 0);
                //Mix_FreeChunk(gun_fire);
                
            }
        }

        weaponTicker = 0;
    }
}

void SoldierAISystem::shoot_rocket(ECS::Entity soldier_entity, float elapsed_ms) {
    if(weaponTicker > 800.f) {
        auto& weapon = soldier_entity.get<Soldier>().weapon;
        auto& soldier_motion = soldier_entity.get<Motion>();
        ECS::Entity cloestEnemy = SoldierAISystem::getCloestEnemy(soldier_motion);
        if (ECS::registry<Motion>.has(cloestEnemy)) {
            auto &enemyMotion = ECS::registry<Motion>.get(cloestEnemy);
            if (weapon.has<Motion>()) {
                auto &motion = weapon.get<Motion>();
                auto dir = enemyMotion.position - motion.position;
                float rad = atan2(dir.y, dir.x);
                motion.offset_angle = rad - soldier_motion.angle;
                auto callback = [](ECS::Entity e){
                    if(e.has<Motion>()) {
                        Explosion::CreateExplosion(e.get<Motion>().position, 20, 0);
                    }
                    ECS::ContainerInterface::remove_all_components_of(e);
                };
                Bullet::createBullet(motion.position, rad, {150, 0},  0, W_ROCKET, "rocket", 2000,
                                     callback);

                Mix_Chunk* gun_fire = Mix_LoadWAV(audio_path("firework.wav").c_str());
                if (gun_fire == nullptr)
                    throw std::runtime_error("Failed to load sounds make sure the data directory is present: " +
                        audio_path("firework.wav"));

                Mix_PlayChannel(-1, gun_fire, 0);
                //Mix_FreeChunk(gun_fire);
            }
        }

        weaponTicker = 0;
    }
}

void SoldierAISystem::shoot_laser(ECS::Entity soldier_entity, float elapsed_ms) {
    if(weaponTicker > 150.f) {
        auto& weapon = soldier_entity.get<Soldier>().weapon;
        auto& soldier_motion = soldier_entity.get<Motion>();
        ECS::Entity cloestEnemy = SoldierAISystem::getCloestEnemy(soldier_motion);
        if (ECS::registry<Motion>.has(cloestEnemy)) {
            auto &enemyMotion = ECS::registry<Motion>.get(cloestEnemy);
            if (weapon.has<Motion>()) {
                auto &motion = weapon.get<Motion>();
                auto dir = enemyMotion.position - motion.position;
                float rad = atan2(dir.y, dir.x);
                motion.offset_angle = rad - soldier_motion.angle;
                Bullet::createBullet(motion.position, rad, {400, 0}, 0, W_LASER, "laser");
                //Bullet::createBullet(motion.position, rad, {400, 0}, 0, "laser");
                // std::cout << "fire_laser \n";
                Mix_Chunk*  gun_fire = Mix_LoadWAV(audio_path("laser.wav").c_str());
                if (gun_fire == nullptr)
                    throw std::runtime_error("Failed to load sounds make sure the data directory is present: " +
                        audio_path("laser.wav"));

                Mix_PlayChannel(-1, gun_fire, 0);
                //Mix_FreeChunk(gun_fire);
            }
        }

        weaponTicker = 0;
    }
}

void SoldierAISystem::shoot_ammo(ECS::Entity soldier_entity, float elapsed_ms) {
    if(weaponTicker > 300.f) {
        auto& weapon = soldier_entity.get<Soldier>().weapon;
        auto& soldier_motion = soldier_entity.get<Motion>();
        ECS::Entity cloestEnemy = SoldierAISystem::getCloestEnemy(soldier_motion);
        if (ECS::registry<Motion>.has(cloestEnemy)) {
            auto &enemyMotion = ECS::registry<Motion>.get(cloestEnemy);
            if (weapon.has<Motion>()) {
                auto &motion = weapon.get<Motion>();
                auto dir = enemyMotion.position - motion.position;
                float rad = atan2(dir.y, dir.x);
                motion.offset_angle = rad - soldier_motion.angle;
                Bullet::createBullet(motion.position, rad, {200, 0}, 0, W_AMMO, "ammo");
                // Bullet::createBullet(motion.position, rad, {200, 0}, 0, "ammo");

                // std::cout << "fire_ammo \n";
                Mix_Chunk* gun_fire = Mix_LoadWAV(audio_path("ammo.wav").c_str());
                if (gun_fire == nullptr)
                    throw std::runtime_error("Failed to load sounds make sure the data directory is present: " +
                        audio_path("ammo.wav"));

                Mix_PlayChannel(-1, gun_fire, 0);
                //Mix_FreeChunk(gun_fire);
            }
        }

        weaponTicker = 0;
    }
}


void SoldierAISystem::a_star_to_closest_enemy(ECS::Entity soldier_entity, float elapsed_ms){
    if (ECS::registry<Motion>.has(soldier_entity) && ECS::registry<Soldier>.has(soldier_entity)) {
        auto& soldier_motion = ECS::registry<Motion>.get(soldier_entity);
        auto& soldier = ECS::registry<Soldier>.get(soldier_entity);

        if (SoldierAISystem::isEnemyExists() && soldier_entity.has<AIPath>()) {
            soldier_entity.get<AIPath>().active = true;

            ECS::Entity cloestEnemy = SoldierAISystem::getCloestEnemy(soldier_motion);
            if (ECS::registry<Motion>.has(cloestEnemy)) {
                auto &enemyMotion = ECS::registry<Motion>.get(cloestEnemy);

                AiState aState = soldier.soldierState;
                if (pathTicker > updateRate) {
                    soldier.soldierState = AiState::WALK_FORWARD;
                    soldier_entity.get<AIPath>().path = AISystem::find_path_to_location(soldier_entity,
                                                                                        enemyMotion.position, 100);
                    soldier_entity.get<AIPath>().progress = 0;
                    soldier_entity.get<AIPath>().desired_speed = {70, 0};
                    pathTicker = 0.f;
                    return;
                }
            }
        } else {
            soldier.soldierState = AiState::IDLE;
            SoldierAISystem::idle(soldier_motion);
            soldier_entity.get<AIPath>().path = Path_with_heuristics{};
            soldier_entity.get<AIPath>().progress = 0;
        }
    }
}

void SoldierAISystem::direct_movement(ECS::Entity soldier_entity, float elapsed_ms)
{
	if (ECS::registry<Motion>.has(soldier_entity) && ECS::registry<Soldier>.has(soldier_entity)) {

		auto& soldier_motion = ECS::registry<Motion>.get(soldier_entity);
		auto& soldier = ECS::registry<Soldier>.get(soldier_entity);
        if(soldier_entity.has<AIPath>()){
            soldier_entity.get<AIPath>().active = false;
        }
		if (SoldierAISystem::isEnemyExists()) {
			ECS::Entity cloestEnemy = SoldierAISystem::getCloestEnemy(soldier_motion);
			if (ECS::registry<Motion>.has(cloestEnemy)) {
				auto& enemyMotion = ECS::registry<Motion>.get(cloestEnemy);

				AiState aState = soldier.soldierState;
				if (SoldierAISystem::isEnemyExistsInRange(soldier_motion, enemyMotion, 300) && aState == AiState::WALK_FORWARD) {
					if (pathTicker > updateRate) {
						soldier.soldierState = AiState::WALK_BACKWARD;
                        SoldierAISystem::walkBackward(soldier_motion, enemyMotion);
                        pathTicker = 0.f;
					}
				}
				else if (SoldierAISystem::isEnemyExistsInRange(soldier_motion, enemyMotion, 500) && aState == AiState::WALK_BACKWARD) {
					if (pathTicker > updateRate) {
						soldier.soldierState = AiState::WALK_FORWARD;
                        SoldierAISystem::walkBackward(soldier_motion, enemyMotion);
                        pathTicker = 0.f;
					}
				}
				else
				{
					if (pathTicker > updateRate) {
						soldier.soldierState = AiState::WALK_FORWARD;
                        SoldierAISystem::walkForward(soldier_motion, enemyMotion);
                        pathTicker = 0.f;
					}

				}
			}
		}
		else
		{
			soldier.soldierState = AiState::IDLE;
			SoldierAISystem::idle(soldier_motion);
		}
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

void SoldierAISystem::walkBackward(Motion& soldierMotion, Motion& enemyMotion)
{
	vec2 soldierPos = soldierMotion.position;
	vec2 enemyPos = enemyMotion.position;
	soldierMotion.velocity = vec2{ -100.f, 0 };
	auto dir = enemyPos - soldierPos;
	float rad = atan2(dir.y, dir.x);
	soldierMotion.angle = rad;
}

void SoldierAISystem::walkForward(Motion& soldierMotion, Motion& enemyMotion)
{

	vec2 soldierPos = soldierMotion.position;
	vec2 enemyPos = enemyMotion.position;
	soldierMotion.velocity = vec2{ 100.f, 0 };
	auto dir = enemyPos - soldierPos;
	float rad = atan2(dir.y, dir.x);
	soldierMotion.angle = rad;
}

ECS::Entity SoldierAISystem::getCloestEnemy(Motion& soldierMotion)
{
	// std::cout << "getCloestEnemy: " << &soldierMotion << "\n";
	auto& enemyList = ECS::registry<Enemy>.entities;
	float minDistance = FLT_MAX;
    ECS::Entity closestEnemy;
    if (enemyList.size() > 0) {
        closestEnemy = enemyList[0];
        for (ECS::Entity enemyEntity : enemyList) {
            if (ECS::registry<Motion>.has(enemyEntity)) {
                auto& enemyMotion = ECS::registry<Motion>.get(enemyEntity);
                float distance = pow(soldierMotion.position.x - enemyMotion.position.x, 2) + pow(soldierMotion.position.y - enemyMotion.position.y, 2);
                if (distance < minDistance) {
                    minDistance = distance;
                    closestEnemy = enemyEntity;
                }
            }
        }
    }
	return closestEnemy;
}
