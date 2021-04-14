//
// Created by Gary on 2/21/2021.
//

#include "Weapon.hpp"
#include "PhysicsObject.hpp"

#include <utility>
#include "render_components.hpp"
#include "render.hpp"
std::unordered_map<WeaponType, std::string> Weapon::weaponTexturePath {
        {W_AMMO, "/soldier/weapon_frozen.png"},
        {W_LASER, "/soldier/sword.png"},
        {W_BULLET, "/soldier/weapon_heavy.png"},
        {W_ROCKET, "/soldier/weapon_rocket.png"}
};
ECS::Entity Weapon::createWeapon(vec2 offset, float offset_angle, ECS::Entity parent, std::string texture_path){
    auto entity = ECS::Entity();


    std::string key = "weapon";
    ShadedMesh& resource = cache_resource(key);
    if (resource.mesh.vertices.size() == 0)
    {
        resource = ShadedMesh();
        RenderSystem::createSprite(resource, textures_path(texture_path), "textured");
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);

    // Setting initial motion values
    Motion& motion = ECS::registry<Motion>.emplace(entity);
    motion.scale = resource.mesh.original_size * 20.f;
    motion.scale.x *= -3.0; // point front to the right
    motion.zValue = ZValuesMap["Weapon"];
    motion.has_parent = true;
    motion.parent = parent;
    motion.offset = offset;
    motion.offset_angle = offset_angle;

    auto& parent_entity = entity.emplace<ParentEntity>();
    parent_entity.parent = parent;
    PhysicsObject physicsObject;
    physicsObject.mass = 100;
    physicsObject.object_type = WEAPON;
    ECS::registry<PhysicsObject>.insert(entity, physicsObject);
    ECS::registry<Weapon>.emplace(entity);
    return entity;
}
