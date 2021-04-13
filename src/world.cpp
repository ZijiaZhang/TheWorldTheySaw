// Header
#include "world.hpp"
#include "physics.hpp"
#include "debug.hpp"
#include "turtle.hpp"
#include "fish.hpp"
#include "shield.hpp"
#include "pebbles.hpp"
#include "render_components.hpp"
#include "tiny_ecs.hpp"
#include "Bullet.hpp"
#include "Wall.hpp"
#include "MoveableWall.hpp"
#include "background.hpp"
#include "start.hpp"
#include "buttonStart.hpp"
#include "buttonSetting.hpp"
#include "loading.hpp"
#include "Weapon.hpp"
#include "Explosion.hpp"
#include "MagicParticle.hpp"
#include "WeaponTimer.hpp"
#include "mainMenu.hpp"

// stlib
#include <string.h>
#include <cassert>
#include <sstream>
#include <iostream>
#include <deque>
#include <nlohmann/json.hpp>
#include <Particle.hpp>
#include <highlight_circle.hpp>
#include <pop_up.hpp>

// for convenience
using json = nlohmann::json;

// Game configuration
LevelLoader level_loader;
//const size_t GUNFIRE_DELAY_MS = 1500;
bool SHIELDUP = false;
bool hasShield = false;
bool fired = false;
std::deque<vec2> mouse_points;
int MOUSE_POINTS_COUNT = 600;
int LOW_RANGE = 0;
int HIGH_RANGE = 1000;
bool DRAWING = false;
int DEGREE_SIZE = 90;
int SECTION_POINT_NUM = 2;
bool WorldSystem::selecting = false;
bool WorldSystem::pause = false;

int KILL_SIZE = 3000;
vec2 prev_pl_pos = {0,0};

std::string WorldSystem::selected_level = "level_3";

static float getDist(vec2 p1, vec2 p2)
{
	float dist = std::sqrt(std::pow(p1.x - p2.x, 2) + std::pow(p1.y - p2.y, 2));
	//    printf("dist: %f \n", dist);
	return dist;
}

//static float getAngle(vec2 p1, vec2 p2)
//{
//	float x1 = p1.x;
//	float y1 = p1.y;
//	float x2 = p2.x;
//	float y2 = p2.y;
//
//	float dot = x1 * x2 + y1 * y2;
//	float det = x1 * y2 - y1 * x2;
//	return atan2(det, dot);
//}


/*
 Dummy way
 split 360 degrees into even sections with DEGREE_SIZE, then
 check points are in the range between LOW_RANGE and HIGH_RANGE from salmon position, and
 check there are SECTION_POINT_NUM in each section

 Change LOW_RANGE, HIGH_RANGE, DEGREE_SIZE AND SECTION_POINT_NUM to simulate the circle.
 */
static bool checkCircle(ECS::Entity player_salmon)
{
	auto motion = ECS::registry<Motion>.get(player_salmon);
	vec2 salmonPos = motion.position;
	std::vector<int> bucket;
	bucket.resize(360 / DEGREE_SIZE);
//	vec2 ori = { 1, 0 };

	for (vec2 p : mouse_points)
	{
		float dist = getDist(p, salmonPos);
		if (dist >= LOW_RANGE && dist <= HIGH_RANGE)
		{
			float angle = atan2(p.y, p.x);
			float degree = angle * 180 / M_PI + 180.0; // shift range from [-180, 180] to [0,360]
			int num = int(degree) / DEGREE_SIZE;
			// prevent overflow
			if (num == 360 / DEGREE_SIZE) {
				num--;
			}
			bucket[num] += 1;
		}
	}
	for (int i : bucket) {
		if (i < SECTION_POINT_NUM) {
			return false;
		}
	}
	return true;
}

// Create the fish world
// Note, this has a lot of OpenGL specific things, could be moved to the renderer; but it also defines the callbacks to the mouse and keyboard. That is why it is called here.
WorldSystem::WorldSystem(ivec2 window_size_px) :
	points(0)
{
	// Seeding rng with random device
	rng = std::default_random_engine(std::random_device()());

	///////////////////////////////////////
	// Initialize GLFW
	auto glfw_err_callback = [](int error, const char* desc) { std::cerr << "OpenGL:" << error << desc << std::endl; };
	glfwSetErrorCallback(glfw_err_callback);
	if (!glfwInit())
		throw std::runtime_error("Failed to initialize GLFW");

	//-------------------------------------------------------------------------
	// GLFW / OGL Initialization, needs to be set before glfwCreateWindow
	// Core Opengl 3.
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
#if __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	glfwWindowHint(GLFW_RESIZABLE, 0);

	// Create the main window (for rendering, keyboard, and mouse input)
	window = glfwCreateWindow(window_size_px.x, window_size_px.y, "Game Project", nullptr, nullptr);
	if (window == nullptr)
		throw std::runtime_error("Failed to glfwCreateWindow");

	// Setting callbacks to member functions (that's why the redirect is needed)
	// Input is handled using GLFW, for more info see
	// http://www.glfw.org/docs/latest/input_guide.html
	glfwSetWindowUserPointer(window, this);

	auto mouse_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse(_0, _1, _2); };
	auto cursor_pos_redirect = [](GLFWwindow* wnd, double _0, double _1) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_move({ _0, _1 }); };

	glfwSetMouseButtonCallback(window, mouse_redirect);
	glfwSetCursorPosCallback(window, cursor_pos_redirect);

	auto key_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2, int _3) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_key(_0, _1, _2, _3); };
	glfwSetKeyCallback(window, key_redirect);

	// Playing background music indefinitely
	init_audio();
	//Mix_PlayMusic(background_music, -1);
	// std::cout << "Loaded music\n";
	Mix_PlayMusic(background_music, -1);

}

WorldSystem::~WorldSystem() {
	// Destroy music components
	if (background_music != nullptr)
		Mix_FreeMusic(background_music);
	Mix_CloseAudio();   //TODO: throw exception

	// Destroy all created components
	ECS::ContainerInterface::clear_all_components();

	// Close the window
	glfwDestroyWindow(window);
}

void WorldSystem::init_audio()
{
	//////////////////////////////////////
	// Loading music and sounds with SDL
	if (SDL_Init(SDL_INIT_AUDIO) < 0)
		throw std::runtime_error("Failed to initialize SDL Audio");

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1)
		throw std::runtime_error("Failed to open audio device");

	background_music = Mix_LoadMUS(audio_path("background.wav").c_str());
	//gun_fire = Mix_LoadWAV(audio_path("gun_fire.wav").c_str());
	//gun_reload = Mix_LoadWAV(audio_path("firework.mp3").c_str());

	if (background_music == nullptr)
		throw std::runtime_error("Failed to load sounds make sure the data directory is present: " +
			audio_path("gun_background.wav") +
			audio_path("gun_fire.wav") +
			audio_path("gun_reload.wav"));
}

// Update our game world
void WorldSystem::step(float elapsed_ms, vec2 window_size_in_game_units)
{
	// Updating window title with points
	std::stringstream title_ss;
	title_ss << "Points: " << points;
	glfwSetWindowTitle(window, title_ss.str().c_str());

	if (screen != window_size_in_game_units) {
		screen = window_size_in_game_units;
	}

	assert(ECS::registry<ScreenState>.components.size() <= 1);



	for (int i = static_cast<int>(ECS::registry<DeathTimer>.components.size()) - 1; i >= 0; --i)
	{
		auto entity = ECS::registry<DeathTimer>.entities[i];
		// Progress timer
		auto& counter = ECS::registry<DeathTimer>.get(entity);
		counter.counter_ms -= elapsed_ms;

		// Restart the game once the death timer expired
		if (counter.counter_ms <= 0)
		{
			bool is_pop_up = entity.has<PopUP>();
			if (is_pop_up) {
				auto& popup = entity.get<PopUP>();
				for (auto& e : popup.relative_entities) {
					ECS::ContainerInterface::remove_all_components_of(e);
				}
				popup.on_destroy();
			}
			ECS::ContainerInterface::remove_all_components_of(entity);
			if (is_pop_up && ECS::registry<PopUP>.entities.empty()) {
				GameInstance::popup_speed = 1.0;
			}

		}
	}

    for (int i = static_cast<int>(ECS::registry<ExplodeTimer>.components.size()) - 1; i >= 0; --i)
    {
        auto entity = ECS::registry<ExplodeTimer>.entities[i];
        // Progress timer
        auto& counter = ECS::registry<ExplodeTimer>.get(entity);
        counter.counter_ms -= elapsed_ms;

        // Restart the game once the death timer expired
        if (counter.counter_ms < 0)
        {
            counter.callback(entity);
        }
    }
//    
//    for (int i = static_cast<int>(ECS::registry<FieldTimer>.components.size()) - 1; i >= 0; --i)
//    {
//        auto entity = ECS::registry<FieldTimer>.entities[i];
//        // Progress timer
//        auto& counter = ECS::registry<FieldTimer>.get(entity);
//        counter.counter_ms -= elapsed_ms;
//
//        // Restart the game once the death timer expired
//        if (counter.counter_ms < 0)
//        {
//            ECS::registry<FieldTimer>.remove(player_soldier);
//            //ECS::registry<Activating>.remove(player_soldier);
//            Soldier::set_shader(player_soldier, true, Soldier::ori_texture_path, Soldier::ori_shader_name);
//            ECS::registry<Soldier>.get(player_soldier).forcefield_on = false;
//        }
//    }

	aiControl = GameInstance::isPlayableLevel();
//	if(player_soldier.has<AIPath>())
//        player_soldier.get<AIPath>().active = aiControl;

	/*
	if (isPlayableLevel(currentLevel) && player_soldier.has<Motion>() && player_soldier.has<Health>()) {
		auto& motion = player_soldier.get<Motion>();
		auto& health = player_soldier.get<Health>();
		Soldier::updateSoldierHealthBar(motion.position, motion.scale, health.hp, health.max_hp);
	}
	*/

	//Healthbar::updateHealthBar(player_soldier, isPlayableLevel(GameInstance::currentLevel));

	// pause = pause && GameInstance::isPlayableLevel();

	endGameTimer += elapsed_ms;
	runTimer(elapsed_ms);
	checkEndGame();
    if(player_soldier.has<Motion>()) {
        vec2 pl = ECS::registry<Motion>.get(player_soldier).position;
        for (auto e : ECS::registry<Background>.entities) {
            auto c = ECS::registry<Background>.get(e);
            auto &bg_m = ECS::registry<Motion>.get(e);
            float depth = c.depth;
            if (depth != 0.f) {
                bg_m.position += (pl - prev_pl_pos) / depth;
                prev_pl_pos = pl;
            }
        }
	}

	// flash the frozen enemies when there is 1000ms left.
	auto frozenEnemies = ECS::registry<FrozenTimer>.entities;
	for (auto e: frozenEnemies) {
	    auto executing_ms = ECS::registry<FrozenTimer>.get(e).executing_ms;
	    if (executing_ms < 1000.f && !ECS::registry<Activating>.has(e)) {
            ECS::registry<Activating>.emplace(e);
            Enemy::set_activating_shader(e);
	    }
	}

	// fix the weapon timer location.
	auto weaponTimers = ECS::registry<WeaponTimer>.entities;
	for (auto e : weaponTimers) {
	    auto& motion = ECS::registry<Motion>.get(e);
	    motion.position = player_soldier.get<Motion>().position - motion.offset;
	}

	bool executingDone = WeaponTimer::updateAllWeaponTimers(elapsed_ms);
    if (executingDone) {
        Soldier::switchWeapon(player_soldier, W_BULLET);
    }
}

// Reset the world state to its initial state
void WorldSystem::restart(std::string level)
{
	level_loader.set_level(level);
	selecting = false;
    GameInstance::currentLevel = level;
	// Debugging for memory/component leaks
	ECS::ContainerInterface::list_all_components();


	// Reset the game speed
	pause = false;
	GameInstance::pause_speed = 1.f;
	GameInstance::ability_speed = 1.f;
	GameInstance::global_speed = 1.f;
	GameInstance::popup_speed = 1.f;
	control_state = ControlState::NORMAL;
    // Remove all entities that we created
	// All that have a motion, we could also iterate over all fish, turtles, ... but that would be more cumbersome
	while (!ECS::registry<Motion>.entities.empty())
		ECS::ContainerInterface::remove_all_components_of(ECS::registry<Motion>.entities.back());


	SHIELDUP = false;
	hasShield = false;
	while (!ECS::registry<Shield>.entities.empty())
		ECS::ContainerInterface::remove_all_components_of(ECS::registry<Shield>.entities.back());


	// Debugging for memory/component leaks
	ECS::ContainerInterface::list_all_components();
	// load background, walls, enemies and player from level_loaders
	level_loader.load_level();

	auto soldiers = ECS::registry<Soldier>.entities;
	if (soldiers.size() != 1) {
		throw std::runtime_error("Can only have one soldier");
	}

	GameInstance::charges_left = GameInstance::getDefaultChargeOfMagic(GameInstance::selectedMagic);

	player_soldier = soldiers.front();

	while (!ECS::registry<Camera>.entities.empty())
		ECS::ContainerInterface::remove_all_components_of(ECS::registry<Camera>.entities.back());

	Global_Meshes::meshes.clear();

	ECS::Entity camera;
	camera.insert(Camera({ 0,0 }, player_soldier));

	prev_pl_pos = ECS::registry<Motion>.get(player_soldier).position;
    aiControl = GameInstance::isPlayableLevel();
    if(player_soldier.has<AIPath>()){
        player_soldier.get<AIPath>().active = true;
    }
    if (aiControl) {
        WeaponTimer::createAllWeaponTimers();
    }

	if (GameInstance::fist_enter_level(level)) {
		if (level == MENU_NAME) {
			GameInstance::popup_speed = 0.0;
			auto e = PopUP::createPopUP(textures_path("/tutorial/You.png"), screen / 2.f - vec2{0.0, 200}, { 200, 100 });
			auto& pop_up = e.get<PopUP>();
			pop_up.relative_entities.push_back(
				HighLightCircle::createHighLightCircle(player_soldier.get<Motion>().position, 30, 5));
			pop_up.on_destroy = [=]() {
				auto e = PopUP::createPopUP(textures_path("/tutorial/Movement.png"), screen / 2.f - vec2{ 0.0, 100 }, { 200, 100 });
				auto& pop_up = e.get<PopUP>();
				pop_up.relative_entities.push_back(
					HighLightCircle::createHighLightCircle({ 480, 675 }, 50, 5));
			};
		}

		if (level == WEAPON_SELECT_NAME) {
			GameInstance::popup_speed = 0.0;
			auto e = PopUP::createPopUP(textures_path("/tutorial/Loadout_pick.png"), screen / 2.f - vec2{ 0.0, 200 }, { 200, 100 });
			auto& pop_up = e.get<PopUP>();
			pop_up.relative_entities.push_back(
				HighLightCircle::createHighLightCircle({ 80,60 }, 30, 5));
			pop_up.relative_entities.push_back(
				HighLightCircle::createHighLightCircle({ 340,60 }, 30, 5));
			pop_up.relative_entities.push_back(
				HighLightCircle::createHighLightCircle({ 340,137 }, 30, 5));
			pop_up.relative_entities.push_back(
				HighLightCircle::createHighLightCircle({ 80,137 }, 30, 5));
			pop_up.relative_entities.push_back(
				HighLightCircle::createHighLightCircle({ 80,405 }, 30, 5));
			pop_up.relative_entities.push_back(
				HighLightCircle::createHighLightCircle({ 340,405 }, 30, 5));
			pop_up.on_destroy = [=]() {
				auto e = PopUP::createPopUP(textures_path("/tutorial/Enter_level.png"), screen / 2.f - vec2{ 0.0, 100 }, { 200, 100 });
				auto& pop_up = e.get<PopUP>();
				pop_up.relative_entities.push_back(
					HighLightCircle::createHighLightCircle({ 315, 540 }, 50, 5));
			};
		}
	}

	if (level == TUTORIAL_NAME) {
		GameInstance::popup_speed = 0.0;
		auto e = PopUP::createPopUP(textures_path("/tutorial/Enemy.png"), screen / 2.f - vec2{ 0.0, 200 }, { 200, 100 });
		auto& pop_up = e.get<PopUP>();
		pop_up.relative_entities.push_back(
			HighLightCircle::createHighLightCircle({ 500,500 }, 30, 5));
		pop_up.on_destroy = [=]() {
			auto e = PopUP::createPopUP(textures_path("/tutorial/Ability.png"), screen / 2.f - vec2{ 0.0, 100 }, { 200, 100 });
			show_ability_tutorial = true;
		};
	}

	GameInstance::set_enter_level(level);

}


void WorldSystem::checkEndGame()
{
	if (GameInstance::isPlayableLevel()) {
        if (ECS::registry<Enemy>.entities.empty()) {
			resetTimer();
			if (GameInstance::currentLevel == TUTORIAL_NAME) {
				restart(MENU_NAME);
			}
			else {
				level_loader.update_level_state(GameInstance::currentLevel, 1);
				restart("win");
			}

        }
        if (ECS::registry<Soldier>.entities.empty()) {
            resetTimer();
            restart(GameInstance::currentLevel == TUTORIAL_NAME ? MENU_NAME : "lose");
        }
		if (endGameTimer > 100000.f) {
			if (!ECS::registry<Enemy>.entities.empty()) {
				resetTimer();
				restart(GameInstance::currentLevel == TUTORIAL_NAME ? MENU_NAME : "lose");
			}
		}
	}
}

void WorldSystem::runTimer(float elapsed_ms)
{
	if (GameInstance::isPlayableLevel()) {
		endGameTimer += elapsed_ms;
	}
	else {
		resetTimer();
	}
}

void WorldSystem::resetTimer()
{
	endGameTimer = 0.f;
}

// Compute collisions between entities
void WorldSystem::handle_collisions()
{
	// Loop over all collisions detected by the physics system
	auto& registry = ECS::registry<PhysicsSystem::Collision>;
	for (unsigned int i = 0; i < registry.components.size(); i++)
	{
		// The entity and its collider
		auto entity = registry.entities[i];
		auto entity_other = registry.components[i].other;

		// For now, we are only interested in collisions that involve the soldier
		if (ECS::registry<Soldier>.has(entity))
		{
			// Checking Soldier - Turtle collisions
			if (ECS::registry<Turtle>.has(entity_other))
			{
				// initiate death unless already dying
				if (!ECS::registry<DeathTimer>.has(entity))
				{
					// Scream, reset timer, and make the soldier sink
					ECS::registry<DeathTimer>.emplace(entity);
				}
			}
		}
	}

	// Remove all collisions from this simulation step
	ECS::registry<PhysicsSystem::Collision>.clear();
}

// Should the game be over ?
bool WorldSystem::is_over() const
{
	return glfwWindowShouldClose(window) > 0;
}

// On key callback
// TODO A1: check out https://www.glfw.org/docs/3.3/input_guide.html
void WorldSystem::on_key(int key, int, int action, int mod)
{
   
	double soldier_speed = 200;
  
    if(key == GLFW_KEY_Q && action == GLFW_PRESS && GameInstance::get_current_speed() != 0 && GameInstance::isPlayableLevel()) {
		if (control_state == ControlState::USING_MAGIC) {
			GameInstance::ability_speed = 1.0;
			control_state = ControlState::NORMAL;
		} else if(player_soldier.has<Soldier>() && GameInstance::charges_left > 0) {
			GameInstance::charges_left--;
			if (GameInstance::selectedMagic == FIREBALL) {
				// Slowdown the world speed
				if (control_state == ControlState::NORMAL) {
					control_state = ControlState::USING_MAGIC;
					GameInstance::ability_speed = 0.2;
				}
				if (show_ability_tutorial) {
					GameInstance::popup_speed = 0.0;
					auto e = PopUP::createPopUP(textures_path("/tutorial/UseMagic.png"), screen / 2.f - vec2{ 0.0, 200 }, { 200, 100 }); 
					show_ability_tutorial = false;
				}
				show_ability_tutorial = false;
			}
			else if (GameInstance::selectedMagic == FIELD) {
				
				Soldier::set_field_shader(player_soldier);
				Soldier::set_field(player_soldier);
			}
        }
    }

	if (key == GLFW_KEY_E && action == GLFW_PRESS) {
		if (player_soldier.has<Soldier>()) {
			Bullet::createBullet(player_soldier.get<Motion>().position, player_soldier.get<Motion>().angle, { 380, 0 }, 0, W_BULLET, "bullet");
		}
	}


    if(key == GLFW_KEY_A && action == GLFW_PRESS) {
        Soldier::switchWeapon(player_soldier, W_LASER);
    }
    if(key == GLFW_KEY_S && action == GLFW_PRESS) {
        Soldier::switchWeapon(player_soldier, W_AMMO);
    }
    if(key == GLFW_KEY_D && action == GLFW_PRESS) {
        Soldier::switchWeapon(player_soldier, W_ROCKET);
    }
    if(key == GLFW_KEY_F && action == GLFW_PRESS) {
        Soldier::switchWeapon(player_soldier, W_BULLET);
    }

    if (!aiControl) {
        // Move soldier if alive
        if (!ECS::registry<DeathTimer>.has(player_soldier) && player_soldier.has<Motion>()) {
            if (key == GLFW_KEY_W) {
                if (player_soldier.has<AIPath>()) {
                    auto &aiPath = player_soldier.get<AIPath>();
                    aiPath.active = false;
                }
                player_soldier.get<Motion>().velocity =
                        vec2{ soldier_speed, 0} * (float) (action == GLFW_PRESS || action == GLFW_REPEAT);
            }

			if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
				selecting = true;
//                auto& registry = ECS::registry<Button>;
//                for (unsigned int i=0; i< registry.components.size(); i++)
//                {
//                    auto entity = registry.entities[i];
//                    ECS::registry<PressTimer>.emplace(entity);
//                }
			}
			else {
				selecting = false;
			}
        }

    }

    if (key == GLFW_KEY_H && action == GLFW_PRESS) {
        if(RenderSystem::renderSystem)
            RenderSystem::renderSystem->create_light_texture(1);
    }
    if (key == GLFW_KEY_L && action == GLFW_PRESS) {
        if(RenderSystem::renderSystem)
            RenderSystem::renderSystem->create_light_texture(0.2);
    }

	// Resetting game
	if (action == GLFW_RELEASE && key == GLFW_KEY_R)
	{
		int w, h;
		glfwGetWindowSize(window, &w, &h);
		restart(GameInstance::currentLevel);
	}

	if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
		restart("settings"); 
	}

	if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
		// level_loader.set_level("level_2");
		// level_loader.at_level = "level_2";
        restart("level_1");
        //ECS::ContainerInterface::remove_all_components_of(player_soldier);

    }
	if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
		// level_loader.set_level("level_3");
		// level_loader.at_level = "level_3";
		restart("level_3");
	}
    if (key == GLFW_KEY_4 && action == GLFW_PRESS) {
        // level_loader.set_level("level_3");
        // level_loader.at_level = "level_3";
        restart("level_4");
    }
    if (key == GLFW_KEY_5 && action == GLFW_PRESS) {
        // level_loader.set_level("level_3");
        // level_loader.at_level = "level_3";
        reload_level = true;
        reload_level_name = TUTORIAL_NAME;
    }

	// Debugging
	if (key == GLFW_KEY_O)
		DebugSystem::in_debug_mode = (action != GLFW_RELEASE);

	// Debugging
	if (key == GLFW_KEY_P)
		DebugSystem::in_profile_mode = (action != GLFW_RELEASE);

	if (key == GLFW_KEY_Z && action == GLFW_RELEASE && GameInstance::isPlayableLevel(GameInstance::currentLevel)) {
		level_loader.save_level_objects(GameInstance::currentLevel);
		reload_level_name = "level_select";
		reload_level = true;
	}

	if (key == GLFW_KEY_X && action == GLFW_RELEASE && GameInstance::isPlayableLevel()  && (pause || GameInstance::get_current_speed() != 0)) {
		pause = !pause;
		if (pause) {
			GameInstance::pause_speed = 0.f;
		}
		else {
			GameInstance::pause_speed = 1.f;
		}
	}

	if (key == GLFW_KEY_C && action == GLFW_RELEASE && GameInstance::isPlayableLevel() && !pause) {
		SHIELDUP = true;
	}
    if (key == GLFW_KEY_K && action == GLFW_RELEASE && GameInstance::isPlayableLevel() && !pause) {
        Soldier::set_field_shader(player_soldier);
        Soldier::set_field(player_soldier);
    }
}

void WorldSystem::on_mouse(int key, int action, int mod) {

    if (action == GLFW_PRESS && key == GLFW_MOUSE_BUTTON_LEFT && !pause)
    {
		if (!ECS::registry<PopUP>.entities.empty()) {
			auto& entity = ECS::registry<PopUP>.entities.back();
			if (!entity.has<DeathTimer>()) {
				entity.emplace<DeathTimer>();
			}
			return;
		}

		if (control_state == ControlState::USING_MAGIC) {
			vec2 mouse_pos = getWorldMousePosition(last_mouse_pos);
			vec2 dir = mouse_pos.y - player_soldier.get<Motion>().position;
			float rad = atan2(dir.y, dir.x);
			MagicParticle::createMagicParticle(player_soldier.get<Motion>().position,
				rad,
				{ 380, 0 },
				0,
				FIREBALL);
			control_state = ControlState::NORMAL;
			GameInstance::ability_speed = 1.f;
		}

		if (!aiControl && player_soldier.has<AIPath>()) {
			auto& aiPath = player_soldier.get<AIPath>();
			aiPath.active = true;
			player_soldier.get<Motion>().velocity = { 200.f, 0.f };
			aiPath.path.path.clear();
			aiPath.progress = 0;
			aiPath.path.path.push_back(AISystem::get_grid_from_loc(getWorldMousePosition(last_mouse_pos)));
		}
		//HighLightCircle::createHighLightCircle(getWorldMousePosition(last_mouse_pos), 100);

		// PopUP::createPopUP(textures_path("/enemy/cannon/alien.png"), screen / 2.f, { 200, 100 });
		DRAWING = true;
        mouse_points.clear();
    }
    else if (action == GLFW_RELEASE && key == GLFW_MOUSE_BUTTON_LEFT)
    {
        DRAWING = false;
        if (checkCircle(player_soldier))
        {
            // SHIELDUP = true;
        }
    }

}

void WorldSystem::on_mouse_move(vec2 mouse_pos)
{
    last_mouse_pos = mouse_pos;
    if (!ECS::registry<DeathTimer>.has(player_soldier) && player_soldier.has<Motion>())
    {
        auto& motion = ECS::registry<Motion>.get(player_soldier);
        // Get world mouse position
//        if(!(ECS::registry<MainMenu>.has(player_soldier) || ECS::registry<Start>.has(player_soldier) || ECS::registry<Button>.components[1].buttonType == ButtonIcon::START)){
//            mouse_pos = getWorldMousePosition(mouse_pos);
//        }
        mouse_pos = getWorldMousePosition(mouse_pos);
        float disY = mouse_pos.y - motion.position.y;
        float disX = mouse_pos.x - motion.position.x;
        float longestL = sqrt(pow(disY, 2) + pow(disX, 2));

        float sinV = asin(disY / longestL);
        float cosV = acos(disX / longestL);
        auto dir = mouse_pos - motion.position;
        // printf("%f,%f\n",mouse_pos.x, mouse_pos.y);
        float rad = atan2(dir.y, dir.x);
        if (!aiControl && player_soldier.has<AIPath>() && player_soldier.get<AIPath>().path.path.empty()) {
            motion.angle = rad;
        }
        if (SHIELDUP && !hasShield) {
            shield = Shield::createShield({ motion.position.x + 300 * cosV, motion.position.y + 300 * sinV }, 0);
            hasShield = true;
        }

        if (SHIELDUP) {
			if (shield.has<Motion>() && !pause) {
				auto& motionSh = ECS::registry<Motion>.get(shield);
				motionSh.position = vec2(motion.position.x + disX / 2, motion.position.y + disY / 2);
				motionSh.angle = rad;
			}
        }
        if (DRAWING) {
            if (mouse_points.size() >= MOUSE_POINTS_COUNT) {
                mouse_points.pop_front();
            }
            mouse_points.push_back(mouse_pos - motion.position);
        }
    }
}

vec2 WorldSystem::getWorldMousePosition(vec2 mouse_pos) const {
    if (!ECS::registry<Camera>.entities.empty()) {
        auto& camera = ECS::registry<Camera>.entities[0];
        if (camera.has<Camera>()) {
            vec2 camera_pos = camera.get<Camera>().get_position();
            mouse_pos += camera_pos;
        }
    }
    return mouse_pos;
}

bool WorldSystem::reload_level = false;
std::string WorldSystem::reload_level_name = "menu";
