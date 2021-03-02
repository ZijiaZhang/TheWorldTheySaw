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
#include "MoveableWall.hpp"
#include "Bullet.hpp"
#include "render_components.hpp"
#include <fstream>
#include <string.h>
#include <cassert>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <utility>

using json = nlohmann::json;

std::string at_level = "level_1";

void enemy_bullet_hit_death(ECS::Entity& self, const ECS::Entity &e) {
    if (e.has<Bullet>() && !self.has<DeathTimer>()){
        self.emplace<DeathTimer>();
    }
};

std::unordered_map<std::string, std::function<void(ECS::Entity&, const  ECS::Entity&)>> LevelLoader::physics_callbacks = {
        {"enemy_bullet_hit_death", enemy_bullet_hit_death},
};

std::unordered_map<std::string, std::function<void(vec2 location, vec2 size, float rotation,
        std::function<void(ECS::Entity&, const  ECS::Entity&)>, std::function<void(ECS::Entity&, const  ECS::Entity&)>)>> LevelLoader::level_objects = {
        {"blocks", Wall::createWall},
        {"borders", Wall::createWall},
        {"movable_wall", MoveableWall::createMoveableWall},
        {"player", [](vec2 location, vec2 size, float rotation,
                std::function<void(ECS::Entity&, const  ECS::Entity&)> overlap,
                std::function<void(ECS::Entity&, const  ECS::Entity&)> hit){return Soldier::createSoldier(location);}},
        {"enemy", [](vec2 location, vec2 size, float rotation,
                     std::function<void(ECS::Entity&, const  ECS::Entity&)> overlap,
                     std::function<void(ECS::Entity&, const  ECS::Entity&)> hit) {return Enemy::createEnemy(location, std::move(overlap), std::move(hit));}}
};

/**
 read json content from the level files
 */
static json readLevelJsonFile(std::string level) {
    auto file_name = level + ".json";
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

void LevelLoader::load_level() {
    json current = readLevelJsonFile(at_level);
    for (auto & level_object : level_objects) {
        if (current.contains(level_object.first)) {
            for (json b: current[level_object.first]) {
                vec2 position = b.contains("position") ? getVec2FromJson(b["position"]) : vec2{};
                vec2 size = b.contains("size") ? getVec2FromJson(b["size"]) : vec2{};
                float rotation = b.contains("rotation") ? static_cast<float>(b["rotation"]) : 0.f;
                auto overlap = b.contains("overlap") ? physics_callbacks[b["overlap"]] : [](ECS::Entity&, const ECS::Entity &e) {};
                auto hit = b.contains("hit") ? physics_callbacks[b["hit"]] : [](ECS::Entity&, const ECS::Entity &e) {};
                level_object.second(position, size, rotation, overlap, hit);
            }
        }
    }
}

void LevelLoader::set_level(std::string level){
    at_level = level;
}
