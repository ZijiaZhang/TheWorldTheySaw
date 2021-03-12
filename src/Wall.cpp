//
// Created by Gary on 1/19/2021.
//

#include "Wall.hpp"

#include <utility>
#include "render.hpp"
#include "PhysicsObject.hpp"
#include "MoveableWall.hpp"
#include "Bullet.hpp"

ECS::Entity Wall::createWall(vec2 location, vec2 size, float rotation,
                             COLLISION_HANDLER overlap,
                             COLLISION_HANDLER hit){
    auto entity = ECS::Entity();

    std::string key = "wall";
    ShadedMesh& resource = cache_resource(key);
    if (resource.mesh.vertices.empty())
    {
        resource = ShadedMesh();
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3 {-0.5, 0.5, -0.02}, vec3{0.0,1.0,0.0}});
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3{0.5, 0.5, -0.02}, vec3{0.0,1.0,0.0}});
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3{0.5, -0.5, -0.02}, vec3{0.0,1.0,0.0}});
        resource.mesh.vertices.emplace_back(ColoredVertex{vec3{-0.5, -0.5, -0.02}, vec3{0.0,1.0,0.0}});

        resource.mesh.vertex_indices = std::vector<uint16_t>({0, 2, 1, 0, 3, 2});

        RenderSystem::createColoredMesh(resource, "salmon");
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource);

    // Setting initial motion values
    Motion& motion = ECS::registry<Motion>.emplace(entity);
    motion.position = location;
    motion.angle = rotation;
    motion.velocity = { 0.f, 0.f };
    motion.scale = size;
    motion.zValue = ZValuesMap["Wall"];

    PhysicsObject physicsObject;
    physicsObject.object_type = WALL;
    physicsObject.fixed = true;
    physicsObject.attach(Overlap, overlap);
    physicsObject.attach(Hit, hit);
    ECS::registry<PhysicsObject>.insert(entity, physicsObject);

    //motion.box = size;
    //motion.mass = 1000;
    // Create and (empty) Salmon component to be able to refer to all turtles
    ECS::registry<Wall>.emplace(entity);
    resource.texture.color = {1,1,1};
    return entity;
}

IntersectionResult find_intersection(vec2 position, vec2 vector, vec2 position2, vec2 vector2) {
    float v1 = cross((position2 - position), vector2)/(cross(vector, vector2));
    float v2 = cross((position - position2), vector)/(cross(vector2, vector));
    // printf("%f, %f, <%f, %f>,< %f, %f>, %f, %f\n", v1, v2, vector.x, vector.y, vector2.x, vector2.y, cross(vector, vector2), cross(vector2, vector));
    if ((cross(vector, vector2) > -0.1 && cross(vector, vector2) < 0.1)  || v1 <= 0 || v1 >= 1 || v2 <= 0 || v2 >= 1){
        return IntersectionResult{false};
    }
    IntersectionResult result{};
    result.has_intersect = true;
    result.intersect_point = position2 + v2 * vector2;
    return result;
}

void Wall::wall_hit(ECS::Entity self, const ECS::Entity e, CollisionResult collision) {
    Force force = PhysicsObject::handle_collision(self, e, collision);
    if(self.has<DeathTimer>() || e.has<Wall>() || e.has<MoveableWall>() || !e.has<Bullet>()){
        return;
    }
//    printf("%f\n", dot(force.force, force.force));
    if(dot(force.force, force.force) == 0 ){
        return;
    }
    auto& physics = self.get<PhysicsObject>();
    auto& mesh = self.get<ShadedMeshRef>();
    int first_intersect_index = -1;
    vec2 first_intersect = vec2{};
    vec2 first_dir = vec2{};
    auto& motion = self.get<Motion>();
    Transform t1 = getTransform(motion);
    for(auto edge : physics.faces) {
        vec2 start =  t1.mat * vec3{physics.vertex[edge.first].position.x, physics.vertex[edge.first].position.y, 1};
        vec2 end = t1.mat * vec3{physics.vertex[edge.second].position.x, physics.vertex[edge.second].position.y, 1};
        IntersectionResult r = find_intersection(force.position + 3.f * normalize(force.force), -normalize(force.force) * 5.f, start, end - start);
        if(r.has_intersect){
            first_intersect_index = edge.first;
            first_intersect = inverse(t1.mat) * vec3{r.intersect_point.x, r.intersect_point.y, 1};
            first_dir = normalize(end - start);
            break;
        }
    }
    int second_intersect_index = -1;
    vec2 second_intersect = vec2{};
    vec2 second_dir = vec2{};
    for(auto edge : physics.faces){
        vec2 start =  t1.mat * vec3{physics.vertex[edge.first].position.x, physics.vertex[edge.first].position.y, 1};
        vec2 end = t1.mat * vec3{physics.vertex[edge.second].position.x, physics.vertex[edge.second].position.y, 1};
        // printf("%f,%f\n", start.x, start.y);

        IntersectionResult r = find_intersection(force.position + 3.f * normalize(force.force), normalize(force.force) * 1000.f, start, end - start);
        if(r.has_intersect){
            second_intersect_index = edge.first;
            second_intersect = inverse(t1.mat) * vec3{r.intersect_point.x, r.intersect_point.y, 1};
            vec2 second_dir = normalize(end - start);
            break;
        }
    }
//    printf("%f,%f\n", first_intersect.x, first_intersect.y);
//    printf("%f,%f\n", second_intersect.x, second_intersect.y);

    if (second_intersect_index == first_intersect_index){
        printf("%d\n",second_intersect_index);
        return;
    }
    if(second_intersect_index < first_intersect_index){
        std::swap(first_intersect_index, second_intersect_index);
        std::swap(first_intersect, second_intersect);
        std::swap(first_dir, second_dir);
    }

    if(first_intersect_index!=-1 && second_intersect_index!=-1){
        // printf("%d,%d\n", first_intersect_index, second_intersect_index);

        self.emplace<DeathTimer>();

        std::vector<ColoredVertex> first;
        std::vector<ColoredVertex> second;

        for (int x =0; x <= first_intersect_index; x++ ){
            first.push_back(mesh.reference_to_cache->mesh.vertices[x]);
        }
        first.push_back(ColoredVertex{vec3{first_intersect.x - 0.01f * first_dir.x, first_intersect.y - 0.01f * first_dir.y, mesh.reference_to_cache->mesh.vertices[0].position.z},
                                      mesh.reference_to_cache->mesh.vertices[0].color});
        first.push_back(ColoredVertex{vec3{second_intersect.x + 0.02f * second_dir.x, second_intersect.y + 0.02f * second_dir.y , mesh.reference_to_cache->mesh.vertices[0].position.z},
                                      mesh.reference_to_cache->mesh.vertices[0].color});
        second.push_back(ColoredVertex{vec3{first_intersect.x + 0.01f * first_dir.x, first_intersect.y + 0.01f * first_dir.y, mesh.reference_to_cache->mesh.vertices[0].position.z},
                                       mesh.reference_to_cache->mesh.vertices[0].color});
        for (int x = first_intersect_index + 1; x <= second_intersect_index; x++ ){
            second.push_back(mesh.reference_to_cache->mesh.vertices[x]);
        }

        for (int x =second_intersect_index + 1; x <= mesh.reference_to_cache->mesh.vertices.size() - 1; x++ ){
            first.push_back(mesh.reference_to_cache->mesh.vertices[x]);
        }
        second.push_back(ColoredVertex{vec3{second_intersect.x - 0.01f * second_dir.x, second_intersect.y - 0.01f * second_dir.y , mesh.reference_to_cache->mesh.vertices[0].position.z},
                                       mesh.reference_to_cache->mesh.vertices[0].color});
        // printf("first len: %d\n", first.size());
        auto ov = physics.collisionHandler[Overlap];
        auto hit = physics.collisionHandler[Hit];
        ECS::Entity e1 = MoveableWall::createCustomMoveableWall(motion.position, motion.scale, first, motion.preserve_world_velocity, motion.angle,
                                                                ov, wall_hit);

        // The vector may resize, re-obtain reference here.
        auto& motion_a = self.get<Motion>();
        ECS::Entity e2 = MoveableWall::createCustomMoveableWall(motion_a.position, motion_a.scale, second, motion_a.preserve_world_velocity, motion_a.angle,
                                                                ov, wall_hit);
        force.force /= 2.f;

        e1.get<PhysicsObject>().add_force(force);
        e2.get<PhysicsObject>().add_force(force);
        motion.position = {10000,10000};
    } else {
        printf("%d, %d\n", first_intersect_index, second_intersect_index);
        return;
    }
}
