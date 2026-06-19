#pragma once
//
// Tiny procedural texture generators so the lighting demo is self-contained (no
// binary art in the repo). Each returns an OpenGL texture id. The engine ships
// no sprites, but the design only needs albedo / normal / height / emissive maps
// — exactly what these synthesize.
//
#include "common.hpp"

namespace proctex {

GLuint solidAlbedo(vec3 color);

// Checkerboard albedo with a faint per-tile normal bump (floors).
GLuint checkerAlbedo(int size, vec3 a, vec3 b, int cells);
GLuint gentleNormal(int size, int cells, float strength);

// Brick albedo + matching raised-mortar normal (walls).
GLuint brickAlbedo(int w, int h);
GLuint brickNormal(int w, int h, float strength);

// Vertical 0..1 gradient used as a wall's height channel (low at the base).
GLuint verticalGradientHeight(int w, int h);

// A rounded "crate/boulder" prop: dome height + outward normals.
GLuint domeHeight(int size);
GLuint domeNormal(int size, float strength);

// Soft radial glow (emissive lamp).
GLuint radialGlow(int size, vec3 color);

} // namespace proctex
