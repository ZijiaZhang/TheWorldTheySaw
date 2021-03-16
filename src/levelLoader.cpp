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
#include "loading.hpp"
#include "Weapon.hpp"
#include <fstream>
#include <string.h>
#include <cassert>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <utility>

using json = nlohmann::json;



void enemy_bullet_hit_death(ECS::Entity self, const ECS::Entity e, CollisionResult) {
	if (e.has<Bullet>() && e.get<Bullet>().teamID != self.get<Enemy>().teamID && !self.has<DeathTimer>()) {
		self.emplace<DeathTimer>();
	}
};

void soldier_bullet_hit_death(ECS::Entity self, const ECS::Entity e, CollisionResult) {
	if (e.has<Bullet>() && e.get<Bullet>().teamID != self.get<Soldier>().teamID && !self.has<DeathTimer>()) {
		self.emplace<DeathTimer>();
	}
};

auto select_algo_of_type(AIAlgorithm algo) {
	return [=](ECS::Entity self, const ECS::Entity other, CollisionResult) {
		if (other.has<Soldier>() && WorldSystem::selecting) {
			auto& soldier = other.get<Soldier>();
			soldier.ai_algorithm = algo;
		}
	};
}

auto select_weapon_of_type(WeaponType type) {
	return [=](ECS::Entity self, const ECS::Entity other, CollisionResult) {
		if (other.has<Soldier>() && WorldSystem::selecting) {
			auto& soldier = other.get<Soldier>();
			if (soldier.weapon.has<Weapon>()) {
				auto& weapon = soldier.weapon.get<Weapon>();
				weapon.type = type;
			}
		}
	};
}





auto select_button_overlap(const std::string& level){
    return [=](ECS::Entity self, const ECS::Entity other, CollisionResult) {
        if (other.has<Soldier>() && WorldSystem::selecting) {
            WorldSystem::reload_level = true;
            WorldSystem::level_name = level;
        }
    };
};

auto select_level_button_overlap(const std::string& level){
    return [=](ECS::Entity self, const ECS::Entity other, CollisionResult) {
        if (other.has<Soldier>() && WorldSystem::selecting) {
            WorldSystem::selected_level = level;
            WorldSystem::reload_level = true;
            WorldSystem::level_name = "loadout";
        }
    };
};




std::unordered_map<std::string, COLLISION_HANDLER> LevelLoader::physics_callbacks = {
		{"enemy_bullet_hit_death", enemy_bullet_hit_death},
        {"soldier_bullet_hit_death", soldier_bullet_hit_death},
        {"wall_scater", Wall::wall_hit},
};

std::unordered_map<std::string, std::function<void(vec2, vec2, float,
	COLLISION_HANDLER, COLLISION_HANDLER, json)>> LevelLoader::level_objects = {
	{"blocks", [](vec2 location, vec2 size, float rotation,
					   COLLISION_HANDLER overlap, COLLISION_HANDLER, const json&) {
		Wall::createWall(location, size, rotation, overlap, physics_callbacks["wall_scater"]);
	}
	},
	{"borders", [](vec2 location, vec2 size, float rotation,
				   COLLISION_HANDLER overlap, COLLISION_HANDLER hit, const json&) {
		Wall::createWall(location, size, rotation, overlap, PhysicsObject::handle_collision);
	}
	},
	{"movable_wall", [](vec2 location, vec2 size, float rotation,
						COLLISION_HANDLER overlap, COLLISION_HANDLER hit, const json&) {
		MoveableWall::createMoveableWall(location, size, rotation, overlap, hit);
	}
	},
	{"player", [](vec2 location, vec2 size, float rotation,
			COLLISION_HANDLER overlap,
			COLLISION_HANDLER hit, const json&) {
		return Soldier::createSoldier(location, overlap, hit);
	}},
	{"enemy", [](vec2 location, vec2 size, float rotation,
				 COLLISION_HANDLER overlap,
				 COLLISION_HANDLER hit, const json&) {return Enemy::createEnemy(location, overlap, hit); }},
	{"button_start", [](vec2 location, vec2 size, float rotation,
				  COLLISION_HANDLER,
				  COLLISION_HANDLER, const json&)
				  {
		return Button::createButton(ButtonType::START, location, select_button_overlap("level_select"));
	}},
	{"button_setting", [](vec2 location, vec2 size, float rotation,
						COLLISION_HANDLER,
						COLLISION_HANDLER, const json&) {
		return Button::createButton(ButtonType::LEVEL_SELECT, location, select_button_overlap("loadout"));}
	},
	{"button_select_rocket", [](vec2 location, vec2 size, float rotation,
                              COLLISION_HANDLER,
                              COLLISION_HANDLER, const json&){
            return Button::createButton( ButtonType::SELECT_ROCKET, location, select_weapon_of_type(W_ROCKET));}},
        {"button_select_ammo", [](vec2 location, vec2 size, float rotation,
                                    COLLISION_HANDLER,
                                    COLLISION_HANDLER, const json&){
            return Button::createButton( ButtonType::SELECT_AMMO, location, select_weapon_of_type(W_AMMO));}},
        {"button_select_laser", [](vec2 location, vec2 size, float rotation,
                                    COLLISION_HANDLER,
                                    COLLISION_HANDLER, const json&){
            return Button::createButton( ButtonType::SELECT_LASER, location, select_weapon_of_type(W_LASER));}},
        {"button_select_bullet", [](vec2 location, vec2 size, float rotation,
                                    COLLISION_HANDLER,
                                    COLLISION_HANDLER, const json&){
            return Button::createButton( ButtonType::SELECT_BULLET, location, select_weapon_of_type(W_BULLET));}},
        {"button_select_direct", [](vec2 location, vec2 size, float rotation,
                                COLLISION_HANDLER,
                                COLLISION_HANDLER, const json&){
        return Button::createButton( ButtonType::SELECT_DIRECT, location, select_algo_of_type(DIRECT));}},
        {"button_select_a_star", [](vec2 location, vec2 size, float rotation,
                                COLLISION_HANDLER,
                                COLLISION_HANDLER, const json&){
        return Button::createButton( ButtonType::SELECT_A_STAR, location, select_algo_of_type(A_STAR));}},
	{"button_enter_level", [](vec2 location, vec2 size, float rotation,
						COLLISION_HANDLER,
						COLLISION_HANDLER, const json&)
					 {
						 return Button::createButton(ButtonType::DEFAULT_BUTTON, location, select_button_overlap( WorldSystem::selected_level));
					 }},
	{"background", [](vec2 location, vec2 size, float rotation,
			COLLISION_HANDLER,
					COLLISION_HANDLER, json additional) {
		if (additional.contains("name")) {
			Background::createBackground(vec2{500, 500}, additional["name"]);
		}
		else {
		Background::createBackground(vec2{500, 500}, "background");
		}
	}},
	{"title", [](vec2 location, vec2 , float ,
					  COLLISION_HANDLER,
					  COLLISION_HANDLER, const json&) {
			Start::createStart(location);
		}},
		{"weapon", [](vec2 location, vec2 , float ,
					COLLISION_HANDLER,
					COLLISION_HANDLER, const json&) {
			Loading::createLoading(location);
		}},
		{ "return_to_menu", [](vec2 location, vec2 size, float rotation,
						COLLISION_HANDLER,
						COLLISION_HANDLER, const json&)
					{
						return Button::createButton(ButtonType::RETURN, location, select_button_overlap("menu"));
					} },
        { "return_to_loadout", [](vec2 location, vec2 size, float rotation,
                    COLLISION_HANDLER,
                    COLLISION_HANDLER, const json&)
                {
                    return Button::createButton(ButtonType::DEFAULT_BUTTON, location, select_button_overlap("loadout"));
                } },
		{ "return_to_level_select", [](vec2 location, vec2 size, float rotation,
							COLLISION_HANDLER,
							COLLISION_HANDLER, const json&)
						{
							return Button::createButton(ButtonType::RETURN, location, select_button_overlap("level_select"));
						} },
		{"select_level_1", [](vec2 location, vec2 size, float rotation,
					COLLISION_HANDLER,
					COLLISION_HANDLER, const json&)
				 {
					 return Button::createButton(ButtonType::LEVEL1, location, select_level_button_overlap("level_4"));
				 }},
		{ "select_level_2", [](vec2 location, vec2 size, float rotation,
						COLLISION_HANDLER,
						COLLISION_HANDLER, const json&)
					{
						return Button::createButton(ButtonType::LEVEL2, location, select_level_button_overlap("level_3"));
					} },
		{ "select_level_3", [](vec2 location, vec2 size, float rotation,
						COLLISION_HANDLER,
						COLLISION_HANDLER, const json&)
					{
						return Button::createButton(ButtonType::DEFAULT_BUTTON, location, select_level_button_overlap("level_5"));
					} }
};

/**
 read json content from the level files
 */
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
	json current = readLevelJsonFile(at_level);
	for (auto& level_object : level_objects) {
		if (current.contains(level_object.first)) {
			for (json b : current[level_object.first]) {
				vec2 position = b.contains("position") ? getVec2FromJson(b["position"]) : vec2{};
				vec2 size = b.contains("size") ? getVec2FromJson(b["size"]) : vec2{};
				float rotation = b.contains("rotation") ? static_cast<float>(b["rotation"]) : 0.f;
				auto overlap = b.contains("overlap") ? physics_callbacks[b["overlap"]] : [](ECS::Entity, const ECS::Entity e, CollisionResult) {};
				auto hit = b.contains("hit") ? physics_callbacks[b["hit"]] : PhysicsObject::handle_collision;
				auto additional = b.contains("additionalProperties") ? b["additionalProperties"] : json{};
				level_object.second(position, size, rotation, overlap, hit, additional);
			}
		}
	}
}

void LevelLoader::set_level(std::string level) {
	at_level = level;
}
