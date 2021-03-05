#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

// Salmon food
struct Background
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createBackground(vec2 position, std::string name);
};
