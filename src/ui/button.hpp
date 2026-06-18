#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "PhysicsObject.hpp"

// A generic clickable UI button. Supply a sprite texture (relative to
// data/textures/, or "" for an untextured quad) and an on-click handler that
// runs when WorldSystem::tryClickButton dispatches an Overlap event to it.
struct Button {
	static ECS::Entity createButton(vec2 position, vec2 size, const std::string& texture_path, COLLISION_HANDLER on_click);

	bool is_selected = false;
	bool selected() const { return is_selected; }
};
