#pragma once
//
// Pass 3 (§8): 2D Radiance Cascades global illumination. Provides noise-free
// ambient / bounced / emissive soft lighting, at a cost roughly independent of
// the number of emitters.
//
// Layout (with base spacing s0 = 2, base directions 2x2 = 4):
//   * Every cascade texture is the SAME size (giW x giH), RGBA16F.
//   * Cascade c partitions that texture into probes spaced s_c = s0*2^c apart,
//     each owning an s_c x s_c block of texels = 4^(c+1) directions.
//   * rgb = radiance gathered in this cascade's interval, a = visibility
//     (1 = ray reached the far end unobstructed -> defer to the upper cascade).
//
// Merge runs top-down with a bilinear probe weight (Bilinear Fix, Osborne &
// Sannikov 2024) to kill ring artifacts. resolve() integrates C0 over direction
// into a per-pixel irradiance (gi) texture for the composite pass.
//
#include "common.hpp"
#include <vector>

class RadianceCascades {
public:
    void init(int giWidth, int giHeight, int numCascades = 6);
    void resize(int giWidth, int giHeight);
    void destroy();

    // sdfTex: R16F distance field (GI res). emissiveTex: RGBA16F emissive (any res).
    void compute(GLuint sdfTex, GLuint emissiveTex);

    GLuint gi() const { return gi_; }          // per-pixel ambient irradiance
    int width()  const { return width_; }
    int height() const { return height_; }
    int cascadeCount() const { return numCascades_; }

    // Tunables (safe to change at runtime).
    float baseInterval = 2.0f;   // d0: C0 ray length in GI pixels
    float gain = 1.2f;           // irradiance multiplier into the composite
    bool  bilinearFix = true;    // toggle the merge bilinear weighting

private:
    void allocTextures();

    GLuint cascadeProgram_ = 0;
    GLuint mergeProgram_   = 0;
    GLuint resolveProgram_ = 0;

    std::vector<GLuint> cascade_; // raw per-interval radiance
    std::vector<GLuint> merged_;  // top-down merged radiance
    GLuint gi_ = 0;

    int width_ = 0;
    int height_ = 0;
    int numCascades_ = 6;
    static constexpr int kBaseSpacing = 2; // s0
};
