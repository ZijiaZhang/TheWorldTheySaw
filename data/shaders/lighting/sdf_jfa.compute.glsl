#version 430
//
// JFA flood (§7 step 2): one jump-flood iteration at the given step size. Each
// pixel samples the 8 neighbours at +/- uStep and keeps the nearest valid seed
// (Voronoi propagation). Run for steps N/2, N/4, ... , 1.
//
layout(local_size_x = 8, local_size_y = 8) in;

layout(rg32f, binding = 0) uniform readonly  image2D uSeedIn;
layout(rg32f, binding = 1) uniform writeonly image2D uSeedOut;

uniform ivec2 uSize;
uniform int   uStep;

void main() {
    ivec2 p = ivec2(gl_GlobalInvocationID.xy);
    if (p.x >= uSize.x || p.y >= uSize.y) return;

    vec2  best = vec2(-1.0);
    float bestDist = 1e30;

    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            ivec2 q = p + ivec2(dx, dy) * uStep;
            if (q.x < 0 || q.y < 0 || q.x >= uSize.x || q.y >= uSize.y) continue;

            vec2 seed = imageLoad(uSeedIn, q).xy;
            if (seed.x < 0.0) continue; // invalid

            float d = distance(vec2(p), seed);
            if (d < bestDist) {
                bestDist = d;
                best = seed;
            }
        }
    }

    imageStore(uSeedOut, p, vec4(best, 0.0, 0.0));
}
