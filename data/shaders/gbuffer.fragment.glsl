#version 330

// Geometry pass (Pass 1) fragment shader.
// Writes the 4-attachment G-buffer (MRT) in the exact formats/locations the
// contract specifies:
//   loc 0  RGBA8    rgb = albedo (linear), a = occluder mask
//   loc 1  RGBA8    rg  = encode(normal.xy) = normal.xy*0.5+0.5, b = roughness, a spare
//   loc 2  R16F     r   = height (world units), in space P.z
//   loc 3  RGBA16F  rgb = emissive (linear radiance), a spare
//
// Normals are expressed in "screen + height" space:
//   +x = screen-right, +y = screen-up, +z = elevation (height axis).
// The per-sprite tangent-space normal map is reoriented into this basis by
// uNormalToSurface (identity for ground-aligned art).

in vec2 vUV;

layout(location = 0) out vec4 oAlbedoMask; // rgb albedo, a occluder mask
layout(location = 1) out vec4 oNormalMat;  // rg encoded normal.xy, b roughness, a spare
layout(location = 2) out float oHeight;    // height (world units) -> R16F target
layout(location = 3) out vec4 oEmissive;   // rgb emissive, a spare

uniform sampler2D uAlbedo;       // unit 0
uniform sampler2D uNormalMap;    // unit 1
uniform sampler2D uHeightMap;    // unit 2
uniform sampler2D uEmissiveMap;  // unit 3

uniform float uIsOccluder;       // 1.0 -> writes 1 into albedo.a, else 0.0
uniform float uRoughness;        // [0,1] stored into normal.b
uniform float uBaseHeight;       // world units
uniform float uHeightRange;      // world units multiplied by heightmap.r
uniform float uHasHeightMap;     // 1.0 if uHeightMap bound, else 0.0
uniform float uHasEmissive;      // 1.0 if uEmissiveMap bound, else 0.0
uniform mat3  uNormalToSurface;  // tangent->surface; world-space path: tangent->WORLD (surface TBN)
uniform float uWorldSpace;       // 1.0 = store full-xyz WORLD normal; else legacy screen+height

void main()
{
    vec4 alb = texture(uAlbedo, vUV);
    if (alb.a < 0.5) discard;                       // alpha clip

    // Tangent-space normal -> screen+height space.
    vec3 nt = texture(uNormalMap, vUV).xyz * 2.0 - 1.0;
    vec3 n  = normalize(uNormalToSurface * nt);

    // Height (world units): base elevation + optional per-texel heightmap.
    float hmap = (uHasHeightMap > 0.5) ? texture(uHeightMap, vUV).r : 0.0;

    oAlbedoMask = vec4(alb.rgb, uIsOccluder);
    // Normal encode. WORLD-space path (uWorldSpace): uNormalToSurface is the per-sprite
    // tangent->WORLD surface TBN (ground sprites bake the flat normal to world +Z, wall
    // sprites to world-horizontal; design 3.2), so n is already the world normal — store
    // its full xyz (z is kept, not reconstructed: near-horizontal wall normals are unstable
    // under z-reconstruct). LEGACY pseudo-world path keeps the screen+height encode with Y
    // negated for projection_2D sy<0.
    if (uWorldSpace > 0.5)
        oNormalMat = vec4(n * 0.5 + 0.5, uRoughness);
    else
        oNormalMat = vec4(vec2(n.x, -n.y) * 0.5 + 0.5, uRoughness, 0.0);
    oHeight     = uBaseHeight + hmap * uHeightRange;
    oEmissive   = (uHasEmissive > 0.5) ? texture(uEmissiveMap, vUV) : vec4(0.0);
}
