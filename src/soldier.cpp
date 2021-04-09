// Header
#include "soldier.hpp"

#include <utility>
#include "render.hpp"
#include "PhysicsObject.hpp"
#include "Weapon.hpp"
#include "Bullet.hpp"


ECS::Entity Soldier::createSoldier(vec2 position,
                                   COLLISION_HANDLER overlap,
                                   COLLISION_HANDLER hit, float light_intensity)
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
	ECS::Entity weapon = Weapon::createWeapon(vec2 {0,20.f}, 0, entity);
	auto& children_entity = entity.emplace<ChildrenEntities>();
	children_entity.children.insert(weapon);
    entity.emplace<AIPath>();

	auto& soldier = ECS::registry<Soldier>.emplace(entity);
	soldier.weapon = weapon;
	soldier.teamID = 0;
	soldier.light_intensity = light_intensity;

	auto& health = ECS::registry<Health>.emplace(entity);
	health.hp = 10;
	health.max_hp = 10;
	
	return entity;
}


void Soldier::soldier_bullet_hit_death(ECS::Entity self, const ECS::Entity e, CollisionResult) {
    if (!self.has<DeathTimer>()) {
        if (e.has<Bullet>() && (e.get<Bullet>().teamID != self.get<Soldier>().teamID)) {
            auto& health = self.get<Health>();
            // auto& bullet_indicator = e.get<Bullet>().velocity_indicator;
            // std::cout << bullet_indicator << "!!!!!";
            health.hp -= Bullet::bulletDamage[e.get<Bullet>().type];

            if (health.hp <= 0)
                self.emplace<DeathTimer>();
        }
        else if (e.has<Explosion>() && (e.get<Explosion>().teamID != self.get<Soldier>().teamID)) {
            auto& health = self.get<Health>();
            health.hp -= e.get<Explosion>().damage;
            e.get<Explosion>().damage = 0.f;

            if (health.hp <= 0)
                self.emplace<DeathTimer>();
        }
    }
    

    // explosion
};
