// Header
#include "soldier.hpp"
#include "render.hpp"
#include "PhysicsObject.hpp"
#include "Weapon.hpp"

std::map<SoldierType, std::string> Soldier::soldierTypes = {
    {SoldierType::BASIC, "basic"},
    {SoldierType::MEDIUM, "medium"},
    {SoldierType::HEAVY, "heavy"}
};

ECS::Entity Soldier::createSoldier(SoldierType type, vec2 position)
{
	auto entity = ECS::Entity();

    std::string key = soldierTypes[type];
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0)
    {
        std::string path = "/soldier/soldier_";
        path.append(key);
        path.append(".png");
        resource = ShadedMesh();
        RenderSystem::createSprite(resource, textures_path(path), "textured");
    }

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	ECS::registry<ShadedMeshRef>.emplace(entity, resource);

	// Setting initial motion values
	Motion& motion = ECS::registry<Motion>.emplace(entity);
	motion.position = position;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = resource.mesh.original_size * 70.f;
	motion.scale.x *= -1; // point front to the right
    motion.zValue = ZValuesMap["Soldier"];

	PhysicsObject physicsObject;
	physicsObject.mass = 100;
	physicsObject.object_type = PLAYER;
	physicsObject.vertex = {
            {
                    PhysicsVertex{{-0.4, 0.2, -0.02}},
                    PhysicsVertex{{0.3, 0.3, -0.02}},
                    PhysicsVertex{{0.3, -0.3, -0.02}},
                    PhysicsVertex{{-0.2, -0.2, -0.02}}
            }
	};
    physicsObject.faces = {{0,1}, {1,2 },{2,3 },{3,0 }};

    ECS::registry<PhysicsObject>.insert(entity, physicsObject);
	// Create and (empty) Soldier component to be able to refer to all turtles
	ECS::Entity weapon = Weapon::createWeapon(vec2 {0,60.f}, 0, entity);
	ECS::registry<Soldier>.emplace(entity);
	return entity;
}
