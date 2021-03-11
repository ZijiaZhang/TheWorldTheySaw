//
// Created by Gary on 1/19/2021.
//

#include "Wall.hpp"

#include <utility>
#include "render.hpp"
#include "PhysicsObject.hpp"

ECS::Entity Wall::createWall(vec2 location, vec2 size, float rotation,
                             COLLISION_HANDLER overlap,
                             COLLISION_HANDLER hit){
    auto entity = ECS::Entity();

    std::string key = "wall";
    ShadedMesh& resource = cache_resource(key);
    if (resource.mesh.vertices.empty())
    {
        resource = ShadedMesh();
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3 {-0.5, 0.5, -0.02}, vec3{0.0,1.0,0.0}});
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3{0.5, 0.5, -0.02}, vec3{0.0,1.0,0.0}});
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3{0.5, -0.5, -0.02}, vec3{0.0,1.0,0.0}});
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3{-0.5, -0.5, -0.02}, vec3{0.0,1.0,0.0}});

        resource.mesh.vertex_indices = std::vector<uint16_t>({0, 2, 1, 0, 3, 2});

        RenderSystem::createColoredMesh(resource, "salmon");
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);

    // Setting initial motion values
    Motion& motion = ECS::registry<Motion>.emplace(entity);
    motion.position = location;
    motion.angle = rotation;
    motion.velocity = { 0.f, 0.f };
    motion.scale = size;
    motion.zValue = ZValuesMap["Wall"];

    PhysicsObject physicsObject;
    physicsObject.object_type = WALL;
    physicsObject.fixed = true;
    physicsObject.attach(Overlap, overlap);
    physicsObject.attach(Hit, hit);
    ECS::registry<PhysicsObject>.insert(entity, physicsObject);

    //motion.box = size;
    //motion.mass = 1000;
    // Create and (empty) Salmon component to be able to refer to all turtles
    ECS::registry<Wall>.emplace(entity);
    resource.texture.color = {1,1,1};
    return entity;
}
