#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include <AiState.hpp>


struct Soldier
{
public:
	// Creates all the associated render resources and default transform
	static ECS::Entity createSoldier(vec2 pos);

	AiState soldierState = AiState::IDLE;
};
