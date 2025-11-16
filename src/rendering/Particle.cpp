//
// Created by Gary on 2/4/2021.
//

#include "Particle.hpp"
#include <render.hpp>

ECS::Entity Particle::createParticle(vec2 position, vec2 size, float lifetime){
    auto entity = ECS::Entity();

    // Setting initial motion values
    Motion& motion = ECS::registry<Motion>.emplace(entity);
    motion.position = position;
    motion.scale = size;
    motion.zValue = ZValuesMap["Enemy"];

    auto& deathTimer = entity.emplace<DeathTimer>();
    deathTimer.counter_ms = lifetime;

    auto& particle = ECS::registry<Particle>.emplace(entity);
    particle.start_time = glfwGetTime();
    glGenBuffers(1, particle.motion_buffer.data());
    glGenBuffers(1, particle.speed_buffer.data());
    glGenBuffers(1, particle.scale_speed_buffer.data());
    particle.mesh.mesh.vertices.emplace_back(ColoredVertex{ vec3 {-0.5, 0.5, -0.02}, vec3{0.0,1.0,0.0} });
    particle.mesh.mesh.vertices.emplace_back(ColoredVertex{ vec3{0.5, 0.5, -0.02}, vec3{0.0,1.0,0.0} });
    particle.mesh.mesh.vertices.emplace_back(ColoredVertex{ vec3{0.5, -0.5, -0.02}, vec3{0.0,1.0,0.0} });
    particle.mesh.mesh.vertices.emplace_back(ColoredVertex{ vec3{-0.5, -0.5, -0.02}, vec3{0.0,1.0,0.0} });

    particle.mesh.mesh.vertex_indices = std::vector<uint16_t>({ 0, 2, 1, 0, 3, 2 });

    // RenderSystem::createColoredMesh(particle.mesh, "particle");
    //RenderSystem::createSpriteAnimation(particle.mesh, textures_path("/enemy/cannon/alien.png"), 4);
    RenderSystem::createSprite(particle.mesh, textures_path("/explosion/blood5.png"), "particle");

    for (int x = 0; x < 10; x++) {
        Motion m;
        m.position = motion.position;
        //m.angle = angle;
        m.scale = motion.scale;
        particle.motions.push_back(m);
        Transform transform;
        transform.translate(m.position);
        //  transform.rotate(m.angle);
        transform.scale(m.scale); 
        particle.t.emplace_back(transform.mat);
        particle.scale_speed.emplace_back(Particle::uniform_dist(Particle::rng) * 0.5f);
        particle.speed.emplace_back(vec2{ Particle::uniform_dist(Particle::rng) - 0.5, Particle::uniform_dist(Particle::rng) - 0.5} * 100.f );
    }
    glBindBuffer(GL_ARRAY_BUFFER, particle.motion_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mat3) * particle.t.size(), particle.t.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, particle.speed_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * particle.speed.size(), particle.speed.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, particle.scale_speed_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * particle.scale_speed.size(), particle.scale_speed.data(), GL_STATIC_DRAW);
    return entity; 
}

std::default_random_engine Particle::rng = std::default_random_engine(std::random_device()());
std::uniform_real_distribution<float> Particle::uniform_dist{};
