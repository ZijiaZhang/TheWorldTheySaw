#include "demo_scene.hpp"
#include "deferred_lighting.hpp"
#include "common.hpp"
#include "tiny_ecs.hpp"

#include <vector>
#include <cmath>
#include <functional>

namespace {

// Build an RGBA8 texture from a per-texel callback fn(u, v) -> 4 bytes.
Texture makeProcTexture(int w, int h, GLenum filter,
                        const std::function<void(float, float, unsigned char[4])>& fn) {
    std::vector<unsigned char> data(static_cast<size_t>(w) * h * 4);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float u = (x + 0.5f) / w;
            float v = (y + 0.5f) / h;
            fn(u, v, &data[(static_cast<size_t>(y) * w + x) * 4]);
        }
    }
    Texture t;
    glGenTextures(1, t.texture_id.data());
    glBindTexture(GL_TEXTURE_2D, t.texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    t.size = { w, h };
    return t;
}

unsigned char toByte(float f) {
    int v = static_cast<int>(f * 255.0f + 0.5f);
    return static_cast<unsigned char>(v < 0 ? 0 : (v > 255 ? 255 : v));
}

// Solid albedo with a circular alpha cutout (so round occluders look round and the
// alpha-clip in the geometry pass trims the quad corners).
Texture solidDiscAlbedo(vec3 color, bool disc) {
    return makeProcTexture(64, 64, GL_LINEAR, [=](float u, float v, unsigned char out[4]) {
        float du = u * 2.f - 1.f, dv = v * 2.f - 1.f;
        float r = std::sqrt(du * du + dv * dv);
        out[0] = toByte(color.x); out[1] = toByte(color.y); out[2] = toByte(color.z);
        out[3] = (disc && r > 1.0f) ? 0 : 255;
    });
}

// Rounded "stud" bump normal in screen+height space.
//
// NOTE: this is deliberately NOT a true hemisphere. A real hemisphere's normals
// reach fully horizontal (nz -> 0) at the silhouette, so a low/grazing flashlight
// lights only the outline as a thin bright crescent and leaves the cap dark.
// Instead we use a *bounded* bump: tilt grows from straight-up at the center to
// at most atan(kBulge) at the rim, so the lit area is a broad cap on the
// light-facing side and there is no bright edge ring. Raise kBulge for a rounder,
// more spherical look (toward a hemisphere); lower it for a flatter disc.
Texture domeNormal() {
    return makeProcTexture(64, 64, GL_LINEAR, [](float u, float v, unsigned char out[4]) {
        const float kBulge = 1.4f;        // max surface tilt = atan(kBulge) ~= 54 deg
        float du = u * 2.f - 1.f, dv = v * 2.f - 1.f;
        float r = std::sqrt(du * du + dv * dv);
        float nx, ny, nz;
        if (r >= 1.0f) { nx = 0.f; ny = 0.f; nz = 1.f; }   // outside the disc: flat up
        else           { nx = du * kBulge; ny = dv * kBulge; nz = 1.0f; }
        float inv = 1.0f / std::sqrt(nx * nx + ny * ny + nz * nz);
        out[0] = toByte((nx * inv) * 0.5f + 0.5f);
        out[1] = toByte((ny * inv) * 0.5f + 0.5f);
        out[2] = toByte((nz * inv) * 0.5f + 0.5f);
        out[3] = 255;
    });
}

// Vertical elevation ramp: r = 0 at the foot (v=0) -> 1 at the crown (v=1). Fed as a
// wall's height map so GBuffer2 holds the TRUE world elevation up the standing face
// (foot at wz=0, top at wz=heightRange). World-space lighting unprojects each face
// pixel back to its true (wx,wy,wz) from this — the foot and crown share one (wx,wy).
Texture verticalRampHeight() {
    return makeProcTexture(4, 64, GL_LINEAR, [](float u, float v, unsigned char out[4]) {
        (void)u;
        unsigned char b = toByte(v);
        out[0] = b; out[1] = b; out[2] = b; out[3] = 255;
    });
}

// Flat normal (0,0,1) for the ground.
Texture flatNormalTex() {
    return makeProcTexture(4, 4, GL_NEAREST, [](float, float, unsigned char out[4]) {
        out[0] = 128; out[1] = 128; out[2] = 255; out[3] = 255;
    });
}

// Radial emissive glow (bright center fading out).
Texture radialEmissive(vec3 color) {
    return makeProcTexture(64, 64, GL_LINEAR, [=](float u, float v, unsigned char out[4]) {
        float du = u * 2.f - 1.f, dv = v * 2.f - 1.f;
        float r = std::sqrt(du * du + dv * dv);
        float g = std::max(0.0f, 1.0f - r);
        g = g * g;
        out[0] = toByte(color.x * g); out[1] = toByte(color.y * g); out[2] = toByte(color.z * g);
        out[3] = (r > 1.0f) ? 0 : 255;
    });
}

ECS::Entity makeLit(vec2 pos, vec2 scale, int z, LitSprite&& lit) {
    ECS::Entity e;
    auto& m = ECS::registry<Motion>.emplace(e);
    m.position = pos;
    m.scale = scale;
    m.angle = 0.f;
    m.zValue = z;
    e.insert<LitSprite>(std::move(lit));
    return e;
}

} // namespace

void setupLightingDemo() {
    // Present the scene as isometric pseudo-3D. A camera already exists (created at
    // the end of WorldSystem::restart, just before this runs); flip it into iso mode.
    if (!ECS::registry<Camera>.entities.empty()) {
        auto& cam = ECS::registry<Camera>.entities[0].get<Camera>();
        cam.isoEnabled      = true;
        cam.oblique_x_scale = 1.0f;
        cam.oblique_y_scale = 0.5f;   // 2:1 iso diamond
        cam.zScale          = 1.0f;   // 1 screen px of rise per world height unit
    }

    // Floor: large flat receiver, not an occluder, ground height 0.
    {
        LitSprite lit;
        lit.albedo = solidDiscAlbedo(vec3(0.34f, 0.40f, 0.46f), false);
        lit.normal = flatNormalTex();
        lit.baseHeight = 0.f;
        lit.roughness = 1.f;
        lit.isOccluder = false;
        lit.isoMode = LitSprite::IsoMode::Ground;   // square skews into the iso diamond
        makeLit(vec2(0, 0), vec2(1400, 1400), 0, std::move(lit));
    }

    // Rounded "stud" occluders at a moderate height — these cast height shadows and
    // show off the normal-mapped wrap of the flashlight.
    struct Ball { vec2 pos; float r; vec3 col; };
    Ball balls[] = {
        { { -200, -90 }, 130, { 0.78f, 0.55f, 0.42f } },
        { {  150,  60 }, 150, { 0.45f, 0.62f, 0.78f } },
        { {  320, -150 }, 110, { 0.70f, 0.72f, 0.50f } },
    };
    for (const Ball& b : balls) {
        LitSprite lit;
        lit.albedo = solidDiscAlbedo(b.col, true);
        lit.normal = domeNormal();
        lit.baseHeight = 60.f;
        lit.roughness = 0.6f;
        lit.isOccluder = true;
        lit.elevation = 0.f;    // foot-anchored: the ball sits on the ground
        makeLit(b.pos, vec2(b.r, b.r), 6, std::move(lit));
    }

    // Standing wall slabs — upright billboards that rise from the ground (their
    // bottom edge is foot-anchored at the iso ground point) and cast wall shadows.
    // scale = (length on screen, height on screen); height also drives the shadow.
    struct Wall { vec2 pos; vec2 scale; };
    Wall walls[] = {
        { { -120, 150 }, { 230, 150 } },
        { {  240, 190 }, { 120, 185 } },
    };
    for (const Wall& w : walls) {
        LitSprite lit;
        lit.albedo = solidDiscAlbedo(vec3(0.62f, 0.64f, 0.68f), false);
        lit.normal = flatNormalTex();                    // flat tangent normal; orientation via the TBN
        lit.normalToSurface = Camera::wall_surface_tbn(); // bakes it to a world-horizontal wall face
        lit.height = verticalRampHeight();               // true elevation 0..heightRange up the face
        lit.hasHeightMap = true;
        lit.heightRange = w.scale.y;                     // wall world height == on-screen px (zScale 1)
        lit.baseHeight = 0.f;                            // foot rests on the ground
        lit.roughness = 0.8f;
        lit.isOccluder = true;
        lit.elevation = 0.f;                             // foot-anchored; the slab rises up-screen
        makeLit(w.pos, w.scale, 6, std::move(lit));
    }

    // Emissive blob: feeds the Radiance Cascades GI (soft ambient bounce).
    {
        LitSprite lit;
        lit.albedo = solidDiscAlbedo(vec3(0.05f, 0.05f, 0.05f), true);
        lit.normal = flatNormalTex();
        lit.emissive = radialEmissive(vec3(1.0f, 0.75f, 0.40f));
        lit.hasEmissive = true;
        lit.baseHeight = 25.f;
        lit.isOccluder = true; // emitters must be occluders so RC rays can hit & gather them
        lit.elevation = 40.f;  // float the glow slightly above the ground
        makeLit(vec2(-330, 210), vec2(120, 120), 12, std::move(lit));
    }
}
