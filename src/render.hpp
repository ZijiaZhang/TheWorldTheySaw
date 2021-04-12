#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "render_components.hpp"
#include "soldier.hpp"
#include "Particle.hpp"

struct InstancedMesh;
struct ShadedMesh;

const std::string fragment_shader_animation = "#version 330\n"
                                              "\n"
                                              "// From vertex shader\n"
                                              "in vec2 texcoord;\n"
                                              "\n"
                                              "// Application data\n"
                                              "uniform sampler2D sampler0;\n"
                                              "uniform vec3 fcolor;\n"
                                              "\n"
                                              "// Output color\n"
                                              "layout(location = 0) out  vec4 color;\n"
                                              "\n"
                                              "void main()\n"
                                              "{\n"
                                              "\tcolor = vec4(fcolor, 1.0) * texture(sampler0, vec2(texcoord.x, texcoord.y));\n"
                                              "}";

// OpenGL utilities
void gl_has_errors();

// System responsible for setting up OpenGL and for rendering all the 
// visual entities in the game
class RenderSystem
{
public:
	// Initialize the window
	RenderSystem(GLFWwindow& window);

	// Destroy resources associated to one or all entities created by the system
	~RenderSystem();

	// Draw all entities
	void draw(vec2 window_size_in_game_units);

	// Expose the creating of visual representations to other systems
	static void createSprite(ShadedMesh& mesh_container, std::string texture_path, std::string shader_name);
	static void createColoredMesh(ShadedMesh& mesh_container, std::string shader_name);
    static void createSpriteAnimation(ShadedMesh &sprite, std::string texture_path, int number_of_frames);
    static RenderSystem* renderSystem;
    void create_light_texture(float quality);
	void recreate_light_texture(float quality);
	void createWeaponTimer(mat3 projection_2D, Motion timer_mesh_motion, ECS::Entity weaponTimer_entity);
private:
	// Initialize the screeen texture used as intermediate render target
	// The draw loop first renders to this texture, then it is used for the water shader
	void initScreenTexture(); 

	// Internal drawing functions for each entity type
	void drawTexturedMesh(ECS::Entity entity, const mat3& projection, bool relative_to_screen = false);
    void drawTexturedMesh(ECS::Entity entity, const mat3 &projection, Motion &motion, const ShadedMesh &texmesh, bool relative_to_screen = false);

	void drawInstanced(const mat3& projection, Particle& particle);

	void drawToScreen(vec2 window_size_in_game_units);

	// Window handle
	GLFWwindow& window;

	// Screen texture handles
	GLuint frame_buffer;
	GLuint ui_buffer;
	GLuint light_frame_buffer;
	GLuint wall_frame_buffer;

	ShadedMesh screen_sprite;
	ShadedMesh health_bar;
	ShadedMesh health_bar_background;
    ShadedMesh weaponTimerMask;
	ShadedMesh wall_screen_sprite;

	Texture light_frame_texture;
	Texture ui_texture;

	GLResource<RENDER_BUFFER> depth_render_buffer_id;
	ECS::Entity screen_state_entity;

    static const std::string build_anim_vertex_shader(int frames);

    void drawLights(vec2 window_size_in_game_units);



};
