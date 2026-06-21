#include "LightingDemo.hpp"

#include "GameInstance.hpp"
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
    GLuint guardBoothFrontFace = 0;
    GLuint guardBoothLeftFace = 0;
    GLuint guardBoothRoofFace = 0;
    GLuint playerReader = 0;
    GLuint playerReaderHeight = 0;
    GLuint oldBusRear = 0;
    GLuint oldBusRearHeight = 0;
    GLuint oldBusRearFace = 0;
    GLuint oldBusRightFace = 0;
    GLuint oldBusRoofFace = 0;
    GLuint forestWarningSign = 0;
    GLuint forestWarningSignHeight = 0;
    GLuint forestWarningSignFlat = 0;
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
    bool built = false;
};

TexLib g_tex;
bool g_uiBuilt = false;
ECS::Entity g_player;
bool g_playerValid = false;
ECS::Entity g_flashlight;
bool g_flashlightValid = false;

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

void buildTextures() {
    if (g_tex.built) return;
    g_tex.guardBooth = loadSpriteTexture("guard_booth.png");
    g_tex.guardBoothHeight = loadHeightFromSpriteAlpha("guard_booth.png");
    g_tex.guardBoothFrontFace = loadSpriteTexture("guard_booth_front_face.png");
    g_tex.guardBoothLeftFace = loadSpriteTexture("guard_booth_left_face.png");
    g_tex.guardBoothRoofFace = loadSpriteTexture("guard_booth_roof_face.png");
    g_tex.playerReader = loadSpriteTexture("player_reader.png");
    g_tex.playerReaderHeight = loadHeightFromSpriteAlpha("player_reader.png", 210);
    g_tex.oldBusRear = loadSpriteTexture("old_bus_rear.png");
    g_tex.oldBusRearHeight = loadHeightFromSpriteAlpha("old_bus_rear.png");
    g_tex.oldBusRearFace = loadSpriteTexture("old_bus_rear_face.png");
    g_tex.oldBusRightFace = loadSpriteTexture("old_bus_right_face.png");
    g_tex.oldBusRoofFace = loadSpriteTexture("old_bus_roof_face.png");
    g_tex.forestWarningSign = loadSpriteTexture("forest_warning_sign.png");
    g_tex.forestWarningSignHeight = loadHeightFromSpriteAlpha("forest_warning_sign.png", 220);
    g_tex.forestWarningSignFlat = loadSpriteTexture("forest_warning_sign_flat.png");
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

ECS::Entity spawnLitWorld(vec3 worldCenter, vec2 worldSize, mat3 orientation,
                          const LitSprite& proto, int sortOffset = 0) {
    ECS::Entity e;
    Motion& m = ECS::registry<Motion>.emplace(e);
    m.position = {worldCenter.x, worldCenter.y};
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

LitSprite planeFaceSprite(GLuint albedo, float roughness = 0.72f, bool occluder = true) {
    LitSprite s;
    s.albedo = albedo;
    s.roughness = roughness;
    s.occluder = occluder;
    return s;
}

LitSprite bakedArtSprite(GLuint albedo, GLuint height, float heightRange, bool occluder = true) {
    LitSprite s;
    s.albedo = albedo;
    s.height = height;
    // These generated sprites are painted cutouts. Keep their lighting neutral
    // while using alpha-derived height for shadowing until authored maps exist.
    s.surface = SurfaceType::Floor;
    s.roughness = 0.88f;
    s.baseHeight = 0.f;
    s.heightRange = heightRange;
    s.occluder = occluder;
    return s;
}

void spawnBus(vec3 baseCenter) {
    const float width = 1.75f;
    const float length = 3.20f;
    const float height = 2.90f;
    const float rearX = baseCenter.x - length * 0.46f;
    const float sideY = baseCenter.y - width * 0.48f;
    const float midZ = baseCenter.z + height * 0.5f;
    const float roofZ = baseCenter.z + height;

    LitSprite roof = planeFaceSprite(g_tex.oldBusRoofFace, 0.42f, true);
    spawnLitWorld({baseCenter.x + 0.08f, baseCenter.y - 0.03f, roofZ},
                  {length, width}, groundQuadBasis(), roof, 12);

    LitSprite side = planeFaceSprite(g_tex.oldBusRightFace, 0.55f, true);
    spawnLitWorld({baseCenter.x + 0.08f, sideY, midZ},
                  {length, height}, mat3(vec3(1, 0, 0), vec3(0, 0, 1), vec3(0, -1, 0)), side, 18);

    LitSprite rear = planeFaceSprite(g_tex.oldBusRearFace, 0.50f, true);
    spawnLitWorld({rearX, baseCenter.y, midZ},
                  {width, height}, mat3(vec3(0, 1, 0), vec3(0, 0, 1), vec3(-1, 0, 0)), rear, 24);
}

void spawnBooth(vec3 baseCenter) {
    const float width = 4.75f;
    const float depth = 2.05f;
    const float height = 4.95f;
    const float frontY = baseCenter.y - depth * 0.48f;
    const float leftX = baseCenter.x - width * 0.48f;
    const float midZ = baseCenter.z + height * 0.5f;
    const float roofZ = baseCenter.z + height;

    LitSprite side = planeFaceSprite(g_tex.guardBoothLeftFace, 0.86f, true);
    spawnLitWorld({leftX, baseCenter.y, midZ},
                  {depth, height}, surfaceQuadBasis(vec3(-1, 0, 0)), side, 12);

    LitSprite roof = planeFaceSprite(g_tex.guardBoothRoofFace, 0.78f, true);
    spawnLitWorld({baseCenter.x, baseCenter.y + 0.02f, roofZ},
                  {width, depth}, groundQuadBasis(), roof, 14);

    LitSprite front = planeFaceSprite(g_tex.guardBoothFrontFace, 0.86f, true);
    spawnLitWorld({baseCenter.x, frontY, midZ},
                  {width, height}, surfaceQuadBasis(vec3(0, -1, 0)), front, 18);
}

void spawnForestWarningSign(vec3 baseCenter) {
    const vec3 right = normalize(vec3(1.f, -0.20f, 0.f));
    const vec3 up = vec3(0.f, 0.f, 1.f);
    const vec3 normal = normalize(cross(right, up));
    const mat3 faceBasis = mat3(right, up, normal);

    const float signWidth = 4.6f;
    const float signHeight = 6.25f;

    LitSprite sign = planeFaceSprite(g_tex.forestWarningSignFlat, 0.84f, true);
    spawnLitWorld({baseCenter.x, baseCenter.y, baseCenter.z + signHeight * 0.5f},
                  {signWidth, signHeight}, faceBasis, sign, 42);
}

LitSprite uprightCutoutSprite(GLuint albedo, GLuint height, float heightRange,
                              float facingAngle = 0.f, float normalLift = 0.f,
                              bool occluder = true) {
    LitSprite s = bakedArtSprite(albedo, height, heightRange, occluder);
    s.surface = SurfaceType::Wall;
    s.facingAngle = facingAngle;
    s.normalLift = normalLift;
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
    if (!g_uiBuilt)
        g_uiBuilt = true;
    spawnUiSprite("rain_streaks", "rain_streaks.png", {600.f, 400.f}, {1200.f, 675.f});
}

void clearScene() {
    g_flashlightValid = false;
    g_playerValid = false;
    while (!ECS::registry<Light>.entities.empty())
        ECS::ContainerInterface::remove_all_components_of(ECS::registry<Light>.entities.back());
    while (!ECS::registry<LitSprite>.entities.empty())
        ECS::ContainerInterface::remove_all_components_of(ECS::registry<LitSprite>.entities.back());
}

void spawnScene() {
    LitSprite mud = floorSprite(g_tex.mudGroundTile, 0.9f);
    spawnLitWorld({0.f, 0.f, -0.03f}, {18.0f, 13.0f}, groundQuadBasis(), mud, -1000000);

    LitSprite road = floorSprite(g_tex.wetRoadTile, 0.35f);
    spawnLitWorld({4.4f, 0.4f, 0.01f}, {7.8f, 8.9f}, rampQuadBasis(0.04f, -0.42f), road, -900000);
    spawnLitWorld({0.7f, -2.7f, 0.04f}, {2.8f, 1.4f}, groundQuadBasis(), floorSprite(g_tex.puddleGlint, 0.18f), -890000);
    spawnLitWorld({6.2f, 0.9f, 0.04f}, {2.5f, 1.25f}, groundQuadBasis(), floorSprite(g_tex.puddleGlint, 0.18f), -890000);

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

    spawnBooth({-2.2f, 0.5f, 0.0f});

    spawnForestWarningSign({-6.9f, -1.8f, 0.0f});

    LitSprite marker = uprightCutoutSprite(g_tex.boundaryMarker, g_tex.boundaryMarkerHeight, 2.1f, 0.0f, 0.04f, true);
    spawnLit({0.8f, -1.8f}, {76.f, 114.f}, marker, 35);

    LitSprite barrier = uprightCutoutSprite(g_tex.stripedBarrier, g_tex.stripedBarrierHeight, 1.25f, -0.35f, 0.03f, true);
    spawnLit({3.6f, -2.5f}, {380.f, 214.f}, barrier, 45);

    LitSprite roadSign = uprightCutoutSprite(g_tex.roadWarningSign, g_tex.roadWarningSignHeight, 2.2f, 0.0f, 0.04f, true);
    spawnLit({7.3f, -3.4f}, {118.f, 177.f}, roadSign, 40);

    spawnBus({7.1f, 2.8f, 0.0f});

    LitSprite player = uprightCutoutSprite(g_tex.playerReader, g_tex.playerReaderHeight, 1.9f, 0.0f, 0.12f, true);
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
}

void LightingDemo::onKey(int key, int action) {
    if (action != GLFW_PRESS) return;
    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_8)
        debugMode = key - GLFW_KEY_0;
    if (key == GLFW_KEY_B)
        bilinearFixToggle = !bilinearFixToggle;
}
