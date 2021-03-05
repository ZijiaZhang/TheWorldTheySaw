#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

// Salmon enemy 
struct Shield
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createShield(vec2 position, int teamID);
	int teamID = 0;
};
