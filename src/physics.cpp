// internal
#include "physics.hpp"
#include "tiny_ecs.hpp"
#include "debug.hpp"
#include "float.h"
#include "PhysicsObject.hpp"
#include "Bullet.hpp"
#include "Enemy.hpp"
#include "Weapon.hpp"
#include "Wall.hpp"
#include <soldier.hpp>
#include <iostream>

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
	float dist_squared = dot(dp, dp);
	float other_r = std::sqrt(std::pow(get_bounding_box(motion1).x / 2.0f, 2.f) + std::pow(get_bounding_box(motion1).y / 2.0f, 2.f));
	float my_r = std::sqrt(std::pow(get_bounding_box(motion2).x / 2.0f, 2.f) + std::pow(get_bounding_box(motion2).y / 2.0f, 2.f));
	float r = (other_r + my_r);
	if (dist_squared < r * r)
		return true;
	return false;
}


CollisionType PhysicsSystem::advanced_collision(ECS::Entity e1, ECS::Entity e2) {
	if (!e1.has<Motion>() || !e2.has<Motion>() || !e1.has<PhysicsObject>() || !e2.has<PhysicsObject>()
		|| PhysicsObject::getCollisionType(e1.get<PhysicsObject>().object_type, e2.get<PhysicsObject>().object_type) == NoCollision) {
		return NoCollision;
	}

	auto& m1 = e1.get<Motion>();
	auto& m2 = e2.get<Motion>();
	auto& p1 = e1.get<PhysicsObject>();
	auto& p2 = e2.get<PhysicsObject>();
	//    if(p1.object_type != PLAYER){
	//        return false;
	//    }
	CollisionResult c1 = collision(e1, e2);
	CollisionResult c2 = collision(e2, e1);
	// Multiplier for reverse collision
	float mul = 1;

	if (c1.penitration == 0 || c1.normal == vec2{ 0,0 }) {
		c1 = c2;
		mul = -1;
	}
	bool ret = c1.penitration != 0 && c1.normal != vec2{ 0,0 };
	if (!ret) {
		return NoCollision;
	}
	// If both collision is on
	if (ret && p1.collide && p2.collide && PhysicsObject::getCollisionType(p1.object_type, p2.object_type) == Hit) {


        float delta_p1 = p2.fixed ? c1.penitration : c1.penitration * p1.mass / (p2.mass + p1.mass);
        float delta_p2 = p1.fixed ? c1.penitration : c1.penitration * p2.mass / (p2.mass + p1.mass);
        if( !e1.get<Motion>().has_parent) {
            m1.position += p1.fixed ? vec2{0, 0} : delta_p1 * c1.normal * mul;
        } else{
            m1.offset_move += p1.fixed ? vec2{0, 0} : delta_p1 * c1.normal * mul;
        } if(!e2.get<Motion>().has_parent) {
            m2.position -= p2.fixed ? vec2{0, 0} : delta_p2 * c1.normal * mul;
        } else {
            m2.offset_move -= p2.fixed ? vec2{0, 0} : delta_p2 * c1.normal * mul;
        };

	}
	// If two objects overlap / hit
	auto result = PhysicsObject::getCollisionType(p1.object_type, p2.object_type);
    if (result != NoCollision) {
        p1.physicsEvent(result, e1, e2, c1);
        // The vector may resize, re-obtain reference here.
        auto& p2_a = ECS::registry<PhysicsObject>.get(e2);
        p2_a.physicsEvent(result, e2, e1, c1);
    }
	return result;
}


CollisionResult PhysicsSystem::collision(ECS::Entity e1, ECS::Entity e2) {
	if (!e1.has<Motion>() || !e2.has<Motion>() || !e1.has<PhysicsObject>() || !e2.has<PhysicsObject>()) {
		return { 0, {0,0}, {0,0} };
	}
	auto& m1 = e1.get<Motion>();
	auto& m2 = e2.get<Motion>();
	auto& p1 = e1.get<PhysicsObject>();
	auto& p2 = e2.get<PhysicsObject>();
	float best_distance = 0;
	vec2 final_normal = { 0,0 };
	vec2 final_n_l = { 0,0 };
	vec2 v = { 0,0 };
	Transform t1 = getTransform(m1);


	Transform t2 = getTransform(m2);
	vec2 delta_x = m2.position - m1.position;


	for (auto& j : p2.vertex) {

		vec3 vertex2_1 = { j.position.x, j.position.y, 1.f };
		// Calculate the vertex in global coordinates
		vec3 global2_v1 = t2.mat * vertex2_1;

		vec2 v2_1 = { global2_v1.x, global2_v1.y };
		//printf("%f, %f\n", v2_1.x , v2_1.y);
		bool x = true;
		float dist = -FLT_MAX;
		vec2 n = { 0,0 };
		vec2 n_l = { 0,0 };

		// Examine if the vertex is inside the other object
		for (unsigned int i = 0; i < p1.faces.size(); ++i) {
			auto edge = p1.faces[i];
			vec3 vertex1_1 = { p1.vertex[edge.first].position.x, p1.vertex[edge.first].position.y, 1.f };
			vec3 vertex1_2 = { p1.vertex[edge.second].position.x, p1.vertex[edge.second].position.y, 1.f };
			vec3 global1_v1 = t1.mat * vertex1_1;
			vec3 global1_v2 = t1.mat * vertex1_2;
			vec2 v1_1 = { global1_v1.x, global1_v1.y };
			vec2 v1_2 = { global1_v2.x, global1_v2.y };
			// Normal of the face
			vec3 normal_t = { vertex1_1.y - vertex1_2.y, vertex1_2.x - vertex1_1.x, 0 };
			// Translate the normal with the matrix in global coordinates
			normal_t = inverse(transpose(t1.mat)) * normal_t;

			normal_t /= sqrt(normal_t.x * normal_t.x + normal_t.y * normal_t.y); // Normalize
			// Normal in local coordinates. I removed it from the final result. So not really useful.
			vec2 local_n = { vertex1_1.y - vertex1_2.y, vertex1_2.x - vertex1_1.x };

			// Convert to vec2
			vec2 normal1 = { normal_t.x, normal_t.y };
			// If the projection >0 then the point is outside the bonding box
			if (dot(v2_1 - v1_1, normal1) > 0) {
				x = false;
				break;
			}
			else if (dot(delta_x, normal1) > 0 && dot(v2_1 - v1_1, normal1) > dist) {
				// get smallest penitration from a edge
				dist = dot(v2_1 - v1_1, normal1);
				n_l = local_n;
				n = normal1;
			}
		}
		// Get smallest penitration from all the vertex // Not really sure about this
		if (x && dist < best_distance) {
			best_distance = dist;
			final_normal = n;
			final_n_l = n_l;
			v = v2_1;
		}
	}

	if (best_distance != 0 && final_normal != vec2{ 0,0 }) {
		return { best_distance, final_normal, v };
	}
	// No collision
	return { 0, {0,0}, {0,0} };
}




void PhysicsSystem::step(float elapsed_ms, vec2 window_size_in_game_units)
{
    float step_seconds = 1.0f * (elapsed_ms / 1000.f);
    for(auto& entity: ECS::registry<AIPath>.entities){
        if(entity.has<Motion>()){

            auto& motion = entity.get<Motion>();
            auto& aiPath = entity.get<AIPath>();
            if (!aiPath.active){
                continue;
            }
            if (aiPath.path.path.size() > 1) {
                auto target = aiPath.path.path[1];
                auto target_position = AISystem::get_grid_location(target);
                auto dir =  target_position - motion.position;
                // Enemy will always face the player
                motion.angle = atan2(dir.y, dir.x);
                aiPath.desired_speed = { 100.f, 0.f };
            }
            else {
                aiPath.desired_speed = { 0.f, 0.f };
            }
            motion.velocity -= (motion.velocity - aiPath.desired_speed) * elapsed_ms / 1000.f;
        }
    }

	// Move entities based on how much time has passed, this is to (partially) avoid
	// having entities move at different speed based on the machine.
    for(auto& entity: ECS::registry<ParentEntity>.entities){
        auto& parent = entity.get<ParentEntity>();
        if(entity.has<PhysicsObject>() && parent.parent.has<PhysicsObject>()){
            auto& physicsObject = entity.get<PhysicsObject>();
            auto& parent_physics = parent.parent.get<PhysicsObject>();
            parent_physics.force.insert( parent_physics.force.end(),physicsObject.force.begin(), physicsObject.force.end());
            physicsObject.force.clear();
        }
    }

    for (auto& entity : ECS::registry<PhysicsObject>.entities){
        auto& physics = entity.get<PhysicsObject>();
        if(physics.fixed) {
            physics.force.clear();
            continue;
        }
        auto& motion = entity.get<Motion>();
        for(auto& f: physics.force) {
            motion.preserve_world_velocity += f.force / physics.mass;
            vec2 rad = f.position - motion.position;
            float radius = static_cast<float>(sqrt(dot(rad,rad)));
            float I = physics.mass/2.0f * static_cast<float>(pow(max(motion.scale.x, motion.scale.y), 2));
            vec2 rotate_force = f.force - dot(f.force, rad) * rad / radius/ radius;
            float mul = rotate_force.x * rad.y - rotate_force.y * rad.x < 0? 1: -1;
            float alpha = sqrt(dot(rotate_force, rotate_force)) * radius/ I * mul;

            motion.angular_velocity += alpha;

        }
        physics.force.clear();
    }

    for(auto& player: ECS::registry<Soldier>.entities){
        player.get<Motion>().preserve_world_velocity*= 0.9;
        player.get<Motion>().angular_velocity*= 0.9;
    }



	for (auto& motion : ECS::registry<Motion>.components)
	{
		if (!motion.has_parent) {
			vec2 v = get_world_velocity(motion);
			motion.position += v * step_seconds;
			motion.angle += motion.angular_velocity * step_seconds;
			motion.angular_velocity *= pow(0.9, step_seconds);
            motion.preserve_world_velocity *= pow(0.9, step_seconds);
		}
		else {
			if (motion.parent.has<Motion>()) {
				Transform t1{};
				t1.rotate(motion.parent.get<Motion>().angle);
				vec3 world_translate = t1.mat * vec3{ motion.offset, 0.f };
                motion.parent.get<Motion>().position += motion.offset_move;
                motion.offset_move = vec2 {0.f,0.f};
                vec2 old_position = motion.position;
				motion.position = motion.parent.get<Motion>().position + vec2{ world_translate };
				motion.angle = motion.offset_angle + motion.parent.get<Motion>().angle;
				motion.velocity = motion.parent.get<Motion>().velocity;
				// Remove this because rotate will cause lots of force
				// motion.velocity = get_local_velocity((motion.position - old_position) / step_seconds, motion);

				// motion.preserve_world_velocity = vec2{0,0};
			}
		}
	}

	// Visualization for debugging the position and scale of objects
	if (DebugSystem::in_debug_mode)
	{
		for (int i = static_cast<int>(ECS::registry<PhysicsObject>.entities.size() - 1); i >= 0; i--)
		{
			auto e = ECS::registry<PhysicsObject>.entities[i];
			if (!e.has<Motion>()) {
				continue;
			}
			auto& motion = e.get<Motion>();
			// draw a cross at the position of all objects
			auto scale_horizontal_line = motion.scale;
			scale_horizontal_line.y *= 0.1f;
			auto scale_vertical_line = motion.scale;
			scale_vertical_line.x *= 0.1f;
			Transform t{};
			t.translate(motion.position);
			t.rotate(motion.angle);
			t.scale(motion.scale);

			auto p = e.get<PhysicsObject>();
			for (auto& v : p.vertex) {
				vec3 world = t.mat * vec3{ v.position.x, v.position.y, 1.f };
				DebugSystem::createLine(vec2{ world.x, world.y }, vec2{ 10,10 });
			}
		}
        for (auto& m : ECS::registry<Motion>.components) {
            DebugSystem::createLine(m.position, {10.f, 10.f});
        }

		for (auto& e : ECS::registry<AIPath>.components) {

			for (auto& grid : e.path.path) {
				// draw a cross at the position of all objects
				auto scale_vertical_line = vec2{ 10.f, 10.f };
				DebugSystem::createLine(
					{ grid.first * AISystem::GRID_SIZE + AISystem::GRID_SIZE / 2, grid.second * AISystem::GRID_SIZE + AISystem::GRID_SIZE / 2 },
					scale_vertical_line);

			}
		}

	}



	// Check for collisions between all moving entities
	auto& physics_object_container = ECS::registry<PhysicsObject>;
	// for (auto [i, motion_i] : enumerate(motion_container.components)) // in c++ 17 we will be able to do this instead of the next three lines
	unsigned int size = physics_object_container.components.size();
	for (int i = size - 1; i >= 0 ; i--)
	{
		ECS::Entity entity_i = physics_object_container.entities[i];
		auto& motion_i = entity_i.get<Motion>();
		// std::cout << "entity i addr: " << &entity_i << "\n";
		for (int j = i - 1; j >= 0 ; j--)
		{

			ECS::Entity entity_j = physics_object_container.entities[j];
			auto& motion_j = entity_j.get<Motion>();
			if (collides(motion_i, motion_j))
			{
				// Create a collision event
				 // Note, we are abusing the ECS system a bit in that we potentially insert muliple collisions for the same entity, hence, emplace_with_duplicates
				CollisionType result = advanced_collision(entity_i, entity_j);

//                if(entity_j.has<Soldier>() || entity_i.has<Soldier>()){
//                    printf("Soldier collide with %d, %d\n", entity_i.get<PhysicsObject>().object_type, result);
//                }

			}
		}
	}
}

PhysicsSystem::Collision::Collision(ECS::Entity other)
{
	this->other = other;
}

vec2 PhysicsSystem::get_world_velocity(const Motion& motion) {
	float ca = cos(motion.angle);
	float sa = sin(motion.angle);
	vec2 v = { motion.velocity.x * ca - motion.velocity.y * sa,  motion.velocity.x * sa + motion.velocity.y * ca };
	return v + motion.preserve_world_velocity;
}

vec2 PhysicsSystem::get_local_velocity(vec2 world_velocity, const Motion& motion) {
	float ca = cos(-motion.angle);
	float sa = sin(-motion.angle);
	vec2 v = { world_velocity.x * ca - world_velocity.y * sa,  world_velocity.x * sa + world_velocity.y * ca };
	return v;
}