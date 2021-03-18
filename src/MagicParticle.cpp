//
// Created by Gary on 3/18/2021.
//

#include "MagicParticle.hpp"
ECS::Entity MagicParticle::createMagicParticle(vec2 position,
                                float angle,
                                vec2 velocity,
                                int teamID,
                                MagicWeapon weapon){

    // Reserve en entity
    auto entity = ECS::Entity();
    // Create the rendering components

    std::string key = "MagicParticle_" + magic_texture_map[weapon];
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0)
    {
        resource = ShadedMesh();
        std::string path = "/bullet/";
        path.append(magic_texture_map[weapon]);
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
    motion.zValue = ZValuesMap["MagicParticle"];
    // printf("%lu\n", ECS::registry<Motion>.entities.size());
    ECS::registry<Motion>.emplace(entity, motion);

    auto& physics = ECS::registry<PhysicsObject>.emplace(entity);
    physics.vertex = {
            {
                    PhysicsVertex{{-0.5, 0.5, -0.02}},
                    PhysicsVertex{{0.5, 0.5, -0.02}},
                    PhysicsVertex{{0.5, -0.5, -0.02}},
                    PhysicsVertex{{-0.5, -0.5, -0.02}}
            }
    };
    physics.faces = {{0,1}, {1,2 },{2,3 },{3,0 }};
    physics.object_type = MAGIC;
    physics.attach(Hit, magic_hit_map[weapon]);
    physics.attach(Overlap, magic_overlap_map[weapon]);

    auto& particle = ECS::registry<MagicParticle>.emplace(entity);
    particle.teamID = teamID;
    particle.type = magic_type_map[weapon];
    particle.damage = magic_damage_map[weapon];
    return entity;
}

std::unordered_map<MagicWeapon , std::string> MagicParticle::magic_texture_map = {
        {FIREBALL, "bullet_advanced"}
};

std::unordered_map<MagicWeapon , float> MagicParticle::magic_damage_map = {
        {FIREBALL, 10.f}
};

std::unordered_map<MagicWeapon , DamageType> MagicParticle::magic_type_map = {
        {FIREBALL, FIRE}
};

std::unordered_map<MagicWeapon , COLLISION_HANDLER> MagicParticle::magic_overlap_map = {
        {FIREBALL, MagicParticle::destroy_on_hit}
};

std::unordered_map<MagicWeapon , COLLISION_HANDLER> MagicParticle::magic_hit_map = {
        {FIREBALL, MagicParticle::destroy_on_hit}
};
