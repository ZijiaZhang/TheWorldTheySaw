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
        static ECS::Entity CreateExplosion(vec2 location, float radius, int teamID);
        static void destroy_on_hit(ECS::Entity self,const ECS::Entity e, CollisionResult) {
            if (e.has<Shield>()) {
                if (e.get<Shield>().teamID == self.get<Explosion>().teamID) {
                    return;
                }
            }
            self.get<DeathTimer>().counter_ms = 0;

        }
};

