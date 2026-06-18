//
//  levelLoader.hpp
//  game_template
//
//  Created by Haofeng Winter Feng on 2021-02-28.
//

#pragma once
#include "common.hpp"
#include "tiny_ecs.hpp"
#include "physics.hpp"
#include "PhysicsObject.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <functional>

using json = nlohmann::json;

// Data-driven scene loader. A level is a JSON file under data/levels/ whose
// top-level keys name an entity "type"; each type maps to a spawner registered
// in `level_objects`. Register your own spawners there to extend the loader.
class LevelLoader {
    public:
        void set_level(std::string level);
        void load_level();

    std::string at_level = "";

    // Named collision callbacks a level entry may reference via its "overlap"/"hit" fields.
    static std::unordered_map<std::string, COLLISION_HANDLER> physics_callbacks;
    // Default "hit" callback per object type when a level entry omits one.
    static std::unordered_map<std::string, COLLISION_HANDLER> default_hit_callback;
    // type name -> spawner(location, size, rotation, overlap, hit, additionalProperties)
    static std::unordered_map<std::string, std::function<void(vec2 location, vec2 size, float rotation,
                                                              COLLISION_HANDLER, COLLISION_HANDLER, json)>> level_objects;
};
