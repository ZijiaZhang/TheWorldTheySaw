#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include <levelLoader.hpp>

static enum class ButtonType {
	DEFAULT_BUTTON, START, LEVEL_SELECT, QUIT
};

struct Button {
	static ECS::Entity createButton(ButtonType buttonType, vec2 position);

	ButtonType buttonType = ButtonType::DEFAULT_BUTTON;

	static std::map<ButtonType, std::string> buttonNames;
};