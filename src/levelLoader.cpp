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
#include "start.hpp"
#include "tiny_ecs.hpp"
#include "MoveableWall.hpp"
#include "Bullet.hpp"
#include "render_components.hpp"
#include "button.hpp"
#include "world.hpp"
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

std::unordered_map<std::string, std::function<void(vec2, vec2, float,
        std::function<void(ECS::Entity&, const  ECS::Entity&)>, std::function<void(ECS::Entity&, const  ECS::Entity&)>, json)>> LevelLoader::level_objects = {
        {"blocks", [](vec2 location, vec2 size, float rotation,
                           std::function<void(ECS::Entity&, const  ECS::Entity&)> overlap, std::function<void(ECS::Entity&, const  ECS::Entity&)>hit, const json&) {
            Wall::createWall(location, size, rotation, overlap, hit);
        }
        },
        {"borders", [](vec2 location, vec2 size, float rotation,
                       std::function<void(ECS::Entity&, const  ECS::Entity&)> overlap, std::function<void(ECS::Entity&, const  ECS::Entity&)>hit, const json&) {
            Wall::createWall(location, size, rotation, overlap, hit);
        }
        },
        {"movable_wall", [](vec2 location, vec2 size, float rotation,
                            std::function<void(ECS::Entity&, const  ECS::Entity&)> overlap, std::function<void(ECS::Entity&, const  ECS::Entity&)>hit, const json&) {
            MoveableWall::createMoveableWall(location, size, rotation, overlap, hit);
        }
        },
        {"player", [](vec2 location, vec2 size, float rotation,
                std::function<void(ECS::Entity&, const  ECS::Entity&)>,
                std::function<void(ECS::Entity&, const  ECS::Entity&)>, const json&){
            return Soldier::createSoldier(location);
        }},
        {"enemy", [](vec2 location, vec2 size, float rotation,
                     std::function<void(ECS::Entity&, const  ECS::Entity&)> overlap,
                     std::function<void(ECS::Entity&, const  ECS::Entity&)> hit, const json&) {return Enemy::createEnemy(location, std::move(overlap), std::move(hit));}},
        {"button_start", [](vec2 location, vec2 size, float rotation,
                      std::function<void(ECS::Entity&, const  ECS::Entity&)>,
                      std::function<void(ECS::Entity&, const  ECS::Entity&)>, const json&)
                      {
            return Button::createButton( ButtonType::START, location, [](ECS::Entity& self, const ECS::Entity& other){
                      if (other.has<Soldier>()){
                          WorldSystem::reload_level = true;
                          WorldSystem::level_name = "level_3";
                      }
            });
        }},
        {"button_setting", [](vec2 location, vec2 size, float rotation,
                            std::function<void(ECS::Entity&, const  ECS::Entity&)>,
                            std::function<void(ECS::Entity&, const  ECS::Entity&)>, const json&){
            return Button::createButton( ButtonType::LEVEL_SELECT, location, [](ECS::Entity& self, const ECS::Entity& other){
                if (other.has<Soldier>()){
                    WorldSystem::reload_level = true;
                    WorldSystem::level_name = "level_2";
                }
        });
        }},
        {"background", [](vec2 location, vec2 size, float rotation,
                std::function<void(ECS::Entity&, const  ECS::Entity&)>,
                        std::function<void(ECS::Entity&, const  ECS::Entity&)>, json additional){
            if(additional.contains("name")) {
                Background::createBackground(vec2{500, 500}, additional["name"]);
            } else {
                Background::createBackground(vec2{500, 500}, "background");
            }
        }},
        {"title", [](vec2 location, vec2 , float ,
                          std::function<void(ECS::Entity&, const  ECS::Entity&)>,
                          std::function<void(ECS::Entity&, const  ECS::Entity&)>, const json&){
                Start::createStart(location);
            }}
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
                auto additional = b.contains("additionalProperties") ? b["additionalProperties"] : json{};
                level_object.second(position, size, rotation, overlap, hit, additional);
            }
        }
    }
}

void LevelLoader::set_level(std::string level){
    at_level = level;
}
