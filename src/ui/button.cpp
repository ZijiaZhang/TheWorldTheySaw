// Header
#include "button.hpp"
#include "render.hpp"
#include "PhysicsObject.hpp"

ECS::Entity Button::createButton(vec2 position, vec2 size, const std::string& texture_path, COLLISION_HANDLER on_click)
{
	// Reserve en entity
	auto entity = ECS::Entity();

	// Create the rendering components
	std::string key = "button_" + texture_path;
	ShadedMesh& resource = cache_resource(key);
	if (resource.effect.program.resource == 0)
	{
		resource = ShadedMesh();
		RenderSystem::createSprite(resource, texture_path.empty() ? "" : textures_path(texture_path), "sprite_textured");
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	ECS::registry<ShadedMeshRef>.emplace(entity, resource);

	// Initialize the position, scale, and physics components
	auto& motion = ECS::registry<Motion>.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.position = position;
	motion.scale = size;
	motion.zValue = ZValuesMap["Button"];

	auto& physics = entity.emplace<PhysicsObject>();
	physics.object_type = BUTTON;
	physics.attach(Overlap, on_click);

	ECS::registry<Button>.emplace(entity);
	return entity;
}
