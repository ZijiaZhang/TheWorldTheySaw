#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

// A fullscreen background-image entity, handy for menu/title screens.
struct MainMenu
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createMainMenu(vec2 position, std::string name, float depth, float size);

	float depth = 0.f;
};
