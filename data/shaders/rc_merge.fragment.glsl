#version 330

// Radiance Cascades — merge pass with the Osborne & Sannikov (2024) bilinear fix
// (design doc 6.3, contract 4.8). Produced TOP-DOWN: highest cascade first
// (merged against a black upper), each lower cascade merged against the
// already-merged cascade N+1. The N==0 merge renders into the half-res giResult.
//
// For cascade N's texel we recompute this cascade's near interval exactly as in
// rc_cascade.fragment.glsl. If that near interval HIT an occluder/emitter we keep
// that radiance (the far field is occluded). On MISS we fetch the continuation
// from cascade N+1: each cascade-N direction maps to the four N+1 sub-directions
// { 4*dirIndex + k | k = 0..3 }; we average those four, each evaluated by
// BILINEARLY weighting the four nearest N+1 probes by this probe's fractional
// position in the N+1 grid. The bilinear weighting is what kills ring artifacts.
//
// Packing identical to rc_cascade (contract 2.1). N+1 grid:
//   D'      = 2*D
//   dirCount' = 4*dirCount
//   spacing' = 2*probeSpacing(N)
//   tileW'  = floor(W/D'), tileH' = floor(H/D').

in vec2 vUV;

layout(location = 0) out vec4 oMerged;   // RGBA16F merged radiance for cascade N

uniform sampler2D uUpperCascade;  // unit 6: already-merged cascade N+1
uniform sampler2D uSDF;           // unit 5: signed distance in pixels
uniform sampler2D uGEmissive;     // unit 3: emissive (RGBA16F)

uniform vec2  uResolution;        // pixels
uniform int   uCascadeIndex;      // N
uniform float uBaseProbeSpacing;
uniform int   uBaseDirCount;
uniform float uD0;
uniform float uIntervalNear;
uniform int   uMaxSteps;
uniform float uEps;
uniform float uMinStep;

const float PI = 3.14159265358979323846;

// --- Cast cascade N's own near interval (mirror of rc_cascade) -------------
// Returns rgb = radiance, a = hit flag.
vec4 castNear(vec2 probePos, float angle, float tNear, float tFar) {
    vec2 dir = vec2(cos(angle), sin(angle));

    float t = tNear;
    for (int i = 0; i < uMaxSteps; i++) {
        if (t >= tFar) break;

        vec2 p = probePos + dir * t;
        if (p.x < 0.0 || p.y < 0.0 || p.x > uResolution.x || p.y > uResolution.y) break;

        vec2  suv = p / uResolution;
        float d   = texture(uSDF, suv).r;

        if (d < uEps) {
            vec3  emis  = texture(uGEmissive, suv).rgb;
            return vec4(emis, 1.0);
        }
        t += max(d, uMinStep);
    }
    return vec4(0.0);
}

// --- Sample one N+1 direction at one N+1 grid cell --------------------------
// Reconstructs the texel for tile (tx',ty') (dirIndex' -> tx'=dirIndex'%D',
// ty'=dirIndex'/D') and probe cell (gx,gy), then samples uUpperCascade.
vec3 fetchUpper(int gx, int gy, int upDirIndex, int Dp, int tileWp, int tileHp,
                ivec2 gridDimP) {
    // Clamp probe cell into the N+1 probe grid.
    gx = clamp(gx, 0, gridDimP.x - 1);
    gy = clamp(gy, 0, gridDimP.y - 1);

    int txp = upDirIndex % Dp;
    int typ = upDirIndex / Dp;

    // Texel center for this (tile, probe-cell).
    vec2 texel = vec2(float(txp * tileWp + gx) + 0.5,
                      float(typ * tileHp + gy) + 0.5);
    return texture(uUpperCascade, texel / uResolution).rgb;
}

void main() {
    // --- decode cascade N geometry from the texel (same as rc_cascade) ------
    int   N  = uCascadeIndex;
    int   D  = 1 << N;
    float fD = float(D);

    int dirCount = uBaseDirCount;
    for (int i = 0; i < N; i++) dirCount *= 4;          // base * 4^N

    float probeSpacing = uBaseProbeSpacing * pow(2.0, float(N));

    int tileW = int(uResolution.x) / D;
    int tileH = int(uResolution.y) / D;

    ivec2 frag = ivec2(gl_FragCoord.xy);

    int tx = (tileW > 0) ? (frag.x / tileW) : 0;
    int ty = (tileH > 0) ? (frag.y / tileH) : 0;
    tx = clamp(tx, 0, D - 1);
    ty = clamp(ty, 0, D - 1);

    int dirIndex = ty * D + tx;

    int px = frag.x - tx * tileW;
    int py = frag.y - ty * tileH;

    vec2 probePos = (vec2(float(px), float(py)) + 0.5) * probeSpacing;

    float angle    = 2.0 * PI * (float(dirIndex) + 0.5) / float(dirCount);
    float interval = uD0 * pow(4.0, float(N));
    float tNear    = uIntervalNear;
    float tFar     = tNear + interval;

    // --- this cascade's near segment ---------------------------------------
    vec4 near = castNear(probePos, angle, tNear, tFar);

    if (near.a > 0.5) {
        // Near interval occluded/hit: far field cannot contribute.
        oMerged = vec4(near.rgb, 1.0);
        return;
    }

    // --- merge: pull continuation from cascade N+1 -------------------------
    // N+1 grid parameters.
    int   Dp           = D * 2;                          // D' = 2^(N+1)
    float spacingP     = probeSpacing * 2.0;             // spacing(N+1)
    int   tileWp       = int(uResolution.x) / Dp;
    int   tileHp       = int(uResolution.y) / Dp;
    // Probe grid dimension for N+1 (probes per axis within one tile).
    ivec2 gridDimP     = ivec2(tileWp, tileHp);

    // This probe's fractional position in the N+1 probe grid.
    // Upper-grid cell center j sits at (j+0.5)*spacingP, so the continuous
    // coordinate is probePos/spacingP - 0.5.
    vec2  gf  = probePos / spacingP - 0.5;
    vec2  g0f = floor(gf);
    ivec2 g0  = ivec2(g0f);
    vec2  w   = gf - g0f;                                // bilinear weights in [0,1]

    // The four bilinear corner weights for the 4 nearest N+1 probes.
    float w00 = (1.0 - w.x) * (1.0 - w.y);
    float w10 = (       w.x) * (1.0 - w.y);
    float w01 = (1.0 - w.x) * (       w.y);
    float w11 = (       w.x) * (       w.y);

    // Average the four N+1 sub-directions { 4*dirIndex + k }.
    vec3 merged = vec3(0.0);
    for (int k = 0; k < 4; k++) {
        int upDir = 4 * dirIndex + k;

        vec3 r =
            w00 * fetchUpper(g0.x,     g0.y,     upDir, Dp, tileWp, tileHp, gridDimP) +
            w10 * fetchUpper(g0.x + 1, g0.y,     upDir, Dp, tileWp, tileHp, gridDimP) +
            w01 * fetchUpper(g0.x,     g0.y + 1, upDir, Dp, tileWp, tileHp, gridDimP) +
            w11 * fetchUpper(g0.x + 1, g0.y + 1, upDir, Dp, tileWp, tileHp, gridDimP);

        merged += r;
    }
    merged *= 0.25;                                      // average of 4 sub-dirs

    // Add this cascade's own (missed) near segment radiance — zero on miss,
    // kept explicit so emissive picked up partway still accumulates.
    merged += near.rgb;

    oMerged = vec4(merged, 0.0);
}
