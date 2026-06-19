#version 430
//
// Radiance Cascades — per-cascade interval ray-march (§8.2).
//
// Texel layout (base spacing s0, cascade c):
//   s_c   = s0 << c                  probe spacing & tile size (px)
//   probe = texel / s_c              which probe this texel belongs to
//   dir   = texel % s_c              direction tile within the probe
//   N_c   = s_c*s_c = 4^(c+1)        directions per probe
//
// Each ray sphere-traces the SDF over [tNear, tFar). On hitting an occluder it
// takes that occluder's emissive (rgb) and reports visibility = 0; if it reaches
// the far end unobstructed it reports radiance 0, visibility 1 (defer upward).
//
layout(local_size_x = 8, local_size_y = 8) in;

layout(rgba16f, binding = 0) uniform writeonly image2D uOut;

uniform sampler2D uSdf;       // R16F distance field, GI resolution
uniform sampler2D uEmissive;  // RGBA16F emissive
uniform ivec2 uSize;
uniform int   uBaseSpacing;   // s0
uniform float uBaseInterval;  // d0
uniform int   uCascade;       // c

const int   MAX_STEPS = 48;
const float HIT_EPS   = 0.75;
const float MIN_STEP  = 0.5;

void main() {
    ivec2 p = ivec2(gl_GlobalInvocationID.xy);
    if (p.x >= uSize.x || p.y >= uSize.y) return;

    int sc = uBaseSpacing << uCascade;
    ivec2 probe = p / sc;
    ivec2 dir   = p % sc;
    int Nc = sc * sc;
    int dirIndex = dir.y * sc + dir.x;

    vec2 center = (vec2(probe) + 0.5) * float(sc);
    float angle = 6.2831853 * (float(dirIndex) + 0.5) / float(Nc);
    vec2 rayDir = vec2(cos(angle), sin(angle));

    // Geometric intervals: tFar_c = d0 * (4^(c+1) - 1) / 3.
    float pow4c  = float(1 << (2 * uCascade));      // 4^c
    float tNear  = uBaseInterval * (pow4c - 1.0) / 3.0;
    float tFar   = uBaseInterval * (pow4c * 4.0 - 1.0) / 3.0;

    vec3  radiance = vec3(0.0);
    float visibility = 1.0;

    float t = tNear;
    for (int i = 0; i < MAX_STEPS; ++i) {
        if (t >= tFar) break;
        vec2 pos = center + rayDir * t;
        if (pos.x < 0.0 || pos.y < 0.0 || pos.x >= float(uSize.x) || pos.y >= float(uSize.y))
            break; // left the screen -> treat as open sky (stays visible)

        vec2 uv = pos / vec2(uSize);
        float d = texture(uSdf, uv).r;
        if (d < HIT_EPS) {
            radiance = texture(uEmissive, uv).rgb;
            visibility = 0.0;
            break;
        }
        t += max(d, MIN_STEP);
    }

    imageStore(uOut, p, vec4(radiance, visibility));
}
