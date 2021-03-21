#version 330

in vec3 vs_vertex;
in vec3 vs_normal;
  
out vec4 color;
  
uniform vec3 parallel_light_dir;
uniform vec3 parallel_light_ambient;
uniform vec3 parallel_light_diffuse;
uniform vec3 parallel_light_specular;
  
uniform vec3 material_ambient;
uniform vec3 material_diffuse;
uniform vec3 material_specular;
uniform float material_specular_pow;
  
uniform vec3 eye_pos;
  
vec3 calc_diffuse(vec3 light_vec, vec3 normal, vec3 diffuse, vec3 light_color) {
    float ratio = dot(light_vec, normal);
    ratio = max(ratio, 0.0);
    return diffuse * light_color * ratio;
}
  
vec3 calc_specular(vec3 light_vec, vec3 normal, vec3 view_vec, vec3 specular, vec3 light_color, float pow_value) {
    vec3 ref_light_vec = reflect(light_vec, normal);
    float ratio = dot(ref_light_vec, view_vec);
    ratio = max(ratio, 0.0);
    ratio = pow(ratio, pow_value);
  
    return specular * light_color * ratio;
}
  
void main() {
    vec3 light_vec = -parallel_light_dir;
    vec3 normal = normalize(vs_normal);
    vec3 view_vec = normalize(eye_pos - vs_vertex);
  
    vec3 ambient = material_ambient * parallel_light_ambient;
    vec3 diffuse = calc_diffuse(light_vec, normal, material_diffuse, parallel_light_diffuse);
    vec3 specular = calc_specular(light_vec, normal, view_vec, material_specular, parallel_light_specular, material_specular_pow);
  
    color = vec4(ambient + diffuse + specular, 1.0);
}  
