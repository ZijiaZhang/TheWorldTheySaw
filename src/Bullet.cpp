//
// Created by Gary on 2/12/2021.
//

#include "Bullet.hpp"
#include "render.hpp"

ECS::Entity Bullet::createBullet(vec2 position)
{
    // Reserve en entity
    auto entity = ECS::Entity();

    // Create the rendering components
    std::string key = "bullet";
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0)
    {
        resource = ShadedMesh();
        RenderSystem::createSprite(resource, textures_path("ARS.png"), "animation");
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);

    // Initialize the position, scale, and physics components
    auto& motion = ECS::registry<Motion>.emplace(entity);
    motion.angle = 0.f;
    motion.velocity = { 380.f, 0 };
    motion.position = position;
    // Setting initial values, scale is negative to make it face the opposite way
    motion.scale = vec2({ -0.4f, 0.4f }) * static_cast<vec2>(resource.texture.size);
    motion.zValue = ZValuesMap["Fish"];

    // Create and (empty) Fish component to be able to refer to all fish
    ECS::registry<Bullet>.emplace(entity);

    return entity;
}
