#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

struct Healthbar
{
public:
	static void updateHealthBar(ECS::Entity e, bool draw);
	// static void updateSoldierHealthBar(vec2 position, vec2 scale, float hp, float max_hp);

	static ECS::Entity drawHealthBar(vec2 position, vec2 scale, float hp, float max_hp, vec3 color, std::string layer);

	static void updateHealthBarPosition(vec2 position, vec2 scale, float hp, float max_hp, ECS::Entity hb);
};
