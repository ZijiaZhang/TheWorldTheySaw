#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "SoldierAi.hpp"
#include <AiState.hpp>
#include "health_bar.hpp"
#include "Weapon.hpp"
#include "GameInstance.hpp"
#include "render_components.hpp"

enum class SoldierType {
    BASIC, MEDIUM, HEAVY
};

struct Soldier
{
public:
	// Creates all the associated render resources and default transform
	static ECS::Entity createSoldier(vec2 pos, COLLISION_HANDLER overlap = [](ECS::Entity, const ECS::Entity, CollisionResult) {},
                                     COLLISION_HANDLER hit = [](ECS::Entity, const ECS::Entity, CollisionResult) {});

	AiState soldierState = AiState::IDLE;
	ECS::Entity weapon;
    MagicWeapon magic = FIREBALL;
	int teamID = 0;

	static void soldier_bullet_hit_death(ECS::Entity self, const ECS::Entity e, CollisionResult);

	static void addHealth(ECS::Entity self, float bloodIn) {
	    if (self.has<Health>()) {
	        auto& health = ECS::registry<Health>.get(self);
	        if (health.hp + bloodIn >= health.max_hp) {
	            health.hp = health.max_hp;
	        } else {
	            health.hp += bloodIn;
	        }
	    }
	}

	static void switchWeapon(ECS::Entity self, WeaponType type) {
	    // change the weapon texture
        // remove old weapon
        ECS::ContainerInterface::remove_all_components_of(self.get<Soldier>().weapon);
        // new weapon
        std::string path = Weapon::weaponTexturePath[type];
        ECS::Entity weapon = Weapon::createWeapon(vec2 {0,20.f}, 0, self, path);
        auto& children_entity = ECS::registry<ChildrenEntities>.get(self);
        children_entity.children.insert(weapon);
        self.get<Soldier>().weapon = weapon;

        GameInstance::selectedWeapon = type;
	}
};
