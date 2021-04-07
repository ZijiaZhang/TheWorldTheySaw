#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

// A simple physics system that moves rigid bodies and checks for collision
struct CollisionResult{
    float penitration = 0;
    vec2 normal = vec2{0,0};
    vec2 vertex = vec2{0,0};
};

struct Force{
    vec2 force = vec2{0,0};
    vec2 position = vec2{0,0};
};


class PhysicsSystem
{
public:
	void step(float elapsed_ms, vec2 window_size_in_game_units, float multiplier);

	// Stucture to store collision information
	struct Collision
	{
		// Note, the first object is stored in the ECS container.entities
		ECS::Entity other; // the second object involved in the collision
		Collision(ECS::Entity other);
	};

    static std::unordered_map<std::string, std::pair<std::vector<std::vector<std::pair<int, int>>>, std::vector<int>>> decomposed_non_convex;

	// Get world velocity from local velocity. Only rotation applied
    static vec2 get_world_velocity(const Motion &motion) ;

    // Get local velocity from world velocity. Only rotation applied
    static vec2 get_local_velocity(vec2 world_velocity, const Motion &motion) ;

    // Trigger advanced collision detection on two objects
    CollisionType advanced_collision(ECS::Entity m1, ECS::Entity m2);

    // The collition results of two advanced physics object
    CollisionResult collision(ECS::Entity m1, ECS::Entity m2);

};


