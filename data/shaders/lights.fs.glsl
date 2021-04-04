#version 330

uniform sampler2D screen_texture;
uniform float time;
uniform float darken_screen_factor;
uniform vec2 player_position;
uniform float texture_size;
uniform vec2 world_size;

in vec2 texcoord;
in vec2 world_pos;

const float max_step = 600.0;
const float accuracy = 255.0;
const float pi = radians(180.0);
layout(location = 0) out vec4 color;

void main()
{
	vec2 coord = floor(texcoord * texture_size);
	// float world_deg = world_size.y/ world_size.x;
	float ray_count = texture_size * texture_size;
	float ray_of_current_pixel = coord.x + coord.y * texture_size;
	float radian = 2.0 * pi * ray_of_current_pixel/ray_count;
	vec2 angle = normalize(vec2(cos(radian), sin(radian)));

	float t = 0.0;
	for(float i = 0.0; i< max_step; i+=3.0){
		if(texture2D(screen_texture, (player_position * world_size + angle * i) / world_size).a > 0.5) {
			t = i;
			break;
		};
	}
	color = vec4(floor(t/ accuracy)/accuracy, mod(t, accuracy) / accuracy,0.0,1.0);
//	color = vec4(cos(radian), sin(radian), 0.0, 1.0);


}