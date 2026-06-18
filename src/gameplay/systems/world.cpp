// Header
#include "world.hpp"
#include "physics.hpp"
#include "debug.hpp"
#include "render_components.hpp"
#include "tiny_ecs.hpp"
#include "PhysicsObject.hpp"
#include "button.hpp"
#include "pop_up.hpp"

// stlib
#include <string.h>
#include <cassert>
#include <sstream>
#include <iostream>

// Game configuration
LevelLoader level_loader;

bool WorldSystem::reload_level = false;
std::string WorldSystem::reload_level_name = "template";
bool WorldSystem::selecting = false;
bool WorldSystem::menuClickOverride = false;

WorldSystem::WorldSystem(ivec2 window_size_px)
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
	window = glfwCreateWindow(window_size_px.x, window_size_px.y, "Game Template", nullptr, nullptr);
	if (window == nullptr)
		throw std::runtime_error("Failed to glfwCreateWindow");

	// Setting callbacks to member functions (that's why the redirect is needed)
	glfwSetWindowUserPointer(window, this);

	auto mouse_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse(_0, _1, _2); };
	auto cursor_pos_redirect = [](GLFWwindow* wnd, double _0, double _1) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_move({ _0, _1 }); };
	auto key_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2, int _3) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_key(_0, _1, _2, _3); };

	glfwSetMouseButtonCallback(window, mouse_redirect);
	glfwSetCursorPosCallback(window, cursor_pos_redirect);
	glfwSetKeyCallback(window, key_redirect);

	init_audio();
}

WorldSystem::~WorldSystem() {
	// Destroy music components
	if (background_music != nullptr)
		Mix_FreeMusic(background_music);
	Mix_CloseAudio();

	// Destroy all created components
	ECS::ContainerInterface::clear_all_components();

	// Close the window
	glfwDestroyWindow(window);
}

void WorldSystem::init_audio()
{
	//////////////////////////////////////
	// Open the audio device with SDL. No audio assets ship with the template;
	// load your own with Mix_LoadMUS / Mix_LoadWAV and play them here.
	if (SDL_Init(SDL_INIT_AUDIO) < 0)
		throw std::runtime_error("Failed to initialize SDL Audio");

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1)
		throw std::runtime_error("Failed to open audio device");

	Mix_Volume(-1, static_cast<int>(GameInstance::effect_volume));
	Mix_VolumeMusic(static_cast<int>(GameInstance::volume));
}

// Update our game world
void WorldSystem::step(float elapsed_ms, vec2 window_size_in_game_units)
{
	glfwSetWindowTitle(window, "Game Template");

	if (screen != window_size_in_game_units) {
		screen = window_size_in_game_units;
	}

	assert(ECS::registry<ScreenState>.components.size() <= 1);

	// Age generic lifetime timers and reap expired entities (used by particles, popups, ...)
	for (int i = static_cast<int>(ECS::registry<DeathTimer>.components.size()) - 1; i >= 0; --i)
	{
		auto entity = ECS::registry<DeathTimer>.entities[i];
		auto& counter = ECS::registry<DeathTimer>.get(entity);
		counter.counter_ms -= elapsed_ms;

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
}

// Reset the world state to its initial state
void WorldSystem::restart(std::string level)
{
	current_level = level;
	level_loader.set_level(level);

	// Reset the game speed
	GameInstance::pause_speed = 1.f;
	GameInstance::ability_speed = 1.f;
	GameInstance::global_speed = 1.f;
	GameInstance::popup_speed = 1.f;

	// Remove all entities that we created (all that have a motion)
	while (!ECS::registry<Motion>.entities.empty())
		ECS::ContainerInterface::remove_all_components_of(ECS::registry<Motion>.entities.back());

	while (!ECS::registry<Camera>.entities.empty())
		ECS::ContainerInterface::remove_all_components_of(ECS::registry<Camera>.entities.back());

	Global_Meshes::meshes.clear();

	// Load whatever the level file defines (an empty file spawns nothing)
	if (!level.empty()) {
		level_loader.load_level();
	}

	// A camera is required by the renderer
	ECS::Entity camera;
	camera.insert(Camera({ 0, 0 }));
}

// Dispatch queued collision events
void WorldSystem::handle_collisions()
{
	// Physics dispatches collision callbacks directly; just drain the event queue.
	ECS::registry<PhysicsSystem::Collision>.clear();
}

// Should the game be over ?
bool WorldSystem::is_over() const
{
	return glfwWindowShouldClose(window) > 0;
}

// On key callback
void WorldSystem::on_key(int key, int, int action, int mod)
{
	// Resetting game
	if (action == GLFW_RELEASE && key == GLFW_KEY_R)
	{
		restart(current_level);
	}

	// Debugging
	if (key == GLFW_KEY_O)
		DebugSystem::in_debug_mode = (action != GLFW_RELEASE);

	// Profiling
	if (key == GLFW_KEY_P)
		DebugSystem::in_profile_mode = (action != GLFW_RELEASE);
}

void WorldSystem::on_mouse(int key, int action, int mod)
{
	if (action == GLFW_PRESS && key == GLFW_MOUSE_BUTTON_LEFT)
	{
		// Dismiss the top-most popup if one is open
		if (!ECS::registry<PopUP>.entities.empty()) {
			auto& entity = ECS::registry<PopUP>.entities.back();
			if (!entity.has<DeathTimer>()) {
				entity.emplace<DeathTimer>();
			}
			return;
		}

		tryClickButton(last_mouse_pos);
	}
}

void WorldSystem::on_mouse_move(vec2 mouse_pos)
{
	last_mouse_pos = mouse_pos;
}

vec2 WorldSystem::getWorldMousePosition(vec2 mouse_pos) const {
	if (!ECS::registry<Camera>.entities.empty()) {
		auto& camera = ECS::registry<Camera>.entities[0];
		if (camera.has<Camera>()) {
			mouse_pos += camera.get<Camera>().get_position();
		}
	}
	return mouse_pos;
}

bool WorldSystem::tryClickButton(vec2 mouse_pos) {
	if (ECS::registry<Button>.entities.empty()) {
		return false;
	}

	vec2 world_mouse = getWorldMousePosition(mouse_pos);
	for (auto button_entity : ECS::registry<Button>.entities) {
		if (!button_entity.has<Motion>() || !button_entity.has<PhysicsObject>()) {
			continue;
		}

		auto& motion = button_entity.get<Motion>();
		vec2 half_extent = vec2{ abs(motion.scale.x) * 0.5f, abs(motion.scale.y) * 0.5f };

		bool inside_x = world_mouse.x >= motion.position.x - half_extent.x && world_mouse.x <= motion.position.x + half_extent.x;
		bool inside_y = world_mouse.y >= motion.position.y - half_extent.y && world_mouse.y <= motion.position.y + half_extent.y;

		if (!inside_x || !inside_y) {
			continue;
		}

		bool previous_selecting = selecting;
		selecting = true;
		menuClickOverride = true;
		button_entity.get<PhysicsObject>().physicsEvent(Overlap, button_entity, button_entity, {});
		menuClickOverride = false;
		selecting = previous_selecting;
		return true;
	}

	return false;
}
