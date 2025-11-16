#version 330

// Back-buffer textures and shader inputs
uniform sampler2D screen_texture;           // Rendered world color buffer
uniform sampler2D ui_texture;               // UI buffer to composite over the scene
uniform float time;
uniform float darken_screen_factor;
uniform sampler2D lighting_texture;         // Light rays rendered in RenderSystem::drawLights
uniform vec2 player_position;               // Player center in normalized coordinates
uniform float texture_size;                 // Width/height of the square lighting texture
uniform vec2 world_size;                    // Screen size in pixels
uniform float light_intensity;              // Current player light reach (gameplay-driven)
uniform bool enable_full_black;             // Toggle for hard vs. soft darkness
uniform float player_inner_light_radius;    // Always-lit bubble radius in pixels
uniform float player_inner_light_soft_edge; // Feather width for the bubble edge
uniform vec2 player_forward_direction;      // Player facing direction in screen space
uniform float player_light_cos_half_angle;  // Cosine of half FOV angle
uniform float player_light_fov_soft_edge;   // Blend width for FOV edge

in vec2 texcoord;

// Constants shared with the lighting pass encoding
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
	// UI pixels bypass the lighting pipeline entirely
	vec4 ui_color = texture(ui_texture, texcoord);
	if(ui_color.a > 0.0){
		color = ui_color;
		return;
	}

	// Convert into lighting texture space and fetch the encoded ray data
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

	// Decode ray information: travel distance
	float ray_len = ray_data.x * accuracy * accuracy + ray_data.y* accuracy;
	float intensity = 0.8 / pow(2, pow(distance/light_intensity, 2)) + 0.2;
    vec4 lit_color = vec4(intensity,intensity,intensity,intensity) * in_color;
    vec4 darkness = enable_full_black ? full_dark : soft_dark;
    bool inner_circle_ray = (player_inner_light_radius > 0.0) && (distance <= player_inner_light_radius);

    vec2 facing_dir = player_forward_direction;
    float forward_dot = 1.0;
    if(distance > 0.0001){
        vec2 ray_dir = normalize(delta);
        forward_dot = dot(ray_dir, facing_dir);
    }
    float fov_soft_edge = max(player_light_fov_soft_edge, 0.0001);
    float fov_blend = smoothstep(player_light_cos_half_angle, player_light_cos_half_angle + fov_soft_edge, forward_dot);

    bool has_light = ray_len >= distance && (intensity >= light_visibility_threshold || inner_circle_ray || fov_blend > 0.0 );

	// Blend depending on whether we are inside the inner bubble or the FOV cone
	if(has_light){
        float circle_blend = 0.0;
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
            circle_blend = 1.0 - circle_progress;
        }
        float fov_blend_clamped = clamp(fov_blend, 0.0, 1.0);
        float final_blend = clamp(max(circle_blend, fov_blend_clamped), 0.0, 1.0);
        color = mix(darkness * in_color, lit_color, final_blend);
	} else {
		color = darkness * in_color;
	}


}
