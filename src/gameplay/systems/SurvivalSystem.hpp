#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include <vector>
#include <unordered_map>
#include <random>

struct Chunk {
    ivec2 grid_pos;
    std::vector<ECS::Entity> entities;
};

class SurvivalSystem {
public:
    SurvivalSystem();
    ~SurvivalSystem();

    void init();
    void step(float elapsed_ms);
    void restart();

    // Configuration
    float wall_density = 0.3f; // Chance of a wall segment appearing

private:
    void updateChunks(vec2 player_pos);
    void generateChunk(ivec2 grid_pos);
    void unloadChunk(ivec2 grid_pos);
    
    // Helper to hash ivec2 for unordered_map
    struct IVec2Hash {
        std::size_t operator()(const ivec2& k) const {
            return std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1);
        }
    };

    // Helper for equality of ivec2
    struct IVec2Equal {
        bool operator()(const ivec2& lhs, const ivec2& rhs) const {
            return lhs.x == rhs.x && lhs.y == rhs.y;
        }
    };

    std::unordered_map<ivec2, Chunk, IVec2Hash, IVec2Equal> active_chunks;
    const float CHUNK_SIZE = 1000.f;
    const int RENDER_DISTANCE = 2; // Chunks radius
    
    std::default_random_engine rng;
};
