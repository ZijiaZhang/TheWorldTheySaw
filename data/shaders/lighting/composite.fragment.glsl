#version 430
//
// Pass 6 (§11): final composite. Implements
//   color = albedo * (ambientGI*ao + ambient) + direct + emissive
// (direct already carries albedo*diffuse + specular, both shadowed, from Pass 4/5)
// followed by an ACES tonemap. A debug switch can show any intermediate buffer.
//
in vec2 vUV;
layout(location = 0) out vec4 fragColor;

uniform sampler2D uAlbedoMask; // rgb albedo, a occluder mask
uniform sampler2D uNormalMat;  // rgb world normal, a roughness
uniform sampler2D uHeight;
uniform sampler2D uSdf;        // GI-res distance field
uniform sampler2D uGi;         // GI-res ambient irradiance
uniform sampler2D uDirect;     // full-res direct lighting
uniform sampler2D uEmissive;   // rgb emissive, a coverage

uniform vec3  uAmbient;        // flat ambient floor so shadows aren't pure black
uniform vec3  uBackground;     // color for uncovered pixels
uniform float uAoRadius;       // contact-AO radius in GI pixels
uniform float uHeightVizScale; // debug only
uniform int   uDebugMode;

vec3 aces(vec3 x) {
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec4  alb   = texture(uAlbedoMask, vUV);
    vec3  gi    = texture(uGi, vUV).rgb;
    vec3  direct= texture(uDirect, vUV).rgb;
    vec4  emis  = texture(uEmissive, vUV);
    float sdfD  = texture(uSdf, vUV).r;
    float coverage = emis.a;

    // Contact AO from the distance field: darken close to occluders.
    float ao = clamp(sdfD / max(uAoRadius, 1e-3), 0.0, 1.0);

    // ---- Debug views (toggled with number keys) ----
    if (uDebugMode == 1) { fragColor = vec4(alb.rgb, 1.0); return; }
    if (uDebugMode == 2) { fragColor = vec4(texture(uNormalMat, vUV).rgb, 1.0); return; }
    if (uDebugMode == 3) { float h = texture(uHeight, vUV).r; fragColor = vec4(vec3(h * uHeightVizScale), 1.0); return; }
    if (uDebugMode == 4) { fragColor = vec4(vec3(clamp(sdfD / 64.0, 0.0, 1.0)), 1.0); return; }
    if (uDebugMode == 5) { fragColor = vec4(aces(gi), 1.0); return; }
    if (uDebugMode == 6) { fragColor = vec4(aces(direct), 1.0); return; }
    if (uDebugMode == 7) { fragColor = vec4(emis.rgb, 1.0); return; }
    if (uDebugMode == 8) { fragColor = vec4(vec3(alb.a), 1.0); return; }

    if (coverage < 0.5) {
        fragColor = vec4(uBackground, 1.0);
        return;
    }

    vec3 color = alb.rgb * (gi * ao + uAmbient) + direct + emis.rgb;
    fragColor = vec4(aces(color), 1.0);
}
