// Dynamic deferred lighting pipeline implementation. See LIGHTING_DESIGN_CONTRACT.md.
#include "deferred_lighting.hpp"
#include "render.hpp"
#include "tiny_ecs.hpp"

#include <vector>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <cstdlib>

// ------------------------------------------------------------------ small helpers
namespace {

void bindTex(int unit, GLuint tex) {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, tex);
}
void uInt(GLuint p, const char* n, int v)   { GLint l = glGetUniformLocation(p, n); if (l >= 0) glUniform1i(l, v); }
void uFlt(GLuint p, const char* n, float v) { GLint l = glGetUniformLocation(p, n); if (l >= 0) glUniform1f(l, v); }
void uV2 (GLuint p, const char* n, vec2 v)  { GLint l = glGetUniformLocation(p, n); if (l >= 0) glUniform2fv(l, 1, &v.x); }
void uV3 (GLuint p, const char* n, vec3 v)  { GLint l = glGetUniformLocation(p, n); if (l >= 0) glUniform3fv(l, 1, &v.x); }
void uM3 (GLuint p, const char* n, const mat3& m) { GLint l = glGetUniformLocation(p, n); if (l >= 0) glUniformMatrix3fv(l, 1, GL_FALSE, (const float*)&m); }

void checkFbo(const char* name) {
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error(std::string("Framebuffer incomplete: ") + name);
}

GLuint makeSingleTargetFbo(GLuint tex) {
    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    GLenum db = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &db);
    checkFbo("single-target");
    return fbo;
}

} // namespace

GLuint makeColorTexture(ivec2 size, GLenum internalFormat, GLenum format, GLenum type, GLenum filter) {
    GLuint t = 0;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size.x, size.y, 0, format, type, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return t;
}

void drawFullScreenTriangle() {
    static GLuint vao = 0;
    if (vao == 0) glGenVertexArrays(1, &vao); // dummy VAO; core profile forbids drawing with VAO 0
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

// ------------------------------------------------------------------ PingPong
void PingPong::create(ivec2 size, GLenum internalFormat, GLenum format, GLenum type, GLenum filter) {
    destroy();
    for (int i = 0; i < 2; i++) {
        tex[i] = makeColorTexture(size, internalFormat, format, type, filter);
        fbo[i] = makeSingleTargetFbo(tex[i]);
    }
    cur = 0;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void PingPong::destroy() {
    for (int i = 0; i < 2; i++) {
        if (fbo[i]) glDeleteFramebuffers(1, &fbo[i]);
        if (tex[i]) glDeleteTextures(1, &tex[i]);
        fbo[i] = 0; tex[i] = 0;
    }
    cur = 0;
}

// ------------------------------------------------------------------ GBuffer
void GBuffer::create(ivec2 size) {
    destroy();
    dim = size;
    g0 = makeColorTexture(size, GL_RGBA8,   GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST);
    g1 = makeColorTexture(size, GL_RGBA8,   GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST);
    g2 = makeColorTexture(size, GL_R16F,    GL_RED,  GL_FLOAT,         GL_NEAREST);
    g3 = makeColorTexture(size, GL_RGBA16F, GL_RGBA, GL_FLOAT,         GL_NEAREST);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, g1, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, g2, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, g3, 0);

    glGenRenderbuffers(1, &depthStencilRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, depthStencilRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size.x, size.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencilRbo);

    GLenum dbs[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
    glDrawBuffers(4, dbs);
    checkFbo("GBuffer");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void GBuffer::bindForWrite() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    GLenum dbs[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
    glDrawBuffers(4, dbs);
    glViewport(0, 0, dim.x, dim.y);
}
void GBuffer::destroy() {
    if (fbo) glDeleteFramebuffers(1, &fbo);
    if (depthStencilRbo) glDeleteRenderbuffers(1, &depthStencilRbo);
    GLuint texs[4] = { g0, g1, g2, g3 };
    for (GLuint t : texs) if (t) glDeleteTextures(1, &t);
    g0 = g1 = g2 = g3 = 0; fbo = 0; depthStencilRbo = 0; dim = {0,0};
}

// ------------------------------------------------------------------ SdfPass
void SdfPass::create(ivec2 size) {
    destroy();
    dim = size;
    seeds.create(size, GL_RG16F, GL_RG, GL_FLOAT, GL_NEAREST);
    sdfDist = makeColorTexture(size, GL_R16F, GL_RED, GL_FLOAT, GL_NEAREST);
    sdfFbo = makeSingleTargetFbo(sdfDist);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    seedFx.load_from_file(shader_path("fullscreen") + ".vertex.glsl", shader_path("jfa_seed") + ".fragment.glsl");
    stepFx.load_from_file(shader_path("fullscreen") + ".vertex.glsl", shader_path("jfa_step") + ".fragment.glsl");
    distFx.load_from_file(shader_path("fullscreen") + ".vertex.glsl", shader_path("sdf_distance") + ".fragment.glsl");
}
void SdfPass::generate(const GBuffer& gbuffer) {
    vec2 res = { (float)dim.x, (float)dim.y };
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    // Pass 2a: seed into seeds.tex[0]
    seeds.cur = 0;
    glBindFramebuffer(GL_FRAMEBUFFER, seeds.fbo[0]);
    glViewport(0, 0, dim.x, dim.y);
    glUseProgram(seedFx.program);
    bindTex(0, gbuffer.albedoMask()); uInt(seedFx.program, "uGAlbedo", 0);
    uV2(seedFx.program, "uResolution", res);
    drawFullScreenTriangle();

    // Pass 2b: log2 jump steps, halving offset
    glUseProgram(stepFx.program);
    int maxdim = std::max(dim.x, dim.y);
    int offset = 1;
    while (offset * 2 < maxdim) offset *= 2; // largest power of two < maxdim
    for (; offset >= 1; offset /= 2) {
        glBindFramebuffer(GL_FRAMEBUFFER, seeds.writeFbo());
        glViewport(0, 0, dim.x, dim.y);
        bindTex(4, seeds.readTex()); uInt(stepFx.program, "uSeedTex", 4);
        uV2(stepFx.program, "uResolution", res);
        uFlt(stepFx.program, "uOffset", (float)offset);
        drawFullScreenTriangle();
        seeds.swap();
    }

    // Pass 2c: distance from final seeds -> sdfDist
    glBindFramebuffer(GL_FRAMEBUFFER, sdfFbo);
    glViewport(0, 0, dim.x, dim.y);
    glUseProgram(distFx.program);
    bindTex(4, seeds.readTex()); uInt(distFx.program, "uSeedTex", 4);
    bindTex(0, gbuffer.albedoMask()); uInt(distFx.program, "uGAlbedo", 0);
    uV2(distFx.program, "uResolution", res);
    drawFullScreenTriangle();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void SdfPass::destroy() {
    seeds.destroy();
    if (sdfFbo) glDeleteFramebuffers(1, &sdfFbo);
    if (sdfDist) glDeleteTextures(1, &sdfDist);
    sdfFbo = 0; sdfDist = 0; dim = {0,0};
}

// ------------------------------------------------------------------ RadianceCascades
void RadianceCascades::create(ivec2 size, const Params& p) {
    destroy();
    prm = p;
    dim = size;
    giDim = prm.halfResGI ? ivec2{ (size.x + 1) / 2, (size.y + 1) / 2 } : size;

    cascades.create(size, GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_LINEAR);
    gi = makeColorTexture(giDim, GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_LINEAR);
    giFbo = makeSingleTargetFbo(gi);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    castFx.load_from_file(shader_path("fullscreen") + ".vertex.glsl", shader_path("rc_cascade") + ".fragment.glsl");
    mergeFx.load_from_file(shader_path("fullscreen") + ".vertex.glsl", shader_path("rc_merge") + ".fragment.glsl");
}
void RadianceCascades::compute(const SdfPass& sdf, const GBuffer& gbuffer) {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    // tNear per cascade: cumulative sum of interval lengths d0*4^k
    std::vector<float> tNear(prm.numCascades, 0.f);
    for (int c = 1; c < prm.numCascades; c++)
        tNear[c] = tNear[c - 1] + prm.d0 * std::pow(4.f, (float)(c - 1));

    // A 1x1 black texture serves as the "upper cascade" for the top cascade (no N+1).
    static GLuint blackTex = 0;
    if (blackTex == 0) {
        float px[4] = { 0, 0, 0, 0 };
        glGenTextures(1, &blackTex);
        glBindTexture(GL_TEXTURE_2D, blackTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 1, 1, 0, GL_RGBA, GL_FLOAT, px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    cascades.cur = 0;
    GLuint upper = blackTex;
    glUseProgram(mergeFx.program);
    GLuint prog = mergeFx.program;

    for (int c = prm.numCascades - 1; c >= 0; --c) {
        bool last = (c == 0);
        ivec2 res = last ? giDim : dim;

        glBindFramebuffer(GL_FRAMEBUFFER, last ? giFbo : cascades.writeFbo());
        glViewport(0, 0, res.x, res.y);

        bindTex(6, upper);                 uInt(prog, "uUpperCascade", 6);
        bindTex(5, sdf.sdfTexture());      uInt(prog, "uSDF", 5);
        bindTex(3, gbuffer.emissive());    uInt(prog, "uGEmissive", 3);
        uV2 (prog, "uResolution", { (float)res.x, (float)res.y });
        uInt(prog, "uCascadeIndex", c);
        uFlt(prog, "uBaseProbeSpacing", prm.baseProbeSpacing);
        uInt(prog, "uBaseDirCount", prm.baseDirCount);
        uFlt(prog, "uD0", prm.d0);
        uFlt(prog, "uIntervalNear", tNear[c]);
        uInt(prog, "uMaxSteps", prm.maxSteps);
        uFlt(prog, "uEps", prm.eps);
        uFlt(prog, "uMinStep", prm.minStep);
        drawFullScreenTriangle();

        if (!last) {
            upper = cascades.writeTex();
            cascades.swap();
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void RadianceCascades::destroy() {
    cascades.destroy();
    if (giFbo) glDeleteFramebuffers(1, &giFbo);
    if (gi) glDeleteTextures(1, &gi);
    giFbo = 0; gi = 0; dim = {0,0}; giDim = {0,0};
}

// ------------------------------------------------------------------ DirectLightPass
void DirectLightPass::create(ivec2 size) {
    destroy();
    dim = size;
    directResult = makeColorTexture(size, GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_LINEAR);
    fbo = makeSingleTargetFbo(directResult);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    fx.load_from_file(shader_path("fullscreen") + ".vertex.glsl", shader_path("direct_light") + ".fragment.glsl");
}
void DirectLightPass::render(const GBuffer& gbuffer, const Spotlight& f, const Camera& cam) {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, dim.x, dim.y);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(fx.program);
    GLuint p = fx.program;

    // World-space lighting (iso) needs the unprojection constants + the light's screen
    // position for the shadow march. Legacy keeps f.pos.xy (already screen px, y-up).
    const bool worldSpace = cam.isoEnabled;
    vec2 lightFrag;
    if (worldSpace) {
        vec2 g = cam.world_to_screen(vec2(f.pos.x, f.pos.y)); // ground projection (game-unit, y-down)
        g.y -= f.pos.z * cam.zScale;                          // raise by the light's world height
        lightFrag = vec2(g.x, (float)dim.y - g.y);            // -> gl_FragCoord (y-up)
    } else {
        lightFrag = vec2(f.pos.x, f.pos.y);
    }

    bindTex(1, gbuffer.normalMat()); uInt(p, "uGNormal", 1);
    bindTex(2, gbuffer.height());    uInt(p, "uGHeight", 2);
    uV2(p, "uResolution", { (float)dim.x, (float)dim.y });
    uFlt(p, "uWorldSpace", worldSpace ? 1.0f : 0.0f);
    uFlt(p, "uOx", cam.oblique_x_scale);
    uFlt(p, "uOy", cam.oblique_y_scale);
    uFlt(p, "uZScale", cam.zScale);
    uV2(p, "uCenter", { (float)dim.x * 0.5f, (float)dim.y * 0.5f });
    uV2(p, "uFocus", cam.get_focus_position());
    uV2(p, "uLightFrag", lightFrag);
    uV3(p, "uSpotDirWorld", normalize(f.spotDirWorld));
    uV3(p, "uLightPos", f.pos);
    uV2(p, "uSpotDir", normalize(f.dir));
    uFlt(p, "uCosInner", f.cosInner);
    uFlt(p, "uCosOuter", f.cosOuter);
    uV3(p, "uLightColor", f.color);
    uFlt(p, "uK1", f.k1);
    uFlt(p, "uK2", f.k2);
    uFlt(p, "uShadowStepLen", f.shadowStepLen);
    uInt(p, "uShadowSteps", f.shadowSteps);
    uFlt(p, "uShadowBias", f.shadowBias);
    uFlt(p, "uShadowStartBias", f.shadowStartBias);
    drawFullScreenTriangle();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void DirectLightPass::destroy() {
    if (fbo) glDeleteFramebuffers(1, &fbo);
    if (directResult) glDeleteTextures(1, &directResult);
    fbo = 0; directResult = 0; dim = {0,0};
}

// ------------------------------------------------------------------ CompositePass
void CompositePass::create() {
    fx.load_from_file(shader_path("fullscreen") + ".vertex.glsl", shader_path("composite") + ".fragment.glsl");
}
void CompositePass::render(const GBuffer& gbuffer, GLuint giTex, GLuint directTex, GLuint sdfTex,
                           ivec2 screenSize, int debugMode, float exposure, bool worldSpace) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenSize.x, screenSize.y);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glUseProgram(fx.program);
    GLuint p = fx.program;
    bindTex(0, gbuffer.albedoMask()); uInt(p, "uGAlbedo", 0);
    bindTex(1, gbuffer.normalMat());  uInt(p, "uGNormal", 1);
    bindTex(2, gbuffer.height());     uInt(p, "uGHeight", 2);
    bindTex(3, gbuffer.emissive());   uInt(p, "uGEmissive", 3);
    bindTex(5, sdfTex);               uInt(p, "uSDF", 5);
    bindTex(7, giTex);                uInt(p, "uGI", 7);
    bindTex(8, directTex);            uInt(p, "uDirect", 8);
    uFlt(p, "uExposure", exposure);
    uFlt(p, "uWorldSpace", worldSpace ? 1.0f : 0.0f);
    uInt(p, "uDebugMode", debugMode);
    uFlt(p, "uHeightDebugScale", 64.0f);
    uFlt(p, "uSdfDebugScale", 64.0f);
    drawFullScreenTriangle();
}
void CompositePass::destroy() {}

// ------------------------------------------------------------------ DeferredRenderer
namespace {
void makeSolid(Texture& t, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    unsigned char px[4] = { r, g, b, a };
    glGenTextures(1, t.texture_id.data());
    glBindTexture(GL_TEXTURE_2D, t.texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    t.size = { 1, 1 };
}
} // namespace

void DeferredRenderer::init(ivec2 framebufferSize, const RadianceCascades::Params& rcParams) {
    dim = framebufferSize;

    RadianceCascades::Params p = rcParams;
    p.halfResGI = false;   // full-res GI for a correct first integration (avoids c0 res mismatch)
    p.baseDirCount = 1;    // dirCount = 4^c packs exactly into the D x D (=2^c x 2^c) tile grid

    gbuffer.create(dim);
    sdf.create(dim);
    rc.create(dim, p);
    direct.create(dim);
    composite.create();

    gbufferFx.load_from_file(shader_path("gbuffer") + ".vertex.glsl", shader_path("gbuffer") + ".fragment.glsl");

    // shared unit quad (reuses createSprite's buffer setup; its effect is unused)
    RenderSystem::createSprite(quad, "", "sprite_textured");

    makeSolid(flatNormal, 128, 128, 255, 255); // (0.5,0.5,1) -> flat up normal
    makeSolid(whiteTex,   255, 255, 255, 255);

    // Demo-friendly flashlight: a bright near-omni pool at moderate height so the
    // height-field shadows and normal shading read clearly. Tune for your game.
    lights.flashlight.color    = vec3(2.4f, 2.15f, 1.8f);
    lights.flashlight.cosInner = -1.0f;     // omnidirectional pool (cone = 1 everywhere)
    lights.flashlight.cosOuter = -1.001f;
    lights.flashlight.k1       = 0.0f;
    lights.flashlight.k2       = 2.0e-5f;
    lights.flashlight.pos.z    = 300.0f;   // raised for the iso scene (taller than the walls)
    lights.flashlight.shadowStepLen   = 3.0f;
    lights.flashlight.shadowSteps     = 200;
    lights.flashlight.shadowBias      = 1.5f;
    lights.flashlight.shadowStartBias = 3.0f;

    initialized = true;
}

void DeferredRenderer::geometryPass(const Camera& camera, const mat3& projection_2D) {
    gbuffer.bindForWrite();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    // Per-attachment clears to the contract's clear values (§1).
    float c0[4] = { 0, 0, 0, 0 };
    float c1[4] = { 0.5f, 0.5f, 1.0f, 0.0f };
    float c2[4] = { 0, 0, 0, 0 };
    float c3[4] = { 0, 0, 0, 0 };
    glClearBufferfv(GL_COLOR, 0, c0);
    glClearBufferfv(GL_COLOR, 1, c1);
    glClearBufferfv(GL_COLOR, 2, c2);
    glClearBufferfv(GL_COLOR, 3, c3);

    GLuint prog = gbufferFx.program;
    glUseProgram(prog);

    // Draw order. Iso mode: ground sprites first (always behind), then billboards
    // back-to-front by iso depth (x + y ascending), zValue as the final tie-break.
    // Non-iso: the legacy zValue painter sort.
    auto entities = ECS::registry<LitSprite>.entities;
    if (camera.isoEnabled) {
        std::sort(entities.begin(), entities.end(), [](const ECS::Entity a, const ECS::Entity b) {
            auto& la = ECS::registry<LitSprite>.get(a);
            auto& lb = ECS::registry<LitSprite>.get(b);
            int ga = (la.isoMode == LitSprite::IsoMode::Ground) ? 0 : 1;
            int gb = (lb.isoMode == LitSprite::IsoMode::Ground) ? 0 : 1;
            if (ga != gb) return ga < gb;                 // ground layer always behind
            auto& ma = ECS::registry<Motion>.get(a);
            auto& mb = ECS::registry<Motion>.get(b);
            float da = ma.position.x + ma.position.y;
            float db = mb.position.x + mb.position.y;
            if (da != db) return da < db;                 // far-first (ascending x + y)
            return ma.zValue < mb.zValue;
        });
    } else {
        std::sort(entities.begin(), entities.end(), [](const ECS::Entity a, const ECS::Entity b) {
            return ECS::registry<Motion>.get(a).zValue < ECS::registry<Motion>.get(b).zValue;
        });
    }

    GLint in_position_loc = glGetAttribLocation(prog, "in_position");
    GLint in_texcoord_loc = glGetAttribLocation(prog, "in_texcoord");
    GLint projection_uloc = glGetUniformLocation(prog, "projection");

    for (ECS::Entity e : entities) {
        if (!ECS::registry<Motion>.has(e)) continue;
        auto& motion = ECS::registry<Motion>.get(e);
        auto& lit = ECS::registry<LitSprite>.get(e);

        mat3 transformMat;
        if (camera.isoEnabled) {
            transformMat = (lit.isoMode == LitSprite::IsoMode::Ground)
                ? camera.iso_ground_transform(motion)
                : camera.iso_billboard_transform(motion, lit.elevation);
        } else {
            Transform t;
            t.translate(motion.position - camera.get_position());
            t.rotate(motion.angle);
            t.scale(motion.scale);
            transformMat = t.mat;
        }
        uM3(prog, "transform", transformMat);
        if (projection_uloc >= 0) glUniformMatrix3fv(projection_uloc, 1, GL_FALSE, (const float*)&projection_2D);

        // Always bind a valid texture to every source unit (§ integrator note 1).
        bindTex(0, lit.albedo.is_valid()   ? (GLuint)lit.albedo.texture_id   : (GLuint)whiteTex.texture_id);   uInt(prog, "uAlbedo", 0);
        bindTex(1, lit.normal.is_valid()   ? (GLuint)lit.normal.texture_id   : (GLuint)flatNormal.texture_id); uInt(prog, "uNormalMap", 1);
        bindTex(2, lit.height.is_valid()   ? (GLuint)lit.height.texture_id   : (GLuint)whiteTex.texture_id);   uInt(prog, "uHeightMap", 2);
        bindTex(3, lit.emissive.is_valid() ? (GLuint)lit.emissive.texture_id : (GLuint)whiteTex.texture_id);   uInt(prog, "uEmissiveMap", 3);

        uFlt(prog, "uIsOccluder",  lit.isOccluder ? 1.0f : 0.0f);
        uFlt(prog, "uRoughness",   lit.roughness);
        uFlt(prog, "uBaseHeight",  lit.baseHeight);
        uFlt(prog, "uHeightRange", lit.heightRange);
        uFlt(prog, "uHasHeightMap", (lit.hasHeightMap && lit.height.is_valid()) ? 1.0f : 0.0f);
        uFlt(prog, "uHasEmissive",  (lit.hasEmissive  && lit.emissive.is_valid()) ? 1.0f : 0.0f);
        uM3 (prog, "uNormalToSurface", lit.normalToSurface);
        uFlt(prog, "uWorldSpace", camera.isoEnabled ? 1.0f : 0.0f);

        glBindVertexArray(quad.mesh.vao);
        glBindBuffer(GL_ARRAY_BUFFER, quad.mesh.vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad.mesh.ibo);
        if (in_position_loc >= 0) {
            glEnableVertexAttribArray(in_position_loc);
            glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
        }
        if (in_texcoord_loc >= 0) {
            glEnableVertexAttribArray(in_texcoord_loc);
            glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3));
        }
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
    }
    glBindVertexArray(0);
}

void DeferredRenderer::draw(const Camera& camera, ivec2 window_size, const mat3& projection_2D) {
    if (!initialized) return;

    // Optional one-time debug-view override from the environment (headless capture).
    static bool once = true;
    if (once) {
        once = false;
        const char* d = getenv("LIGHTDBG");
        if (d) debugMode = atoi(d);
    }

    geometryPass(camera, projection_2D);   // Pass 1
    sdf.generate(gbuffer);                  // Pass 2
    rc.compute(sdf, gbuffer);               // Pass 3
    direct.render(gbuffer, lights.flashlight, camera); // Pass 4 + 5
    composite.render(gbuffer, rc.giResult(), direct.result(), sdf.sdfTexture(),
                     window_size, debugMode, 1.0f, camera.isoEnabled);  // Pass 6

    // Leave a clean state for any forward overlays drawn afterward.
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DeferredRenderer::destroy() {
    initialized = false;
    // gbuffer/sdf/rc/direct/composite clean themselves up via their destructors.
}
