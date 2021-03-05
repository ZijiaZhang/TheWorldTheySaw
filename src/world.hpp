#pragma once

// internal
#include "common.hpp"
#include "soldier.hpp"
#include "Enemy.hpp"
#include "Camera.hpp"
#include "button.hpp"

// stlib
#include <vector>
#include <random>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>
#include <levelLoader.hpp>
#include <button.hpp>

// Container for all our entities and game logic. Individual rendering / update is 
// deferred to the relative update() methods
class WorldSystem
{
public:
	// Creates a window
	WorldSystem(ivec2 window_size_px);

	// Releases all associated resources
	~WorldSystem();

	// restart level
	void restart(std::string level);

	// Steps the game ahead by ms milliseconds
	void step(float elapsed_ms, vec2 window_size_in_game_units);



	// Check for collisions
	void handle_collisions();


	// Renders our scene
	void draw();

	// Should the game be over ?
	bool is_over() const;

	std::string currentLevel;

	// OpenGL window handle
	GLFWwindow* window;

	static std::map<ButtonType, std::function<void()>> buttonCallbacks;
    static bool reload_level;
    static std::string level_name;
	// LevelLoader level_loader;
private:
	// Input callback functions
	void on_key(int key, int, int action, int mod);
	void on_mouse_move(vec2 mouse_pos);

	void initializeCallbacks();

	// Loads the audio
	void init_audio();

	// Number of fish eaten by the salmon, displayed in the window title
	unsigned int points;

	// Game state
	float current_speed;
	float next_turtle_spawn;
	float next_fish_spawn;
	float next_gunfire_spawn;
	ECS::Entity player_soldier;
	ECS::Entity shield;

	// music references
	Mix_Music* background_music;
	Mix_Chunk* gun_fire;
	Mix_Chunk* gun_reload;

	// C++ random number generator
	std::default_random_engine rng;
	std::uniform_real_distribution<float> uniform_dist; // number between 0..1
	vec2 screen;

    void on_mouse(int key, int action, int mod);
};
