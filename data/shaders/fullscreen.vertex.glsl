#version 330

// Shared full-screen pass vertex shader.
// Emits a single large triangle that covers the screen using gl_VertexID only.
// NO vertex attributes / NO vertex buffer are bound (a dummy VAO must still be
// bound because core profile forbids drawing with VAO 0). Draw with
// glDrawArrays(GL_TRIANGLES, 0, 3).
//
// vUV is [0,1] across the visible screen, origin bottom-left (matches
// gl_FragCoord and the template's sprite texcoords). The triangle's vUV spans
// [0,2] so the visible [0,1] region tiles the framebuffer exactly once.

out vec2 vUV;

void main()
{
    // gl_VertexID -> (0,0), (2,0), (0,2)
    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    vUV = p;                                  // [0,2]; covers [0,1] over the screen
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
