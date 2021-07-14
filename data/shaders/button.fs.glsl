#version 330

// From vertex shader
in vec2 texcoord;

// Application data
uniform sampler2D sampler0;
uniform vec3 fcolor;
uniform bool selected;
uniform sampler2D selected_background;

// Output color
layout(location = 0) out  vec4 color;

void main()
{	
	vec4 front_color =  texture(sampler0, vec2(texcoord.x, texcoord.y));
	if ( front_color.a == 0.0 && selected){
		color = vec4(fcolor, 1.0) * texture(selected_background, vec2(texcoord.x, texcoord.y));
		return;
	}
	color = vec4(fcolor, 1.0) * front_color;
}
