#include "DestructibleWall.hpp"
#include "Wall.hpp"
#include "render.hpp"
#include "MagicParticle.hpp"
#include <random>
#include <iostream>
#include <array>

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
    physics.attach(Hit, DestructibleWallSystem::wall_hit);

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

void DestructibleWallSystem::breakWall(ECS::Entity wall_entity, vec2 impact_point, vec2 impact_velocity) {
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

    // Generate Debris using Random Cut (Shatter) Algorithm
    struct Polygon {
        std::vector<vec2> vertices;
        
        float area() const {
            float a = 0.f;
            for (size_t i = 0; i < vertices.size(); ++i) {
                vec2 v1 = vertices[i];
                vec2 v2 = vertices[(i + 1) % vertices.size()];
                a += (v1.x * v2.y - v2.x * v1.y);
            }
            return std::abs(a) * 0.5f;
        }
        
        vec2 centroid() const {
            vec2 c = {0,0};
            for (auto& v : vertices) c += v;
            return c / (float)vertices.size();
        }
    };

    std::vector<Polygon> pieces;
    
    // Initial gap dimensions in local space
    float gap_w = split_y ? motion_scale.x : gap_size;
    float gap_h = split_y ? gap_size : motion_scale.y;
    
    vec2 p1 = {-gap_w / 2.f, -gap_h / 2.f};
    vec2 p2 = {gap_w / 2.f, -gap_h / 2.f};
    vec2 p3 = {gap_w / 2.f, gap_h / 2.f};
    vec2 p4 = {-gap_w / 2.f, gap_h / 2.f};

    pieces.push_back({ {p1, p2, p3, p4} });

    std::default_random_engine rng = std::default_random_engine(std::random_device()());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    // Split Loop
    int target_pieces = dw_debris_count;
    for (int i = 0; i < target_pieces - 1; ++i) {
        // Pick largest piece to split
        auto max_it = std::max_element(pieces.begin(), pieces.end(), 
            [](const Polygon& a, const Polygon& b) { return a.area() < b.area(); });
        
        if (max_it == pieces.end()) break;
        
        Polygon poly = *max_it;
        
        // Generate random cut line
        // Pick a point inside (centroid + jitter)
        vec2 center = poly.centroid();
        // Jitter relative to bounding box? Or just use centroid. 
        // Let's use centroid for stability, maybe slight jitter.
        
        float angle = dist(rng) * 3.14159f * 2.f;
        vec2 normal = {cos(angle), sin(angle)};
        
        // Split
        Polygon pos_poly, neg_poly;
        
        for (size_t j = 0; j < poly.vertices.size(); ++j) {
            vec2 v1 = poly.vertices[j];
            vec2 v2 = poly.vertices[(j + 1) % poly.vertices.size()];
            
            float d1 = dot(v1 - center, normal);
            float d2 = dot(v2 - center, normal);
            
            if (d1 >= 0) pos_poly.vertices.push_back(v1);
            if (d1 < 0) neg_poly.vertices.push_back(v1);
            
            // Check intersection
            if ((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) {
                float t = d1 / (d1 - d2);
                vec2 intersect = v1 + (v2 - v1) * t;
                pos_poly.vertices.push_back(intersect);
                neg_poly.vertices.push_back(intersect);
            }
        }
        
        if (pos_poly.vertices.size() >= 3 && neg_poly.vertices.size() >= 3) {
            *max_it = pos_poly;
            pieces.push_back(neg_poly);
        } else {
            // Failed to split (e.g. line didn't cross enough), try again or just skip
            // To avoid infinite loop if we can't split, just continue but don't increment i? 
            // Or just accept fewer pieces.
            // Let's just continue.
        }
    }

    // Create entities for each piece
    for (const auto& poly : pieces) {
        if (poly.vertices.size() < 3) continue;

        ECS::Entity debris = ECS::Entity();

        // Calculate Bounding Box
        float min_x = 1e9, max_x = -1e9, min_y = 1e9, max_y = -1e9;
        for (auto& v : poly.vertices) {
            min_x = std::min(min_x, v.x);
            max_x = std::max(max_x, v.x);
            min_y = std::min(min_y, v.y);
            max_y = std::max(max_y, v.y);
        }

        float width = max_x - min_x;
        float height = max_y - min_y;
        vec2 center = {min_x + width / 2.f, min_y + height / 2.f};

        // Transform center to world space
        vec2 gap_center_local;
        if (split_y) {
             gap_center_local = {0, impact_pos}; 
        } else {
             gap_center_local = {impact_pos, 0}; 
        }

        vec2 poly_center_wall_local = gap_center_local + center;

        float wc = cos(motion_angle);
        float ws = sin(motion_angle);
        vec2 world_offset = { 
            poly_center_wall_local.x * wc - poly_center_wall_local.y * ws, 
            poly_center_wall_local.x * ws + poly_center_wall_local.y * wc 
        };
        vec2 world_pos = motion_position + world_offset;

        // Create Motion
        Motion& debris_motion = debris.insert(Motion());
        debris_motion.position = world_pos;
        debris_motion.angle = motion_angle; 
        debris_motion.scale = {width, height};
        
        if (ZValuesMap.count("Wall")) {
            debris_motion.zValue = ZValuesMap["Wall"];
        } else {
            debris_motion.zValue = 0.5f;
        }

        // Velocity
        vec2 dir = world_pos - impact_point;
        float dist = length(dir);
        if (length(dir) < 0.1f) dir = impact_velocity;
        dir = normalize(dir);
        // std::cout << "dir: " << dir.x << ", " << dir.y << std::endl;
        
        // Bias strongly towards impact velocity
        // Radial component reduced to 20%, Impact component increased to 80%
        debris_motion.velocity = dir * (dw_explosion_force * dist / 100.f * 0.8f) + impact_velocity * 0.5f;

        // Physics
        PhysicsObject& debris_physics = debris.insert(PhysicsObject());
        debris_physics.object_type = MOVEABLEWALL; 
        debris_physics.mass = 10;
        debris_physics.vertex.clear();
        debris_physics.faces.clear();
        debris_physics.attach(Hit, DestructibleWallSystem::wall_hit);

        // Use real polygon vertices for physics
        for (size_t i = 0; i < poly.vertices.size(); ++i) {
            vec2 v = poly.vertices[i];
            vec2 norm_v = { (v.x - center.x) / width, (v.y - center.y) / height };
            debris_physics.vertex.push_back(PhysicsVertex{{norm_v.x, norm_v.y, -0.02}});
            debris_physics.faces.push_back({(int)i, (int)((i + 1) % poly.vertices.size())});
        }

        // Create Custom Mesh on Heap
        ShadedMesh* resource = new ShadedMesh();
        if (width < 0.001f || height < 0.001f) {
            delete resource;
            continue;
        }

        // Normalize vertices to [-0.5, 0.5] relative to the bounding box center
        // And create triangle fan indices
        // Center of fan can be the first vertex, or the centroid.
        // Convex polygon: Fan from vertex 0 covers it.
        
        for (const auto& v : poly.vertices) {
            vec2 norm_v = { (v.x - center.x) / width, (v.y - center.y) / height };
            resource->mesh.vertices.emplace_back(ColoredVertex{vec3{norm_v.x, norm_v.y, -0.02}, vec3{0.0,0.0,0.0}});
        }
        
        // Indices: 0, 1, 2; 0, 2, 3; ... 0, N-2, N-1
        for (size_t k = 1; k < poly.vertices.size() - 1; ++k) {
            resource->mesh.vertex_indices.push_back(0);
            resource->mesh.vertex_indices.push_back(k);
            resource->mesh.vertex_indices.push_back(k + 1);
        }

        RenderSystem::createColoredMesh(*resource, "mesh_flat_highlight");
        resource->texture.color = {0, 0, 1}; // Blue

        ECS::registry<ShadedMeshRef>.emplace(debris, *resource);

        // Debris Component
        Debris& d = debris.insert(Debris());
        d.collision_disable_timer = 500.f;
        d.life_time = 5000.f;
        d.custom_mesh = resource; 
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
        // debris.life_time -= elapsed_ms;
        // debris.collision_disable_timer -= elapsed_ms;

        if (entity.has<Motion>()) {
            auto& motion = entity.get<Motion>();
            // motion.velocity *= 0.99f; // Strong decay
        }

        if (debris.collision_disable_timer <= 0) {
            if (entity.has<PhysicsObject>()) {
                entity.remove<PhysicsObject>();
            }
        }

        if (debris.life_time <= 0) {
            if (debris.custom_mesh) {
                delete debris.custom_mesh;
                debris.custom_mesh = nullptr;
            }
            ECS::ContainerInterface::remove_all_components_of(entity);
        }
    }
}

void DestructibleWallSystem::onOverlap(ECS::Entity self, const ECS::Entity e, CollisionResult collision) {
    if (e.has<MagicParticle>()) {
        vec2 impact_vel = {0,0};
        if (e.has<Motion>()) {
            impact_vel = PhysicsSystem::get_world_velocity(e.get<Motion>());
        }
        breakWall(self, collision.vertex, impact_vel);
        ECS::ContainerInterface::remove_all_components_of(e);
    }
}

void DestructibleWallSystem::wall_hit(ECS::Entity self, ECS::Entity e, CollisionResult collision) {
    PhysicsObject::handle_collision(self, e, collision);
}
