//
//  levelLoader.cpp
//  game_template
//
//  Created by Haofeng Winter Feng on 2021-02-28.
//

#include "levelLoader.hpp"
#include <nlohmann/json.hpp>
#include "tiny_ecs.hpp"
#include "render_components.hpp"
#include "Wall.hpp"
#include "MoveableWall.hpp"
#include <fstream>
#include <string>

using json = nlohmann::json;

// Named collision callbacks levels can reference. Register game-specific ones here.
std::unordered_map<std::string, COLLISION_HANDLER> LevelLoader::physics_callbacks = {};

std::unordered_map<std::string, COLLISION_HANDLER> LevelLoader::default_hit_callback = {
    {"movable_wall", MoveableWall::wall_hit},
};

static COLLISION_HANDLER get_default_hit_callback(const std::string& key) {
    if (LevelLoader::default_hit_callback.find(key) != LevelLoader::default_hit_callback.end()) {
        return LevelLoader::default_hit_callback[key];
    }
    return PhysicsObject::handle_collision;
}

// The registry of spawnable object types. Add entries to teach the loader about
// your game's entities; each receives the parsed transform plus collision handlers.
std::unordered_map<std::string, std::function<void(vec2, vec2, float,
    COLLISION_HANDLER, COLLISION_HANDLER, json)>> LevelLoader::level_objects = {
    {"blocks", [](vec2 location, vec2 size, float rotation,
                  COLLISION_HANDLER overlap, COLLISION_HANDLER hit, const json&) {
        Wall::createWall(location, size, rotation, overlap, hit);
    }},
    {"borders", [](vec2 location, vec2 size, float rotation,
                   COLLISION_HANDLER overlap, COLLISION_HANDLER hit, const json&) {
        Wall::createWall(location, size, rotation, overlap, hit);
    }},
    {"movable_wall", [](vec2 location, vec2 size, float rotation,
                        COLLISION_HANDLER overlap, COLLISION_HANDLER hit, const json&) {
        MoveableWall::createMoveableWall(location, size, rotation, overlap, hit);
    }},
};

// Read the JSON content from a level file.
static json readLevelJsonFile(std::string level) {
    auto file_name = level + ".json";
    auto obj_path = level_path(file_name);
    auto level_file = std::ifstream{ obj_path };
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
    if (at_level.empty()) {
        return;
    }
    json current = readLevelJsonFile(at_level);
    for (auto& level_object : level_objects) {
        if (!current.contains(level_object.first)) {
            continue;
        }
        for (json b : current[level_object.first]) {
            vec2 position = b.contains("position") ? getVec2FromJson(b["position"]) : vec2{};
            vec2 size = b.contains("size") ? getVec2FromJson(b["size"]) : vec2{};
            float rotation = b.contains("rotation") ? static_cast<float>(b["rotation"]) : 0.f;

            COLLISION_HANDLER overlap = [](ECS::Entity, const ECS::Entity, CollisionResult) {};
            if (b.contains("overlap")) {
                auto it = physics_callbacks.find(b["overlap"]);
                if (it != physics_callbacks.end()) {
                    overlap = it->second;
                }
            }
            COLLISION_HANDLER hit = get_default_hit_callback(level_object.first);
            if (b.contains("hit")) {
                auto it = physics_callbacks.find(b["hit"]);
                if (it != physics_callbacks.end()) {
                    hit = it->second;
                }
            }
            auto additional = b.contains("additionalProperties") ? b["additionalProperties"] : json{};

            level_object.second(position, size, rotation, overlap, hit, additional);
        }
    }
}

void LevelLoader::set_level(std::string level) {
    at_level = level;
}
