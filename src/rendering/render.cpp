// internal
#include "render.hpp"
#include "render_components.hpp"
#include "tiny_ecs.hpp"
#include "Camera.hpp"
#include "Explosion.hpp"
#include "button.hpp"
#include "mainMenu.hpp"
#include "start.hpp"
#include "GameInstance.hpp"
#include "WeaponTimer.hpp"

#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <soldier.hpp>
#include <Wall.hpp>
#include <MoveableWall.hpp>
#include <Particle.hpp>
#include <MagicParticle.hpp>
#include <highlight_circle.hpp>
#include <pop_up.hpp>
#include <Enemy.hpp>
#include <Weapon.hpp>

namespace {
    constexpr float PLAYER_LIGHT_CONE_DEGREES = 90.f;
    constexpr float PLAYER_LIGHT_INNER_RADIUS = 80.f;
    constexpr float PLAYER_LIGHT_INNER_SOFT_EDGE_RATIO = 0.35f;
    constexpr float PLAYER_LIGHT_INNER_SOFT_EDGE_MIN = 12.f;
    constexpr float PLAYER_LIGHT_FOV_SOFT_EDGE = 0.08f;
    bool rendering_wall_blockers = false;
    bool rendering_wall_surface_mask = false;

    bool oblique_view_enabled() {
        return GameInstance::isPlayableLevel();
    }

    bool is_ground_entity(ECS::Entity entity) {
        if (entity.has<IsoGround>()) {
            return true;
        }
        if (entity.has<Motion>()) {
            return entity.get<Motion>().zValue <= ZValuesMap["Background"];
        }
        return false;
    }

    bool should_project_mesh_geometry(ECS::Entity entity) {
        return entity.has<IsoGround>() || entity.has<Wall>() || entity.has<MoveableWall>();
    }

    float projected_angle_for_motion(const Camera& camera, float world_angle) {
        vec2 direction = camera.world_delta_to_screen({ std::cos(world_angle), std::sin(world_angle) });
        if (length(direction) < 0.0001f) {
            return world_angle;
        }
        return std::atan2(direction.y, direction.x);
    }

    int directional_sprite_index(const Camera& camera, float world_angle) {
        float angle = projected_angle_for_motion(camera, world_angle);
        int index = static_cast<int>(std::round(angle / (PI / 4.f)));
        index %= 8;
        if (index < 0) {
            index += 8;
        }
        return index;
    }

    ShadedMesh& directional_sprite_mesh(ECS::Entity entity, const Camera& camera) {
        auto& directional = entity.get<DirectionalSprite>();
        int index = directional_sprite_index(camera, entity.get<Motion>().angle);
        std::string key = directional.cache_prefix + "_" + std::to_string(index);
        ShadedMesh& resource = cache_resource(key);
        if (resource.effect.program.resource == 0) {
            resource = ShadedMesh();
            RenderSystem::createSprite(resource, textures_path(directional.texture_paths[index]), "sprite_textured");
        }
        return resource;
    }

    bool should_hide_directional_child_weapon(ECS::Entity entity) {
        if (!entity.has<Weapon>() || !entity.has<ParentEntity>()) {
            return false;
        }
        auto parent = entity.get<ParentEntity>().parent;
        return parent.has<DirectionalSprite>();
    }

    float compute_pixels_per_unit(vec2 window_size_in_game_units, ivec2 framebuffer_size) {
        float pixels_per_unit_x = static_cast<float>(framebuffer_size.x) / window_size_in_game_units.x;
        float pixels_per_unit_y = static_cast<float>(framebuffer_size.y) / window_size_in_game_units.y;
        return std::min(pixels_per_unit_x, pixels_per_unit_y);
    }

    float compute_inner_radius_pixels(vec2 window_size_in_game_units, ivec2 framebuffer_size) {
        return PLAYER_LIGHT_INNER_RADIUS * compute_pixels_per_unit(window_size_in_game_units, framebuffer_size);
    }

    float compute_inner_soft_edge_pixels(float radius_pixels) {
        return std::max(radius_pixels * PLAYER_LIGHT_INNER_SOFT_EDGE_RATIO, PLAYER_LIGHT_INNER_SOFT_EDGE_MIN);
    }

    vec2 rotate_vec(vec2 value, float angle) {
        float ca = std::cos(angle);
        float sa = std::sin(angle);
        return {
            value.x * ca - value.y * sa,
            value.x * sa + value.y * ca
        };
    }

    bool should_render_wall_prism(ECS::Entity entity) {
        return oblique_view_enabled() && !rendering_wall_blockers && (entity.has<Wall>() || entity.has<MoveableWall>());
    }

    ShadedMesh& wall_prism_mesh() {
        ShadedMesh& resource = cache_resource("wall_prism_dynamic");
        if (resource.effect.program.resource == 0) {
            resource = ShadedMesh();
            resource.mesh.vertices = {
                ColoredVertex{ {0.f, 0.f, -0.02f}, {1.f, 1.f, 1.f} },
                ColoredVertex{ {1.f, 0.f, -0.02f}, {1.f, 1.f, 1.f} },
                ColoredVertex{ {0.f, 1.f, -0.02f}, {1.f, 1.f, 1.f} }
            };
            resource.mesh.vertex_indices = { 0, 1, 2 };
            resource.texture.color = { 1.f, 1.f, 1.f };
            RenderSystem::createColoredMesh(resource, "mesh_flat_highlight");
        }
        return resource;
    }

}

void RenderSystem::drawWallPrism(ECS::Entity entity, const mat3& projection, const Camera& camera, Motion& motion) {
    const bool mask_pass = rendering_wall_surface_mask;
    const vec3 top_color = mask_pass ? vec3{ 1.f, 1.f, 1.f } : vec3{ 0.82f, 0.92f, 0.94f };
    const vec3 lit_side_color = mask_pass ? vec3{ 1.f, 1.f, 1.f } : vec3{ 0.48f, 0.61f, 0.66f };
    const vec3 shadow_side_color = mask_pass ? vec3{ 1.f, 1.f, 1.f } : vec3{ 0.16f, 0.22f, 0.26f };

    vec2 center = camera.world_to_screen(motion.position);
    vec2 half_x = camera.world_delta_to_screen(rotate_vec({ motion.scale.x * 0.5f, 0.f }, motion.angle));
    vec2 half_y = camera.world_delta_to_screen(rotate_vec({ 0.f, motion.scale.y * 0.5f }, motion.angle));
    bool x_is_thin_axis = std::abs(motion.scale.x) < std::abs(motion.scale.y);
    vec2 top_half_x = half_x * (x_is_thin_axis ? 0.62f : 0.96f);
    vec2 top_half_y = half_y * (x_is_thin_axis ? 0.96f : 0.62f);
    float wall_height = std::clamp(1.1f * std::min(std::abs(motion.scale.x), std::abs(motion.scale.y)), 46.f, 150.f);
    vec2 drop = { 0.f, wall_height };

    vec2 top0 = center - top_half_x - top_half_y;
    vec2 top1 = center + top_half_x - top_half_y;
    vec2 top2 = center + top_half_x + top_half_y;
    vec2 top3 = center - top_half_x + top_half_y;
    vec2 base1 = center + half_x - half_y + drop;
    vec2 base2 = center + half_x + half_y + drop;
    vec2 base3 = center - half_x + half_y + drop;

    std::vector<ColoredVertex> vertices = {
        { { top0.x, top0.y, -0.02f }, top_color },
        { { top1.x, top1.y, -0.02f }, top_color },
        { { top2.x, top2.y, -0.02f }, top_color * 0.92f },
        { { top3.x, top3.y, -0.02f }, top_color * 0.95f },
        { { top1.x, top1.y, -0.02f }, shadow_side_color },
        { { top2.x, top2.y, -0.02f }, shadow_side_color },
        { { base2.x, base2.y, -0.02f }, shadow_side_color * 0.62f },
        { { base1.x, base1.y, -0.02f }, shadow_side_color * 0.72f },
        { { top3.x, top3.y, -0.02f }, lit_side_color },
        { { top2.x, top2.y, -0.02f }, lit_side_color },
        { { base2.x, base2.y, -0.02f }, lit_side_color * 0.7f },
        { { base3.x, base3.y, -0.02f }, lit_side_color * 0.82f }
    };
    std::vector<uint16_t> indices = {
        0, 3, 1, 1, 3, 2,
        4, 7, 5, 5, 7, 6,
        8, 11, 9, 9, 11, 10
    };

    ShadedMesh& resource = wall_prism_mesh();
    resource.mesh.vertices = vertices;
    resource.mesh.vertex_indices = indices;
    resource.texture.color = { 1.f, 1.f, 1.f };
    glBindBuffer(GL_ARRAY_BUFFER, resource.mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ColoredVertex) * resource.mesh.vertices.size(), resource.mesh.vertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, resource.mesh.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * resource.mesh.vertex_indices.size(), resource.mesh.vertex_indices.data(), GL_DYNAMIC_DRAW);

    Motion screen_motion{};
    screen_motion.position = { 0.f, 0.f };
    screen_motion.scale = { 1.f, 1.f };
    screen_motion.angle = 0.f;
    drawTexturedMesh(entity, projection, screen_motion, resource, true);
}

void RenderSystem::drawTexturedMesh(ECS::Entity entity, const mat3& projection, bool relative_to_screen)
{
    auto& motion = ECS::registry<Motion>.get(entity);
    auto& screen = screen_state_entity.get<ScreenState>();
    auto& camera = ECS::registry<Camera>.get(screen.camera);
    if (should_render_wall_prism(entity) && !relative_to_screen) {
        drawWallPrism(entity, projection, camera, motion);
        return;
    }
	auto& texmesh = entity.has<DirectionalSprite>()
        ? directional_sprite_mesh(entity, camera)
        : *ECS::registry<ShadedMeshRef>.get(entity).reference_to_cache;
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
        transform.rotate(motion.angle);
        transform.scale(motion.scale);
    } else if (oblique_view_enabled() && should_project_mesh_geometry(entity)) {
        Transform world_transform = getTransform(motion);
        transform.mat = camera.get_world_to_screen_transform() * world_transform.mat;
    } else if (oblique_view_enabled()) {
        transform.translate(camera.world_to_screen(motion.position));
        if (!entity.has<DirectionalSprite>()) {
            transform.rotate(projected_angle_for_motion(camera, motion.angle));
        }
        vec2 scale = motion.scale;
        if (entity.has<DirectionalSprite>()) {
            scale *= entity.get<DirectionalSprite>().visual_scale;
            scale.x = std::abs(scale.x);
        }
        transform.scale(scale);
    } else {
        transform.translate(motion.position - camera.get_position());
        transform.rotate(motion.angle);
        transform.scale(motion.scale);
    }
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
    glUniform3fv(color_uloc, 1, (float*)&texmesh.texture.color);
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
    if(color_uloc >= 0){
        if(entity.has<Button>() && entity.get<Button>().selected()){
            float color[] = { 0.f, 0.5f, 1.f };
            glUniform3fv(color_uloc, 1, color);
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
        vec2 center_loc = oblique_view_enabled() ? camera.world_to_screen(motion.position) : motion.position - camera.get_position();
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
    GLint inner_radius_loc = glGetUniformLocation(screen_sprite.effect.program, "player_inner_light_radius");
    GLint inner_soft_edge_loc = glGetUniformLocation(screen_sprite.effect.program, "player_inner_light_soft_edge");
    GLint player_forward_loc = glGetUniformLocation(screen_sprite.effect.program, "player_forward_direction");
    GLint player_light_cos_loc = glGetUniformLocation(screen_sprite.effect.program, "player_light_cos_half_angle");
    GLint player_light_fov_soft_edge_loc = glGetUniformLocation(screen_sprite.effect.program, "player_light_fov_soft_edge");
    GLint enable_full_black_loc = glGetUniformLocation(screen_sprite.effect.program, "enable_full_black");
    gl_has_errors();
	auto& screen = ECS::registry<ScreenState>.get(screen_state_entity);
    gl_has_errors();
    glUniform1f(texture_size_loc, light_frame_texture.size.x);
    gl_has_errors();
    vec2 world_size{w,h};
    glUniform2fv(world_size_loc, 1, (float*)&world_size);
    bool enable_full_black = GameInstance::currentLevel != "settings";
    glUniform1i(enable_full_black_loc, enable_full_black ? 1 : 0);
    float player_inner_light_radius_pixels = 0.f;
    float player_inner_light_soft_edge_pixels = 0.f;
    vec2 player_forward{1.f, 0.f};
    float player_light_cos_half_angle = -1.f;
    ivec2 framebuffer_pixels{w, h};
    if(!ECS::registry<Soldier>.entities.empty() && ECS::registry<Soldier>.entities[0].has<Motion>() && ECS::registry<Camera>.has(screen.camera)) {
        auto& player_entity = ECS::registry<Soldier>.entities[0];
        auto& player_motion = player_entity.get<Motion>();
        auto player_loc = player_motion.position;
        auto &camera = ECS::registry<Camera>.get(screen.camera);
        vec2 player_screen = oblique_view_enabled() ? camera.world_to_screen(player_loc) : player_loc - camera.get_position();
        vec2 player_loccation{player_screen.x / window_size_in_game_units.x, player_screen.y / window_size_in_game_units.y};
        glUniform2fv(in_player, 1, (float*)&player_loccation);
        glUniform1f(light_intensity_loc, ECS::registry<Soldier>.components[0].light_intensity);
        player_inner_light_radius_pixels = compute_inner_radius_pixels(window_size_in_game_units, framebuffer_pixels);
        player_inner_light_soft_edge_pixels = compute_inner_soft_edge_pixels(player_inner_light_radius_pixels);
        vec2 forward_dir = oblique_view_enabled()
            ? camera.world_delta_to_screen({ std::cos(player_motion.angle), std::sin(player_motion.angle) })
            : vec2{ std::cos(player_motion.angle), -std::sin(player_motion.angle) };
        if (oblique_view_enabled()) {
            forward_dir.y *= -1.f;
        }
        float magnitude = std::sqrt(forward_dir.x * forward_dir.x + forward_dir.y * forward_dir.y);
        if (magnitude > 0.0001f) {
            player_forward = forward_dir / magnitude;
        }
        float half_angle_radians = (PLAYER_LIGHT_CONE_DEGREES * 0.5f) * PI / 180.f;
        player_light_cos_half_angle = std::cos(half_angle_radians);
    }
    glUniform1f(inner_radius_loc, player_inner_light_radius_pixels);
    glUniform1f(inner_soft_edge_loc, player_inner_light_soft_edge_pixels);
    glUniform2fv(player_forward_loc, 1, (float*)&player_forward);
    glUniform1f(player_light_cos_loc, player_light_cos_half_angle);
    glUniform1f(player_light_fov_soft_edge_loc, PLAYER_LIGHT_FOV_SOFT_EDGE);
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
    GLint wall_surface_texture_loc = glGetUniformLocation(screen_sprite.effect.program, "wall_surface_texture");
    glUniform1i(normal_texture_loc, 0);
    glUniform1i(ui_texture_loc,  1);
    glUniform1i(light_texture_loc, 2);
    glUniform1i(wall_surface_texture_loc, 3);

	// Bind our texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, screen_sprite.texture.texture_id);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ui_texture.texture_id);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, light_frame_texture.texture_id);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, wall_surface_texture.texture_id);

	// Draw
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr); // two triangles = 6 vertices; nullptr indicates that there is no offset from the bound index buffer
	glBindVertexArray(0);
	gl_has_errors();
}

void RenderSystem::drawMenuScene(const mat3& projection_2D, ivec2 frame_buffer_size)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, frame_buffer_size.x, frame_buffer_size.y);
    glDepthRange(0.00001, 10);
    glClearColor(0, 0, 0, 1.0f);
    glClearDepth(1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl_has_errors();

    auto entities = ECS::registry<ShadedMeshRef>.entities;
    sort(entities.begin(), entities.end(), [](const ECS::Entity e1, const ECS::Entity e2)
    {
        return ECS::registry<Motion>.get(e1).zValue < ECS::registry<Motion>.get(e2).zValue;
    });

    for (ECS::Entity entity : entities)
    {
        if (!ECS::registry<Motion>.has(entity) || entity.get<ShadedMeshRef>().is_ui || should_hide_directional_child_weapon(entity))
            continue;
        drawTexturedMesh(entity, projection_2D);
        gl_has_errors();
    }

    auto& ui_entities = ECS::registry<ShadedMeshRefUI>.entities;
    for (auto& entity : ui_entities) {
        if (entity.has<Motion>()) {
            drawTexturedMesh(entity, projection_2D, entity.get<Motion>(), *entity.get<ShadedMeshRefUI>().reference_to_cache, true);
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
            auto back_graound_motion = motion;
            back_graound_motion.scale *= 1.1f;
            drawTexturedMesh(entity, projection_2D, back_graound_motion, PopUP::get_background(), true);
            drawTexturedMesh(entity, projection_2D, motion, texmesh, true);
        }
    }

    glfwSwapInterval(0);
    glfwSwapBuffers(&window);
}
 

// Raycast the scene from the player to encode light reach per direction
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
        auto& player_entity = ECS::registry<Soldier>.entities[0];
        auto& player_motion = player_entity.get<Motion>();
        auto player_loc = player_motion.position;
        auto &camera = ECS::registry<Camera>.get(screen.camera);
        vec2 player_screen = oblique_view_enabled() ? camera.world_to_screen(player_loc) : player_loc - camera.get_position();
        vec2 player_loccation{player_screen.x / window_size_in_game_units.x, player_screen.y / window_size_in_game_units.y};
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

    bool has_player_pipeline = GameInstance::isPlayableLevel() || GameInstance::currentLevel == "settings";
    if (!has_player_pipeline) {
        drawMenuScene(projection_2D, frame_buffer_size);
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, ui_buffer);
    gl_has_errors();

    // Clearing backbuffer
    glViewport(0, 0, frame_buffer_size.x, frame_buffer_size.y);
    glDepthRange(0.00001, 10);
    glClearColor(0, 0, 0, 0);
    glClearDepth(1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl_has_errors();

    // Render UI

    if(GameInstance::isPlayableLevel()){
        auto& e = ECS::registry<Health>.entities;
        for(auto& entity: e){
            if (entity.has<Motion>() && !entity.has<Enemy>()) {
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

        // auto& wts = ECS::registry<WeaponTimer>.entities;
        // for(auto& entity: wts){
        //     if (entity.has<Motion>()) {
        //         auto& et = ECS::registry<EffectTimer>.get(entity);
        //         auto& entity_motion = entity.get<Motion>();

        //         Motion timer_mesh_motion{};
        //         timer_mesh_motion.position = entity_motion.position;
        //         timer_mesh_motion.scale = entity_motion.scale;
        //         timer_mesh_motion.angle = 0;
        //         RenderSystem::createWeaponTimer(projection_2D, timer_mesh_motion, entity);

        //         Motion mask_motion{};
        //         mask_motion.position = entity_motion.position;
        //         mask_motion.scale = entity_motion.scale;
        //         mask_motion.position.x -= mask_motion.scale.x / 2;
        //         mask_motion.angle = 0;
        //         if (et.status == COOLDOWN) {
        //             mask_motion.scale.x *= et.cooldown_ms / WeaponTimer::effectAttributes[et.type][1];
        //         } else {
        //             mask_motion.scale.x = 0;
        //         }
        //         drawTexturedMesh(entity, projection_2D, mask_motion, weaponTimerMask);
        //     }
        // }
    }

    auto& ui_entiries = ECS::registry<ShadedMeshRefUI>.entities;
    for (auto& entity : ui_entiries) {
        if (entity.has<Motion>()) {
            drawTexturedMesh(entity, projection_2D, entity.get<Motion>(), *entity.get<ShadedMeshRefUI>().reference_to_cache, true);
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
            auto back_graound_motion = motion;
            back_graound_motion.scale *= 1.1f;
            drawTexturedMesh(entity, projection_2D, back_graound_motion, PopUP::get_background(), true);
            drawTexturedMesh(entity, projection_2D, motion, texmesh, true);
        }
    }



    // First render to the custom framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
    gl_has_errors();

    // Clearing backbuffer
    glViewport(0, 0, frame_buffer_size.x, frame_buffer_size.y);
    glDepthRange(0.00001, 10);
    glClearColor(0.035f, 0.055f, 0.06f, 1.0f);
    glClearDepth(1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl_has_errors();

    glClearColor(1, 1, 1, 0);
    gl_has_errors();

    // Draw all textured meshes that have a position and size component.
    // Ground draws first; the rest are depth-sorted along the oblique floor.
    auto entities = ECS::registry<ShadedMeshRef>.entities;
    sort(entities.begin(), entities.end(), [&](const ECS::Entity e1, const ECS::Entity e2)
    {
        auto& m1 = ECS::registry<Motion>.get(e1);
        auto& m2 = ECS::registry<Motion>.get(e2);
        if (oblique_view_enabled()) {
            bool e1_ground = is_ground_entity(e1);
            bool e2_ground = is_ground_entity(e2);
            if (e1_ground != e2_ground) {
                return e1_ground;
            }
            float d1 = camera.depth_for_world_position(m1.position);
            float d2 = camera.depth_for_world_position(m2.position);
            if (std::abs(d1 - d2) > 0.001f) {
                return d1 < d2;
            }
        }
        return m1.zValue < m2.zValue;
    });

    for (ECS::Entity entity : entities)
    {
        if (!ECS::registry<Motion>.has(entity) || entity.get<ShadedMeshRef>().is_ui || should_hide_directional_child_weapon(entity))
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
    rendering_wall_blockers = true;
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
    rendering_wall_blockers = false;

    glBindFramebuffer(GL_FRAMEBUFFER, wall_surface_frame_buffer);
    glViewport(0, 0, frame_buffer_size.x, frame_buffer_size.y);
    glDepthRange(0.00001, 10);
    glClearColor(0, 0, 0, 0);
    glClearDepth(1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    rendering_wall_surface_mask = true;
    for (ECS::Entity entity : wall_entities)
    {
        if (!ECS::registry<Motion>.has(entity) || !ECS::registry<ShadedMeshRef>.has(entity))
            continue;
        drawTexturedMesh(entity, projection_2D);
        gl_has_errors();
    }
    for (ECS::Entity entity : moveable_wall_entities)
    {
        if (!ECS::registry<Motion>.has(entity) || !ECS::registry<ShadedMeshRef>.has(entity))
            continue;
        drawTexturedMesh(entity, projection_2D);
        gl_has_errors();
    }
    rendering_wall_surface_mask = false;


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
        RenderSystem::createSprite(resource, textures_path("/bullet/"+wt.texture_path+".png"), "sprite_textured");
        drawTexturedMesh(weaponTimer_entity, projection_2D, timer_mesh_motion, resource);
    } else {
        drawTexturedMesh(weaponTimer_entity, projection_2D, timer_mesh_motion, resource);
    }
}
