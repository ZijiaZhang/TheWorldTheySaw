#version 330

// Radiance Cascades — cascade cast pass (design doc 6.2, contract 4.7).
// For the probe owning this texel, cast its direction over the near interval
// [tNear, tFar] by sphere-tracing the SDF. On hit, sample emissive (distance
// attenuated) and flag the hit; on miss, write zero so the merge pass pulls
// radiance from the upper cascade.
//
// Texture packing follows contract 2.1 EXACTLY:
//   D = 1 << c; texture is a D x D grid of angular tiles.
//   tileW = floor(W/D), tileH = floor(H/D).
//   tile (tx,ty) holds dirIndex = ty*D + tx for every probe.
//   probe grid cell (px,py) lives at texel (tx*tileW+px, ty*tileH+py).
//   probe screen position = (px+0.5, py+0.5) * probeSpacing  (pixels).
//   angle = 2*pi*(dirIndex+0.5)/dirCount; dir = vec2(cos,sin).

in vec2 vUV;

layout(location = 0) out vec4 oRadiance;   // RGBA16F: rgb radiance, a = hit flag

uniform sampler2D uSDF;            // unit 5: signed distance in pixels (+out,-in)
uniform sampler2D uGEmissive;     // unit 3: emissive (RGBA16F)

uniform vec2  uResolution;        // pixels (W,H)
uniform int   uCascadeIndex;      // c
uniform float uBaseProbeSpacing;  // px; spacing = base * 2^c
uniform int   uBaseDirCount;      // 4 -> dirCount = base * 4^c
uniform float uD0;                // cascade-0 ray length (px); interval = uD0*4^c
uniform float uIntervalNear;      // tNear (px) for this cascade
uniform int   uMaxSteps;          // sphere-trace cap
uniform float uEps;               // hit threshold (px)
uniform float uMinStep;           // min advance (px)

const float PI = 3.14159265358979323846;

void main() {
    // --- decode cascade geometry from the texel position (contract 2.1) ---
    int   c  = uCascadeIndex;
    int   D  = 1 << c;                                  // tiles per axis = 2^c
    float fD = float(D);

    int dirCount = uBaseDirCount;
    for (int i = 0; i < c; i++) dirCount *= 4;          // base * 4^c

    float probeSpacing = uBaseProbeSpacing * pow(2.0, float(c));

    // Integer tile dimensions (must match the writer's floor(W/D)).
    int tileW = int(uResolution.x) / D;
    int tileH = int(uResolution.y) / D;

    // This fragment's integer texel coords.
    ivec2 frag = ivec2(gl_FragCoord.xy);

    // Which angular tile this texel belongs to.
    int tx = (tileW > 0) ? (frag.x / tileW) : 0;
    int ty = (tileH > 0) ? (frag.y / tileH) : 0;
    // Clamp to valid tile range (right/top remainder columns fold into last tile).
    tx = clamp(tx, 0, D - 1);
    ty = clamp(ty, 0, D - 1);

    int dirIndex = ty * D + tx;

    // Probe grid cell within the tile.
    int px = frag.x - tx * tileW;
    int py = frag.y - ty * tileH;

    // Probe screen position (pixel center of the grid cell).
    vec2 probePos = (vec2(float(px), float(py)) + 0.5) * probeSpacing;

    // Direction for this dirIndex.
    float angle = 2.0 * PI * (float(dirIndex) + 0.5) / float(dirCount);
    vec2  dir   = vec2(cos(angle), sin(angle));

    // Interval for this cascade (contract 7): tFar = tNear + uD0 * 4^c.
    float interval = uD0 * pow(4.0, float(c));
    float tNear    = uIntervalNear;
    float tFar     = tNear + interval;

    // --- sphere-trace the SDF over [tNear, tFar] ---
    vec3  radiance = vec3(0.0);
    float hit      = 0.0;

    float t = tNear;
    for (int i = 0; i < uMaxSteps; i++) {
        if (t >= tFar) break;

        vec2 p = probePos + dir * t;

        // Outside the screen -> treat as no occluder/emitter ahead; stop.
        if (p.x < 0.0 || p.y < 0.0 || p.x > uResolution.x || p.y > uResolution.y) break;

        vec2  suv = p / uResolution;
        float d   = texture(uSDF, suv).r;      // distance to nearest occluder (px)

        if (d < uEps) {
            // Hit an occluder: emit its self-emission (distance-attenuated),
            // else the direction is simply blocked (radiance stays 0, hit=1).
            vec3  emis  = texture(uGEmissive, suv).rgb;
            // Surface emitters keep constant radiance with distance; RC's spatial
            // falloff comes from angular coverage across the cascade hierarchy.
            radiance = emis;
            hit      = 1.0;
            break;
        }

        t += max(d, uMinStep);                 // sphere tracing advance
    }

    oRadiance = vec4(radiance, hit);
}
