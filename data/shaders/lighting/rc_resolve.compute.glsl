#version 430
//
// Radiance Cascades — resolve (§8.4). Integrate the fully merged cascade C0 over
// direction to get per-pixel incoming irradiance (the "ambient GI" the composite
// pass multiplies into albedo). Bilinear over the 4 nearest C0 probes.
//
layout(local_size_x = 8, local_size_y = 8) in;

layout(rgba16f, binding = 0) uniform readonly  image2D uC0; // merged cascade 0
layout(rgba16f, binding = 1) uniform writeonly image2D uGi;

uniform ivec2 uSize;
uniform int   uBaseSpacing; // s0
uniform float uGain;

// Mean radiance over all directions of one C0 probe.
vec3 probeIrradiance(ivec2 probe, int s0, int W0, int H0) {
    probe = clamp(probe, ivec2(0), ivec2(W0 - 1, H0 - 1));
    vec3 acc = vec3(0.0);
    int N0 = s0 * s0;
    for (int dy = 0; dy < s0; ++dy)
        for (int dx = 0; dx < s0; ++dx)
            acc += imageLoad(uC0, probe * s0 + ivec2(dx, dy)).rgb;
    return acc / float(N0);
}

void main() {
    ivec2 q = ivec2(gl_GlobalInvocationID.xy);
    if (q.x >= uSize.x || q.y >= uSize.y) return;

    int s0 = uBaseSpacing;
    int W0 = uSize.x / s0;
    int H0 = uSize.y / s0;
    if (W0 < 1) W0 = 1;
    if (H0 < 1) H0 = 1;

    vec2 fp = (vec2(q) + 0.5) / float(s0) - 0.5;
    vec2 base = floor(fp);
    vec2 f = fp - base;
    ivec2 b = ivec2(base);

    vec3 i00 = probeIrradiance(b + ivec2(0, 0), s0, W0, H0);
    vec3 i10 = probeIrradiance(b + ivec2(1, 0), s0, W0, H0);
    vec3 i01 = probeIrradiance(b + ivec2(0, 1), s0, W0, H0);
    vec3 i11 = probeIrradiance(b + ivec2(1, 1), s0, W0, H0);
    vec3 irr = mix(mix(i00, i10, f.x), mix(i01, i11, f.x), f.y);

    imageStore(uGi, q, vec4(irr * uGain, 1.0));
}
