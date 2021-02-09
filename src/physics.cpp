// internal
#include "physics.hpp"
#include "tiny_ecs.hpp"
#include "debug.hpp"

// Returns the local bounding coordinates scaled by the current size of the entity 
vec2 get_bounding_box(const Motion& motion)
{
	// fabs is to avoid negative scale due to the facing direction.
	return { abs(motion.scale.x), abs(motion.scale.y) };
}

// This is a SUPER APPROXIMATE check that puts a circle around the bounding boxes and sees
// if the center point of either object is inside the other's bounding-box-circle. You don't
// need to try to use this technique.
bool collides(const Motion& motion1, const Motion& motion2)
{
	auto dp = motion1.position - motion2.position;
	float dist_squared = dot(dp,dp);
	float other_r = std::sqrt(std::pow(get_bounding_box(motion1).x/2.0f, 2.f) + std::pow(get_bounding_box(motion1).y/2.0f, 2.f));
	float my_r = std::sqrt(std::pow(get_bounding_box(motion2).x/2.0f, 2.f) + std::pow(get_bounding_box(motion2).y/2.0f, 2.f));
	float r = (other_r + my_r);
	if (dist_squared < r * r)
		return true;
	return false;
}


bool PhysicsSystem::advanced_collision(ECS::Entity& e1, ECS::Entity& e2){
    if (!e1.has<Motion>() || !e2.has<Motion>() || !e1.has<PhysicsObject>() || !e2.has<PhysicsObject>()){
        return false;
    }
    auto& m1 = e1.get<Motion>();
    auto& m2 = e2.get<Motion>();
    auto& p1 = e1.get<PhysicsObject>();
    auto& p2 = e2.get<PhysicsObject>();
//    if(p1.id != PLAYER){
//        return false;
//    }
    CollisionResult c1 = collision(e1, e2);
    CollisionResult c2 = collision(e2, e1);
    // Multiplier for reverse collision
    float mul = 1;

    if (c1.penitration == 0 || c1.normal == vec2{0,0}){
        c1 = c2;
        mul = -1;
    }
    bool ret = c1.penitration != 0 && c1.normal != vec2{0,0};
    if (ret){
        printf("collode\n");
        vec2 col_v_1 = c1.normal * dot(get_world_velocity(m1), c1.normal);
        vec2 col_v_2 = c1.normal * dot(get_world_velocity(m2), c1.normal);
// equation from https://courses.lumenlearning.com/boundless-physics/chapter/collisions/#:~:text=If%20two%20particles%20are%20involved,m%201%20)%20v%202%20i%20.
        vec2 delta_v1 = p2.fixed? (-col_v_1 * 2.f):  (-col_v_1 + (p1.mass-p2.mass)/(p1.mass+p2.mass) * col_v_1 + 2 * p2.mass/(p2.mass + p1.mass) * col_v_2);
        vec2 delta_v2 = p1.fixed? (-col_v_2 * 2.f): (-col_v_2 + (p2.mass-p1.mass)/(p1.mass+p2.mass) * col_v_2 + 2 * p1.mass/(p2.mass + p1.mass) * col_v_1);


        float delta_p1 = p2.fixed? c1.penitration :c1.penitration * p1.mass / (p2.mass + p1.mass);
        float delta_p2 = p1.fixed? c1.penitration :c1.penitration * p2.mass / (p2.mass + p1.mass);

        //printf("%f %f <%f,%f>\n", delta_p1, delta_p2, c1.normal.x, c1.normal.y);

        m2.position -= p2.fixed ? vec2{0,0} :delta_p2 * c1.normal * mul;
        m1.position += p1.fixed ? vec2{0,0} : delta_p1 * c1.normal * mul;
        m1.velocity += p1.fixed? vec2{0,0} : get_local_velocity(delta_v1, m1);
        m2.velocity += p2.fixed? vec2{0,0} : get_local_velocity(delta_v2, m2);
    }
    return ret;
}


CollisionResult PhysicsSystem::collision(ECS::Entity& e1, ECS::Entity& e2){
    if (!e1.has<Motion>() || !e2.has<Motion>() || !e1.has<PhysicsObject>() || !e2.has<PhysicsObject>()){
        return {0, {0,0}, {0,0}};
    }
    auto& m1 = e1.get<Motion>();
    auto& m2 = e2.get<Motion>();
    auto& p1 = e1.get<PhysicsObject>();
    auto& p2 = e2.get<PhysicsObject>();
    float best_distance = -FLT_MAX;
    vec2 final_normal = {0,0};
    vec2 final_n_l = {0,0};
    vec2 v = {0,0};
    Transform t1{};
    t1.translate(m1.position);
    t1.rotate(m1.angle);
    t1.scale(m1.scale);

    Transform t2{};
    t2.translate(m2.position);
    t2.rotate(m2.angle);
    t2.scale(m2.scale);

    for(auto & j : p2.vertex){

        vec3 vertex2_1 = {j.position.x, j.position.y, 1.f};
        vec3 global2_v1 = t2.mat * vertex2_1;

        vec2 v2_1 = {global2_v1.x, global2_v1.y};
        //printf("%f, %f\n", v2_1.x , v2_1.y);
        bool x = true;
        float dist = -FLT_MAX;
        vec2 n = {0,0};
        vec2 n_l = {0,0};

        for (unsigned int i=0; i < p1.faces.size(); ++i) {
            auto edge = p1.faces[i];
            vec3 vertex1_1 = {p1.vertex[edge.first].position.x, p1.vertex[edge.first].position.y, 1.f};
            vec3 vertex1_2 = {p1.vertex[edge.second].position.x, p1.vertex[edge.second].position.y, 1.f};
            vec3 global1_v1 = t1.mat * vertex1_1;
            vec3 global1_v2 = t1.mat * vertex1_2;

            vec2 v1_1 = {global1_v1.x, global1_v1.y};
            vec2 v1_2 = {global1_v2.x, global1_v2.y};
            vec3 normal_t = {vertex1_1.y - vertex1_2.y, vertex1_2.x - vertex1_1.x, 0};
            normal_t = inverse(transpose(t1.mat)) * normal_t;
            normal_t /= sqrt(normal_t.x * normal_t.x + normal_t.y * normal_t.y);
            vec2 local_n =  {vertex1_1.y - vertex1_2.y, vertex1_2.x - vertex1_1.x};

            vec2 normal1 = {normal_t.x, normal_t.y};


            //printf("%d \t <%f, %f> \t<%f, %f> \t<%f, %f> \n", i, v1_1.x, v1_1.y,  v1_2.x, v1_2.y, normal1.x, normal1.y);

            if(dot(v2_1-v1_1, normal1) > 0 ){
                x = false;
                break;
            } else if(dot(v2_1-v1_1, normal1) > dist) {
                dist =  dot(v2_1-v1_1, normal1);
                n_l = local_n;
                n = normal1;
            }
        }

        if (x && dist > best_distance){
            best_distance = dist;
            final_normal = n;
            final_n_l = n_l;
            v = v2_1;

        }
    }
    if (best_distance != -FLT_MAX && final_normal != vec2{0,0}){
        return {best_distance, final_normal, v};
    }
    return {0, {0,0}, {0,0}};
}



void PhysicsSystem::step(float elapsed_ms, vec2 window_size_in_game_units)
{
	// Move entities based on how much time has passed, this is to (partially) avoid
	// having entities move at different speed based on the machine.

	for (auto& motion : ECS::registry<Motion>.components)
	{
		float step_seconds = 1.0f * (elapsed_ms / 1000.f);
        vec2 v = get_world_velocity(motion);
        printf("%f,%f\n", v.x, v.y);
        motion.position += v * step_seconds;
	}

	(void)elapsed_ms; // placeholder to silence unused warning until implemented
	(void)window_size_in_game_units;

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A3: HANDLE PEBBLE UPDATES HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 3
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// Visualization for debugging the position and scale of objects
	if (DebugSystem::in_debug_mode)
	{
		for (auto& motion : ECS::registry<Motion>.components)
		{
			// draw a cross at the position of all objects
			auto scale_horizontal_line = motion.scale;
			scale_horizontal_line.y *= 0.1f;
			auto scale_vertical_line = motion.scale;
			scale_vertical_line.x *= 0.1f;
			DebugSystem::createLine(motion.position, scale_horizontal_line);
			DebugSystem::createLine(motion.position, scale_vertical_line);
		}
	}

	// Check for collisions between all moving entities
	auto& motion_container = ECS::registry<Motion>;
	// for (auto [i, motion_i] : enumerate(motion_container.components)) // in c++ 17 we will be able to do this instead of the next three lines
	for (unsigned int i=0; i<motion_container.components.size(); i++)
	{
		Motion& motion_i = motion_container.components[i];
		ECS::Entity entity_i = motion_container.entities[i];
		for (unsigned int j=i+1; j<motion_container.components.size(); j++)
		{
			Motion& motion_j = motion_container.components[j];
			ECS::Entity entity_j = motion_container.entities[j];

			if (collides(motion_i, motion_j))
			{
				// Create a collision event
				// Note, we are abusing the ECS system a bit in that we potentially insert muliple collisions for the same entity, hence, emplace_with_duplicates
				if(advanced_collision(entity_i, entity_j)) {
//                    ECS::registry<Collision>.emplace_with_duplicates(entity_i, entity_j);
//                    ECS::registry<Collision>.emplace_with_duplicates(entity_j, entity_i);
                }
			}
		}
	}
}

PhysicsSystem::Collision::Collision(ECS::Entity& other)
{
	this->other = other;
}

vec2 PhysicsSystem::get_world_velocity(const Motion &motion) {
    float ca = cos(motion.angle);
    float sa = sin(motion.angle);
    vec2 v = {motion.velocity.x * ca - motion.velocity.y * sa,  motion.velocity.x * sa +  motion.velocity.y * ca};
    return v;
}

vec2 PhysicsSystem::get_local_velocity(vec2 world_velocity, const Motion &motion) {
    float ca = cos(-motion.angle);
    float sa = sin(-motion.angle);
    vec2 v = {world_velocity.x * ca - world_velocity.y * sa,  world_velocity.x * sa +  world_velocity.y * ca};
    return v;
}

