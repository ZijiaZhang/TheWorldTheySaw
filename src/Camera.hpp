//
// Created by Gary on 2/2/2021.
//

#pragma once
#include "common.hpp"
#include "tiny_ecs.hpp"

class Camera {
    public:
        explicit Camera(vec2 off);

        Camera(vec2 off, ECS::Entity binding);

        vec2 offset{};
        ECS::Entity binding;

    vec2 get_position() const;

    vec2 screen_size{};

    void set_screen_size(vec2 size);
};

