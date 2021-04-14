//
//  levelLoader.cpp
//  salmon
//
//  Created by Haofeng Winter Feng on 2021-02-28.
//

#include "levelLoader.hpp"
#include <nlohmann/json.hpp>
#include "background.hpp"
#include "mainMenu.hpp"
#include "start.hpp"
#include "tiny_ecs.hpp"
#include "Bullet.hpp"
#include "render_components.hpp"
#include "button.hpp"
#include "world.hpp"
#include "loading.hpp"
#include "Weapon.hpp"
#include "GameInstance.hpp"
#include "avatar.hpp"
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
    "settings",
	"level_2",
	"level_3",
	"level_4",
	"level_5",
	"level_6",
	"level_7",
	"level_8",
	"level_9",
	"level_10",
	"level_11",
	"level_12",
	TUTORIAL_NAME
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
	"level_1",
	"level_2",
	"level_3",
	"level_4",
	"level_5",
	"level_6",
	"level_7",
	"level_8",
	"level_9",
	"level_10",
	"level_11",
	"level_12"
};

std::unordered_map<std::string, LevelEntityState> LevelLoader::saved_level_states = {};
std::unordered_map<std::string, bool> LevelLoader::saved_flag = {
	{"level_1", false},
	{"level_2", false},
	{"level_2", false},
	{"level_3", false},
	{"level_4", false},
	{"level_5", false},
	{"level_6", false},
	{"level_7", false},
	{"level_8", false},
	{"level_9", false},
	{"level_10", false},
	{"level_11", false},
	{"level_12", false}
};

std::string get_save_directory() {
	std::string path = "../save.txt";
	return path;
}

static void save_level_data()
{
	std::ofstream data(get_save_directory(), std::ofstream::trunc);
	for (int i = 0; i < LevelLoader::level_order.size(); i++) {
		data << level_progression[LevelLoader::level_order[i]] << "\n";
	}
	data.close();
	return;
}

static void load_level_data()
{
	std::ifstream data(get_save_directory());

	if (!data.is_open()) {
		for (int i = 0; i < LevelLoader::level_order.size(); i++) {
			level_progression[LevelLoader::level_order[i]] = 0;
		}
		level_progression[LevelLoader::level_order[0]] = 1;
	}
	else {
		std::string line;
		for (int i = 0; std::getline(data, line) && i < level_progression.size(); i++) {
			level_progression[LevelLoader::level_order[i]] = line[0] - '0';
		}
	}
	return;
}

void enemy_bullet_hit_death(ECS::Entity self, const ECS::Entity e, CollisionResult) {
	if (e.has<Bullet>() && e.get<Bullet>().teamID != self.get<Enemy>().teamID && !self.has<DeathTimer>()) {
		self.emplace<DeathTimer>();
	}
};

auto select_algo_of_type(AIAlgorithm algo) {
	return [=](ECS::Entity self, const ECS::Entity other, CollisionResult) {
		if (other.has<Soldier>() && WorldSystem::selecting) {
			GameInstance::algorithm = algo;
			if (!self.has<PressTimer>()) {
				self.emplace<PressTimer>();
			}
			//self.emplace<PressTimer>();
		}
	};
}

auto select_ability_of_type(MagicWeapon magic) {
	return [=](ECS::Entity self, const ECS::Entity other, CollisionResult) {
		if (other.has<Soldier>() && WorldSystem::selecting) {
			GameInstance::selectedMagic = magic;
		}
	};
}

auto select_weapon_of_type(WeaponType type) {
	return [=](ECS::Entity self, const ECS::Entity other, CollisionResult) {
		if (other.has<Soldier>() && WorldSystem::selecting) {
			GameInstance::selectedWeapon = type;
			if (!self.has<PressTimer>()) {
				self.emplace<PressTimer>();
			}
		}
	};
}


auto select_button_overlap(const std::string& level) {
	return [=](ECS::Entity self, const ECS::Entity other, CollisionResult) {
		if (other.has<Soldier>() && WorldSystem::selecting && std::count(LevelLoader::existing_level.begin(), LevelLoader::existing_level.end(), level)) {
			WorldSystem::reload_level = true;
			WorldSystem::reload_level_name = level;
		}
	};
};

auto select_level_button_overlap(const std::string& level) {
	return [=](ECS::Entity self, const ECS::Entity other, CollisionResult) {
		if (other.has<Soldier>() && WorldSystem::selecting && std::count(LevelLoader::existing_level.begin(), LevelLoader::existing_level.end(), level)) {
			WorldSystem::selected_level = level;
			WorldSystem::reload_level = true;

			std::string l = level;
			if (level == "intro") {
				l = "level_1";
			}
			if (LevelLoader::saved_flag[l]) {
				WorldSystem::reload_level_name = l;
			}
			else {
				WorldSystem::reload_level_name = "loadout";
			}
		}
	};
};

auto select_save_data() {
	return [=](ECS::Entity self, const ECS::Entity other, CollisionResult) {
		if (other.has<Soldier>() && WorldSystem::selecting) {
			save_level_data();
		}
	};
}

auto select_continue() {
	return [=](ECS::Entity self, const ECS::Entity other, CollisionResult) {
		if (other.has<Soldier>() && WorldSystem::selecting) {
			load_level_data();
			WorldSystem::reload_level = true;
			WorldSystem::reload_level_name = "level_select";
		}
	};
}

std::unordered_map<std::string, COLLISION_HANDLER> LevelLoader::physics_callbacks = {
		{"enemy_bullet_hit_death", Enemy::enemy_bullet_hit_death},
        {"soldier_bullet_hit_death", Soldier::soldier_bullet_hit_death},
        {"wall_scater", Wall::wall_overlap},

};

std::unordered_map<std::string, COLLISION_HANDLER> LevelLoader::default_hit_callback = {
		{"movable_wall", MoveableWall::wall_hit},
};

COLLISION_HANDLER get_default_hit_callback(const std::string& key) {
	if (LevelLoader::default_hit_callback.find(key) != LevelLoader::default_hit_callback.end()) {
		return LevelLoader::default_hit_callback[key];
	}
	return PhysicsObject::handle_collision;
}

COLLISION_HANDLER get_slider_callback(float val_min, float val_max, float x_min, float x_max, float y_min, float y_max) {
	return [val_min, val_max, x_min, x_max, y_min, y_max](ECS::Entity self, ECS::Entity e, CollisionResult) mutable {
		if (self.has<Motion>() && e.has<Motion>()) {
			auto& motion = self.get<Motion>();
			auto& other_motion = e.get<Motion>();
			if (motion.position.x > x_max) {
				float delta = motion.position.x - x_max;
				motion.position.x -= delta;
				other_motion.position.x -= delta;
			}
			else 	if (motion.position.x < x_min) {
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
			if (GameInstance::light_quality != quality) {
				GameInstance::light_quality = quality;
				RenderSystem::renderSystem->recreate_light_texture(2 * GameInstance::light_quality);
				printf("light_quality: %f\n", GameInstance::light_quality);
			}
		}
	};
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
        float light_intensity = DEFAULT_LIGHT_INTENSITY;
        if(additional.contains("light_intensity")){
            light_intensity = additional["light_intensity"];
        }
		return Soldier::createSoldier(location, overlap, hit, light_intensity);
	}},
    {"avatar", [](vec2 location, vec2 size, float rotation,
            COLLISION_HANDLER overlap,
            COLLISION_HANDLER hit, const json& additional) {
            return;
    }},
	{"enemy", [](vec2 location, vec2 size, float rotation,
				 COLLISION_HANDLER overlap,
				 COLLISION_HANDLER hit, const json& additional) {
		EnemyType type = EnemyType::STANDARD;
		int team_id = ENEMY_DEFAULT_TEAM_ID;
		if (additional.contains("enemy_type")) {
			type = Enemy::enemy_type_map[additional["enemy_type"]];
		}
		if (additional.contains("team_id")) {
			team_id = additional["team_id"];
		}
		return Enemy::createEnemy(location, overlap, hit, team_id, type);
	}},
	{"button_start", [](vec2 location, vec2 size, float rotation,
				  COLLISION_HANDLER,
				  COLLISION_HANDLER, const json&)
				  {
		return Button::createButton(ButtonIcon::START, location, select_button_overlap("level_select"));
	}},
		{"button_save", [](vec2 location, vec2 size, float rotation,
				  COLLISION_HANDLER,
				  COLLISION_HANDLER, const json&)
				  {
		return Button::createButton(ButtonIcon::SAVE, location, select_save_data());
	}},
		{"button_continue", [](vec2 location, vec2 size, float rotation,
				  COLLISION_HANDLER,
				  COLLISION_HANDLER, const json&)
				  {
		return Button::createButton(ButtonIcon::CONTINUE, location, select_continue());
	}},
	{"button_setting", [](vec2 location, vec2 size, float rotation,
						COLLISION_HANDLER,
						COLLISION_HANDLER, const json&) {
		return Button::createButton(ButtonIcon::LEVEL_SELECT, location, select_button_overlap("loadout")); }
	},
	{"button_select_rocket", [](vec2 location, vec2 size, float rotation,
							  COLLISION_HANDLER,
							  COLLISION_HANDLER, const json&) {
			return Button::createButton(ButtonIcon::SELECT_ROCKET, location, select_weapon_of_type(W_ROCKET)); }},
		{"button_select_ammo", [](vec2 location, vec2 size, float rotation,
									COLLISION_HANDLER,
									COLLISION_HANDLER, const json&) {
			return Button::createButton(ButtonIcon::SELECT_AMMO, location, select_weapon_of_type(W_AMMO)); }},
		{"button_select_laser", [](vec2 location, vec2 size, float rotation,
									COLLISION_HANDLER,
									COLLISION_HANDLER, const json&) {
			return Button::createButton(ButtonIcon::SELECT_LASER, location, select_weapon_of_type(W_LASER)); }},
		{"button_select_bullet", [](vec2 location, vec2 size, float rotation,
									COLLISION_HANDLER,
									COLLISION_HANDLER, const json&) {
			return Button::createButton(ButtonIcon::SELECT_BULLET, location, select_weapon_of_type(W_BULLET)); }},
		{"button_select_direct", [](vec2 location, vec2 size, float rotation,
								COLLISION_HANDLER,
								COLLISION_HANDLER, const json&) {
		return Button::createButton(ButtonIcon::SELECT_DIRECT, location, select_algo_of_type(DIRECT)); }},
		{"button_select_a_star", [](vec2 location, vec2 size, float rotation,
								COLLISION_HANDLER,
								COLLISION_HANDLER, const json&) {
		return Button::createButton(ButtonIcon::SELECT_A_STAR, location, select_algo_of_type(A_STAR)); }},
			{"button_select_fire_ball", [](vec2 location, vec2 size, float rotation,
								COLLISION_HANDLER,
								COLLISION_HANDLER, const json&) {
		return Button::createButton(ButtonIcon::SELECT_FIREBALL, location, select_ability_of_type(FIREBALL)); }},
			{"button_select_force_sphere", [](vec2 location, vec2 size, float rotation,
								COLLISION_HANDLER,
								COLLISION_HANDLER, const json&) {
		return Button::createButton(ButtonIcon::SELECT_FIELD, location, select_ability_of_type(FIELD)); }},
	{"button_enter_level", [](vec2 location, vec2 size, float rotation,
						COLLISION_HANDLER,
						COLLISION_HANDLER, const json&)
					 {
						 return Button::createButton(ButtonIcon::NEXT, location, select_button_overlap(WorldSystem::selected_level));
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
	}},
    {"mainmenu", [](vec2 location, vec2 size, float rotation,
            COLLISION_HANDLER,
                    COLLISION_HANDLER, json additional) {
        std::string name = "mainmenu";
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
        MainMenu::createMainMenu(vec2{500, 500}, name, depth, scale);
    }},
	{"quality_slider", [](vec2 location, vec2 size, float,
			COLLISION_HANDLER overlap,
					COLLISION_HANDLER hit, json additional) {
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

	auto e = MoveableWall::createMoveableWall(location, size, 0, overlap, get_slider_callback(val_min, val_max, x_min, x_max, y_min, y_max));
	e.get<Motion>().position.x = (GameInstance::light_quality - val_min) / (val_max - val_min) * (x_max - x_min) + x_min;
	e.get<PhysicsObject>().mass = 100.f;
	}
	},

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
					 return level_progression["level_1"] > 0 ? Button::createButton(ButtonIcon::LEVEL1, location, select_level_button_overlap("intro")) : Button::createButton(ButtonIcon::LOCKED, location, select_button_overlap(""));
				 }},
		{ "select_level_2", [](vec2 location, vec2 size, float rotation,
						COLLISION_HANDLER,
						COLLISION_HANDLER, const json&)
					{
						return level_progression["level_2"] > 0 ? Button::createButton(ButtonIcon::LEVEL2, location, select_level_button_overlap("level_2")) : Button::createButton(ButtonIcon::LOCKED, location, select_button_overlap(""));
					} },
		{ "select_level_3", [](vec2 location, vec2 size, float rotation,
						COLLISION_HANDLER,
						COLLISION_HANDLER, const json&)
					{
						return level_progression["level_3"] > 0 ? Button::createButton(ButtonIcon::LEVEL3, location, select_level_button_overlap("level_3")) : Button::createButton(ButtonIcon::LOCKED, location, select_button_overlap(""));
					} },
		{ "select_level_4", [](vec2 location, vec2 size, float rotation,
					COLLISION_HANDLER,
					COLLISION_HANDLER, const json&)
					{
						return level_progression["level_4"] > 0 ? Button::createButton(ButtonIcon::LEVEL4, location, select_level_button_overlap("level_4")) : Button::createButton(ButtonIcon::LOCKED, location, select_button_overlap(""));
					} },
		{ "select_level_5", [](vec2 location, vec2 size, float rotation,
			COLLISION_HANDLER,
			COLLISION_HANDLER, const json&)
		{
			return level_progression["level_5"] > 0 ? Button::createButton(ButtonIcon::LEVEL5, location, select_level_button_overlap("level_5")) : Button::createButton(ButtonIcon::LOCKED, location, select_button_overlap(""));
		} },
		{ "select_level_6", [](vec2 location, vec2 size, float rotation,
						COLLISION_HANDLER,
						COLLISION_HANDLER, const json&)
					{
						return level_progression["level_6"] > 0 ? Button::createButton(ButtonIcon::LEVEL6, location, select_level_button_overlap("level_6")) : Button::createButton(ButtonIcon::LOCKED, location, select_button_overlap(""));
					} }, 
		{ "select_level_7", [](vec2 location, vec2 size, float rotation,
			COLLISION_HANDLER,
			COLLISION_HANDLER, const json&)
			{
				return level_progression["level_7"] > 0 ? Button::createButton(ButtonIcon::LEVEL7, location, select_level_button_overlap("level_7")) : Button::createButton(ButtonIcon::LOCKED, location, select_button_overlap(""));
					} },
		{ "select_level_8", [](vec2 location, vec2 size, float rotation,
		COLLISION_HANDLER,
		COLLISION_HANDLER, const json&)
		{
		return level_progression["level_8"] > 0 ? Button::createButton(ButtonIcon::LEVEL8, location, select_level_button_overlap("level_8")) : Button::createButton(ButtonIcon::LOCKED, location, select_button_overlap(""));
		} },
		{ "select_level_9", [](vec2 location, vec2 size, float rotation,
		COLLISION_HANDLER,
		COLLISION_HANDLER, const json&)
		{
		return level_progression["level_9"] > 0 ? Button::createButton(ButtonIcon::LEVEL9, location, select_level_button_overlap("level_9")) : Button::createButton(ButtonIcon::LOCKED, location, select_button_overlap(""));
		} },
		{ "select_level_10", [](vec2 location, vec2 size, float rotation,
		COLLISION_HANDLER,
		COLLISION_HANDLER, const json&)
		{
			return level_progression["level_10"] > 0 ? Button::createButton(ButtonIcon::LEVEL10, location, select_level_button_overlap("level_10")) : Button::createButton(ButtonIcon::LOCKED, location, select_button_overlap(""));
		} },
		{ "select_level_11", [](vec2 location, vec2 size, float rotation,
			COLLISION_HANDLER,
			COLLISION_HANDLER, const json&)
		{
			return level_progression["level_11"] > 0 ? Button::createButton(ButtonIcon::LEVEL11, location, select_level_button_overlap("level_11")) : Button::createButton(ButtonIcon::LOCKED, location, select_button_overlap(""));
		} },
		{ "select_level_12", [](vec2 location, vec2 size, float rotation,
			COLLISION_HANDLER,
			COLLISION_HANDLER, const json&)
		{
			return level_progression["level_12"] > 0 ? Button::createButton(ButtonIcon::LEVEL12, location, select_level_button_overlap("level_12")) : Button::createButton(ButtonIcon::LOCKED, location, select_button_overlap(""));
		} },
		{ "select_tutorial", [](vec2 location, vec2 size, float rotation, 
	COLLISION_HANDLER,
	COLLISION_HANDLER, const json&)
{
	return Button::createButton(ButtonIcon::TUTORIAL, location, select_button_overlap(TUTORIAL_NAME));
} },
    { "select_setting", [](vec2 location, vec2 size, float rotation,
COLLISION_HANDLER,
COLLISION_HANDLER, const json&)
{
return Button::createButton(ButtonIcon::SETTING, location, select_button_overlap("settings"));
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

				if (GameInstance::isPlayableLevel(at_level)) {
					if (!saved_flag[at_level]) {
						level_object.second(position, size, rotation, overlap, hit, additional);
					}
					else {
						if (!(level_object.first == "blocks" || level_object.first == "borders" || level_object.first == "movable_wall" || level_object.first == "player" || level_object.first == "enemy")) {
							level_object.second(position, size, rotation, overlap, hit, additional);
						}
					}
				}
				else {
					level_object.second(position, size, rotation, overlap, hit, additional);
				}
			}
		}
	}
	load_level_objects(at_level);
}

void LevelLoader::save_level_objects(std::string level)
{
	auto& physics_entities = ECS::registry<PhysicsObject>.entities;
	// auto& enemies = ECS::registry<Enemy>.entities;
	saved_level_states[level] = {};
	saved_flag[level] = true;

	saved_level_states[level].ai = GameInstance::algorithm;
	saved_level_states[level].weapon = GameInstance::selectedWeapon;

	for (auto entity : physics_entities) {
		if (entity.has<Soldier>() && entity.has<Motion>() && entity.has<PhysicsObject>() && entity.has<AIPath>() && entity.has<Health>()) {
			saved_level_states[level].soldiers.push_back(SoldierArg(entity.get<Motion>(), entity.get<Soldier>(), entity.get<Health>(), entity.get<AIPath>(), entity.get<PhysicsObject>()));
		}
		else if (entity.has<Wall>() && entity.has<Motion>() && entity.has<PhysicsObject>()) {
			saved_level_states[level].walls.push_back(WallArg(entity.get<Motion>(), entity.get<Wall>(), entity.get<PhysicsObject>()));
		}
		else if (entity.has<MoveableWall>() && entity.has<Motion>() && entity.has<PhysicsObject>()) {
			saved_level_states[level].moveable_walls.push_back(MoveableWallArg(entity.get<Motion>(), entity.get<MoveableWall>(), entity.get<PhysicsObject>()));
		}
		else if (entity.has<Enemy>() && entity.has<Motion>() && entity.has<PhysicsObject>() && entity.has<AIPath>() && entity.has<Health>()) {
			saved_level_states[level].enemies.push_back(EnemyArg(entity.get<Motion>(), entity.get<Enemy>(), entity.get<Health>(), entity.get<AIPath>(), entity.get<PhysicsObject>()));
		}
		else if (entity.has<Shield>() && entity.has<Motion>() && entity.has<PhysicsObject>()) {
			saved_level_states[level].shields.push_back(ShieldArg(entity.get<Motion>(), entity.get<Shield>(), entity.get<Health>(), entity.get<PhysicsObject>()));
		}
	}
}

void LevelLoader::load_level_objects(std::string level)
{
	if (!saved_flag[at_level]) {
		return;
	}
	LevelEntityState current_level = saved_level_states[level];

	if (current_level.soldiers.size() > 0) {
		Soldier::createSoldier(std::get<0>(current_level.soldiers[0]), std::get<1>(current_level.soldiers[0]), std::get<2>(current_level.soldiers[0]), std::get<3>(current_level.soldiers[0]), std::get<4>(current_level.soldiers[0]));
	}
	for (auto wall_arg : current_level.walls) {
		Wall::createWall(std::get<0>(wall_arg), std::get<1>(wall_arg), std::get<2>(wall_arg));
	}
	for (auto moveable_wall_arg : current_level.moveable_walls) {
		MoveableWall::createMoveableWall(std::get<0>(moveable_wall_arg), std::get<1>(moveable_wall_arg), std::get<2>(moveable_wall_arg));
	}
	for (auto enemy_arg : current_level.enemies) {
		Enemy::createEnemy(std::get<0>(enemy_arg), std::get<1>(enemy_arg), std::get<2>(enemy_arg), std::get<3>(enemy_arg), std::get<4>(enemy_arg));
	}
	if (current_level.shields.size() > 0) {
		WorldSystem::hasShield = true;
		WorldSystem::shield = Shield::createShield(std::get<0>(current_level.shields[0]), std::get<1>(current_level.shields[0]), std::get<2>(current_level.shields[0]), std::get<3>(current_level.shields[0]));
		WorldSystem::SHIELDUP = true;
	}

	GameInstance::algorithm = current_level.ai;
	GameInstance::selectedWeapon = current_level.weapon;

	saved_level_states[at_level] = {};
	saved_flag[at_level] = false;
}


int LevelLoader::get_level_state(std::string level)
{
	return level_progression[level];
}

bool LevelLoader::is_level_unlocked(std::string level)
{
	return level_progression[level] == 1;
}

std::string LevelLoader::get_next_level_name(std::string level)
{
	if (level == level_order.back())
		return level_order.back();
	auto it = std::find(level_order.begin(), level_order.end(), level);
	auto next = std::next(it, 1);
	return *next;
}

void LevelLoader::update_level_state(std::string level, int state)
{
	if (std::find(level_order.begin(), level_order.end(), level) != level_order.end()) {
		if (state == 1) {
			level_progression[level] = state;
			if (!is_level_unlocked(get_next_level_name(level))) {
				level_progression[get_next_level_name(level)] = 1;
				save_level_data();
			}
		}
	}
}

/*
void LevelLoader::set_level_value(std::string level, int value)
{
	level_progression[level] = value;
}
*/


void LevelLoader::set_level(std::string level) {
	at_level = level;
}
