#version 430
//
// Radiance Cascades — merge (§8.3). Runs top-down: cascade c's merged value is
// its own interval radiance plus, where the ray stayed visible, the radiance
// continuing from cascade c+1. The upper lookup uses a bilinear weight over the
// 4 nearest upper probes (Bilinear Fix) and averages the 4 finer sub-directions
// that correspond to this cascade's coarser direction.
//
layout(local_size_x = 8, local_size_y = 8) in;

layout(rgba16f, binding = 0) uniform readonly  image2D uSelf;  // cascade c (raw)
layout(rgba16f, binding = 1) uniform readonly  image2D uUpper; // cascade c+1 (merged)
layout(rgba16f, binding = 2) uniform writeonly image2D uOut;   // cascade c (merged)

uniform ivec2 uSize;
uniform int   uBaseSpacing; // s0
uniform int   uCascade;     // c
uniform int   uHasUpper;
uniform int   uBilinearFix;

// Average the 4 finer directions of an upper probe that refine coarse direction
// `coarseDir`. Returns (rgb radiance, a visibility).
vec4 upperProbeDir(ivec2 uprobe, int su, int coarseDir, int Wu, int Hu) {
    uprobe = clamp(uprobe, ivec2(0), ivec2(Wu - 1, Hu - 1));
    vec4 acc = vec4(0.0);
    int fineBase = coarseDir * 4;
    for (int k = 0; k < 4; ++k) {
        int fine = fineBase + k;
        ivec2 tile = ivec2(fine % su, fine / su);
        acc += imageLoad(uUpper, uprobe * su + tile);
    }
    return acc * 0.25;
}

void main() {
    ivec2 p = ivec2(gl_GlobalInvocationID.xy);
    if (p.x >= uSize.x || p.y >= uSize.y) return;

    vec4 self = imageLoad(uSelf, p);
    if (uHasUpper == 0) {
        imageStore(uOut, p, self);
        return;
    }

    int sc = uBaseSpacing << uCascade;
    ivec2 probe = p / sc;
    ivec2 dir   = p % sc;
    int dirIndex = dir.y * sc + dir.x;

    int su = sc * 2;                 // upper spacing
    int Wu = uSize.x / su;
    int Hu = uSize.y / su;
    if (Wu < 1) Wu = 1;
    if (Hu < 1) Hu = 1;

    vec2 center = (vec2(probe) + 0.5) * float(sc);
    vec2 fp = center / float(su) - 0.5;   // fractional upper-probe coordinate

    vec4 upper;
    if (uBilinearFix == 1) {
        vec2 base = floor(fp);
        vec2 f = fp - base;
        ivec2 b = ivec2(base);
        vec4 c00 = upperProbeDir(b + ivec2(0, 0), su, dirIndex, Wu, Hu);
        vec4 c10 = upperProbeDir(b + ivec2(1, 0), su, dirIndex, Wu, Hu);
        vec4 c01 = upperProbeDir(b + ivec2(0, 1), su, dirIndex, Wu, Hu);
        vec4 c11 = upperProbeDir(b + ivec2(1, 1), su, dirIndex, Wu, Hu);
        upper = mix(mix(c00, c10, f.x), mix(c01, c11, f.x), f.y);
    } else {
        ivec2 nearest = ivec2(floor(fp + 0.5));
        upper = upperProbeDir(nearest, su, dirIndex, Wu, Hu);
    }

    vec3  mergedRgb = self.rgb + self.a * upper.rgb;
    float mergedVis = self.a * upper.a;
    imageStore(uOut, p, vec4(mergedRgb, mergedVis));
}
