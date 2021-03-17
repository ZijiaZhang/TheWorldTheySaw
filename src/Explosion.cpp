//
// Created by Gary on 3/16/2021.
//

#include "Explosion.hpp"
#include "render_components.hpp"
#include "render.hpp"

ECS::Entity Explosion::CreateExplosion(vec2 location, vec2 radius) {
    auto entity = ECS::Entity();

    std::string key = "explosion";
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0)
    {
        resource = ShadedMesh();
        std::string path = "/bullet/";
        path.append("bullet_asuka_explosion");
        path.append(".png");
        RenderSystem::createSprite(resource, textures_path(path), "textured");
    }
    return  entity;
}
