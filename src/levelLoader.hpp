//
//  levelLoader.hpp
//  salmon
//
//  Created by Haofeng Winter Feng on 2021-02-28.
//

#pragma once
#include "common.hpp"
#include "tiny_ecs.hpp"
#include "physics.hpp"
#include "PhysicsObject.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct IntersectionResult{
    bool has_intersect = false;
    vec2 intersect_point = vec2{0, 0};
};

class LevelLoader {
    public:
        void set_level(std::string level);
        void load_level();
    
    std::string at_level = "level_1";

    std::vector<json> levels;
    static std::unordered_map<std::string, COLLISION_HANDLER> physics_callbacks;
    static std::unordered_map<std::string, std::function<void(vec2 location, vec2 size, float rotation,
                                                              COLLISION_HANDLER, COLLISION_HANDLER, json)>> level_objects;

    static void LevelLoader::wall_hit(ECS::Entity& self, const ECS::Entity& e, CollisionResult collision);
};
