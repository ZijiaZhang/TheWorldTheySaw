#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "render_components.hpp"
#define POP_UP_BACKGROUND_KEY "pop_up_background"

// Salmon food
struct PopUP
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createPopUP(std::string texture_path, vec2 position, vec2 size);
	static ShadedMesh& get_background();
};
