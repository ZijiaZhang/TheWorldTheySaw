//
// Created by Gary on 2/14/2021.
//

#pragma once

#include "common.hpp"
#include "physics.hpp"

class PhysicsObject{
public:
    PhysicsObject() = default;
    // The convec bonding box of a object
    std::vector<PhysicsVertex> vertex = {PhysicsVertex{{-0.5, 0.5, -0.02}},
                                         PhysicsVertex{{0.5, 0.5, -0.02}},
                                         PhysicsVertex{{0.5, -0.5, -0.02}},
                                         PhysicsVertex{{-0.5, -0.5, -0.02}}};

    // The edges of connecting the vertex tha forms a bonding box
    std::vector<std::pair<int,int>> faces = {{0,1}, {1,2 },{2,3 },{3,0 }};
    // The mass of the object
    float mass = 10;
    // Is object fixed in a location
    bool fixed = false;
    // If collision is enabled
    bool collide = true;
    // Object type
    CollisionObjectType object_type = DEFAULT;
    std::vector<Force> force = std::vector<Force>{};
    ECS::Entity parent;

// Which object type is ignored

static std::map<CollisionObjectType, std::set<CollisionObjectType>> ignore_collision_of_type;
static std::map<CollisionObjectType, std::set<CollisionObjectType>> only_overlap_of_type;

static CollisionType getCollisionType(CollisionObjectType c1, CollisionObjectType c2){
    if (PhysicsObject::ignore_collision_of_type[c1].find(c2) != PhysicsObject::ignore_collision_of_type[c1].end()){
        return NoCollision;
    }
    if (PhysicsObject::only_overlap_of_type[c1].find(c2) != PhysicsObject::only_overlap_of_type[c1].end()){
        return Overlap;
    }
    return Hit;
}
};