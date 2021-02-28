#pragma once

// stlib
#include <string>
#include <tuple>
#include <vector>
#include <stdexcept>
#include <map>

// glfw (OpenGL)
#define NOMINMAX
#include <gl3w.h>
#include <GLFW/glfw3.h>

// The glm library provides vector and matrix operations as in GLSL
#include <glm/vec2.hpp>				// vec2
#include <glm/ext/vector_int2.hpp>  // ivec2
#include <glm/vec3.hpp>             // vec3
#include <glm/mat3x3.hpp>           // mat3

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
};

// For the order of drawing
static std::map<std::string, int> ZValuesMap = {
    {"Soldier", 10},
    {"Turtle", 9},
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
    WALL
} CollisionObjectType;

struct PhysicsVertex
{
    vec3 position;
};

struct PhysicsObject{
    // The convec bonding box of a object
    std::vector<PhysicsVertex> vertex = {PhysicsVertex{{-0.5, 0.5, -0.02}},
                                         PhysicsVertex{{0.5, 0.5, -0.02}},
                                         PhysicsVertex{{0.5, -0.5, -0.02}},
                                         PhysicsVertex{{-0.5, -0.5, -0.02}}};

    // The edges of connecting the vertex tha forms a bonding box
    std::vector<std::pair<int,int>> faces = {{0,1}, {1,2 },{2,3 },{3,0 }};
    // The mass of the object
    float mass = 10;
    // Is object fixed in a location
    bool fixed = false;
    // If collision is enabled
    bool collide = true;
    // Object type
    CollisionObjectType object_type = DEFAULT;
    // Which object type is ignored
    std::vector<CollisionObjectType> ignore_collision_of_type;

};
