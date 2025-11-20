#include "DestructibleWall.hpp"
#include "Wall.hpp"
#include "render.hpp"
#include "MagicParticle.hpp"
#include <random>
#include <iostream>

ECS::Entity DestructibleWallSystem::createDestructibleWall(vec2 location, vec2 size, float rotation) {
    // Create a standard wall first, but add DestructibleWall component
    // We use the standard wall creation but might need to override collision handler if we want specific behavior
    // For now, let's assume we use a custom collision handler or just check for the component in the global handler
    
    ECS::Entity entity = ECS::Entity();

    // Create Motion
    Motion& motion = entity.insert(Motion());
    motion.position = location;
    motion.angle = rotation;
    motion.scale = size;

    // Create PhysicsObject
    PhysicsObject& physics = entity.insert(PhysicsObject());
    physics.object_type = WALL;
    physics.fixed = true;
    physics.mass = 1000;
    physics.attach(Overlap, DestructibleWallSystem::onOverlap);

    // Add DestructibleWall component
    entity.insert(DestructibleWall());
    
    // Add Wall component (tag)
    entity.insert(Wall());

    // Add Rendering Components (Mesh)
    std::string key = "wall";
    ShadedMesh& resource = cache_resource(key);
    if (resource.mesh.vertices.empty())
    {
        resource = ShadedMesh();
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3 {-0.5, 0.5, -0.02}, vec3{0.0,0.0,0.0}});
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3{0.5, 0.5, -0.02}, vec3{0.0,0.0,0.0}});
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3{0.5, -0.5, -0.02}, vec3{0.0,0.0,0.0}});
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3{-0.5, -0.5, -0.02}, vec3{0.0,0.0,0.0}});

        resource.mesh.vertex_indices = std::vector<uint16_t>({0, 2, 1, 0, 3, 2});

        RenderSystem::createColoredMesh(resource, "mesh_flat_highlight");
    }
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);
    resource.texture.color = {0,0,1}; // Blue color for now to distinguish

    // Set Z Value
    if (ZValuesMap.count("Wall")) {
        motion.zValue = ZValuesMap["Wall"];
    } else {
        motion.zValue = 0.5f; // Default fallback
    }

    return entity;
}

void DestructibleWallSystem::breakWall(ECS::Entity wall_entity, vec2 impact_point) {
    if (!wall_entity.has<Motion>() || !wall_entity.has<DestructibleWall>()) return;

    // Copy data to local variables to avoid reference invalidation when creating new entities
    auto motion_component = wall_entity.get<Motion>();
    auto dw_component = wall_entity.get<DestructibleWall>();

    vec2 motion_position = motion_component.position;
    float motion_angle = motion_component.angle;
    vec2 motion_scale = motion_component.scale;
    int dw_debris_count = dw_component.debris_count;
    float dw_explosion_force = dw_component.explosion_force;

    // Calculate local impact point
    vec2 local_impact = impact_point - motion_position;
    float c = cos(-motion_angle);
    float s = sin(-motion_angle);
    vec2 local_p = { local_impact.x * c - local_impact.y * s, local_impact.x * s + local_impact.y * c };

    // Determine split axis (longest dimension)
    bool split_y = motion_scale.y > motion_scale.x;
    float total_len = split_y ? motion_scale.y : motion_scale.x;
    float impact_pos = split_y ? local_p.y : local_p.x;

    float gap_size = 150.f; // Configurable gap size
    float half_gap = gap_size / 2.f;

    // Check if we are close to the edge, if so, just shorten the wall
    // Bounds are [-total_len/2, total_len/2]
    float min_bound = -total_len / 2.f;
    float max_bound = total_len / 2.f;

    // Define the two new segments
    // Segment 1: [min_bound, impact_pos - half_gap]
    // Segment 2: [impact_pos + half_gap, max_bound]

    auto create_segment = [&](float start, float end) {
        if (end - start < 20.f) return; // Too small

        float new_len = end - start;
        float center_offset = start + new_len / 2.f;

        vec2 new_size = motion_scale;
        if (split_y) new_size.y = new_len;
        else new_size.x = new_len;

        // Transform center back to world space
        vec2 local_center = split_y ? vec2{0, center_offset} : vec2{center_offset, 0};
        float wc = cos(motion_angle);
        float ws = sin(motion_angle);
        vec2 world_center_offset = { local_center.x * wc - local_center.y * ws, local_center.x * ws + local_center.y * wc };
        vec2 new_pos = motion_position + world_center_offset;

        createDestructibleWall(new_pos, new_size, motion_angle);
    };

    create_segment(min_bound, impact_pos - half_gap);
    create_segment(impact_pos + half_gap, max_bound);

    // Generate Debris in the gap
    std::default_random_engine rng = std::default_random_engine(std::random_device()());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> size_dist(0.2f, 0.5f);

    for (int i = 0; i < dw_debris_count; ++i) {
        ECS::Entity debris = ECS::Entity();
        
        // Random position within the gap
        float gap_pos = (dist(rng) * half_gap) + impact_pos;
        vec2 local_debris_pos = split_y ? vec2{dist(rng) * motion_scale.x * 0.4f, gap_pos} : vec2{gap_pos, dist(rng) * motion_scale.y * 0.4f};
        
        // Rotate to world
        float wc = cos(motion_angle);
        float ws = sin(motion_angle);
        vec2 rotated_offset = { local_debris_pos.x * wc - local_debris_pos.y * ws, local_debris_pos.x * ws + local_debris_pos.y * wc };
        
        Motion& debris_motion = debris.insert(Motion());
        debris_motion.position = motion_position + rotated_offset;
        
        float thickness = std::min(motion_scale.x, motion_scale.y);
        float debris_size = thickness * (0.4f + 0.4f * std::abs(dist(rng))); // 0.4 to 0.8 of thickness
        debris_motion.scale = vec2{debris_size, debris_size};
        
        std::cout << "Debris created at: " << debris_motion.position.x << ", " << debris_motion.position.y 
                  << " Scale: " << debris_motion.scale.x << " Z: " << debris_motion.zValue << std::endl;

        debris_motion.angle = dist(rng) * 3.14f;
        if (ZValuesMap.count("Wall")) {
            debris_motion.zValue = ZValuesMap["Wall"];
        } else {
            debris_motion.zValue = 0.5f;
        }
        
        // Push away from impact point
        vec2 dir = debris_motion.position - impact_point;
        if (length(dir) < 0.1f) dir = {1.0f, 0.0f}; 
        dir = normalize(dir);
        
        debris_motion.velocity = dir * dw_explosion_force;
        
        // Physics
        PhysicsObject& debris_physics = debris.insert(PhysicsObject());
        debris_physics.object_type = MOVEABLEWALL; 
        debris_physics.mass = 10;
        
        // Debris Component
        Debris& d = debris.insert(Debris());
        d.collision_disable_timer = 500.f;
        d.life_time = 5000.f;
        
        // Render
        std::string key = "wall";
        ShadedMesh& resource = cache_resource(key);
        if (resource.mesh.vertices.empty())
        {
            resource = ShadedMesh();
            resource.mesh.vertices.emplace_back(ColoredVertex{vec3 {-0.5, 0.5, -0.02}, vec3{0.0,0.0,0.0}});
            resource.mesh.vertices.emplace_back(ColoredVertex{vec3{0.5, 0.5, -0.02}, vec3{0.0,0.0,0.0}});
            resource.mesh.vertices.emplace_back(ColoredVertex{vec3{0.5, -0.5, -0.02}, vec3{0.0,0.0,0.0}});
            resource.mesh.vertices.emplace_back(ColoredVertex{vec3{-0.5, -0.5, -0.02}, vec3{0.0,0.0,0.0}});

            resource.mesh.vertex_indices = std::vector<uint16_t>({0, 2, 1, 0, 3, 2});

            RenderSystem::createColoredMesh(resource, "mesh_flat_highlight");
        }
        ECS::registry<ShadedMeshRef>.emplace(debris, resource);
        resource.texture.color = {0,0,1};
    }

    // Destroy the old wall
    ECS::registry<DestructibleWall>.remove(wall_entity);
    ECS::registry<Wall>.remove(wall_entity);
    ECS::registry<PhysicsObject>.remove(wall_entity);
    ECS::registry<Motion>.remove(wall_entity);
    ECS::ContainerInterface::remove_all_components_of(wall_entity);
}

void DestructibleWallSystem::updateDebris(float elapsed_ms) {
    for (auto& entity : ECS::registry<Debris>.entities) {
        auto& debris = entity.get<Debris>();
        debris.life_time -= elapsed_ms;
        debris.collision_disable_timer -= elapsed_ms;

        if (entity.has<Motion>()) {
            auto& motion = entity.get<Motion>();
            motion.velocity *= 0.9f; // Strong decay
        }

        if (debris.collision_disable_timer <= 0) {
            if (entity.has<PhysicsObject>()) {
                entity.remove<PhysicsObject>();
            }
        }

        if (debris.life_time <= 0) {
            ECS::ContainerInterface::remove_all_components_of(entity);
        }
    }
}

void DestructibleWallSystem::onOverlap(ECS::Entity self, const ECS::Entity e, CollisionResult collision) {
    if (e.has<MagicParticle>()) {
        breakWall(self, collision.vertex);
        ECS::ContainerInterface::remove_all_components_of(e);
    }
}
