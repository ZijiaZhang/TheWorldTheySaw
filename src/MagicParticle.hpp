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

typedef enum{
    DAMAGE_DEFAULT,
    FIRE,
}DamageType;


class MagicParticle {
public:
    static ECS::Entity createMagicParticle(vec2 position,
                                           float angle,
                                           vec2 velocity,
                                           int teamID,
                                           MagicWeapon weapon);

    static void destroy_on_hit(ECS::Entity self,const ECS::Entity e, CollisionResult) {
        if(e.has<Shield>()){
            if (e.get<Shield>().teamID == self.get<MagicParticle>().teamID){
                return;
            }
        }
        if(e.has<Enemy>()){
            if (e.get<Enemy>().teamID == self.get<MagicParticle>().teamID){
                return;
            }
        }
        if(e.has<Soldier>()){
            if (e.get<Soldier>().teamID == self.get<MagicParticle>().teamID){
                return;
            }
        }
        if (e.has<Explosion>()) {
            return;
        }
        if (!self.has<DeathTimer>()) {
            self.emplace<DeathTimer>();
        }
    };
    int teamID = 0;
    float damage = 0;
    DamageType type;

    static std::unordered_map<MagicWeapon , std::string> magic_texture_map;

    static std::unordered_map<MagicWeapon , float> magic_damage_map;

    static std::unordered_map<MagicWeapon , DamageType> magic_type_map;

    static std::unordered_map<MagicWeapon , COLLISION_HANDLER> magic_overlap_map;

    static std::unordered_map<MagicWeapon , COLLISION_HANDLER> magic_hit_map;

};

