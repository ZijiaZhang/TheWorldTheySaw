//
// Created by Gary on 2/2/2021.
//

#include "Camera.hpp"
#include <cmath>
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

mat3 Camera::iso_ground_transform(const Motion& m) const {
    // get_world_to_screen_transform() already maps a world point into game-unit
    // screen space (the iso diamond, focus + center folded in); scaling the unit
    // square by m.scale turns a world square into the diamond. (Phase 1 ignores
    // m.angle for ground tiles.)
    mat3 w2s = get_world_to_screen_transform();
    mat3 S   = { { m.scale.x, 0.f, 0.f }, { 0.f, m.scale.y, 0.f }, { 0.f, 0.f, 1.f } };
    return w2s * S;
}

vec3 Camera::unproject_frag(vec2 fragYUp, float wz) const {
    vec2 c = screen_size * 0.5f;
    vec2 focus = get_focus_position();
    float sx = fragYUp.x;
    float sy = screen_size.y - fragYUp.y;                 // y-up frag -> y-down game-unit
    float a = (sx - c.x) / oblique_x_scale;               // (wx-fx) - (wy-fy)
    float b = (sy - c.y + wz * zScale) / oblique_y_scale; // (wx-fx) + (wy-fy)
    return vec3(0.5f * (a + b) + focus.x, 0.5f * (b - a) + focus.y, wz);
}

vec3 Camera::ground_world_from_mouse(double mx, double my, float wz) const {
    vec2 c = screen_size * 0.5f;
    vec2 focus = get_focus_position();
    float a = (static_cast<float>(mx) - c.x) / oblique_x_scale;  // mouse is already y-down game-unit
    float b = (static_cast<float>(my) - c.y) / oblique_y_scale;
    return vec3(0.5f * (a + b) + focus.x, 0.5f * (b - a) + focus.y, wz);
}

mat3 Camera::wall_surface_tbn() {
    const float s = 0.70710678f;
    // columns [T | B | N]: flat tangent normal (0,0,1) -> N = +(1,1,0)/sqrt2 (world-horizontal,
    // toward the viewer); tangent +y -> world +Z (up the face); orthonormal, right-handed.
    return mat3(vec3(-s, s, 0.f), vec3(0.f, 0.f, 1.f), vec3(s, s, 0.f));
}

vec3 Camera::world_view_dir() const {
    return normalize(vec3(1.f, 1.f, 2.f * oblique_y_scale / zScale));
}

mat3 Camera::iso_billboard_transform(const Motion& m, float elevation) const {
    // Upright (screen-axis-aligned) quad, FOOT-ANCHORED: its bottom edge stands on
    // the iso ground point (motion.position) and the quad rises up-screen, so tall
    // sprites read as standing on the floor instead of floating. `elevation` lifts
    // the foot above the ground (0 = resting on it). Game-unit y is screen-down
    // (projection_2D sy < 0), so SUBTRACT to move up the screen.
    vec2 c = world_to_screen(m.position);
    c.y -= elevation * zScale;        // foot height above the ground
    c.y -= m.scale.y * 0.5f;          // foot-anchor: bottom edge at the ground point
    float ca = std::cos(m.angle), sa = std::sin(m.angle);
    // translate(c) * rotate(angle) * scale(scale), column-major glm mat3.
    return mat3(
        vec3( m.scale.x * ca,  m.scale.x * sa, 0.f),
        vec3(-m.scale.y * sa,  m.scale.y * ca, 0.f),
        vec3( c.x,             c.y,            1.f));
}
