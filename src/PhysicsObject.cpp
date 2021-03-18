//
// Created by Gary on 2/14/2021.
//

#include "PhysicsObject.hpp"

std::map<CollisionObjectType, std::set<CollisionObjectType>> PhysicsObject::ignore_collision_of_type{
    {WALL,{WALL, SHIELD}},
        {BULLET,{COLLISION_DEFAULT,
                 BULLET,
                WEAPON},
         },
    {PLAYER, {SHIELD, WEAPON, MAGIC}},
     {MOVEABLEWALL, {SHIELD}},
    {SHIELD, {   COLLISION_DEFAULT,
                     PLAYER,
                     ENEMY,
                     WALL,
                     MOVEABLEWALL,
                     WEAPON,
                     BUTTON,
                     SHIELD}},
    {BUTTON, {BUTTON, SHIELD, WEAPON}},
    {WEAPON, {BULLET, BUTTON, SHIELD, PLAYER, MAGIC}},
    {ENEMY, {SHIELD}},
    {MAGIC,{MAGIC, PLAYER, WEAPON}}
};
std::map<CollisionObjectType, std::set<CollisionObjectType>> PhysicsObject::only_overlap_of_type{
    {BULLET,{COLLISION_DEFAULT,
                    PLAYER,
                    ENEMY,
                    BULLET,
                    SHIELD,
                    WEAPON}},
    {ENEMY,{BULLET}},
    {BUTTON, {PLAYER}},
    {PLAYER, {BUTTON, BULLET}},
    {SHIELD, {BULLET}},
    {MAGIC, {WALL,MOVEABLEWALL}}
    };


