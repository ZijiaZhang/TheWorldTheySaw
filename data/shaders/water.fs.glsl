#version 330

uniform sampler2D screen_texture;
uniform sampler2D ui_texture;
uniform float time;
uniform float darken_screen_factor;
uniform sampler2D lighting_texture;
uniform vec2 player_position;
uniform float texture_size;
uniform vec2 world_size;

in vec2 texcoord;

const float max_step = 255.0;
const float pi = radians(180);

layout(location = 0) out vec4 color;

void main()
{
	vec2 coord = floor(texcoord * world_size);
	float ray_count = texture_size * texture_size;

    vec4 in_color = texture(screen_texture, texcoord);
	vec2 light_position = player_position * world_size;
	vec2 delta = coord - light_position;
	float radian = atan(delta.y , delta.x);
	float distance = length(delta);
	float index = floor(fract(radian / 2.0 / pi) * ray_count);
	vec2 ray_loc = vec2(mod(index, texture_size)  + 0.5, floor((index + 0.5) / texture_size)) / texture_size;
	float ray_len = texture(lighting_texture, ray_loc).x * float(max_step);

	// vec4 auto = texture(lighting_texture, ray_loc);

	if(ray_len >= distance){
		color = vec4(1.0,1.0,1.0,1.0) * in_color;
	} else {
		color = vec4(0.2,0.2,0.2,1.0) * in_color;
	}
//	color = vec4(cos(radian), sin(radian), 0.0, 1.0);
	//color = texture(lighting_texture, texcoord);
//	if(light_color.x < 0.5){
//		color *= 0.2;
//	}

}