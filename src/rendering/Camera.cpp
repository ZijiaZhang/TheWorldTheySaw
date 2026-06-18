//
// Created by Gary on 2/2/2021.
//

#include "Camera.hpp"
Camera::Camera(vec2 off){
    offset = off;
}

Camera::Camera(vec2 off, ECS::Entity bind){
    offset = off;
    binding = bind;
}

vec2 Camera::get_position() const{
    if (binding.has<Motion>())
        return offset + ECS::registry<Motion>.get(binding).position - (screen_size / 2.f);
    return offset - (screen_size / 2.f);
}

vec2 Camera::get_focus_position() const {
    return get_position() + (screen_size / 2.f);
}

vec2 Camera::world_delta_to_screen(vec2 world_delta) const {
    return {
        (world_delta.x - world_delta.y) * oblique_x_scale,
        (world_delta.x + world_delta.y) * oblique_y_scale
    };
}

vec2 Camera::world_to_screen(vec2 world_position) const {
    return (screen_size / 2.f) + world_delta_to_screen(world_position - get_focus_position());
}

vec2 Camera::screen_to_world(vec2 screen_position) const {
    vec2 delta = screen_position - (screen_size / 2.f);
    return get_focus_position() + screen_delta_to_world_delta(delta);
}

vec2 Camera::screen_delta_to_world_delta(vec2 delta) const {
    float x_minus_y = delta.x / oblique_x_scale;
    float x_plus_y = delta.y / oblique_y_scale;
    return {
        (x_plus_y + x_minus_y) * 0.5f,
        (x_plus_y - x_minus_y) * 0.5f
    };
}

float Camera::depth_for_world_position(vec2 world_position) const {
    return world_position.x + world_position.y;
}

mat3 Camera::get_world_to_screen_transform() const {
    vec2 focus = get_focus_position();
    vec2 center = screen_size / 2.f;

    float tx = center.x - oblique_x_scale * focus.x + oblique_x_scale * focus.y;
    float ty = center.y - oblique_y_scale * focus.x - oblique_y_scale * focus.y;

    return {
        { oblique_x_scale, oblique_y_scale, 0.f },
        { -oblique_x_scale, oblique_y_scale, 0.f },
        { tx, ty, 1.f }
    };
}

void Camera::set_screen_size(vec2 size) {
    screen_size = size;
}
