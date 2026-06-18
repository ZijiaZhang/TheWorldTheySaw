#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

// A screen-space ring used to highlight a point of interest.
struct HighLightCircle
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createHighLightCircle(vec2 position, float radius, float thickness = 20);
	float radius;
	float thickness;

};
