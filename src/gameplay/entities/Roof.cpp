#include "Roof.hpp"
#include "render.hpp"
#include "Building.hpp"
#include <cmath>
#include <iostream>

ECS::Entity RoofSystem::createRoof(vec2 position, vec2 size, float rotation, ECS::Entity parent_building) {
    ECS::Entity entity = ECS::Entity();

    // Create Motion (slightly larger than building for overhang effect)
    Motion& motion = entity.emplace<Motion>();
    motion.position = position;
    motion.angle = rotation;
    motion.scale = size * 1.1f; // 10% overhang
    
    // Set high Z value to render on top
    if (ZValuesMap.count("Roof")) {
        motion.zValue = ZValuesMap["Roof"];
    } else {
        motion.zValue = 0.9f; // High Z to render above walls (0.5), but < 1.0 to avoid clipping
    }

    // Add Roof component
    Roof& roof = entity.emplace<Roof>();
   roof.parent_building = parent_building;
    roof.current_opacity = 1.0f;

    // Add Rendering Components with transparent shader
    std::string key = "roof";
    ShadedMesh& resource = cache_resource(key);
    if (resource.mesh.vertices.empty()) {
        resource = ShadedMesh();
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3{-0.5, 0.5, -0.02}, vec3{1.0, 1.0, 1.0}});
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3{0.5, 0.5, -0.02}, vec3{1.0, 1.0, 1.0}});
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3{0.5, -0.5, -0.02}, vec3{1.0, 1.0, 1.0}});
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3{-0.5, -0.5, -0.02}, vec3{1.0, 1.0, 1.0}});

        resource.mesh.vertex_indices = std::vector<uint16_t>({0, 2, 1, 0, 3, 2});

        RenderSystem::createColoredMesh(resource, "roof_transparent");
    }
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);
    
    // Brown color for roof
    resource.texture.color = {0.6f, 0.4f, 0.2f};
    
    std::cout << "Created roof at position (" << position.x << ", " << position.y << ") with size (" << size.x << ", " << size.y << "), zValue=" << motion.zValue << ", opacity=" << roof.current_opacity << std::endl;
    std::cout << "Roof has Motion: " << entity.has<Motion>() << ", has Roof: " << entity.has<Roof>() << ", has ShadedMeshRef: " << entity.has<ShadedMeshRef>() << std::endl;

    return entity;
}

void RoofSystem::updateRoofTransparency(vec2 player_pos) {
    for (auto& entity : ECS::registry<Roof>.entities) {
        auto& roof = entity.get<Roof>();

        // Check if parent building still exists
        if (!roof.parent_building.has<Building>() || !roof.parent_building.has<Motion>()) {
            continue;
        }

        // Get parent building position
        vec2 building_pos = roof.parent_building.get<Motion>().position;
        
        // Calculate distance from player to building
        std::cout << "player_pos" << player_pos.x << " " << player_pos.y << " " << building_pos.x << " " << building_pos.y << std::endl;
        float distance = length(player_pos - building_pos);
        std::cout << "Distance:" << distance  << std::endl;

        // Calculate opacity based on distance
        float new_opacity = 1.0f;
        if (distance < roof.transparency_distance) {
            new_opacity = 0.0f; // Fully transparent when player is close
        } else if (distance < roof.transparency_distance + roof.fade_range) {
            // Smooth fade in the transition zone
            float fade_factor = (distance - roof.transparency_distance) / roof.fade_range;
            new_opacity = fade_factor;
        }
        
        if (std::abs(new_opacity - roof.current_opacity) > 0.01f) {
            std::cout << "Roof opacity changed: " << roof.current_opacity << " -> " << new_opacity << " (distance=" << distance << ")" << std::endl;
        }
        std::cout << "opacity=" << roof.current_opacity << " -> " << new_opacity << std::endl;
        roof.current_opacity = new_opacity;
        // Note: The opacity will be passed to the shader in the render system
    }
}
