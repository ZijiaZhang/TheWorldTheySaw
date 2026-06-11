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
        // Move manually
        //algorithmMap[GameInstance::algorithm](soldier, elapsed_ms);
        if(soldier.get<Soldier>().weapon.has<Weapon>()) {
            SoldierAISystem::handleWeaponFire(soldier, GameInstance::selectedWeapon);
        }
        SoldierAISystem::underEffectControl(soldier, elapsed_ms);
	}
}
void SoldierAISystem::handleWeaponFire(ECS::Entity soldier_entity, WeaponType weaponType) {
    if (!ECS::registry<Soldier>.has(soldier_entity) || !ECS::registry<Motion>.has(soldier_entity)) {
        return;
    }

    WeaponConfigRegistry::initializeDefaults();

    const auto* config = WeaponConfigRegistry::getWeaponConfig(weaponType);
    if (config == nullptr) {
        return;
    }

    if (weaponTicker <= config->reloadMs) {
        return;
    }

    auto& soldier = ECS::registry<Soldier>.get(soldier_entity);
    ECS::Entity weapon = soldier.weapon;

    if (!weapon.has<Motion>()) {
        return;
    }

    auto& soldier_motion = ECS::registry<Motion>.get(soldier_entity);
    auto& weapon_motion = weapon.get<Motion>();

    float rad = soldier_motion.angle;
    if (!resolveAimAngle(soldier_motion, weapon_motion, rad)) {
        return;
    }

    weapon_motion.offset_angle = rad - soldier_motion.angle;

    auto spawnConfig = makeSpawnConfig(*config, weapon_motion.position, rad, soldier.teamID);

    applyPreSpawnModifiers(spawnConfig);

    ECS::Entity bulletEntity = spawnBullet(spawnConfig);

    applySpawnScale(bulletEntity, spawnConfig);
    configureExplosionOnHit(bulletEntity, spawnConfig);
    applyPostSpawnModifiers(bulletEntity, spawnConfig);

    playWeaponSound(*config);

    weaponTicker = 0.f;
}

bool SoldierAISystem::resolveAimAngle(Motion& soldierMotion, Motion& weaponMotion, float& outAngle) {
    outAngle = soldierMotion.angle;

    if (!GameInstance::weaponAutoAim) {
        return true;
    }

    ECS::Entity cloestEnemy = SoldierAISystem::getCloestEnemy(soldierMotion);
    if (!ECS::registry<Motion>.has(cloestEnemy)) {
        return false;
    }

    auto &enemyMotion = ECS::registry<Motion>.get(cloestEnemy);
    auto dir = enemyMotion.position - weaponMotion.position;
    outAngle = atan2(dir.y, dir.x);
    return true;
}

BulletSpawnConfig SoldierAISystem::makeSpawnConfig(const WeaponFireConfig& config, vec2 position, float angle, int teamId) {
    BulletSpawnConfig spawnConfig;
    spawnConfig.position = position;
    spawnConfig.angle = angle;
    spawnConfig.velocity = config.muzzleVelocity;
    spawnConfig.scaleMultiplier = config.scaleMultiplier;
    spawnConfig.lifetime_ms = config.lifetime_ms;
    spawnConfig.type = config.type;
    spawnConfig.texture = config.texture;
    spawnConfig.teamId = teamId;
    spawnConfig.explodeOnHit = config.explodeOnHit;
    return spawnConfig;
}

ECS::Entity SoldierAISystem::spawnBullet(const BulletSpawnConfig& config) {
    return Bullet::createBullet(config.position, config.angle, config.velocity, config.teamId, config.type,
                                config.texture, config.lifetime_ms);
}

void SoldierAISystem::applySpawnScale(ECS::Entity bulletEntity, const BulletSpawnConfig& config) {
    if (!ECS::registry<Motion>.has(bulletEntity)) {
        return;
    }
    if (config.scaleMultiplier.x == 1.f && config.scaleMultiplier.y == 1.f) {
        return;
    }
    auto& motion = ECS::registry<Motion>.get(bulletEntity);
    motion.scale.x *= config.scaleMultiplier.x;
    motion.scale.y *= config.scaleMultiplier.y;
}

void SoldierAISystem::configureExplosionOnHit(ECS::Entity bulletEntity, const BulletSpawnConfig& config) {
    if (!config.explodeOnHit) {
        return;
    }

    if (!bulletEntity.has<Bullet>()) {
        return;
    }

    bulletEntity.get<Bullet>().on_destroy = [](ECS::Entity e){
        if(e.has<Motion>()) {
            Explosion::CreateExplosion(e.get<Motion>().position, 80, 0, 5);
        }
        ECS::ContainerInterface::remove_all_components_of(e);
    };
}

void SoldierAISystem::applyPreSpawnModifiers(BulletSpawnConfig& config) {
    const auto& modifiers = WeaponConfigRegistry::getBulletModifiers();
    for (const auto& entry : modifiers) {
        if (entry.second.adjustSpawnConfig) {
            entry.second.adjustSpawnConfig(config);
        }
    }
}

void SoldierAISystem::applyPostSpawnModifiers(ECS::Entity bulletEntity, const BulletSpawnConfig& config) {
    const auto& modifiers = WeaponConfigRegistry::getBulletModifiers();
    for (const auto& entry : modifiers) {
        if (entry.second.afterSpawn) {
            entry.second.afterSpawn(bulletEntity, config);
        }
    }
}

void SoldierAISystem::playWeaponSound(const WeaponFireConfig& config) {
    if (config.soundEffect.empty()) {
        return;
    }

    Mix_Chunk* gun_fire = Mix_LoadWAV(audio_path(config.soundEffect).c_str());
    if (gun_fire == nullptr)
        throw std::runtime_error("Failed to load sounds make sure the data directory is present: " +
                                 audio_path(config.soundEffect));

    Mix_PlayChannel(-1, gun_fire, 0);
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
                    soldier_entity.get<AIPath>().progress = 1;
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

void SoldierAISystem::underEffectControl(ECS::Entity soldier, float elapsed_ms) {
    // if (ECS::registry<FieldTimer>.has(soldier)) {
    //     auto& fc = soldier.get<FieldTimer>();
    //     fc.counter_ms -= elapsed_ms;
    //     if (fc.counter_ms <= 0.) {
    //         ECS::registry<FieldTimer>.remove(soldier);
    //         //ECS::registry<Activating>.remove(soldier);
    //         ECS::registry<Soldier>.get(soldier).forcefield_on = false;
    //         Soldier::set_shader(soldier, true, Soldier::ori_texture_path, Soldier::ori_shader_name);
    //     } else {
    //         soldier.get<Soldier>().soldierState = AiState::IDLE;
    //         SoldierAISystem::idle(ECS::registry<Motion>.get(soldier));
    //     }
    // }
}

