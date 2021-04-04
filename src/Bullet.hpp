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
    static ECS::Entity createBullet(vec2 position, float angle, vec2 velocity, int teamID, std::string texture_name, float lifetime = -1,
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
    int teamID = 0;
    int velocity_indicator = 0;
    std::function<void(ECS::Entity)> on_destroy = [](ECS::Entity e){
        if(!e.has<DeathTimer>())
        {
            e.emplace<DeathTimer>();
        }
    };
};

// A timer that will be associated to dying object
struct ExplodeTimer
{
    float counter_ms = 1000;
    std::function<void(ECS::Entity)> callback;
};
