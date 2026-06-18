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

	initScreenTexture();

	renderSystem = this;
}

RenderSystem::~RenderSystem()
{
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
	sprite.effect.load_from_file(shader_path(shader_name) + ".vertex.glsl", shader_path(shader_name) + ".fragment.glsl");
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

	glBindVertexArray(0); // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs), remember: do NOT unbind the EBO, keep it bound to this VAO

	// Loading shaders
	texmesh.effect.load_from_file(shader_path(shader_name)+".vertex.glsl", shader_path(shader_name)+".fragment.glsl");
}

// Set up the shared screen state (holds the active camera for the draw loop)
void RenderSystem::initScreenTexture()
{
	ECS::registry<ScreenState>.emplace(screen_state_entity);
}

RenderSystem* RenderSystem::renderSystem = nullptr;
