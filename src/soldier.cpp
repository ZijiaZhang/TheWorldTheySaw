// Header
#include "soldier.hpp"

#include <utility>
#include "render.hpp"
#include "PhysicsObject.hpp"
#include "Weapon.hpp"
#include "Bullet.hpp"

std::string Soldier::ori_texture_path = "/soldier/soldier_basic.png";
std::string Soldier::field_texture_path = "/soldier/forcefield.png";
std::string Soldier::ori_shader_name = "textured";
std::string Soldier::field_shader_name = "frozen";

ECS::Entity Soldier::createSoldier(vec2 position,
                                   COLLISION_HANDLER overlap,
                                   COLLISION_HANDLER hit, float light_intensity, float hp)
{
	auto entity = ECS::Entity();

    Soldier::set_shader(entity);
//    std::string key = "soldier";
//    ShadedMesh& resource = cache_resource(key);
//    if (resource.effect.program.resource == 0)
//    {
//        RenderSystem::createSprite(resource, textures_path("/soldier/soldier_basic.png"), "textured");
//    }

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	//ECS::registry<ShadedMeshRef>.emplace(entity, resource);

	// Setting initial motion values
	Motion& motion = ECS::registry<Motion>.emplace(entity);
	motion.position = position;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = entity.get<ShadedMeshRef>().reference_to_cache->mesh.original_size * 70.f;
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

	auto& health_component = ECS::registry<Health>.emplace(entity);
    float max_health = 10.f;
    if (hp < max_health && hp > 0) {
        health_component.hp = hp;
    }
    else {
        health_component.hp = max_health;
    }
    health_component.max_hp = max_health;
	
	return entity;
}

ECS::Entity Soldier::createSoldier(Motion m, Soldier s, Health h, AIPath ai, PhysicsObject po)
{
    auto e = ECS::Entity();

    Soldier::set_shader(e);

    ECS::Entity weapon = Weapon::createWeapon(vec2{ 0,20.f }, 0, e);
    auto& children_entity = e.emplace<ChildrenEntities>();
    children_entity.children.insert(weapon);

    e.emplace<Motion>(m);
    e.emplace<Soldier>(s).weapon = weapon;
    e.emplace<Health>(h);
    e.emplace<AIPath>(ai);
    e.emplace<PhysicsObject>(po);

    return e;
}


void Soldier::soldier_bullet_hit_death(ECS::Entity self, const ECS::Entity e, CollisionResult c) {
    if (!self.has<DeathTimer>()) {
        if (e.has<Bullet>() && (e.get<Bullet>().teamID != self.get<Soldier>().teamID)) {
            auto& health = self.get<Health>();
            // auto& bullet_indicator = e.get<Bullet>().velocity_indicator;
            // std::cout << bullet_indicator << "!!!!!";
            if(!ECS::registry<Soldier>.get(self).forcefield_on){
               health.hp -= Bullet::bulletDamage[e.get<Bullet>().type];
        }
            
            if (health.hp <= 0)
                self.emplace<DeathTimer>();
            Particle::createParticle(c.vertex, { 50,50 }, 1000);

        }
        else if (e.has<Explosion>() && (e.get<Explosion>().teamID != self.get<Soldier>().teamID)) {
            auto& health = self.get<Health>();
             if(!ECS::registry<Soldier>.get(self).forcefield_on){
              health.hp -= e.get<Explosion>().damage;
        }
            e.get<Explosion>().damage = 0.f;

            if (health.hp <= 0)
                self.emplace<DeathTimer>();
        }
    }
    

    // explosion
};

void Soldier::set_shader(ECS::Entity self, bool effect, std::string texture_path, std::string shader_name) {
    if (ECS::registry<ShadedMeshRef>.has(self)) {
        ECS::registry<ShadedMeshRef>.remove(self);
    }
    std::string key = effect ? "soldier" + std::to_string(self.id) : "soldier";
    ShadedMesh& resource = cache_resource(key);
    if (resource.mesh.vertices.size() == 0)
    {
        resource = ShadedMesh();
        RenderSystem::createSprite(resource, textures_path(texture_path), shader_name);
    }

    ECS::registry<ShadedMeshRef>.emplace(self, resource);
}

void Soldier::set_field_shader(ECS::Entity self) {
    set_shader(self, true, Soldier::field_texture_path);
}

void Soldier::set_field(ECS::Entity self) {
    ECS::registry<Soldier>.get(self).forcefield_on = true;
    if (!ECS::registry<FieldTimer>.has(self)) {
        ECS::registry<FieldTimer>.emplace(self);
    }
}
