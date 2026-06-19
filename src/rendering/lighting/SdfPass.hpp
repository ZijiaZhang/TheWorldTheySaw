#pragma once
//
// Pass 2 (§7): build a distance field of the occluders with Jump Flooding (JFA),
// for the Radiance Cascades ray-march. Runs every frame, so it tracks fully
// dynamic scenes. Resolution is the GI resolution (typically half screen).
//
//   seed  : RG32F  nearest occluder pixel coords (ping-pong)
//   sdf   : R16F   distance (in GI pixels) to the nearest occluder
//
#include "common.hpp"

class SdfPass {
public:
    void init(int width, int height);
    void resize(int width, int height);
    void destroy();

    // Seed from the G-buffer occluder mask (GBuffer0.a) and run JFA + distance.
    void generate(GLuint occluderMaskTex);

    GLuint sdf()  const { return sdf_; }
    GLuint seed() const { return seedA_; }
    int width()  const { return width_; }
    int height() const { return height_; }

private:
    GLuint seedProgram_     = 0;
    GLuint jfaProgram_      = 0;
    GLuint distanceProgram_ = 0;

    GLuint seedA_ = 0;
    GLuint seedB_ = 0;
    GLuint sdf_   = 0;

    int width_ = 0;
    int height_ = 0;
};
