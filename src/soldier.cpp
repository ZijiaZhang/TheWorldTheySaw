// Header
#include "soldier.hpp"
#include "render.hpp"

ECS::Entity Soldier::createSoldier(vec2 position)
{
	auto entity = ECS::Entity();


	std::string key = "soldier";
	ShadedMesh& resource = cache_resource(key);
	if (resource.mesh.vertices.size() == 0)
    {
        resource = ShadedMesh();
        RenderSystem::createSprite(resource, textures_path("/soldier/soldier.png"), "textured");
    }

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	ECS::registry<ShadedMeshRef>.emplace(entity, resource);

	// Setting initial motion values
	Motion& motion = ECS::registry<Motion>.emplace(entity);
	motion.position = position;
	motion.angle = 0.f;
	motion.velocity = { 100.f, 0.f };
	motion.scale = resource.mesh.original_size * 150.f;
	motion.scale.x *= -1; // point front to the right
    motion.zValue = ZValuesMap["Soldier"];

	PhysicsObject physicsObject;
	physicsObject.mass = 100;
	physicsObject.object_type = PLAYER;
    ECS::registry<PhysicsObject>.insert(entity, physicsObject);
	// Create and (empty) Soldier component to be able to refer to all turtles
	ECS::registry<Soldier>.emplace(entity);
	return entity;
}
