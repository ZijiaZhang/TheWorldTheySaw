#version 430
//
// Pass 4 + 5 (§9, §10): deferred direct lighting for the "main" lights
// (flashlight, point lights). Everything is done in world space: the pixel's 3D
// position is unprojected from gl_FragCoord + height, the G-buffer normal is
// already world space. Adds spot cones, true 3D-distance attenuation, constant-
// view specular, and a height-field ray-march shadow per shadowing light.
//
// Output = albedo * Σ(diffuse·cone·atten·shadow) + Σ(specular·shadow), so the
// composite only has to add ambient GI and emissive.
//
in vec2 vUV;
layout(location = 0) out vec4 oDirect;

uniform sampler2D uAlbedoMask;
uniform sampler2D uNormalMat;
uniform sampler2D uHeight;
uniform sampler2D uEmissive;   // .a = coverage

uniform vec2  uScreenSize;
uniform vec2  uFocus;          // camera focus (world XY)
uniform float uKx, uKy, uKz;   // oblique projection scales
uniform vec3  uViewDir;        // constant view direction (specular)
uniform float uShininess;
uniform float uSpecStrength;

// Height-field shadow controls.
uniform float uShadowStepPx;
uniform float uShadowBias;
uniform int   uShadowSteps;

const int MAX_LIGHTS = 16;
uniform int   uLightCount;
uniform vec3  uLightPos[MAX_LIGHTS];
uniform vec3  uLightColor[MAX_LIGHTS]; // color * intensity
uniform vec3  uLightDir[MAX_LIGHTS];
uniform float uCosInner[MAX_LIGHTS];
uniform float uCosOuter[MAX_LIGHTS];
uniform float uK1[MAX_LIGHTS];
uniform float uK2[MAX_LIGHTS];
uniform int   uLightType[MAX_LIGHTS];   // 0 = point, 1 = spot
uniform int   uLightShadow[MAX_LIGHTS]; // 0/1

vec2 projectWorld(vec3 w) {
    vec2 c = uScreenSize * 0.5;
    float ex = w.x - uFocus.x;
    float ey = w.y - uFocus.y;
    return vec2(c.x + (ex - ey) * uKx, c.y + (ex + ey) * uKy + w.z * uKz);
}

vec2 unproject(vec2 s, float h) {
    vec2 c = uScreenSize * 0.5;
    float A = (s.x - c.x) / uKx;
    float B = (s.y - c.y - h * uKz) / uKy;
    float ex = 0.5 * (A + B);
    float ey = 0.5 * (B - A);
    return vec2(uFocus.x + ex, uFocus.y + ey);
}

// March across the height field toward the light, climbing by the light's
// elevation. If the terrain rises above the ray, the pixel is shadowed (§10).
float heightShadow(vec3 P, vec3 Lp, vec2 fragPx) {
    vec2 lightScreen = projectWorld(Lp);
    vec2 toLight = lightScreen - fragPx;
    float screenLen = length(toLight);
    if (screenLen < 1e-3) return 1.0;
    vec2 stepDir = toLight / screenLen;

    float horiz = max(length(Lp.xy - P.xy), 1e-3);
    float elevTan = (Lp.z - P.z) / horiz;
    float worldPerPx = horiz / screenLen;

    float rayHeight = P.z;
    float t = uShadowBias;
    for (int i = 0; i < uShadowSteps; ++i) {
        t += uShadowStepPx;
        if (t >= screenLen) break;               // reached the light
        vec2 sp = fragPx + stepDir * t;
        if (sp.x < 0.0 || sp.y < 0.0 || sp.x >= uScreenSize.x || sp.y >= uScreenSize.y) break;

        rayHeight += uShadowStepPx * worldPerPx * elevTan;
        float h = texture(uHeight, sp / uScreenSize).r;
        if (h > rayHeight + uShadowBias) return 0.0;
    }
    return 1.0;
}

void main() {
    float coverage = texture(uEmissive, vUV).a;
    if (coverage < 0.5) { oDirect = vec4(0.0); return; }

    vec3  albedo = texture(uAlbedoMask, vUV).rgb;
    vec4  nm     = texture(uNormalMat, vUV);
    vec3  N      = normalize(nm.xyz * 2.0 - 1.0);
    float height = texture(uHeight, vUV).r;

    vec2  fragPx = vUV * uScreenSize;
    vec3  P      = vec3(unproject(fragPx, height), height);
    vec3  V      = normalize(uViewDir);

    vec3 diffuseSum = vec3(0.0);
    vec3 specSum    = vec3(0.0);

    for (int i = 0; i < uLightCount; ++i) {
        vec3  toL  = uLightPos[i] - P;
        float dist = length(toL);
        if (dist < 1e-4) continue;
        vec3  L = toL / dist;

        float ndl = max(dot(N, L), 0.0);
        if (ndl <= 0.0) continue;

        float cone = 1.0;
        if (uLightType[i] == 1) {
            float a = dot(-L, normalize(uLightDir[i]));
            cone = smoothstep(uCosOuter[i], uCosInner[i], a);
            if (cone <= 0.0) continue;
        }

        float atten = 1.0 / (1.0 + uK1[i] * dist + uK2[i] * dist * dist);

        float shadow = 1.0;
        if (uLightShadow[i] == 1)
            shadow = heightShadow(P, uLightPos[i], fragPx);

        vec3  H    = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0), uShininess) * uSpecStrength;

        vec3 lit = uLightColor[i] * (cone * atten * shadow);
        diffuseSum += lit * ndl;
        specSum    += lit * spec;
    }

    oDirect = vec4(albedo * diffuseSum + specSum, 1.0);
}
