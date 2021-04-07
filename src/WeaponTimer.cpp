
#include "WeaponTimer.hpp"
#include <utility>
#include "render_components.hpp"
#include "render.hpp"

std::vector<float> WeaponTimer::laser_attr {3000, 10000};
std::vector<float> WeaponTimer::ammo_attr {3000, 5000};
std::vector<float> WeaponTimer::rocket_attr{ 3000, 10000 };
std::vector<float> WeaponTimer::bullet_attr{ 3000, 5000 };
// {WeaponType, [executing_ms, cooldown_ms]}
std::unordered_map<WeaponType , std::vector<float>> WeaponTimer::effectAttributes {
        {W_LASER, laser_attr},
        {W_AMMO, ammo_attr},
        {W_BULLET, bullet_attr},
        {W_ROCKET, rocket_attr}
};

ECS::Entity WeaponTimer::createWeaponTimer(vec2 offset, WeaponType type, std::string texture_path){
    auto entity = ECS::Entity();

    std::string key = "weaponTimer_"+texture_path;
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0)
    {
        resource = ShadedMesh();
        RenderSystem::createSprite(resource, textures_path("/bullet/"+texture_path+".png"), "textured");
    }

    ECS::registry<ShadedMeshRef>.emplace(entity, resource);

    // Initialize the position, scale, and physics components
    auto& motion = ECS::registry<Motion>.emplace(entity);
    motion.angle = 0.f;
    motion.velocity = { 0.f, 0 };
    motion.offset = offset;
    motion.scale = vec2({ 0.5f, 0.5f }) * static_cast<vec2>(resource.texture.size);
    motion.zValue = ZValuesMap["Weapon"];

    auto& wt = ECS::registry<WeaponTimer>.emplace(entity);
    wt.type = type;

    auto& effectTimer = ECS::registry<EffectTimer>.emplace(entity);
    effectTimer.type = type;
    effectTimer.status = READY;
    effectTimer.executing_ms = WeaponTimer::effectAttributes[type][0];
    effectTimer.cooldown_ms = WeaponTimer::effectAttributes[type][1];

    return entity;
}
