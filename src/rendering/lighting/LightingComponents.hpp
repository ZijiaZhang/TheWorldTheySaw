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
#include <cmath>

// How a flat sprite is oriented in the world. This drives the surface TBN that
// rotates tangent-space normals into world space in the geometry pass (§3.2).
enum class SurfaceType {
    Floor, // lies flat on the ground; world normal points up (+Z)
    Wall   // stands vertical; world normal points outward/front (horizontal)
};

// How a lit sprite's quad is placed. Upright is the default screen-aligned
// billboard (tall, camera-facing things: characters, trees, signs). WorldQuad
// transforms the 4 corners into world space and oblique-projects them, so the
// quad foreshortens with the camera — for flat ground decals/shadows, angled
// walls/ramps, and tumbling props. See docs/LIGHTING.md "fake-3D placement".
enum class Placement {
    Upright,
    WorldQuad
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
    // For upright wall cutouts, raise the default outward normal toward +Z.
    // This approximates painted oblique props that include a roof/top plane in a
    // single reusable sprite. 0 = vertical wall, 0.5 = noticeably top-facing.
    float normalLift = 0.f;

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

    // --- Optional world-space placement (Placement::WorldQuad) ---
    // When WorldQuad, the geometry pass ignores Motion.scale/position and instead
    // builds the quad from worldCenter + orientation + worldSize, writing the true
    // per-pixel world-z to the height channel so deferred unprojection stays exact.
    Placement placement = Placement::Upright;
    // Columns = the quad's world right (T), up (B), and normal (N) unit vectors.
    // Doubles as the surface TBN, so the lit normal always matches the visible
    // plane. Identity = a flat ground quad facing +Z.
    mat3 orientation = mat3(1.f);
    vec3 worldCenter = {0.f, 0.f, 0.f}; // quad center in world (wx, wy, wz)
    vec2 worldSize   = {0.f, 0.f};      // quad extents in WORLD units (not pixels)
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

// --- World-quad orientation helpers (fake-3D placement) ---
// Each returns a basis whose columns are the quad's world right/up/normal; assign
// it to LitSprite::orientation. The normal (column 2) also drives lighting.

// A flat quad lying in the ground plane, normal +Z. For decals, blob shadows,
// rugs, AoE rings, item drops (case 1).
inline mat3 groundQuadBasis() {
    return mat3(vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1));
}

// A quad oriented by an explicit world normal, with `upHint` choosing the
// in-plane axes. For arbitrary surfaces.
inline mat3 surfaceQuadBasis(vec3 normal, vec3 upHint = vec3(0, 0, 1)) {
    vec3 N = normalize(normal);
    vec3 R = cross(upHint, N);
    if (dot(R, R) < 1e-6f) R = vec3(1, 0, 0); // degenerate: upHint parallel to N
    R = normalize(R);
    vec3 U = cross(N, R);
    return mat3(R, U, N);
}

// A ground quad yawed by `yawRad` about +Z, then tilted up by `tiltRad` about its
// right axis. tilt 0 => flat; tilt -> pi flips it over (case 2 ramps, case 3
// tumbling props with an animated angle).
inline mat3 rampQuadBasis(float tiltRad, float yawRad) {
    vec3 R = vec3(std::cos(yawRad), std::sin(yawRad), 0.f);          // right (in ground plane)
    vec3 flatUp = vec3(-std::sin(yawRad), std::cos(yawRad), 0.f);    // up before tilt
    vec3 U = normalize(flatUp * std::cos(tiltRad) + vec3(0, 0, 1) * std::sin(tiltRad));
    vec3 N = normalize(cross(R, U));
    return mat3(R, U, N);
}
