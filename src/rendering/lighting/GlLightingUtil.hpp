#pragma once
//
// Small OpenGL helpers shared by the deferred lighting passes: texture/render-
// target creation, compute & fullscreen program loading, and a fullscreen-
// triangle draw. Kept separate from the engine's Effect/ShadedMesh so the
// lighting code can use 4.3 compute shaders and image load/store freely.
//
#include "common.hpp"
#include <string>

namespace lgl {

// Create an immutable-storage-free 2D texture with the given sized internal
// format (e.g. GL_RGBA8, GL_R16F, GL_RGBA16F, GL_RG32F). filter is GL_NEAREST or
// GL_LINEAR; wrap is e.g. GL_CLAMP_TO_EDGE. The texture is left bound to unit 0.
GLuint createTexture2D(int w, int h, GLenum internalFormat,
                       GLenum filter = GL_NEAREST, GLenum wrap = GL_CLAMP_TO_EDGE);

// Re-allocate the storage of an existing texture (used on window resize).
void resizeTexture2D(GLuint tex, int w, int h, GLenum internalFormat);

// Read a whole text file (throws std::runtime_error if missing).
std::string readTextFile(const std::string& path);

// Compile + link a compute program from a single .glsl source file.
GLuint createComputeProgram(const std::string& path);

// Compile + link a vertex/fragment program from two source files.
GLuint createRenderProgram(const std::string& vsPath, const std::string& fsPath);

// An empty VAO so core-profile fullscreen draws are legal. Draw a fullscreen
// triangle with: glBindVertexArray(vao); glDrawArrays(GL_TRIANGLES, 0, 3);
GLuint createEmptyVAO();

// Convenience: bind program, bind the empty VAO, draw the fullscreen triangle.
void drawFullscreen(GLuint vao);

// Number of compute groups needed to cover `size` with `local` threads.
inline int groups(int size, int local) { return (size + local - 1) / local; }

} // namespace lgl
