//
// Created by Gary on 2/21/2021.
//

#include "Weapon.hpp"
#include "PhysicsObject.hpp"

#include <utility>
#include "render_components.hpp"
#include "render.hpp"

ECS::Entity Weapon::createWeapon(vec2 offset, float offset_angle, ECS::Entity parent){
    auto entity = ECS::Entity();


    std::string key = "weapon";
    ShadedMesh& resource = cache_resource(key);
    if (resource.mesh.vertices.size() == 0)
    {
        resource = ShadedMesh();
        RenderSystem::createSprite(resource, textures_path("/bullet/rocket.png"), "textured");
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);

    // Setting initial motion values
    Motion& motion = ECS::registry<Motion>.emplace(entity);
    motion.scale = resource.mesh.original_size * 20.f;
    motion.scale.x *= -3.0; // point front to the right
    motion.zValue = ZValuesMap["Soldier"];
    motion.has_parent = true;
    motion.parent = std::move(parent);
    motion.offset = offset;
    motion.offset_angle = offset_angle;

    PhysicsObject physicsObject;
    physicsObject.mass = 100;
    physicsObject.object_type = WEAPON;
    ECS::registry<PhysicsObject>.insert(entity, physicsObject);
    ECS::registry<Weapon>.emplace(entity);
    return entity;
}
