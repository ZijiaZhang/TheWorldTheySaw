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

class levelLoader {
    public:

        levelLoader();

        void set_level(int level);
        

        ECS::Entity load_level();
    
    void make_level1();
    
    void make_level2();
    
    int at_level;
    
    json level1;
    
    json level2;
    
    std::vector<json> levels;

};
