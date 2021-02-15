#pragma once

#include <vector>

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "PhysicsObject.hpp"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// DON'T WORRY ABOUT THIS CLASS UNTIL ASSIGNMENT 3
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

class AISystem
{
public:
	void step(float elapsed_ms, vec2 window_size_in_game_units);

    void enemy_ai_step(ECS::Entity e, float elapsed_ms);

    void find_path_to_location(const ECS::Entity &agent, vec2 position);

    template <CollisionObjectType T>
    void add_grids_to_set(const Motion &motion, const PhysicsObject &obj);

    template <CollisionObjectType T>
    static std::set<std::pair<int,int>> occupied_grids_Enemy;

    template <CollisionObjectType T>
    void build_grids_for_type();
};


