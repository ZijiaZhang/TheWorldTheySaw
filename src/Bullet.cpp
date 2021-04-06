//
// Created by Gary on 2/12/2021.
//

#include "Bullet.hpp"


ECS::Entity Bullet::createBullet(vec2 position, float angle, vec2 velocity, int teamID,  std::string texture_name, float lifetime,
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
    physics.attach(Hit, destroy_on_hit);
    physics.attach(Overlap, destroy_on_hit);
    return entity;
}
