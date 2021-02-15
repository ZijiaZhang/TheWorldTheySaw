// internal
#include "ai.hpp"
#include "Enemy.hpp"
#include "soldier.hpp"
#include "debug.hpp"

#include <set>

const int GRID_SIZE = 20;

template<CollisionObjectType T> std::set<std::pair<int,int>> AISystem::occupied_grids_Enemy;

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
        build_grids_for_type<ENEMY>();
        find_path_to_location(soldier, {0.f,0.f});
    }
}




void AISystem::find_path_to_location(const ECS::Entity& agent,vec2 position){
    auto& agent_physics = agent.get<PhysicsObject>();
    if (DebugSystem::in_debug_mode)
    {
        for (auto& position : AISystem::occupied_grids_Enemy<ENEMY>)
        {
            // draw a cross at the position of all objects
            auto scale_horizontal_line = vec2{GRID_SIZE, 10.f};
            auto scale_vertical_line = vec2{10.f, GRID_SIZE};
            DebugSystem::createLine(vec2{position.first * GRID_SIZE, position.second * GRID_SIZE + GRID_SIZE/ 2}, scale_vertical_line);
            DebugSystem::createLine(vec2{(position.first +1) * GRID_SIZE, position.second * GRID_SIZE + GRID_SIZE/ 2}, scale_vertical_line);
            DebugSystem::createLine(vec2{position.first * GRID_SIZE + GRID_SIZE/ 2, (position.second + 1) * GRID_SIZE }, scale_horizontal_line);
            DebugSystem::createLine(vec2{position.first * GRID_SIZE + GRID_SIZE/ 2, position.second * GRID_SIZE}, scale_horizontal_line);
        }
    }
}

template <CollisionObjectType T>
void AISystem::build_grids_for_type(){
    AISystem::occupied_grids_Enemy<T>.clear();
    for(auto& object: ECS::registry<PhysicsObject>.entities) {
        auto &motion = object.get<Motion>();
        auto &physics_object = object.get<PhysicsObject>();
        if (physics_object.object_type == PLAYER &&
            PhysicsObject::ignore_collision_of_type<T>.find(T) == PhysicsObject::ignore_collision_of_type<T>.end()) {
            add_grids_to_set<T>(motion, physics_object);
        }
    }
}

template <CollisionObjectType T>
void AISystem::add_grids_to_set(const Motion& motion, const PhysicsObject& obj) {
    std::vector<vec2> world_vertex;
    Transform t = getTransform(motion);
    std::transform(obj.vertex.begin(), obj.vertex.end(), std::back_inserter(world_vertex),
                   [t](PhysicsVertex v){vec3 world_pos =  t.mat * vec3{v.position.x,v.position.y, 1}; return vec2{world_pos.x, world_pos.y};});

    for (auto& position : world_vertex)
    {
        // draw a cross at the position of all objects
        auto scale_vertical_line = vec2{10.f, 10.f};
        DebugSystem::createLine(position, scale_vertical_line);
    }

    for (auto& edge : obj.faces){
        auto v1 = world_vertex[edge.first];
        auto v2 = world_vertex[edge.second];
        if (v1.x > v2.x){
            std::swap(v1, v2);
        }
        if (v1.x != v2.x) {

            printf("%f,%f \n", v1.x, v1.y);

            float horizontal_start = v1.x;
            float horizontal_end = v2.x;
            for (int x = ceil(horizontal_start / GRID_SIZE); x <= floor(horizontal_end / GRID_SIZE); x++) {
                float y = v1.y + (float(x) * GRID_SIZE - v1.x) * (v2.y - v1.y) / (v2.x - v1.x);
                // printf("%f, \n", y);
                AISystem::occupied_grids_Enemy<T>.insert(std::pair<int, int>(x - 1, int(floor(y / GRID_SIZE))));
                AISystem::occupied_grids_Enemy<T>.insert(std::pair<int, int>(x, int(floor(y / GRID_SIZE))));
            }
        }

        if (v1.y > v2.y){
            std::swap(v1, v2);
        }
        if (v1.y != v2.y) {
            float vertical_start = v1.y;
            float vertical_end = v2.y;
            for (int y = ceil(vertical_start / GRID_SIZE); y <= floor(vertical_end / GRID_SIZE); y++) {

                float x = v1.x + (float(y) * GRID_SIZE - v1.y) * (v2.x - v1.x) / (v2.y - v1.y);
                AISystem::occupied_grids_Enemy<T>.insert(std::pair<int, int>(int(floor(x / GRID_SIZE)), y - 1 ));
                AISystem::occupied_grids_Enemy<T>.insert(std::pair<int, int>(int(floor(x / GRID_SIZE)), y ));
            }
        }

    }
}




