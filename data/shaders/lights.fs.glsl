#version 330

uniform sampler2D screen_texture;
uniform float time;
uniform float darken_screen_factor;
uniform float player_position_x;
uniform float player_position_y;

in vec2 texcoord;
in vec2 world_pos;

layout(location = 0) out vec4 color;

void main()
{
	vec2 coord = texcoord;

    vec4 in_color = texture(screen_texture, coord);
	vec2 player_position = vec2(player_position_x, player_position_y);
	vec2 direction = texcoord - player_position;
	color = in_color;
	for (int i =0; i< 500; i++){
		vec4 color2 = texture(screen_texture,player_position + direction / 500.0 * float(i));
		if (color2.x < 0.5) {
			color = vec4(0, 0, 0,1);
			break;
		}
	}

}