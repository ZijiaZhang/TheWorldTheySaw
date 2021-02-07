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

void Camera::set_screen_size(vec2 size) {
    screen_size = size;
}
