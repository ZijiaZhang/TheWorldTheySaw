#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

// Salmon food
struct Background
{
	// Creates all the associated render resources and default transform
<<<<<<< Updated upstream
	static ECS::Entity createBackground(vec2 position, std::string name);
=======
	static ECS::Entity createBackground(vec2 position, std::string name, float depth, float size);

	float depth = 0.f;
>>>>>>> Stashed changes
};
