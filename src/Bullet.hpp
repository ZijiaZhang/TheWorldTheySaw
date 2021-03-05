//
// Created by Gary on 2/12/2021.
//

#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "render.hpp"
#include "shield.hpp"

class Bullet {
public:
    static ECS::Entity createBullet(vec2 position, float angle, int teamID);

//    static void destroy_on_hit(ECS::Entity &self,const ECS::Entity &e) {
//        if (!self.has<DeathTimer>()) {
//            self.emplace<DeathTimer>();
//        }
//    };

    static void destroy_on_hit(ECS::Entity &self,const ECS::Entity &e) {
        if(e.has<Shield>()){
            if (e.get<Shield>().teamID == self.get<Bullet>().teamID){
                return;
            }

        }
        if (!self.has<DeathTimer>()) {
            self.emplace<DeathTimer>();
        }
    };
    int teamID = 0;
};


