// Header
#include "highlight_circle.hpp"
#include "render.hpp"

ECS::Entity HighLightCircle::createHighLightCircle(vec2 position, float radius, float thickness)
{
	// Reserve en entity 
	auto entity = ECS::Entity();

	// Create the rendering components
	std::string key = "highlight_circle";
	ShadedMesh& resource = cache_resource(key);
	if (resource.effect.program.resource == 0)
	{
		resource = ShadedMesh();
		RenderSystem::createSprite(resource, "", "highlight_circle");
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	auto& mesh = ECS::registry<ShadedMeshRef>.emplace(entity, resource);
	mesh.is_ui = true;

	// Create and (empty) Fish component to be able to refer to all fish
	auto& highlight = ECS::registry<HighLightCircle>.emplace(entity);
	auto& motion = entity.emplace<Motion>();
	motion.position = position;
	motion.scale = vec2{ radius + thickness, radius + thickness} * 2.f;

	highlight.radius = radius;
	highlight.thickness = thickness;

	return entity;
}
