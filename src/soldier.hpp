#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "SoldierAi.hpp"
#include <AiState.hpp>
#include "health_bar.hpp"

#define DEFAULT_LIGHT_INTENSITY 200

enum class SoldierType {
    BASIC, MEDIUM, HEAVY
};

struct Soldier
{
public:
	// Creates all the associated render resources and default transform
	static ECS::Entity createSoldier(vec2 pos, COLLISION_HANDLER overlap = [](ECS::Entity, const ECS::Entity, CollisionResult) {},
                                     COLLISION_HANDLER hit = [](ECS::Entity, const ECS::Entity, CollisionResult) {}, float light_intensity = DEFAULT_LIGHT_INTENSITY);

	AiState soldierState = AiState::IDLE;
	ECS::Entity weapon;
    MagicWeapon magic = FIREBALL;
	int teamID = 0;
	float light_intensity = 200;
	static void soldier_bullet_hit_death(ECS::Entity self, const ECS::Entity e, CollisionResult);
};
