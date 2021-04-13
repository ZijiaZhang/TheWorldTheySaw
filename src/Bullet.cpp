//
// Created by Gary on 2/12/2021.
//

#include "Bullet.hpp"
std::unordered_map<WeaponType, float> Bullet::bulletDamage {
        {W_AMMO, 0.5},
        {W_LASER, 0.3},
        {W_BULLET, 1.0},
        {W_ROCKET, 5.0}
};

std::unordered_map<WeaponType , std::function<void(ECS::Entity, ECS::Entity, float)>> Bullet::bulletEffect = {
        {W_ROCKET, heal_soldier},
        {W_AMMO, freeze_enemy},
};

ECS::Entity Bullet::createBullet(vec2 position, float angle, vec2 velocity, int teamID, WeaponType type, std::string texture_name, float lifetime,
                                 std::function<void(ECS::Entity)> callback){
    // Reserve en entity
    auto entity = ECS::Entity();
    // Create the rendering components

    std::string key = "bullet_" + texture_name;
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0)
    {
        resource = ShadedMesh();
        std::string path = "/bullet/";
        path.append(texture_name);
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

    if(lifetime > 0){
        auto& explode_timer = entity.emplace<ExplodeTimer>();
        explode_timer.counter_ms = lifetime;
        explode_timer.callback = callback;
    }
    auto& bullet = ECS::registry<Bullet>.emplace(entity);
    bullet.teamID = teamID;
    bullet.bullet_indicator = texture_name;
    bullet.on_destroy = callback;

    switch (type) {
        case W_LASER:
            bullet.penetration_counter = 200;
            physics.attach(Hit, [](ECS::Entity self, const ECS::Entity e, CollisionResult) {return; });
            physics.attach(Overlap, lazer_penetrate);
            break;
        default:
            physics.attach(Hit, destroy_on_hit);
            physics.attach(Overlap, destroy_on_hit);
            break;
    }

    bullet.type = type;
    bullet.damage = Bullet::bulletDamage[type];

    return entity;
}


void Bullet::heal_soldier(ECS::Entity soldier_entity, ECS::Entity enemy_entity, float elapsed_ms) {
    auto entities = ECS::registry<Soldier>.entities;
    for (auto e : entities) {
        ECS::registry<Soldier>.get(e).addHealth(e, 1);
    }
}

void Bullet::freeze_enemy(ECS::Entity soldier_entity, ECS::Entity enemy_entity, float elapsed_ms) {
    if (!ECS::registry<FrozenTimer>.has(enemy_entity)) {
        ECS::registry<FrozenTimer>.emplace(enemy_entity);
        Enemy::set_frozen_shader(enemy_entity);
    }
}
