//
// Created by Gary on 2/12/2021.
//

#include "Bullet.hpp"
#include "render.hpp"
#include "PhysicsObject.hpp"

ECS::Entity Bullet::createBullet(vec2 position, float angle)
{
    // Reserve en entity
    auto entity = ECS::Entity();
    // Create the rendering components

    auto overlap = [=](const ECS::Entity &e) mutable {
        entity.emplace<DeathTimer>();
    };
    entity.attach(Overlap, overlap);

    std::string key = "bullet";
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0)
    {
        resource = ShadedMesh();
        RenderSystem::createSprite(resource, textures_path("/bullet/laser.png"), "textured");
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);

    // Initialize the position, scale, and physics components
    Motion motion;

    motion.angle = angle;
    motion.velocity = { 380.f, 0 };
    motion.position = position;

    // Setting initial values, scale is negative to make it face the opposite way
    motion.scale = vec2({ 0.1f, 0.1f }) * static_cast<vec2>(resource.texture.size);
    motion.zValue = ZValuesMap["Fish"];
    printf("%d\n", ECS::registry<Motion>.entities.size());
    ECS::registry<Motion>.emplace(entity, motion);

    auto& physics = ECS::registry<PhysicsObject>.emplace(entity);
    physics.object_type = BULLET;
    // Create and (empty) Fish component to be able to refer to all fish
    ECS::registry<Bullet>.emplace(entity);

    return entity;
}
