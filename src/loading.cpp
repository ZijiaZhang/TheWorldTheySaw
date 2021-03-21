// Header
#include "loading.hpp"
#include "render.hpp"

ECS::Entity Loading::createLoading(vec2 position)
{
    // Reserve en entity
    auto entity = ECS::Entity();

    // Create the rendering components
    std::string key = "loading";
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0)
    {
        resource = ShadedMesh();
        RenderSystem::createSprite(resource, textures_path("/main scene/weapon_selection.png"), "textured");
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);

    // Initialize the position, scale, and physics components
    auto& motion = ECS::registry<Motion>.emplace(entity);
    motion.angle = 0.f;
    motion.velocity = { 0.f, 0 };
    motion.position = position;
    // Setting initial values, scale is negative to make it face the opposite way
    motion.scale = vec2({ 0.6f, 0.6f }) * static_cast<vec2>(resource.texture.size);
    motion.zValue = ZValuesMap["Loading"];

    // Create and (empty) Fish component to be able to refer to all fish
    ECS::registry<Loading>.emplace(entity);

    return entity;
}
