#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include <levelLoader.hpp>

enum class ButtonIcon {
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

enum class ButtonClass{
    BUTTON_CLASS_OTHER,
    WEAPON_SELECTION,
    ALGORITHM_SELECTION,
    MAGIC_SELECTION,
};

struct Button {
	static ECS::Entity createButton(ButtonIcon buttonType, vec2 position, COLLISION_HANDLER overlap);

	ButtonIcon buttonType = ButtonIcon::DEFAULT_BUTTON;
	ButtonClass buttonClass = ButtonClass::BUTTON_CLASS_OTHER;
    bool selected = false;
	static std::map<ButtonIcon, std::string> buttonNamesMap;
	static std::map<ButtonIcon, ButtonClass> buttonClassMap;
};
