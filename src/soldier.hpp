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
	static ECS::Entity createSoldier(vec2 pos);

	AiState soldierState = AiState::IDLE;
	ECS::Entity weapon;
    AIAlgorithm ai_algorithm;
};
