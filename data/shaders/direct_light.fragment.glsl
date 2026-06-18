#version 330

// =====================================================================
// direct_light.fragment.glsl  (Pass 4 + Pass 5)
// Deferred spotlight "hero" light (design §7) multiplied by a height-field
// raymarched shadow (design §8). The flashlight exists ONLY here
// (iron rule 1: the flashlight is NOT fed into Radiance Cascades).
//
// Working space P = (gl_FragCoord.xy in PIXELS, height in WORLD UNITS).
// uv = gl_FragCoord.xy / uResolution (origin bottom-left).
// Output is straight RGBA16F direct radiance (rgb light, a = 1).
// =====================================================================

in vec2 vUV;

layout(location = 0) out vec4 oDirect;   // RGBA16F: rgb flashlight, a = 1

// --- G-buffer inputs (fixed texture units per contract §3) ---
uniform sampler2D uGNormal;              // unit 1: rg encoded normal.xy, b roughness
uniform sampler2D uGHeight;              // unit 2: r = height (world units)

uniform vec2  uResolution;               // pixels (W, H)

// --- Spotlight (screen+height space) ---
uniform vec3  uLightPos;                 // (px, px, height_world)
uniform vec2  uSpotDir;                  // screen-plane aim (normalized)
uniform float uCosInner;                 // cos(inner half-angle)
uniform float uCosOuter;                 // cos(outer half-angle)
uniform vec3  uLightColor;               // linear intensity
uniform float uK1;                       // linear attenuation
uniform float uK2;                       // quadratic attenuation

// --- Height-field shadow march ---
uniform float uShadowStepLen;            // march step (px)
uniform int   uShadowSteps;              // max steps
uniform float uShadowBias;               // height bias (world units)
uniform float uShadowStartBias;          // initial offset (px)

// Decode normal from GBuffer1 (screen+height space, z reconstructed >= 0).
vec3 decodeNormal(vec2 uv)
{
    vec2 nxy = texture(uGNormal, uv).rg * 2.0 - 1.0;   // back to [-1,1]
    float nz = sqrt(max(0.0, 1.0 - dot(nxy, nxy)));    // reconstruct z >= 0
    return vec3(nxy, nz);                              // already unit-length
}

// Pass 5 — 3D-feel shadow (design §8).
// March in screen space toward the light; the ray climbs by the light's
// elevation tangent. If the scene height anywhere along the ray exceeds the
// ray height (+bias), this pixel is occluded -> shadow.
//
//   P            : (gl_FragCoord.xy px, height world) of the shaded pixel.
//   dirToLightXY : normalized screen-plane direction toward the light.
//   invLen       : 1 / horizontal distance (px) from P.xy to the light.
float traceHeightShadow(vec3 P, vec2 dirToLightXY, float invLen)
{
    // Elevation tangent: world-height gained per pixel of screen travel.
    float elevTan = (uLightPos.z - P.z) * invLen;

    float rayHeight = P.z;
    float t = uShadowStartBias;          // skip the surface itself (px)

    for (int i = 0; i < uShadowSteps; i++)
    {
        t += uShadowStepLen;

        // Sample point in pixel space, then to uv (origin bottom-left).
        vec2 sp_px = P.xy + dirToLightXY * t;
        vec2 sp    = sp_px / uResolution;

        // Off-screen: nothing left to occlude us; stop marching, stay lit.
        if (any(lessThan(sp, vec2(0.0))) || any(greaterThan(sp, vec2(1.0))))
            break;

        rayHeight += uShadowStepLen * elevTan;     // climb toward the light

        float h = texture(uGHeight, sp).r;         // scene height here
        if (h > rayHeight + uShadowBias)
            return 0.0;                            // blocked by a taller object
    }
    return 1.0;                                    // reached the light unblocked
}

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;

    vec3 N = decodeNormal(uv);
    vec3 P = vec3(gl_FragCoord.xy, texture(uGHeight, uv).r);   // screen+height

    // Vector to the light in screen+height space.
    vec3  toL  = uLightPos - P;
    float dist = length(toL);

    // Degenerate (light exactly on the pixel): no direction; emit nothing.
    if (dist < 1e-5)
    {
        oDirect = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 L = toL / dist;

    // Lambert term against the decoded surface normal.
    float ndotl = max(dot(N, L), 0.0);

    // Cone soft edge. Contract: use the SCREEN-PLANE aim (2D).
    // -L.xy points from the light toward the pixel on screen; compare it to
    // the spotlight's screen aim direction.
    vec2  aim   = normalize(uSpotDir);
    float theta = dot(-L.xy, aim);
    float cone  = smoothstep(uCosOuter, uCosInner, theta);

    // Distance attenuation (full 3D distance, includes the height term).
    float atten = 1.0 / (1.0 + uK1 * dist + uK2 * dist * dist);

    // Height-field shadow (Pass 5). Horizontal distance to the light.
    vec2  horiz   = uLightPos.xy - P.xy;
    float horizLen = length(horiz);
    float shadow = 1.0;
    if (horizLen > 1e-5)
    {
        vec2 dirToLightXY = horiz / horizLen;
        shadow = traceHeightShadow(P, dirToLightXY, 1.0 / horizLen);
    }
    // (If the light is directly overhead with no horizontal offset, there is
    //  no screen-space direction to march; leave shadow = 1.)

    vec3 direct = uLightColor * ndotl * cone * atten * shadow;

    oDirect = vec4(direct, 1.0);
}
