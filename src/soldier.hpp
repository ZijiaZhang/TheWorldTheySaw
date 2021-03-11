#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "SoldierAi.hpp"
#include <AiState.hpp>

enum class SoldierType {
    BASIC, MEDIUM, HEAVY
};

struct Soldier
{
public:
	// Creates all the associated render resources and default transform
	static ECS::Entity createSoldier(vec2 pos, COLLISION_HANDLER overlap = [](ECS::Entity&, const ECS::Entity&, CollisionResult) {},
                                     COLLISION_HANDLER hit = [](ECS::Entity&, const ECS::Entity&, CollisionResult) {});

	AiState soldierState = AiState::IDLE;
	ECS::Entity weapon;
    AIAlgorithm ai_algorithm;
	int teamID = 0;
};
