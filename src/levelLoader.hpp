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
        int get_level_state(std::string level);
        bool is_level_unlocked(std::string level);
        std::string get_next_level_name(std::string level);
        void update_level_state(std::string level, int state);
        // void set_level_value(std::string level, int value);
        void set_level(std::string level);
        void load_level();
    
    std::string at_level = "level_1";
    
    std::vector<json> levels;

    // static std::unordered_map<std::string, int> level_progression;
    static std::vector<std::string> level_order;
    static std::vector<std::string> existing_level;
    static std::unordered_map<std::string, COLLISION_HANDLER> physics_callbacks;
    static std::unordered_map<std::string, std::function<void(vec2 location, vec2 size, float rotation,
                                                              COLLISION_HANDLER, COLLISION_HANDLER, json)>> level_objects;
    static std::unordered_map<std::string, COLLISION_HANDLER> default_hit_callback;

};
