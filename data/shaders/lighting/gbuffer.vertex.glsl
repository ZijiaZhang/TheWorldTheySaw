#version 430
//
// Geometry pass vertex shader (§6). Two placement modes:
//  * Upright (uPlacement==0): screen-aligned billboard — the unit quad is placed
//    at the sprite's oblique-projected anchor and scaled in screen pixels. Used
//    for tall, camera-facing things; per-pixel world pos comes from the height
//    channel.
//  * WorldQuad (uPlacement==1): the 4 corners are transformed into WORLD space
//    (center + right/up axes) and each is oblique-projected with the SAME formula
//    as direct_light.fragment.glsl, then the true world-z is passed to the
//    fragment shader to write as height. Used for ground decals, ramps, props.
//
layout(location = 0) in vec3 in_position;  // unit quad, xy in [-0.5, 0.5]
layout(location = 1) in vec2 in_texcoord;

uniform int   uPlacement;    // 0 = Upright, 1 = WorldQuad
uniform vec2  uScreenSize;   // framebuffer size in pixels

// Upright
uniform vec2  uAnchorScreen; // sprite anchor in screen pixels (oblique-projected)
uniform vec2  uSpriteSize;   // sprite size in pixels
uniform float uAngle;        // screen-space rotation (usually 0)

// WorldQuad: corner = uWorldCenter + in_position.x*uQuadRight + in_position.y*uQuadUp
uniform vec3  uWorldCenter;
uniform vec3  uQuadRight;     // world right axis * worldSize.x
uniform vec3  uQuadUp;        // world up axis    * worldSize.y
uniform vec2  uFocus;         // oblique projection (must match direct_light)
uniform float uKx, uKy, uKz;

out vec2 vUV;
noperspective out float vWorldZ;

void main() {
    vUV = in_texcoord;

    if (uPlacement == 1) {
        vec3 w = uWorldCenter + in_position.x * uQuadRight + in_position.y * uQuadUp;
        vec2 c = uScreenSize * 0.5;
        float ex = w.x - uFocus.x;
        float ey = w.y - uFocus.y;
        vec2 screen = vec2(c.x + (ex - ey) * uKx, c.y + (ex + ey) * uKy + w.z * uKz);
        gl_Position = vec4(screen / uScreenSize * 2.0 - 1.0, 0.0, 1.0);
        vWorldZ = w.z;
    } else {
        vec2 local = in_position.xy * uSpriteSize;
        float cs = cos(uAngle), sn = sin(uAngle);
        vec2 rotated = vec2(local.x * cs - local.y * sn, local.x * sn + local.y * cs);
        vec2 screen = uAnchorScreen + rotated;
        gl_Position = vec4(screen / uScreenSize * 2.0 - 1.0, 0.0, 1.0);
        vWorldZ = 0.0; // unused in Upright mode (fragment uses the height channel)
    }
}
