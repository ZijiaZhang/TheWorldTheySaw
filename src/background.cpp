// Header
#include "background.hpp"
#include "render.hpp"

ECS::Entity Background::createBackground(vec2 position, std::string name, float depth)
{
	// Reserve en entity
	auto entity = ECS::Entity();

	// Create the rendering components
	std::string key = name;
	ShadedMesh& resource = cache_resource(key);
	if (resource.effect.program.resource == 0)
	{
		resource = ShadedMesh();
        std::string path = "/main scene/";
        path.append(name);
        path.append(".png");
		RenderSystem::createSprite(resource, textures_path(path), "textured");
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	ECS::registry<ShadedMeshRef>.emplace(entity, resource);

	// Initialize the position, scale, and physics components
	auto& motion = ECS::registry<Motion>.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0 };
	motion.position = position;
	// Setting initial values, scale is negative to make it face the opposite way
	motion.scale = vec2({ 1.5f, 1.5f }) * static_cast<vec2>(resource.texture.size);
    motion.zValue = ZValuesMap["Background"];
    motion.depth = depth;

	// Create and (empty) Fish component to be able to refer to all fish
	ECS::registry<Background>.emplace(entity);

	return entity;
}
