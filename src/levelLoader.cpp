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
#include "GameInstance.hpp"
#include <fstream>
#include <string.h>
#include <cassert>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <utility>

using json = nlohmann::json;

std::vector<std::string> LevelLoader::existing_level = {
	"level_select",
	"loadout",
	"menu",
	"win",
	"lose",
	"level_1",
	"intro",
	"level_2",
	"level_3",
	"level_4",
	"level_5",
	"level_6",
	"level_7",
};

/*
std::unordered_map<std::string, int> LevelLoader::level_progression = {
	{"intro", 0},
	{"level_2", -1},
	{"level_3", -1},
	{"level_4", -1},
	{"level_5", -1},
	{"level_6", -1},
	{"level_7", -1},
	{"level_8", -1},
	{"level_9", -1},
	{"level_10", -1}
};
*/


std::vector<std::string> LevelLoader::level_order = {
	"intro",
	"level_2",
	"level_3",
	"level_4",
	"level_5",
	"level_6",
	"level_7",
	"level_8",
	"level_9",
	"level_10",
};

void enemy_bullet_hit_death(ECS::Entity self, const ECS::Entity e, CollisionResult) {
	if (e.has<Bullet>() && e.get<Bullet>().teamID != self.get<Enemy>().teamID && !self.has<DeathTimer>()) {
		self.emplace<DeathTimer>();
	}
};

auto select_algo_of_type(AIAlgorithm algo) {
	return [=](ECS::Entity self, const ECS::Entity other, CollisionResult) {
        if (other.has<Soldier>() && WorldSystem::selecting) {

            for (auto &button: ECS::registry<Button>.components) {
                if (button.buttonClass == ButtonClass::ALGORITHM_SELECTION) {
                    button.selected = false;
                }
            }
            self.get<Button>().selected = true;
            GameInstance::algorithm = algo;
            if(!self.has<PressTimer>()){
                self.emplace<PressTimer>();
            }
            //self.emplace<PressTimer>();
        }
    };
}

auto select_weapon_of_type(WeaponType type) {
	return [=](ECS::Entity self, const ECS::Entity other, CollisionResult) {
	    if (other.has<Soldier>() && WorldSystem::selecting) {
            for(auto& button: ECS::registry<Button>.components){
                if(button.buttonClass == ButtonClass::WEAPON_SELECTION){
                    button.selected = false;
                }
            }
			self.get<Button>().selected = true;
			GameInstance::selectedWeapon = type;
            if(!self.has<PressTimer>()){
                self.emplace<PressTimer>();
            }
		}
	};
}


auto select_button_overlap(const std::string& level){
    return [=](ECS::Entity self, const ECS::Entity other, CollisionResult) {
        if (other.has<Soldier>() && WorldSystem::selecting && std::count(LevelLoader::existing_level.begin(), LevelLoader::existing_level.end(), level)) {
            WorldSystem::reload_level = true;
            WorldSystem::reload_level_name = level;
        }
    };
};

auto select_level_button_overlap(const std::string& level){
    return [=](ECS::Entity self, const ECS::Entity other, CollisionResult) {
        if (other.has<Soldier>() && WorldSystem::selecting && std::count(LevelLoader::existing_level.begin(), LevelLoader::existing_level.end(), level)) {
            WorldSystem::selected_level = level;
            WorldSystem::reload_level = true;
            WorldSystem::reload_level_name = "loadout";
        }
    };
};




std::unordered_map<std::string, COLLISION_HANDLER> LevelLoader::physics_callbacks = {
		{"enemy_bullet_hit_death", Enemy::enemy_bullet_hit_death},
        {"soldier_bullet_hit_death", Soldier::soldier_bullet_hit_death},
        {"wall_scater", Wall::wall_overlap},
};

std::unordered_map<std::string, COLLISION_HANDLER> LevelLoader::default_hit_callback = {
        {"movable_wall", MoveableWall::wall_hit},
};

COLLISION_HANDLER get_default_hit_callback(const std::string& key){
    if(LevelLoader::default_hit_callback.find(key)!= LevelLoader::default_hit_callback.end()){
        return LevelLoader::default_hit_callback[key];
    }
    return PhysicsObject::handle_collision;
}

std::unordered_map<std::string, std::function<void(vec2, vec2, float,
	COLLISION_HANDLER, COLLISION_HANDLER, json)>> LevelLoader::level_objects = {
	{"blocks", [](vec2 location, vec2 size, float rotation,
					   COLLISION_HANDLER overlap, COLLISION_HANDLER, const json&) {
		Wall::createWall(location, size, rotation, physics_callbacks["wall_scater"], Wall::wall_hit);
	}
	},
	{"borders", [](vec2 location, vec2 size, float rotation,
				   COLLISION_HANDLER overlap, COLLISION_HANDLER hit, const json&) {
		Wall::createWall(location, size, rotation, overlap, PhysicsObject::handle_collision);
	}
	},
	{"movable_wall", [](vec2 location, vec2 size, float rotation,
						COLLISION_HANDLER overlap, COLLISION_HANDLER hit, const json&) {
		MoveableWall::createMoveableWall(location, size, rotation, physics_callbacks["wall_scater"], Wall::wall_hit);
	}
	},
	{"player", [](vec2 location, vec2 size, float rotation,
			COLLISION_HANDLER overlap,
			COLLISION_HANDLER hit, const json& additional) {
		float light_intensity = additional.contains("light_intensity") ? additional["light_intensity"] : DEFAULT_LIGHT_INTENSITY;
		return Soldier::createSoldier(location, overlap, hit, light_intensity);
	}},
	{"enemy", [](vec2 location, vec2 size, float rotation,
				 COLLISION_HANDLER overlap,
				 COLLISION_HANDLER hit, const json& additional) {
	    float health = ENEMY_DEFAULT_HEALTH;
	    int team_id = ENEMY_DEFAULT_TEAM_ID;
	    if(additional.contains("health")){
	        health = additional["health"];
	    }
        if(additional.contains("team_id")){
            team_id = additional["team_id"];
        }
	    return Enemy::createEnemy(location, overlap, hit, team_id, health);
	}},
	{"button_start", [](vec2 location, vec2 size, float rotation,
				  COLLISION_HANDLER,
				  COLLISION_HANDLER, const json&)
				  {
		return Button::createButton(ButtonIcon::START, location, select_button_overlap("level_select"));
	}},
	{"button_setting", [](vec2 location, vec2 size, float rotation,
						COLLISION_HANDLER,
						COLLISION_HANDLER, const json&) {
		return Button::createButton(ButtonIcon::LEVEL_SELECT, location, select_button_overlap("loadout"));}
	},
	{"button_select_rocket", [](vec2 location, vec2 size, float rotation,
                              COLLISION_HANDLER,
                              COLLISION_HANDLER, const json&){
            return Button::createButton( ButtonIcon::SELECT_ROCKET, location, select_weapon_of_type(W_ROCKET));}},
        {"button_select_ammo", [](vec2 location, vec2 size, float rotation,
                                    COLLISION_HANDLER,
                                    COLLISION_HANDLER, const json&){
            return Button::createButton( ButtonIcon::SELECT_AMMO, location, select_weapon_of_type(W_AMMO));}},
        {"button_select_laser", [](vec2 location, vec2 size, float rotation,
                                    COLLISION_HANDLER,
                                    COLLISION_HANDLER, const json&){
            return Button::createButton( ButtonIcon::SELECT_LASER, location, select_weapon_of_type(W_LASER));}},
        {"button_select_bullet", [](vec2 location, vec2 size, float rotation,
                                    COLLISION_HANDLER,
                                    COLLISION_HANDLER, const json&){
            return Button::createButton( ButtonIcon::SELECT_BULLET, location, select_weapon_of_type(W_BULLET));}},
        {"button_select_direct", [](vec2 location, vec2 size, float rotation,
                                COLLISION_HANDLER,
                                COLLISION_HANDLER, const json&){
        return Button::createButton( ButtonIcon::SELECT_DIRECT, location, select_algo_of_type(DIRECT));}},
        {"button_select_a_star", [](vec2 location, vec2 size, float rotation,
                                COLLISION_HANDLER,
                                COLLISION_HANDLER, const json&){
        return Button::createButton( ButtonIcon::SELECT_A_STAR, location, select_algo_of_type(A_STAR));}},
	{"button_enter_level", [](vec2 location, vec2 size, float rotation,
						COLLISION_HANDLER,
						COLLISION_HANDLER, const json&)
					 {
						 return Button::createButton(ButtonIcon::NEXT, location, select_button_overlap( WorldSystem::selected_level));
					 }},
	{"background", [](vec2 location, vec2 size, float rotation,
			COLLISION_HANDLER,
					COLLISION_HANDLER, json additional) {
	    std::string name = "background";  
	    float depth = 0.f;
      float scale = 1.5f;
		if (additional.contains("name")) {
			name = additional["name"];
		}
    if (additional.contains("depth")) {
		    depth = additional["depth"];
		}
    if (additional.contains("scale")) {
        scale = additional["scale"];
    }
		Background::createBackground(vec2{500, 500}, name, depth, scale);
	}}, 	{"quality_slider", [](vec2 location, vec2 size, float rotation,
			COLLISION_HANDLER overlap,
					COLLISION_HANDLER, json additional) {
		float val_min, val_max,
		x_min = location.x, x_max = location.x, 
		y_min = location.y, y_max = location.y;
		if (additional.contains("val_min")) {
			val_min = additional["val_min"];
		}
	if (additional.contains("val_max")) {
		val_max = additional["val_max"];
		}
	if (additional.contains("x_min")) {
		x_min = additional["x_min"];
	}
	if (additional.contains("x_max")) {
		x_max = additional["x_max"];
	}
	if (additional.contains("y_min")) {
		y_min = additional["y_min"];
	}
	if (additional.contains("y_max")) {
		y_max = additional["y_max"];
	} 
	 
	auto e = MoveableWall::createMoveableWall(location, size, 0, overlap, [=](ECS::Entity self, ECS::Entity e, CollisionResult collision) {
		if (self.has<Motion>() && e.has<Motion>()) {
			auto& motion = self.get<Motion>();
			auto& other_motion = e.get<Motion>();
			if (motion.position.x > x_max) {
				float delta = motion.position.x - x_max;
				motion.position.x -= delta;
				other_motion.position.x -= delta;
			} else 	if (motion.position.x < x_min) {
				float delta = motion.position.x - x_min;
				motion.position.x -= delta;
				other_motion.position.x -= delta;
			}

			if (motion.position.y > y_max) {
				float delta = motion.position.y - y_max;
				motion.position.y -= delta;
				other_motion.position.y -= delta;
			}
			else if (motion.position.y < y_min) {
				float delta = motion.position.y - y_min;
				motion.position.y -= delta;
				other_motion.position.y -= delta;
			}
			float quality = floor((motion.position.x - x_min) / (x_max - x_min) * (val_max - val_min) + val_min);
			if (GameInstance::light_quality != quality){
				GameInstance::light_quality = quality;
				RenderSystem::renderSystem->recreate_light_texture(2 * GameInstance::light_quality);
				printf("light_quality: %f\n", GameInstance::light_quality);
			}
		}
		});
	e.get<PhysicsObject>().mass = 100.f;
	e.get<Motion>().position.x = (GameInstance::light_quality - val_min) / (val_max - val_min) * (x_max-x_min) + x_min;
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
						return Button::createButton(ButtonIcon::RETURN, location, select_button_overlap("menu"));
					} },
        { "return_to_loadout", [](vec2 location, vec2 size, float rotation,
                    COLLISION_HANDLER,
                    COLLISION_HANDLER, const json&)
                {
                    return Button::createButton(ButtonIcon::RESTART, location, select_button_overlap("loadout"));
                } },
		{ "return_to_level_select", [](vec2 location, vec2 size, float rotation,
							COLLISION_HANDLER,
							COLLISION_HANDLER, const json&)
						{
							return Button::createButton(ButtonIcon::RETURN, location, select_button_overlap("level_select"));
						} },
        { "next_to_level_select", [](vec2 location, vec2 size, float rotation,
                        COLLISION_HANDLER,
                        COLLISION_HANDLER, const json&)
                    {
                        return Button::createButton(ButtonIcon::NEXT, location, select_button_overlap("level_select"));
                    } },
        { "next_to_level", [](vec2 location, vec2 size, float rotation,
                    COLLISION_HANDLER,
                    COLLISION_HANDLER, const json&)
                {
                    return Button::createButton(ButtonIcon::NEXT, location, select_button_overlap("level_1"));
                } },
		{"select_level_1", [](vec2 location, vec2 size, float rotation,
					COLLISION_HANDLER,
					COLLISION_HANDLER, const json&)
				 {
					 return level_progression["intro"] >= 0 ? Button::createButton(ButtonIcon::LEVEL1, location, select_level_button_overlap("intro")) : Button::createButton(ButtonIcon::DEFAULT_BUTTON, location, select_button_overlap(""));
				 }},
		{ "select_level_2", [](vec2 location, vec2 size, float rotation,
						COLLISION_HANDLER,
						COLLISION_HANDLER, const json&)
					{
						return level_progression["level_2"] >= 0 ? Button::createButton(ButtonIcon::LEVEL2, location, select_level_button_overlap("level_2")) : Button::createButton(ButtonIcon::DEFAULT_BUTTON, location, select_button_overlap(""));
					} },
		{ "select_level_3", [](vec2 location, vec2 size, float rotation,
						COLLISION_HANDLER,
						COLLISION_HANDLER, const json&)
					{
						return level_progression["level_3"] >= 0 ? Button::createButton(ButtonIcon::LEVEL3, location, select_level_button_overlap("level_3")) : Button::createButton(ButtonIcon::DEFAULT_BUTTON, location, select_button_overlap(""));
					} },
		{ "select_level_4", [](vec2 location, vec2 size, float rotation,
					COLLISION_HANDLER,
					COLLISION_HANDLER, const json&)
					{
						return level_progression["level_4"] >= 0 ? Button::createButton(ButtonIcon::LEVEL4, location, select_level_button_overlap("level_4")) : Button::createButton(ButtonIcon::DEFAULT_BUTTON, location, select_button_overlap(""));
					} },
		{ "select_level_5", [](vec2 location, vec2 size, float rotation,
			COLLISION_HANDLER,
			COLLISION_HANDLER, const json&)
		{
			return level_progression["level_5"] >= 0 ? Button::createButton(ButtonIcon::LEVEL5, location, select_level_button_overlap("level_5")) : Button::createButton(ButtonIcon::DEFAULT_BUTTON, location, select_button_overlap(""));
		} },
		{ "select_level_6", [](vec2 location, vec2 size, float rotation,
						COLLISION_HANDLER,
						COLLISION_HANDLER, const json&)
					{
						return level_progression["level_6"] >= 0 ? Button::createButton(ButtonIcon::LEVEL6, location, select_level_button_overlap("level_6")) : Button::createButton(ButtonIcon::DEFAULT_BUTTON, location, select_button_overlap(""));
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
				auto hit = b.contains("hit") ? physics_callbacks[b["hit"]] : get_default_hit_callback(level_object.first);
				auto additional = b.contains("additionalProperties") ? b["additionalProperties"] : json{};
				level_object.second(position, size, rotation, overlap, hit, additional);
			}
		}
	}
}

int LevelLoader::get_level_state(std::string level)
{
	return level_progression[level];
}

bool LevelLoader::is_level_unlocked(std::string level)
{
	return level_progression[level] > -1;
}

bool LevelLoader::is_level_cleared(std::string level)
{
	return level_progression[level] > 0;
}

std::string LevelLoader::get_next_level_name(std::string level)
{
	auto it = std::find(level_order.begin(), level_order.end(), level);
	auto next = std::next(it, 1);
	return *next;
}

void LevelLoader::update_level_state(std::string level, int state)
{
	if (state > get_level_state(level)) {
		level_progression[level] = state;
		if (!is_level_unlocked(get_next_level_name(level))) {
			level_progression[get_next_level_name(level)] = 0;
		}
	}
}

void LevelLoader::set_level(std::string level) {
	at_level = level;
}
