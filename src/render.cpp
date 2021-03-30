// internal
#include "render.hpp"
#include "render_components.hpp"
#include "tiny_ecs.hpp"
#include "Camera.hpp"
#include "Explosion.hpp"
#include "button.hpp"
#include "Wall.hpp"
#include "soldier.hpp"
#include "MoveableWall.hpp"

#include <iostream>
#include <sstream>

void RenderSystem::drawTexturedMesh(ECS::Entity entity, const mat3& projection)
{
	auto& motion = ECS::registry<Motion>.get(entity);
	auto& texmesh = *ECS::registry<ShadedMeshRef>.get(entity).reference_to_cache;
    auto& screen = screen_state_entity.get<ScreenState>();
    auto& camera = ECS::registry<Camera>.get(screen.camera);
    // Transformation code, see Rendering and Transformation in the template specification for more info
	// Incrementally updates transformation matrix, thus ORDER IS IMPORTANT
	Transform transform;
	transform.translate(motion.position - camera.get_position());
	transform.rotate(motion.angle);
	transform.scale(motion.scale);

	// Setting shaders
	glUseProgram(texmesh.effect.program);
	glBindVertexArray(texmesh.mesh.vao);
	gl_has_errors();

	// Enabling alpha channel for textures
	glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	gl_has_errors();

	GLint transform_uloc = glGetUniformLocation(texmesh.effect.program, "transform");
	GLint projection_uloc = glGetUniformLocation(texmesh.effect.program, "projection");
	gl_has_errors();

	// Setting vertex and index buffers
	glBindBuffer(GL_ARRAY_BUFFER, texmesh.mesh.vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, texmesh.mesh.ibo);
	gl_has_errors();

	// Input data location as in the vertex buffer
    GLuint time_uloc       = glGetUniformLocation(texmesh.effect.program, "time");
	GLint in_position_loc = glGetAttribLocation(texmesh.effect.program, "in_position");
	GLint in_texcoord_loc = glGetAttribLocation(texmesh.effect.program, "in_texcoord");
	GLint in_color_loc = glGetAttribLocation(texmesh.effect.program, "in_color");
    glUniform1f(time_uloc, static_cast<float>(glfwGetTime() * 10.0f));
    if (in_texcoord_loc >= 0)
	{
		glEnableVertexAttribArray(in_position_loc);
		glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(in_texcoord_loc);
		glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), reinterpret_cast<void*>(sizeof(vec3))); // note the stride to skip the preceeding vertex position
		// Enabling and binding texture to slot 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texmesh.texture.texture_id);
	} else if (in_color_loc >= 0) {
		glEnableVertexAttribArray(in_position_loc);
		glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(in_color_loc);
		glVertexAttribPointer(in_color_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), reinterpret_cast<void*>(sizeof(vec3)));

	} else {
		throw std::runtime_error("This type of entity is not yet supported");
	}
	gl_has_errors();

	// Getting uniform locations for glUniform* calls
	GLint color_uloc = glGetUniformLocation(texmesh.effect.program, "fcolor");
	//glUniform3fv(color_uloc, 1, (float*)&texmesh.texture.color);
    if(ECS::registry<PressTimer>.has(entity)){
        float color[] = {0.f, 0.5f, 1.f};
        glUniform3fv(color_uloc, 1, color);
    } else{
        glUniform3fv(color_uloc, 1, (float*)&texmesh.texture.color);
    }
    gl_has_errors();

	// Get number of indices from index buffer, which has elements uint16_t
	GLint size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	gl_has_errors();
	GLsizei num_indices = size / sizeof(uint16_t);
	//GLsizei num_triangles = num_indices / 3;

	// Setting uniform values to the currently bound program
	glUniformMatrix3fv(transform_uloc, 1, GL_FALSE, (float*)&transform.mat);
	glUniformMatrix3fv(projection_uloc, 1, GL_FALSE, (float*)&projection);
	gl_has_errors();

    GLint shining_uloc = glGetUniformLocation(texmesh.effect.program, "shining");
    if(shining_uloc > 0){
        if(entity.has<Button>()){
            glUniform1i(shining_uloc, entity.get<Button>().selected);
        }
    }

    // Drawing of num_indices/3 triangles specified in the index buffer
	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
	glBindVertexArray(0);
}

// Draw the intermediate texture to the screen, with some distortion to simulate water
void RenderSystem::drawToScreen() 
{
	// Setting shaders
	glUseProgram(screen_sprite.effect.program);
	glBindVertexArray(screen_sprite.mesh.vao);
	gl_has_errors();

	// Clearing backbuffer
	int w, h;
	glfwGetFramebufferSize(&window, &w, &h);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, w, h);
	glDepthRange(0, 10);
	glClearColor(1.f, 0, 0, 1.0);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gl_has_errors();
	
	// Disable alpha channel for mapping the screen texture onto the real screen
	glDisable(GL_BLEND); // we have a single texture without transparency. Areas with alpha <1 cab arise around the texture transparency boundary, enabling blending would make them visible.
	glDisable(GL_DEPTH_TEST);

	glBindBuffer(GL_ARRAY_BUFFER, screen_sprite.mesh.vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, screen_sprite.mesh.ibo); // Note, GL_ELEMENT_ARRAY_BUFFER associates indices to the bound GL_ARRAY_BUFFER
	gl_has_errors();

	// Draw the screen texture on the quad geometry
	gl_has_errors();

	// Set clock
	GLuint time_uloc       = glGetUniformLocation(screen_sprite.effect.program, "time");
	GLuint dead_timer_uloc = glGetUniformLocation(screen_sprite.effect.program, "darken_screen_factor");
	glUniform1f(time_uloc, static_cast<float>(glfwGetTime() * 10.0f));
	auto& screen = ECS::registry<ScreenState>.get(screen_state_entity);
	glUniform1f(dead_timer_uloc, screen.darken_screen_factor);
	gl_has_errors();

	// Set the vertex position and vertex texture coordinates (both stored in the same VBO)
	GLint in_position_loc = glGetAttribLocation(screen_sprite.effect.program, "in_position");
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
	GLint in_texcoord_loc = glGetAttribLocation(screen_sprite.effect.program, "in_texcoord");
	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3)); // note the stride to skip the preceeding vertex position
	gl_has_errors();
    GLint normal_texture_loc = glGetUniformLocation(screen_sprite.effect.program, "screen_texture");
    GLint light_texture_loc = glGetUniformLocation(screen_sprite.effect.program, "lighting_texture");
    glUniform1i(normal_texture_loc, 0);
    glUniform1i(light_texture_loc,  1);
	// Bind our texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, screen_sprite.texture.texture_id);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, light_frame_texture.texture_id);
	// Draw
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr); // two triangles = 6 vertices; nullptr indicates that there is no offset from the bound index buffer
	glBindVertexArray(0);
	gl_has_errors();
}


// Draw the intermediate texture to the screen, with some distortion to simulate water
void RenderSystem::drawLights()
{
    // Setting shaders
    glUseProgram(wall_screen_sprite.effect.program);
    glBindVertexArray(wall_screen_sprite.mesh.vao);
    gl_has_errors();

    // Clearing backbuffer
    int w, h;
    glfwGetFramebufferSize(&window, &w, &h);
    glBindFramebuffer(GL_FRAMEBUFFER, light_frame_buffer);
    glViewport(0, 0, w, h);
    glDepthRange(0, 10);
    glClearColor(1.f, 0, 0, 1.0);
    glClearDepth(1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl_has_errors();

    // Disable alpha channel for mapping the screen texture onto the real screen
    glDisable(GL_BLEND); // we have a single texture without transparency. Areas with alpha <1 cab arise around the texture transparency boundary, enabling blending would make them visible.
    glDisable(GL_DEPTH_TEST);

    glBindBuffer(GL_ARRAY_BUFFER, wall_screen_sprite.mesh.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wall_screen_sprite.mesh.ibo); // Note, GL_ELEMENT_ARRAY_BUFFER associates indices to the bound GL_ARRAY_BUFFER
    gl_has_errors();

    // Draw the screen texture on the quad geometry
    gl_has_errors();

    // Set clock
    GLuint time_uloc       = glGetUniformLocation(wall_screen_sprite.effect.program, "time");
    GLuint dead_timer_uloc = glGetUniformLocation(wall_screen_sprite.effect.program, "darken_screen_factor");
    GLint in_player_x = glGetUniformLocation(wall_screen_sprite.effect.program, "player_position_x");
    GLint in_player_y = glGetUniformLocation(wall_screen_sprite.effect.program, "player_position_y");


    glUniform1f(time_uloc, static_cast<float>(glfwGetTime() * 10.0f));
    auto& screen = ECS::registry<ScreenState>.get(screen_state_entity);
    glUniform1f(dead_timer_uloc, screen.darken_screen_factor);
    auto player_loc = ECS::registry<Soldier>.entities[0].get<Motion>().position;
    auto& camera = ECS::registry<Camera>.get(screen.camera);
    auto camera_loc = camera.get_position();
    glUniform1f(in_player_x, (player_loc.x -camera_loc.x)/ w);
    glUniform1f(in_player_y, (player_loc.y -camera_loc.y)/ h);
    gl_has_errors();

    // Set the vertex position and vertex texture coordinates (both stored in the same VBO)
    GLint in_position_loc = glGetAttribLocation(wall_screen_sprite.effect.program, "in_position");
    glEnableVertexAttribArray(in_position_loc);
    glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
    GLint in_texcoord_loc = glGetAttribLocation(wall_screen_sprite.effect.program, "in_texcoord");
    glEnableVertexAttribArray(in_texcoord_loc);
    glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3)); // note the stride to skip the preceeding vertex position

    gl_has_errors();

    // Bind our texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, wall_screen_sprite.texture.texture_id);

    // Draw
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr); // two triangles = 6 vertices; nullptr indicates that there is no offset from the bound index buffer
    glBindVertexArray(0);
    gl_has_errors();
}

// Render our game world
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-14-render-to-texture/
void RenderSystem::draw(vec2 window_size_in_game_units)
{
	// Getting size of window
	ivec2 frame_buffer_size; // in pixels
	glfwGetFramebufferSize(&window, &frame_buffer_size.x, &frame_buffer_size.y);

	// First render to the custom framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	gl_has_errors();

	// Clearing backbuffer
	glViewport(0, 0, frame_buffer_size.x, frame_buffer_size.y);
	glDepthRange(0.00001, 10);
	glClearColor(0, 0, 0, 1.0);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gl_has_errors();

	// Fake projection matrix, scales with respect to window coordinates
	float left = 0.f;
	float top = 0.f;
	float right = window_size_in_game_units.x;
	float bottom = window_size_in_game_units.y;

	float sx = 2.f / (right - left);
	float sy = 2.f / (top - bottom);
	float tx = -(right + left) / (right - left);
	float ty = -(top + bottom) / (top - bottom);
	mat3 projection_2D{ { sx, 0.f, 0.f },{ 0.f, sy, 0.f },{ tx, ty, 1.f } };
    auto& screen = screen_state_entity.get<ScreenState>();

    if(ECS::registry<Camera>.size() == 0){
        screen.camera.insert(Camera({0, 0}));
    } else if(!screen.camera.has<Camera>()) {
        screen.camera = ECS::registry<Camera>.entities[0];
    }
    auto& camera = ECS::registry<Camera>.get(screen.camera);
    camera.set_screen_size(window_size_in_game_units);
	// Draw all textured meshes that have a position and size component
    // Draw by the order of motion zValue, the smaller zValue, draw earlier
    auto entities = ECS::registry<ShadedMeshRef>.entities;
    sort(entities.begin(), entities.end(), [](const ECS::Entity e1, const ECS::Entity e2)
    {
        return ECS::registry<Motion>.get(e1).zValue < ECS::registry<Motion>.get(e2).zValue;
    });
    
	for (ECS::Entity entity : entities)
	{
		if (!ECS::registry<Motion>.has(entity))
			continue;
		// Note, its not very efficient to access elements indirectly via the entity albeit iterating through all Sprites in sequence
		drawTexturedMesh(entity, projection_2D);
		gl_has_errors();
	}


    glBindFramebuffer(GL_FRAMEBUFFER, wall_frame_buffer);
    gl_has_errors();
    // Clearing backbuffer
    glViewport(0, 0, frame_buffer_size.x, frame_buffer_size.y);
    gl_has_errors();
    glDepthRange(0.00001, 10);
    gl_has_errors();

    glClearColor(1, 1, 1, 1.0);
    gl_has_errors();

    glClearDepth(1.f);
    gl_has_errors();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl_has_errors();
    // Draw all textured meshes that have a position and size component
    // Draw by the order of motion zValue, the smaller zValue, draw earlier
    auto wall_entities = ECS::registry<Wall>.entities;
    for (ECS::Entity entity : wall_entities)
    {
        if (!ECS::registry<Motion>.has(entity) || !ECS::registry<ShadedMeshRef>.has(entity))
            continue;
        // Note, its not very efficient to access elements indirectly via the entity albeit iterating through all Sprites in sequence
        drawTexturedMesh(entity, projection_2D);
        gl_has_errors();
    }

    auto moveable_wall_entities = ECS::registry<MoveableWall>.entities;
    for (ECS::Entity entity : moveable_wall_entities)
    {
        if (!ECS::registry<Motion>.has(entity) || !ECS::registry<ShadedMeshRef>.has(entity))
            continue;
        // Note, its not very efficient to access elements indirectly via the entity albeit iterating through all Sprites in sequence
        drawTexturedMesh(entity, projection_2D);
        gl_has_errors();
    }


	// Truely render to the screen
    drawLights();

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
    gl_has_errors();
	drawToScreen();

	// flicker-free display with a double buffer
	glfwSwapBuffers(&window);
}

const std::string RenderSystem::build_anim_vertex_shader(int frames) {
    int width = ceil(sqrt(frames));
    std::stringstream output;
    output << "#version 330\n"
           << "\n"
           << "// Input attributes\n"
           << "in vec3 in_position;\n"
           << "in vec2 in_texcoord;\n"
           << "uniform float time;\n"
           << "// Passed to fragment shader\n"
           << "out vec2 texcoord;\n"
           << "\n"
           << "// Application data\n"
           << "uniform mat3 transform;\n"
           << "uniform mat3 projection;\n"
           << "\n"
           << "void main()\n"
           << "{\n"
           << "    float mytime = floor(mod(int(time), "<< frames <<".0));\n"
           << "    float x_offset = mod(mytime, "<< width<<".0); \n"
           << "    float y_offset = mod(floor(mytime/"<< width << "), "<< width<<".0); \n"
           << "    texcoord = vec2((in_texcoord.x + x_offset) * 1.0/" << width <<".0 , (in_texcoord.y + y_offset) * 1.0/" << width <<".0);\n"
           << "    vec3 pos = projection * transform * vec3(in_position.xy, 1.0);\n"
           << "    gl_Position = vec4(pos.xy, in_position.z, 1.0);\n"
           << "}";
    std::string ret = output.str();
    return ret;
}

void gl_has_errors()
{
	GLenum error = glGetError();

	if (error == GL_NO_ERROR)
		return;
	
	const char* error_str = "";
	while (error != GL_NO_ERROR)
	{
		switch (error)
		{
		case GL_INVALID_OPERATION:
			error_str = "INVALID_OPERATION";
			break;
		case GL_INVALID_ENUM:
			error_str = "INVALID_ENUM";
			break;
		case GL_INVALID_VALUE:
			error_str = "INVALID_VALUE";
			break;
		case GL_OUT_OF_MEMORY:
			error_str = "OUT_OF_MEMORY";
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			error_str = "INVALID_FRAMEBUFFER_OPERATION";
			break;
		}

		std::cerr << "OpenGL:" << error_str << std::endl;
		error = glGetError();
	}
	throw std::runtime_error("last OpenGL error:" + std::string(error_str));
}
