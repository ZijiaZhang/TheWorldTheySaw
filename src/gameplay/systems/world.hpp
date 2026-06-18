#pragma once

// internal
#include "common.hpp"
#include "Camera.hpp"
#include "GameInstance.hpp"

// stlib
#include <string>
#include <random>
#include <functional>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>
#include <levelLoader.hpp>

// Container for all our entities and game logic. Individual rendering / update is
// deferred to the relative update() methods
class WorldSystem
{
public:
	// Creates a window
	WorldSystem(ivec2 window_size_px);

	// Releases all associated resources
	~WorldSystem();

	// (Re)load a level by name
	void restart(std::string level);

	// Steps the game ahead by ms milliseconds
	void step(float elapsed_ms, vec2 window_size_in_game_units);

	// Dispatch queued collision events
	void handle_collisions();

	// Should the game be over ?
	bool is_over() const;

	// OpenGL window handle
	GLFWwindow* window;

	// Request a deferred level switch from anywhere (e.g. a button callback)
	static bool reload_level;
	static std::string reload_level_name;

	// Set true while a UI click is being dispatched, so button callbacks can gate on it
	static bool selecting;
	static bool menuClickOverride;

	// Optional hook run at the end of every restart() (after the level is loaded). Lets the
	// app (re)populate programmatic scene content — e.g. the lighting demo — so it survives R.
	static std::function<void()> post_restart;

	vec2 screen;

private:
	// Input callback functions
	void on_key(int key, int, int action, int mod);
	void on_mouse(int key, int action, int mod);
	void on_mouse_move(vec2 mouse_pos);

	// Loads the audio device (no bundled audio assets in the template)
	void init_audio();

	vec2 getWorldMousePosition(vec2 mouse_pos) const;
	bool tryClickButton(vec2 mouse_pos);

	// Currently loaded level name (used by the restart hotkey)
	std::string current_level;

	// music references
	Mix_Music* background_music = nullptr;

	// C++ random number generator
	std::default_random_engine rng;

	vec2 last_mouse_pos;
};
