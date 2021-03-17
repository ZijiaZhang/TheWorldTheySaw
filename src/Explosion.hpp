//
// Created by Gary on 3/16/2021.
//

#pragma once


#include "tiny_ecs.hpp"
#include "common.hpp"

class Explosion {
    static ECS::Entity CreateExplosion(vec2 location, vec2 radius);
};

