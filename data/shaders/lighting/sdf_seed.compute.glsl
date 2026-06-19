#version 430
//
// JFA seed (§7 step 1): occluder pixels seed their own coordinates; everything
// else gets an "invalid" sentinel of (-1, -1).
//
layout(local_size_x = 8, local_size_y = 8) in;

layout(rg32f, binding = 0) uniform writeonly image2D uSeed;

uniform sampler2D uMask;  // GBuffer0: rgb = albedo, a = occluder mask
uniform ivec2 uSize;

void main() {
    ivec2 p = ivec2(gl_GlobalInvocationID.xy);
    if (p.x >= uSize.x || p.y >= uSize.y) return;

    vec2 uv = (vec2(p) + 0.5) / vec2(uSize);
    float occ = texture(uMask, uv).a;

    vec2 seed = (occ > 0.5) ? vec2(p) : vec2(-1.0);
    imageStore(uSeed, p, vec4(seed, 0.0, 0.0));
}
