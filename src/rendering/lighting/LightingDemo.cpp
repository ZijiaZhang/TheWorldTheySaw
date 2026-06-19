#include "LightingDemo.hpp"
#include "ProceduralTextures.hpp"
#include "tiny_ecs.hpp"

#include <GLFW/glfw3.h>
#include <cmath>

bool LightingDemo::active = false;
int  LightingDemo::debugMode = 0;
bool LightingDemo::bilinearFixToggle = true;
ObliqueProjection LightingDemo::projection;

namespace {

// Persistent procedural texture library (created once, never freed before exit).
struct TexLib {
    GLuint floorAlbedo = 0, floorNormal = 0;
    GLuint wallAlbedo = 0, wallNormal = 0, wallHeight = 0;
    GLuint propAlbedo = 0, propNormal = 0, propHeight = 0;
    GLuint lampAlbedo = 0, lampGlow = 0;
    bool built = false;
};
TexLib g_tex;

ECS::Entity g_flashlight;
bool g_flashlightValid = false;

void buildTextures() {
    if (g_tex.built) return;
    g_tex.floorAlbedo = proctex::checkerAlbedo(128, {0.30f, 0.33f, 0.38f}, {0.20f, 0.22f, 0.26f}, 8);
    g_tex.floorNormal = proctex::gentleNormal(128, 8, 0.8f);
    g_tex.wallAlbedo  = proctex::brickAlbedo(96, 128);
    g_tex.wallNormal  = proctex::brickNormal(96, 128, 2.2f);
    g_tex.wallHeight  = proctex::verticalGradientHeight(16, 128);
    g_tex.propAlbedo  = proctex::solidAlbedo({0.45f, 0.40f, 0.32f});
    g_tex.propNormal  = proctex::domeNormal(96, 2.2f);
    g_tex.propHeight  = proctex::domeHeight(96);
    g_tex.lampAlbedo  = proctex::solidAlbedo({0.10f, 0.08f, 0.05f});
    g_tex.lampGlow    = proctex::radialGlow(96, {1.0f, 1.0f, 1.0f});
    g_tex.built = true;
}

ECS::Entity spawnLit(vec2 worldPos, vec2 sizePx, const LitSprite& proto) {
    ECS::Entity e;
    Motion& m = ECS::registry<Motion>.emplace(e);
    m.position = worldPos;
    m.scale = sizePx;
    m.angle = 0.f;
    LitSprite s = proto;
    // Painter order: farther (larger wx+wy, higher on screen) drawn first.
    s.sortKey = (int)(-(worldPos.x + worldPos.y) * 16.f);
    ECS::registry<LitSprite>.insert(e, s);
    return e;
}

ECS::Entity spawnLight(const Light& proto) {
    ECS::Entity e;
    ECS::registry<Light>.insert(e, proto);
    return e;
}

// Remove the demo's dynamic entities. Lit sprites carry a Motion and are wiped
// by WorldSystem::restart already; lights have no Motion and survive, so clear
// them here to avoid accumulating duplicates across reloads.
void clearScene() {
    g_flashlightValid = false;
    while (!ECS::registry<Light>.entities.empty())
        ECS::ContainerInterface::remove_all_components_of(ECS::registry<Light>.entities.back());
    while (!ECS::registry<LitSprite>.entities.empty())
        ECS::ContainerInterface::remove_all_components_of(ECS::registry<LitSprite>.entities.back());
}

void spawnScene() {
    // ---- Floor ----
    {
        LitSprite floor;
        floor.albedo = g_tex.floorAlbedo;
        floor.normal = g_tex.floorNormal;
        floor.surface = SurfaceType::Floor;
        floor.roughness = 0.75f;
        floor.occluder = false;
        floor.sortKey = -1000000;
        ECS::Entity e;
        Motion& m = ECS::registry<Motion>.emplace(e);
        m.position = {0.f, 0.f};
        m.scale = {1800.f, 1400.f};
        ECS::registry<LitSprite>.insert(e, floor);
    }

    // ---- Walls (occluders with height) ----
    {
        LitSprite wall;
        wall.albedo = g_tex.wallAlbedo;
        wall.normal = g_tex.wallNormal;
        wall.height = g_tex.wallHeight;
        wall.surface = SurfaceType::Wall;
        wall.roughness = 0.85f;
        wall.baseHeight = 0.f;
        wall.heightRange = 6.f;
        wall.occluder = true;

        vec2 wallPx = {110.f, 120.f};
        vec2 walls[] = {
            {-8.f, -3.f}, {-8.f, 0.f}, {-8.f, 3.f},
            { 3.f,  7.f}, { 6.f, 7.f},
            { 9.f, -4.f}
        };
        for (vec2 p : walls) spawnLit(p, wallPx, wall);
    }

    // ---- Rounded props (short occluders) ----
    {
        LitSprite prop;
        prop.albedo = g_tex.propAlbedo;
        prop.normal = g_tex.propNormal;
        prop.height = g_tex.propHeight;
        prop.surface = SurfaceType::Floor; // dome normals already point outward
        prop.roughness = 0.5f;
        prop.baseHeight = 0.f;
        prop.heightRange = 2.5f;
        prop.occluder = true;

        spawnLit({2.f, -4.f}, {90.f, 90.f}, prop);
        spawnLit({-2.f, 4.f}, {80.f, 80.f}, prop);
        spawnLit({5.f, 1.f},  {70.f, 70.f}, prop);
    }

    // ---- Emissive lamp (feeds Radiance Cascades GI) ----
    {
        LitSprite lamp;
        lamp.albedo = g_tex.lampAlbedo;
        lamp.emissive = g_tex.lampGlow;
        lamp.emissiveColor = {3.0f, 1.7f, 0.7f}; // warm HDR
        lamp.surface = SurfaceType::Floor;
        lamp.occluder = true;
        lamp.baseHeight = 0.4f;
        spawnLit({7.f, -1.f}, {80.f, 80.f}, lamp);

        // A cool emitter on the far side for color contrast.
        LitSprite lamp2 = lamp;
        lamp2.emissiveColor = {0.5f, 1.2f, 2.4f};
        spawnLit({-6.f, 6.f}, {70.f, 70.f}, lamp2);
    }

    // ---- Lights ----
    {
        // Flashlight (updated every frame).
        Light fl;
        fl.type = Light::Type::Spot;
        fl.color = {1.0f, 0.95f, 0.85f};
        fl.intensity = 5.0f;
        fl.position = {0.f, 0.f, 5.f};
        fl.direction = {0.f, 1.f, -0.3f};
        fl.cosInner = std::cos(0.28f); // ~16 deg
        fl.cosOuter = std::cos(0.50f); // ~28 deg
        fl.k1 = 0.0f;
        fl.k2 = 0.006f;
        fl.castsShadow = true;
        g_flashlight = spawnLight(fl);
        g_flashlightValid = true;

        // Warm point light co-located with the lamp (direct pool; RC adds bounce).
        Light lampLight;
        lampLight.type = Light::Type::Point;
        lampLight.color = {1.0f, 0.55f, 0.2f};
        lampLight.intensity = 3.0f;
        lampLight.position = {7.f, -1.f, 1.4f};
        lampLight.k2 = 0.02f;
        lampLight.castsShadow = false;
        spawnLight(lampLight);

        // Dim cool fill from above so unlit areas read as moonlight, not black.
        Light fill;
        fill.type = Light::Type::Point;
        fill.color = {0.25f, 0.30f, 0.42f};
        fill.intensity = 0.6f;
        fill.position = {0.f, 0.f, 40.f};
        fill.k2 = 0.0006f;
        fill.castsShadow = false;
        spawnLight(fill);
    }
}

} // namespace

void LightingDemo::setup(vec2 screenSize) {
    (void)screenSize;
    buildTextures();

    projection.tileW = 64.f;
    projection.tileH = 40.f;
    projection.heightScale = 20.f;

    spawnScene();
    active = true;
}

void LightingDemo::respawn() {
    if (!active) return;
    clearScene();
    spawnScene();
}

void LightingDemo::update(vec2 mouseTopLeft, vec2 focus, vec2 screenSize, const ObliqueProjection& proj) {
    if (!g_flashlightValid || !g_flashlight.has<Light>()) return;

    // GLFW gives top-left origin; the projection uses bottom-left (y up).
    vec2 mouseGl = {mouseTopLeft.x, screenSize.y - mouseTopLeft.y};
    vec2 targetXY = proj.unproject(mouseGl, 0.f, focus, screenSize);

    Light& fl = g_flashlight.get<Light>();
    fl.position = vec3(focus.x, focus.y, 5.0f);
    vec3 target = vec3(targetXY, 0.f);
    vec3 dir = target - fl.position;
    if (length(dir) > 1e-4f) fl.direction = normalize(dir);
}

void LightingDemo::onKey(int key, int action) {
    if (action != GLFW_PRESS) return;
    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_8)
        debugMode = key - GLFW_KEY_0;
    if (key == GLFW_KEY_B)
        bilinearFixToggle = !bilinearFixToggle;
}
