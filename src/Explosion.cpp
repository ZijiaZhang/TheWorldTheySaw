//
// Created by Gary on 3/16/2021.
//

#include "Explosion.hpp"
#include "render_components.hpp"
#include "render.hpp"
#include "PhysicsObject.hpp"

ECS::Entity Explosion::CreateExplosion(vec2 location, float radius, int teamID, float damage) {
    auto entity = ECS::Entity();

    std::string key = "explosion";
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0)
    {
        resource = ShadedMesh();
        std::string path = "/bullet/";
        path.append("smoke");
        path.append(".png");
        RenderSystem::createSpriteAnimation(resource, textures_path(path), 16);
    }
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);


    auto& motion = entity.emplace<Motion>();
    motion.position = location;
    motion.scale = {radius, radius};
    motion.zValue = ZValuesMap["Soldier"];

    PhysicsObject&  physicsObject = ECS::registry<PhysicsObject>.emplace(entity);
    physicsObject.object_type = EXPLOSION;
    physicsObject.vertex.clear();
    physicsObject.faces.clear();

    const int segment = 10;
    physicsObject.vertex.emplace_back(PhysicsVertex{vec3{sin(0 * 2 * 3.14/(float)segment),
                                                         cos(0 * 2 * 3.14/(float)segment),
                                                         -0.02}});
    for(int index = 1; index < segment; index++){
        physicsObject.vertex.emplace_back(PhysicsVertex{vec3{sin(index * 2 * 3.14/(float)segment),
                                                             cos(index * 2 * 3.14/(float)segment),
                                                             -0.02}});
        physicsObject.faces.emplace_back(std::pair<int,int>{index - 1, index});

    }
    physicsObject.faces.emplace_back(std::pair<int,int>{segment - 1, 0});

    physicsObject.attach(Overlap, Explosion::destroy_on_hit);
    physicsObject.attach(Hit, Explosion::destroy_on_hit);

    auto& death = entity.emplace<DeathTimer>();
    death.counter_ms = 1000;

    auto& explosion = entity.emplace<Explosion>();
    explosion.teamID = teamID;
    explosion.damage = damage;
    return  entity;
}
