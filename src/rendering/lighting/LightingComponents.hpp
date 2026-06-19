#pragma once
//
// Shared data types for the pseudo-3D oblique deferred lighting system.
//
// See data/shaders/lighting and the design doc "伪3D 斜视角 Sprite 游戏 — 动态光照系统".
// Everything the lighting passes need is expressed in a single world coordinate
// system (wx, wy, wz); the oblique projection is the only place 2D/3D mix.
//
#include "common.hpp"
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

// How a flat sprite is oriented in the world. This drives the surface TBN that
// rotates tangent-space normals into world space in the geometry pass (§3.2).
enum class SurfaceType {
    Floor, // lies flat on the ground; world normal points up (+Z)
    Wall   // stands vertical; world normal points outward/front (horizontal)
};

// A sprite that participates in the deferred lighting pipeline. Pairs with a
// Motion component (position = world XY, scale = on-screen pixel size).
struct LitSprite {
    // GL texture ids. 0 means "use the built-in default" (flat normal, zero
    // height, black emissive, white albedo).
    GLuint albedo   = 0;
    GLuint normal   = 0;
    GLuint height   = 0;
    GLuint emissive = 0;

    SurfaceType surface = SurfaceType::Floor;

    // For walls: facing direction in the ground plane, radians. 0 => faces -Y
    // (toward the bottom of the screen / the viewer). Unused for floors.
    float facingAngle = 0.f;

    vec3  tint      = {1.f, 1.f, 1.f};
    float roughness = 0.6f;

    // Height channel mapping (world units): stored height = base + map.r * range.
    float baseHeight  = 0.f;
    float heightRange = 0.f;

    // Multiplies the emissive texture (lets one emissive sprite be colored/tuned).
    vec3  emissiveColor = {0.f, 0.f, 0.f};

    // Writes the occluder mask (GBuffer0.A) so JFA/RC and height shadows treat
    // this sprite as something that blocks light.
    bool  occluder = false;

    // Painter sort key; higher draws later (on top) in the G-buffer.
    int   sortKey = 0;
};

// A dynamic light, fully described in world space (§9). Point and spot share the
// struct; a point light simply ignores the cone.
struct Light {
    enum class Type { Point, Spot };
    Type  type = Type::Point;

    vec3  position = {0.f, 0.f, 0.f}; // world (wx, wy, wz)
    vec3  color    = {1.f, 1.f, 1.f};
    float intensity = 1.f;

    // Spot cone (world space). direction need not be normalized.
    vec3  direction = {0.f, 1.f, 0.f};
    float cosInner  = 0.96f;
    float cosOuter  = 0.86f;

    // Distance attenuation: 1 / (1 + k1*d + k2*d^2), d = 3D distance (§3.1).
    float k1 = 0.0f;
    float k2 = 0.0f;

    bool  castsShadow = true;
};

// Parameters of the oblique (isometric / dimetric) projection (§2). Maps a world
// point (wx, wy, wz) to a screen pixel and back. Shared verbatim with the shaders
// so CPU placement and GPU unprojection agree exactly.
//
//   sx = cx + (ex - ey) * kx
//   sy = cy + (ex + ey) * ky + wz * kz      (screen pixels, origin bottom-left, y up)
//   where ex = wx - focus.x, ey = wy - focus.y, (cx,cy) = screenSize/2
//
struct ObliqueProjection {
    float tileW = 64.f;       // tile footprint width  in px  -> kx = tileW/2
    float tileH = 32.f;       // tile footprint height in px  -> ky = tileH/2
    float heightScale = 1.f;  // world wz unit -> screen px    -> kz

    float kx() const { return tileW * 0.5f; }
    float ky() const { return tileH * 0.5f; }
    float kz() const { return heightScale; }

    vec2 project(vec3 world, vec2 focus, vec2 screenSize) const {
        vec2 c = screenSize * 0.5f;
        float ex = world.x - focus.x;
        float ey = world.y - focus.y;
        return {
            c.x + (ex - ey) * kx(),
            c.y + (ex + ey) * ky() + world.z * kz()
        };
    }

    // Inverse: given a screen pixel and a known height, recover world (wx, wy).
    vec2 unproject(vec2 screen, float height, vec2 focus, vec2 screenSize) const {
        vec2 c = screenSize * 0.5f;
        float A = (screen.x - c.x) / kx();                       // ex - ey
        float B = (screen.y - c.y - height * kz()) / ky();       // ex + ey
        float ex = 0.5f * (A + B);
        float ey = 0.5f * (B - A);
        return { focus.x + ex, focus.y + ey };
    }
};
