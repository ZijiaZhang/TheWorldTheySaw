#version 330

// Input attributes
in vec3 in_position;
in vec2 in_texcoord;

// Passed to fragment shader
out vec2 texcoord;
out vec2 vpos;
out vec2 ori_pos;

// Application data
uniform mat3 transform;
uniform mat3 projection;
uniform float time;
void main()
{
    texcoord = in_texcoord;
    vec3 pos = projection * transform * vec3(in_position.xy, 1.0);
    vpos = pos.xy;
    ori_pos = in_position.xy;
    gl_Position = vec4(pos.xy, in_position.z, 1.0);
}