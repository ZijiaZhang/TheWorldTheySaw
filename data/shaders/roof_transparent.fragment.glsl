#version 330

// From Vertex Shader
in vec3 vcolor;
in vec2 vpos;

// Application data
uniform vec3 fcolor;
uniform float opacity; // Roof opacity (0.0 = transparent, 1.0 = opaque)

// Output color
layout(location = 0) out vec4 color;

void main()
{
	// Use opacity for alpha channel
	color = vec4(fcolor * vcolor, opacity);
}
