#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "render_components.hpp"

// A generic screen-space dialog backed by a sprite texture.
struct PopUP
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createPopUP(std::string texture_path, vec2 position, vec2 size);
	std::vector<ECS::Entity> relative_entities;
	std::function<void()> on_destroy = [] (){};
};
