//
// Created by Gary on 2/12/2021.
//

#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "render.hpp"
#include "shield.hpp"
#include "Enemy.hpp"
#include "soldier.hpp"

class Bullet {
public:
    static ECS::Entity createBullet(vec2 position, float angle, vec2 velocity, int teamID, WeaponType type, std::string texture_name, float lifetime = -1,
                                    std::function<void(ECS::Entity)> callback = [](ECS::Entity e){
                                    if(!e.has<DeathTimer>())
                                    {
                                        e.emplace<DeathTimer>();
                                    }
                                    });

    static void destroy_on_hit(ECS::Entity self,const ECS::Entity e, CollisionResult) {
        if(e.has<Shield>()){
            if (e.get<Shield>().teamID == self.get<Bullet>().teamID){
                return;
            }
        }
        if(e.has<Enemy>()){
            if (e.get<Enemy>().teamID == self.get<Bullet>().teamID){
                return;
            }
        }
        if(e.has<Soldier>()){
            if (e.get<Soldier>().teamID == self.get<Bullet>().teamID){
                return;
            }
        }
        self.get<Bullet>().on_destroy(self);
    };

    static void lazer_penetrate(ECS::Entity self, const ECS::Entity e, CollisionResult) {
        if (e.has<Shield>()) {
            if (e.get<Shield>().teamID == self.get<Bullet>().teamID) {
                return;
            }
        }
        if (e.has<Enemy>()) {
            if (e.get<Enemy>().teamID == self.get<Bullet>().teamID) {
                return;
            }
        }
        if (e.has<Soldier>()) {
            if (e.get<Soldier>().teamID == self.get<Bullet>().teamID) {
                return;
            }
        }
        self.get<Bullet>().penetration_counter--;

        if (self.get<Bullet>().penetration_counter <= 0) {
            self.get<Bullet>().on_destroy(self);
        }
    };

    int teamID = 0;
    std::string bullet_indicator = "";
    std::function<void(ECS::Entity)> on_destroy = [](ECS::Entity e){
        if(!e.has<DeathTimer>())
        {
            e.emplace<DeathTimer>();
        }
    };

    WeaponType type;
    float damage = 1.0;
    int penetration_counter = 0;
    static std::unordered_map<WeaponType, float> bulletDamage;
    static std::unordered_map<WeaponType, std::function<void(ECS::Entity, ECS::Entity, float)>> bulletEffect;
    static void heal_soldier(ECS::Entity soldier_entity, ECS::Entity enemy_entity, float elapsed_ms);
    static void freeze_enemy(ECS::Entity soldier_entity, ECS::Entity enemy_entity, float elapsed_ms);
};

// A timer that will be associated to dying object
struct ExplodeTimer
{
    float counter_ms = 1000;
    std::function<void(ECS::Entity)> callback;
};

struct FrozenTimer {
    float executing_ms = 2000;
};

struct Activating {
    // empty, only used for Activating shader
};