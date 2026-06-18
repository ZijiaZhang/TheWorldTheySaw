#version 330

// =====================================================================
// direct_light.fragment.glsl  (Pass 4 + Pass 5)
// Deferred spotlight "hero" light + height-field raymarched shadow. The
// flashlight exists ONLY here (it is NOT fed into Radiance Cascades).
//
// Two modes, gated by uWorldSpace:
//   uWorldSpace > 0.5 : TRUE WORLD-SPACE lighting (full design). Per pixel we
//     unproject (gl_FragCoord.xy + elevation) -> world P=(wx,wy,wz), decode a
//     full-xyz WORLD normal, and run real 3D Lambert + 3D-distance attenuation.
//   else : legacy screen+height pseudo-world path (unchanged).
// Output is straight RGBA16F direct radiance (rgb light, a = 1).
// =====================================================================

in vec2 vUV;

layout(location = 0) out vec4 oDirect;

// --- G-buffer inputs ---
uniform sampler2D uGNormal;              // unit 1: world normal xyz (or rg encoded, legacy)
uniform sampler2D uGHeight;              // unit 2: r = elevation (world units)

uniform vec2  uResolution;               // pixels (W, H)

uniform float uWorldSpace;               // 1.0 = world-space path

// --- world<->screen mapping (for unprojection); see Camera ---
uniform float uOx;                       // oblique_x_scale (tileW/2)
uniform float uOy;                       // oblique_y_scale (tileH/2)
uniform float uZScale;                   // heightScale
uniform vec2  uCenter;                   // screen center (px)
uniform vec2  uFocus;                    // camera focus (world)

// --- light ---
uniform vec3  uLightPos;                 // WORLD (wx,wy,wz)   [legacy: (px,px,height)]
uniform vec2  uLightFrag;                // light screen px (gl_FragCoord, y-up) for the shadow march
uniform vec3  uSpotDirWorld;             // world aim
uniform vec2  uSpotDir;                  // legacy screen aim
uniform float uCosInner;
uniform float uCosOuter;
uniform vec3  uLightColor;
uniform float uK1;
uniform float uK2;

// --- height-field shadow march ---
uniform float uShadowStepLen;            // march step (px)
uniform int   uShadowSteps;              // max steps
uniform float uShadowBias;               // height bias (world units)
uniform float uShadowStartBias;          // initial offset (px)

vec3 decodeNormalWorld(vec2 uv) { return normalize(texture(uGNormal, uv).rgb * 2.0 - 1.0); }
vec3 decodeNormalLegacy(vec2 uv) {
    vec2 nxy = texture(uGNormal, uv).rg * 2.0 - 1.0;
    float nz = sqrt(max(0.0, 1.0 - dot(nxy, nxy)));
    return vec3(nxy, nz);
}

// Inverse of the iso placement (gl_FragCoord y-up + elevation -> world).
vec3 unprojectWorld() {
    float wz = texture(uGHeight, gl_FragCoord.xy / uResolution).r;
    float sx = gl_FragCoord.x;
    float sy = uResolution.y - gl_FragCoord.y;          // y-up frag -> y-down game-unit
    float a = (sx - uCenter.x) / uOx;                   // (wx-fx) - (wy-fy)
    float b = (sy - uCenter.y + wz * uZScale) / uOy;    // (wx-fx) + (wy-fy)
    return vec3(0.5 * (a + b) + uFocus.x, 0.5 * (b - a) + uFocus.y, wz);
}

// March in SCREEN px toward the light's screen position, climbing by the light's
// elevation tangent; if the elevation field exceeds the ray height -> shadow.
float traceHeightShadow(vec2 fragXY, float surfWz) {
    vec2 toL = uLightFrag - fragXY;
    float fragLen = length(toL);
    if (fragLen < 1e-3) return 1.0;
    vec2 dir = toL / fragLen;
    float elevTan = (uLightPos.z - surfWz) / fragLen;   // world-height gained per screen px (approx)

    float rayHeight = surfWz;
    float t = uShadowStartBias;
    for (int i = 0; i < uShadowSteps; i++) {
        t += uShadowStepLen;
        vec2 sp = (fragXY + dir * t) / uResolution;
        if (any(lessThan(sp, vec2(0.0))) || any(greaterThan(sp, vec2(1.0)))) break;
        rayHeight += uShadowStepLen * elevTan;
        if (texture(uGHeight, sp).r > rayHeight + uShadowBias) return 0.0;
    }
    return 1.0;
}

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    bool world = uWorldSpace > 0.5;

    vec3  N  = world ? decodeNormalWorld(uv) : decodeNormalLegacy(uv);
    float wz = texture(uGHeight, uv).r;
    vec3  P  = world ? unprojectWorld() : vec3(gl_FragCoord.xy, wz);

    vec3  toL  = uLightPos - P;
    float dist = length(toL);
    if (dist < 1e-5) { oDirect = vec4(0.0, 0.0, 0.0, 1.0); return; }
    vec3 L = toL / dist;

    float ndotl = max(dot(N, L), 0.0);

    float cone = world
        ? smoothstep(uCosOuter, uCosInner, dot(-L, normalize(uSpotDirWorld)))
        : smoothstep(uCosOuter, uCosInner, dot(-L.xy, normalize(uSpotDir)));

    float atten = 1.0 / (1.0 + uK1 * dist + uK2 * dist * dist);   // d = world 3D distance

    float shadow = traceHeightShadow(gl_FragCoord.xy, wz);

    oDirect = vec4(uLightColor * ndotl * cone * atten * shadow, 1.0);
}
