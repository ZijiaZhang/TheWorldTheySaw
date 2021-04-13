
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
#include "ai.hpp"
#include "debug.hpp"
#include <SoldierAi.hpp>
#include <EnemyAi.hpp>

using Clock = std::chrono::high_resolution_clock;
int ai_count = 0;
const ivec2 window_size_in_px = {1200, 800};
const vec2 window_size_in_game_units = { 1200, 800 };
// Note, here the window will show a width x height part of the game world, measured in px. 
// You could also define a window to show 1.5 x 1 part of your game world, where the aspect ratio depends on your window size.

struct Description {
	std::string name;
	Description(const char* str) : name(str) {};
};

// Entry point
int main()
{
	// Initialize the main systems
	WorldSystem world(window_size_in_px);
	RenderSystem renderer(*world.window);
	PhysicsSystem physics;
	AISystem ai;
	SoldierAISystem soldierAi;
	EnemyAISystem enemyAi;

	world.screen = window_size_in_game_units;
	// Set all states to default
	world.restart("menu");
	auto t = Clock::now();
	// Variable timestep loop
	while (!world.is_over())
	{
	    if(WorldSystem::reload_level){
	        WorldSystem::reload_level = false;
	        world.restart(WorldSystem::reload_level_name);
	    }
	    ai_count++;
		// Processes system messages, if this wasn't present the window would become unresponsive
		glfwPollEvents();

		// Calculating elapsed times in milliseconds from the previous iteration
		auto now = Clock::now();
		float elapsed_ms = static_cast<float>((std::chrono::duration_cast<std::chrono::microseconds>(now - t)).count()) / 1000.f;
		elapsed_ms *= GameInstance::get_current_speed();
		t = now;

		DebugSystem::clearDebugComponents();
		auto debug_time = Clock::now();
		if(DebugSystem::in_profile_mode)
		    printf("Debug: %f\n", static_cast<float>((std::chrono::duration_cast<std::chrono::microseconds>(debug_time - t)).count()) / 1000.f);

		
		if (world.aiControl) {
		    ai.build_grid();
			soldierAi.step(elapsed_ms, window_size_in_game_units);
			enemyAi.step(elapsed_ms, window_size_in_game_units);
		}

		auto ai_time = Clock::now();
        if(DebugSystem::in_profile_mode)
            printf("AI: %f\n", static_cast<float>((std::chrono::duration_cast<std::chrono::microseconds>(ai_time - debug_time)).count()) / 1000.f);
		world.step(elapsed_ms, window_size_in_game_units);
        auto world_time = Clock::now();
        if(DebugSystem::in_profile_mode)
            printf("World: %f\n", static_cast<float>((std::chrono::duration_cast<std::chrono::microseconds>(world_time - ai_time)).count()) / 1000.f);
		physics.step(elapsed_ms, window_size_in_game_units);
        auto physics_time = Clock::now();
        if(DebugSystem::in_profile_mode)
            printf("Physics: %f\n", static_cast<float>((std::chrono::duration_cast<std::chrono::microseconds>(physics_time - world_time)).count()) / 1000.f);
		world.handle_collisions();

		renderer.draw(window_size_in_game_units);
	}

	return EXIT_SUCCESS;
}
