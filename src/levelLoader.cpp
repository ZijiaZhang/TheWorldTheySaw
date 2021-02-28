//
//  levelLoader.cpp
//  salmon
//
//  Created by Haofeng Winter Feng on 2021-02-28.
//

#include "levelLoader.hpp"
#include <nlohmann/json.hpp>
#include "Wall.hpp"
#include "soldier.hpp"
#include "Enemy.hpp"
#include "tiny_ecs.hpp"
#include <fstream>
#include <string.h>
#include <cassert>
#include <sstream>
#include <iostream>
#include <unordered_map>

using json = nlohmann::json;

levelLoader::levelLoader() {
    std::cout << "\n Level is loading!!!!!! \n";
    at_level = 1;
    make_level1();
    make_level2();
    levels.push_back(level1);
    levels.push_back(level2);
    // std::cout << level1;
}

ECS::Entity levelLoader::load_level() {
    json current = levels[at_level - 1];
    auto soldier_pos = current["player"]["position"].get<std::vector<int>>();
    auto enemies = current["enemy"].get<std::vector<json>>();
    for (auto e : enemies) {
        auto enemy_pos = e["position"].get<std::vector<int>>();
        Enemy::createEnemy({enemy_pos[0], enemy_pos[1]});
    }
    auto blocks = current["map"].get<std::vector<json>>();
    for (auto b : blocks) {
        int size = b["size"].get<int>();
        auto block_pos = b["position"].get<std::vector<int>>();
        Wall::createWall(vec2{block_pos[0] + size, block_pos[1] + size/2}, {size/5, size}, 0);
        Wall::createWall(vec2{block_pos[0], block_pos[1] + size/2}, {size/5, size}, 0);
        Wall::createWall(vec2{block_pos[0] + size/2, block_pos[1]}, {size, size/5}, 0);
        Wall::createWall(vec2{block_pos[0] + size/2, block_pos[1] + size}, {size, size/5}, 0);
    }
    return Soldier::createSoldier({soldier_pos[0],soldier_pos[1]});
}

void levelLoader::set_level(int level){
    at_level = level;
}

void levelLoader::make_level1() {
    json l1;
    l1 = {
        {"player", {
            {"position", {100, 200}},
                {"velocity", {100, 50}}
        }
        },
        {"enemy", {
            {{"position", {200, 500}},
                {"velocity", {50, 50}},
            {"ai", "basic"}
            }}
        },
        {"map", {{
            {"position", {600, 500}},
            {"size", 88}
        }}}
    };
    level1 = l1;
}

void levelLoader::make_level2() {
    json l2;
    l2 = {
        {"player", {
            {"position", {800, 600}},
                {"velocity", {100, 50}}
        }
        },
        {"enemy", {
            {{"position", {200, 500}},
                {"velocity", {50, 50}},
            {"ai", "basic"}
            },
            {{"position", {800, 300}},
                {"velocity", {50, 50}},
            {"ai", "basic"}
            },
            {{"position", {850, 100}},
                {"velocity", {50, 50}},
            {"ai", "basic"}
            }
        }
        },
        {"map", {{
            {"position", {600, 500}},
            {"size", 88}
        }}}
    };
    level2 = l2;
}

