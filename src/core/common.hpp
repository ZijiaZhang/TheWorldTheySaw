#pragma once

// stlib
#include <string>
#include <tuple>
#include <vector>
#include <stdexcept>
#include <map>
#include <set>
#include <array>
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

// Draw-order lookup keyed by a small set of generic layer names. Replace or
// extend with the layers your game needs; entities default to zValue 0.
static std::map<std::string, int> ZValuesMap = {
    {"Background", 0},
    {"Wall", 6},
    {"Particle", 12},
    {"Button", 15},
    {"UI", 20},
};

// Generic collision layers. Add your own and update PhysicsObject::getCollisionType
// to control which pairs collide, overlap, or ignore one another.
typedef enum
{
    COLLISION_DEFAULT,
    WALL,
    MOVEABLEWALL,
    BUTTON,
    LAST

} CollisionObjectType;

struct PhysicsVertex
{
    vec3 position;
};

inline float cross(vec2 x, vec2 y){
    return (x.x * y.y - y.x* x.y);
}


Transform getTransform(const Motion &m1);
