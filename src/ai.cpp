// internal
#include "ai.hpp"
#include "tiny_ecs.hpp"
#include "Enemy.hpp"
#include "soldier.hpp"

void AISystem::step(float elapsed_ms, vec2 window_size_in_game_units)
{

	(void)elapsed_ms; // placeholder to silence unused warning until implemented
	(void)window_size_in_game_units; // placeholder to silence unused warning until implemented

	for (auto& e: ECS::registry<Enemy>.entities){
	    enemy_ai_step(e, elapsed_ms);
	}


}

void AISystem::enemy_ai_step(ECS::Entity e, float elapsed_ms) {
    auto& motion = ECS::registry<Motion>.get(e);
    for (auto& soldier: ECS::registry<Soldier>.entities){
        auto& soldier_motion = ECS::registry<Motion>.get(soldier);
        // Get delta position
        auto dir = soldier_motion.position - motion.position;
        // Enemy will always face the player
        motion.angle = atan2(dir.y, dir.x);
        //Let enemy stop moving if the distance is close
        vec2 desired_speed = {dot(dir, dir) > 160000? motion.max_control_speed : 0.f, 0.f};
        motion.velocity -= (motion.velocity - desired_speed) * elapsed_ms / 1000.f;
        motion.velocity.x = max(-motion.max_control_speed, min(motion.max_control_speed, motion.velocity.x));
        motion.velocity.y = max(-motion.max_control_speed, min(motion.max_control_speed, motion.velocity.y));
    }
}


