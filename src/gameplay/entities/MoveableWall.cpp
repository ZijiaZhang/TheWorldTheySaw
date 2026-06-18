//
// Created by Gary on 1/19/2021.
//

#include "MoveableWall.hpp"

#include "render.hpp"
#include "PhysicsObject.hpp"
#include "Wall.hpp"

ECS::Entity MoveableWall::createMoveableWall(vec2 location, vec2 size, float rotation,
                                             COLLISION_HANDLER overlap,
                                             COLLISION_HANDLER hit){
    auto entity = ECS::Entity();

    std::string key = "wall";
    ShadedMesh& resource = cache_resource(key);
    if (resource.mesh.vertices.empty())
    {
        resource = ShadedMesh();
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3 {-0.5, 0.5, -0.02}, vec3{0.72,0.86,0.92}});
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3{0.5, 0.5, -0.02}, vec3{0.58,0.72,0.78}});
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3{0.5, -0.5, -0.02}, vec3{0.32,0.43,0.48}});
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3{-0.5, -0.5, -0.02}, vec3{0.46,0.59,0.64}});

        resource.mesh.vertex_indices = std::vector<uint16_t>({0, 2, 1, 0, 3, 2});

        RenderSystem::createColoredMesh(resource, "mesh_flat_color");
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
    physicsObject.attach(Overlap,overlap);
    physicsObject.attach(Hit, hit);
    ECS::registry<PhysicsObject>.insert(entity, physicsObject);

    ECS::registry<MoveableWall>.emplace(entity);
    resource.texture.color = {0.55f,0.68f,0.72f};
    return entity;
}

ECS::Entity MoveableWall::createMoveableWall(Motion m, MoveableWall mw, PhysicsObject po)
{
    auto e = ECS::Entity();

    std::string key = "wall";
    ShadedMesh& resource = cache_resource(key);
    if (resource.mesh.vertices.empty())
    {
        resource = ShadedMesh();
        resource.mesh.vertices.emplace_back(ColoredVertex{ vec3 {-0.5, 0.5, -0.02}, vec3{0.72,0.86,0.92} });
        resource.mesh.vertices.emplace_back(ColoredVertex{ vec3{0.5, 0.5, -0.02}, vec3{0.58,0.72,0.78} });
        resource.mesh.vertices.emplace_back(ColoredVertex{ vec3{0.5, -0.5, -0.02}, vec3{0.32,0.43,0.48} });
        resource.mesh.vertices.emplace_back(ColoredVertex{ vec3{-0.5, -0.5, -0.02}, vec3{0.46,0.59,0.64} });

        resource.mesh.vertex_indices = std::vector<uint16_t>({ 0, 2, 1, 0, 3, 2 });

        RenderSystem::createColoredMesh(resource, "mesh_flat_color");
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(e, resource);
    resource.texture.color = { 0.55f,0.68f,0.72f };

    e.emplace<Motion>(m);
    e.emplace<MoveableWall>(mw);
    e.emplace<PhysicsObject>(po);
    return e;
}

void MoveableWall::wall_hit(ECS::Entity self, ECS::Entity e, CollisionResult collision) {
    PhysicsObject::handle_collision(self, e, collision);
}
