//
// Created by Gary on 2/14/2021.
//

#include "PhysicsObject.hpp"

template<>
std::set<CollisionObjectType> PhysicsObject::ignore_collision_of_type<PLAYER>{};

template<>
std::set<CollisionObjectType> PhysicsObject::ignore_collision_of_type<ENEMY>{};

template<>
std::set<CollisionObjectType> PhysicsObject::ignore_collision_of_type<WALL>{WALL};

