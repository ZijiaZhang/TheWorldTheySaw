// Header
#include "pop_up.hpp"
#include "render.hpp"


ECS::Entity PopUP::createPopUP(std::string texture_path, vec2 position, vec2 size)
{
	// Reserve en entity 
	auto entity = ECS::Entity();

	get_background();

	// Create the rendering components
	std::string key = texture_path;
	ShadedMesh& resource = cache_resource(key);
	if (resource.effect.program.resource == 0)
	{
		resource = ShadedMesh();
		RenderSystem::createSprite(resource, texture_path, "textured");
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	auto& mesh = ECS::registry<ShadedMeshRef>.emplace(entity, resource);
	mesh.is_ui = true;

	// Create and (empty) Fish component to be able to refer to all fish
	auto& pop_up = ECS::registry<PopUP>.emplace(entity);
	auto& motion = ECS::registry<Motion>.emplace(entity);
	motion.position = position;
	motion.scale = size;
	return entity;
}

ShadedMesh& PopUP::get_background()
{
	ShadedMesh& pop_up_background = cache_resource(POP_UP_BACKGROUND_KEY);
	if (pop_up_background.effect.program.resource == 0)
	{
		pop_up_background = ShadedMesh();
		RenderSystem::createSprite(pop_up_background, textures_path("/main scene/background2.png"), "textured");
	}
	return pop_up_background;
}
