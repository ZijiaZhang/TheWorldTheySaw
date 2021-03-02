//
//  levelLoader.hpp
//  salmon
//
//  Created by Haofeng Winter Feng on 2021-02-28.
//

#pragma once
#include "common.hpp"
#include "tiny_ecs.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class LevelLoader {
    public:
        void set_level(int level);
        void load_level();
    
    int at_level = 1;

    std::vector<json> levels;
    static std::unordered_map<std::string, std::function<void(ECS::Entity&, const  ECS::Entity&)>> physics_callbacks;
    static std::unordered_map<std::string, std::function<void(vec2 location, vec2 size, float rotation,
                                                              std::function<void(ECS::Entity&, const  ECS::Entity&)>, std::function<void(ECS::Entity&, const  ECS::Entity&)>)>> level_objects;
};
