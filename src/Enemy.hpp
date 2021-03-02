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
	static ECS::Entity createEnemy(vec2 position);
	Path_with_heuristics path;
	vec2 desired_speed = { 0.f, 0.f };

	AiState enemyState = AiState::IDLE;
};


