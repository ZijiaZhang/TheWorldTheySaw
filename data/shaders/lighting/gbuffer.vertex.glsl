#version 430
//
// Geometry pass vertex shader (§6). Sprites are screen-aligned billboards: the
// unit quad is positioned at the sprite's oblique-projected anchor and scaled to
// its on-screen pixel size. Per-pixel world position is recovered later from
// gl_FragCoord + the height channel, so the quad orientation is purely 2D here.
//
layout(location = 0) in vec3 in_position;  // unit quad, xy in [-0.5, 0.5]
layout(location = 1) in vec2 in_texcoord;

uniform vec2  uAnchorScreen; // sprite anchor in screen pixels (oblique-projected)
uniform vec2  uSpriteSize;   // sprite size in pixels
uniform vec2  uScreenSize;   // framebuffer size in pixels
uniform float uAngle;        // screen-space rotation (usually 0)

out vec2 vUV;

void main() {
    vUV = in_texcoord;

    vec2 local = in_position.xy * uSpriteSize;
    float c = cos(uAngle), s = sin(uAngle);
    vec2 rotated = vec2(local.x * c - local.y * s, local.x * s + local.y * c);

    vec2 screen = uAnchorScreen + rotated;
    vec2 ndc = screen / uScreenSize * 2.0 - 1.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
}
