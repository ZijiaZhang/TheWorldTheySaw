#pragma once
//
// Orchestrates the full pseudo-3D deferred lighting pipeline (§4):
//   Pass 1 geometry -> Pass 2 SDF(JFA) -> Pass 3 RC GI
//   -> Pass 4/5 direct spotlight + height shadows -> Pass 6 composite
//
// Drives ECS components: every entity with a Motion + LitSprite is rendered into
// the G-buffer; every Light contributes to the direct pass. The composite is
// written to whatever framebuffer is bound on entry (the back buffer).
//
#include "common.hpp"
#include "tiny_ecs.hpp"
#include "LightingComponents.hpp"
#include "GBuffer.hpp"
#include "SdfPass.hpp"
#include "RadianceCascades.hpp"

class DeferredLighting {
public:
    void init(int screenW, int screenH);
    void destroy();

    // Run all passes for the given camera focus (world XY) and framebuffer size,
    // compositing to the currently bound framebuffer. debugMode 0 = final image.
    void render(vec2 focus, vec2 screenSize, int debugMode);

    ObliqueProjection& projection() { return proj_; }
    RadianceCascades&  cascades()   { return rc_; }

    // ---- Look / tuning (safe to change per frame) ----
    float giScale     = 0.5f;                 // GI/SDF resolution fraction
    vec3  viewDir     = {0.0f, -0.45f, 1.0f}; // constant specular view dir
    float shininess   = 24.0f;
    float specStrength= 0.5f;
    vec3  ambient     = {0.05f, 0.06f, 0.08f};
    vec3  background  = {0.035f, 0.055f, 0.06f};
    float aoRadius    = 18.0f;                // contact-AO radius (GI px)
    float shadowStepPx= 3.0f;
    float shadowBias  = 0.75f;
    int   shadowSteps = 64;

private:
    void resize(int screenW, int screenH);
    void geometryPass(vec2 focus, vec2 screenSize);
    void directPass(vec2 focus, vec2 screenSize);
    void compositePass(vec2 screenSize, int debugMode);
    void bindSpriteTexture(int unit, GLuint tex, GLuint fallback, const char* uniform);

    GBuffer          gbuffer_;
    SdfPass          sdf_;
    RadianceCascades rc_;
    ObliqueProjection proj_;

    GLuint geomProgram_      = 0;
    GLuint directProgram_    = 0;
    GLuint compositeProgram_ = 0;

    GLuint quadVao_ = 0, quadVbo_ = 0, quadIbo_ = 0;
    GLuint emptyVao_ = 0;

    GLuint directFbo_ = 0;
    GLuint directTex_ = 0;

    GLuint whiteTex_ = 0;       // default albedo / height
    GLuint flatNormalTex_ = 0;  // default normal

    int screenW_ = 0, screenH_ = 0;
    int giW_ = 0, giH_ = 0;
};
