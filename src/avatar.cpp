// Header
#include "avatar.hpp"
#include "render.hpp"
#include "soldier.hpp"
#include "Bullet.hpp"

std::map<AvatarType, std::string> Avatar::avatarNamesMap = {
    {AvatarType::AVATAR, "normal"},
    {AvatarType::AVATAR_HIT, "hit"}
};

ECS::Entity Avatar::createAvatar(vec2 position,  AvatarType avatarType)
{
	// Reserve en entity
	auto entity = ECS::Entity();
    
    std::string key = avatarNamesMap[avatarType];
    ShadedMesh& resource = cache_resource(key);

    if (resource.effect.program.resource == 0)
    {
        std::string path = "/avatar/avatar_";
        path.append(key);
        path.append(".png");
        resource = ShadedMesh();
        RenderSystem::createSprite(resource, textures_path(path), "textured");
    }
	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    auto ref = ECS::registry<ShadedMeshRefUI>.emplace(entity, resource);
    // Initialize the position, scale, and physics components
    auto& motion = ECS::registry<Motion>.emplace(entity);
    motion.angle = 0.f;
    motion.velocity = { 0.f, 0.f };
    motion.position = position;

    motion.scale = vec2({ 1.0f, 1.0f }) * static_cast<vec2>(resource.texture.size);

    // Update ZValuesMap in common
    motion.zValue = ZValuesMap["Fish"];

    // Create and (empty) Fish component to be able to refer to all fish
    auto& avatar = ECS::registry<Avatar>.emplace(entity);


    avatar.avatarType = avatarType;
    return entity;
}
