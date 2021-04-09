//
// Created by Gary on 2/4/2021.
//

#include "Particle.hpp"
#include <render.hpp>

ECS::Entity Particle::createParticle(vec2 position, vec2 size, float angle){
    auto entity = ECS::Entity();

    // Setting initial motion values
    Motion& motion = ECS::registry<Motion>.emplace(entity);
    motion.position = position;
    motion.angle = angle;
    motion.scale = size;
    motion.zValue = ZValuesMap["Enemy"];

   

    auto& particle = ECS::registry<Particle>.emplace(entity);
    glGenBuffers(1, particle.motion_buffer.data());

    particle.mesh.mesh.vertices.emplace_back(ColoredVertex{ vec3 {-0.5, 0.5, -0.02}, vec3{0.0,1.0,0.0} });
    particle.mesh.mesh.vertices.emplace_back(ColoredVertex{ vec3{0.5, 0.5, -0.02}, vec3{0.0,1.0,0.0} });
    particle.mesh.mesh.vertices.emplace_back(ColoredVertex{ vec3{0.5, -0.5, -0.02}, vec3{0.0,1.0,0.0} });
    particle.mesh.mesh.vertices.emplace_back(ColoredVertex{ vec3{-0.5, -0.5, -0.02}, vec3{0.0,1.0,0.0} });

    particle.mesh.mesh.vertex_indices = std::vector<uint16_t>({ 0, 2, 1, 0, 3, 2 });

    // RenderSystem::createColoredMesh(particle.mesh, "particle");
    //RenderSystem::createSpriteAnimation(particle.mesh, textures_path("/enemy/cannon/alien.png"), 4);
    RenderSystem::createSprite(particle.mesh, textures_path("/enemy/cannon/alien.png"), "particle");

    for (int x = 0; x < 10; x++) {
        Motion m;
        m.position = vec2{ 100 * x, 0 };
        m.scale = vec2{ 100, 100 };
        particle.motions.push_back(m);
    }
    return entity;
}