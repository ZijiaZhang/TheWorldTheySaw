#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "SoldierAi.hpp"
enum effectStatus {
    READY, EXECUTING, COOLDOWN,
};

struct EffectTimer {
    WeaponType type = W_BULLET;
    effectStatus status = READY;
    float cooldown_ms = 10000;
    float executing_ms = 3000;
};

struct WeaponTimer {
    static std::unordered_map<WeaponType , std::vector<float>> effectAttributes;
    static std::vector<float> laser_attr;
    static std::vector<float> ammo_attr;
    static ECS::Entity createWeaponTimer(vec2 offset, WeaponType type, std::string texture_path);
    WeaponType type;
    std::string texture_path;
    static void createAllWeaponTimers() {
        WeaponTimer::createWeaponTimer({400, 200}, W_LASER, "laser");
        WeaponTimer::createWeaponTimer({400, 130}, W_AMMO, "ammo");
    }

    static bool updateAllWeaponTimers(float elapsed_ms) {
        auto entities = ECS::registry<WeaponTimer>.entities;
        for (auto e : entities) {
            auto& et = ECS::registry<EffectTimer>.get(e);
            if (et.status == EXECUTING) {
                et.executing_ms -= elapsed_ms;
                if (et.executing_ms <= 0.) {
                    et.status = COOLDOWN;
                    et.executing_ms = WeaponTimer::effectAttributes[et.type][0];
                    return true;
                }
            } else if (et.status == COOLDOWN) {
                et.cooldown_ms -= elapsed_ms;
                if (et.cooldown_ms <= 0.f) {
                    et.status = READY;
                    et.cooldown_ms = WeaponTimer::effectAttributes[et.type][1];
                }
            }
        }
        return false;
    }

    static void updateTimerWhenSwitchWeapon(WeaponType type) {
        auto entities = ECS::registry<EffectTimer>.entities;
        for (auto e : entities) {
            auto& effectTimer = ECS::registry<EffectTimer>.get(e);
            if (effectTimer.status == EXECUTING) {
                // set the current executing effect to COOLDOWN
                effectTimer.status = COOLDOWN;
            }
            if (effectTimer.type == type) {
                effectTimer.status = EXECUTING;
            }
        }
    }
};
