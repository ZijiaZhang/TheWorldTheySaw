#version 330

// From vertex shader
in vec2 texcoord;
in vec2 world_position;

// Application data
uniform vec3 fcolor;
uniform float radius;
uniform float thickness;
uniform vec2 center;
// Output color
layout(location = 0) out  vec4 color;

void main()
{
	if(length(world_position - center) < radius || length(world_position - center) > radius + thickness)
		discard;
	color = vec4(0.765, 0.976, 1.0, 1.0);
}
