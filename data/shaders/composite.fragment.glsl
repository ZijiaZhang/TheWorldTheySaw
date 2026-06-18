#version 330

// =============================================================================
// composite.fragment.glsl  (Pass 6 - final combine + ACES tonemap)
// =============================================================================
// Full-screen pass. Vertex stage is the shared fullscreen.vertex.glsl
// (big-triangle, emits vUV in [0,1], origin bottom-left).
//
// Combines the deferred lighting buffers into the final image:
//     color = albedo * (gi + direct) + emissive
// then applies the Narkowicz ACES filmic tonemap and writes to FBO 0.
//
// Contract refs: LIGHTING_DESIGN_CONTRACT.md  ::4.10, ::5 (Pass 6), ::3 (units).
//
// Texture-unit map (NORMATIVE, contract ::3):
//     uGAlbedo   -> GL_TEXTURE0  (gbuffer0: rgb albedo, a occluder mask)
//     uGEmissive -> GL_TEXTURE3  (gbuffer3: rgb emissive)
//     uGI        -> GL_TEXTURE7  (giResult, half-res; bilinear upsample)
//     uDirect    -> GL_TEXTURE8  (directResult, flashlight contribution)
//
// uv convention: vUV from fullscreen.vertex.glsl == gl_FragCoord.xy/uResolution,
// origin bottom-left. The half-res uGI is upsampled implicitly by sampling at
// the same [0,1] vUV with GL_LINEAR filtering (contract ::0, ::2).
//
// DEVIATION FROM FROZEN CONTRACT (integrator must know):
//   uDebugMode (int) is ADDED beyond the contract ::4.10 uniform list, per the
//   author-phase task brief, to surface raw milestone buffers. It defaults to 0
//   (DEBUG_OFF = normal composite). When 0 the output is byte-identical to the
//   contract spec. Some debug modes need buffers this pass does not normally
//   bind (uGNormal on unit 1, uGHeight on unit 2, uSDF on unit 5); the C++
//   composite path binds these ONLY when uDebugMode selects them, and may leave
//   them unbound (reads return 0) when uDebugMode == 0. These three debug
//   samplers reuse their canonical contract ::3 units (1, 2, 5) with no
//   deviation in unit numbering.
//
// uDebugMode enum values:
//   0  DEBUG_OFF      normal composite (albedo*(gi+direct)+emissive, ACES)
//   1  DEBUG_ALBEDO   raw albedo.rgb (gbuffer0)            [unit 0]
//   2  DEBUG_NORMAL   decoded normal, remapped to [0,1]    [unit 1, uGNormal]
//   3  DEBUG_HEIGHT   height field, grayscale (uHeightDebugScale-normalized) [unit 2, uGHeight]
//   4  DEBUG_SDF      signed distance: green=outside, red=inside (uSdfDebugScale-normalized) [unit 5, uSDF]
//   5  DEBUG_GI       raw GI irradiance (uGI), no tonemap   [unit 7]
//   6  DEBUG_DIRECT   raw flashlight (uDirect), no tonemap  [unit 8]
//   7  DEBUG_EMISSIVE raw emissive.rgb (gbuffer3)           [unit 3]
//   8  DEBUG_OCCLUDER occluder mask (gbuffer0.a) as grayscale [unit 0]
// =============================================================================

// From fullscreen.vertex.glsl
in vec2 vUV;

// Output to default framebuffer (FBO 0)
layout(location = 0) out vec4 oColor;

// --- contract ::4.10 uniforms (NORMATIVE - do not rename) ---
uniform sampler2D uGAlbedo;     // unit 0 (albedo in rgb, occluder mask in a)
uniform sampler2D uGEmissive;   // unit 3 (emissive in rgb)
uniform sampler2D uGI;          // unit 7 (RC GI, half-res; bilinear upsample)
uniform sampler2D uDirect;      // unit 8 (flashlight result)
uniform float     uExposure;    // pre-tonemap multiply (default 1.0)

// --- DEVIATION: debug-only uniforms (see header) ---
uniform int   uDebugMode;        // 0 = off (normal composite). Default 0.
uniform sampler2D uGNormal;      // unit 1 (debug: decoded normal)   - bound only for DEBUG_NORMAL
uniform sampler2D uGHeight;      // unit 2 (debug: height field)     - bound only for DEBUG_HEIGHT
uniform sampler2D uSDF;          // unit 5 (debug: signed distance)  - bound only for DEBUG_SDF
uniform float uHeightDebugScale; // world-units mapped to white in DEBUG_HEIGHT (default 64.0)
uniform float uSdfDebugScale;    // pixels mapped to full color in DEBUG_SDF   (default 64.0)
uniform float uWorldSpace;       // 1.0 = gbuffer normal is full-xyz world space (DEBUG_NORMAL decode)

// Narkowicz 2015 ACES filmic fit. Operates per-channel; clamps to [0,1].
// a=2.51, b=0.03, c=2.43, d=0.59, e=0.14  (contract ::4.10).
vec3 acesTonemap(vec3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Decode screen+height-space normal from gbuffer1.rg (contract ::1.1).
vec3 decodeNormal(vec2 enc)
{
    vec2 nxy = enc * 2.0 - 1.0;                      // back to [-1,1]
    float nz = sqrt(max(0.0, 1.0 - dot(nxy, nxy)));  // reconstruct z >= 0
    return vec3(nxy, nz);
}

void main()
{
    // --- debug overrides (uDebugMode != 0) -----------------------------------
    if (uDebugMode != 0)
    {
        if (uDebugMode == 1) {                       // DEBUG_ALBEDO
            oColor = vec4(texture(uGAlbedo, vUV).rgb, 1.0);
        } else if (uDebugMode == 2) {                // DEBUG_NORMAL
            vec3 N = (uWorldSpace > 0.5)
                ? normalize(texture(uGNormal, vUV).rgb * 2.0 - 1.0)  // full-xyz world normal
                : decodeNormal(texture(uGNormal, vUV).rg);
            oColor = vec4(N * 0.5 + 0.5, 1.0);       // remap [-1,1] -> [0,1]
        } else if (uDebugMode == 3) {                // DEBUG_HEIGHT
            float h = texture(uGHeight, vUV).r;
            float g = clamp(h / max(uHeightDebugScale, 1e-4), 0.0, 1.0);
            oColor = vec4(vec3(g), 1.0);
        } else if (uDebugMode == 4) {                // DEBUG_SDF
            float sd = texture(uSDF, vUV).r;
            float m  = clamp(abs(sd) / max(uSdfDebugScale, 1e-4), 0.0, 1.0);
            // green ramp outside (sd>=0), red ramp inside (sd<0)
            vec3 col = (sd >= 0.0) ? vec3(0.0, m, 0.0) : vec3(m, 0.0, 0.0);
            oColor = vec4(col, 1.0);
        } else if (uDebugMode == 5) {                // DEBUG_GI
            oColor = vec4(texture(uGI, vUV).rgb, 1.0);
        } else if (uDebugMode == 6) {                // DEBUG_DIRECT
            oColor = vec4(texture(uDirect, vUV).rgb, 1.0);
        } else if (uDebugMode == 7) {                // DEBUG_EMISSIVE
            oColor = vec4(texture(uGEmissive, vUV).rgb, 1.0);
        } else if (uDebugMode == 8) {                // DEBUG_OCCLUDER
            float occ = texture(uGAlbedo, vUV).a;
            oColor = vec4(vec3(occ), 1.0);
        } else {
            // unknown debug code -> magenta error tint
            oColor = vec4(1.0, 0.0, 1.0, 1.0);
        }
        return;
    }

    // --- normal composite (contract ::4.10) ----------------------------------
    vec3 albedo   = texture(uGAlbedo,   vUV).rgb;
    vec3 gi       = texture(uGI,        vUV).rgb;   // bilinear upsample from half-res
    vec3 direct   = texture(uDirect,    vUV).rgb;
    vec3 emissive = texture(uGEmissive, vUV).rgb;

    vec3 color = albedo * (gi + direct) + emissive;
    color *= uExposure;

    oColor = vec4(acesTonemap(color), 1.0);          // ACES, no sRGB encode
}
