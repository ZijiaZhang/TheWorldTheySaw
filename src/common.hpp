#pragma once

// stlib
#include <string>
#include <tuple>
#include <vector>
#include <stdexcept>
#include <map>
#include <set>
// glfw (OpenGL)
#define NOMINMAX
#include <gl3w.h>
#include <GLFW/glfw3.h>

// The glm library provides vector and matrix operations as in GLSL
#include <glm/vec2.hpp>				// vec2
#include <glm/ext/vector_int2.hpp>  // ivec2
#include <glm/vec3.hpp>             // vec3
#include <glm/mat3x3.hpp>           // mat3
#include "tiny_ecs.hpp"

using namespace glm;
static const float PI = 3.14159265359f;

// Simple utility functions to avoid mistyping directory name
inline std::string data_path() { return "data"; };
inline std::string shader_path(const std::string& name) { return data_path() + "/shaders/" + name;};
inline std::string textures_path(const std::string& name) { return data_path() + "/textures/" + name; };
inline std::string audio_path(const std::string& name) { return data_path() + "/audio/" + name; };
inline std::string mesh_path(const std::string& name) { return data_path() + "/meshes/" + name; };
inline std::string level_path(const std::string& name) { return data_path() + "/levels/" + name; };

// The 'Transform' component handles transformations passed to the Vertex shader
// (similar to the gl Immediate mode equivalent, e.g., glTranslate()...)
struct Transform {
	mat3 mat = { { 1.f, 0.f, 0.f }, { 0.f, 1.f, 0.f}, { 0.f, 0.f, 1.f} }; // start with the identity
	void scale(vec2 scale);
	void rotate(float radians);
	void translate(vec2 offset);
};

// All data relevant to the shape and motion of entities
struct Motion {
	vec2 position = { 0, 0 };
	float angle = 0;
	vec2 velocity = { 0, 0 };
    vec2 preserve_world_velocity = {0,0};
    float angular_velocity = 0.f;
	vec2 scale = { 10, 10 };
    int zValue = 0;

	// Max speed on one axis
	float max_control_speed = 100;

	// If object is bind to parent, Will not handle collision if bind to parents
	bool has_parent = false;
	ECS::Entity parent;
	// Offset will be relative to parents
	vec2 offset = {0.f,0.f};
	float offset_angle = 0.f;
    vec2 offset_move = {0.f,0.f};
};

struct Health {
    float shield = 0.f;
    float hp = 0.f;
    float max_hp = 0.f;
    vec2 health_bar_offset = {0,-50};
};

static std::unordered_map<std::string, int> level_progression = {
    {"level_1", 1},
    {"level_2", 0},
    {"level_3", 0},
    {"level_4", 0},
    {"level_5", 0},
    {"level_6", 0},
    {"level_7", 0},
    {"level_8", 0},
    {"level_9", 0},
    {"level_10", 0}
};

// For the order of drawing
static std::map<std::string, int> ZValuesMap = {
        {"Start", 6},
        {"MagicParticle", 12},
    {"Weapon", 11},
    {"Soldier", 10},
    {"Turtle", 9},
    {"Shield",20},
    {"Fish", 8},
    {"Enemy", 7},
    {"Wall",6},
    {"Background", 5}

};

typedef enum
{
    COLLISION_DEFAULT,
    PLAYER,
    ENEMY,
    BULLET,
    WALL,
    MOVEABLEWALL,
    WEAPON,
    BUTTON,
    SHIELD,
    EXPLOSION,
    MAGIC,
    LAST

} CollisionObjectType;

typedef enum{
    FIREBALL
}MagicWeapon;


struct PhysicsVertex
{
    vec3 position;
};

struct Path_with_heuristics{
    std::vector<std::pair<int,int>> path;
    float cost;
    float heuristic;
};

struct AIPath{
    bool active = true;
    Path_with_heuristics path;
    vec2 desired_speed = {0.f, 0.f};
    int progress = 0;
};


inline float cross(vec2 x, vec2 y){
    return (x.x * y.y - y.x* x.y);
}


Transform getTransform(const Motion &m1);

