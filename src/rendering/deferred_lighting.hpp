#pragma once
//
// Dynamic deferred lighting pipeline (G-buffer -> JFA SDF -> Radiance Cascades GI
// -> deferred spotlight + height-field shadows -> composite). Implemented as
// full-screen fragment passes on OpenGL 3.3 core per LIGHTING_DESIGN_CONTRACT.md.
//
#include "common.hpp"
#include "render_components.hpp"
#include "Camera.hpp"

// ---- ECS component consumed by the geometry pass -------------------------------------------
// One LitSprite per drawable. The geometry pass (Pass 1) reads these textures + scalars and
// writes the 4 MRT outputs. baseHeight/heightRange feed GBuffer2; isOccluder feeds GBuffer0.a.
struct LitSprite {
    Texture albedo;        // required; .a alpha-clips
    Texture normal;        // optional; bind flat (0.5,0.5,1) if absent
    Texture height;        // optional; per-texel height in [0,1] -> scaled by heightRange
    Texture emissive;      // optional; linear emissive radiance
    float   baseHeight   = 0.0f;  // world units, uBaseHeight
    float   heightRange  = 0.0f;  // world units, uHeightRange (0 => flat)
    float   roughness    = 1.0f;  // [0,1] -> GBuffer1.b
    bool    isOccluder   = false; // true => writes 1.0 into GBuffer0.a (blocks light)
    bool    hasHeightMap = false; // -> uHasHeightMap
    bool    hasEmissive  = false; // -> uHasEmissive
    mat3    normalToSurface = mat3(1.0f); // -> uNormalToSurface (identity = ground-aligned)
};

// Draws the shared full-screen triangle (binds a lazily-created dummy VAO and issues
// glDrawArrays(GL_TRIANGLES,0,3)). The program + uniforms must already be bound.
void drawFullScreenTriangle();

// Create a color texture (raw GLuint, caller owns/deletes). filter = GL_NEAREST or GL_LINEAR.
GLuint makeColorTexture(ivec2 size, GLenum internalFormat, GLenum format, GLenum type, GLenum filter);

// Ping-pong pair of identically-formatted color textures behind two FBOs.
struct PingPong {
    GLuint tex[2] = {0, 0};
    GLuint fbo[2] = {0, 0};
    int cur = 0;                                  // index of the "current"/read texture
    void create(ivec2 size, GLenum internalFormat, GLenum format, GLenum type, GLenum filter);
    GLuint readTex()  const { return tex[cur]; }
    GLuint writeFbo() const { return fbo[cur ^ 1]; }
    GLuint writeTex() const { return tex[cur ^ 1]; }
    void   swap() { cur ^= 1; }
    void   destroy();
};

// ---- pass / resource modules ----------------------------------------------------------------
class GBuffer {
public:
    void create(ivec2 size);
    void bindForWrite();          // bind FBO + glDrawBuffers(4) + set viewport
    GLuint albedoMask() const { return g0; }
    GLuint normalMat()  const { return g1; }
    GLuint height()     const { return g2; }
    GLuint emissive()   const { return g3; }
    ivec2  size() const { return dim; }
    void destroy();
    ~GBuffer() { destroy(); }
private:
    GLuint g0 = 0, g1 = 0, g2 = 0, g3 = 0;
    GLuint fbo = 0, depthStencilRbo = 0;
    ivec2 dim{0,0};
};

class SdfPass {
public:
    void create(ivec2 size);
    void generate(const GBuffer& gbuffer); // jfa_seed -> log2 jfa_step rounds -> sdf_distance
    GLuint sdfTexture() const { return sdfDist; }
    void destroy();
    ~SdfPass() { destroy(); }
private:
    PingPong seeds;               // RG16F, GL_NEAREST
    GLuint sdfDist = 0;           // R16F
    GLuint sdfFbo = 0;
    Effect seedFx, stepFx, distFx;
    ivec2 dim{0,0};
};

class RadianceCascades {
public:
    struct Params {
        int   numCascades      = 4;
        float baseProbeSpacing = 2.0f;
        int   baseDirCount     = 4;
        float d0               = 16.0f;
        int   maxSteps         = 32;
        float eps              = 1.0f;
        float minStep          = 0.5f;
        bool  halfResGI        = true;
        bool  bilinearFix      = true;
    };
    void create(ivec2 size, const Params& p);
    void compute(const SdfPass& sdf, const GBuffer& gbuffer);
    GLuint giResult() const { return gi; }
    const Params& params() const { return prm; }
    void destroy();
    ~RadianceCascades() { destroy(); }
private:
    PingPong cascades;            // RGBA16F, GL_LINEAR (full-res)
    GLuint gi = 0;                // RGBA16F (half-res)
    GLuint giFbo = 0;
    Effect castFx, mergeFx;
    Params prm;
    ivec2 dim{0,0};
    ivec2 giDim{0,0};
};

struct Spotlight; // fwd

class DirectLightPass {
public:
    void create(ivec2 size);
    void render(const GBuffer& gbuffer, const Spotlight& flashlight);
    GLuint result() const { return directResult; }
    void destroy();
    ~DirectLightPass() { destroy(); }
private:
    GLuint directResult = 0;      // RGBA16F
    GLuint fbo = 0;
    Effect fx;                    // fullscreen + direct_light
    ivec2 dim{0,0};
};

class CompositePass {
public:
    void create();                // no owned target (renders to FBO 0)
    void render(const GBuffer& gbuffer, GLuint giTex, GLuint directTex, GLuint sdfTex, ivec2 screenSize,
                int debugMode = 0, float exposure = 1.0f);
    void destroy();
    ~CompositePass() { destroy(); }
private:
    Effect fx;                    // fullscreen + composite
};

// ---- lights ---------------------------------------------------------------------------------
// Flashlight hero light. All positional data in "screen + height" space (px, px, world height).
struct Spotlight {
    vec3  pos      = {0, 0, 64};   // (px, px, height)
    vec2  dir      = {1, 0};       // screen-plane aim (will be normalized)
    float cosInner = 0.96f;        // ~16 deg half-angle
    float cosOuter = 0.86f;        // ~30 deg half-angle
    vec3  color    = {1, 1, 1};    // linear intensity
    float k1       = 0.0f;         // linear attenuation
    float k2       = 0.0005f;      // quadratic attenuation
    // height-field shadow march
    float shadowStepLen   = 3.0f;
    int   shadowSteps     = 96;
    float shadowBias      = 1.0f;
    float shadowStartBias = 2.0f;
};

class LightManager {
public:
    Spotlight flashlight;             // the single hero light (iron rule 1)
};

// ---- top-level renderer ---------------------------------------------------------------------
class DeferredRenderer {
public:
    void init(ivec2 framebufferSize, const RadianceCascades::Params& rcParams = {});
    // Runs Pass 1..6 for the frame over all entities carrying LitSprite + Motion.
    // window_size is the framebuffer pixel size; camera supplies world->screen for Pass 1.
    void draw(const Camera& camera, ivec2 window_size, const mat3& projection_2D);
    void destroy();
    ~DeferredRenderer() { destroy(); }

    LightManager lights;
    int debugMode = 0;            // composite debug switch (0 = full pipeline)
private:
    void geometryPass(const Camera& camera, const mat3& projection_2D);

    GBuffer          gbuffer;
    SdfPass          sdf;
    RadianceCascades rc;
    DirectLightPass  direct;
    CompositePass    composite;
    Effect           gbufferFx;       // gbuffer.vertex + gbuffer.fragment
    ShadedMesh       quad;            // shared unit quad reused for every LitSprite
    Texture          flatNormal;      // (0.5,0.5,1) fallback normal
    Texture          whiteTex;        // (1,1,1,1) fallback albedo/height/emissive
    ivec2            dim{0,0};
    bool             initialized = false;
};
