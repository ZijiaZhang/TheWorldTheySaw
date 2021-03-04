#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

enum class SoldierType {
    BASIC, MEDIUM, HEAVY
};

struct Soldier
{
	// Creates all the associated render resources and default transform
	static ECS::Entity createSoldier(SoldierType type, vec2 pos);
    SoldierType type = SoldierType::BASIC;
    static std::map<SoldierType, std::string> soldierTypes;
};
