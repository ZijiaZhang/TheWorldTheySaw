// Header
#include "pop_up.hpp"
#include "render.hpp"


ECS::Entity PopUP::createPopUP(std::string texture_path, vec2 position, vec2 size)
{
	// Reserve en entity
	auto entity = ECS::Entity();

	// Create the rendering components
	std::string key = texture_path;
	ShadedMesh& resource = cache_resource(key);
	if (resource.effect.program.resource == 0)
	{
		resource = ShadedMesh();
		RenderSystem::createSprite(resource, texture_path, "sprite_textured");
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	auto& mesh = ECS::registry<ShadedMeshRef>.emplace(entity, resource);
	mesh.is_ui = true;

	auto& pop_up = ECS::registry<PopUP>.emplace(entity);
	(void) pop_up;
	auto& motion = ECS::registry<Motion>.emplace(entity);
	motion.position = position;
	motion.scale = size;
	return entity;
}
