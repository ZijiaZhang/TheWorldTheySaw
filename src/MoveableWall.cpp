//
// Created by Gary on 1/19/2021.
//

#include "MoveableWall.hpp"
#include "render.hpp"
#include "PhysicsObject.hpp"

ECS::Entity MoveableWall::createMoveableWall(vec2 location, vec2 size, float rotation,
                                             COLLISION_HANDLER overlap,
                                             COLLISION_HANDLER hit){
    auto entity = ECS::Entity();

    std::string key = "wall";
    ShadedMesh& resource = cache_resource(key);
    if (resource.mesh.vertices.empty())
    {
        resource = ShadedMesh();
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3 {-0.5, 0.5, -0.02}, vec3{0.0,0.0,0.0}});
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3{0.5, 0.5, -0.02}, vec3{0.0,0.0,0.0}});
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3{0.5, -0.5, -0.02}, vec3{0.0,0.0,0.0}});
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3{-0.5, -0.5, -0.02}, vec3{0.0,0.0,0.0}});

        resource.mesh.vertex_indices = std::vector<uint16_t>({0, 3, 1, 1, 3, 2});

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
    physicsObject.object_type = MOVEABLEWALL;
    physicsObject.fixed = false;
    physicsObject.mass = 30;

    ECS::registry<PhysicsObject>.insert(entity, physicsObject);
    //motion.box = size;
    // Create and (empty) Salmon component to be able to refer to all turtles
    ECS::registry<MoveableWall>.emplace(entity);
    resource.texture.color = {1,1,1};
    return entity;
}
