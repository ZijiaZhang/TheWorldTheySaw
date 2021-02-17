//
// Created by Gary on 2/4/2021.
//

#include "Enemy.hpp"
#include "render.hpp"
#include "PhysicsObject.hpp"

ECS::Entity Enemy::createEnemy(vec2 position){
    auto entity = ECS::Entity();

    std::string key = "enemy";
    ShadedMesh& resource = cache_resource(key);
    if (resource.mesh.vertices.size() == 0)
    {
        resource = ShadedMesh();
        RenderSystem::createSprite(resource, textures_path("enemy.png"), "textured");
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);

    // Setting initial motion values
    Motion& motion = ECS::registry<Motion>.emplace(entity);
    motion.position = position;
    motion.angle = 0.f;
    motion.velocity = { 0.f, 0.f };
    motion.scale = resource.mesh.original_size *50.f;
    motion.scale.x *= -1; // point front to the right
    motion.zValue = ZValuesMap["Enemy"];

    motion.max_control_speed = 70.f;
    PhysicsObject physicsObject;
    physicsObject.mass = 10;
    physicsObject.object_type = ENEMY;
    ECS::registry<PhysicsObject>.insert(entity, physicsObject);


    // Create and (empty) Salmon component to be able to refer to all turtles
    ECS::registry<Enemy>.emplace(entity);

    return entity;
}
