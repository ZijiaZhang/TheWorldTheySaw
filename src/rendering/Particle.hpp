//
// Created by Gary on 2/4/2021.
//

#pragma once
#include "tiny_ecs.hpp"
#include "common.hpp"
#include <random>
#include "render_components.hpp"

class Particle {

public:
    static ECS::Entity createParticle(vec2 position, vec2 size, float lifetime);

    GLResource<BUFFER> motion_buffer;
    GLResource<BUFFER> speed_buffer;
    GLResource<BUFFER> scale_speed_buffer;
    std::vector<Motion> motions;
    std::vector<vec2> speed;
    std::vector<float> scale_speed;
    std::vector<mat3> t;
    ShadedMesh mesh;
    float start_time;
    static std::default_random_engine rng;
    static std::uniform_real_distribution<float> uniform_dist;
};



