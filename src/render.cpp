// internal
#include "render.hpp"
#include "render_components.hpp"
#include "tiny_ecs.hpp"
#include "Camera.hpp"
#include "Explosion.hpp"
#include "button.hpp"
#include "GameInstance.hpp"
#include "WeaponTimer.hpp"

#include <iostream>
#include <sstream>
#include <soldier.hpp>
#include <Wall.hpp>
#include <MoveableWall.hpp>
#include <Particle.hpp>
#include <MagicParticle.hpp>
#include <highlight_circle.hpp>
#include <pop_up.hpp>

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
    transform.translate(relative_to_screen? motion.position: (motion.position - camera.get_position()));
    transform.rotate(motion.angle);
    transform.scale(motion.scale);
    // !!! TODO A1: add rotation to the chain of transformations, mind the order of transformations

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

    GLint radius_uloc = glGetUniformLocation(texmesh.effect.program, "radius");
    if (radius_uloc >= 0) {
        if (entity.has<HighLightCircle>()) {
            glUniform1f(radius_uloc, entity.get<HighLightCircle>().radius);
        }
    }
    gl_has_errors();
    GLint thickness_uloc = glGetUniformLocation(texmesh.effect.program, "thickness");
    if (thickness_uloc >= 0) {
        if (entity.has<HighLightCircle>()) {
            glUniform1f(thickness_uloc, entity.get<HighLightCircle>().thickness);
        }
    }
    gl_has_errors();
    GLint center_uloc = glGetUniformLocation(texmesh.effect.program, "center");
    if (center_uloc >= 0) { 
        glUniform2fv(center_uloc, 1, (float*)&(motion.position - camera.get_position()));
    }
    gl_has_errors();
    // Drawing of num_indices/3 triangles specified in the index buffer
    glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);
}

void RenderSystem::drawInstanced(const mat3& projection, Particle& particle) {
    auto& screen = screen_state_entity.get<ScreenState>();
    auto& camera = ECS::registry<Camera>.get(screen.camera);
    // Transformation code, see Rendering and Transformation in the template specification for more info
// Incrementally updates transformation matrix, thus ORDER IS IMPORTANT

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


    // glBindBuffer(GL_ARRAY_BUFFER, texmesh.mesh.vbo);

    
    gl_has_errors();


    // Getting uniform locations for glUniform* calls

    glUniform3fv(color_uloc, 1, (float*)&particle.mesh.texture.color);
    gl_has_errors();

    // Get number of indices from index buffer, which has elements uint16_t
    GLint size = 0;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    gl_has_errors();
    GLsizei num_indices = size / sizeof(uint16_t);
    //GLsizei num_triangles = num_indices / 3;

    // Setting uniform values to the currently bound program
    glUniformMatrix3fv(projection_uloc, 1, GL_FALSE, (float*)&projection);
    vec2 camera_position = camera.get_position();
    glUniform2fv(camera_position_uloc, 1, (float*)&camera_position);

    gl_has_errors();
    // Drawing of num_indices/3 triangles specified in the index buffer
    glDrawElementsInstanced(GL_TRIANGLES,num_indices, GL_UNSIGNED_SHORT, nullptr, particle.motions.size());
    // glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);

    glBindVertexArray(0);
    gl_has_errors();
}



// Draw the intermediate texture to the screen, with some distortion to simulate water
void RenderSystem::drawToScreen(vec2 window_size_in_game_units)
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
    GLint in_player = glGetUniformLocation(screen_sprite.effect.program, "player_position");
    GLint texture_size_loc = glGetUniformLocation(screen_sprite.effect.program, "texture_size");
    GLint world_size_loc = glGetUniformLocation(screen_sprite.effect.program, "world_size");
    GLint light_intensity_loc = glGetUniformLocation(screen_sprite.effect.program, "light_intensity");
    gl_has_errors();
	auto& screen = ECS::registry<ScreenState>.get(screen_state_entity);
    gl_has_errors();
    glUniform1f(texture_size_loc, light_frame_texture.size.x);
    gl_has_errors();
    vec2 world_size{w,h};
    glUniform2fv(world_size_loc, 1, (float*)&world_size);
    if(!ECS::registry<Soldier>.entities.empty() && ECS::registry<Soldier>.entities[0].has<Motion>() && ECS::registry<Camera>.has(screen.camera)) {
        auto player_loc = ECS::registry<Soldier>.entities[0].get<Motion>().position;
        auto &camera = ECS::registry<Camera>.get(screen.camera);
        auto camera_loc = camera.get_position();
        vec2 player_loccation{(player_loc.x - camera_loc.x) /window_size_in_game_units.x, (player_loc.y - camera_loc.y) / window_size_in_game_units.y};
        glUniform2fv(in_player, 1, (float*)&player_loccation);
        glUniform1f(light_intensity_loc, ECS::registry<Soldier>.components[0].light_intensity);
    }
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
    GLint ui_texture_loc = glGetUniformLocation(screen_sprite.effect.program, "ui_texture");
    GLint light_texture_loc = glGetUniformLocation(screen_sprite.effect.program, "lighting_texture");
    glUniform1i(normal_texture_loc, 0);
    glUniform1i(ui_texture_loc,  1);
    glUniform1i(light_texture_loc, 2);

	// Bind our texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, screen_sprite.texture.texture_id);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ui_texture.texture_id);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, light_frame_texture.texture_id);

	// Draw
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr); // two triangles = 6 vertices; nullptr indicates that there is no offset from the bound index buffer
	glBindVertexArray(0);
	gl_has_errors();
}
 

// Draw the intermediate texture to the screen, with some distortion to simulate water
void RenderSystem::drawLights(vec2 window_size_in_game_units)
{
    // Setting shaders
    glUseProgram(wall_screen_sprite.effect.program);
    glBindVertexArray(wall_screen_sprite.mesh.vao);
    gl_has_errors();

    // Clearing backbuffer
    int w, h;
    glfwGetFramebufferSize(&window, &w, &h);
//    w = light_frame_texture.size.x;
//    h = light_frame_texture.size.y;
//    //printf("w: %d\n", w);
    glBindFramebuffer(GL_FRAMEBUFFER, light_frame_buffer);
    glViewport(0, 0, light_frame_texture.size.x, light_frame_texture.size.y);
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
    GLint in_player = glGetUniformLocation(wall_screen_sprite.effect.program, "player_position");
    GLint texture_size_loc = glGetUniformLocation(wall_screen_sprite.effect.program, "texture_size");
    GLint world_size_loc = glGetUniformLocation(wall_screen_sprite.effect.program, "world_size");


    glUniform1f(time_uloc, static_cast<float>(glfwGetTime() * 10.0f));
    glUniform1f(texture_size_loc, light_frame_texture.size.x);

    auto& screen = ECS::registry<ScreenState>.get(screen_state_entity);
    vec2 world_size{w,h};
    glUniform2fv(world_size_loc, 1, (float*)&world_size);
    glUniform1f(dead_timer_uloc, screen.darken_screen_factor);
    if(!ECS::registry<Soldier>.entities.empty() && ECS::registry<Soldier>.entities[0].has<Motion>() && ECS::registry<Camera>.has(screen.camera)) {
        auto player_loc = ECS::registry<Soldier>.entities[0].get<Motion>().position;
        auto &camera = ECS::registry<Camera>.get(screen.camera);
        auto camera_loc = camera.get_position();
        vec2 player_loccation{(player_loc.x - camera_loc.x) / window_size_in_game_units.x, (player_loc.y - camera_loc.y) / window_size_in_game_units.y};
        glUniform2fv(in_player, 1, (float *) &player_loccation);
    }
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

    glBindFramebuffer(GL_FRAMEBUFFER, ui_buffer);
    gl_has_errors();

    // Clearing backbuffer
    glViewport(0, 0, frame_buffer_size.x, frame_buffer_size.y);
    glDepthRange(0.00001, 10);
    glClearColor(0, 0, 0, 0);
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

    // Render UI
    if(GameInstance::isPlayableLevel()){
        auto& e = ECS::registry<Health>.entities;
        for(auto& entity: e){
            if (entity.has<Motion>()) {
                auto& health = entity.get<Health>();
                auto& enemy_motion = entity.get<Motion>();
                Motion motion{};
                motion.position = enemy_motion.position + health.health_bar_offset;
                motion.scale = {50,5};
                motion.position.x -= motion.scale.x /2;
                motion.angle = 0;
                drawTexturedMesh(entity, projection_2D, motion, health_bar_background);
                motion.scale.x *= health.hp / health.max_hp;
                if (health.hp >= 0) {
                    drawTexturedMesh(entity, projection_2D, motion, health_bar);
                }
            }
        }

        auto& wts = ECS::registry<WeaponTimer>.entities;
        for(auto& entity: wts){
            if (entity.has<Motion>()) {
                auto& et = ECS::registry<EffectTimer>.get(entity);
                auto& entity_motion = entity.get<Motion>();

                Motion timer_mesh_motion{};
                timer_mesh_motion.position = entity_motion.position;
                timer_mesh_motion.scale = entity_motion.scale;
                timer_mesh_motion.angle = 0;
                RenderSystem::createWeaponTimer(projection_2D, timer_mesh_motion, entity);

                Motion mask_motion{};
                mask_motion.position = entity_motion.position;
                mask_motion.scale = entity_motion.scale;
                mask_motion.position.x -= mask_motion.scale.x / 2;
                mask_motion.angle = 0;
                if (et.status == COOLDOWN) {
                    mask_motion.scale.x *= et.cooldown_ms / WeaponTimer::effectAttributes[et.type][1];
                } else {
                    mask_motion.scale.x = 0;
                }
                drawTexturedMesh(entity, projection_2D, mask_motion, weaponTimerMask);
            }
        }

        auto& circles = ECS::registry<HighLightCircle>.entities;
        for (auto& entity : circles) {
            if (entity.has<Motion>()) {
                drawTexturedMesh(entity, projection_2D);
            }
        }

        auto& pop_ups = ECS::registry<PopUP>.entities;
        for (auto& entity : pop_ups) {
            if (entity.has<Motion>()) {
                auto& motion = ECS::registry<Motion>.get(entity);
                auto& texmesh = *ECS::registry<ShadedMeshRef>.get(entity).reference_to_cache;
                drawTexturedMesh(entity, projection_2D, motion, PopUP::get_background(), true);
                drawTexturedMesh(entity, projection_2D, motion, texmesh, true);
            }
        }

    }

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

    glClearColor(1, 1, 1, 0);
    gl_has_errors();

    // Draw all textured meshes that have a position and size component
    // Draw by the order of motion zValue, the smaller zValue, draw earlier
    auto entities = ECS::registry<ShadedMeshRef>.entities;
    sort(entities.begin(), entities.end(), [](const ECS::Entity e1, const ECS::Entity e2)
    {
        return ECS::registry<Motion>.get(e1).zValue < ECS::registry<Motion>.get(e2).zValue;
    });

    for (ECS::Entity entity : entities)
    {
        if (!ECS::registry<Motion>.has(entity) || entity.get<ShadedMeshRef>().is_ui)
            continue;
        // Note, its not very efficient to access elements indirectly via the entity albeit iterating through all Sprites in sequence
        drawTexturedMesh(entity, projection_2D);
        gl_has_errors();
    }

    for (auto entity : ECS::registry<Particle>.entities) {
        auto& p = entity.get<Particle>();
        //drawTexturedMesh(entity, projection_2D, p.motions[5], p.mesh);
        drawInstanced(projection_2D, p);
    }
    for (auto entity : ECS::registry<MagicParticle>.entities) {
        auto& p = entity.get<MagicParticle>();
        // drawTexturedMesh(entity, projection_2D, entity.get<Motion>(), *entity.get<ShadedMeshRef>().reference_to_cache);
        //drawInstanced(projection_2D, p.motions, p.mesh, p.motion_buffer);
    }
    


    glBindFramebuffer(GL_FRAMEBUFFER, wall_frame_buffer);
    gl_has_errors();
    // Clearing backbuffer
    glViewport(0, 0, frame_buffer_size.x, frame_buffer_size.y);
    gl_has_errors();
    glDepthRange(0.00001, 10);
    gl_has_errors();

    glClearColor(1, 1, 1, 1.0);
    glClearColor(1, 1, 1, 0);
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
    drawLights(window_size_in_game_units);

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
    gl_has_errors();
	drawToScreen(window_size_in_game_units);
    glfwSwapInterval( 0 );
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

void RenderSystem::createWeaponTimer(mat3 projection_2D, Motion timer_mesh_motion, ECS::Entity weaponTimer_entity) {
    auto wt = ECS::registry<WeaponTimer>.get(weaponTimer_entity);
    std::string key = "weaponTimer_" + wt.texture_path;
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0) {
        resource = ShadedMesh();
        RenderSystem::createSprite(resource, textures_path("/bullet/"+wt.texture_path+".png"), "textured");
        drawTexturedMesh(weaponTimer_entity, projection_2D, timer_mesh_motion, resource);
    } else {
        drawTexturedMesh(weaponTimer_entity, projection_2D, timer_mesh_motion, resource);
    }
}