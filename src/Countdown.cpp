#include "Countdown.hpp"
#include <render_components.hpp>
#include <render.hpp>

ShadedMesh& get_cashed_number(int num) {
    std::string key = "countdown_" + std::to_string(num);
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0)
    {
        resource = ShadedMesh();
        std::string path = "/countdown/";
        path.append(std::to_string(num));
        path.append(".png");
        RenderSystem::createSprite(resource, textures_path(path), "textured");
    }
    return resource;
}


ECS::Entity Countdown::createCountdown(float countdown_ms)
{
	ECS::Entity e;
    ECS::registry<Countdown>.emplace(e);
	auto& cd = ECS::registry<CountDownTimer>.emplace(e);
	cd.pause_time = countdown_ms;

    get_cashed_number(1);
    get_cashed_number(2);
    ShadedMesh& resource = get_cashed_number(3);

    ECS::registry<ShadedMeshRefUI>.emplace(e, resource);
    
    auto& motion = ECS::registry<Motion>.emplace(e);
    motion.scale = vec2{ 300,300 };
	return e;
}
