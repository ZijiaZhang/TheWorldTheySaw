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
uniform mat3  uNormalToSurface;  // reorients tangent-space normal into screen+height space

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
    // Flip normal Y to match the lighting pass's gl_FragCoord space (Y up).
    // The world->screen projection (render.cpp: sy = 2/(top-bottom) < 0) reflects
    // local +Y to screen-DOWN, so a standard OpenGL +Y-up tangent-space normal
    // (e.g. demo_scene's domeNormal) must have its Y negated here; otherwise the
    // lit cap mirrors and orbits opposite to the light. n.x is correct (sx > 0),
    // and flat normals (n.y = 0) plus nz = sqrt(1 - dot(nxy,nxy)) are sign-invariant.
    // Applied after uNormalToSurface so per-sprite rotation (set uNormalToSurface =
    // the sprite's rotation matrix) still composes correctly.
    oNormalMat  = vec4(vec2(n.x, -n.y) * 0.5 + 0.5, uRoughness, 0.0);
    oHeight     = uBaseHeight + hmap * uHeightRange;
    oEmissive   = (uHasEmissive > 0.5) ? texture(uEmissiveMap, vUV) : vec4(0.0);
}
