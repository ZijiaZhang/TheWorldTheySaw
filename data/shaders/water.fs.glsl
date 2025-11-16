#version 330

uniform sampler2D screen_texture;
uniform sampler2D ui_texture;
uniform float time;
uniform float darken_screen_factor;
uniform sampler2D lighting_texture;
uniform vec2 player_position;
uniform float texture_size;
uniform vec2 world_size;
uniform float light_intensity;
uniform bool enable_full_black;
uniform float player_inner_light_radius;
uniform float player_inner_light_soft_edge;

in vec2 texcoord;

const float max_step = 255.0 * 255.0;
const float pi = radians(180.0);
const float accuracy = 255.0;
const float light_visibility_threshold = 0.05;
const float inner_circle_soft_edge_ratio = 0.35;
const float inner_circle_soft_edge_min = 8.0;
const vec4 full_dark = vec4(0.0, 0.0, 0.0, 1.0);
const vec4 soft_dark = vec4(0.2, 0.2, 0.2, 1.0);

layout(location = 0) out vec4 color;

void main()
{
	vec4 ui_color = texture(ui_texture, texcoord);
	if(ui_color.a > 0.0){
		color = ui_color;
		return;
	}
	vec2 coord = floor(texcoord * world_size);
	float ray_count = texture_size * texture_size;

    vec4 in_color = texture(screen_texture, texcoord);
	vec2 light_position = player_position * world_size;
	vec2 delta = coord - light_position;
	float radian = atan(delta.y , delta.x);
	float distance = length(delta);
	float index = floor(fract(radian / 2.0 / pi) * ray_count);
	vec2 ray_loc = vec2(mod(index, texture_size)  + 0.5, floor((index) / texture_size)  + 0.5) / texture_size;
	vec4 ray_data = texture(lighting_texture, ray_loc);

	float ray_len = ray_data.x * accuracy * accuracy + ray_data.y* accuracy;
	float intensity = 0.8 / pow(2, pow(distance/light_intensity, 2)) + 0.2;
    vec4 lit_color = vec4(intensity,intensity,intensity,intensity) * in_color;
    vec4 darkness = enable_full_black ? full_dark : soft_dark;
    bool inner_circle_ray = ray_data.z > 0.5;
    bool has_light = ray_len >= distance && (intensity >= light_visibility_threshold || inner_circle_ray);

	if(has_light){
        if(inner_circle_ray){
            float effective_radius = ray_len;
            if(player_inner_light_radius > 0.0){
                effective_radius = min(player_inner_light_radius, ray_len);
            }
            float soft_edge = player_inner_light_soft_edge;
            if(soft_edge <= 0.0){
                soft_edge = max(effective_radius * inner_circle_soft_edge_ratio, inner_circle_soft_edge_min);
            }
            float inner_start = max(effective_radius - soft_edge, 0.0);
            float circle_progress = smoothstep(inner_start, effective_radius, distance);
            float blend = 1.0 - circle_progress;
            color = mix(darkness * in_color, lit_color, blend);
        } else {
		    color = lit_color;
        }
	} else {
		color = darkness * in_color;
	}


}
