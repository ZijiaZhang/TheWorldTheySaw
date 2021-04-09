//
// Created by Gary on 2/4/2021.
//

#pragma once
#include "tiny_ecs.hpp"
#include "common.hpp"
#include "render_components.hpp"


class Particle {

public:
    static ECS::Entity createParticle(vec2 position, vec2 size, float angle);
    GLResource<BUFFER> motion_buffer;
    std::list<Motion> motions;
    ShadedMesh mesh;
};



