// internal
#include "render.hpp"
#include "render_components.hpp"
#include "tiny_ecs.hpp"
#include "Camera.hpp"
#include "button.hpp"
#include "highlight_circle.hpp"
#include "pop_up.hpp"
#include "Particle.hpp"

#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>

void RenderSystem::drawTexturedMesh(ECS::Entity entity, const mat3& projection, bool relative_to_screen)
{
    auto& motion = ECS::registry<Motion>.get(entity);
    auto& texmesh = *ECS::registry<ShadedMeshRef>.get(entity).reference_to_cache;
    drawTexturedMesh(entity, projection, motion, texmesh, relative_to_screen);
}

void RenderSystem::drawTexturedMesh(ECS::Entity entity, const mat3 &projection, Motion &motion, const ShadedMesh &texmesh, bool relative_to_screen) {
    auto& screen = screen_state_entity.get<ScreenState>();
    auto& camera = ECS::registry<Camera>.get(screen.camera);

    // Transformation code, see Rendering and Transformation in the template specification for more info
    // Incrementally updates transformation matrix, thus ORDER IS IMPORTANT
    Transform transform;
    if (relative_to_screen) {
        transform.translate(motion.position);
    } else {
        transform.translate(motion.position - camera.get_position());
    }
    transform.rotate(motion.angle);
    transform.scale(motion.scale);

    // Setting shaders
    glUseProgram(texmesh.effect.program);
    glBindVertexArray(texmesh.mesh.vao);
    gl_has_errors();

    // Enabling alpha channel for textures
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
    }
    else if (in_color_loc >= 0) {
        glEnableVertexAttribArray(in_position_loc);
        glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(in_color_loc);
        glVertexAttribPointer(in_color_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), reinterpret_cast<void*>(sizeof(vec3)));
    } else if (in_position_loc >= 0) {
        glEnableVertexAttribArray(in_position_loc);
        glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), reinterpret_cast<void*>(0));
	} else {
		throw std::runtime_error("This type of entity is not yet supported");
	}
    gl_has_errors();

    // Getting uniform locations for glUniform* calls
    GLint color_uloc = glGetUniformLocation(texmesh.effect.program, "fcolor");
    glUniform3fv(color_uloc, 1, (float*)&texmesh.texture.color);
    gl_has_errors();

    // Get number of indices from index buffer, which has elements uint16_t
    GLint size = 0;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    gl_has_errors();
    GLsizei num_indices = size / sizeof(uint16_t);

    // Setting uniform values to the currently bound program
    glUniformMatrix3fv(transform_uloc, 1, GL_FALSE, (float*)&transform.mat);
    glUniformMatrix3fv(projection_uloc, 1, GL_FALSE, (float*)&projection);
    gl_has_errors();

    // Highlight a selected button by tinting its fill color
    if (color_uloc >= 0 && entity.has<Button>() && entity.get<Button>().selected()) {
        float color[] = { 0.f, 0.5f, 1.f };
        glUniform3fv(color_uloc, 1, color);
    }

    // Selection-ring style shaders read the ring geometry from these uniforms
    GLint radius_uloc = glGetUniformLocation(texmesh.effect.program, "radius");
    if (radius_uloc >= 0 && entity.has<HighLightCircle>()) {
        glUniform1f(radius_uloc, entity.get<HighLightCircle>().radius);
    }
    GLint thickness_uloc = glGetUniformLocation(texmesh.effect.program, "thickness");
    if (thickness_uloc >= 0 && entity.has<HighLightCircle>()) {
        glUniform1f(thickness_uloc, entity.get<HighLightCircle>().thickness);
    }
    GLint center_uloc = glGetUniformLocation(texmesh.effect.program, "center");
    if (center_uloc >= 0) {
        vec2 center_loc = motion.position - camera.get_position();
        glUniform2fv(center_uloc, 1, (float*)&(center_loc));
    }
    gl_has_errors();

    // Drawing of num_indices/3 triangles specified in the index buffer
    glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);
}

void RenderSystem::drawInstanced(const mat3& projection, Particle& particle) {
    auto& screen = screen_state_entity.get<ScreenState>();
    auto& camera = ECS::registry<Camera>.get(screen.camera);

    // Setting shaders
    glUseProgram(particle.mesh.effect.program);
    glBindVertexArray(particle.mesh.mesh.vao);
    gl_has_errors();

    // Enabling alpha channel for textures
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    gl_has_errors();

    GLint projection_uloc = glGetUniformLocation(particle.mesh.effect.program, "projection");
    gl_has_errors();
    // Setting vertex and index buffers
    glBindBuffer(GL_ARRAY_BUFFER, particle.mesh.mesh.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, particle.mesh.mesh.ibo);
    gl_has_errors();

    // Input data location as in the vertex buffer
    GLint in_position_loc = glGetAttribLocation(particle.mesh.effect.program, "in_position");
    GLint in_texcoord_loc = glGetAttribLocation(particle.mesh.effect.program, "in_texcoord");

    GLuint color_uloc = glGetUniformLocation(particle.mesh.effect.program, "fcolor");
    GLuint camera_position_uloc = glGetUniformLocation(particle.mesh.effect.program, "camera_position");
    GLuint delta_second_uloc = glGetUniformLocation(particle.mesh.effect.program, "delta_second");
    glUniform1f(delta_second_uloc, static_cast<float>(glfwGetTime()) - particle.start_time);

    glEnableVertexAttribArray(in_position_loc);
    glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(in_texcoord_loc);
    gl_has_errors();
    glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), reinterpret_cast<void*>(sizeof(vec3))); // note the stride to skip the preceeding vertex position
    // Enabling and binding texture to slot 0
    glActiveTexture(GL_TEXTURE0);
    gl_has_errors();
    glBindTexture(GL_TEXTURE_2D, particle.mesh.texture.texture_id);
    gl_has_errors();

    GLint in_transform_loc = glGetAttribLocation(particle.mesh.effect.program, "transform");
    glBindBuffer(GL_ARRAY_BUFFER, particle.motion_buffer);
    gl_has_errors();
    for (int i = 0; i < 3; i++) {
        gl_has_errors();
        glEnableVertexAttribArray(in_transform_loc + i);
        gl_has_errors();
        glVertexAttribPointer(in_transform_loc + i, 3, GL_FLOAT, GL_FALSE, sizeof(mat3), (GLvoid*)(sizeof(vec3) * i));
        glVertexAttribDivisor(in_transform_loc + i, 1);
    }

    GLint in_scale_speed_loc = glGetAttribLocation(particle.mesh.effect.program, "scale_speed");
    GLint in_speed_loc = glGetAttribLocation(particle.mesh.effect.program, "speed");
    glBindBuffer(GL_ARRAY_BUFFER, particle.scale_speed_buffer);
    glEnableVertexAttribArray(in_scale_speed_loc);
    gl_has_errors();
    glVertexAttribPointer(in_scale_speed_loc, 1, GL_FLOAT, GL_FALSE, sizeof(float), 0);
    glVertexAttribDivisor(in_scale_speed_loc, 1);

    glBindBuffer(GL_ARRAY_BUFFER, particle.speed_buffer);
    glEnableVertexAttribArray(in_speed_loc);
    gl_has_errors();
    glVertexAttribPointer(in_speed_loc, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), 0);
    glVertexAttribDivisor(in_speed_loc, 1);
    gl_has_errors();

    // Getting uniform locations for glUniform* calls
    glUniform3fv(color_uloc, 1, (float*)&particle.mesh.texture.color);
    gl_has_errors();

    // Get number of indices from index buffer, which has elements uint16_t
    GLint size = 0;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    gl_has_errors();
    GLsizei num_indices = size / sizeof(uint16_t);

    // Setting uniform values to the currently bound program
    glUniformMatrix3fv(projection_uloc, 1, GL_FALSE, (float*)&projection);
    vec2 camera_position = camera.get_position();
    glUniform2fv(camera_position_uloc, 1, (float*)&camera_position);
    gl_has_errors();

    // Drawing of num_indices/3 triangles specified in the index buffer
    glDrawElementsInstanced(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr, particle.motions.size());
    glBindVertexArray(0);
    gl_has_errors();
}

// Render our game world straight to the back buffer.
void RenderSystem::draw(vec2 window_size_in_game_units)
{
	// Getting size of window
	ivec2 frame_buffer_size; // in pixels
	glfwGetFramebufferSize(&window, &frame_buffer_size.x, &frame_buffer_size.y);

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
    if (ECS::registry<Camera>.size() == 0) {
        screen.camera.insert(Camera({ 0, 0 }));
    } else if (!screen.camera.has<Camera>()) {
        screen.camera = ECS::registry<Camera>.entities[0];
    }

    auto& camera = ECS::registry<Camera>.get(screen.camera);
    camera.set_screen_size(window_size_in_game_units);

    // Aim the flashlight at the cursor (screen+height space; gl_FragCoord origin is bottom-left).
    double mx = 0.0, my = 0.0;
    glfwGetCursorPos(&window, &mx, &my);
    deferred.lights.flashlight.pos.x = static_cast<float>(mx);
    deferred.lights.flashlight.pos.y = static_cast<float>(frame_buffer_size.y) - static_cast<float>(my);

    // Number keys 0-8 switch the composite debug view (0 = full pipeline).
    for (int k = 0; k <= 8; k++) {
        if (glfwGetKey(&window, GLFW_KEY_0 + k) == GLFW_PRESS) deferred.debugMode = k;
    }

    // Pass 1..6: deferred lit scene composited straight to the back buffer.
    deferred.draw(camera, frame_buffer_size, projection_2D);

    // Forward overlays drawn on top of the composited scene (no clear): legacy non-lit
    // sprites, particles, and UI.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, frame_buffer_size.x, frame_buffer_size.y);
    gl_has_errors();

    // Draw all textured meshes that have a position and size component, painter-sorted by zValue.
    auto entities = ECS::registry<ShadedMeshRef>.entities;
    std::sort(entities.begin(), entities.end(), [&](const ECS::Entity e1, const ECS::Entity e2)
    {
        return ECS::registry<Motion>.get(e1).zValue < ECS::registry<Motion>.get(e2).zValue;
    });

    for (ECS::Entity entity : entities)
    {
        if (!ECS::registry<Motion>.has(entity) || entity.get<ShadedMeshRef>().is_ui)
            continue;
        drawTexturedMesh(entity, projection_2D);
        gl_has_errors();
    }

    // Instanced particle systems
    for (auto entity : ECS::registry<Particle>.entities) {
        auto& p = entity.get<Particle>();
        drawInstanced(projection_2D, p);
    }

    // Screen-space UI overlays
    for (auto& entity : ECS::registry<ShadedMeshRefUI>.entities) {
        if (entity.has<Motion>()) {
            drawTexturedMesh(entity, projection_2D, entity.get<Motion>(), *entity.get<ShadedMeshRefUI>().reference_to_cache, true);
        }
    }

    for (auto& entity : ECS::registry<HighLightCircle>.entities) {
        if (entity.has<Motion>()) {
            drawTexturedMesh(entity, projection_2D);
        }
    }

    for (auto& entity : ECS::registry<PopUP>.entities) {
        if (entity.has<Motion>()) {
            auto& motion = ECS::registry<Motion>.get(entity);
            auto& texmesh = *ECS::registry<ShadedMeshRef>.get(entity).reference_to_cache;
            drawTexturedMesh(entity, projection_2D, motion, texmesh, true);
        }
    }

    glfwSwapInterval(0);
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
