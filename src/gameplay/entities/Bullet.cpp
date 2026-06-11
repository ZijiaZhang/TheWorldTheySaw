//
// Created by Gary on 2/12/2021.
//

#include "Bullet.hpp"
#include "WeaponConfig.hpp"

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
        RenderSystem::createSprite(resource, textures_path(path), "sprite_textured");
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);

    // Initialize the position, scale, and physics components
    Motion motion;

    motion.angle = angle;
    motion.velocity = velocity;
    motion.position = position;

    switch (type) {
        case W_ROCKET:
            motion.scale = { 36.f, 20.f };
            break;
        case W_LASER:
            motion.scale = { 42.f, 10.f };
            break;
        case W_AMMO:
            motion.scale = { 26.f, 18.f };
            break;
        default:
            motion.scale = { 24.f, 10.f };
            break;
    }
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
    bullet.damage = 1.f;
    WeaponConfigRegistry::initializeDefaults();
    if (const auto* config = WeaponConfigRegistry::getWeaponConfig(type)) {
        bullet.damage = config->damage;
    }

    return entity;
}


