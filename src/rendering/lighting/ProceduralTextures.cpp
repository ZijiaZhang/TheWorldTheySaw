#include "ProceduralTextures.hpp"
#include "render.hpp" // gl_has_errors

#include <vector>
#include <cmath>
#include <algorithm>
#include <functional>

namespace proctex {
namespace {

GLuint upload(int w, int h, const std::vector<unsigned char>& rgba, GLenum filter, GLenum wrap) {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    gl_has_errors();
    return tex;
}

unsigned char toByte(float v) {
    return (unsigned char)std::min(255.f, std::max(0.f, v * 255.f + 0.5f));
}

// Encode a world/tangent normal into an RGBA8 texel.
void writeNormal(std::vector<unsigned char>& d, int i, vec3 n) {
    n = normalize(n);
    d[i + 0] = toByte(n.x * 0.5f + 0.5f);
    d[i + 1] = toByte(n.y * 0.5f + 0.5f);
    d[i + 2] = toByte(n.z * 0.5f + 0.5f);
    d[i + 3] = 255;
}

// Build a normal map from a height function via central differences.
GLuint normalFromHeight(int w, int h, float strength, GLenum wrap,
                        const std::function<float(int, int)>& height) {
    std::vector<unsigned char> d(w * h * 4);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float hl = height((x - 1 + w) % w, y);
            float hr = height((x + 1) % w, y);
            float hd = height(x, (y - 1 + h) % h);
            float hu = height(x, (y + 1) % h);
            vec3 n = vec3((hl - hr) * strength, (hd - hu) * strength, 1.0f);
            writeNormal(d, (y * w + x) * 4, n);
        }
    }
    return upload(w, h, d, GL_LINEAR, wrap);
}

} // namespace

GLuint solidAlbedo(vec3 color) {
    std::vector<unsigned char> d(4);
    d[0] = toByte(color.x); d[1] = toByte(color.y); d[2] = toByte(color.z); d[3] = 255;
    return upload(1, 1, d, GL_NEAREST, GL_REPEAT);
}

GLuint checkerAlbedo(int size, vec3 a, vec3 b, int cells) {
    std::vector<unsigned char> d(size * size * 4);
    int cs = std::max(1, size / cells);
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x) {
            bool on = ((x / cs) + (y / cs)) & 1;
            vec3 c = on ? a : b;
            // subtle grout lines
            bool line = (x % cs < 1) || (y % cs < 1);
            if (line) c *= 0.7f;
            int i = (y * size + x) * 4;
            d[i + 0] = toByte(c.x); d[i + 1] = toByte(c.y); d[i + 2] = toByte(c.z); d[i + 3] = 255;
        }
    return upload(size, size, d, GL_LINEAR, GL_REPEAT);
}

GLuint gentleNormal(int size, int cells, float strength) {
    int cs = std::max(1, size / cells);
    auto height = [=](int x, int y) -> float {
        // small dome per tile so floor tiles catch grazing light
        float fx = (float)(x % cs) / cs * 2.f - 1.f;
        float fy = (float)(y % cs) / cs * 2.f - 1.f;
        return 1.0f - std::min(1.0f, fx * fx + fy * fy);
    };
    return normalFromHeight(size, size, strength, GL_REPEAT, height);
}

namespace {
// Shared brick layout helpers.
bool inMortar(int x, int y, int w, int h, int& localY, int rowH, int brickW) {
    localY = y % rowH;
    int row = y / rowH;
    int offset = (row & 1) ? brickW / 2 : 0;
    int bx = (x + offset) % brickW;
    bool mortar = (localY < 2) || (bx < 2);
    (void)w; (void)h;
    return mortar;
}
} // namespace

GLuint brickAlbedo(int w, int h) {
    std::vector<unsigned char> d(w * h * 4);
    int rowH = std::max(4, h / 6);
    int brickW = std::max(6, w / 4);
    vec3 brick = {0.55f, 0.28f, 0.22f};
    vec3 mortar = {0.62f, 0.60f, 0.55f};
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            int ly;
            vec3 c = inMortar(x, y, w, h, ly, rowH, brickW) ? mortar : brick;
            // faint vertical shading within each brick
            c *= 0.9f + 0.1f * std::sin((float)x * 0.3f);
            int i = (y * w + x) * 4;
            d[i + 0] = toByte(c.x); d[i + 1] = toByte(c.y); d[i + 2] = toByte(c.z); d[i + 3] = 255;
        }
    return upload(w, h, d, GL_LINEAR, GL_REPEAT);
}

GLuint brickNormal(int w, int h, float strength) {
    int rowH = std::max(4, h / 6);
    int brickW = std::max(6, w / 4);
    auto height = [=](int x, int y) -> float {
        int ly;
        return inMortar(x, y, w, h, ly, rowH, brickW) ? 0.0f : 1.0f; // bricks proud of mortar
    };
    return normalFromHeight(w, h, strength, GL_REPEAT, height);
}

GLuint verticalGradientHeight(int w, int h) {
    std::vector<unsigned char> d(w * h * 4);
    for (int y = 0; y < h; ++y) {
        float v = (float)y / (float)(h - 1); // 0 at bottom row (v=0) -> 1 at top
        unsigned char b = toByte(v);
        for (int x = 0; x < w; ++x) {
            int i = (y * w + x) * 4;
            d[i + 0] = b; d[i + 1] = b; d[i + 2] = b; d[i + 3] = 255;
        }
    }
    return upload(w, h, d, GL_LINEAR, GL_CLAMP_TO_EDGE);
}

GLuint domeHeight(int size) {
    std::vector<unsigned char> d(size * size * 4);
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x) {
            float fx = (float)x / (size - 1) * 2.f - 1.f;
            float fy = (float)y / (size - 1) * 2.f - 1.f;
            float r2 = fx * fx + fy * fy;
            float hgt = r2 < 1.f ? std::sqrt(1.f - r2) : 0.f;
            unsigned char b = toByte(hgt);
            int i = (y * size + x) * 4;
            d[i + 0] = b; d[i + 1] = b; d[i + 2] = b; d[i + 3] = r2 < 1.f ? 255 : 0; // alpha clip outside
        }
    return upload(size, size, d, GL_LINEAR, GL_CLAMP_TO_EDGE);
}

GLuint domeNormal(int size, float strength) {
    auto height = [=](int x, int y) -> float {
        float fx = (float)x / (size - 1) * 2.f - 1.f;
        float fy = (float)y / (size - 1) * 2.f - 1.f;
        float r2 = fx * fx + fy * fy;
        return r2 < 1.f ? std::sqrt(1.f - r2) : 0.f;
    };
    return normalFromHeight(size, size, strength, GL_CLAMP_TO_EDGE, height);
}

GLuint radialGlow(int size, vec3 color) {
    std::vector<unsigned char> d(size * size * 4);
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x) {
            float fx = (float)x / (size - 1) * 2.f - 1.f;
            float fy = (float)y / (size - 1) * 2.f - 1.f;
            float r = std::sqrt(fx * fx + fy * fy);
            float glow = std::max(0.f, 1.f - r);
            glow = glow * glow;
            vec3 c = color * glow;
            int i = (y * size + x) * 4;
            d[i + 0] = toByte(c.x); d[i + 1] = toByte(c.y); d[i + 2] = toByte(c.z);
            d[i + 3] = toByte(glow > 0.02f ? 1.f : 0.f);
        }
    return upload(size, size, d, GL_LINEAR, GL_CLAMP_TO_EDGE);
}

} // namespace proctex
