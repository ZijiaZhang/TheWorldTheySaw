#version 330 

// Input attributes
in vec3 in_position;
in vec2 in_texcoord;
layout (location = 2) in mat3 transform;
in vec2 speed;
in float scale_speed;
// Passed to fragment shader
out vec2 texcoord;

// Application data

uniform mat3 projection;
uniform vec2 camera_position;
uniform float delta_second;
void main()
{
	texcoord = in_texcoord;
	mat3 final_transform = transform;
	final_transform[2] = final_transform[2] - vec3(camera_position, 0.0) + delta_second * vec3(speed, 0.0);
	final_transform[0][0] = final_transform[0][0] * pow(scale_speed, delta_second);
	final_transform[1][1] = final_transform[1][1] * pow(scale_speed, delta_second);

	vec3 pos = projection * final_transform * vec3(in_position.xy, 1.0);
	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}