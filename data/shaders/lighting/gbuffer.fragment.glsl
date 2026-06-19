#version 430
//
// Geometry pass fragment shader (§6). Writes the 4-MRT G-buffer and — the heart
// of correct oblique normal mapping (§3.2) — rotates the tangent-space normal
// into world space with the per-sprite surface TBN (floor -> +Z, wall -> facing
// outward). Emissive alpha doubles as a 1-bit coverage flag for later passes.
//
in vec2 vUV;

layout(location = 0) out vec4  oAlbedoMask; // rgb albedo, a occluder mask
layout(location = 1) out vec4  oNormalMat;  // rgb world normal (encoded), a roughness
layout(location = 2) out float oHeight;     // elevation
layout(location = 3) out vec4  oEmissive;   // rgb emissive, a coverage

uniform sampler2D uAlbedo;
uniform sampler2D uNormal;
uniform sampler2D uHeight;
uniform sampler2D uEmissive;

uniform mat3  uSurfaceTBN;     // tangent space -> world space
uniform float uRoughness;
uniform float uBaseHeight;
uniform float uHeightRange;
uniform float uIsOccluder;
uniform vec3  uTint;
uniform vec3  uEmissiveColor;

void main() {
    vec4 alb = texture(uAlbedo, vUV);
    if (alb.a < 0.5) discard;                 // alpha clip (works with painter order)

    vec3 nT = texture(uNormal, vUV).xyz * 2.0 - 1.0;
    vec3 nW = normalize(uSurfaceTBN * nT);

    float h = uBaseHeight + texture(uHeight, vUV).r * uHeightRange;
    vec3  emis = texture(uEmissive, vUV).rgb * uEmissiveColor;

    oAlbedoMask = vec4(alb.rgb * uTint, uIsOccluder);
    oNormalMat  = vec4(nW * 0.5 + 0.5, uRoughness);
    oHeight     = h;
    oEmissive   = vec4(emis, 1.0);            // a = 1 marks covered pixels
}
