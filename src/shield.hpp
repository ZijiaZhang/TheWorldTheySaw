#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "PhysicsObject.hpp"

// Salmon enemy 
struct Shield
{
public:
	// Creates all the associated render resources and default transform
	static ECS::Entity createShield(vec2 position, int teamID, float hp = -1);
	static ECS::Entity createShield(Motion m, Shield s, Health h, PhysicsObject po);
    static void shield_bullet_hit_death(ECS::Entity self, const ECS::Entity e, CollisionResult);
	int teamID = 0;
};
