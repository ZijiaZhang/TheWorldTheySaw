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
#include "background.hpp"
#include "tiny_ecs.hpp"
#include <fstream>
#include <string.h>
#include <cassert>
#include <sstream>
#include <iostream>
#include <unordered_map>

using json = nlohmann::json;

int at_level = 1;

/**
 read json content from the level files
 */
static json readLevelJsonFile(int level) {
    auto file_name = "level_" + std::to_string(level) + ".json";
    auto obj_path =level_path(file_name);
    auto level_file = std::ifstream{obj_path};
    if (!level_file) {
        throw std::runtime_error("Could not open json file " + obj_path);
    }
    json j;
    level_file >> j;

    return j;
}

static vec2 getVec2FromJson(json j) {
    return vec2(j["x"], j["y"]);
}

static void loadPlayer(json current) {
    auto soldier_pos = current["player"]["position"];
    Soldier::createSoldier(getVec2FromJson(soldier_pos));
}

static void loadBackground(json current) {
    auto background = current["background"];
    Background::createBackground(getVec2FromJson(background["position"]));
}

static void loadWalls(json map, std::string type) {
    auto blocks = map[type];
    for (auto b : blocks) {
        auto block_pos = getVec2FromJson(b["position"]);
        auto block_size = getVec2FromJson(b["size"]);
        auto block_rot = b["rotation"];
        
        Wall::createWall(block_pos, block_size, block_rot);
    }
}

static void loadMap(json current){
    auto map = current["map"];
    loadWalls(map, "borders");
    loadWalls(map, "blocks");
}

static void loadEnemies(json current) {
    auto enemies = current["enemy"];
    for (auto e : enemies) {
        auto enemy_pos = e["position"];
        Enemy::createEnemy(getVec2FromJson(enemy_pos));
    }
}

void LevelLoader::load_level() {
    json current = readLevelJsonFile(at_level);
    
    loadPlayer(current);
    loadBackground(current);
    loadEnemies(current);
    loadMap(current);
    
}

void LevelLoader::set_level(int level){
    at_level = level;
}
