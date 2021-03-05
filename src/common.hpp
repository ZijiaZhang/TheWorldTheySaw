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
};

// For the order of drawing
static std::map<std::string, int> ZValuesMap = {
        {"Start", 6},
    {"Soldier", 10},
    {"Turtle", 9},
    {"Shield",8},
    {"Fish", 8},
    {"Enemy", 7},
    {"Wall",6},
    {"Background", 5}

};

typedef enum
{
    DEFAULT,
    PLAYER,
    ENEMY,
    BULLET,
    WALL,
    MOVEABLEWALL,
    WEAPON,
    BUTTON,
    LAST

} CollisionObjectType;

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
    Path_with_heuristics path;
    vec2 desired_speed = {0.f, 0.f};
};


Transform getTransform(const Motion &m1);