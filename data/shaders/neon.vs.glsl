#version 330

// Input attributes
in vec3 in_position;
in vec2 in_texcoord;
uniform float time;
// Passed to fragment shader
out vec2 texcoord;

// Application data
uniform mat3 transform;
uniform mat3 projection;

void main()
{
    highp int mytime = int(time);
    texcoord = vec2((in_texcoord.x + mod(int(time),5.0)) * 0.5,  in_texcoord.y/2.0);
    vec3 pos = projection * transform * vec3(vec2(in_position.y, in_position.x), 1.0);
    gl_Position = vec4(pos.xy, in_position.z, 1.0);
}
