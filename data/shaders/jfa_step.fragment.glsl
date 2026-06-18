#version 330

// JFA step pass (Pass 2b), run once per jump offset (N/2, N/4, ..., 1).
// For each fragment, examine the 3x3 neighborhood at +/- uOffset pixels in the
// previous ping-pong seed texture (8 neighbours + the center tap that carries
// the fragment's own current best). Keep whichever valid seed is nearest to this
// fragment's pixel position. This iteratively builds the occluder Voronoi map.

in vec2 vUV;

// RG16F: best nearest occluder seed position in PIXELS, or (-1,-1) when none found.
layout(location = 0) out vec2 oSeed;

uniform sampler2D uSeedTex;   // unit 4: previous ping-pong seed texture (RG16F)
uniform vec2  uResolution;    // render-target size in pixels (for uv conversion)
uniform float uOffset;        // jump step in PIXELS for this iteration

void main()
{
    vec2 fragPos = gl_FragCoord.xy;     // this fragment's pixel-space position

    vec2  bestSeed = vec2(-1.0, -1.0);
    float bestDist = 3.4e38;            // large sentinel (no valid seed yet)

    // 3x3 neighborhood at the current jump offset.
    for (int dy = -1; dy <= 1; ++dy)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            // Neighbour pixel center, converted to [0,1] uv for the texture fetch.
            vec2 sampleUV = (fragPos + vec2(float(dx), float(dy)) * uOffset) / uResolution;
            vec2 seed = texture(uSeedTex, sampleUV).rg;

            // Skip the invalid sentinel. A valid seed always has non-negative coords.
            if (seed.x < 0.0)
                continue;

            float d = distance(seed, fragPos);
            if (d < bestDist)
            {
                bestDist = d;
                bestSeed = seed;
            }
        }
    }

    oSeed = bestSeed;
}
