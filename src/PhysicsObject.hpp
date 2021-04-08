//
// Created by Gary on 2/14/2021.
//

#pragma once

#include <utility>

#include "common.hpp"
#include "physics.hpp"

typedef std::function<void(ECS::Entity self, const ECS::Entity other, CollisionResult result)> COLLISION_HANDLER;

class PhysicsObject{
public:
    PhysicsObject() = default;
    // The convec bonding box of a object
    std::vector<PhysicsVertex> vertex = {PhysicsVertex{{-0.5, 0.5, -0.02}},
                                         PhysicsVertex{{0.5, 0.5, -0.02}},
                                         PhysicsVertex{{0.5, -0.5, -0.02}},
                                         PhysicsVertex{{-0.5, -0.5, -0.02}}};
    bool non_convex = false;
    std::string decompose_key = "";
    // The edges of connecting the vertex tha forms a bonding box
    std::vector<std::pair<int,int>> faces = {{0,1}, {1,2 },{2,3 },{3,0 }};
    // The mass of the object
    float mass = 10;
    // Is object fixed in a location
    bool fixed = false;
    // If collision is enabled
    bool collide = true;
    // Object type
    CollisionObjectType object_type = COLLISION_DEFAULT;
    std::vector<Force> force = std::vector<Force>{};
    ECS::Entity parent;
    std::unordered_map<CollisionType, COLLISION_HANDLER> collisionHandler;
public:
    void attach(CollisionType key,  COLLISION_HANDLER callback) {
        this->collisionHandler.try_emplace(key, callback);
    };

    void physicsEvent(CollisionType key, ECS::Entity self, ECS::Entity other_entity, CollisionResult result) {
        if(this->collisionHandler.find(key) != this->collisionHandler.end()) {
            this->collisionHandler[key](self, other_entity, result);
        }
    };

    // Which object type is ignored

    static std::map<CollisionObjectType, std::set<CollisionObjectType>> ignore_collision_of_type;
    static std::map<CollisionObjectType, std::set<CollisionObjectType>> only_overlap_of_type;

    static CollisionType getCollisionType(CollisionObjectType c1, CollisionObjectType c2){
        if (PhysicsObject::ignore_collision_of_type[c1].find(c2) != PhysicsObject::ignore_collision_of_type[c1].end()
        || PhysicsObject::ignore_collision_of_type[c2].find(c1) != PhysicsObject::ignore_collision_of_type[c2].end()){
            return NoCollision;
        }
        if (PhysicsObject::only_overlap_of_type[c1].find(c2) != PhysicsObject::only_overlap_of_type[c1].end()
        ||PhysicsObject::only_overlap_of_type[c2].find(c1) != PhysicsObject::only_overlap_of_type[c2].end()){
            return Overlap;
        }
        return Hit;
    }

    static Force calculate_force(ECS::Entity self, const ECS::Entity other, CollisionResult collision){
        if (!other.has<Motion>() || !self.has<Motion>()) {
            return Force{};
        }
        // Handel collision
        auto& m1 = self.get<Motion>();
        auto& m2 = other.get<Motion>();
        auto& p1 = self.get<PhysicsObject>();
        auto& p2 = other.get<PhysicsObject>();
        vec2 relative_velocity = PhysicsSystem::get_world_velocity(m2) - PhysicsSystem::get_world_velocity(m1);
        float relative_velocity_on_normal = dot(relative_velocity, collision.normal);


        float m1r = p1.fixed ? 0.f : 1.f / p1.mass;
        float m2r = p2.fixed ? 0.f : 1.f / p2.mass;
        vec2 impulse = collision.normal * (-2.f * relative_velocity_on_normal / (m1r + m2r));

        vec2 impact_location = collision.vertex - collision.normal * collision.penitration;
        // DebugSystem::createLine(impact_location, vec2{10,10});
        impulse.x = min(max(impulse.x, -1000.f), 1000.f);
        impulse.y = min(max(impulse.y, -1000.f), 1000.f);
        Force f = Force{ -1.f * impulse, impact_location };
        
        return f;
    }

    static Force handle_collision(ECS::Entity self, const ECS::Entity other, CollisionResult collision){
        auto f = calculate_force(self, other, collision);
        auto& p1 = self.get<PhysicsObject>();
        p1.force.push_back(f);
        return f;
    }
    void add_force(Force f){
        force.push_back(f);
    }
};