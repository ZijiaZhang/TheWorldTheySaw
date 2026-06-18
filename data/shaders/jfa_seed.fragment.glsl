#version 330

// JFA seed pass (Pass 2a).
// Initializes the Jump Flooding ping-pong seed texture from the G-buffer
// occluder mask. A texel that belongs to an occluder (GBuffer0.a > 0.5) seeds
// itself with its own pixel-space coordinate; everything else is the invalid
// sentinel (-1,-1). Subsequent jfa_step passes propagate the nearest seed.

in vec2 vUV;

// RG16F: nearest occluder seed position in PIXELS, or (-1,-1) when invalid.
layout(location = 0) out vec2 oSeed;

uniform sampler2D uGAlbedo;   // unit 0: gbuffer0, occluder mask in .a
uniform vec2 uResolution;     // render-target size in pixels (unused here, kept per contract)

void main()
{
    float mask = texture(uGAlbedo, vUV).a;
    // gl_FragCoord.xy is the pixel-space position (origin bottom-left, GL convention),
    // which is exactly the "P.xy" working-space coordinate used by the SDF and RC passes.
    oSeed = (mask > 0.5) ? gl_FragCoord.xy : vec2(-1.0, -1.0);
}
