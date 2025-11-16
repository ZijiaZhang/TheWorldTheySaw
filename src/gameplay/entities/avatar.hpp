#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "PhysicsObject.hpp"

enum class AvatarType {
    AVATAR,
    AVATAR_HIT
};
// Soldier avatar
struct Avatar
{
public:
	// Creates all the associated render resources and default transform
	static ECS::Entity createAvatar(vec2 position, AvatarType avatarType);
    void avatar_bullet_hit(ECS::Entity self);
    AvatarType avatarType = AvatarType::AVATAR;
    static std::map<AvatarType, std::string> avatarNamesMap;
    bool on_hit = false;
};
