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
    vec2 get_focus_position() const;
    vec2 world_to_screen(vec2 world_position) const;
    vec2 screen_to_world(vec2 screen_position) const;
    vec2 screen_delta_to_world_delta(vec2 screen_delta) const;
    vec2 world_delta_to_screen(vec2 world_delta) const;
    float depth_for_world_position(vec2 world_position) const;
    mat3 get_world_to_screen_transform() const;

    vec2 screen_size{};

    void set_screen_size(vec2 size);

    // Projection scales for world_to_screen / screen_to_world. Default (1, 1)
    // gives a plain axis-aligned view; set these for an oblique/dimetric camera.
    float oblique_x_scale = 1.f;
    float oblique_y_scale = 1.f;
};

