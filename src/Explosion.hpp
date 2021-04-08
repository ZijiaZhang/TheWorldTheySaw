//
// Created by Gary on 3/16/2021.
//

#pragma once


#include "tiny_ecs.hpp"
#include "common.hpp"
#include "physics.hpp"
#include "shield.hpp"
#include "render_components.hpp"

class Explosion {
    public:
        int teamID;
        float damage;
        static ECS::Entity CreateExplosion(vec2 location, float radius, int teamID, float damage = 1.0f);
        static void destroy_on_hit(ECS::Entity self,const ECS::Entity e, CollisionResult) {
            if (e.has<Shield>() && e.has<Health>()) {
                if (e.get<Shield>().teamID == self.get<Explosion>().teamID) {
                    return;
                }
                else {
                    e.get<Health>().hp -= self.get<Explosion>().damage;
                    self.get<Explosion>().damage = 0;
                }
            }
            // self.get<DeathTimer>().counter_ms = 0;
        }
};

