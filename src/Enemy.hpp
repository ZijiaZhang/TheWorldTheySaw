//
// Created by Gary on 2/4/2021.
//

#pragma once
#include "tiny_ecs.hpp"
#include "common.hpp"
#include "ai.hpp"
#include <AiState.hpp>

class Enemy {

public:
    static ECS::Entity createEnemy(vec2 position, std::function<void(ECS::Entity&, const  ECS::Entity&)> overlap = [](ECS::Entity&, const ECS::Entity &) {},
                                   std::function<void(ECS::Entity&, const  ECS::Entity&)> hit = [](ECS::Entity&, const ECS::Entity &) {});
    AiState enemyState = AiState::IDLE;
};



