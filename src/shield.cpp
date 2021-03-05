// Header
#include "shield.hpp"
#include "render.hpp"
#include "PhysicsObject.hpp"

ECS::Entity Shield::createShield(vec2 position,  int teamID)
{
	// Reserve en entity
	auto entity = ECS::Entity();

	// Create the rendering components
	std::string key = "shield";
	ShadedMesh& resource = cache_resource(key);
	if (resource.effect.program.resource == 0)
	{
		resource = ShadedMesh();
		RenderSystem::createSprite(resource, textures_path("/shield/shield2.png"), "textured");
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	ECS::registry<ShadedMeshRef>.emplace(entity, resource);

	// Initialize the position, scale, and physics components
	auto& motion = ECS::registry<Motion>.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0, 0 };
	motion.position = position;
	// Setting initial values, scale is negative to make it face the opposite way
	motion.scale = vec2({ -0.09f, 0.09f }) * static_cast<vec2>(resource.texture.size);
    motion.zValue = ZValuesMap["Shield"];

    auto& physics = entity.emplace<PhysicsObject>();
    physics.object_type = SHIELD;

	// Create and (empty) Fish component to be able to refer to all fish
	auto& shield = ECS::registry<Shield>.emplace(entity);
    shield.teamID = teamID;
	return entity;
}
