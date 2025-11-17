#include "WeaponConfig.hpp"

bool WeaponConfigRegistry::defaultsInitialized = false;

std::unordered_map<WeaponType, WeaponFireConfig>& WeaponConfigRegistry::weaponConfigStorage() {
    static std::unordered_map<WeaponType, WeaponFireConfig> storage;
    return storage;
}

std::unordered_map<std::string, BulletModifier>& WeaponConfigRegistry::bulletModifierStorage() {
    static std::unordered_map<std::string, BulletModifier> storage;
    return storage;
}

void WeaponConfigRegistry::initializeDefaults() {
    if (defaultsInitialized) {
        return;
    }

    registerWeaponConfig(
            WeaponFireConfig{W_BULLET, BULLET_RELOAD, vec2{380.f, 0.f}, vec2{1.f, 1.f}, 1200.f, "bullet",
                             "/soldier/weapon_heavy.png", "gun_fire.wav", false, 1.1f});
    registerWeaponConfig(
            WeaponFireConfig{W_ROCKET, ROCKET_RELOAD, vec2{150.f, 0.f}, vec2{1.f, 1.f}, 2500.f, "rocket",
                             "/soldier/weapon_rocket.png", "firework.wav", true, 3.85f});
    registerWeaponConfig(
            WeaponFireConfig{W_LASER, LAZER_RELOAD, vec2{400.f, 0.f}, vec2{1.f, 1.f}, 750.f, "laser",
                             "/soldier/sword.png", "laser.wav", false, 0.73f});
    registerWeaponConfig(
            WeaponFireConfig{W_AMMO, AMMO_RELOAD, vec2{200.f, 0.f}, vec2{1.f, 1.f}, 1800.f, "ammo",
                             "/soldier/weapon_frozen.png", "ammo.wav", false, 0.58f});

    defaultsInitialized = true;
}

void WeaponConfigRegistry::registerWeaponConfig(const WeaponFireConfig& config) {
    weaponConfigStorage()[config.type] = config;
}

void WeaponConfigRegistry::removeWeaponConfig(WeaponType type) {
    weaponConfigStorage().erase(type);
}

const WeaponFireConfig* WeaponConfigRegistry::getWeaponConfig(WeaponType type) {
    auto& configs = weaponConfigStorage();
    auto it = configs.find(type);
    if (it == configs.end()) {
        return nullptr;
    }
    return &it->second;
}

void WeaponConfigRegistry::registerBulletModifier(const std::string& id, const BulletModifier& modifier) {
    bulletModifierStorage()[id] = modifier;
}

void WeaponConfigRegistry::unregisterBulletModifier(const std::string& id) {
    bulletModifierStorage().erase(id);
}

void WeaponConfigRegistry::clearBulletModifiers() {
    bulletModifierStorage().clear();
}

const std::unordered_map<std::string, BulletModifier>& WeaponConfigRegistry::getBulletModifiers() {
    return bulletModifierStorage();
}


