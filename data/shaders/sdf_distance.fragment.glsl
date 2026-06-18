#version 330

// SDF resolve pass (Pass 2c).
// Converts the final JFA nearest-seed texture into a signed distance field in
// PIXELS. The unsigned distance is the pixel distance from this fragment to its
// nearest occluder seed; the sign is taken from the occluder mask so the field
// is positive outside occluders and negative inside them. RC sphere-tracing
// reads this single R16F distance.

in vec2 vUV;

// R16F: signed distance in PIXELS (+ outside occluders, - inside).
layout(location = 0) out float oDist;

uniform sampler2D uSeedTex;   // unit 4: final JFA seeds (RG16F), nearest occluder xy in pixels
uniform sampler2D uGAlbedo;   // unit 0: gbuffer0, occluder mask in .a signs the distance
uniform vec2 uResolution;     // render-target size in pixels (unused here, kept per contract)

void main()
{
    vec2 fragPos = gl_FragCoord.xy;
    vec2 seed    = texture(uSeedTex, vUV).rg;

    // Unsigned pixel distance to the nearest occluder seed. If no seed propagated
    // here (sentinel), there are no occluders in range; report a large outside
    // distance so sphere-tracing makes maximal progress.
    float d;
    if (seed.x < 0.0)
        d = 3.0e4;                        // safely large for an R16F target (< 65504)
    else
        d = distance(seed, fragPos);

    // Sign: inside occluders (mask set on this very texel) the distance is negative.
    float mask = texture(uGAlbedo, vUV).a;
    float sign = (mask > 0.5) ? -1.0 : 1.0;

    oDist = d * sign;
}
