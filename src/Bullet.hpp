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
    static ECS::Entity createBullet(vec2 position, float angle, vec2 velocity, int teamID, std::string name);

    static void destroy_on_hit(ECS::Entity &self,const ECS::Entity &e, CollisionResult) {
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

        if (!self.has<DeathTimer>()) {
            self.emplace<DeathTimer>();
        }
    };
    int teamID = 0;
};


