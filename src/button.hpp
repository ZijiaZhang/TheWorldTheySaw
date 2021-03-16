#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include <levelLoader.hpp>

enum class ButtonType {
	DEFAULT_BUTTON,
	START,
	LEVEL_SELECT,
	QUIT,
	SELECT_ROCKET,
    SELECT_AMMO,
    SELECT_BULLET,
    SELECT_LASER,
    SELECT_DIRECT,
    SELECT_A_STAR,
    RETURN,
    LEVEL1,
    LEVEL2,
	LEVEL3
};

struct Button {
	static ECS::Entity createButton(ButtonType buttonType, vec2 position, COLLISION_HANDLER overlap);

	ButtonType buttonType = ButtonType::DEFAULT_BUTTON;

	static std::map<ButtonType, std::string> buttonNames;
};
