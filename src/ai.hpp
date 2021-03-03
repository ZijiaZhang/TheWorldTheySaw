#pragma once

#include <vector>

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "PhysicsObject.hpp"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// DON'T WORRY ABOUT THIS CLASS UNTIL ASSIGNMENT 3
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

struct Path_with_heuristics{
    std::vector<std::pair<int,int>> path;
    float cost;
    float heuristic;
};

class AISystem
{
public:
	void step(float elapsed_ms);

    void enemy_ai_step(ECS::Entity e, float elapsed_ms, float radius);

    Path_with_heuristics find_path_to_location(const ECS::Entity &agent, vec2 position, float radius);

    template <CollisionObjectType T>
    void add_grids_to_set(const Motion &motion, const PhysicsObject &obj);

    static std::map<CollisionObjectType, std::set<std::pair<int,int>>> occupied_grids_Enemy;

    template <CollisionObjectType T>
    void build_grids_for_type();

    static std::pair<int, int> get_grid_from_loc(vec2 vec);

    float get_dist(const std::pair<int, int> &cur_grid, const std::pair<int, int> &dest_grid) const;

    vec2 get_grid_location(std::pair<int, int> grid);
};


