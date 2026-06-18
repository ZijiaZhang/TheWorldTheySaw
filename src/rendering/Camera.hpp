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

    // Isometric pseudo-3D. When enabled, the geometry/forward passes place sprites
    // via iso_*_transform() and depth-sort by depth_for_world_position (x + y).
    // zScale = screen px of up-screen rise per world elevation unit.
    bool  isoEnabled = false;
    float zScale     = 1.f;

    // Per-sprite model matrices in GAME-UNIT space (feed straight into projection_2D,
    // exactly like the legacy Transform path). Ground skews the unit square into the
    // iso diamond; Billboard keeps the quad screen-axis-aligned, iso-projecting only
    // its center and raising it up-screen by `elevation`.
    mat3 iso_ground_transform(const Motion& m) const;
    mat3 iso_billboard_transform(const Motion& m, float elevation) const;

    // ---- world-space lighting helpers (lighting is done in TRUE world space) ----
    // Inverse of the iso placement: gl_FragCoord (y-up px) + world elevation wz -> world (wx,wy,wz).
    vec3 unproject_frag(vec2 fragYUp, float wz) const;
    // Mouse (glfw y-down px) -> world ground position at elevation wz (for the flashlight).
    vec3 ground_world_from_mouse(double mx, double my, float wz) const;
    // Constant tangent->world surface basis for a standing wall/object face: the flat tangent
    // normal (0,0,1) maps to world-horizontal toward the viewer +(1,1,0). Ground sprites instead
    // use identity (flat normal -> world +Z up).
    static mat3 wall_surface_tbn();
    // Constant world view direction (surface->camera) implied by the iso projection.
    vec3 world_view_dir() const;
};

