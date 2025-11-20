#include "Building.hpp"
#include "Roof.hpp"
#include "Wall.hpp"
#include "render.hpp"

ECS::Entity BuildingSystem::createBuilding(vec2 position, vec2 size, float rotation) {
    ECS::Entity entity = ECS::Entity();

    // Create Motion (this is the center reference point)
    Motion& motion = entity.insert(Motion());
    motion.position = position;
    motion.angle = rotation;
    motion.scale = size;

    // Add Building component
    Building& building = entity.emplace<Building>();

    // Set Z Value for visualization/debugging
    if (ZValuesMap.count("Wall")) {
        motion.zValue = ZValuesMap["Wall"];
    } else {
        motion.zValue = 0.5f;
    }

    // Instead of a solid rectangle, create 3 walls forming a U-shape with entrance at bottom
    float wall_thickness = 20.f;
    float half_width = size.x / 2.f;
    float half_height = size.y / 2.f;
    float entrance_width = size.x * 0.4f; // Entrance is 40% of building width
    
    // Top wall (full width)
    vec2 top_pos = position + vec2{0, half_height};
    Wall::createWall(top_pos, vec2{size.x, wall_thickness}, rotation,
        [](ECS::Entity, const ECS::Entity, CollisionResult) {},
        Wall::wall_hit);
    
    // Left wall (full height)
    vec2 left_pos = position + vec2{-half_width, 0};
    Wall::createWall(left_pos, vec2{wall_thickness, size.y}, rotation,
        [](ECS::Entity, const ECS::Entity, CollisionResult) {},
        Wall::wall_hit);
    
    // Right wall (full height)
    vec2 right_pos = position + vec2{half_width, 0};
    Wall::createWall(right_pos, vec2{wall_thickness, size.y}, rotation,
        [](ECS::Entity, const ECS::Entity, CollisionResult) {},
        Wall::wall_hit);
    
    // Bottom wall segments (with gap for entrance in the middle)
    float side_segment_width = (size.x - entrance_width) / 2.f;
    
    // Bottom-left segment
    vec2 bottom_left_pos = position + vec2{-half_width + side_segment_width / 2.f, -half_height};
    Wall::createWall(bottom_left_pos, vec2{side_segment_width, wall_thickness}, rotation,
        [](ECS::Entity, const ECS::Entity, CollisionResult) {},
        Wall::wall_hit);
    
    // Bottom-right segment
    vec2 bottom_right_pos = position + vec2{half_width - side_segment_width / 2.f, -half_height};
    Wall::createWall(bottom_right_pos, vec2{side_segment_width, wall_thickness}, rotation,
        [](ECS::Entity, const ECS::Entity, CollisionResult) {},
        Wall::wall_hit);

    // Create the roof (covers entire building area including entrance)
    ECS::Entity roof = RoofSystem::createRoof(position, size, rotation, entity);
    building.roof = roof;

    return entity;
}
