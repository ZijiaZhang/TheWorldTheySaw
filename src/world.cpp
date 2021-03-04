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

// stlib
#include <string.h>
#include <cassert>
#include <sstream>
#include <iostream>
#include <deque>
#include <nlohmann/json.hpp>

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
int DEGREE_SIZE = 45;
int SECTION_POINT_NUM = 3;

int KILL_SIZE = 3000;

static float getDist(vec2 p1, vec2 p2)
{
	float dist = std::sqrt(std::pow(p1.x - p2.x, 2) + std::pow(p1.y - p2.y, 2));
	//    printf("dist: %f \n", dist);
	return dist;
}

//static float getAngle(vec2 p1, vec2 p2)
//{
//    float x1 = p1.x;
//    float y1 = p1.y;
//    float x2 = p2.x;
//    float y2 = p2.y;
//
//    float dot = x1*x2 + y1*y2;
//    float det = x1*y2 - y1*x2;
//    return atan2(det, dot);
//}

static std::map<std::string, bool> playableLevelMap = {
		{"level_1", false},
		{"level_2", false},
		{"level_3", true},
		{"level_4", true}
};

/*
 Dummy way
 split 360 degrees into even sections with DEGREE_SIZE, then
 check points are in the range between LOW_RANGE and HIGH_RANGE from salmon position, and
 check there are SECTION_POINT_NUM in each section

 Change LOW_RANGE, HIGH_RANGE, DEGREE_SIZE AND SECTION_POINT_NUM to simulate the circle.
 */
static bool checkCircle(ECS::Entity& player_salmon)
{
	auto motion = ECS::registry<Motion>.get(player_salmon);
	vec2 salmonPos = motion.position;
	std::vector<int> bucket;
	bucket.resize(360 / DEGREE_SIZE);
	//    vec2 ori = {1, 0};

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
	points(0),
	next_turtle_spawn(0.f),
	next_fish_spawn(0.f),
	next_gunfire_spawn(0.f)

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
	std::cout << "Loaded music\n";
}

WorldSystem::~WorldSystem() {
	// Destroy music components
	if (background_music != nullptr)
		Mix_FreeMusic(background_music);
	if (gun_fire != nullptr)
		Mix_FreeChunk(gun_fire);
	if (gun_reload != nullptr)
		Mix_FreeChunk(gun_reload);
	Mix_CloseAudio();

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

	background_music = Mix_LoadMUS(audio_path("gun_background.wav").c_str());
	gun_fire = Mix_LoadWAV(audio_path("gun_fire.wav").c_str());
	gun_reload = Mix_LoadWAV(audio_path("gun_reload.wav").c_str());

	if (background_music == nullptr || gun_fire == nullptr || gun_reload == nullptr)
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
	for (int i = static_cast<int>(ECS::registry<Motion>.components.size()) - 1; i >= 0; --i)
	{
		auto& m = ECS::registry<Motion>.components[i];
		if (abs(m.position.x) > KILL_SIZE || abs(m.position.y) > KILL_SIZE) {
			ECS::ContainerInterface::remove_all_components_of(ECS::registry<Motion>.entities[i]);
		}
	}
	//    auto& motion = player_soldier.get<Motion>();
		// motion.velocity = {100.f,0};
		// Spawning new turtles
	next_turtle_spawn -= elapsed_ms * current_speed;

	if (screen != window_size_in_game_units) {
		screen = window_size_in_game_units;
	}

	//	if (ECS::registry<Turtle>.components.size() <= MAX_TURTLES && next_turtle_spawn < 0.f)
	//	{
	//		// Reset timer
	//		next_turtle_spawn = (TURTLE_DELAY_MS / 2) + uniform_dist(rng) * (TURTLE_DELAY_MS / 2);
	//		// Create turtle
	//		ECS::Entity entity = Turtle::createTurtle({0, 0});
	//		// Setting random initial position and constant velocity
	//		auto& motion = ECS::registry<Motion>.get(entity);
	//		motion.position = vec2(window_size_in_game_units.x - 150.f, 50.f + uniform_dist(rng) * (window_size_in_game_units.y - 100.f));
	//		motion.velocity = vec2(-100.f, 0.f );
	//	}


	//	next_gunfire_spawn -= elapsed_ms * current_speed;
	//	if (fired && next_gunfire_spawn < 0.f)
	//	{
	//		next_gunfire_spawn = (GUNFIRE_DELAY_MS / 2) + uniform_dist(rng) * (GUNFIRE_DELAY_MS / 2);
	//		Mix_PlayChannel(-1, gun_reload, 0);
	//		fired = false;
	//	}
	//	else if (!fired && next_gunfire_spawn < 0.f)
	//	{
	//		next_gunfire_spawn = (GUNFIRE_DELAY_MS / 2) + uniform_dist(rng) * (GUNFIRE_DELAY_MS / 2);
	//		Mix_PlayChannel(-1, gun_fire, 0);
	//		fired = true;
	//	}

		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// TODO A3: HANDLE PEBBLE SPAWN/UPDATES HERE
		// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 3
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		// Processing the soldier state
	assert(ECS::registry<ScreenState>.components.size() <= 1);
	//	auto& screen = ECS::registry<ScreenState>.components[0];

	for (int i = static_cast<int>(ECS::registry<DeathTimer>.components.size()) - 1; i >= 0; --i)
	{
		auto entity = ECS::registry<DeathTimer>.entities[i];
		// Progress timer
		auto& counter = ECS::registry<DeathTimer>.get(entity);
		counter.counter_ms -= elapsed_ms;

		// Restart the game once the death timer expired
		if (counter.counter_ms < 0)
		{
			ECS::ContainerInterface::remove_all_components_of(entity);
		}
	}

	aiControl = WorldSystem::isPlayableLevel(currentLevel);
	std::cout << "current level: " << currentLevel << " ai control: " << aiControl << "\n";

	endGameTimer += elapsed_ms;

	buttonHandler(elapsed_ms, window_size_in_game_units);
	runTimer(elapsed_ms);
	checkEndGame();

	// !!! TODO A1: update LightUp timers and remove if time drops below zero, similar to the DeathTimer
}

// Reset the world state to its initial state
void WorldSystem::restart(std::string level)
{
	level_loader.set_level(level);
	currentLevel = level_loader.at_level;
	// Debugging for memory/component leaks
	ECS::ContainerInterface::list_all_components();
	// std::cout << "Restarting\n";

	// WorldSystem::initializeCallbacks();

	// Reset the game speed
	current_speed = 1.f;

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

	auto soliders = ECS::registry<Soldier>.entities;
	if (soliders.size() > 1) {
		throw std::runtime_error("Cannot have more than one solider");
	}
	player_soldier = soliders.front();

	// std::cout << "soldier addr: " << &player_soldier << "\n";


	while (!ECS::registry<Camera>.entities.empty())
		ECS::ContainerInterface::remove_all_components_of(ECS::registry<Camera>.entities.back());
	ECS::Entity camera;
	camera.insert(Camera({ 0,0 }, player_soldier));

	if (level_loader.at_level == "level_1") {
		Start::createStart(vec2{ 300,300 });
		// ButtonStart::createButtonStart(vec2{300,450});

		Button::createButton(ButtonType::START, vec2{ 300,525 });
		// ButtonSetting::createButtonSetting(vec2{300,600});
		Button::createButton(ButtonType::LEVEL_SELECT, vec2{ 300,600 });
	}
	else if (level_loader.at_level == "level_2") {
		Loading::createLoading(vec2{ 100,200 });
	}
	else {
		Background::createBackground(vec2{ 500,500 });
	}
}

void WorldSystem::buttonHandler(float elapsed_ms, vec2 window_size_in_game_units)
{
	bool isSoldierExisting = !ECS::registry<Soldier>.components.empty();
	bool isButtonExisting = !ECS::registry<Button>.components.empty();

	if (isSoldierExisting && isButtonExisting) {
		auto& soldier = ECS::registry<Soldier>.entities[0];
		auto vecOfButtons = ECS::registry<Button>.entities;

		if (soldier.has<Motion>()) {
			auto& soldierMotion = soldier.get<Motion>();
			vec2 soldierPos = soldierMotion.position;
			for (auto button : vecOfButtons) {
				if (button.has<Motion>()) {
					if (button.has<Button>()) {
						auto buttonMotion = button.get<Motion>();
						auto buttonButton = button.get<Button>();

						vec2 buttonPos = buttonMotion.position;
						vec2 buttonScale = buttonMotion.scale;

						vec2 yAxisBorder = vec2{ buttonPos.y + abs(buttonScale.y) / 2 * 1.f, buttonPos.y - abs(buttonScale.y) / 2 * 1.f };
						vec2 xAxisBorder = vec2{ buttonPos.x + abs(buttonScale.x) / 2 * 1.f, buttonPos.x - abs(buttonScale.x) / 2 * 1.f };

						bool xTouch = soldierPos.x < xAxisBorder.x&& soldierPos.x > xAxisBorder.y;
						bool yTouch = soldierPos.y < yAxisBorder.x&& soldierPos.y > yAxisBorder.y;

						bool pressButton = xTouch && yTouch;

						// std::cout << "px: " << soldierPos.x << ", " << soldierPos.y << " buttonPosX: " << xAxisBorder.x << ", " << xAxisBorder.y << "PosY: " << yAxisBorder.x << ", " << yAxisBorder.y << "\n";

						if (pressButton) {
							take_button_action(buttonButton.buttonType);
						}
					}

				}
			}
		}
	}
}

void WorldSystem::take_button_action(ButtonType type) {
	switch (type) {
	case ButtonType::DEFAULT_BUTTON:
		break;
	case ButtonType::START:
		std::cout << "press start\n";
		restart("level_4");
		break;
	case ButtonType::LEVEL_SELECT:
		std::cout << "press level select\n";
		restart("level_3");
		break;
	case ButtonType::QUIT:
		break;
	}
}

bool WorldSystem::isPlayableLevel(std::string level)
{
	return playableLevelMap[level];
}

void WorldSystem::checkEndGame()
{
	if (WorldSystem::isPlayableLevel(currentLevel)) {
		if (endGameTimer > 10000.f) {
			if (ECS::registry<Enemy>.entities.size() <= 0) {
				resetTimer();
				restart("level_1");
			}

			if (ECS::registry<Enemy>.entities.size() > 0) {
				resetTimer();
				restart("level_2");
			}
		}
	}
}

void WorldSystem::runTimer(float elapsed_ms)
{
	if (isPlayableLevel(currentLevel)) {
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


					// !!! TODO A1: change the salmon motion to float down up-side down

					// !!! TODO A1: change the salmon color
				}
			}
			// Checking Soldier - Fish collisions
			else if (ECS::registry<Fish>.has(entity_other))
			{
				if (!ECS::registry<DeathTimer>.has(entity))
				{
					// chew, ai_count points, and set the LightUp timer
					ECS::ContainerInterface::remove_all_components_of(entity_other);
					Mix_PlayChannel(-1, gun_reload, 0);
					++points;

					// !!! TODO A1: create a new struct called LightUp in render_components.hpp and add an instance to the salmon entity
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
	// Move soldier if alive
	if (!ECS::registry<DeathTimer>.has(player_soldier) && player_soldier.has<Motion>())
	{
		if (key == GLFW_KEY_D) {
			player_soldier.get<Motion>().velocity = vec2{ 100,0 } *(float)(action == GLFW_PRESS || action == GLFW_REPEAT);
		}
		else if (key == GLFW_KEY_A) {
			player_soldier.get<Motion>().velocity = vec2{ -100,0 } *(float)(action == GLFW_PRESS || action == GLFW_REPEAT);
		}
		else if (key == GLFW_KEY_S) {
			player_soldier.get<Motion>().velocity = vec2{ 0,100 } *(float)(action == GLFW_PRESS || action == GLFW_REPEAT);
		}
		else if (key == GLFW_KEY_W) {
			player_soldier.get<Motion>().velocity = vec2{ 0,-100 } *(float)(action == GLFW_PRESS || action == GLFW_REPEAT);
		}
	}


	//Shield up
	if (action == GLFW_PRESS && key == GLFW_KEY_S)
	{
		auto& motion = ECS::registry<Motion>.get(player_soldier);
		auto bullet = Bullet::createBullet(player_soldier.get<Motion>().position, motion.angle);
		//        auto& motionBu = bullet.get<Motion>();
		//        motionBu.angle = motion.angle;
	}

	// Resetting game
	if (action == GLFW_RELEASE && key == GLFW_KEY_R)
	{
		int w, h;
		glfwGetWindowSize(window, &w, &h);

		restart(currentLevel);
	}

	if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
		// level_loader.set_level("level_1");
		// level_loader.at_level = "level_1";
		restart("level_1");
	}

	if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
		// level_loader.set_level("level_2");
		// level_loader.at_level = "level_2";
		restart("level_2");
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
		restart("level_5");
	}

	// Debugging
	if (key == GLFW_KEY_D)
		DebugSystem::in_debug_mode = (action != GLFW_RELEASE);

	// Debugging
	if (key == GLFW_KEY_P)
		DebugSystem::in_profile_mode = (action != GLFW_RELEASE);

	// Control the current speed with `<` `>`
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_COMMA)
	{
		current_speed -= 0.1f;
		// std::cout << "Current speed = " << current_speed << std::endl;
	}
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_PERIOD)
	{
		current_speed += 0.1f;
		// std::cout << "Current speed = " << current_speed << std::endl;
	}
	current_speed = std::max(0.f, current_speed);
}

void WorldSystem::on_mouse(int key, int action, int mod) {
	if (!aiControl) {
		if (action == GLFW_PRESS && key == GLFW_MOUSE_BUTTON_LEFT)
		{
			DRAWING = true;
			mouse_points.clear();
		}
		else if (action == GLFW_RELEASE && key == GLFW_MOUSE_BUTTON_LEFT)
		{
			DRAWING = false;
			if (checkCircle(player_soldier))
			{
				SHIELDUP = true;
			}
		}
	}
}

void WorldSystem::on_mouse_move(vec2 mouse_pos)
{
	if (!aiControl) {
		if (!ECS::registry<DeathTimer>.has(player_soldier))
		{
			auto& motion = ECS::registry<Motion>.get(player_soldier);
			// Get world mouse position
			if (!ECS::registry<Camera>.entities.empty()) {
				auto& camera = ECS::registry<Camera>.entities[0];
				if (camera.has<Camera>()) {
					vec2 camera_pos = camera.get<Camera>().get_position();
					mouse_pos += camera_pos;
				}
			}
			float disY = mouse_pos.y - motion.position.y;
			float disX = mouse_pos.x - motion.position.x;
			float longestL = sqrt(pow(disY, 2) + pow(disX, 2));

			float sinV = asin(disY / longestL);
			float cosV = acos(disX / longestL);
			auto dir = mouse_pos - motion.position;
			// printf("%f,%f\n",mouse_pos.x, mouse_pos.y);
			float rad = atan2(dir.y, dir.x);
			motion.angle = rad;

			if (SHIELDUP && !hasShield) {
				shield = Shield::createShield({ motion.position.x + 300 * cosV, motion.position.y + 300 * sinV });
				hasShield = true;
			}

			if (SHIELDUP) {
				auto& motionSh = ECS::registry<Motion>.get(shield);
				motionSh.position = vec2(motion.position.x + disX / 2, motion.position.y + disY / 2);
				motionSh.angle = rad;
			}
			if (DRAWING) {
				if (mouse_points.size() >= MOUSE_POINTS_COUNT) {
					mouse_points.pop_front();
				}
				mouse_points.push_back(mouse_pos - motion.position);
			}
		}
	}
}
