//
// Created by Gary on 2/4/2021.
//

#include "Enemy.hpp"

#include <utility>
#include "render.hpp"
#include "PhysicsObject.hpp"
#include "Bullet.hpp"
#include "Explosion.hpp"
#include "EnemyAi.hpp"

std::string Enemy::ori_texture_path = "/enemy/cannon/alien.png";
std::string Enemy::frozen_texture_path = "/enemy/frozen.jpeg";
std::string Enemy::ori_shader_name = "textured";
std::string Enemy::frozen_shader_name = "frozen";

std::unordered_map<std::string, EnemyType> Enemy::enemy_type_map = {
    {"standard", EnemyType::STANDARD},
    {"suicide", EnemyType::SUICIDE},
    {"elite", EnemyType::ELITE}
};

ECS::Entity Enemy::createEnemy(vec2 position,
                               COLLISION_HANDLER overlap,
                               COLLISION_HANDLER hit, int teamID, EnemyType type, float hp){
    auto entity = ECS::Entity();

    Enemy::set_shader(entity);

    // Setting initial motion values
    Motion& motion = ECS::registry<Motion>.emplace(entity);
    motion.position = position;
    motion.angle = 0.f;
    motion.velocity = { 0.f, 0.f };
    motion.scale = entity.get<ShadedMeshRef>().reference_to_cache->mesh.original_size * 50.f;
    motion.scale.x *= -1; // point front to the right
    motion.zValue = ZValuesMap["Enemy"];

    motion.max_control_speed = 70.f;
    PhysicsObject physicsObject;
    physicsObject.mass = 30;
    physicsObject.object_type = ENEMY;
    physicsObject.vertex = {
            {
                    PhysicsVertex{{-0.4, 0.4, -0.02}},
                    PhysicsVertex{{0.2, 0.2, -0.02}},
                    PhysicsVertex{{0.3, 0, -0.02}},
                    PhysicsVertex{{0.2, -0.2, -0.02}},
                    PhysicsVertex{{-0.4, -0.4, -0.02}},
                    PhysicsVertex{{0, 0, -0.02}},
            }
    };
    physicsObject.non_convex = true;
    physicsObject.decompose_key = "enemy_basic";
    physicsObject.faces = {{0,1}, {1,2 },{2,3 },{3,4 }, {4,5 }, {5,0}};
    physicsObject.attach(Overlap, std::move(overlap));
    physicsObject.attach(Hit, std::move(hit));
    ECS::registry<PhysicsObject>.insert(entity, physicsObject);

    auto& e = ECS::registry<Enemy>.emplace(entity);
    e.teamID = teamID;
    e.type = type;

    auto& health_component = entity.emplace<Health>();
    float max_health = 5;
    switch (e.type) {
        case STANDARD:
            max_health = 5;
            break;
        case SUICIDE:
            max_health = 1;
            break;
        case ELITE:
            max_health = 10;
            break;
        default:
            max_health = 5;
            break;
    }
    if (hp < max_health && hp > 0) {
        health_component.hp = hp;
    }
    else {
        health_component.hp = max_health;
    }
    
    health_component.max_hp = max_health;

    entity.emplace<AIPath>();
    
    return entity;
}

void Enemy::enemy_bullet_hit_death(ECS::Entity self, const ECS::Entity e, CollisionResult) {
    if (e.has<Bullet>() && e.get<Bullet>().teamID != self.get<Enemy>().teamID && !self.has<DeathTimer>()) {
        if (Bullet::bulletEffect[e.get<Bullet>().type]) {
            Bullet::bulletEffect[e.get<Bullet>().type](e, self, 0.0);
        }
        // EnemyAISystem::takeDamage(self, e.get<Bullet>().damage);
        std::cout << "hit\n";
        auto bullet_type = e.get<Bullet>().bullet_indicator;
        // bullet
        EnemyAISystem::takeDamage(self, e.get<Bullet>().damage);

    }
    else if (e.has<Explosion>() && e.get<Explosion>().teamID != self.get<Enemy>().teamID && !self.has<DeathTimer>()) {
        EnemyAISystem::takeDamage(self, e.get<Explosion>().damage);
        e.get<Explosion>().damage = 0.f;
    }
    /*
    else if ((e.has<Soldier>() || e.has<Shield>()) && (self.has<Enemy>() && self.get<Enemy>().type == EnemyType::SUICIDE) && self.has<Health>() && !self.has<DeathTimer>()) {
        std::cout << "explode\n";
        EnemyAISystem::takeDamage(self, self.get<Health>().max_hp);
    }
    */
};

void Enemy::set_shader(ECS::Entity self, bool effect, std::string texture_path, std::string shader_name) {
    if (ECS::registry<ShadedMeshRef>.has(self)) {
        ECS::registry<ShadedMeshRef>.remove(self);
    }
    std::string key = effect ? "enemy" + std::to_string(self.id) : "enemy";
    ShadedMesh& resource = cache_resource(key);
    if (resource.mesh.vertices.size() == 0)
    {
        resource = ShadedMesh();
        RenderSystem::createSprite(resource, textures_path(texture_path), shader_name);
    }

    ECS::registry<ShadedMeshRef>.emplace(self, resource);
}

void Enemy::set_frozen_shader(ECS::Entity self) {
    set_shader(self, true, Enemy::frozen_texture_path);
}

void Enemy::set_activating_shader(ECS::Entity self) {
    set_shader(self, true, Enemy::frozen_texture_path, Enemy::frozen_shader_name);
}