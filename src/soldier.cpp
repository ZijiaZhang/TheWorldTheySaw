// Header
#include "soldier.hpp"

#include <utility>
#include "render.hpp"
#include "PhysicsObject.hpp"
#include "Weapon.hpp"



ECS::Entity Soldier::createSoldier(vec2 position,
                                   COLLISION_HANDLER overlap,
                                   COLLISION_HANDLER hit)
{
	auto entity = ECS::Entity();


    std::string key = "soldier";
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0)
    {
        RenderSystem::createSprite(resource, textures_path("/soldier/soldier_basic.png"), "textured");
    }

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	ECS::registry<ShadedMeshRef>.emplace(entity, resource);

	// Setting initial motion values
	Motion& motion = ECS::registry<Motion>.emplace(entity);
	motion.position = position;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = resource.mesh.original_size * 70.f;
	motion.scale.x *= -1; // point front to the right
    motion.zValue = ZValuesMap["Soldier"];

	PhysicsObject physicsObject;
	physicsObject.mass = 100;
	physicsObject.object_type = PLAYER;
	physicsObject.vertex = {
            {
                    PhysicsVertex{{-0.4, 0.2, -0.02}},
                    PhysicsVertex{{0.3, 0.3, -0.02}},
                    PhysicsVertex{{0.3, -0.3, -0.02}},
                    PhysicsVertex{{-0.2, -0.2, -0.02}}
            }
	};
    physicsObject.faces = {{0,1}, {1,2 },{2,3 },{3,0 }};
    physicsObject.attach(Overlap, std::move(overlap));
    physicsObject.attach(Hit, std::move(hit));
    ECS::registry<PhysicsObject>.insert(entity, physicsObject);
	// Create and (empty) Soldier component to be able to refer to all turtles
	ECS::Entity weapon = Weapon::createWeapon(vec2 {0,50.f}, 0, entity);
	auto& children_entity = entity.emplace<ChildrenEntities>();
	children_entity.children.insert(weapon);
    entity.emplace<AIPath>();

	auto& soldier = ECS::registry<Soldier>.emplace(entity);
	soldier.ai_algorithm = A_STAR;
	soldier.weapon = weapon;
	soldier.teamID = 0;

	auto& health = ECS::registry<Health>.emplace(entity);
	health.hp = 3;
	health.max_hp = 3;

	// updateSoldierHealthBar(entity);

	return entity;
}

void Soldier::updateSoldierHealthBar(vec2 position, vec2 scale, float hp, float max_hp)
{
	auto entity = ECS::Entity();

	std::string key = "soldier_hp";
	ShadedMesh& resource = cache_resource(key);
	double thickness = 0.5;
	if (resource.effect.program.resource == 0) {
		// create a procedural circle
		constexpr float z = -0.02f;
		vec3 red = { 0.8,0.1,0.1 };

		// Corner points
		ColoredVertex v;
		v.position = { -thickness,-thickness,z };
		v.color = red;
		resource.mesh.vertices.push_back(v);
		v.position = { -thickness,thickness,z };
		v.color = red;
		resource.mesh.vertices.push_back(v);
		v.position = { thickness,thickness,z };
		v.color = red;
		resource.mesh.vertices.push_back(v);
		v.position = { thickness,-thickness,z };
		v.color = red;
		resource.mesh.vertices.push_back(v);

		// Two triangles
		resource.mesh.vertex_indices.push_back(0);
		resource.mesh.vertex_indices.push_back(1);
		resource.mesh.vertex_indices.push_back(3);
		resource.mesh.vertex_indices.push_back(1);
		resource.mesh.vertex_indices.push_back(2);
		resource.mesh.vertex_indices.push_back(3);

		RenderSystem::createColoredMesh(resource, "colored_mesh");
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	ECS::registry<ShadedMeshRef>.emplace(entity, resource);

	// Create motion
	auto& motion = ECS::registry<Motion>.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0, 0 };
	motion.position = { position.x + (scale.x / 2 - scale.x * hp / max_hp / 2), position.y - scale.y };
	motion.scale = { scale.x * hp / max_hp, scale.y / 10 };
	motion.zValue = ZValuesMap["Soldier"];

	// Delete old health bar visuals in main.cpp line 63
	ECS::registry<DebugComponent>.emplace(entity);

	backgroundHealthBar(position, scale, max_hp, max_hp);
}

void Soldier::backgroundHealthBar(vec2 position, vec2 scale, float hp, float max_hp)
{
	auto entity = ECS::Entity();

	std::string key = "soldier_hp_background";
	ShadedMesh& resource = cache_resource(key);
	double thickness = 0.5;
	if (resource.effect.program.resource == 0) {
		// create a procedural circle
		constexpr float z = -0.02f;
		vec3 black = { 0.1,0.1,0.1 };

		// Corner points
		ColoredVertex v;
		v.position = { -thickness,-thickness,z };
		v.color = black;
		resource.mesh.vertices.push_back(v);
		v.position = { -thickness,thickness,z };
		v.color = black;
		resource.mesh.vertices.push_back(v);
		v.position = { thickness,thickness,z };
		v.color = black;
		resource.mesh.vertices.push_back(v);
		v.position = { thickness,-thickness,z };
		v.color = black;
		resource.mesh.vertices.push_back(v);

		// Two triangles
		resource.mesh.vertex_indices.push_back(0);
		resource.mesh.vertex_indices.push_back(1);
		resource.mesh.vertex_indices.push_back(3);
		resource.mesh.vertex_indices.push_back(1);
		resource.mesh.vertex_indices.push_back(2);
		resource.mesh.vertex_indices.push_back(3);

		RenderSystem::createColoredMesh(resource, "colored_mesh");
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	ECS::registry<ShadedMeshRef>.emplace(entity, resource);

	// Create motion
	auto& motion = ECS::registry<Motion>.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0, 0 };
	motion.position = { position.x, position.y - scale.y };
	motion.scale = { scale.x, scale.y / 10 };
	motion.zValue = ZValuesMap["Turtle"];

	// Delete old health bar visuals in main.cpp line 63
	ECS::registry<DebugComponent>.emplace(entity);
}
