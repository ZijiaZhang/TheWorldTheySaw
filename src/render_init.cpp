// internal
#include "render.hpp"
#include "render_components.hpp"

#include <iostream>
#include <fstream>

// World initialization
RenderSystem::RenderSystem(GLFWwindow& window) :
	window(window)
{
	glfwMakeContextCurrent(&window);
	glfwSwapInterval(1); // vsync

	// Load OpenGL function pointers
	gl3w_init();
    if(renderSystem)
        throw std::runtime_error("Should not create second RenderSystem");
	// Create a frame buffer
	frame_buffer = 0;
	ui_buffer = 0;
	glGenFramebuffers(1, &frame_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);

	initScreenTexture();
    glGenFramebuffers(1, &ui_buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, ui_buffer);
    ui_texture.create_from_screen(&window, depth_render_buffer_id.data());


    health_bar = ShadedMesh(); 
    health_bar.mesh.vertices.emplace_back(ColoredVertex{vec3 {0, 0.5, -0.02}, vec3{1.0,1.0,1.0}});
    health_bar.mesh.vertices.emplace_back(ColoredVertex{vec3{1, 0.5, -0.02}, vec3{1.0,1.0,1.0}});
    health_bar.mesh.vertices.emplace_back(ColoredVertex{vec3{1, -0.5, -0.02}, vec3{1.0,1.0,1.0}});
    health_bar.mesh.vertices.emplace_back(ColoredVertex{vec3{0, -0.5, -0.02}, vec3{1.0,1.0,1.0}});

    health_bar.mesh.vertex_indices = std::vector<uint16_t>({0, 2, 1, 0, 3, 2});
    health_bar.texture.color = vec3{1,0,0};
    RenderSystem::createColoredMesh(health_bar, "salmon");


    health_bar_background = ShadedMesh();
    health_bar_background.mesh.vertices.emplace_back(ColoredVertex{vec3 {0, 0.5, -0.02}, vec3{1.0,1.0,1.0}});
    health_bar_background.mesh.vertices.emplace_back(ColoredVertex{vec3{1, 0.5, -0.02}, vec3{1.0,1.0,1.0}});
    health_bar_background.mesh.vertices.emplace_back(ColoredVertex{vec3{1, -0.5, -0.02}, vec3{1.0,1.0,1.0}});
    health_bar_background.mesh.vertices.emplace_back(ColoredVertex{vec3{0, -0.5, -0.02}, vec3{1.0,1.0,1.0}});

    health_bar_background.mesh.vertex_indices = std::vector<uint16_t>({0, 2, 1, 0, 3, 2});
    health_bar_background.texture.color = vec3{0.1,0.1,0.1};
    RenderSystem::createColoredMesh(health_bar_background, "salmon");

    glGenFramebuffers(1, &background_buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, background_buffer);
    background_texture.create_from_screen(&window, depth_render_buffer_id.data());

    glGenFramebuffers(1, &background_mask_buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, background_mask_buffer);
    background_mask_texture.create_from_screen(&window, depth_render_buffer_id.data());

    // Initialize the screen texture and its state
     glGenFramebuffers(1, &wall_frame_buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, wall_frame_buffer);
    createSprite(wall_screen_sprite, "", "lights");

    wall_screen_sprite.texture.create_from_screen(&window, depth_render_buffer_id.data());

    create_light_texture(32);

    weaponTimerMask = ShadedMesh();
    weaponTimerMask.mesh.vertices.emplace_back(ColoredVertex{ vec3 {0, 0.5, -0.02}, vec3{1.0,1.0,1.0} });
    weaponTimerMask.mesh.vertices.emplace_back(ColoredVertex{ vec3{1, 0.5, -0.02}, vec3{1.0,1.0,1.0} });
    weaponTimerMask.mesh.vertices.emplace_back(ColoredVertex{ vec3{1, -0.5, -0.02}, vec3{1.0,1.0,1.0} });
    weaponTimerMask.mesh.vertices.emplace_back(ColoredVertex{ vec3{0, -0.5, -0.02}, vec3{1.0,1.0,1.0} });
    weaponTimerMask.mesh.vertex_indices = std::vector<uint16_t>({ 0, 2, 1, 0, 3, 2 });
    weaponTimerMask.texture.color = vec3{ 0.2,.8,0.2 };
    RenderSystem::createColoredMesh(weaponTimerMask, "salmon");

    renderSystem = this;
}

void RenderSystem::create_light_texture(float quality){
    glGenFramebuffers(1, &light_frame_buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, light_frame_buffer);
    light_frame_texture.create_from_screen(&window, depth_render_buffer_id.data());

    glGenTextures(1, light_frame_texture.texture_id.data());
    glBindTexture(GL_TEXTURE_2D, light_frame_texture.texture_id);

    light_frame_texture.size.x = int(quality);
    light_frame_texture.size.y = int(quality);
    //printf("%d, %d \n", x, y);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, light_frame_texture.size.x, light_frame_texture.size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // Generate the render buffer with the depth buffer
    glGenRenderbuffers(1, depth_render_buffer_id.data());
    glBindRenderbuffer(GL_RENDERBUFFER, *depth_render_buffer_id.data());
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, light_frame_texture.size.x, light_frame_texture.size.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, *depth_render_buffer_id.data());

    // Set id as colour attachement #0
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, light_frame_texture.texture_id, 0);

    // Set the list of draw buffers
    GLenum draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, draw_buffers); // "1" is the size of DrawBuffers

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("glCheckFramebufferStatus(GL_FRAMEBUFFER)");

    gl_has_errors();

}

void RenderSystem::recreate_light_texture(float quality) {
    glDeleteTextures(1, light_frame_texture.texture_id.data());
    glDeleteFramebuffers(1, &light_frame_buffer);
    create_light_texture(quality);
}

RenderSystem::~RenderSystem()
{ 
	// delete allocated resources
	glDeleteFramebuffers(1, &frame_buffer);

	// remove all entities created by the render system
	while (ECS::registry<Motion>.entities.size() > 0)
		ECS::ContainerInterface::remove_all_components_of(ECS::registry<Motion>.entities.back());
	while (ECS::registry<ShadedMeshRef>.entities.size() > 0)
		ECS::ContainerInterface::remove_all_components_of(ECS::registry<ShadedMeshRef>.entities.back());
}

// Create a new sprite and register it with ECS
void RenderSystem::createSprite(ShadedMesh& sprite, std::string texture_path, std::string shader_name)
{
	if (texture_path.length() > 0)
		sprite.texture.load_from_file(texture_path.c_str());

	// The position corresponds to the center of the texture.
	TexturedVertex vertices[4];
	vertices[0].position = { -1.f/2, +1.f/2, 0.f };
	vertices[1].position = { +1.f/2, +1.f/2, 0.f };
	vertices[2].position = { +1.f/2, -1.f/2, 0.f };
	vertices[3].position = { -1.f/2, -1.f/2, 0.f };
	vertices[0].texcoord = { 0.f, 1.f };
	vertices[1].texcoord = { 1.f, 1.f };
	vertices[2].texcoord = { 1.f, 0.f };
	vertices[3].texcoord = { 0.f, 0.f };

	// Counterclockwise as it's the default opengl front winding direction.
	uint16_t indices[] = { 0, 3, 1, 1, 3, 2 };

	glGenVertexArrays(1, sprite.mesh.vao.data());
	glGenBuffers(1, sprite.mesh.vbo.data());
	glGenBuffers(1, sprite.mesh.ibo.data());
	gl_has_errors();

	// Vertex Buffer creation
	glBindBuffer(GL_ARRAY_BUFFER, sprite.mesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // sizeof(TexturedVertex) * 4
	gl_has_errors();

	// Index Buffer creation
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sprite.mesh.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // sizeof(uint16_t) * 6
	gl_has_errors();

	glBindVertexArray(0); // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs), remember: do NOT unbind the EBO, keep it bound to this VAO

	// Loading shaders
	sprite.effect.load_from_file(shader_path(shader_name) + ".vs.glsl", shader_path(shader_name) + ".fs.glsl");
}


// Create a new sprite and register it with ECS
void RenderSystem::createSpriteAnimation(ShadedMesh& sprite, std::string texture_path, int animation_frames)
{
    if (texture_path.length() > 0)
        sprite.texture.load_from_file(texture_path.c_str());

    // The position corresponds to the center of the texture.
    TexturedVertex vertices[4];
    vertices[0].position = { -1.f/2, +1.f/2, 0.f };
    vertices[1].position = { +1.f/2, +1.f/2, 0.f };
    vertices[2].position = { +1.f/2, -1.f/2, 0.f };
    vertices[3].position = { -1.f/2, -1.f/2, 0.f };
    vertices[0].texcoord = { 0.f, 1.f };
    vertices[1].texcoord = { 1.f, 1.f };
    vertices[2].texcoord = { 1.f, 0.f };
    vertices[3].texcoord = { 0.f, 0.f };

    // Counterclockwise as it's the default opengl front winding direction.
    uint16_t indices[] = { 0, 3, 1, 1, 3, 2 };

    glGenVertexArrays(1, sprite.mesh.vao.data());
    glGenBuffers(1, sprite.mesh.vbo.data());
    glGenBuffers(1, sprite.mesh.ibo.data());
    gl_has_errors();

    // Vertex Buffer creation
    glBindBuffer(GL_ARRAY_BUFFER, sprite.mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // sizeof(TexturedVertex) * 4
    gl_has_errors();

    // Index Buffer creation
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sprite.mesh.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // sizeof(uint16_t) * 6
    gl_has_errors();

    glBindVertexArray(0); // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs), remember: do NOT unbind the EBO, keep it bound to this VAO

    // Loading shaders
    sprite.effect.load_from_string(build_anim_vertex_shader(animation_frames), fragment_shader_animation);
}

// Load a new mesh from disc and register it with ECS
void RenderSystem::createColoredMesh(ShadedMesh& texmesh, std::string shader_name)
{
	// Vertex Array
	glGenVertexArrays(1, texmesh.mesh.vao.data());
	glGenBuffers(1, texmesh.mesh.vbo.data());
	glGenBuffers(1, texmesh.mesh.ibo.data());
	glBindVertexArray(texmesh.mesh.vao);

	// Vertex Buffer creation
	glBindBuffer(GL_ARRAY_BUFFER, texmesh.mesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(ColoredVertex) * texmesh.mesh.vertices.size(), texmesh.mesh.vertices.data(), GL_STATIC_DRAW);

	// Index Buffer creation
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, texmesh.mesh.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * texmesh.mesh.vertex_indices.size(), texmesh.mesh.vertex_indices.data(), GL_STATIC_DRAW);
	gl_has_errors();

	// Note, one could set vertex attributes here...
	// glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	// glEnableVertexAttribArray(0);
	// glBindBuffer(GL_ARRAY_BUFFER, 0); // Note that this is allowed, the call to glVertexAttribPointer registered VBO as the currently bound vertex buffer object so afterwards we can safely unbind

	glBindVertexArray(0); // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs), remember: do NOT unbind the EBO, keep it bound to this VAO

	// Loading shaders
	texmesh.effect.load_from_file(shader_path(shader_name)+".vs.glsl", shader_path(shader_name)+".fs.glsl");
}

// Initialize the screen texture from a standard sprite
void RenderSystem::initScreenTexture()
{
	// Create a sprite withour loading a texture
	createSprite(screen_sprite, "", "water");

	// Initialize the screen texture and its state
	screen_sprite.texture.create_from_screen(&window, depth_render_buffer_id.data());
	ECS::registry<ScreenState>.emplace(screen_state_entity);



}

RenderSystem* RenderSystem::renderSystem = nullptr;
