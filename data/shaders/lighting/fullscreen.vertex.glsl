#version 430
//
// Fullscreen triangle (§14). Draw with glDrawArrays(GL_TRIANGLES, 0, 3) and an
// empty VAO bound. vUV spans [0,1] across the screen.
//
out vec2 vUV;

void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    vUV = p;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
