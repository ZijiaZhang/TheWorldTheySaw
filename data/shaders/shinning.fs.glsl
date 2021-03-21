#version 330

// From vertex shader
in vec2 texcoord;
in vec2 vpos;
in vec2 ori_pos;

// Application data
uniform sampler2D sampler0;
uniform vec3 fcolor;
uniform float time;

// Output color
layout(location = 0) out  vec4 color;

vec4 anisotropic(vec3 vColor) {
    float shininess = 10;
    float specular = 0.5;
    float amb = 0.4;
    vec3 ambColor = vec3(0.4, 0.4, 0.4);

    vec3 vPosition = vec3(ori_pos - vpos, 0);
    vec3 uLight = vec3(0.49,0.79, 0.49);
    // the normal of the point,
    // since it's 2d, we shift by the position on normal to make it visually varies
    vec3 vNormal = vec3(0, 0, 1) - vPosition;
    vec3 toLight = normalize(uLight);

    // diffuse, if nl is large means the angle between normal and toLight is smaller
    vec3 normal = normalize(vNormal);
    float nl = dot(normal, toLight);
    float dif = max(0.0, 0.75 * nl + 0.25);

    // the far from center reflect, the less shininess
    vec3 toV = -normalize(vPosition);// the view vector,
    vec3 h = normalize(toV + toLight);  // half vector
    vec3 tangent = vec3(0.5, .5, 0);
    vec3 vTangent3 = normalize(cross(vNormal, tangent));
    float v = dot(vTangent3, h);
    v = pow(1.0 - v*v, shininess);

    // diffuse + specular + ambient
    vec3 c = vColor * dif + specular * v + ambColor * amb;

    return vec4(c, 1.0);
}

void main()
{
    color = vec4(fcolor, 1.0) * texture(sampler0, vec2(texcoord.x, texcoord.y));

    if(color.a < 0.8) {
        discard;
    }

    color = anisotropic(color.xyz);

}
