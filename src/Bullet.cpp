//
// Created by Gary on 2/12/2021.
//

#include "Bullet.hpp"
#include "PhysicsObject.hpp"

ECS::Entity Bullet::createBullet(vec2 position, float angle, vec2 velocity, int teamID,  std::string name)
{
    // Reserve en entity
    auto entity = ECS::Entity();
    // Create the rendering components

    entity.attach(Hit, destroy_on_hit);
    entity.attach(Overlap, destroy_on_hit);

    std::string key = "bullet_" + name;
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0)
    {
        resource = ShadedMesh();
        std::string path = "/bullet/";
        path.append(name);
        path.append(".png");
        RenderSystem::createSprite(resource, textures_path(path), "textured");
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);

    // Initialize the position, scale, and physics components
    Motion motion;

    motion.angle = angle;
    motion.velocity = velocity;
    motion.position = position;

    // Setting initial values, scale is negative to make it face the opposite way
    motion.scale = vec2({ 0.1f, 0.1f }) * static_cast<vec2>(resource.texture.size);
    motion.zValue = ZValuesMap["Fish"];
    // printf("%lu\n", ECS::registry<Motion>.entities.size());
    ECS::registry<Motion>.emplace(entity, motion);

    auto& physics = ECS::registry<PhysicsObject>.emplace(entity);
    physics.vertex = {
            {
                    PhysicsVertex{{-0.25, 0.05, -0.02}},
                    PhysicsVertex{{0.25, 0.05, -0.02}},
                    PhysicsVertex{{0.25, -0.05, -0.02}},
                    PhysicsVertex{{-0.25, -0.05, -0.02}}
            }
    };
    physics.faces = {{0,1}, {1,2 },{2,3 },{3,0 }};
    physics.object_type = BULLET;
    // Create and (empty) Fish component to be able to refer to all fish
    auto& bullet = ECS::registry<Bullet>.emplace(entity);
    bullet.teamID = teamID;
    return entity;
}
