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
    SELECT_FIREBALL,
    SELECT_FIELD,
    NEXT,
    RESTART,
    RETURN,
    SAVE,
    CONTINUE,
    LOCKED,
    TUTORIAL,
    SETTING,
    LEVEL1,
    LEVEL2,
	LEVEL3,
    LEVEL4,
    LEVEL5,
    LEVEL6,
    LEVEL7,
    LEVEL8,
    LEVEL9,
    LEVEL10,
    LEVEL11,
    LEVEL12
};

enum class ButtonClass{
    BUTTON_CLASS_OTHER,
    WEAPON_SELECTION,
    ALGORITHM_SELECTION,
    MAGIC_SELECTION,
};

struct Button {
    static ECS::Entity createButton(ButtonIcon buttonType, vec2 position,  COLLISION_HANDLER overlap, float scale = 0.5);

	ButtonIcon buttonType = ButtonIcon::DEFAULT_BUTTON;
	ButtonClass buttonClass = ButtonClass::BUTTON_CLASS_OTHER;
    bool selected() {
        if (buttonClass == ButtonClass::WEAPON_SELECTION) {
            return weaponTypeMap[buttonType] == GameInstance::selectedWeapon;
        } else if (buttonClass == ButtonClass::ALGORITHM_SELECTION) {
                return algorithmMap[buttonType] == GameInstance::algorithm;
        }
        else if (buttonClass == ButtonClass::MAGIC_SELECTION) {
            return magicMap[buttonType] == GameInstance::selectedMagic;
        }
        return false;
    }
	static std::map<ButtonIcon, std::string> buttonNamesMap;
	static std::map<ButtonIcon, ButtonClass> buttonClassMap;
    static std::map<ButtonIcon, WeaponType> weaponTypeMap;
    static std::map<ButtonIcon, AIAlgorithm> algorithmMap;
    static std::map<ButtonIcon, MagicWeapon> magicMap;
};
