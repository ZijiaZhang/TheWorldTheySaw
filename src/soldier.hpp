#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"


struct Soldier
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createSoldier(vec2 pos);

	enum state {IDLE, WALK_BACKWARD_AND_SHOOT, WALK_FORWARD_AND_SHOOT} soldierState;
};
