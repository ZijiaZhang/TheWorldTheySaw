#include "health_bar.hpp"
#include "render.hpp"

void Healthbar::updateHealthBar(ECS::Entity e, bool draw)
{
	if (e.has<Health>() && e.has<Motion>()) {
		auto& motion = e.get<Motion>();
		auto& health = e.get<Health>();

		if (draw) {
			if (health.hb_available) {
				updateHealthBarPosition(motion.position, motion.scale, health.hp, health.max_hp, health.healthbar);
				updateHealthBarPosition(motion.position, motion.scale, health.max_hp, health.max_hp, health.healthbarbg);
			}
			else if (!health.hb_available) {
				health.healthbar = drawHealthBar(motion.position, motion.scale, health.hp, health.max_hp, {0.8, 0.1, 0.1}, "Soldier");
				health.healthbarbg = drawHealthBar(motion.position, motion.scale, health.max_hp, health.max_hp, { 0.1, 0.1, 0.1 }, "Turtle");
				health.hb_available = true;
			}
		}
		else {
			if (health.hb_available) {
				ECS::registry<DeathTimer>.emplace(health.healthbar);
				ECS::registry<DeathTimer>.emplace(health.healthbarbg);
				health.hb_available = false;
			}
		}
		
	}
}

ECS::Entity Healthbar::drawHealthBar(vec2 position, vec2 scale, float hp, float max_hp, vec3 color, std::string layer)
{
	auto entity = ECS::Entity();

	std::string key = "hp_" + layer;
	ShadedMesh& resource = cache_resource(key);
	double thickness = 0.5;
	if (resource.effect.program.resource == 0) {
		// create a procedural circle
		constexpr float z = -0.02f;

		// Corner points
		ColoredVertex v;
		v.position = { -thickness,-thickness,z };
		v.color = color;
		resource.mesh.vertices.push_back(v);
		v.position = { -thickness,thickness,z };
		v.color = color;
		resource.mesh.vertices.push_back(v);
		v.position = { thickness,thickness,z };
		v.color = color;
		resource.mesh.vertices.push_back(v);
		v.position = { thickness,-thickness,z };
		v.color = color;
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
	motion.zValue = ZValuesMap[layer];

	updateHealthBarPosition(position, scale, hp, max_hp, entity);

	return entity;
}

void Healthbar::updateHealthBarPosition(vec2 position, vec2 scale, float hp, float max_hp, ECS::Entity hb)
{
	if (hb.has<Motion>()) {
		auto& motion = hb.get<Motion>();
		motion.position = { position.x + (scale.x / 2 - scale.x * hp / max_hp / 2), position.y - scale.y };
		motion.scale = { scale.x * hp / max_hp, scale.y / 10 };
	}
}
