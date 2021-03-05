//
// Created by Gary on 2/14/2021.
//

#include "PhysicsObject.hpp"

std::map<CollisionObjectType, std::set<CollisionObjectType>> PhysicsObject::ignore_collision_of_type{
    {WALL,{WALL}},
        {BULLET,{DEFAULT,
                 PLAYER,
                 BULLET,
                WEAPON},
         },
    {PLAYER, {BULLET}},
     {MOVEABLEWALL, {BULLET}},
};
std::map<CollisionObjectType, std::set<CollisionObjectType>> PhysicsObject::only_overlap_of_type{
    {BULLET,{DEFAULT,
                    // PLAYER,
                    ENEMY,
                    BULLET,


                    WEAPON}},
    {ENEMY,{BULLET}},
    {BUTTON, {PLAYER}},
    {PLAYER, {BUTTON}},
    };


