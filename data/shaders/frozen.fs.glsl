#version 330

// From vertex shader
in vec2 texcoord;
in vec2 vpos;

// Application data
uniform sampler2D sampler0;
uniform vec3 fcolor;
uniform float time;

// Output color
layout(location = 0) out  vec4 color;

void main()
{
    color = vec4(fcolor, 1.0) * texture(sampler0, vec2(texcoord.x, texcoord.y));
    if(color.a < 0.2) {
        discard;
    }

    color = color * sin(time * 200);

}
