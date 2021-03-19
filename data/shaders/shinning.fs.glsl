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
    vec3 vNormal = vec3(0, 0, 1) - vPosition;
    vec3 tangent = vec3(0, 0.5, 0);
    vec3 toLight = normalize(uLight);
    vec3 toV = -normalize(vPosition);
    vec3 h = normalize(toV + toLight);
    vec3 normal = normalize(vNormal);
    vec3 vTangent3 = normalize(cross(vNormal, tangent));

    float nl = dot(normal, toLight);
    float dif = max(0.0, 0.75 * nl + 0.25);
    float v = dot(vTangent3, h);
    v = pow(1.0 - v*v, shininess);
    float r = vColor.r * (dif ) + specular * v + ambColor.x * amb;
    float g = vColor.g * (dif) + specular * v + ambColor.y * amb;
    float b = vColor.b * (dif ) + specular * v + ambColor.z * amb;

    return vec4(r, g, b, 1.0);
}

void main()
{
    color = vec4(fcolor, 1.0) * texture(sampler0, vec2(texcoord.x, texcoord.y));
    float range = 0.4;
    if(color.a < 0.8) {
        discard;
    }
    float distance = distance(vec2(0, 0), vpos);

    if (distance <= range) {
        color = anisotropic(color.xyz);
    } else {
        color = vec4(0.1, 0.1, 0.1, 1);
    }

}
