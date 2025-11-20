// Header
#include "background.hpp"
#include "render.hpp"

ECS::Entity Background::createBackground(vec2 position, std::string name, float depth, float size)
{
	// Reserve en entity
	auto entity = ECS::Entity();

	// Create the rendering components
	std::string key = name;
	ShadedMesh& resource = cache_resource(key);
	if (resource.effect.program.resource == 0)
	{
		resource = ShadedMesh();
        if (name == "white") {
            resource.mesh.vertices.emplace_back(ColoredVertex{vec3 {-0.5, 0.5, -0.02}, vec3{1.0,1.0,1.0}});
            resource.mesh.vertices.emplace_back(ColoredVertex{vec3{0.5, 0.5, -0.02}, vec3{1.0,1.0,1.0}});
            resource.mesh.vertices.emplace_back(ColoredVertex{vec3{0.5, -0.5, -0.02}, vec3{1.0,1.0,1.0}});
            resource.mesh.vertices.emplace_back(ColoredVertex{vec3{-0.5, -0.5, -0.02}, vec3{1.0,1.0,1.0}});
            resource.mesh.vertex_indices = std::vector<uint16_t>({0, 2, 1, 0, 3, 2});
            RenderSystem::createColoredMesh(resource, "mesh_flat_highlight");
        } else {
            std::string path = "/main scene/";
            path.append(name);
            path.append(".png");
            RenderSystem::createSprite(resource, textures_path(path), "sprite_textured");
        }
	}

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	ECS::registry<ShadedMeshRef>.emplace(entity, resource);

	// Initialize the position, scale, and physics components
	auto& motion = ECS::registry<Motion>.emplace(entity);
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0 };
	motion.position = position;
	// Setting initial values, scale is negative to make it face the opposite way
    if (name == "white") {
        motion.scale = vec2({ size, size }) * 1000.f; // Arbitrary large size for white background
    } else {
	    motion.scale = vec2({ size, size }) * static_cast<vec2>(resource.texture.size);
    }
    motion.zValue = ZValuesMap["Background"];

	// Create and (empty) Fish component to be able to refer to all fish
    auto& bg = ECS::registry<Background>.emplace(entity);
    bg.depth = depth;

	return entity;
}
