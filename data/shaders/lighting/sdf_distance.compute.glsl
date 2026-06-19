#version 430
//
// JFA distance (§7 step 3): convert nearest-seed coordinates to a scalar
// distance field (in GI pixels). Pixels with no seed get a large distance.
//
layout(local_size_x = 8, local_size_y = 8) in;

layout(rg32f, binding = 0) uniform readonly  image2D uSeed;
layout(r16f,  binding = 1) uniform writeonly image2D uSdf;

uniform ivec2 uSize;

void main() {
    ivec2 p = ivec2(gl_GlobalInvocationID.xy);
    if (p.x >= uSize.x || p.y >= uSize.y) return;

    vec2 seed = imageLoad(uSeed, p).xy;
    float d = (seed.x < 0.0) ? 65000.0 : distance(vec2(p), seed);
    imageStore(uSdf, p, vec4(d, 0.0, 0.0, 0.0));
}
