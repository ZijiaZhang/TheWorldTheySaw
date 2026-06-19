#include "GlLightingUtil.hpp"
#include "render.hpp" // gl_has_errors

#include <fstream>
#include <sstream>
#include <vector>
#include <stdexcept>

namespace lgl {

static GLenum baseFormatFor(GLenum internalFormat) {
    switch (internalFormat) {
        case GL_R8: case GL_R16F: case GL_R32F:           return GL_RED;
        case GL_RG8: case GL_RG16F: case GL_RG32F:        return GL_RG;
        case GL_RGB8: case GL_RGB16F: case GL_RGB32F:     return GL_RGB;
        case GL_RGBA8: case GL_RGBA16F: case GL_RGBA32F:  return GL_RGBA;
        case GL_DEPTH_COMPONENT24:                        return GL_DEPTH_COMPONENT;
        default:                                          return GL_RGBA;
    }
}

static GLenum typeFor(GLenum internalFormat) {
    switch (internalFormat) {
        case GL_R8: case GL_RG8: case GL_RGB8: case GL_RGBA8: return GL_UNSIGNED_BYTE;
        case GL_DEPTH_COMPONENT24:                            return GL_UNSIGNED_INT;
        default:                                              return GL_FLOAT;
    }
}

GLuint createTexture2D(int w, int h, GLenum internalFormat, GLenum filter, GLenum wrap) {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0,
                 baseFormatFor(internalFormat), typeFor(internalFormat), nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    gl_has_errors();
    return tex;
}

void resizeTexture2D(GLuint tex, int w, int h, GLenum internalFormat) {
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0,
                 baseFormatFor(internalFormat), typeFor(internalFormat), nullptr);
    gl_has_errors();
}

std::string readTextFile(const std::string& path) {
    std::ifstream is(path);
    if (!is.good())
        throw std::runtime_error("Lighting: failed to open shader file " + path);
    std::stringstream ss;
    ss << is.rdbuf();
    return ss.str();
}

static GLuint compileShader(GLenum stage, const std::string& src, const std::string& label) {
    GLuint sh = glCreateShader(stage);
    const char* c = src.c_str();
    GLint len = (GLint)src.size();
    glShaderSource(sh, 1, &c, &len);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint logLen = 0;
        glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(logLen > 1 ? logLen : 1);
        glGetShaderInfoLog(sh, logLen, &logLen, log.data());
        glDeleteShader(sh);
        throw std::runtime_error("Lighting GLSL compile error in " + label + ":\n" + std::string(log.data()));
    }
    return sh;
}

static void linkProgram(GLuint program, const std::string& label) {
    glLinkProgram(program);
    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint logLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(logLen > 1 ? logLen : 1);
        glGetProgramInfoLog(program, logLen, &logLen, log.data());
        throw std::runtime_error("Lighting GLSL link error in " + label + ":\n" + std::string(log.data()));
    }
}

GLuint createComputeProgram(const std::string& path) {
    std::string src = readTextFile(path);
    GLuint cs = compileShader(GL_COMPUTE_SHADER, src, path);
    GLuint program = glCreateProgram();
    glAttachShader(program, cs);
    linkProgram(program, path);
    glDeleteShader(cs);
    gl_has_errors();
    return program;
}

GLuint createRenderProgram(const std::string& vsPath, const std::string& fsPath) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, readTextFile(vsPath), vsPath);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, readTextFile(fsPath), fsPath);
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    linkProgram(program, vsPath + " / " + fsPath);
    glDeleteShader(vs);
    glDeleteShader(fs);
    gl_has_errors();
    return program;
}

GLuint createEmptyVAO() {
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    return vao;
}

void drawFullscreen(GLuint vao) {
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

} // namespace lgl
