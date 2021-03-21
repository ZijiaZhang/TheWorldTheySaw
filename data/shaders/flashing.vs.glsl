#version 330

in vec3 vertex;
in vec3 normal;
  
uniform mat4 proj;
uniform mat4 view;
uniform mat4 world;
uniform mat4 trans_inv_world;
  
out vec3 vs_vertex;
out vec3 vs_normal;

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
  
void main() {
    vs_vertex = (world * vec4(in_position, 1.0)).xyz;
    vs_normal = (trans_inv_world * vec4(normal, 0.0)).xyz;
    vec3 pos = projection * transform * vec3(in_position.xy, 1.0);
    gl_Position = vec4(pos.xy, in_position.z, 1.0);
}
