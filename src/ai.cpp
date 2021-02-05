// internal
#include "ai.hpp"
#include "tiny_ecs.hpp"
#include "Enemy.hpp"
#include "salmon.hpp"

void AISystem::step(float elapsed_ms, vec2 window_size_in_game_units)
{

	(void)elapsed_ms; // placeholder to silence unused warning until implemented
	(void)window_size_in_game_units; // placeholder to silence unused warning until implemented

	for (auto& e: ECS::registry<Enemy>.entities){
	    enemy_ai_step(e);
	}


}

void AISystem::enemy_ai_step(ECS::Entity e) {
    auto& motion = ECS::registry<Motion>.get(e);
    for (auto& salmon: ECS::registry<Salmon>.entities){
        auto& salmon_motion = ECS::registry<Motion>.get(salmon);
        auto dir = salmon_motion.position - motion.position;
        motion.angle = atan2(dir.y, dir.x);
        motion.velocity = dot(dir, dir) < 40000 ? vec2{0,0}: vec2{100,0};
    }
}


