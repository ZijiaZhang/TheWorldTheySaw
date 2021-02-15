// internal
#include "ai.hpp"
#include "Enemy.hpp"
#include "soldier.hpp"
#include "debug.hpp"

#include <set>
#include <queue>
const int GRID_SIZE = 100;

std::map<CollisionObjectType, std::set<std::pair<int,int>>> AISystem::occupied_grids_Enemy;

void AISystem::step(float elapsed_ms, vec2 window_size_in_game_units)
{

	(void)elapsed_ms; // placeholder to silence unused warning until implemented
	(void)window_size_in_game_units; // placeholder to silence unused warning until implemented

	for (auto& e: ECS::registry<Enemy>.entities){
	    enemy_ai_step(e, elapsed_ms);
	}


}

void AISystem::enemy_ai_step(ECS::Entity e, float elapsed_ms) {
    for (auto& enemy: ECS::registry<Enemy>.entities){
        auto& enemy_motion = ECS::registry<Motion>.get(enemy);
        if(!ECS::registry<Soldier>.components.empty()) {
            auto soldier = ECS::registry<Soldier>.entities[0];
            auto& soldier_motion = soldier.get<Motion>();
            if(soldier.has<Motion>()) {
                // Get delta position
//                auto dir = enemy_motion.position - soldier_motion.position;
//                // Enemy will always face the player
//                enemy_motion.angle = atan2(dir.y, dir.x);
//                //Let enemy stop moving if the distance is close
//                vec2 desired_speed = {dot(dir, dir) > 160000? enemy_motion.max_control_speed : 0.f, 0.f};
//                enemy_motion.velocity -= (enemy_motion.velocity - desired_speed) * elapsed_ms / 1000.f;
//                enemy_motion.velocity.x = max(-enemy_motion.max_control_speed, min(enemy_motion.max_control_speed, enemy_motion.velocity.x));
//                enemy_motion.velocity.y = max(-enemy_motion.max_control_speed, min(enemy_motion.max_control_speed, enemy_motion.velocity.y));
                build_grids_for_type<PLAYER>();
                build_grids_for_type<WALL>();

                printf("p: %f, %f \n", soldier_motion.position.x, soldier_motion.position.y);
                if (abs(soldier_motion.position.x) > 10000 || abs(soldier_motion.position.y) > 10000){
                    return;
                }
                auto path = find_path_to_location(enemy, soldier_motion.position, 200.f);

                for (auto &grid : path.path) {
                    // draw a cross at the position of all objects
                    auto scale_vertical_line = vec2{10.f, 10.f};
                    DebugSystem::createLine(
                            {grid.first * GRID_SIZE + GRID_SIZE / 2, grid.second * GRID_SIZE + GRID_SIZE / 2},
                            scale_vertical_line);
                }
            }
        }
    }
}


Path_with_heuristics AISystem::find_path_to_location(const ECS::Entity& agent, vec2 position, float radius){
    auto& agent_motion = agent.get<Motion>();
    auto& agent_physics = agent.get<PhysicsObject>();
    std::set<std::pair<int,int>> collisions;
    for (int x = DEFAULT; x!=LAST; x++){
        auto c = static_cast<CollisionObjectType>(x);
        if (PhysicsObject::ignore_collision_of_type[agent_physics.object_type].find(c) == PhysicsObject::ignore_collision_of_type[agent_physics.object_type].end()) {
            collisions.insert(occupied_grids_Enemy[c].begin(), occupied_grids_Enemy[c].end());
        }
    }

    auto comp = []( Path_with_heuristics a, Path_with_heuristics b ) { return a.cost + a.heuristic > b.cost + b.heuristic; };
    std::priority_queue<Path_with_heuristics, std::vector<Path_with_heuristics>, decltype( comp )> front(comp);
    std::pair<int,int> cur_grid = get_grid_from_loc(agent_motion.position);
    std::pair<int,int> dest_grid = get_grid_from_loc(position);

    front.push(Path_with_heuristics{std::vector<std::pair<int,int>>{cur_grid}, 0.f,
                                    get_dist(cur_grid, dest_grid)});
    std::pair<int, int> neighbors[]{ {0,1}, {0, -1}, {1,0}, {-1,0}};
    std::pair<int, int> neighbors2[]{ {-1,-1}, {1, -1}, {-1,1}, {1,1}};

    if (DebugSystem::in_debug_mode)
    {
        for (auto& cur_grid : collisions)
        {
            // draw a cross at the position of all objects
            auto scale_horizontal_line = vec2{GRID_SIZE, 10.f};
            auto scale_vertical_line = vec2{10.f, GRID_SIZE};
            DebugSystem::createLine(vec2{cur_grid.first * GRID_SIZE, cur_grid.second * GRID_SIZE + GRID_SIZE/ 2}, scale_vertical_line);
            DebugSystem::createLine(vec2{(cur_grid.first +1) * GRID_SIZE, cur_grid.second * GRID_SIZE + GRID_SIZE/ 2}, scale_vertical_line);
            DebugSystem::createLine(vec2{cur_grid.first * GRID_SIZE + GRID_SIZE/ 2, (cur_grid.second + 1) * GRID_SIZE }, scale_horizontal_line);
            DebugSystem::createLine(vec2{cur_grid.first * GRID_SIZE + GRID_SIZE/ 2, cur_grid.second * GRID_SIZE}, scale_horizontal_line);
        }
    }
    while(!front.empty()) {
        auto path = front.top();
        front.pop();
//        if (path.path.size() > 10){
//            continue;
//        }
        auto last_node = path.path.back();
        if (get_dist(last_node, dest_grid) <= radius){
            return path;
        }
        for (auto n: neighbors){
            std::pair<int, int> next_node {last_node.first + n.first, last_node.second + n.second};
            if (collisions.find(next_node) == collisions.end()){
                auto v = path.path;
                v.emplace_back(next_node);
                front.push(Path_with_heuristics{std::move(v), path.cost + GRID_SIZE, get_dist(next_node, dest_grid)});
            }
        }
        for (auto n: neighbors2){
            std::pair<int, int> next_node {last_node.first + n.first, last_node.second + n.second};
            if (collisions.find(next_node) == collisions.end()){
                auto v = path.path;
                v.emplace_back(next_node);
                front.push(Path_with_heuristics{std::move(v), static_cast<float>(path.cost + GRID_SIZE * sqrt(2)), get_dist(next_node, dest_grid)});
            }
        }
    }


    return Path_with_heuristics{};
}

float AISystem::get_dist(const std::pair<int, int> &cur_grid,
                         const std::pair<int, int> &dest_grid) const {
    return static_cast<float>(sqrt(pow(cur_grid.first - dest_grid.first, 2) + pow(cur_grid.second - dest_grid.second, 2)) * GRID_SIZE); }

template <CollisionObjectType T>
void AISystem::build_grids_for_type(){
    AISystem::occupied_grids_Enemy[T].clear();
    for(auto& object: ECS::registry<PhysicsObject>.entities) {
        auto &motion = object.get<Motion>();
        auto &physics_object = object.get<PhysicsObject>();
        if (physics_object.object_type == T) {
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
                AISystem::occupied_grids_Enemy[T].insert(std::pair<int, int>(x - 1, int(floor(y / GRID_SIZE))));
                AISystem::occupied_grids_Enemy[T].insert(std::pair<int, int>(x, int(floor(y / GRID_SIZE))));
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
                AISystem::occupied_grids_Enemy[T].insert(std::pair<int, int>(int(floor(x / GRID_SIZE)), y - 1 ));
                AISystem::occupied_grids_Enemy[T].insert(std::pair<int, int>(int(floor(x / GRID_SIZE)), y ));
            }
        }

    }
}

std::pair<int, int> AISystem::get_grid_from_loc(vec2 vec) {
    return std::pair<int, int>(floor(vec.x / GRID_SIZE), floor(vec.y / GRID_SIZE));
}




