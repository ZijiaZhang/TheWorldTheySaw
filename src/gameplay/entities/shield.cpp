// Header
#include "shield.hpp"
#include "render.hpp"
#include "PhysicsObject.hpp"
#include "Bullet.hpp"

ECS::Entity Shield::createShield(vec2 position,  int teamID, float hp)
{
	// Reserve en entity
	auto entity = ECS::Entity();

	// Create the rendering components
	std::string key = "shield";
	ShadedMesh& resource = cache_resource(key);
	if (resource.effect.program.resource == 0)
	{
		resource = ShadedMesh();
		RenderSystem::createSprite(resource, textures_path("/shield/shield3.png"), "textured");
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
    physics.vertex = {
        {
                PhysicsVertex{{-0.4, 0.2, -0.02}},
                PhysicsVertex{{0.3, 0.3, -0.02}},
                PhysicsVertex{{0.3, -0.3, -0.02}},
                PhysicsVertex{{-0.2, -0.2, -0.02}}
            }
    };
    physics.faces = {{0,1}, {1,2 },{2,3 },{3,0 }};
    physics.attach(Overlap, Shield::shield_bullet_hit_death);
    physics.attach(Hit, Shield::shield_bullet_hit_death);

	// Create and (empty) Fish component to be able to refer to all fish
	auto& shield = ECS::registry<Shield>.emplace(entity);
    shield.teamID = teamID;

    auto& health_component = ECS::registry<Health>.emplace(entity);
    float max_health = 20.f;
    if (hp < max_health && hp > 0) {
        health_component.hp = hp;
    }
    else {
        health_component.hp = max_health;
    }
    health_component.max_hp = max_health;

	return entity;
}

ECS::Entity Shield::createShield(Motion m, Shield s, Health h, PhysicsObject po)
{
    auto entity = ECS::Entity();

    // Create the rendering components
    std::string key = "shield";
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0)
    {
        resource = ShadedMesh();
        RenderSystem::createSprite(resource, textures_path("/shield/shield3.png"), "textured");
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);

    entity.emplace<Motion>(m);
    entity.emplace<Shield>(s);
    entity.emplace<Health>(h);
    entity.emplace<PhysicsObject>(po);

    return entity;
}

void Shield::shield_bullet_hit_death(ECS::Entity self, const ECS::Entity e, CollisionResult) {
    if (e.has<Bullet>() && (e.get<Bullet>().teamID != self.get<Shield>().teamID) && !self.has<DeathTimer>()) {
        auto& health = self.get<Health>();
        health.hp -= Bullet::bulletDamage[e.get<Bullet>().type];
        if (health.hp <= 0)
            self.emplace<DeathTimer>();
    }
};