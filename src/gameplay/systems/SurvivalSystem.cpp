#include "SurvivalSystem.hpp"
#include "Wall.hpp"
#include "DestructibleWall.hpp"
#include "soldier.hpp"
#include "Enemy.hpp"
#include <iostream>

SurvivalSystem::SurvivalSystem() {
    rng = std::default_random_engine(std::random_device()());
}

SurvivalSystem::~SurvivalSystem() {
    active_chunks.clear();
}

void SurvivalSystem::init() {
    restart();
}

void SurvivalSystem::restart() {
    active_chunks.clear();
    // Initial generation will happen in step() when player is detected
}

void SurvivalSystem::step(float elapsed_ms) {
    // Update Debris
    DestructibleWallSystem::updateDebris(elapsed_ms);

    // Find player
    ECS::Entity player;
    bool player_found = false;
    for (auto& entity : ECS::registry<Soldier>.entities) {
        if (entity.has<Motion>() && !entity.has<AIPath>()) { 
             if (!entity.has<Enemy>()) {
                 player = entity;
                 player_found = true;
                 break;
             }
        }
    }

    if (player_found) {
        vec2 player_pos = ECS::registry<Motion>.get(player).position;
        updateChunks(player_pos);
    }
}

void SurvivalSystem::updateChunks(vec2 player_pos) {
    ivec2 current_chunk_pos = {
        static_cast<int>(floor(player_pos.x / CHUNK_SIZE)),
        static_cast<int>(floor(player_pos.y / CHUNK_SIZE))
    };

    // Identify chunks to keep
    std::vector<ivec2> chunks_to_load;
    for (int x = -RENDER_DISTANCE; x <= RENDER_DISTANCE; ++x) {
        for (int y = -RENDER_DISTANCE; y <= RENDER_DISTANCE; ++y) {
            chunks_to_load.push_back({current_chunk_pos.x + x, current_chunk_pos.y + y});
        }
    }

    // Unload far chunks
    auto it = active_chunks.begin();
    while (it != active_chunks.end()) {
        bool keep = false;
        for (const auto& pos : chunks_to_load) {
            if (it->first.x == pos.x && it->first.y == pos.y) {
                keep = true;
                break;
            }
        }
        if (!keep) {
            unloadChunk(it->first);
            it = active_chunks.erase(it);
        } else {
            ++it;
        }
    }

    // Load new chunks
    for (const auto& pos : chunks_to_load) {
        if (active_chunks.find(pos) == active_chunks.end()) {
            generateChunk(pos);
        }
    }
}

void SurvivalSystem::generateChunk(ivec2 grid_pos) {
    Chunk new_chunk;
    new_chunk.grid_pos = grid_pos;

    // Seed based on position for deterministic generation per chunk
    std::mt19937 chunk_rng(grid_pos.x * 73856093 ^ grid_pos.y * 19349663);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::uniform_real_distribution<float> pos_dist(0.0f, CHUNK_SIZE);
    std::uniform_real_distribution<float> scale_dist(50.f, 200.f);
    std::uniform_real_distribution<float> angle_dist(0.f, 6.28f);

    vec2 chunk_origin = {grid_pos.x * CHUNK_SIZE, grid_pos.y * CHUNK_SIZE};

    // Generate Walls
    int num_walls = static_cast<int>(dist(chunk_rng) * 10 + 5); // 5 to 15 walls per chunk
    for (int i = 0; i < num_walls; ++i) {
        vec2 local_pos = {pos_dist(chunk_rng), pos_dist(chunk_rng)};
        vec2 world_pos = chunk_origin + local_pos;
        vec2 size = {scale_dist(chunk_rng), 20.f}; // Thin walls
        float angle = angle_dist(chunk_rng);

        // Avoid spawning on top of player (simple check)
        if (grid_pos.x == 0 && grid_pos.y == 0 && length(world_pos) < 200.f) {
            continue; 
        }

        // 50% chance to be destructible
        ECS::Entity wall;
        if (dist(chunk_rng) > 0.5f) {
             wall = DestructibleWallSystem::createDestructibleWall(world_pos, size, angle);
        } else {
             wall = Wall::createWall(world_pos, size, angle, 
                [](ECS::Entity, const ECS::Entity, CollisionResult) {}, 
                Wall::wall_hit);
        }
        
        new_chunk.entities.push_back(wall);
    }

    active_chunks[grid_pos] = new_chunk;
}

void SurvivalSystem::unloadChunk(ivec2 grid_pos) {
    if (active_chunks.find(grid_pos) != active_chunks.end()) {
        for (auto& entity : active_chunks[grid_pos].entities) {
            ECS::ContainerInterface::remove_all_components_of(entity);
        }
    }
}
