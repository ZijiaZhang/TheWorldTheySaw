#pragma once
//
// G-buffer (§5): a screen-space MRT FBO that the geometry pass fills.
//
//   0  RGBA8    rgb = albedo,            a = occluder mask
//   1  RGBA8    rgb = world normal,      a = roughness / material id
//   2  R16F     height / elevation       (the pseudo-3D key channel)
//   3  RGBA16F  rgb = emissive           (fed to Radiance Cascades)
//   depth  D24S8  optional, for §12 z-buffer sorting
//
#include "common.hpp"

class GBuffer {
public:
    void init(int width, int height);
    void resize(int width, int height);
    void destroy();

    // Bind for the geometry pass: FBO + 4 draw buffers + viewport. Does not clear.
    void bindForWriting();
    void clear();

    GLuint albedoMask()  const { return tex_[0]; }
    GLuint normalRough() const { return tex_[1]; }
    GLuint heightTex()   const { return tex_[2]; }
    GLuint emissive()    const { return tex_[3]; }
    GLuint depth()       const { return depthRb_; }
    GLuint fbo()         const { return fbo_; }

    int width()  const { return width_; }
    int height() const { return height_; }

private:
    GLuint fbo_ = 0;
    GLuint tex_[4] = {0, 0, 0, 0};
    GLuint depthRb_ = 0;
    int width_ = 0;
    int height_ = 0;
};
