#include "DeferredLighting.hpp"
#include "GlLightingUtil.hpp"
#include "render.hpp"             // gl_has_errors
#include "render_components.hpp"  // TexturedVertex

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

// Tangent-space -> world-space basis for a sprite's surface (§3.2). Floors face
// +Z; walls face outward in the ground plane (facingAngle 0 => toward -Y).
mat3 computeSurfaceTBN(const LitSprite& s) {
    if (s.surface == SurfaceType::Floor) {
        return mat3(vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1));
    }
    float a = s.facingAngle;
    vec3 N = vec3(std::sin(a), -std::cos(a), 0.f); // outward, horizontal
    vec3 T = vec3(std::cos(a),  std::sin(a), 0.f); // along the wall, horizontal
    vec3 B = vec3(0, 0, 1);                         // up
    return mat3(T, B, N);
}

GLuint makeSolidTexture(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    unsigned char px[4] = {r, g, b, a};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return tex;
}

} // namespace

void DeferredLighting::init(int screenW, int screenH) {
    screenW_ = screenW;
    screenH_ = screenH;
    giW_ = std::max(1, int(screenW * giScale));
    giH_ = std::max(1, int(screenH * giScale));

    geomProgram_ = lgl::createRenderProgram(
        shader_path("lighting/gbuffer.vertex") + ".glsl",
        shader_path("lighting/gbuffer.fragment") + ".glsl");
    directProgram_ = lgl::createRenderProgram(
        shader_path("lighting/fullscreen.vertex") + ".glsl",
        shader_path("lighting/direct_light.fragment") + ".glsl");
    compositeProgram_ = lgl::createRenderProgram(
        shader_path("lighting/fullscreen.vertex") + ".glsl",
        shader_path("lighting/composite.fragment") + ".glsl");

    // Unit quad (matches RenderSystem::createSprite winding).
    TexturedVertex verts[4];
    verts[0].position = {-0.5f,  0.5f, 0.f}; verts[0].texcoord = {0.f, 1.f};
    verts[1].position = { 0.5f,  0.5f, 0.f}; verts[1].texcoord = {1.f, 1.f};
    verts[2].position = { 0.5f, -0.5f, 0.f}; verts[2].texcoord = {1.f, 0.f};
    verts[3].position = {-0.5f, -0.5f, 0.f}; verts[3].texcoord = {0.f, 0.f};
    uint16_t indices[6] = {0, 3, 1, 1, 3, 2};

    glGenVertexArrays(1, &quadVao_);
    glGenBuffers(1, &quadVbo_);
    glGenBuffers(1, &quadIbo_);
    glBindVertexArray(quadVao_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIbo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3));
    glBindVertexArray(0);

    emptyVao_ = lgl::createEmptyVAO();

    whiteTex_      = makeSolidTexture(255, 255, 255, 255);
    flatNormalTex_ = makeSolidTexture(128, 128, 255, 255);

    gbuffer_.init(screenW_, screenH_);
    sdf_.init(giW_, giH_);
    rc_.init(giW_, giH_);

    // Direct-light target (full res).
    directTex_ = lgl::createTexture2D(screenW_, screenH_, GL_RGBA16F, GL_LINEAR);
    glGenFramebuffers(1, &directFbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, directFbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, directTex_, 0);
    GLenum db = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &db);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gl_has_errors();
}

void DeferredLighting::resize(int screenW, int screenH) {
    if (screenW == screenW_ && screenH == screenH_) return;
    screenW_ = screenW;
    screenH_ = screenH;
    giW_ = std::max(1, int(screenW * giScale));
    giH_ = std::max(1, int(screenH * giScale));
    gbuffer_.resize(screenW_, screenH_);
    sdf_.resize(giW_, giH_);
    rc_.resize(giW_, giH_);
    lgl::resizeTexture2D(directTex_, screenW_, screenH_, GL_RGBA16F);
}

void DeferredLighting::bindSpriteTexture(int unit, GLuint tex, GLuint fallback, const char* uniform) {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, tex ? tex : fallback);
    glUniform1i(glGetUniformLocation(geomProgram_, uniform), unit);
}

void DeferredLighting::geometryPass(vec2 focus, vec2 screenSize) {
    gbuffer_.bindForWriting();
    gbuffer_.clear();

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    glUseProgram(geomProgram_);
    glBindVertexArray(quadVao_);
    glUniform2f(glGetUniformLocation(geomProgram_, "uScreenSize"), screenSize.x, screenSize.y);

    // Collect + painter-sort lit sprites.
    std::vector<ECS::Entity> ents;
    for (auto e : ECS::registry<LitSprite>.entities)
        if (e.has<Motion>()) ents.push_back(e);
    std::sort(ents.begin(), ents.end(), [](ECS::Entity a, ECS::Entity b) {
        return a.get<LitSprite>().sortKey < b.get<LitSprite>().sortKey;
    });

    for (ECS::Entity e : ents) {
        const LitSprite& s = e.get<LitSprite>();
        const Motion& m = e.get<Motion>();

        vec2 anchor = proj_.project(vec3(m.position, 0.f), focus, screenSize);
        vec2 size = {std::abs(m.scale.x), std::abs(m.scale.y)};

        glUniform2f(glGetUniformLocation(geomProgram_, "uAnchorScreen"), anchor.x, anchor.y);
        glUniform2f(glGetUniformLocation(geomProgram_, "uSpriteSize"), size.x, size.y);
        glUniform1f(glGetUniformLocation(geomProgram_, "uAngle"), m.angle);

        mat3 tbn = computeSurfaceTBN(s);
        glUniformMatrix3fv(glGetUniformLocation(geomProgram_, "uSurfaceTBN"), 1, GL_FALSE, &tbn[0][0]);
        glUniform1f(glGetUniformLocation(geomProgram_, "uRoughness"), s.roughness);
        glUniform1f(glGetUniformLocation(geomProgram_, "uBaseHeight"), s.baseHeight);
        glUniform1f(glGetUniformLocation(geomProgram_, "uHeightRange"), s.heightRange);
        glUniform1f(glGetUniformLocation(geomProgram_, "uIsOccluder"), s.occluder ? 1.f : 0.f);
        glUniform3fv(glGetUniformLocation(geomProgram_, "uTint"), 1, &s.tint.x);
        glUniform3fv(glGetUniformLocation(geomProgram_, "uEmissiveColor"), 1, &s.emissiveColor.x);

        bindSpriteTexture(0, s.albedo,   whiteTex_,      "uAlbedo");
        bindSpriteTexture(1, s.normal,   flatNormalTex_, "uNormal");
        bindSpriteTexture(2, s.height,   whiteTex_,      "uHeight");
        bindSpriteTexture(3, s.emissive, whiteTex_,      "uEmissive");

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
    }
    glBindVertexArray(0);
    gl_has_errors();
}

void DeferredLighting::directPass(vec2 focus, vec2 screenSize) {
    glBindFramebuffer(GL_FRAMEBUFFER, directFbo_);
    glViewport(0, 0, screenW_, screenH_);
    glDisable(GL_BLEND);
    glUseProgram(directProgram_);

    auto bind = [&](int unit, GLuint tex, const char* name) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, tex);
        glUniform1i(glGetUniformLocation(directProgram_, name), unit);
    };
    bind(0, gbuffer_.albedoMask(),  "uAlbedoMask");
    bind(1, gbuffer_.normalRough(), "uNormalMat");
    bind(2, gbuffer_.heightTex(),   "uHeight");
    bind(3, gbuffer_.emissive(),    "uEmissive");

    glUniform2f(glGetUniformLocation(directProgram_, "uScreenSize"), screenSize.x, screenSize.y);
    glUniform2f(glGetUniformLocation(directProgram_, "uFocus"), focus.x, focus.y);
    glUniform1f(glGetUniformLocation(directProgram_, "uKx"), proj_.kx());
    glUniform1f(glGetUniformLocation(directProgram_, "uKy"), proj_.ky());
    glUniform1f(glGetUniformLocation(directProgram_, "uKz"), proj_.kz());
    vec3 vd = normalize(viewDir);
    glUniform3fv(glGetUniformLocation(directProgram_, "uViewDir"), 1, &vd.x);
    glUniform1f(glGetUniformLocation(directProgram_, "uShininess"), shininess);
    glUniform1f(glGetUniformLocation(directProgram_, "uSpecStrength"), specStrength);
    glUniform1f(glGetUniformLocation(directProgram_, "uShadowStepPx"), shadowStepPx);
    glUniform1f(glGetUniformLocation(directProgram_, "uShadowBias"), shadowBias);
    glUniform1i(glGetUniformLocation(directProgram_, "uShadowSteps"), shadowSteps);

    // Build light arrays (world space).
    const int MAX = 16;
    int count = 0;
    float pos[MAX * 3], color[MAX * 3], dir[MAX * 3];
    float cosInner[MAX], cosOuter[MAX], k1[MAX], k2[MAX];
    int   type[MAX], shadow[MAX];
    for (auto e : ECS::registry<Light>.entities) {
        if (count >= MAX) break;
        const Light& L = e.get<Light>();
        pos[count * 3 + 0] = L.position.x; pos[count * 3 + 1] = L.position.y; pos[count * 3 + 2] = L.position.z;
        vec3 c = L.color * L.intensity;
        color[count * 3 + 0] = c.x; color[count * 3 + 1] = c.y; color[count * 3 + 2] = c.z;
        vec3 d = L.direction;
        dir[count * 3 + 0] = d.x; dir[count * 3 + 1] = d.y; dir[count * 3 + 2] = d.z;
        cosInner[count] = L.cosInner; cosOuter[count] = L.cosOuter;
        k1[count] = L.k1; k2[count] = L.k2;
        type[count] = (L.type == Light::Type::Spot) ? 1 : 0;
        shadow[count] = L.castsShadow ? 1 : 0;
        ++count;
    }
    glUniform1i(glGetUniformLocation(directProgram_, "uLightCount"), count);
    if (count > 0) {
        glUniform3fv(glGetUniformLocation(directProgram_, "uLightPos"), count, pos);
        glUniform3fv(glGetUniformLocation(directProgram_, "uLightColor"), count, color);
        glUniform3fv(glGetUniformLocation(directProgram_, "uLightDir"), count, dir);
        glUniform1fv(glGetUniformLocation(directProgram_, "uCosInner"), count, cosInner);
        glUniform1fv(glGetUniformLocation(directProgram_, "uCosOuter"), count, cosOuter);
        glUniform1fv(glGetUniformLocation(directProgram_, "uK1"), count, k1);
        glUniform1fv(glGetUniformLocation(directProgram_, "uK2"), count, k2);
        glUniform1iv(glGetUniformLocation(directProgram_, "uLightType"), count, type);
        glUniform1iv(glGetUniformLocation(directProgram_, "uLightShadow"), count, shadow);
    }

    lgl::drawFullscreen(emptyVao_);
    gl_has_errors();
}

void DeferredLighting::compositePass(vec2 screenSize, int debugMode) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenW_, screenH_);
    glDisable(GL_BLEND);
    glUseProgram(compositeProgram_);

    auto bind = [&](int unit, GLuint tex, const char* name) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, tex);
        glUniform1i(glGetUniformLocation(compositeProgram_, name), unit);
    };
    bind(0, gbuffer_.albedoMask(),  "uAlbedoMask");
    bind(1, gbuffer_.normalRough(), "uNormalMat");
    bind(2, gbuffer_.heightTex(),   "uHeight");
    bind(3, sdf_.sdf(),             "uSdf");
    bind(4, rc_.gi(),               "uGi");
    bind(5, directTex_,             "uDirect");
    bind(6, gbuffer_.emissive(),    "uEmissive");

    glUniform3fv(glGetUniformLocation(compositeProgram_, "uAmbient"), 1, &ambient.x);
    glUniform3fv(glGetUniformLocation(compositeProgram_, "uBackground"), 1, &background.x);
    glUniform1f(glGetUniformLocation(compositeProgram_, "uAoRadius"), aoRadius);
    glUniform1f(glGetUniformLocation(compositeProgram_, "uHeightVizScale"), 0.02f);
    glUniform1i(glGetUniformLocation(compositeProgram_, "uDebugMode"), debugMode);

    (void)screenSize;
    lgl::drawFullscreen(emptyVao_);
    gl_has_errors();
}

void DeferredLighting::render(vec2 focus, vec2 screenSize, int debugMode) {
    resize((int)screenSize.x, (int)screenSize.y);
    geometryPass(focus, screenSize);
    sdf_.generate(gbuffer_.albedoMask());
    rc_.compute(sdf_.sdf(), gbuffer_.emissive());
    directPass(focus, screenSize);
    compositePass(screenSize, debugMode);
}

void DeferredLighting::destroy() {
    if (geomProgram_)      glDeleteProgram(geomProgram_);
    if (directProgram_)    glDeleteProgram(directProgram_);
    if (compositeProgram_) glDeleteProgram(compositeProgram_);
    if (quadVao_) glDeleteVertexArrays(1, &quadVao_);
    if (quadVbo_) glDeleteBuffers(1, &quadVbo_);
    if (quadIbo_) glDeleteBuffers(1, &quadIbo_);
    if (emptyVao_) glDeleteVertexArrays(1, &emptyVao_);
    if (directFbo_) glDeleteFramebuffers(1, &directFbo_);
    if (directTex_) glDeleteTextures(1, &directTex_);
    if (whiteTex_) glDeleteTextures(1, &whiteTex_);
    if (flatNormalTex_) glDeleteTextures(1, &flatNormalTex_);
    gbuffer_.destroy();
    sdf_.destroy();
    rc_.destroy();
}
