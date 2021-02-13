#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"


struct Soldier
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createSoldier(vec2 pos);
};
