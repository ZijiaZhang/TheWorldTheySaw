//
// Created by Gary on 2/4/2021.
//

#include "Enemy.hpp"

#include <utility>
#include "render.hpp"
#include "PhysicsObject.hpp"
#include "Bullet.hpp"

ECS::Entity Enemy::createEnemy(vec2 position,
                               std::function<void(ECS::Entity&, const  ECS::Entity&)> overlap,
                               std::function<void(ECS::Entity&, const  ECS::Entity&)> hit, int teamID){
    auto entity = ECS::Entity();
    entity.attach(Overlap, overlap);
    entity.attach(Hit, std::move(hit));

    std::string key = "enemy";
    ShadedMesh& resource = cache_resource(key);
    if (resource.mesh.vertices.size() == 0)
    {
        resource = ShadedMesh();
        RenderSystem::createSprite(resource, textures_path("/enemy/cannon/asuka_ani.png"), "animation");
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);

    // Setting initial motion values
    Motion& motion = ECS::registry<Motion>.emplace(entity);
    motion.position = position;
    motion.angle = 0.f;
    motion.velocity = { 0.f, 0.f };
    motion.scale = resource.mesh.original_size * 50.f;
    motion.scale.x *= -1; // point front to the right
    motion.zValue = ZValuesMap["Enemy"];

    motion.max_control_speed = 70.f;
    PhysicsObject physicsObject;
    physicsObject.mass = 30;
    physicsObject.object_type = ENEMY;
    physicsObject.vertex = {
            {
                    PhysicsVertex{{0.1, 0.2, -0.02}},
                    PhysicsVertex{{0.3, 0.2, -0.02}},
                    PhysicsVertex{{0.4, 0, -0.02}},
                    PhysicsVertex{{0.3, -0.2, -0.02}},
                    PhysicsVertex{{0.1, -0.2, -0.02}},
                    PhysicsVertex{{-0.2, 0, -0.02}},
            }
    };
    physicsObject.faces = {{0,1}, {1,2 },{2,3 },{3,4 }, {4,5 }, {5,0}};
    ECS::registry<PhysicsObject>.insert(entity, physicsObject);

    entity.emplace<AIPath>();
    // Create and (empty) Salmon component to be able to refer to all turtles
    auto& e =ECS::registry<Enemy>.emplace(entity);
    e.teamID = teamID;
    return entity;
}
