#include "Roof.hpp"
#include "render.hpp"
#include "Building.hpp"
#include <cmath>

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
        motion.zValue = 2.0f; // High Z to render above walls
    }

    // Add Roof component
    Roof& roof = entity.emplace<Roof>();
    roof.parent_building = parent_building;
    roof.current_opacity = 1.0f;

    // Add Rendering Components (same as wall, but with transparency)
    std::string key = "roof";
    ShadedMesh& resource = cache_resource(key);
    if (resource.mesh.vertices.empty()) {
        resource = ShadedMesh();
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3{-0.5, 0.5, -0.02}, vec3{0.0, 0.0, 0.0}});
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3{0.5, 0.5, -0.02}, vec3{0.0, 0.0, 0.0}});
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3{0.5, -0.5, -0.02}, vec3{0.0, 0.0, 0.0}});
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3{-0.5, -0.5, -0.02}, vec3{0.0, 0.0, 0.0}});

        resource.mesh.vertex_indices = std::vector<uint16_t>({0, 2, 1, 0, 3, 2});

        RenderSystem::createColoredMesh(resource, "mesh_flat_highlight");
    }
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);
    
    // Reddish-brown color for roof (like terracotta)
    resource.texture.color = {0.6f, 0.3f, 0.2f};

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
        float distance = length(player_pos - building_pos);
        
        // Calculate opacity based on distance
        float new_opacity = 1.0f;
        if (distance < roof.transparency_distance) {
            new_opacity = 0.0f; // Fully transparent when player is close
        } else if (distance < roof.transparency_distance + roof.fade_range) {
            // Smooth fade in the transition zone
            float fade_factor = (distance - roof.transparency_distance) / roof.fade_range;
            new_opacity = fade_factor;
        }
        
        roof.current_opacity = new_opacity;
        
        // Update color alpha (this will be used by the shader if it supports it)
        // For now, we can scale the color by opacity
        if (entity.has<ShadedMeshRef>()) {
            auto& mesh_ref = entity.get<ShadedMeshRef>();
            // Store base color and multiply by opacity
            // Note: This is a simple approach. A proper alpha channel would be better.
            mesh_ref.reference_to_cache->texture.color = {
                0.6f * new_opacity,
                0.3f * new_opacity,
                0.2f * new_opacity
            };
        }
    }
}
