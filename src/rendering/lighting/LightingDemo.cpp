#include "LightingDemo.hpp"

#include "GameInstance.hpp"
#include "ProceduralTextures.hpp"
#include "render.hpp"
#include "render_components.hpp"
#include "tiny_ecs.hpp"
#include "stb_image.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

bool LightingDemo::active = false;
int  LightingDemo::debugMode = 0;
bool LightingDemo::bilinearFixToggle = true;
ObliqueProjection LightingDemo::projection;

namespace {

const std::string kSpriteDir = "forest_gate/sprites/";

struct TexLib {
    GLuint guardBooth = 0;
    GLuint guardBoothHeight = 0;
    GLuint playerReader = 0;
    GLuint playerReaderHeight = 0;
    GLuint oldBusRear = 0;
    GLuint oldBusRearHeight = 0;
    GLuint forestWarningSign = 0;
    GLuint forestWarningSignHeight = 0;
    GLuint stripedBarrier = 0;
    GLuint stripedBarrierHeight = 0;
    GLuint boundaryMarker = 0;
    GLuint boundaryMarkerHeight = 0;
    GLuint roadWarningSign = 0;
    GLuint roadWarningSignHeight = 0;
    GLuint wetRoadTile = 0;
    GLuint mudGroundTile = 0;
    GLuint pineForestCluster = 0;
    GLuint pineForestClusterHeight = 0;
    GLuint shrubsRocks = 0;
    GLuint shrubsRocksHeight = 0;
    GLuint puddleGlint = 0;
    // Fake-3D placement showcase (procedurally generated).
    GLuint rugDecal = 0;
    GLuint blobShadow = 0;
    GLuint coin = 0;
    GLuint arrow[4] = {0, 0, 0, 0};
    bool built = false;
};

TexLib g_tex;
bool g_uiBuilt = false;
ECS::Entity g_player;
bool g_playerValid = false;
ECS::Entity g_flashlight;
bool g_flashlightValid = false;
// Fake-3D showcase entities (updated each frame).
ECS::Entity g_coin;
bool g_coinValid = false;
ECS::Entity g_blob;
bool g_blobValid = false;
ECS::Entity g_compass;
bool g_compassValid = false;

GLuint loadSpriteTexture(const std::string& file) {
    int w = 0;
    int h = 0;
    int channels = 0;
    const std::string path = textures_path(kSpriteDir + file);

    stbi_set_flip_vertically_on_load(true);
    stbi_uc* data = stbi_load(path.c_str(), &w, &h, &channels, 4);
    stbi_set_flip_vertically_on_load(false);
    if (data == nullptr)
        throw std::runtime_error("Failed to load forest gate sprite: " + path);

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    stbi_image_free(data);
    gl_has_errors();
    return tex;
}

GLuint loadHeightFromSpriteAlpha(const std::string& file, unsigned char maxHeight = 255) {
    int w = 0;
    int h = 0;
    int channels = 0;
    const std::string path = textures_path(kSpriteDir + file);

    stbi_set_flip_vertically_on_load(true);
    stbi_uc* data = stbi_load(path.c_str(), &w, &h, &channels, 4);
    stbi_set_flip_vertically_on_load(false);
    if (data == nullptr)
        throw std::runtime_error("Failed to load forest gate sprite height source: " + path);

    std::vector<unsigned char> height((size_t)w * (size_t)h * 4, 0);
    for (int i = 0; i < w * h; ++i) {
        unsigned char a = data[i * 4 + 3];
        unsigned char v = (unsigned char)((int)a * (int)maxHeight / 255);
        height[i * 4 + 0] = v;
        height[i * 4 + 1] = v;
        height[i * 4 + 2] = v;
        height[i * 4 + 3] = a;
    }
    stbi_image_free(data);

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, height.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl_has_errors();
    return tex;
}

unsigned char clampByte(float v) {
    return (unsigned char)std::max(0.f, std::min(255.f, v * 255.f + 0.5f));
}

GLuint uploadRGBA(int w, int h, const std::vector<unsigned char>& px) {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl_has_errors();
    return tex;
}

// A filled chevron pointing one of four ways (0=+Y,1=+X,2=-Y,3=-X), for the
// directional-facing demo. In a real game these would be authored per-direction
// sprite frames; here they stand in to show the per-frame texture swap.
GLuint makeArrowTexture(int dir, vec3 color) {
    const int N = 64;
    std::vector<unsigned char> d((size_t)N * N * 4, 0);
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            float fx = (x + 0.5f) / N * 2.f - 1.f;
            float fy = (y + 0.5f) / N * 2.f - 1.f;
            float u, v;
            if (dir == 0)      { u = fx;  v = fy;  }
            else if (dir == 1) { u = fy;  v = -fx; }
            else if (dir == 2) { u = -fx; v = -fy; }
            else               { u = -fy; v = fx;  }
            bool inside = (v < 0.7f) && (v > -0.55f) && (std::abs(u) < (0.72f - v) * 0.5f);
            if (inside) {
                size_t i = ((size_t)y * N + x) * 4;
                d[i + 0] = clampByte(color.x);
                d[i + 1] = clampByte(color.y);
                d[i + 2] = clampByte(color.z);
                d[i + 3] = 255;
            }
        }
    return uploadRGBA(N, N, d);
}

void buildTextures() {
    if (g_tex.built) return;
    g_tex.guardBooth = loadSpriteTexture("guard_booth.png");
    g_tex.guardBoothHeight = loadHeightFromSpriteAlpha("guard_booth.png");
    g_tex.playerReader = loadSpriteTexture("player_reader.png");
    g_tex.playerReaderHeight = loadHeightFromSpriteAlpha("player_reader.png", 210);
    g_tex.oldBusRear = loadSpriteTexture("old_bus_rear.png");
    g_tex.oldBusRearHeight = loadHeightFromSpriteAlpha("old_bus_rear.png");
    g_tex.forestWarningSign = loadSpriteTexture("forest_warning_sign.png");
    g_tex.forestWarningSignHeight = loadHeightFromSpriteAlpha("forest_warning_sign.png", 220);
    g_tex.stripedBarrier = loadSpriteTexture("striped_barrier.png");
    g_tex.stripedBarrierHeight = loadHeightFromSpriteAlpha("striped_barrier.png", 185);
    g_tex.boundaryMarker = loadSpriteTexture("boundary_marker.png");
    g_tex.boundaryMarkerHeight = loadHeightFromSpriteAlpha("boundary_marker.png", 220);
    g_tex.roadWarningSign = loadSpriteTexture("road_warning_sign.png");
    g_tex.roadWarningSignHeight = loadHeightFromSpriteAlpha("road_warning_sign.png", 210);
    g_tex.wetRoadTile = loadSpriteTexture("wet_road_tile.png");
    g_tex.mudGroundTile = loadSpriteTexture("mud_ground_tile.png");
    g_tex.pineForestCluster = loadSpriteTexture("pine_forest_cluster.png");
    g_tex.pineForestClusterHeight = loadHeightFromSpriteAlpha("pine_forest_cluster.png", 140);
    g_tex.shrubsRocks = loadSpriteTexture("shrubs_rocks.png");
    g_tex.shrubsRocksHeight = loadHeightFromSpriteAlpha("shrubs_rocks.png", 120);
    g_tex.puddleGlint = loadSpriteTexture("puddle_glint.png");

    // Fake-3D showcase textures (procedural so they need no art assets).
    g_tex.rugDecal   = proctex::checkerAlbedo(64, {0.82f, 0.74f, 0.30f}, {0.62f, 0.16f, 0.13f}, 4);
    g_tex.blobShadow = proctex::radialGlow(64, {0.015f, 0.015f, 0.02f});
    g_tex.coin       = proctex::radialGlow(64, {1.0f, 0.82f, 0.32f});
    g_tex.arrow[0]   = makeArrowTexture(0, {0.35f, 0.85f, 1.0f});
    g_tex.arrow[1]   = makeArrowTexture(1, {0.35f, 0.85f, 1.0f});
    g_tex.arrow[2]   = makeArrowTexture(2, {0.35f, 0.85f, 1.0f});
    g_tex.arrow[3]   = makeArrowTexture(3, {0.35f, 0.85f, 1.0f});
    g_tex.built = true;
}

int sortFor(vec2 worldPos, int offset = 0) {
    return (int)(-(worldPos.x + worldPos.y) * 16.f) + offset;
}

ECS::Entity spawnLit(vec2 worldPos, vec2 sizePx, const LitSprite& proto, int sortOffset = 0) {
    ECS::Entity e;
    Motion& m = ECS::registry<Motion>.emplace(e);
    m.position = worldPos;
    m.scale = sizePx;
    m.angle = 0.f;

    LitSprite s = proto;
    s.sortKey = sortFor(worldPos, sortOffset);
    ECS::registry<LitSprite>.insert(e, s);
    return e;
}

// Spawn a sprite placed as a world-space quad (Placement::WorldQuad): its 4
// corners are oriented by `orientation` (columns = right/up/normal), centered at
// `worldCenter`, sized in WORLD units. Used for ground decals, ramps, props.
ECS::Entity spawnLitWorld(vec3 worldCenter, vec2 worldSize, mat3 orientation,
                          const LitSprite& proto, int sortOffset = 0) {
    ECS::Entity e;
    Motion& m = ECS::registry<Motion>.emplace(e);
    m.position = {worldCenter.x, worldCenter.y}; // for the collector + painter sort
    m.scale = {1.f, 1.f};
    m.angle = 0.f;

    LitSprite s = proto;
    s.placement = Placement::WorldQuad;
    s.orientation = orientation;
    s.worldCenter = worldCenter;
    s.worldSize = worldSize;
    s.sortKey = sortFor({worldCenter.x, worldCenter.y}, sortOffset);
    ECS::registry<LitSprite>.insert(e, s);
    return e;
}

LitSprite floorSprite(GLuint albedo, float roughness = 0.82f) {
    LitSprite s;
    s.albedo = albedo;
    s.surface = SurfaceType::Floor;
    s.roughness = roughness;
    s.occluder = false;
    return s;
}

LitSprite bakedArtSprite(GLuint albedo, GLuint height, float heightRange, bool occluder = true) {
    LitSprite s;
    s.albedo = albedo;
    s.height = height;
    // These image-tool sprites are already painted with oblique form and baked
    // shading. Until they have authored normal/height maps, a neutral +Z normal
    // is less wrong than pretending the whole cutout is one vertical wall plane.
    s.surface = SurfaceType::Floor;
    s.roughness = 0.88f;
    s.baseHeight = 0.f;
    s.heightRange = heightRange;
    s.occluder = occluder;
    return s;
}

ECS::Entity spawnLight(const Light& proto) {
    ECS::Entity e;
    ECS::registry<Light>.insert(e, proto);
    return e;
}

void spawnUiSprite(const std::string& key, const std::string& file, vec2 screenPos, vec2 sizePx) {
    ShadedMesh& resource = cache_resource("forest_gate/ui/" + key);
    if (resource.mesh.vao.resource == 0) {
        stbi_set_flip_vertically_on_load(true);
        RenderSystem::createSprite(resource, textures_path(kSpriteDir + file), "sprite_textured");
        stbi_set_flip_vertically_on_load(false);
    }

    ECS::Entity e;
    Motion& m = ECS::registry<Motion>.emplace(e);
    m.position = screenPos;
    m.scale = sizePx;
    m.zValue = ZValuesMap["UI"];
    e.insert(ShadedMeshRefUI(resource));
}

void spawnOverlayUi() {
    if (!g_uiBuilt) {
        // Resources are cached, entities are recreated on restart.
        g_uiBuilt = true;
    }
    spawnUiSprite("rain_streaks", "rain_streaks.png", {600.f, 400.f}, {1200.f, 675.f});
}

void clearScene() {
    g_flashlightValid = false;
    g_playerValid = false;
    g_coinValid = false;
    g_blobValid = false;
    g_compassValid = false;
    while (!ECS::registry<Light>.entities.empty())
        ECS::ContainerInterface::remove_all_components_of(ECS::registry<Light>.entities.back());
    while (!ECS::registry<LitSprite>.entities.empty())
        ECS::ContainerInterface::remove_all_components_of(ECS::registry<LitSprite>.entities.back());
}

void spawnScene() {
    LitSprite mud = floorSprite(g_tex.mudGroundTile, 0.9f);
    mud.sortKey = -1000000;
    ECS::Entity ground;
    Motion& gm = ECS::registry<Motion>.emplace(ground);
    gm.position = {0.f, 0.f};
    gm.scale = {1380.f, 920.f};
    ECS::registry<LitSprite>.insert(ground, mud);

    LitSprite road = floorSprite(g_tex.wetRoadTile, 0.35f);
    spawnLit({4.2f, 0.8f}, {820.f, 547.f}, road, -900000);
    spawnLit({0.8f, -2.8f}, {410.f, 273.f}, floorSprite(g_tex.puddleGlint, 0.18f), -890000);
    spawnLit({6.6f, 1.0f}, {360.f, 240.f}, floorSprite(g_tex.puddleGlint, 0.18f), -890000);

    LitSprite tree = bakedArtSprite(g_tex.pineForestCluster, g_tex.pineForestClusterHeight, 0.8f, false);
    tree.tint = {0.64f, 0.70f, 0.64f};
    spawnLit({-8.4f, 4.2f}, {470.f, 313.f}, tree, -6000);
    spawnLit({-1.0f, 7.1f}, {560.f, 373.f}, tree, -6000);
    spawnLit({8.5f, 6.0f}, {500.f, 333.f}, tree, -6000);

    LitSprite shrubs = bakedArtSprite(g_tex.shrubsRocks, g_tex.shrubsRocksHeight, 0.25f, false);
    shrubs.tint = {0.78f, 0.80f, 0.72f};
    spawnLit({-5.5f, -3.8f}, {230.f, 153.f}, shrubs, -800);
    spawnLit({1.8f, -4.2f}, {260.f, 173.f}, shrubs, -800);
    spawnLit({6.5f, -3.9f}, {250.f, 167.f}, shrubs, -800);
    spawnLit({1.7f, 3.6f}, {210.f, 140.f}, shrubs, -800);

    LitSprite booth = bakedArtSprite(g_tex.guardBooth, g_tex.guardBoothHeight, 3.7f, true);
    booth.roughness = 0.83f;
    spawnLit({-2.2f, 0.5f}, {390.f, 293.f}, booth, 20);

    LitSprite sign = bakedArtSprite(g_tex.forestWarningSign, g_tex.forestWarningSignHeight, 2.7f, true);
    spawnLit({-6.9f, -1.8f}, {420.f, 280.f}, sign, 40);

    LitSprite marker = bakedArtSprite(g_tex.boundaryMarker, g_tex.boundaryMarkerHeight, 2.1f, true);
    spawnLit({0.8f, -1.8f}, {76.f, 114.f}, marker, 35);

    LitSprite barrier = bakedArtSprite(g_tex.stripedBarrier, g_tex.stripedBarrierHeight, 1.25f, true);
    spawnLit({3.6f, -2.5f}, {380.f, 214.f}, barrier, 45);

    LitSprite roadSign = bakedArtSprite(g_tex.roadWarningSign, g_tex.roadWarningSignHeight, 2.2f, true);
    spawnLit({7.3f, -3.4f}, {118.f, 177.f}, roadSign, 40);

    LitSprite bus = bakedArtSprite(g_tex.oldBusRear, g_tex.oldBusRearHeight, 3.2f, true);
    bus.roughness = 0.54f;
    spawnLit({7.1f, 2.8f}, {230.f, 191.f}, bus, 20);

    LitSprite player = bakedArtSprite(g_tex.playerReader, g_tex.playerReaderHeight, 1.9f, true);
    player.roughness = 0.62f;
    g_player = spawnLit({-1.9f, -2.0f}, {118.f, 177.f}, player, 30);
    g_playerValid = true;

    Light fl;
    fl.type = Light::Type::Spot;
    fl.color = {1.0f, 0.94f, 0.82f};
    fl.intensity = 3.4f;
    fl.position = {-1.9f, -2.0f, 3.0f};
    fl.direction = {1.f, 1.f, -0.25f};
    fl.cosInner = std::cos(0.24f);
    fl.cosOuter = std::cos(0.58f);
    fl.k2 = 0.02f;
    fl.castsShadow = true;
    g_flashlight = spawnLight(fl);
    g_flashlightValid = true;

    Light boothLamp;
    boothLamp.type = Light::Type::Point;
    boothLamp.color = {1.0f, 0.62f, 0.28f};
    boothLamp.intensity = 2.8f;
    boothLamp.position = {-2.7f, -0.1f, 2.9f};
    boothLamp.k2 = 0.09f;
    boothLamp.castsShadow = true;
    spawnLight(boothLamp);

    Light tailLights;
    tailLights.type = Light::Type::Point;
    tailLights.color = {1.0f, 0.06f, 0.035f};
    tailLights.intensity = 1.4f;
    tailLights.position = {6.8f, 2.1f, 1.1f};
    tailLights.k2 = 0.11f;
    tailLights.castsShadow = false;
    spawnLight(tailLights);

    Light moonFill;
    moonFill.type = Light::Type::Point;
    moonFill.color = {0.20f, 0.27f, 0.38f};
    moonFill.intensity = 1.4f;
    moonFill.position = {0.f, 0.f, 34.f};
    moonFill.k2 = 0.0007f;
    moonFill.castsShadow = false;
    spawnLight(moonFill);

    // ---- Fake-3D placement showcase (Placement::WorldQuad) ----
    // Case 1: a flat rug decal laid IN the ground plane — it foreshortens with the
    // camera into a diamond, instead of standing up like a screen-aligned billboard.
    {
        LitSprite rug;
        rug.albedo = g_tex.rugDecal;
        rug.roughness = 0.7f;
        spawnLitWorld({2.7f, -1.1f, 0.04f}, {3.0f, 3.0f}, groundQuadBasis(), rug, -860000);
    }
    // Case 1: a contact blob-shadow that follows the player on the ground.
    {
        LitSprite blob;
        blob.albedo = g_tex.blobShadow;
        blob.roughness = 1.0f;
        g_blob = spawnLitWorld({-1.9f, -2.0f, 0.02f}, {1.7f, 1.2f}, groundQuadBasis(), blob, 24);
        g_blobValid = true;
    }
    // Case 2: a tilted ramp board oriented along its real surface (its normal and
    // height follow the slope, so light and shadows land correctly).
    {
        LitSprite ramp;
        ramp.albedo = g_tex.rugDecal;
        ramp.tint = {0.72f, 0.86f, 1.0f};
        ramp.roughness = 0.5f;
        spawnLitWorld({5.3f, -1.0f, 0.0f}, {2.4f, 3.0f}, rampQuadBasis(0.55f, 0.0f), ramp, -120);
    }
    // Case 3: a tumbling coin — its orientation (and thus normal + height) is
    // animated each frame in update().
    {
        LitSprite coin;
        coin.albedo = g_tex.coin;
        coin.emissive = g_tex.coin;
        coin.emissiveColor = {0.5f, 0.38f, 0.12f};
        coin.roughness = 0.25f;
        g_coin = spawnLitWorld({0.5f, -1.3f, 1.5f}, {1.1f, 1.1f}, rampQuadBasis(0.f, 0.f), coin, 60);
        g_coinValid = true;
    }
    // Case 4: directional facing — an Upright billboard whose albedo frame is
    // chosen each frame from the aim heading (no geometry rotation).
    {
        LitSprite compass;
        compass.albedo = g_tex.arrow[0];
        compass.emissive = g_tex.arrow[0];
        compass.emissiveColor = {0.18f, 0.45f, 0.55f};
        compass.surface = SurfaceType::Floor;
        compass.roughness = 0.6f;
        g_compass = spawnLit({-3.7f, -1.2f}, {90.f, 90.f}, compass, 28);
        g_compassValid = true;
    }

    spawnOverlayUi();
}

void updatePlayerMovement() {
    if (!g_playerValid || !g_player.has<Motion>()) return;

    GLFWwindow* window = glfwGetCurrentContext();
    if (window == nullptr) return;

    vec2 move = {0.f, 0.f};
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) move.y += 1.f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) move.y -= 1.f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) move.x += 1.f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) move.x -= 1.f;

    if (length(move) < 0.001f) return;
    move = normalize(move);

    Motion& m = g_player.get<Motion>();
    float dt = GameInstance::frame_time / 1000.f;
    m.position += move * (3.1f * dt);
    m.position.x = std::max(-4.7f, std::min(3.8f, m.position.x));
    m.position.y = std::max(-4.2f, std::min(2.2f, m.position.y));

    if (g_player.has<LitSprite>())
        g_player.get<LitSprite>().sortKey = sortFor(m.position, 30);
}

} // namespace

void LightingDemo::setup(vec2 screenSize) {
    (void)screenSize;
    buildTextures();

    projection.tileW = 78.f;
    projection.tileH = 42.f;
    projection.heightScale = 24.f;

    spawnScene();
    active = true;
}

void LightingDemo::respawn() {
    if (!active) return;
    clearScene();
    spawnScene();
}

void LightingDemo::update(vec2 mouseTopLeft, vec2 focus, vec2 screenSize, const ObliqueProjection& proj) {
    updatePlayerMovement();
    if (!g_flashlightValid || !g_flashlight.has<Light>()) return;
    if (!g_playerValid || !g_player.has<Motion>()) return;

    vec2 mouseGl = {mouseTopLeft.x, screenSize.y - mouseTopLeft.y};
    vec2 targetXY = proj.unproject(mouseGl, 0.f, focus, screenSize);

    const Motion& pm = g_player.get<Motion>();
    Light& fl = g_flashlight.get<Light>();
    fl.position = vec3(pm.position.x, pm.position.y, 3.0f);
    vec3 target = vec3(targetXY, 0.f);
    vec3 dir = target - fl.position;
    if (length(dir) > 1e-4f) fl.direction = normalize(dir);

    // ---- Fake-3D showcase per-frame updates ----
    float t = (float)glfwGetTime();
    // Case 3: animate the coin's orientation so it tumbles (normal + height sweep).
    if (g_coinValid && g_coin.has<LitSprite>())
        g_coin.get<LitSprite>().orientation = rampQuadBasis(t * 2.2f, 0.6f);
    // Case 1: keep the blob shadow under the moving player.
    if (g_blobValid && g_blob.has<LitSprite>()) {
        g_blob.get<LitSprite>().worldCenter = vec3(pm.position.x, pm.position.y, 0.02f);
        if (g_blob.has<Motion>()) g_blob.get<Motion>().position = pm.position;
    }
    // Case 4: pick the directional frame from the aim heading (frame swap, not rotation).
    if (g_compassValid && g_compass.has<LitSprite>()) {
        vec2 aim = {fl.direction.x, fl.direction.y};
        int d = 0; // 0=+Y, 1=+X, 2=-Y, 3=-X
        if (length(aim) > 1e-4f) {
            if (std::abs(aim.x) > std::abs(aim.y)) d = (aim.x > 0.f) ? 1 : 3;
            else                                   d = (aim.y > 0.f) ? 0 : 2;
        }
        LitSprite& c = g_compass.get<LitSprite>();
        c.albedo = g_tex.arrow[d];
        c.emissive = g_tex.arrow[d];
    }
}

void LightingDemo::onKey(int key, int action) {
    if (action != GLFW_PRESS) return;
    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_8)
        debugMode = key - GLFW_KEY_0;
    if (key == GLFW_KEY_B)
        bilinearFixToggle = !bilinearFixToggle;
}
