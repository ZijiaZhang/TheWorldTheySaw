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

};
