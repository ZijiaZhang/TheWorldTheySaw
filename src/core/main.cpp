
#define GL3W_IMPLEMENTATION
#include <gl3w.h>

// stlib
#include <chrono>
#include <iostream>

// internal
#include "common.hpp"
#include "world.hpp"
#include "tiny_ecs.hpp"
#include "render.hpp"
#include "physics.hpp"
#include "debug.hpp"
#include "GameInstance.hpp"

#if defined(_WIN32)
#include <windows.h>
#endif

using Clock = std::chrono::high_resolution_clock;

// Template defaults — adjust the window size and starting level for your game.
const ivec2 window_size_in_px = {1200, 800};
const vec2 window_size_in_game_units = { 1200, 800 };
const std::string start_level = "template";

// Entry point
int main()
{
#if defined(_WIN32) && defined(NDEBUG)
	HWND console_window = GetConsoleWindow();
	if (console_window != nullptr) {
		ShowWindow(console_window, SW_HIDE);
	}
#endif
	// Initialize the main systems
	WorldSystem world(window_size_in_px);
	RenderSystem renderer(*world.window);
	PhysicsSystem physics;

	world.screen = window_size_in_game_units;
	// Set all states to default
	world.restart(start_level);
	auto t = Clock::now();
	// Variable timestep loop
	while (!world.is_over())
	{
	    if(WorldSystem::reload_level){
	        WorldSystem::reload_level = false;
	        world.restart(WorldSystem::reload_level_name);
	    }
		// Processes system messages, if this wasn't present the window would become unresponsive
		glfwPollEvents();

		// Calculating elapsed times in milliseconds from the previous iteration
		auto now = Clock::now();
		float elapsed_ms = static_cast<float>((std::chrono::duration_cast<std::chrono::microseconds>(now - t)).count()) / 1000.f;
		GameInstance::frame_time = elapsed_ms;
		elapsed_ms *= GameInstance::get_current_speed();
		GameInstance::game_time = elapsed_ms;
		t = now;

		DebugSystem::clearDebugComponents();

		world.step(elapsed_ms, window_size_in_game_units);
		physics.step(elapsed_ms, window_size_in_game_units);
		world.handle_collisions();

		renderer.draw(window_size_in_game_units);
	}

	return EXIT_SUCCESS;
}
